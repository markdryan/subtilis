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

#ifndef __SUBTILIS_SYMBOL_TABLE_H
#define __SUBTILIS_SYMBOL_TABLE_H

#include "../common/lexer.h"
#include "hash_table.h"

#define SUBTILIS_SYMBOL_MAX_LEVELS 32

struct subtilis_symbol_t_ {
	size_t loc;
	subtilis_type_t t;     // owned by hash table
	subtilis_type_t sub_t; // owned by hash table
	const char *key;       // owned by hash table
	size_t size;
	bool is_reg;
	bool no_rc;
};

typedef struct subtilis_symbol_t_ subtilis_symbol_t;

struct subtilis_symbol_level_t_ {
	size_t size;
	size_t max_size;
	const subtilis_symbol_t **symbols;
};

typedef struct subtilis_symbol_level_t_ subtilis_symbol_level_t;

struct subtilis_symbol_table_t_ {
	subtilis_symbol_level_t levels[SUBTILIS_SYMBOL_MAX_LEVELS];
	subtilis_hashtable_t *h;
	size_t allocated;
	size_t max_allocated;
	size_t level;
	size_t tmp_count;
};

typedef struct subtilis_symbol_table_t_ subtilis_symbol_table_t;

subtilis_symbol_table_t *subtilis_symbol_table_new(subtilis_error_t *err);
void subtilis_symbol_table_delete(subtilis_symbol_table_t *st);
void subtilis_symbol_table_level_up(subtilis_symbol_table_t *st,
				    subtilis_lexer_t *l, subtilis_error_t *err);
void subtilis_symbol_table_level_down(subtilis_symbol_table_t *st,
				      subtilis_error_t *err);
const subtilis_symbol_t *
subtilis_symbol_table_insert(subtilis_symbol_table_t *st, const char *key,
			     const subtilis_type_t *id_type,
			     subtilis_error_t *err);
const subtilis_symbol_t *
subtilis_symbol_table_insert_no_rc(subtilis_symbol_table_t *st, const char *key,
				   const subtilis_type_t *id_type,
				   subtilis_error_t *err);

const subtilis_symbol_t *
subtilis_symbol_table_insert_tmp(subtilis_symbol_table_t *st,
				 const subtilis_type_t *id_type,
				 char **tmp_name, subtilis_error_t *err);
void subtilis_symbol_table_insert_type(subtilis_symbol_table_t *st,
				       const char *name,
				       const subtilis_type_t *type,
				       subtilis_error_t *err);

/* clang-format off */
const subtilis_symbol_t *subtilis_symbol_table_promote_tmp(
	subtilis_symbol_table_t *st, const subtilis_type_t *id_type,
	const char *tmp_name, const char *new_name, subtilis_error_t *err);
/* clang-format on */

const subtilis_symbol_t *
subtilis_symbol_table_create_local_buf(subtilis_symbol_table_t *st, size_t size,
				       subtilis_error_t *err);
const subtilis_symbol_t *
subtilis_symbol_table_create_named_local_buf(subtilis_symbol_table_t *st,
					     const char *name, size_t size,
					     subtilis_error_t *err);

const subtilis_symbol_t *
subtilis_symbol_table_insert_reg(subtilis_symbol_table_t *st, const char *key,
				 const subtilis_type_t *id_type, size_t reg_num,
				 subtilis_error_t *err);
bool subtilis_symbol_table_remove(subtilis_symbol_table_t *st, const char *key);
const subtilis_symbol_t *
subtilis_symbol_table_lookup(subtilis_symbol_table_t *st, const char *key);
void subtilis_symbol_table_reset(subtilis_symbol_table_t *st);

#endif
