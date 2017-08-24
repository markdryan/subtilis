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

#ifndef __SUBTILIS_HASH_TABLE_H
#define __SUBTILIS_HASH_TABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "error.h"

typedef struct subtilis_hashtable_t_ subtilis_hashtable_t;

typedef size_t (*subtilis_hashtable_func_t)(subtilis_hashtable_t *,
					    const void *);
typedef bool (*subtilis_hashtable_equal_t)(const void *, const void *);
typedef void (*subtilis_hashtable_free_t)(void *);

typedef struct subtilis_hashtable_node_t_ subtilis_hashtable_node_t;

struct subtilis_hashtable_t_ {
	subtilis_hashtable_node_t **buckets;
	size_t elements;
	size_t num_buckets;
	subtilis_hashtable_func_t hash_func;
	subtilis_hashtable_equal_t equal_func;
	subtilis_hashtable_free_t free_key;
	subtilis_hashtable_free_t free_value;
};

subtilis_hashtable_t *subtilis_hashtable_new(size_t num_buckets,
					     subtilis_hashtable_func_t h_func,
					     subtilis_hashtable_equal_t eq_func,
					     subtilis_hashtable_free_t free_k,
					     subtilis_hashtable_free_t free_v,
					     subtilis_error_t *err);

void *subtilis_hashtable_find(subtilis_hashtable_t *h, const void *k);
void subtilis_hashtable_delete(subtilis_hashtable_t *h);
size_t subtilis_hashtable_perfection(subtilis_hashtable_t *h);
bool subtilis_hashtable_insert(subtilis_hashtable_t *h, void *k, void *v,
			       subtilis_error_t *err);
bool subtilis_hashtable_remove(subtilis_hashtable_t *h, const void *k);
void subtilis_hashtable_reset(subtilis_hashtable_t *h);
size_t subtilis_hashtable_sdbm(subtilis_hashtable_t *h, const void *k);
size_t subtilis_hashtable_djb2(subtilis_hashtable_t *h, const void *k);
bool subtilis_hashtable_string_equal(const void *k1, const void *k2);

#endif
