/*
 * Copyright (c) 2019 Mark Ryan
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

#include "variable.h"
#include "expression.h"

void subtilis_var_assign_to_reg(subtilis_parser_t *p, subtilis_token_t *t,
				const char *tbuf, size_t loc, subtilis_exp_t *e,
				subtilis_error_t *err)
{
	subtilis_op_instr_type_t instr;
	subtilis_ir_operand_t op0;
	subtilis_ir_operand_t op2;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		instr = SUBTILIS_OP_INSTR_MOVI_I32;
		break;
	case SUBTILIS_EXP_INTEGER:
		instr = SUBTILIS_OP_INSTR_MOV;
		break;
	case SUBTILIS_EXP_CONST_REAL:
		instr = SUBTILIS_OP_INSTR_MOVI_REAL;
		break;
	case SUBTILIS_EXP_REAL:
		instr = SUBTILIS_OP_INSTR_MOVFP;
		break;
	case SUBTILIS_EXP_STRING:
	default:
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		goto cleanup;
	}
	op0.reg = loc;
	op2.integer = 0;
	subtilis_ir_section_add_instr_reg(p->current, instr, op0, e->exp.ir_op,
					  op2, err);

cleanup:
	subtilis_exp_delete(e);
}

void subtilis_var_assign_to_mem(subtilis_parser_t *p, const char *tbuf,
				subtilis_ir_operand_t op1, size_t loc,
				subtilis_exp_t *e, subtilis_error_t *err)
{
	size_t reg;
	subtilis_op_instr_type_t instr;
	subtilis_ir_operand_t op2;

	switch (e->type) {
	case SUBTILIS_EXP_CONST_INTEGER:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_I32, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_delete(e);
		e = subtilis_exp_new_var(SUBTILIS_EXP_INTEGER, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		instr = SUBTILIS_OP_INSTR_STOREO_I32;
		break;
	case SUBTILIS_EXP_CONST_REAL:
		reg = subtilis_ir_section_add_instr2(
		    p->current, SUBTILIS_OP_INSTR_MOVI_REAL, e->exp.ir_op, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		subtilis_exp_delete(e);
		e = subtilis_exp_new_var(SUBTILIS_EXP_REAL, reg, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
		instr = SUBTILIS_OP_INSTR_STOREO_REAL;
		break;
	case SUBTILIS_EXP_CONST_STRING:
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		goto cleanup;
	case SUBTILIS_EXP_INTEGER:
		instr = SUBTILIS_OP_INSTR_STOREO_I32;
		break;
	case SUBTILIS_EXP_REAL:
		instr = SUBTILIS_OP_INSTR_STOREO_REAL;
		break;
	case SUBTILIS_EXP_STRING:
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		goto cleanup;
	}
	op2.integer = loc;
	subtilis_ir_section_add_instr_reg(p->current, instr, e->exp.ir_op, op1,
					  op2, err);

cleanup:
	subtilis_exp_delete(e);
}

void subtilis_var_assign_hidden(subtilis_parser_t *p, const char *var_name,
				subtilis_type_t id_type, subtilis_exp_t *e,
				subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_ir_operand_t op1;

	s = subtilis_symbol_table_lookup(p->st, var_name);
	if (!s) {
		s = subtilis_symbol_table_insert(p->st, var_name, id_type, err);
		if (err->type != SUBTILIS_ERROR_OK)
			goto cleanup;
	}

	op1.reg = SUBTILIS_IR_REG_GLOBAL;

	e = subtilis_exp_coerce_type(p, e, s->t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	if (s->is_reg)
		subtilis_var_assign_to_reg(p, NULL, var_name, s->loc, e, err);
	else
		subtilis_var_assign_to_mem(p, var_name, op1, s->loc, e, err);

	return;

cleanup:

	subtilis_exp_delete(e);
}

subtilis_exp_t *subtilis_var_lookup_var(subtilis_parser_t *p, const char *tbuf,
					subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	const subtilis_symbol_t *s;
	subtilis_exp_type_t type;
	subtilis_op_instr_type_t itype;

	op1.reg = SUBTILIS_IR_REG_LOCAL;
	s = subtilis_symbol_table_lookup(p->local_st, tbuf);
	if (!s) {
		s = subtilis_symbol_table_lookup(p->st, tbuf);
		if (!s) {
			subtilis_error_set_unknown_variable(
			    err, tbuf, p->l->stream->name, p->l->line);
			return NULL;
		}
		op1.reg = SUBTILIS_IR_REG_GLOBAL;
	}

	if (s->t == SUBTILIS_TYPE_INTEGER) {
		type = SUBTILIS_EXP_INTEGER;
		itype = SUBTILIS_OP_INSTR_LOADO_I32;
	} else if (s->t == SUBTILIS_TYPE_REAL) {
		type = SUBTILIS_EXP_REAL;
		itype = SUBTILIS_OP_INSTR_LOADO_REAL;
	} else {
		subtilis_error_set_assertion_failed(err);
		return NULL;
	}
	if (!s->is_reg) {
		op2.integer = s->loc;
		reg = subtilis_ir_section_add_instr(p->current, itype, op1, op2,
						    err);
		if (err->type != SUBTILIS_ERROR_OK)
			return NULL;
	} else {
		reg = s->loc;
	}

	return subtilis_exp_new_var(type, reg, err);
}
