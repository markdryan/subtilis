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

#include "config.h"
#include "symbol_table.h"

subtilis_symbol_table_t *subtilis_symbol_table_new(subtilis_error_t *err)
{
	subtilis_hashtable_t *h;
	subtilis_symbol_table_t *st;

	h = subtilis_hashtable_new(
	    SUBTILIS_CONFIG_ST_SIZE, subtilis_hashtable_djb2,
	    subtilis_hashtable_string_equal, free, free, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	st = malloc(sizeof(*st));
	if (!st) {
		subtilis_error_set_oom(err);
		goto on_error;
	}
	st->h = h;
	st->allocated = 0;

	return st;

on_error:
	subtilis_hashtable_delete(h);

	return NULL;
}

void subtilis_symbol_table_delete(subtilis_symbol_table_t *st)
{
	if (!st)
		return;
	subtilis_hashtable_delete(st->h);
	free(st);
}

const subtilis_symbol_t *
subtilis_symbol_table_insert(subtilis_symbol_table_t *st, const char *key,
			     subtilis_type_t id_type, subtilis_error_t *err)
{
	subtilis_symbol_t *sym;
	char *key_dup = NULL;

	sym = subtilis_hashtable_find(st->h, key);
	if (sym)
		return sym;

	key_dup = malloc(strlen(key) + 1);
	if (!key_dup)
		goto on_error;
	(void)strcpy(key_dup, key);

	sym = malloc(sizeof(*sym));
	if (!sym)
		goto on_error;

	// TODO: There's target specific information here.  We assume
	// pointers are 32 bit which they may not be.  We also need to
	// add support for other integer types.

	sym->loc = st->allocated;
	switch (id_type) {
	case SUBTILIS_TYPE_REAL:
		sym->size = 8;
		break;
	case SUBTILIS_TYPE_INTEGER:
		sym->size = 4;
		break;
	case SUBTILIS_TYPE_STRING:
		sym->size = 8;
		break;
	}

	sym->t = id_type;

	(void)subtilis_hashtable_insert(st->h, key_dup, sym, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	st->allocated += sym->size;

	return sym;

on_error:
	subtilis_error_set_oom(err);

	free(key_dup);
	free(sym);
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
	subtilis_hashtable_reset(st->h);
	st->allocated = 0;
}
