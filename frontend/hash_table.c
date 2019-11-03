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

#include "hash_table.h"

struct subtilis_hashtable_node_t_ {
	subtilis_hashtable_node_t *next;
	void *key;
	void *value;
};

static void prv_free_bucket(subtilis_hashtable_t *h, size_t bucket)
{
	subtilis_hashtable_node_t *next;
	subtilis_hashtable_node_t *node = h->buckets[bucket];

	while (node) {
		next = node->next;
		if (h->free_key)
			h->free_key(node->key);
		if (h->free_value)
			h->free_value(node->value);
		free(node);
		node = next;
	}
}

subtilis_hashtable_t *subtilis_hashtable_new(size_t num_buckets,
					     subtilis_hashtable_func_t h_func,
					     subtilis_hashtable_equal_t eq_func,
					     subtilis_hashtable_free_t free_k,
					     subtilis_hashtable_free_t free_v,
					     subtilis_error_t *err)
{
	subtilis_hashtable_t *h = malloc(sizeof(*h));

	if (!h)
		goto on_error;

	h->buckets = calloc(num_buckets, sizeof(subtilis_hashtable_node_t *));
	if (!h->buckets)
		goto on_error;

	h->num_buckets = num_buckets;
	h->elements = 0;
	h->hash_func = h_func;
	h->equal_func = eq_func;
	h->free_key = free_k;
	h->free_value = free_v;

	return h;

on_error:

	subtilis_error_set_oom(err);
	free(h);
	return NULL;
}

void subtilis_hashtable_delete(subtilis_hashtable_t *h)
{
	size_t i;

	if (!h)
		return;

	if (h->buckets) {
		for (i = 0; i < h->num_buckets; i++)
			prv_free_bucket(h, i);
		free(h->buckets);
	}
	free(h);
}

static subtilis_hashtable_node_t *prv_find_node(subtilis_hashtable_t *h,
						const void *key, size_t hash)
{
	subtilis_hashtable_node_t *n;

	n = h->buckets[hash];
	while (n && !h->equal_func(key, n->key))
		n = n->next;

	if (!n)
		return NULL;

	return n;
}

void *subtilis_hashtable_find(subtilis_hashtable_t *h, const void *k)
{
	size_t hash = h->hash_func(h, k);
	subtilis_hashtable_node_t *n = prv_find_node(h, k, hash);

	if (!n)
		return NULL;

	return n->value;
}

bool subtilis_hashtable_insert(subtilis_hashtable_t *h, void *k, void *v,
			       subtilis_error_t *err)
{
	subtilis_hashtable_node_t *old_n;
	size_t hash = h->hash_func(h, k);
	subtilis_hashtable_node_t *n = prv_find_node(h, k, hash);

	if (n)
		return false;

	n = malloc(sizeof(*n));
	if (!n) {
		subtilis_error_set_oom(err);
		return false;
	}

	old_n = h->buckets[hash];
	n->key = k;
	n->value = v;
	n->next = old_n;
	h->buckets[hash] = n;
	h->elements++;

	return true;
}

bool subtilis_hashtable_remove(subtilis_hashtable_t *h, const void *k)
{
	subtilis_hashtable_node_t *n;
	subtilis_hashtable_node_t *old_n = NULL;
	size_t hash = h->hash_func(h, k);

	n = h->buckets[hash];
	while (n && !h->equal_func(k, n->key)) {
		old_n = n;
		n = n->next;
	}

	if (!n)
		return false;

	if (h->free_key)
		h->free_key(n->key);
	if (h->free_value)
		h->free_value(n->value);
	if (old_n)
		old_n->next = n->next;
	else
		h->buckets[hash] = n->next;
	free(n);

	h->elements--;

	return n;
}

void subtilis_hashtable_reset(subtilis_hashtable_t *h)
{
	size_t i;

	for (i = 0; i < h->num_buckets; i++) {
		prv_free_bucket(h, i);
		h->buckets[i] = NULL;
	}
}

size_t subtilis_hashtable_perfection(subtilis_hashtable_t *h)
{
	size_t i;
	subtilis_hashtable_node_t *n;
	size_t percentage;
	size_t list_count = 0;
	size_t duplicates = 0;

	if (h->elements == 0)
		return 100;

	for (i = 0; i < h->num_buckets; i++) {
		n = h->buckets[i];
		while (n) {
			++list_count;
			n = n->next;
		}

		if (list_count > 1)
			duplicates += list_count - 1;

		list_count = 0;
	}

	percentage = 100 - ((duplicates * 100) / h->elements);

	return percentage;
}

size_t subtilis_hashtable_sdbm(subtilis_hashtable_t *h, const void *k)
{
	const char *ch;
	size_t hash = 0;

	for (ch = (const char *)k; *ch; ch++)
		hash = *ch + (hash << 6) + (hash << 16) - hash;

	return hash % h->num_buckets;
}

size_t subtilis_hashtable_djb2(subtilis_hashtable_t *h, const void *k)
{
	const char *ch;
	size_t hash = 5381;

	for (ch = (const char *)k; *ch; ch++)
		hash = ((hash << 5) + hash) + *ch;

	return hash % h->num_buckets;
}

bool subtilis_hashtable_string_equal(const void *k1, const void *k2)
{
	return strcmp((const char *)k1, (const char *)k2) == 0;
}
