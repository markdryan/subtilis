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

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "ir.h"

struct subtilis_ir_op_desc_t_ {
	const char *const name;
	subtilis_op_instr_class_t cls;
};

typedef struct subtilis_ir_op_desc_t_ subtilis_ir_op_desc_t;

typedef enum {
	SUBTILIS_IR_OPERAND_REGISTER,
	SUBTILIS_IR_OPERAND_FREGISTER,
	SUBTILIS_IR_OPERAND_I32,
	SUBTILIS_IR_OPERAND_REAL,
	SUBTILIS_IR_OPERAND_LABEL,
} subtilis_ir_operand_class_t;

struct subtilis_ir_class_info_t_ {
	size_t op_count;
	subtilis_ir_operand_class_t classes[SUBTILIS_IR_MAX_OP_ARGS];
};

typedef struct subtilis_ir_class_info_t_ subtilis_ir_class_info_t;

/* The ordering of this table is very important.  The entries in
 * it must correspond to the enumerated types in
 * subtilis_op_instr_type_t
 */

/* TODO We should probably order this by alpabetical ordering
 * of instructions so we can do a bsearch
 */

/* clang-format off */
static const subtilis_ir_op_desc_t op_desc[] = {
	{ "addi32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "addr", SUBTILIS_OP_CLASS_FREG_FREG_FREG },
	{ "subi32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "subr", SUBTILIS_OP_CLASS_FREG_FREG_FREG },
	{ "muli32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "mulr", SUBTILIS_OP_CLASS_FREG_FREG_FREG },
	{ "divi32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "modi32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "divr", SUBTILIS_OP_CLASS_FREG_FREG_FREG },
	{ "addii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "addir", SUBTILIS_OP_CLASS_FREG_FREG_REAL },
	{ "subii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "subir", SUBTILIS_OP_CLASS_FREG_FREG_REAL },
	{ "mulii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "mulir", SUBTILIS_OP_CLASS_FREG_FREG_REAL},
	{ "divii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "divir", SUBTILIS_OP_CLASS_FREG_FREG_REAL},
	{ "loadoi8", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "loadoi32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "loador", SUBTILIS_OP_CLASS_FREG_REG_I32},
	{ "loadi32", SUBTILIS_OP_CLASS_REG_REG},
	{ "loadr", SUBTILIS_OP_CLASS_REG_REG},
	{ "storeoi8", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "storeoi32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "storeor", SUBTILIS_OP_CLASS_FREG_REG_I32},
	{ "storei32", SUBTILIS_OP_CLASS_REG_REG},
	{ "storer", SUBTILIS_OP_CLASS_REG_REG},
	{ "movii32", SUBTILIS_OP_CLASS_REG_I32},
	{ "movir", SUBTILIS_OP_CLASS_FREG_REAL},
	{ "mov", SUBTILIS_OP_CLASS_REG_REG},
	{ "movfp", SUBTILIS_OP_CLASS_FREG_FREG},
	{ "printstr", SUBTILIS_OP_CLASS_REG_REG },
	{ "printnl", SUBTILIS_OP_CLASS_NONE},
	{ "rsubii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "rsubir", SUBTILIS_OP_CLASS_FREG_FREG_REAL },
	{ "rdivir", SUBTILIS_OP_CLASS_FREG_FREG_REAL},
	{ "andi32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "andii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "ori32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "orii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "eori32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "eorii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "noti32", SUBTILIS_OP_CLASS_REG_REG},
	{ "eqi32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "eqr", SUBTILIS_OP_CLASS_REG_FREG_FREG},
	{ "eqii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "eqir", SUBTILIS_OP_CLASS_REG_FREG_REAL},
	{ "neqi32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "neqr", SUBTILIS_OP_CLASS_REG_FREG_FREG},
	{ "neqii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "neqir", SUBTILIS_OP_CLASS_REG_FREG_REAL},
	{ "gti32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "gtr", SUBTILIS_OP_CLASS_REG_FREG_FREG},
	{ "gtii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "gtir", SUBTILIS_OP_CLASS_REG_FREG_REAL},
	{ "ltei32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "lter", SUBTILIS_OP_CLASS_REG_FREG_FREG},
	{ "lteii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "lteir", SUBTILIS_OP_CLASS_REG_FREG_REAL},
	{ "lti32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "ltr", SUBTILIS_OP_CLASS_REG_FREG_FREG},
	{ "ltii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "ltir", SUBTILIS_OP_CLASS_REG_FREG_REAL},
	{ "gtei32", SUBTILIS_OP_CLASS_REG_REG_REG},
	{ "gter", SUBTILIS_OP_CLASS_REG_FREG_FREG},
	{ "gteii32", SUBTILIS_OP_CLASS_REG_REG_I32},
	{ "gteir", SUBTILIS_OP_CLASS_REG_FREG_REAL},
	{ "jmpc", SUBTILIS_OP_CLASS_REG_LABEL_LABEL},
	{ "jmpcnf", SUBTILIS_OP_CLASS_REG_LABEL_LABEL},
	{ "jmp", SUBTILIS_OP_CLASS_LABEL},
	{ "ret", SUBTILIS_OP_CLASS_NONE},
	{ "reti32", SUBTILIS_OP_CLASS_REG},
	{ "retii32", SUBTILIS_OP_CLASS_I32},
	{ "retr", SUBTILIS_OP_CLASS_FREG},
	{ "retir", SUBTILIS_OP_CLASS_REAL},
	{ "lsli32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "lslii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "lsri32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "lsrii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "asri32", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "asrii32", SUBTILIS_OP_CLASS_REG_REG_I32 },
	{ "movi32fp", SUBTILIS_OP_CLASS_FREG_REG },
	{ "movfpi32", SUBTILIS_OP_CLASS_REG_FREG },
	{ "movfprdi32", SUBTILIS_OP_CLASS_REG_FREG },
	{ "nop", SUBTILIS_OP_CLASS_NONE },
	{ "modei32", SUBTILIS_OP_CLASS_REG },
	{ "plot", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "gcol", SUBTILIS_OP_CLASS_REG_REG },
	{ "gcoltint", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "origin", SUBTILIS_OP_CLASS_REG_REG },
	{ "gettime", SUBTILIS_OP_CLASS_REG },
	{ "cls", SUBTILIS_OP_CLASS_NONE },
	{ "clg", SUBTILIS_OP_CLASS_NONE },
	{ "on", SUBTILIS_OP_CLASS_NONE },
	{ "off", SUBTILIS_OP_CLASS_NONE },
	{ "wait", SUBTILIS_OP_CLASS_NONE },
	{ "sin", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "cos", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "tan", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "asn", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "acs", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "atn", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "sqr", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "log", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "ln", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "absr", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "powr", SUBTILIS_OP_CLASS_FREG_FREG_FREG },
	{ "expr", SUBTILIS_OP_CLASS_FREG_FREG },
	{ "get", SUBTILIS_OP_CLASS_REG },
	{ "gettimeout", SUBTILIS_OP_CLASS_REG_REG },
	{ "inkey", SUBTILIS_OP_CLASS_REG_REG },
	{ "osbyteid", SUBTILIS_OP_CLASS_REG },
	{ "vdui", SUBTILIS_OP_CLASS_I32 },
	{ "vdu", SUBTILIS_OP_CLASS_REG },
	{ "point", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "tint", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "end", SUBTILIS_OP_CLASS_NONE },
	{ "testesc", SUBTILIS_OP_CLASS_NONE },
	{ "alloc", SUBTILIS_OP_CLASS_REG_REG },
	{ "realloc", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "ref", SUBTILIS_OP_CLASS_REG },
	{ "deref", SUBTILIS_OP_CLASS_REG },
	{ "getref", SUBTILIS_OP_CLASS_REG_REG },
	{ "pushi32", SUBTILIS_OP_CLASS_REG },
	{ "popi32", SUBTILIS_OP_CLASS_REG },
	{ "lca", SUBTILIS_OP_CLASS_REG_I32 },
	{ "at", SUBTILIS_OP_CLASS_REG_REG },
	{ "pos", SUBTILIS_OP_CLASS_REG },
	{ "vpos", SUBTILIS_OP_CLASS_REG },
	{ "tcol", SUBTILIS_OP_CLASS_REG },
	{ "tcoltint", SUBTILIS_OP_CLASS_REG_REG },
	{ "palette", SUBTILIS_OP_CLASS_REG_REG_REG_REG},
	{ "i32todec", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "realtodec", SUBTILIS_OP_CLASS_REG_FREG_REG },
	{ "i32tohex", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "heapfree", SUBTILIS_OP_CLASS_REG },
	{ "blockfree", SUBTILIS_OP_CLASS_REG_REG },
	{ "blockadjust", SUBTILIS_OP_CLASS_REG_REG },
	{ "cmovi32", SUBTILIS_OP_CLASS_REG_REG_REG_REG },
	{ "openout", SUBTILIS_OP_CLASS_REG_REG },
	{ "openup", SUBTILIS_OP_CLASS_REG_REG },
	{ "openin", SUBTILIS_OP_CLASS_REG_REG },
	{ "close", SUBTILIS_OP_CLASS_REG },
	{ "bget", SUBTILIS_OP_CLASS_REG_REG },
	{ "bput", SUBTILIS_OP_CLASS_REG_REG },
	{ "blockget", SUBTILIS_OP_CLASS_REG_REG_REG_REG },
	{ "blockput", SUBTILIS_OP_CLASS_REG_REG_REG },
	{ "eof", SUBTILIS_OP_CLASS_REG_REG },
	{ "ext", SUBTILIS_OP_CLASS_REG_REG },
	{ "getptr", SUBTILIS_OP_CLASS_REG_REG },
	{ "setptr", SUBTILIS_OP_CLASS_REG_REG },
	{ "signx8to32", SUBTILIS_OP_CLASS_REG_REG },
	{ "movi8tofp", SUBTILIS_OP_CLASS_FREG_REG },
	{ "movfptoi32i32", SUBTILIS_OP_CLASS_REG_REG_FREG },
	{ "oscli", SUBTILIS_OP_CLASS_REG },
	{ "getprocaddr", SUBTILIS_OP_CLASS_REG_I32 },
};

/*
 * Must match the order of the enumerated types declared in
 * subtilis_op_instr_class_t.
 */

static const subtilis_ir_class_info_t class_details[] = {
	{4, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_REGISTER} },
	{3, { SUBTILIS_IR_OPERAND_FREGISTER, SUBTILIS_IR_OPERAND_FREGISTER,
	      SUBTILIS_IR_OPERAND_FREGISTER} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_I32} },
	{3, { SUBTILIS_IR_OPERAND_FREGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_I32} },
	{3, { SUBTILIS_IR_OPERAND_FREGISTER, SUBTILIS_IR_OPERAND_FREGISTER,
	      SUBTILIS_IR_OPERAND_REAL} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_LABEL,
	      SUBTILIS_IR_OPERAND_LABEL} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_FREGISTER,
	      SUBTILIS_IR_OPERAND_FREGISTER} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_FREGISTER,
	      SUBTILIS_IR_OPERAND_REAL} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_FREGISTER,
	      SUBTILIS_IR_OPERAND_REGISTER} },
	{3, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER,
	      SUBTILIS_IR_OPERAND_FREGISTER} },
	{2, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_I32} },
	{2, { SUBTILIS_IR_OPERAND_FREGISTER, SUBTILIS_IR_OPERAND_REAL} },
	{2, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_REGISTER} },
	{2, { SUBTILIS_IR_OPERAND_FREGISTER, SUBTILIS_IR_OPERAND_FREGISTER} },
	{2, { SUBTILIS_IR_OPERAND_REGISTER, SUBTILIS_IR_OPERAND_FREGISTER} },
	{2, { SUBTILIS_IR_OPERAND_FREGISTER, SUBTILIS_IR_OPERAND_REGISTER} },
	{1, { SUBTILIS_IR_OPERAND_REGISTER} },
	{1, { SUBTILIS_IR_OPERAND_FREGISTER} },
	{1, { SUBTILIS_IR_OPERAND_I32} },
	{1, { SUBTILIS_IR_OPERAND_REAL} },
	{1, { SUBTILIS_IR_OPERAND_LABEL} },
	{0, { SUBTILIS_IR_OPERAND_REGISTER} },
};

/* clang-format on */

void subtilis_handler_list_free(subtilis_handler_list_t *list)
{
	subtilis_handler_list_t *p;

	while (list) {
		p = list->next;
		free(list);
		list = p;
	}
}

subtilis_handler_list_t *
subtilis_handler_list_truncate(subtilis_handler_list_t *list, size_t level)
{
	subtilis_handler_list_t *p;

	while (list && level < list->level) {
		p = list->next;
		free(list);
		list = p;
	}

	return list;
}

subtilis_handler_list_t *
subtilis_handler_list_update(subtilis_handler_list_t *list, size_t level,
			     size_t label, subtilis_error_t *err)
{
	subtilis_handler_list_t *p;

	if (!list || level > list->level) {
		p = malloc(sizeof(*p));
		if (!p) {
			subtilis_error_set_oom(err);
			return NULL;
		}
		p->level = level;
		p->label = label;
		p->next = list;
		return p;
	}

	if (level < list->level) {
		subtilis_error_set_assertion_failed(err);
		return list;
	}

	list->label = label;

	return list;
}

static subtilis_ir_section_t *prv_ir_section_new(subtilis_error_t *err)
{
	subtilis_ir_section_t *s = calloc(1, sizeof(*s));

	if (!s) {
		subtilis_error_set_oom(err);
		return NULL;
	}
	s->reg_counter = SUBTILIS_IR_REG_TEMP_START;
	s->ftype = SUBTILIS_BUILTINS_MAX;
	s->array_access = SIZE_MAX;
	s->cleanup_stack = SIZE_MAX;
	s->cleanup_stack_nop = SIZE_MAX;
	s->cleanup_stack_reg = SIZE_MAX;

	return s;
}

static void prv_ensure_buffer(subtilis_ir_section_t *s, subtilis_error_t *err)
{
	subtilis_ir_op_t **new_ops;
	size_t new_max;

	if (s->len < s->max_len)
		return;

	new_max = s->max_len + SUBTILIS_CONFIG_PROGRAM_GRAN;
	new_ops = realloc(s->ops, new_max * sizeof(subtilis_ir_op_t *));
	if (!new_ops) {
		subtilis_error_set_oom(err);
		return;
	}
	s->max_len = new_max;
	s->ops = new_ops;
}

static void prv_ensure_error_buffer(subtilis_ir_section_t *s,
				    subtilis_error_t *err)
{
	subtilis_ir_op_t **new_ops;
	size_t new_max;

	if (s->error_len < s->max_error_len)
		return;

	new_max = s->max_error_len + SUBTILIS_CONFIG_PROGRAM_GRAN;
	new_ops = realloc(s->error_ops, new_max * sizeof(subtilis_ir_op_t *));
	if (!new_ops) {
		subtilis_error_set_oom(err);
		return;
	}
	s->max_error_len = new_max;
	s->error_ops = new_ops;
}

static bool prv_is_call(subtilis_ir_op_t *op)
{
	return (op->type == SUBTILIS_OP_CALL) ||
	       (op->type == SUBTILIS_OP_CALLI32) ||
	       (op->type == SUBTILIS_OP_CALLREAL) ||
	       (op->type == SUBTILIS_OP_CALL_PTR) ||
	       (op->type == SUBTILIS_OP_CALLI32_PTR) ||
	       (op->type == SUBTILIS_OP_CALLREAL_PTR);
}

static void prv_ir_section_delete(subtilis_ir_section_t *s)
{
	size_t i;
	subtilis_ir_op_t *op;

	if (!s)
		return;

	subtilis_type_section_delete(s->type);
	for (i = 0; i < s->len; i++) {
		op = s->ops[i];
		if (prv_is_call(op)) {
			free(op->op.call.args);
		} else if (op->type == SUBTILIS_OP_SYS_CALL) {
			free(op->op.sys_call.in_regs);
			free(op->op.sys_call.out_regs);
		}

		free(op);
	}
	free(s->ops);

	for (i = 0; i < s->error_len; i++) {
		op = s->error_ops[i];
		if (prv_is_call(op)) {
			free(op->op.call.args);
		} else if (op->type == SUBTILIS_OP_SYS_CALL) {
			free(op->op.sys_call.in_regs);
			free(op->op.sys_call.out_regs);
		}
		free(op);
	}
	free(s->error_ops);
	subtilis_handler_list_free(s->handler_list);

	if (s->asm_free_fn)
		s->asm_free_fn(s->asm_code);

	free(s);
}

void subtilis_ir_merge_errors(subtilis_ir_section_t *s, subtilis_error_t *err)
{
	size_t i;

	s->handler_offset = s->len;
	for (i = 0; i < s->error_len; i++) {
		prv_ensure_buffer(s, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		s->ops[s->len++] = s->error_ops[i];
		s->error_ops[i] = NULL;
	}
	s->error_len = 0;
	s->max_error_len = 0;
	free(s->error_ops);
	s->error_ops = NULL;
}

size_t subtilis_ir_section_add_instr(subtilis_ir_section_t *s,
				     subtilis_op_instr_type_t type,
				     subtilis_ir_operand_t op1,
				     subtilis_ir_operand_t op2,
				     subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;
	const subtilis_ir_class_info_t *details;
	size_t *counter;

	details = &class_details[op_desc[type].cls];
	if (details->op_count == 0) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}
	if (details->classes[0] == SUBTILIS_IR_OPERAND_REGISTER) {
		counter = &s->reg_counter;
	} else if (details->classes[0] == SUBTILIS_IR_OPERAND_FREGISTER) {
		counter = &s->freg_counter;
	} else {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	op0.reg = *counter;
	subtilis_ir_section_add_instr_reg(s, type, op0, op1, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	(*counter)++;
	return op0.reg;
}

size_t subtilis_ir_section_add_instr2(subtilis_ir_section_t *s,
				      subtilis_op_instr_type_t type,
				      subtilis_ir_operand_t op1,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t op2;

	memset(&op2, 0, sizeof(op2));
	return subtilis_ir_section_add_instr(s, type, op1, op2, err);
}

size_t subtilis_ir_section_add_instr1(subtilis_ir_section_t *s,
				      subtilis_op_instr_type_t type,
				      subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	memset(&op1, 0, sizeof(op1));
	memset(&op2, 0, sizeof(op2));
	return subtilis_ir_section_add_instr(s, type, op1, op2, err);
}

void subtilis_ir_section_add_instr_no_reg(subtilis_ir_section_t *s,
					  subtilis_op_instr_type_t type,
					  subtilis_ir_operand_t op0,
					  subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;

	memset(&op1, 0, sizeof(op1));
	memset(&op2, 0, sizeof(op2));
	subtilis_ir_section_add_instr_reg(s, type, op0, op1, op2, err);
}

void subtilis_ir_section_add_instr_no_reg2(subtilis_ir_section_t *s,
					   subtilis_op_instr_type_t type,
					   subtilis_ir_operand_t op0,
					   subtilis_ir_operand_t op1,
					   subtilis_error_t *err)
{
	subtilis_ir_operand_t op2;

	memset(&op2, 0, sizeof(op2));
	subtilis_ir_section_add_instr_reg(s, type, op0, op1, op2, err);
}

void subtilis_ir_section_add_instr_no_arg(subtilis_ir_section_t *s,
					  subtilis_op_instr_type_t type,
					  subtilis_error_t *err)
{
	subtilis_ir_operand_t op0;

	memset(&op0, 0, sizeof(op0));
	subtilis_ir_section_add_instr_no_reg(s, type, op0, err);
}

void subtilis_ir_section_add_instr_reg(subtilis_ir_section_t *s,
				       subtilis_op_instr_type_t type,
				       subtilis_ir_operand_t op0,
				       subtilis_ir_operand_t op1,
				       subtilis_ir_operand_t op2,
				       subtilis_error_t *err)
{
	subtilis_ir_op_t *op;
	subtilis_ir_inst_t *instr;

	if (s->in_error_handler)
		prv_ensure_error_buffer(s, err);
	else
		prv_ensure_buffer(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return;
	}

	op->type = SUBTILIS_OP_INSTR;
	instr = &op->op.instr;
	instr->type = type;
	instr->operands[0] = op0;
	instr->operands[1] = op1;
	instr->operands[2] = op2;

	if (s->in_error_handler)
		s->error_ops[s->error_len++] = op;
	else
		s->ops[s->len++] = op;
}

/* clang-format off */
void subtilis_ir_section_add_instr4(subtilis_ir_section_t *s,
				    subtilis_op_instr_type_t type,
				    subtilis_ir_operand_t op0,
				    subtilis_ir_operand_t op1,
				    subtilis_ir_operand_t op2,
				    subtilis_ir_operand_t op3,
				    subtilis_error_t *err)

/* clang-format on */

{
	subtilis_ir_op_t *op;
	subtilis_ir_inst_t *instr;

	if (s->in_error_handler)
		prv_ensure_error_buffer(s, err);
	else
		prv_ensure_buffer(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return;
	}

	op->type = SUBTILIS_OP_INSTR;
	instr = &op->op.instr;
	instr->type = type;
	instr->operands[0] = op0;
	instr->operands[1] = op1;
	instr->operands[2] = op2;
	instr->operands[3] = op3;

	if (s->in_error_handler)
		s->error_ops[s->error_len++] = op;
	else
		s->ops[s->len++] = op;
}

size_t subtilis_ir_section_add_nop(subtilis_ir_section_t *s,
				   subtilis_error_t *err)
{
	size_t ret;
	subtilis_ir_op_t *op;

	if (s->in_error_handler)
		prv_ensure_error_buffer(s, err);
	else
		prv_ensure_buffer(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return 0;
	}

	op->type = SUBTILIS_OP_INSTR;
	op->op.instr.type = SUBTILIS_OP_INSTR_NOP;

	ret = s->len;

	if (s->in_error_handler)
		s->error_ops[s->error_len++] = op;
	else
		s->ops[s->len++] = op;

	return ret;
}

size_t subtilis_ir_section_promote_nop(subtilis_ir_section_t *s, size_t nop,
				       subtilis_op_instr_type_t type,
				       size_t op1, subtilis_error_t *err)
{
	subtilis_ir_inst_t *instr;
	subtilis_ir_op_t *op;

	if (s->in_error_handler || ((type != SUBTILIS_OP_INSTR_MOV_I32_FP) &&
				    (type != SUBTILIS_OP_INSTR_MOV_FP_I32) &&
				    (type != SUBTILIS_OP_INSTR_SIGNX_8_TO_32) &&
				    (type != SUBTILIS_OP_INSTR_MOV_I8_FP))) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	if (nop >= s->len) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	op = s->ops[nop];
	if ((op->type != SUBTILIS_OP_INSTR) ||
	    (op->op.instr.type != SUBTILIS_OP_INSTR_NOP)) {
		subtilis_error_set_assertion_failed(err);
		return 0;
	}

	instr = &op->op.instr;
	instr->type = type;
	instr->operands[0].reg = (type == SUBTILIS_OP_INSTR_MOV_I32_FP ||
				  type == SUBTILIS_OP_INSTR_MOV_I8_FP)
				     ? s->freg_counter++
				     : s->reg_counter++;
	instr->operands[1].reg = op1;

	return instr->operands[0].reg;
}

subtilis_ir_prog_t *subtilis_ir_prog_new(const subtilis_settings_t *settings,
					 subtilis_error_t *err)
{
	subtilis_ir_prog_t *p;

	p = calloc(1, sizeof(*p));
	if (!p) {
		subtilis_error_set_oom(err);
		return NULL;
	}

	p->string_pool = subtilis_string_pool_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->constant_pool = subtilis_constant_pool_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->settings = settings;

	return p;

cleanup:

	subtilis_ir_prog_delete(p);

	return NULL;
}

/* clang-format off */
static subtilis_ir_section_t *prv_ir_prog_section_new(
	subtilis_ir_prog_t *p, const char *name,
	subtilis_ir_section_type_t section_type, size_t locals,
	subtilis_type_section_t *tp, subtilis_builtin_type_t ftype,
	const char *file, size_t line, int32_t eflag_offset,
	int32_t error_offset, size_t *index, subtilis_error_t *err)

/* clang-format on */
{
	subtilis_ir_section_t *s;
	size_t name_index;
	size_t new_max;
	subtilis_ir_section_t **new_pool;

	s = prv_ir_section_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	s->section_type = section_type;
	s->eflag_offset = eflag_offset;
	s->error_offset = error_offset;
	s->reg_counter += tp->int_regs;
	s->cleanup_stack_reg = s->reg_counter++;
	s->freg_counter += tp->fp_regs;
	s->ftype = ftype;
	s->type = tp;
	tp = NULL;

	name_index = subtilis_string_pool_register(p->string_pool, name, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (name_index < p->num_sections) {
		if (p->sections[name_index]) {
			if (index)
				*index = name_index;

			subtilis_error_set_already_defined(err, name, file,
							   line);
			goto cleanup;
		}
	} else {
		if (name_index >= p->max_sections) {
			new_max = p->max_sections + SUBTILIS_CONFIG_PROC_GRAN;
			if (new_max < name_index + 1)
				new_max = name_index + 1;
			new_pool = realloc(p->sections,
					   sizeof(*p->sections) * new_max);
			if (!new_pool) {
				subtilis_error_set_oom(err);
				goto cleanup;
			}
			memset(&new_pool[p->num_sections], 0,
			       sizeof(*p->sections) *
				   (new_max - p->num_sections));
			p->sections = new_pool;
			p->max_sections = new_max;
		}
		p->num_sections = name_index + 1;
	}
	s->locals = locals;
	s->end_label = subtilis_ir_section_new_label(s);
	s->nofree_label = subtilis_ir_section_new_label(s);
	switch (s->type->type.params.fn.ret_val->type) {
	case SUBTILIS_TYPE_VOID:
		s->ret_reg = SIZE_MAX;
		break;
	case SUBTILIS_TYPE_REAL:
		s->ret_reg = s->freg_counter++;
		break;
	case SUBTILIS_TYPE_INTEGER:
	case SUBTILIS_TYPE_BYTE:
	case SUBTILIS_TYPE_ARRAY_REAL:
	case SUBTILIS_TYPE_ARRAY_INTEGER:
	case SUBTILIS_TYPE_ARRAY_BYTE:
	case SUBTILIS_TYPE_ARRAY_STRING:
	case SUBTILIS_TYPE_ARRAY_FN:
	case SUBTILIS_TYPE_ARRAY_REC:
	case SUBTILIS_TYPE_VECTOR_REAL:
	case SUBTILIS_TYPE_VECTOR_INTEGER:
	case SUBTILIS_TYPE_VECTOR_BYTE:
	case SUBTILIS_TYPE_VECTOR_STRING:
	case SUBTILIS_TYPE_VECTOR_FN:
	case SUBTILIS_TYPE_VECTOR_REC:
	case SUBTILIS_TYPE_STRING:
	case SUBTILIS_TYPE_FN:
		s->ret_reg = s->reg_counter++;
		break;
	default:
		subtilis_error_set_assertion_failed(err);
		goto cleanup;
	}

	p->sections[name_index] = s;
	if (index)
		*index = name_index;

	return s;

cleanup:

	subtilis_type_section_delete(tp);
	prv_ir_section_delete(s);

	return NULL;
}

/* clang-format off */
subtilis_ir_section_t *subtilis_ir_prog_section_new(
	subtilis_ir_prog_t *p, const char *name, size_t locals,
	subtilis_type_section_t *tp, subtilis_builtin_type_t ftype,
	const char *file, size_t line, int32_t eflag_offset,
	int32_t error_offset, size_t *call_index, subtilis_error_t *err)

/* clang-format on */

{
	subtilis_ir_section_type_t section_type;

	section_type = ftype == SUBTILIS_BUILTINS_MAX
			   ? SUBTILIS_IR_SECTION_IR
			   : SUBTILIS_IR_SECTION_BACKEND_BUILTIN;

	return prv_ir_prog_section_new(p, name, section_type, locals, tp, ftype,
				       file, line, eflag_offset, error_offset,
				       call_index, err);
}

/* clang-format off */
void subtilis_ir_prog_asm_section_new(
	subtilis_ir_prog_t *p, const char *name, subtilis_type_section_t *tp,
	const char *file, size_t line, int32_t eflag_offset,
	int32_t error_offset, subtilis_backend_asm_free_t asm_free,
	void *asm_code, size_t *call_index, subtilis_error_t *err)

/* clang-format on */

{
	subtilis_ir_section_t *s;

	s = prv_ir_prog_section_new(
	    p, name, SUBTILIS_IR_SECTION_ASM, 0, tp, SUBTILIS_BUILTINS_MAX,
	    file, line, eflag_offset, error_offset, call_index, err);
	if (err->type != SUBTILIS_ERROR_OK) {
		asm_free(asm_code);
		return;
	}

	s->asm_free_fn = asm_free;
	s->asm_code = asm_code;
}

subtilis_ir_section_t *subtilis_ir_prog_find_section(subtilis_ir_prog_t *p,
						     const char *name)
{
	size_t index;

	if (!subtilis_string_pool_find(p->string_pool, name, &index))
		return NULL;

	if (index >= p->num_sections)
		return NULL;

	return p->sections[index];
}

void subtilis_ir_prog_dump(subtilis_ir_prog_t *p)
{
	size_t i;
	subtilis_ir_section_t *s;

	for (i = 0; i < p->num_sections; i++) {
		s = p->sections[i];
		if (s->section_type != SUBTILIS_IR_SECTION_IR)
			continue;
		if (s->type->type.params.fn.ret_val->type == SUBTILIS_TYPE_VOID)
			printf("PROC%s\n----------------\n\n",
			       p->string_pool->strings[i]);
		else
			printf("FN%s\n----------------\n\n",
			       p->string_pool->strings[i]);
		subtilis_ir_section_dump(p->sections[i]);
		printf("\n");
	}

	printf("String Pool\n----------------\n\n");
	subtilis_string_pool_dump(p->string_pool);
	printf("\n");

	printf("Constant Pool\n----------------\n\n");
	subtilis_constant_pool_dump(p->constant_pool);
}

void subtilis_ir_prog_delete(subtilis_ir_prog_t *p)
{
	size_t i;

	if (!p)
		return;

	for (i = 0; i < p->num_sections; i++)
		prv_ir_section_delete(p->sections[i]);
	free(p->sections);

	subtilis_string_pool_delete(p->string_pool);
	subtilis_constant_pool_delete(p->constant_pool);
	free(p);
}

static void prv_dump_instr(subtilis_ir_inst_t *instr)
{
	if (op_desc[instr->type].cls == SUBTILIS_OP_CLASS_NONE) {
		printf("\t%s", op_desc[instr->type].name);
		return;
	}

	printf("\t%s ", op_desc[instr->type].name);
	switch (op_desc[instr->type].cls) {
	case SUBTILIS_OP_CLASS_REG_REG_REG_REG:
		printf("r%zu, r%zu, r%zu, r%zu", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].reg,
		       instr->operands[3].reg);
		break;
	case SUBTILIS_OP_CLASS_REG_REG_REG:
		printf("r%zu, r%zu, r%zu", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].reg);
		break;
	case SUBTILIS_OP_CLASS_FREG_FREG_FREG:
		printf("f%zu, f%zu, f%zu", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].reg);
		break;
	case SUBTILIS_OP_CLASS_REG_REG_I32:
		printf("r%zu, r%zu, #%" PRIi32, instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].integer);
		break;
	case SUBTILIS_OP_CLASS_FREG_REG_I32:
		printf("f%zu, r%zu, #%" PRIi32, instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].integer);
		break;
	case SUBTILIS_OP_CLASS_FREG_FREG_REAL:
		printf("f%zu, f%zu, #%lf", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].real);
		break;
	case SUBTILIS_OP_CLASS_REG_LABEL_LABEL:
		printf("r%zu, label_%zu, label_%zu", instr->operands[0].reg,
		       instr->operands[1].label, instr->operands[2].label);
		break;
	case SUBTILIS_OP_CLASS_REG_FREG_FREG:
		printf("r%zu, f%zu, f%zu", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].reg);
		break;
	case SUBTILIS_OP_CLASS_REG_FREG_REAL:
		printf("r%zu, f%zu, #%lf", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].real);
		break;
	case SUBTILIS_OP_CLASS_REG_FREG_REG:
		printf("r%zu, f%zu, r%zu", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].reg);
		break;
	case SUBTILIS_OP_CLASS_REG_REG_FREG:
		printf("r%zu, r%zu, f%zu", instr->operands[0].reg,
		       instr->operands[1].reg, instr->operands[2].reg);
		break;
	case SUBTILIS_OP_CLASS_REG_I32:
		printf("r%zu, #%" PRIi32, instr->operands[0].reg,
		       instr->operands[1].integer);
		break;
	case SUBTILIS_OP_CLASS_FREG_REAL:
		printf("f%zu, #%lf", instr->operands[0].reg,
		       instr->operands[1].real);
		break;
	case SUBTILIS_OP_CLASS_REG_REG:
		printf("r%zu, r%zu", instr->operands[0].reg,
		       instr->operands[1].reg);
		break;
	case SUBTILIS_OP_CLASS_FREG_FREG:
		printf("f%zu, f%zu", instr->operands[0].reg,
		       instr->operands[1].reg);
		break;
	case SUBTILIS_OP_CLASS_REG_FREG:
		printf("r%zu, f%zu", instr->operands[0].reg,
		       instr->operands[1].reg);
		break;
	case SUBTILIS_OP_CLASS_FREG_REG:
		printf("f%zu, r%zu", instr->operands[0].reg,
		       instr->operands[1].reg);
		break;
	case SUBTILIS_OP_CLASS_REG:
		printf("r%zu", instr->operands[0].reg);
		break;
	case SUBTILIS_OP_CLASS_FREG:
		printf("f%zu", instr->operands[0].reg);
		break;
	case SUBTILIS_OP_CLASS_I32:
		printf("%" PRIi32, instr->operands[0].integer);
		break;
	case SUBTILIS_OP_CLASS_REAL:
		printf("%lf", instr->operands[0].real);
		break;
	case SUBTILIS_OP_CLASS_LABEL:
		printf("r%zu", instr->operands[0].reg);
		break;
	case SUBTILIS_OP_CLASS_NONE:
		break;
	}
}

static void prv_dump_call(subtilis_op_type_t type, subtilis_ir_call_t *c)
{
	size_t i;

	printf("\t");
	if (type == SUBTILIS_OP_CALLI32 || type == SUBTILIS_OP_CALLI32_PTR)
		printf("r%zu = ", c->reg);
	else if (type == SUBTILIS_OP_CALLREAL ||
		 type == SUBTILIS_OP_CALLREAL_PTR)
		printf("f%zu = ", c->reg);
	if ((type == SUBTILIS_OP_CALL_PTR) ||
	    (type == SUBTILIS_OP_CALLI32_PTR) ||
	    (type == SUBTILIS_OP_CALLREAL_PTR))
		printf("call [%zu]", c->proc_id);
	else
		printf("call %zu", c->proc_id);

	if (c->arg_count > 0) {
		printf(" (");
		printf("%s%zu",
		       c->args[0].type == SUBTILIS_IR_REG_TYPE_REAL ? "f" : "r",
		       c->args[0].reg);
		for (i = 1; i < c->arg_count; i++)
			printf(", %s%zu",
			       c->args[i].type == SUBTILIS_IR_REG_TYPE_REAL
				   ? "f"
				   : "r",
			       c->args[i].reg);
		printf(")");
	}
}

static void prv_dump_sys_call(subtilis_ir_sys_call_t *s)
{
	size_t i;
	size_t max_reg;

	printf("\tsyscall %zx", s->call_id);
	if (s->in_mask) {
		max_reg = 0;
		for (i = 0; i < 16; i++)
			if ((1 << i) & s->in_mask)
				max_reg++;
		i = 0;
		do {
			printf(", ");
			if ((1 << i) & s->in_mask)
				printf("%zu", s->in_regs[i]);
			i++;
		} while (i < max_reg);
	}

	if (s->out_mask) {
		printf("to ");
		max_reg = 0;
		for (i = 0; i < 16; i++)
			if ((1 << i) & s->out_mask)
				max_reg++;
		i = 0;
		do {
			if ((1 << i) & s->out_mask) {
				if (s->out_regs[i].local)
					printf("%zu", s->out_regs[i].reg);
				else
					printf("[%zu]", s->out_regs[i].reg);
			}
			i++;
			if (i == max_reg)
				break;
			printf(", ");
		} while (true);
	}

	if (s->flags_reg != SIZE_MAX) {
		if (s->flags_local)
			printf(" : %zu", s->flags_reg);
		else
			printf(" : [%zu]", s->flags_reg);
	}
}

static void prv_dump_section_ops(subtilis_ir_op_t **ops, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		if (!ops[i])
			continue;
		if (ops[i]->type == SUBTILIS_OP_INSTR)
			prv_dump_instr(&ops[i]->op.instr);
		else if (ops[i]->type == SUBTILIS_OP_LABEL)
			printf("label_%zu", ops[i]->op.label);
		else if (prv_is_call(ops[i]))
			prv_dump_call(ops[i]->type, &ops[i]->op.call);
		else if (ops[i]->type == SUBTILIS_OP_SYS_CALL)
			prv_dump_sys_call(&ops[i]->op.sys_call);
		else
			continue;
		printf("\n");
	}
}

void subtilis_ir_section_dump(subtilis_ir_section_t *s)
{
	prv_dump_section_ops(s->ops, s->len);
	prv_dump_section_ops(s->error_ops, s->error_len);
}

size_t subtilis_ir_section_new_label(subtilis_ir_section_t *s)
{
	return s->label_counter++;
}

void subtilis_ir_section_add_label(subtilis_ir_section_t *s, size_t l,
				   subtilis_error_t *err)
{
	subtilis_ir_op_t *op;

	if (s->in_error_handler)
		prv_ensure_error_buffer(s, err);
	else
		prv_ensure_buffer(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return;
	}
	op->type = SUBTILIS_OP_LABEL;
	op->op.label = l;
	if (s->in_error_handler)
		s->error_ops[s->error_len++] = op;
	else
		s->ops[s->len++] = op;
}

static void prv_add_call(subtilis_ir_section_t *s, subtilis_op_type_t type,
			 size_t arg_count, subtilis_ir_arg_t *args,
			 size_t proc_id, subtilis_error_t *err)
{
	subtilis_ir_op_t *op;

	if (s->in_error_handler)
		prv_ensure_error_buffer(s, err);
	else
		prv_ensure_buffer(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return;
	}
	op->type = type;
	op->op.call.proc_id = proc_id;
	op->op.call.arg_count = arg_count;
	op->op.call.args = args;
	if (s->in_error_handler)
		s->error_ops[s->error_len++] = op;
	else
		s->ops[s->len++] = op;
}

size_t subtilis_ir_section_add_get_partial_addr(subtilis_ir_section_t *s,
						size_t *call_site,
						subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	size_t call_reg;
	size_t cs;

	op1.label = SIZE_MAX;
	call_reg = subtilis_ir_section_add_instr2(
	    s, SUBTILIS_OP_INSTR_GET_PROC_ADDR, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return SIZE_MAX;

	if (s->in_error_handler)
		cs = s->error_len;
	else
		cs = s->len;
	*call_site = cs - 1;

	return call_reg;
}

void subtilis_ir_section_add_sys_call(subtilis_ir_section_t *s, size_t call_id,
				      size_t *in_regs,
				      subtilis_ir_sys_out_reg_t *out_regs,
				      uint32_t in_mask, uint32_t out_mask,
				      size_t flags_reg, bool flags_local,
				      subtilis_error_t *err)
{
	subtilis_ir_op_t *op;

	if (s->in_error_handler)
		prv_ensure_error_buffer(s, err);
	else
		prv_ensure_buffer(s, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	op = malloc(sizeof(*op));
	if (!op) {
		subtilis_error_set_oom(err);
		return;
	}
	op->type = SUBTILIS_OP_SYS_CALL;
	op->op.sys_call.call_id = call_id;
	op->op.sys_call.in_regs = in_regs;
	op->op.sys_call.out_regs = out_regs;
	op->op.sys_call.in_mask = in_mask;
	op->op.sys_call.out_mask = out_mask;
	op->op.sys_call.flags_reg = flags_reg;
	op->op.sys_call.flags_local = flags_local;
	if (s->in_error_handler)
		s->error_ops[s->error_len++] = op;
	else
		s->ops[s->len++] = op;
}

void subtilis_ir_section_add_call(subtilis_ir_section_t *s, size_t arg_count,
				  subtilis_ir_arg_t *args,
				  subtilis_error_t *err)
{
	prv_add_call(s, SUBTILIS_OP_CALL, arg_count, args, 0, err);
}

static size_t prv_get_i32_call_reg(subtilis_ir_section_t *s)
{
	subtilis_ir_op_t *op;

	if (s->in_error_handler)
		op = s->error_ops[s->error_len - 1];
	else
		op = s->ops[s->len - 1];
	op->op.call.reg = s->reg_counter++;
	return op->op.call.reg;
}

size_t subtilis_ir_section_add_i32_call(subtilis_ir_section_t *s,
					size_t arg_count,
					subtilis_ir_arg_t *args,
					subtilis_error_t *err)
{
	prv_add_call(s, SUBTILIS_OP_CALLI32, arg_count, args, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	return prv_get_i32_call_reg(s);
}

static size_t prv_get_real_call_reg(subtilis_ir_section_t *s)
{
	subtilis_ir_op_t *op;

	if (s->in_error_handler)
		op = s->error_ops[s->error_len - 1];
	else
		op = s->ops[s->len - 1];
	op->op.call.reg = s->freg_counter++;
	return op->op.call.reg;
}

size_t subtilis_ir_section_add_real_call(subtilis_ir_section_t *s,
					 size_t arg_count,
					 subtilis_ir_arg_t *args,
					 subtilis_error_t *err)
{
	prv_add_call(s, SUBTILIS_OP_CALLREAL, arg_count, args, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	return prv_get_real_call_reg(s);
}

void subtilis_ir_section_add_call_ptr(subtilis_ir_section_t *s,
				      size_t arg_count, subtilis_ir_arg_t *args,
				      size_t ptr, subtilis_error_t *err)
{
	prv_add_call(s, SUBTILIS_OP_CALL_PTR, arg_count, args, ptr, err);
}

size_t subtilis_ir_section_add_i32_call_ptr(subtilis_ir_section_t *s,
					    size_t arg_count,
					    subtilis_ir_arg_t *args, size_t ptr,
					    subtilis_error_t *err)
{
	prv_add_call(s, SUBTILIS_OP_CALLI32_PTR, arg_count, args, ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	return prv_get_i32_call_reg(s);
}

size_t subtilis_ir_section_add_real_call_ptr(subtilis_ir_section_t *s,
					     size_t arg_count,
					     subtilis_ir_arg_t *args,
					     size_t ptr, subtilis_error_t *err)
{
	prv_add_call(s, SUBTILIS_OP_CALLREAL_PTR, arg_count, args, ptr, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return 0;
	return prv_get_real_call_reg(s);
}

/*
 * Not using strsep as not sure it will be available on all C compilers
 * subtilis will be compiled with.
 */

static bool prv_is_white_space(const char *s)
{
	return *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r';
}

#define LABEL_STR_SIZE (sizeof("label_") - 1)

static void prv_parse_label(const char *text, subtilis_ir_label_match_t *label,
			    subtilis_error_t *err)
{
	unsigned long num;
	char *end_ptr = 0;

	if (!strcmp(text, "*")) {
		label->label = 0;
		label->op_match = SUBTILIS_OP_MATCH_ANY;
		return;
	}

	errno = 0;
	num = strtoul(text, &end_ptr, 10);
	if (*end_ptr != 0 || errno != 0 || num > INT_MAX) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	label->label = (size_t)num;
	label->op_match = SUBTILIS_OP_MATCH_FLOATING;
}

static const char *prv_parse_operands(const char *rule,
				      subtilis_ir_op_match_t *match,
				      subtilis_error_t *err)
{
	const int max_op = 32;
	char operand[max_op];
	size_t i;
	size_t j;
	const subtilis_ir_class_info_t *details;
	unsigned long num;
	const char *num_ptr;
	char *end_ptr = 0;
	subtilis_ir_label_match_t label;

	label.op_match = SUBTILIS_OP_MATCH_ANY;
	label.label = SIZE_MAX;

	details = &class_details[op_desc[match->op.instr.type].cls];

	for (j = 0; j < details->op_count; j++) {
		while (prv_is_white_space(rule))
			rule++;

		for (i = 0; *rule && i < max_op && !prv_is_white_space(rule);
		     i++) {
			if (*rule == ',') {
				rule++;
				break;
			}
			operand[i] = *rule++;
		}
		operand[i] = 0;
		if (!strcmp(operand, "*")) {
			match->op.instr.op_match[j] = SUBTILIS_OP_MATCH_ANY;
		} else if (!strncmp(operand, "label_", LABEL_STR_SIZE)) {
			if (details->classes[j] != SUBTILIS_IR_OPERAND_LABEL) {
				subtilis_error_set_assertion_failed(err);
				return NULL;
			}
			prv_parse_label(operand + LABEL_STR_SIZE, &label, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return NULL;
			match->op.instr.operands[j].label = label.label;
			match->op.instr.op_match[j] = label.op_match;
		} else if (operand[0] == '#') {
			if (details->classes[j] != SUBTILIS_IR_OPERAND_I32) {
				subtilis_error_set_assertion_failed(err);
				return NULL;
			}
			errno = 0;
			num = strtoul(operand + 1, &end_ptr, 10);
			if (*end_ptr != 0 || errno != 0 || num > INT_MAX) {
				subtilis_error_set_assertion_failed(err);
				return NULL;
			}
			match->op.instr.operands[j].integer = (int32_t)num;
			match->op.instr.op_match[j] = SUBTILIS_OP_MATCH_FIXED;
		} else if ((operand[0] == 'r') || (operand[0] == 'f')) {
			if ((operand[0] == 'r') &&
			    details->classes[j] !=
				SUBTILIS_IR_OPERAND_REGISTER) {
				subtilis_error_set_assertion_failed(err);
				return NULL;
			}
			if ((operand[0] == 'f') &&
			    details->classes[j] !=
				SUBTILIS_IR_OPERAND_FREGISTER) {
				subtilis_error_set_assertion_failed(err);
				return NULL;
			}
			num_ptr = operand + 1;
			if (operand[1] == 0) {
				subtilis_error_set_assertion_failed(err);
				return NULL;
			} else if (operand[1] == '_') {
				match->op.instr.op_match[j] =
				    SUBTILIS_OP_MATCH_FLOATING;
				num_ptr++;
			} else {
				match->op.instr.op_match[j] =
				    SUBTILIS_OP_MATCH_FIXED;
			}
			errno = 0;
			num = strtoul(num_ptr, &end_ptr, 10);
			if (*end_ptr != 0 || errno != 0 || num > INT_MAX) {
				subtilis_error_set_assertion_failed(err);
				return NULL;
			}
			match->op.instr.operands[j].reg = (int32_t)num;
		} else {
			/* Missing arguments on rule */

			subtilis_error_set_assertion_failed(err);
			return NULL;
		}
	}

	return rule;
}

static const char *prv_parse_match(const char *rule,
				   subtilis_ir_op_match_t *match,
				   subtilis_error_t *err)
{
	const int max_instr = 32;
	char instr[max_instr];
	size_t i;

	while (prv_is_white_space(rule))
		rule++;
	if (!rule[0])
		return NULL;

	for (i = 0; *rule && i < max_instr && !prv_is_white_space(rule); i++)
		instr[i] = *rule++;
	instr[i] = 0;

	if (!strncmp(instr, "label_", LABEL_STR_SIZE)) {
		match->type = SUBTILIS_OP_LABEL;
		prv_parse_label(instr + LABEL_STR_SIZE, &match->op.label, err);
		return rule;
	}

	if (!strcmp(instr, "call")) {
		match->type = SUBTILIS_OP_CALL;
		return rule;
	}

	if (!strcmp(instr, "calli32")) {
		match->type = SUBTILIS_OP_CALLI32;
		return rule;
	}

	if (!strcmp(instr, "callr")) {
		match->type = SUBTILIS_OP_CALLREAL;
		return rule;
	}

	if (!strcmp(instr, "callptr")) {
		match->type = SUBTILIS_OP_CALL_PTR;
		return rule;
	}

	if (!strcmp(instr, "calli32ptr")) {
		match->type = SUBTILIS_OP_CALLI32_PTR;
		return rule;
	}

	if (!strcmp(instr, "callrptr")) {
		match->type = SUBTILIS_OP_CALLREAL_PTR;
		return rule;
	}

	if (!strcmp(instr, "syscall")) {
		match->type = SUBTILIS_OP_SYS_CALL;
		return rule;
	}

	for (i = 0; i < sizeof(op_desc) / sizeof(subtilis_ir_op_desc_t); i++)
		if (!strcmp(instr, op_desc[i].name))
			break;

	if (i == sizeof(op_desc) / sizeof(subtilis_ir_op_desc_t)) {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}

	match->type = SUBTILIS_OP_INSTR;
	match->op.instr.type = (subtilis_op_instr_type_t)i;
	return prv_parse_operands(rule, match, err);
}

void subtilis_ir_parse_rules(const subtilis_ir_rule_raw_t *raw,
			     subtilis_ir_rule_t *parsed, size_t count,
			     subtilis_error_t *err)
{
	size_t i;
	size_t j;
	const char *rule;

	for (i = 0; i < count; i++) {
		rule = raw[i].text;
		j = 0;
		while (
		    j < SUBTILIS_IR_MAX_MATCHES &&
		    (rule = prv_parse_match(rule, &parsed[i].matches[j], err)))
			j++;
		if (err->type != SUBTILIS_ERROR_OK)
			return;
		if (j == 0) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
		parsed[i].action = raw[i].action;
		parsed[i].matches_count = j;
	}
}

#define MAX_MATCH_WILDCARDS 16

struct subtilis_ir_match_pair_t_ {
	size_t key;
	size_t value;
};

typedef struct subtilis_ir_match_pair_t_ subtilis_ir_match_pair_t;

struct subtilis_ir_match_state_t_ {
	subtilis_ir_match_pair_t regs[MAX_MATCH_WILDCARDS];
	subtilis_ir_match_pair_t fregs[MAX_MATCH_WILDCARDS];
	subtilis_ir_match_pair_t labels[MAX_MATCH_WILDCARDS];
	size_t regs_count;
	size_t fregs_count;
	size_t labels_count;
};

typedef struct subtilis_ir_match_state_t_ subtilis_ir_match_state_t;

static bool prv_process_floating_label(subtilis_ir_match_state_t *state,
				       size_t instr_label, size_t match_label,
				       subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_match_pair_t *pair;

	for (i = 0;
	     i < state->labels_count && state->labels[i].key != match_label;
	     i++)
		;
	if (i == state->labels_count) {
		if (i == MAX_MATCH_WILDCARDS) {
			subtilis_error_set_assertion_failed(err);
			return false;
		}
		pair = &state->labels[i];
		pair->key = match_label;
		pair->value = instr_label;
		state->labels_count++;
		return true;
	}

	return state->labels[i].value == instr_label;
}

static bool prv_process_floating_reg(subtilis_ir_match_state_t *state,
				     size_t instr_reg, size_t match_reg,
				     subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_match_pair_t *pair;

	for (i = 0; i < state->regs_count && state->regs[i].key != match_reg;
	     i++)
		;
	if (i == state->regs_count) {
		if (i == MAX_MATCH_WILDCARDS) {
			subtilis_error_set_assertion_failed(err);
			return false;
		}
		pair = &state->regs[i];
		pair->key = match_reg;
		pair->value = instr_reg;
		state->regs_count++;
		return true;
	}

	return state->regs[i].value == instr_reg;
}

static bool prv_process_floating_freg(subtilis_ir_match_state_t *state,
				      size_t instr_reg, size_t match_reg,
				      subtilis_error_t *err)
{
	size_t i;
	subtilis_ir_match_pair_t *pair;

	for (i = 0; i < state->fregs_count && state->fregs[i].key != match_reg;
	     i++)
		;
	if (i == state->fregs_count) {
		if (i == MAX_MATCH_WILDCARDS) {
			subtilis_error_set_assertion_failed(err);
			return false;
		}
		pair = &state->fregs[i];
		pair->key = match_reg;
		pair->value = instr_reg;
		state->fregs_count++;
		return true;
	}

	return state->fregs[i].value == instr_reg;
}

static bool prv_match_op(subtilis_ir_op_t *op, subtilis_ir_op_match_t *match,
			 subtilis_ir_match_state_t *state,
			 subtilis_error_t *err)
{
	size_t j;
	subtilis_ir_inst_t *instr;
	subtilis_ir_inst_match_t *match_instr;
	const subtilis_ir_class_info_t *details;

	if (op->type != match->type)
		return false;
	if (prv_is_call(op))
		return true;
	if (op->type == SUBTILIS_OP_SYS_CALL)
		return true;
	if (op->type == SUBTILIS_OP_LABEL)
		return prv_process_floating_label(state, op->op.label,
						  match->op.label.label, err);

	instr = &op->op.instr;
	match_instr = &match->op.instr;
	if (instr->type != match_instr->type)
		return false;

	details = &class_details[op_desc[match->op.instr.type].cls];

	for (j = 0; j < details->op_count; j++) {
		if (match_instr->op_match[j] == SUBTILIS_OP_MATCH_ANY)
			continue;
		if (match_instr->op_match[j] == SUBTILIS_OP_MATCH_FIXED) {
			switch (details->classes[j]) {
			case SUBTILIS_IR_OPERAND_FREGISTER:
			case SUBTILIS_IR_OPERAND_REGISTER:
				if (match_instr->operands[j].reg ==
				    instr->operands[j].reg)
					continue;
				break;
			case SUBTILIS_IR_OPERAND_I32:
				if (match_instr->operands[j].integer ==
				    instr->operands[j].integer)
					continue;
				break;
			case SUBTILIS_IR_OPERAND_REAL:
				if (match_instr->operands[j].real ==
				    instr->operands[j].real)
					continue;
				break;
			default:
				subtilis_error_set_assertion_failed(err);
				return false;
			}
			return false;
		}
		switch (details->classes[j]) {
		case SUBTILIS_IR_OPERAND_REGISTER:
			if (prv_process_floating_reg(
				state, instr->operands[j].reg,
				match_instr->operands[j].reg, err))
				continue;
			break;
		case SUBTILIS_IR_OPERAND_FREGISTER:
			if (prv_process_floating_freg(
				state, instr->operands[j].reg,
				match_instr->operands[j].reg, err))
				continue;
			break;
		case SUBTILIS_IR_OPERAND_LABEL:
			if (prv_process_floating_label(
				state, instr->operands[j].label,
				match_instr->operands[j].label, err))
				continue;
			break;
		default:
			subtilis_error_set_assertion_failed(err);
			return false;
		}
		return false;
	}

	return true;
}

void subtilis_ir_match(subtilis_ir_section_t *s, subtilis_ir_rule_t *rules,
		       size_t rule_count, void *user_data,
		       subtilis_error_t *err)
{
	size_t pc;
	size_t i;
	size_t j;
	subtilis_ir_match_state_t state;
	subtilis_ir_op_t *op;

	if (s->in_error_handler) {
		subtilis_error_set_assertion_failed(err);
		return;
	}

	for (pc = 0; pc < s->len;) {
		op = s->ops[pc];
		if ((op->type == SUBTILIS_OP_INSTR) &&
		    (op->op.instr.type == SUBTILIS_OP_INSTR_NOP)) {
			pc++;
			continue;
		}
		for (i = 0; i < rule_count; i++) {
			state.regs_count = 0;
			state.fregs_count = 0;
			state.labels_count = 0;
			for (j = 0;
			     j < rules[i].matches_count && pc + j < s->len;
			     j++) {
				op = s->ops[pc + j];
				if (!prv_match_op(op, &rules[i].matches[j],
						  &state, err))
					break;
			}
			if (err->type != SUBTILIS_ERROR_OK)
				return;
			if (j == rules[i].matches_count) {
				rules[i].action(s, pc, user_data, err);
				if (err->type != SUBTILIS_ERROR_OK)
					return;
				pc += j;
				break;
			}
		}
		if (i == rule_count) {
			subtilis_error_set_assertion_failed(err);
			return;
		}
	}
}
