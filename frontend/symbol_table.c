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

static void prv_symbol_delete(void *s)
{
	subtilis_symbol_t *sym = s;

	if (!sym)
		return;

	subtilis_type_free(&sym->t);
	free(sym);
}

subtilis_symbol_table_t *subtilis_symbol_table_new(subtilis_error_t *err)
{
	subtilis_hashtable_t *h;
	subtilis_symbol_table_t *st;

	h = subtilis_hashtable_new(
	    SUBTILIS_CONFIG_ST_SIZE, subtilis_hashtable_djb2,
	    subtilis_hashtable_string_equal, free, prv_symbol_delete, err);
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
	size_t i;

	if (!st)
		return;
	for (i = 0; i < SUBTILIS_SYMBOL_MAX_LEVELS; i++)
		free(st->levels[i].symbols);
	subtilis_hashtable_delete(st->h);
	free(st);
}

static void prv_ensure_symbol_level(subtilis_symbol_table_t *st,
				    size_t level_idx, subtilis_error_t *err)
{
	size_t new_size;
	subtilis_symbol_level_t *level = &st->levels[level_idx];
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
	level->size = 0;
	st->level--;
}

static subtilis_symbol_t *prv_symbol_new(const char *key, size_t loc,
					 const subtilis_type_t *id_type,
					 size_t size, bool is_reg, bool no_rc,
					 subtilis_error_t *err)
{
	subtilis_symbol_t *sym;

	sym = malloc(sizeof(*sym));
	if (!sym) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	subtilis_type_init_copy(&sym->t, id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	sym->size = size;
	sym->key = key;
	sym->loc = loc;
	sym->is_reg = is_reg;
	sym->no_rc = no_rc;

	return sym;

on_error:
	free(sym);

	return NULL;
}

static const subtilis_symbol_t *
prv_symbol_table_insert(subtilis_symbol_table_t *st, const char *key,
			const subtilis_type_t *id_type, size_t size,
			size_t level, bool no_rc, subtilis_error_t *err)

{
	subtilis_symbol_t *sym;
	size_t align_bytes;
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

	/*
	 * TODO:  This can leave holes in our global data.  It would
	 * be good to keep track of those holes so we could plug them
	 * with bytes variables.
	 */

	align_bytes = st->allocated % size;
	if (align_bytes > 0)
		st->allocated += size - align_bytes;

	sym = prv_symbol_new(key_dup, st->allocated, id_type, size, false,
			     no_rc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	prv_ensure_symbol_level(st, level, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	st->levels[level].symbols[st->levels[level].size++] = sym;

	(void)subtilis_hashtable_insert(st->h, key_dup, sym, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	st->allocated += sym->size;
	if (st->allocated > st->max_allocated)
		st->max_allocated = st->allocated;

	return sym;

on_error:
	free(key_dup);
	prv_symbol_delete(sym);
	return NULL;
}

const subtilis_symbol_t *
subtilis_symbol_table_insert(subtilis_symbol_table_t *st, const char *key,
			     const subtilis_type_t *id_type,
			     subtilis_error_t *err)
{
	size_t size;

	size = subtilis_type_if_size(id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return prv_symbol_table_insert(st, key, id_type, size, st->level, false,
				       err);
}

const subtilis_symbol_t *
subtilis_symbol_table_insert_no_rc(subtilis_symbol_table_t *st, const char *key,
				   const subtilis_type_t *id_type,
				   subtilis_error_t *err)
{
	size_t size;

	size = subtilis_type_if_size(id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	return prv_symbol_table_insert(st, key, id_type, size, st->level, true,
				       err);
}

const subtilis_symbol_t *
subtilis_symbol_table_create_local_buf(subtilis_symbol_table_t *st, size_t size,
				       subtilis_error_t *err)
{
	char buf[64];

	/*
	 * Everything on the stack needs to be 32 bit word aligned.
	 */

	if (size & 3) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	/*
	 * Subtilis identifiers cannot start with a number so there'll be no
	 * clash here.
	 */

	sprintf(buf, "%zu", st->tmp_count++);
	return prv_symbol_table_insert(st, buf, &subtilis_type_local_buffer,
				       size, 0, false, err);
}

const subtilis_symbol_t *
subtilis_symbol_table_create_named_local_buf(subtilis_symbol_table_t *st,
					     const char *name, size_t size,
					     subtilis_error_t *err)
{
	/*
	 * Everything on the stack needs to be 32 bit word aligned.
	 */

	if (size & 3) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	return prv_symbol_table_insert(st, name, &subtilis_type_local_buffer,
				       size, 0, false, err);
}

const subtilis_symbol_t *
subtilis_symbol_table_insert_tmp(subtilis_symbol_table_t *st,
				 const subtilis_type_t *id_type,
				 char **tmp_name, subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	char *name = NULL;
	char buf[64];

	/*
	 * Subtilis identifiers cannot start with a number so there'll be no
	 * clash here.
	 */

	sprintf(buf, "%zu", st->tmp_count++);
	if (tmp_name) {
		name = malloc(strlen(buf) + 1);
		if (!name) {
			subtilis_error_set_oom(err);
			return NULL;
		}
		strcpy(name, buf);
	}

	s = subtilis_symbol_table_insert(st, buf, id_type, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		free(name);
		return NULL;
	}

	if (tmp_name)
		*tmp_name = name;

	return s;
}

/* clang-format off */
const subtilis_symbol_t *subtilis_symbol_table_promote_tmp(
	subtilis_symbol_table_t *st, const subtilis_type_t *id_type,
	const char *tmp_name, const char *new_name, subtilis_error_t *err)
/* clang-format on */

{
	subtilis_symbol_t *s;
	char *name = NULL;

	s = subtilis_hashtable_extract(st->h, tmp_name);
	if (!s) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	if (!subtilis_type_eq(id_type, &s->t)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	name = malloc(strlen(new_name) + 1);
	if (!name) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	strcpy(name, new_name);
	s->key = name;

	(void)subtilis_hashtable_insert(st->h, name, s, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		free(s);
		free(name);
		return NULL;
	}

	return s;
}

const subtilis_symbol_t *
subtilis_symbol_table_insert_reg(subtilis_symbol_table_t *st, const char *key,
				 const subtilis_type_t *id_type, size_t reg_num,
				 subtilis_error_t *err)
{
	subtilis_symbol_t *sym;
	size_t size;
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

	size = subtilis_type_if_size(id_type, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	sym = prv_symbol_new(key_dup, reg_num, id_type, size, true, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	prv_ensure_symbol_level(st, st->level, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	st->levels[st->level].symbols[st->levels[st->level].size++] = sym;

	(void)subtilis_hashtable_insert(st->h, key_dup, sym, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return sym;

on_error:
	free(key_dup);
	prv_symbol_delete(sym);
	return NULL;
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
