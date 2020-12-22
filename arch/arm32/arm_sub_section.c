/*
 * Copyright (c) 2018 Mark Ryan
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

#include "arm_reg_alloc.h"
#include "arm_sub_section.h"

void subtilis_arm_subsections_init(subtilis_arm_subsections_t *sss)
{
	sss->sub_sections = NULL;
	sss->count = 0;
	sss->max_count = 0;
	sss->ss_link_map = NULL;
	sss->link_count = 0;
	sss->max_links = 0;
	subtilis_bitset_init(&sss->int_save);
	subtilis_bitset_init(&sss->real_save);
}

static size_t prv_add_new_sub_section(subtilis_arm_subsections_t *sss,
				      subtilis_arm_section_t *arm_s,
				      size_t start, subtilis_error_t *err)
{
	subtilis_arm_ss_t *ss;
	size_t new_max;
	subtilis_arm_ss_t *new_ss;
	size_t ptr;

	if (sss->count == sss->max_count) {
		new_max = sss->max_count + SUBTILIS_CONFIG_SSS_GRAN;
		new_ss = realloc(sss->sub_sections,
				 new_max * sizeof(*sss->sub_sections));
		if (!new_ss) {
			subtilis_error_set_oom(err);
			return SIZE_MAX;
		}
		sss->sub_sections = new_ss;
		sss->max_count = new_max;
	}

	ptr = sss->count;
	ss = &sss->sub_sections[sss->count++];
	memset(ss, 0, sizeof(*ss));

	subtilis_bitset_init(&ss->int_inputs);
	subtilis_bitset_init(&ss->real_inputs);
	ss->start = start;
	ss->end = 0;
	ss->num_links = 0;

	return ptr;
}

static void prv_sss_link_map_insert(subtilis_arm_subsections_t *sss,
				    size_t ss_ptr, size_t label,
				    subtilis_error_t *err)
{
	size_t new_max;
	size_t i;
	size_t *new_link_map;

	if (label >= sss->max_links) {
		new_max = label + SUBTILIS_CONFIG_SSS_GRAN;
		new_link_map =
		    realloc(sss->ss_link_map, new_max * sizeof(size_t));
		if (!new_link_map) {
			subtilis_error_set_oom(err);
			return;
		}
		for (i = sss->max_links; i < new_max; i++)
			new_link_map[i] = SIZE_MAX;
		sss->ss_link_map = new_link_map;
		sss->max_links = new_max;
	}
	sss->ss_link_map[label] = ss_ptr;
	if (label >= sss->link_count)
		sss->link_count = label + 1;
}

static void prv_add_link(subtilis_arm_ss_t *ss, subtilis_arm_section_t *arm_s,
			 size_t ptr, size_t label, subtilis_error_t *err)
{
	subtilis_arm_ss_link_t *first_link;
	subtilis_arm_ss_link_t *link;
	subtilis_arm_op_t *from;
	subtilis_regs_used_virt_t regs_used;
	size_t max_int_regs;
	size_t max_real_regs;
	subtilis_arm_op_t *to;

	if (ss->num_links >= 2) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	to = &arm_s->op_pool->ops[ptr];
	subtilis_arm_section_max_regs(arm_s, &max_int_regs, &max_real_regs);
	subtilis_regs_used_virt_init(&regs_used);

	link = &ss->links[ss->num_links++];
	subtilis_bitset_init(&link->int_outputs);
	subtilis_bitset_init(&link->real_outputs);
	subtilis_bitset_init(&link->int_save);
	subtilis_bitset_init(&link->real_save);

	/*
	 * If the basic block has two links, the block essentially ends with
	 * conditional branch.  This branch will be followed by a label but
	 * this label doesn't affect the inputs of outputs.  Thus the inputs
	 * and outputs for both links will be identical.  There's no need to
	 * compute them twice, particularly as this is one of the most expensive
	 * parts of the compilation.
	 */

	if (ss->num_links == 1) {
		from = &arm_s->op_pool->ops[ss->start];
		subtilis_arm_regs_used_before_from_tov(
		    arm_s, from, to, max_int_regs, max_real_regs, &regs_used,
		    err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_bitset_claim(&link->int_outputs, &regs_used.int_regs);
		subtilis_bitset_claim(&link->real_outputs,
				      &regs_used.real_regs);
	} else {
		first_link = &ss->links[0];
		subtilis_bitset_or(&link->int_outputs, &first_link->int_outputs,
				   err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_bitset_or(&link->real_outputs,
				   &first_link->real_outputs, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	link->link = label;
	link->op = ptr;

cleanup:

	subtilis_regs_used_virt_free(&regs_used);
}

static void prv_finalize_sub_section(subtilis_arm_ss_t *ss,
				     subtilis_arm_section_t *arm_s, size_t end,
				     size_t count, subtilis_error_t *err)
{
	subtilis_arm_op_t *from;
	subtilis_arm_op_t *op;
	subtilis_regs_used_virt_t regs_used;
	size_t max_int_regs;
	size_t max_real_regs;
	size_t ptr;

	subtilis_arm_section_max_regs(arm_s, &max_int_regs, &max_real_regs);
	subtilis_regs_used_virt_init(&regs_used);

	ss->end = end;
	ss->size = count;
	from = &arm_s->op_pool->ops[ss->start];
	op = &arm_s->op_pool->ops[ss->end];

	subtilis_arm_regs_used_afterv(arm_s, from, op, max_int_regs,
				      max_real_regs, count, &regs_used, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_bitset_claim(&ss->int_inputs, &regs_used.int_regs);
	subtilis_bitset_claim(&ss->real_inputs, &regs_used.real_regs);

	if (((op->type != SUBTILIS_ARM_OP_INSTR) ||
	     (op->op.instr.type != SUBTILIS_ARM_INSTR_B) ||
	     (op->op.instr.operands.br.ccode != SUBTILIS_ARM_CCODE_AL)) &&
	    (op->next != SIZE_MAX)) {
		ptr = op->next;
		op = &arm_s->op_pool->ops[ptr];
		if (op->type == SUBTILIS_ARM_OP_LABEL)
			prv_add_link(ss, arm_s, ptr, op->op.label, err);
	}

cleanup:

	subtilis_regs_used_virt_free(&regs_used);
}

static void prv_add_args(subtilis_arm_ss_t *ss, subtilis_arm_section_t *arm_s,
			 subtilis_error_t *err)
{
	size_t i;
	size_t j;
	subtilis_arm_ss_link_t *link;

	for (j = 0; j < ss->num_links; j++) {
		link = &ss->links[j];
		for (i = 0; i < arm_s->stype->int_regs; i++) {
			subtilis_bitset_set(&link->int_outputs,
					    SUBTILIS_ARM_INT_VIRT_REG_START + i,
					    err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
		for (i = 0; i < arm_s->stype->fp_regs; i++) {
			subtilis_bitset_set(&link->real_outputs,
					    arm_s->fp_if->max_regs + i, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}

static void
prv_visit_subsection(subtilis_arm_subsections_t *sss, subtilis_arm_ss_t *ss,
		     subtilis_arm_ss_link_t *start, subtilis_bitset_t *int_save,
		     subtilis_bitset_t *real_save, subtilis_bitset_t *visited,
		     subtilis_error_t *err)
{
	subtilis_bitset_t int_scratch;
	subtilis_bitset_t real_scratch;
	subtilis_arm_ss_t *next;
	size_t i;
	size_t link;
	size_t label;

	subtilis_bitset_init(&int_scratch);
	subtilis_bitset_init(&real_scratch);

	subtilis_bitset_or(&int_scratch, &start->int_outputs, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_bitset_or(&real_scratch, &start->real_outputs, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_bitset_and(&int_scratch, &ss->int_inputs);
	subtilis_bitset_and(&real_scratch, &ss->real_inputs);
	subtilis_bitset_or(int_save, &int_scratch, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_bitset_or(real_save, &real_scratch, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 0; i < ss->num_links; i++) {
		link = ss->links[i].link;
		if (link >= sss->link_count) {
			subtilis_error_set_assertion_failed(err);
			goto cleanup;
		}
		if (subtilis_bitset_isset(visited, link))
			continue;
		subtilis_bitset_set(visited, link, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		label = sss->ss_link_map[link];
		if (label == SIZE_MAX) {
			subtilis_error_set_assertion_failed(err);
			goto cleanup;
		}
		next = &sss->sub_sections[label];
		prv_visit_subsection(sss, next, start, int_save, real_save,
				     visited, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

cleanup:

	subtilis_bitset_free(&int_scratch);
	subtilis_bitset_free(&real_scratch);
}

static void prv_link_must_save(subtilis_arm_subsections_t *sss,
			       subtilis_arm_ss_t *ss, size_t link,
			       subtilis_error_t *err)
{
	subtilis_bitset_t visited;
	subtilis_arm_ss_link_t *start;
	subtilis_arm_ss_t *next;
	size_t label;

	if (link >= ss->num_links) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_bitset_init(&visited);

	start = &ss->links[link];
	if (start->link >= sss->link_count) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	subtilis_bitset_set(&visited, start->link, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	label = sss->ss_link_map[start->link];
	if (label == SIZE_MAX) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}
	next = &sss->sub_sections[label];
	prv_visit_subsection(sss, next, start, &start->int_save,
			     &start->real_save, &visited, err);

	subtilis_bitset_or(&sss->int_save, &start->int_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_bitset_or(&sss->real_save, &start->real_save, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

cleanup:

	subtilis_bitset_free(&visited);
}

static void prv_compute_must_save(subtilis_arm_subsections_t *sss,
				  subtilis_error_t *err)
{
	size_t i;
	size_t j;
	subtilis_arm_ss_t *ss;

	for (i = 0; i < sss->count; i++) {
		ss = &sss->sub_sections[i];
		for (j = 0; j < ss->num_links; j++) {
			prv_link_must_save(sss, ss, j, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		}
	}
}

void subtilis_arm_subsections_calculate(subtilis_arm_subsections_t *sss,
					subtilis_arm_section_t *arm_s,
					subtilis_error_t *err)
{
	size_t ptr;
	subtilis_arm_ss_t *ss;
	subtilis_arm_op_t *op;
	subtilis_arm_op_t *next;
	size_t count;
	size_t label;
	size_t ss_ptr;

	if (arm_s->first_op == arm_s->last_op) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	ptr = arm_s->first_op;
	ss_ptr = prv_add_new_sub_section(sss, arm_s, ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	ss = &sss->sub_sections[ss_ptr];

	count = 0;
	while (ptr != SIZE_MAX) {
		op = &arm_s->op_pool->ops[ptr];
		if (op->type == SUBTILIS_ARM_OP_LABEL) {
			prv_finalize_sub_section(ss, arm_s, op->prev, count,
						 err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			count = 0;
			ss_ptr = prv_add_new_sub_section(sss, arm_s, ptr, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			ss = &sss->sub_sections[ss_ptr];
			label = op->op.label;
			if (label >= arm_s->label_counter) {
				subtilis_error_set_assertion_failed(err);
				return;
			}
			prv_sss_link_map_insert(sss, ss_ptr, label, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
		} else if ((op->type == SUBTILIS_ARM_OP_INSTR) &&
			   (op->op.instr.type == SUBTILIS_ARM_INSTR_B) &&
			   (!op->op.instr.operands.br.link) &&
			   (op->op.instr.operands.br.ccode !=
			    SUBTILIS_ARM_CCODE_NV)) {
			prv_add_link(ss, arm_s, ptr,
				     op->op.instr.operands.br.target.label,
				     err);
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (op->next != SIZE_MAX) {
				next = &arm_s->op_pool->ops[op->next];
				if (next->type != SUBTILIS_ARM_OP_LABEL) {
					subtilis_arm_section_insert_label(
					    arm_s, arm_s->label_counter, next,
					    err);
					if (err->type != SUBTILIS_ERROR_OK)
						return;
				}
			}
		}

		count++;
		if (ptr == arm_s->last_op)
			break;

		ptr = op->next;
	}

	if (count > 0) {
		prv_finalize_sub_section(ss, arm_s, arm_s->last_op, count, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}

	/*
	 * We need to update the outputs for the first subsection if
	 * it has any arguments.
	 */

	prv_add_args(&sss->sub_sections[0], arm_s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_compute_must_save(sss, err);
}

static void prv_free_sub_section(subtilis_arm_ss_t *ss)
{
	size_t i;

	for (i = 0; i < ss->num_links; i++) {
		subtilis_bitset_free(&ss->links[i].int_outputs);
		subtilis_bitset_free(&ss->links[i].real_outputs);
		subtilis_bitset_free(&ss->links[i].int_save);
		subtilis_bitset_free(&ss->links[i].real_save);
	}
	subtilis_bitset_free(&ss->int_inputs);
	subtilis_bitset_free(&ss->real_inputs);
}

void subtilis_arm_subsections_free(subtilis_arm_subsections_t *sss)
{
	size_t i;

	subtilis_bitset_free(&sss->int_save);
	subtilis_bitset_free(&sss->real_save);

	free(sss->ss_link_map);
	if (!sss->sub_sections)
		return;

	for (i = 0; i < sss->count; i++)
		prv_free_sub_section(&sss->sub_sections[i]);
	free(sss->sub_sections);
}

void subtilis_arm_subsections_must_save(subtilis_arm_subsections_t *sss,
					subtilis_arm_ss_t *ss, size_t link,
					subtilis_bitset_t *int_save,
					subtilis_bitset_t *real_save,
					subtilis_error_t *err)
{
	subtilis_bitset_t visited;
	subtilis_arm_ss_link_t *start;
	subtilis_arm_ss_t *next;

	if (link >= ss->num_links) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	subtilis_bitset_init(&visited);

	start = &ss->links[link];
	if (start->link >= sss->link_count) {
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	subtilis_bitset_set(&visited, start->link, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;
	next = &sss->sub_sections[sss->ss_link_map[start->link]];
	prv_visit_subsection(sss, next, start, int_save, real_save, &visited,
			     err);

cleanup:

	subtilis_bitset_free(&visited);

	subtilis_bitset_free(&sss->int_save);
	subtilis_bitset_free(&sss->real_save);
}

void subtilis_arm_subsections_dump(subtilis_arm_subsections_t *sss,
				   subtilis_arm_section_t *arm_s)
{
	size_t i;
	size_t j;
	size_t ptr;
	subtilis_arm_ss_t *ss;
	subtilis_arm_op_t *op;
	subtilis_error_t err;

	subtilis_error_init(&err);

	printf("Combined must Save Int: ");
	subtilis_bitset_dump(&sss->int_save);
	printf("Combined Must Save Real: ");
	subtilis_bitset_dump(&sss->real_save);
	printf("\n");

	for (i = 0; i < sss->count; i++) {
		ss = &sss->sub_sections[i];
		printf("\nSubsection %zu:\n", i);
		printf("------------------------------\n");
		printf("Size %zu\n", ss->size);
		printf("Int Inputs: ");
		subtilis_bitset_dump(&ss->int_inputs);
		printf("\n");
		printf("Real Inputs: ");
		subtilis_bitset_dump(&ss->real_inputs);
		printf("\n");
		printf("Links: ");
		for (j = 0; j < ss->num_links; j++) {
			printf("\t%zu\n", ss->links[j].link);
			printf("\tInt Outputs: ");
			subtilis_bitset_dump(&ss->links[j].int_outputs);
			printf("\tReal Outputs: ");
			subtilis_bitset_dump(&ss->links[j].real_outputs);
			printf("\tMust Save Int: ");
			subtilis_bitset_dump(&ss->links[j].int_save);
			printf("\tMust Save Real: ");
			subtilis_bitset_dump(&ss->links[j].real_save);
			op = &arm_s->op_pool->ops[ss->links[j].op];
			printf("\tOP ptr %zu: ", ss->links[j].op);
			if (op->type == SUBTILIS_ARM_OP_LABEL)
				printf("label_%zu\n", op->op.label);
			else
				subtilis_arm_instr_dump(&op->op.instr);
			printf("\n");
		}
		printf("\n");
		printf("------------------------------\n");
		printf("\n");
		ptr = ss->start;
		while (ptr != SIZE_MAX) {
			op = &arm_s->op_pool->ops[ptr];
			if (op->type == SUBTILIS_ARM_OP_LABEL)
				printf("label_%zu\n", op->op.label);
			else
				subtilis_arm_instr_dump(&op->op.instr);
			if (ptr == ss->end)
				break;

			ptr = op->next;
		}
		printf("\n");
	}
}
