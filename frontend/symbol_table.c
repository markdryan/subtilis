/*
 * Copyright (c) 2017 Mark Ryan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>

#include "../common/config.h"
#include "symbol_table.h"
#include "type_if.h"

subtilis_symbol_table_t *subtilis_symbol_table_new(subtilis_error_t *err)
{
	subtilis_hashtable_t *h;
	subtilis_symbol_table_t *st;

	h = subtilis_hashtable_new(
	    SUBTILIS_CONFIG_ST_SIZE, subtilis_hashtable_djb2,
	    subtilis_hashtable_string_equal, free, free, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	st = calloc(sizeof(*st), 1);
	if (!st) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	st->h = h;
	st->allocated = 0;
	st->max_allocated = 0;
	st->level = 0;

	return st;

on_error:
	subtilis_hashtable_delete(h);

	return NULL;
}

void subtilis_symbol_table_delete(subtilis_symbol_table_t *st)
{
	if (!st)
		return;
	free(st->levels[0].symbols);
	subtilis_hashtable_delete(st->h);
	free(st);
}

static void prv_ensure_symbol_level(subtilis_symbol_table_t *st,
				    subtilis_error_t *err)
{
	size_t new_size;
	subtilis_symbol_level_t *level = &st->levels[st->level];
	const subtilis_symbol_t **new_symbols;

	if (level->size < level->max_size)
		return;

	new_size = level->size + SUBTILIS_SYMBOL_MAX_LEVELS;
	new_symbols = realloc(level->symbols, new_size * sizeof(*new_symbols));
	if (!new_symbols) {
		subtilis_error_set_oom(err);
		return;
	}
	level->symbols = new_symbols;
	level->max_size = new_size;
}

void subtilis_symbol_table_level_up(subtilis_symbol_table_t *st,
				    subtilis_lexer_t *l, subtilis_error_t *err)
{
	if (st->level >= SUBTILIS_SYMBOL_MAX_LEVELS) {
		subtilis_error_set_too_many_blocks(err, l->stream->name,
						   l->line);
		return;
	}

	st->level++;
}

void subtilis_symbol_table_level_down(subtilis_symbol_table_t *st,
				      subtilis_error_t *err)
{
	size_t i;
	subtilis_symbol_level_t *level;

	if (st->level == 0) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	level = &st->levels[st->level];
	for (i = 0; i < level->size; i++)
		subtilis_symbol_table_remove(st, level->symbols[i]->key);
	free(level->symbols);
	level->symbols = NULL;
	level->size = 0;
	st->level--;
}

static subtilis_symbol_t *prv_symbol_new(const char *key, size_t loc,
					 const subtilis_type_t *id_type,
					 bool is_reg, subtilis_error_t *err)
{
	subtilis_symbol_t *sym;

	sym = malloc(sizeof(*sym));
	if (!sym) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	sym->size = subtilis_type_if_size(id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	sym->key = key;
	sym->t = *id_type;
	sym->loc = loc;
	sym->is_reg = is_reg;

	return sym;

on_error:
	free(sym);

	return NULL;
}

const subtilis_symbol_t *
subtilis_symbol_table_insert(subtilis_symbol_table_t *st, const char *key,
			     const subtilis_type_t *id_type,
			     subtilis_error_t *err)
{
	subtilis_symbol_t *sym;
	char *key_dup = NULL;

	sym = subtilis_hashtable_find(st->h, key);
	if (sym)
		return sym;

	key_dup = malloc(strlen(key) + 1);
	if (!key_dup) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	(void)strcpy(key_dup, key);

	sym = prv_symbol_new(key_dup, st->allocated, id_type, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	prv_ensure_symbol_level(st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	st->levels[st->level].symbols[st->levels[st->level].size++] = sym;

	(void)subtilis_hashtable_insert(st->h, key_dup, sym, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	st->allocated += sym->size;
	if (st->allocated > st->max_allocated)
		st->max_allocated = st->allocated;

	return sym;

on_error:
	free(key_dup);
	free(sym);
	return NULL;
}

const subtilis_symbol_t *
subtilis_symbol_table_insert_reg(subtilis_symbol_table_t *st, const char *key,
				 const subtilis_type_t *id_type, size_t reg_num,
				 subtilis_error_t *err)
{
	subtilis_symbol_t *sym;
	char *key_dup = NULL;

	sym = subtilis_hashtable_find(st->h, key);
	if (sym)
		return sym;

	key_dup = malloc(strlen(key) + 1);
	if (!key_dup) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	(void)strcpy(key_dup, key);

	sym = prv_symbol_new(key_dup, reg_num, id_type, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	prv_ensure_symbol_level(st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	st->levels[st->level].symbols[st->levels[st->level].size++] = sym;

	(void)subtilis_hashtable_insert(st->h, key_dup, sym, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return sym;

on_error:
	free(key_dup);
	free(sym);
	return NULL;
}

const subtilis_symbol_t *
subtilis_symbol_table_block_param(subtilis_symbol_table_t *st, const char *key,
				  const subtilis_type_t *type,
				  size_t source_num, subtilis_error_t *err)
{
	subtilis_symbol_t *s;

	/* I know, but we know it's not const */

	s = (subtilis_symbol_t *)subtilis_symbol_table_insert(st, key, type,
							      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	s->source_reg = source_num;

	return s;
}

bool subtilis_symbol_table_remove(subtilis_symbol_table_t *st, const char *key)
{
	return subtilis_hashtable_remove(st->h, key);
}

const subtilis_symbol_t *
subtilis_symbol_table_lookup(subtilis_symbol_table_t *st, const char *key)
{
	return subtilis_hashtable_find(st->h, key);
}

void subtilis_symbol_table_reset(subtilis_symbol_table_t *st)
{
	st->levels[0].size = 0;
	subtilis_hashtable_reset(st->h);
	st->allocated = 0;
	st->max_allocated = 0;
	st->level = 0;
}
