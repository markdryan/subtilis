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

#include "array_type.h"
#include "globals.h"
#include "parser.h"
#include "parser_array.h"
#include "parser_assignment.h"
#include "parser_call.h"
#include "parser_compound.h"
#include "parser_cond.h"
#include "parser_error.h"
#include "parser_file.h"
#include "parser_graphics.h"
#include "parser_loops.h"
#include "parser_mem.h"
#include "parser_os.h"
#include "parser_output.h"
#include "parser_string.h"
#include "reference_type.h"
#include "string_type.h"
#include "type_if.h"
#include "variable.h"

#define SUBTILIS_MAIN_FN "subtilis_main"

subtilis_parser_t *subtilis_parser_new(subtilis_lexer_t *l,
				       const subtilis_backend_t *backend,
				       const subtilis_settings_t *settings,
				       subtilis_error_t *err)
{
	const subtilis_symbol_t *s;
	subtilis_parser_t *p = calloc(1, sizeof(*p));
	subtilis_type_section_t *stype = NULL;

	if (!p) {
		subtilis_error_set_oom(err);
		goto on_error;
	}

	p->settings = *settings;

	p->st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->main_st = subtilis_symbol_table_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	p->local_st = p->main_st;

	p->prog = subtilis_ir_prog_new(&p->settings, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	stype = subtilis_type_section_new(&subtilis_type_void, 0, NULL, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	/*
	 * We pre-seed the symbol table here to ensure that these variables
	 * have offsets 0 and 0 + sizeof(INT).
	 */

	s = subtilis_symbol_table_insert(p->st, subtilis_eflag_hidden_var,
					 &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->eflag_offset = (int32_t)s->loc;
	p->error_offset = s->size;
	s = subtilis_symbol_table_insert(p->st, subtilis_err_hidden_var,
					 &subtilis_type_integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	p->current = subtilis_ir_prog_section_new(
	    p->prog, SUBTILIS_MAIN_FN, 0, stype, SUBTILIS_BUILTINS_MAX,
	    l->stream->name, l->line, p->eflag_offset, p->error_offset, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;
	stype = NULL;
	p->main = p->current;

	p->l = l;
	p->backend = *backend;
	p->level = 0;

	return p;

on_error:

	subtilis_type_section_delete(stype);
	subtilis_parser_delete(p);

	return NULL;
}

void subtilis_parser_delete(subtilis_parser_t *p)
{
	size_t i;

	if (!p)
		return;

	for (i = 0; i < p->num_calls; i++)
		subtilis_parser_call_delete(p->calls[i]);
	free(p->calls);

	subtilis_ir_prog_delete(p->prog);
	subtilis_symbol_table_delete(p->main_st);
	subtilis_symbol_table_delete(p->st);
	free(p);
}

typedef void (*subtilis_keyword_fn)(subtilis_parser_t *p, subtilis_token_t *,
				    subtilis_error_t *);

static const subtilis_keyword_fn keyword_fns[];

static void prv_dim(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	if (p->current != p->main) {
		subtilis_error_dim_in_proc(err, p->l->stream->name, p->l->line);
		return;
	}

	subtilis_parser_create_array(p, t, false, err);
}

static void prv_end(subtilis_parser_t *p, subtilis_token_t *t,
		    subtilis_error_t *err)
{
	subtilis_ir_operand_t end_label;

	if (p->current != p->main) {
		subtilis_ir_section_add_instr_no_arg(
		    p->current, SUBTILIS_OP_INSTR_END, err);
	} else {
		end_label.label = p->current->nofree_label;
		subtilis_ir_section_add_instr_no_reg(
		    p->current, SUBTILIS_OP_INSTR_JMP, end_label, err);
	}
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->endproc = true;
}

void subtilis_parser_statement(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	subtilis_keyword_fn fn;
	const char *tbuf;
	int key_type;

	if ((p->current->endproc) &&
	    !((t->type == SUBTILIS_TOKEN_KEYWORD) &&
	      (t->tok.keyword.type == SUBTILIS_KEYWORD_DEF))) {
		subtilis_error_set_useless_statement(err, p->l->stream->name,
						     p->l->line);
		return;
	}

	tbuf = subtilis_token_get_text(t);
	if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
		subtilis_parser_assignment(p, t, err);
		return;
	} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
		   !strcmp(tbuf, "<-")) {
		subtilis_parser_return(p, t, err);
		return;
	} else if (t->type != SUBTILIS_TOKEN_KEYWORD) {
		subtilis_error_set_keyword_expected(
		    err, tbuf, p->l->stream->name, p->l->line);
		return;
	}

	key_type = t->tok.keyword.type;
	fn = keyword_fns[key_type];
	if (!fn) {
		tbuf = subtilis_token_get_text(t);
		subtilis_error_set_not_supported(err, tbuf, p->l->stream->name,
						 p->l->line);
		return;
	}
	fn(p, t, err);
}

int subtilis_parser_if_compound(subtilis_parser_t *p, subtilis_token_t *t,
				subtilis_error_t *err)
{
	const char *tbuf;
	subtilis_keyword_fn fn;
	subtilis_ir_operand_t var_reg;
	int key_type = SUBTILIS_KEYWORD_MAX;
	unsigned int start;

	subtilis_symbol_table_level_up(p->local_st, p->l, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return key_type;
	p->level++;
	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return key_type;
	start = p->l->line;
	while (t->type != SUBTILIS_TOKEN_EOF) {
		tbuf = subtilis_token_get_text(t);
		if (t->type == SUBTILIS_TOKEN_IDENTIFIER) {
			if (p->current->endproc) {
				subtilis_error_set_useless_statement(
				    err, p->l->stream->name, p->l->line);
				return key_type;
			}
			subtilis_parser_assignment(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return key_type;
			continue;
		} else if ((t->type == SUBTILIS_TOKEN_OPERATOR) &&
			   !strcmp(tbuf, "<-")) {
			subtilis_parser_return(p, t, err);
			if (err->type != SUBTILIS_ERROR_OK)
				return key_type;
			continue;
		} else if (t->type != SUBTILIS_TOKEN_KEYWORD) {
			subtilis_error_set_keyword_expected(
			    err, tbuf, p->l->stream->name, p->l->line);
			return key_type;
		}

		key_type = t->tok.keyword.type;
		if ((key_type == SUBTILIS_KEYWORD_ELSE) ||
		    (key_type == SUBTILIS_KEYWORD_ENDIF))
			break;

		if (p->current->endproc) {
			subtilis_error_set_useless_statement(
			    err, p->l->stream->name, p->l->line);
			return key_type;
		}

		fn = keyword_fns[key_type];
		if (!fn) {
			tbuf = subtilis_token_get_text(t);
			subtilis_error_set_not_supported(
			    err, tbuf, p->l->stream->name, p->l->line);
			return key_type;
		}
		fn(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return key_type;
	}

	if (t->type == SUBTILIS_TOKEN_EOF)
		subtilis_error_set_compund_not_term(err, p->l->stream->name,
						    start);

	p->current->endproc = false;

	var_reg.reg = SUBTILIS_IR_REG_LOCAL;
	subtilis_reference_deallocate_refs(p, var_reg, p->local_st, p->level,
					   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return key_type;

	p->level--;
	subtilis_symbol_table_level_down(p->local_st, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return key_type;
	p->current->handler_list =
	    subtilis_handler_list_truncate(p->current->handler_list, p->level);
	return key_type;
}

static void prv_proc(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	(void)subtilis_parser_call(p, t, err);
}

static void prv_initialise_free_mem(subtilis_parser_t *p, subtilis_token_t *t,
				    subtilis_error_t *err)
{
	size_t reg;
	subtilis_exp_t *e;

	if (!p->settings.check_mem_leaks)
		return;

	reg = subtilis_ir_section_add_instr1(p->current,
					     SUBTILIS_OP_INSTR_HEAP_FREE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	e = subtilis_exp_new_int32_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_heap_free_on_startup_var,
				   &subtilis_type_integer, e, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
}

static void prv_check_free_mem(subtilis_parser_t *p, subtilis_token_t *t,
			       subtilis_error_t *err)
{
	size_t reg;
	subtilis_ir_operand_t msg_ptr;
	subtilis_ir_operand_t msg_len_reg;
	subtilis_ir_operand_t msg_len;
	subtilis_ir_operand_t leak_label;
	subtilis_ir_operand_t no_leak_label;
	const char *msg = " BYTES LEAKED!";
	subtilis_exp_t *old_value = NULL;
	subtilis_exp_t *new_value = NULL;

	if (!p->settings.check_mem_leaks)
		return;

	leak_label.label = subtilis_ir_section_new_label(p->current);
	no_leak_label.label = subtilis_ir_section_new_label(p->current);

	old_value =
	    subtilis_var_lookup_var(p, subtilis_heap_free_on_startup_var, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	reg = subtilis_ir_section_add_instr1(p->current,
					     SUBTILIS_OP_INSTR_HEAP_FREE, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	new_value = subtilis_exp_new_int32_var(reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	old_value = subtilis_type_if_sub(p, old_value, new_value, err);
	new_value = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_instr_reg(p->current, SUBTILIS_OP_INSTR_JMPC,
					  old_value->exp.ir_op, leak_label,
					  no_leak_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_section_add_label(p->current, leak_label.label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/*
	 * Any error here would cause the program to loop and possibly never
	 * end.  This is a debug feature but still.  We know dectostr will
	 * not generate an error but we need to ensure backends with native
	 * support for this function won't either.
	 */

	p->settings.ignore_graphics_errors = true;

	subtilis_type_if_print(p, old_value, err);
	old_value = NULL;
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	msg_len.integer = strlen(msg);
	msg_ptr.reg =
	    subtilis_string_type_lca_const(p, msg, msg_len.integer, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	msg_len_reg.reg = subtilis_ir_section_add_instr2(
	    p->current, SUBTILIS_OP_INSTR_MOVI_I32, msg_len, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_reg2(
	    p->current, SUBTILIS_OP_INSTR_PRINT_STR, msg_ptr, msg_len_reg, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, no_leak_label.label, err);

cleanup:

	subtilis_exp_delete(old_value);
	subtilis_exp_delete(new_value);
}

static void prv_root(subtilis_parser_t *p, subtilis_token_t *t,
		     subtilis_error_t *err)
{
	subtilis_exp_t *seed;

	seed = subtilis_exp_new_int32(0x3fffffff, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_rnd_hidden_var,
				   &subtilis_type_integer, seed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	seed = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_eflag_hidden_var,
				   &subtilis_type_integer, seed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	seed = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_var_assign_hidden(p, subtilis_err_hidden_var,
				   &subtilis_type_integer, seed, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_initialise_free_mem(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_lexer_get(p->l, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	p->current->cleanup_stack_nop =
	    subtilis_ir_section_add_nop(p->current, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	while (t->type != SUBTILIS_TOKEN_EOF) {
		subtilis_parser_statement(p, t, err);
		if (err->type != SUBTILIS_ERROR_OK)
			return;
	}
	p->main->locals = p->main_st->max_allocated;

	subtilis_ir_section_add_label(p->current, p->current->end_label, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_parser_unwind(p, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_label(p->current, p->current->nofree_label,
				      err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_check_free_mem(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_section_add_instr_no_arg(p->current, SUBTILIS_OP_INSTR_END,
					     err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_ir_merge_errors(p->current, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_array_gen_index_error_code(p, err);
}

void subtilis_parse(subtilis_parser_t *p, subtilis_error_t *err)
{
	subtilis_token_t *t = NULL;

	t = subtilis_token_new(err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	prv_root(p, t, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parser_check_calls(p, err);

cleanup:

	subtilis_token_delete(t);
}

/* The ordering of this table is very important.  The functions it
 * contains must correspond to the enumerated types in
 * keywords.h and basic_keywords.h.  Note the first three keywords
 * are common keywords the lexer knows about and so come from
 * keywords.h.
 */

/* clang-format off */
static const subtilis_keyword_fn keyword_fns[] = {
	NULL, /* SUBTILIS_KEYWORD_FN */
	prv_proc, /* SUBTILIS_KEYWORD_PROC */
	NULL, /* SUBTILIS_KEYWORD_REM */
	NULL, /* SUBTILIS_KEYWORD_ABS */
	NULL, /* SUBTILIS_KEYWORD_ACS */
	NULL, /* SUBTILIS_KEYWORD_ADVAL */
	NULL, /* SUBTILIS_KEYWORD_AND */
	subtilis_parser_append, /* SUBTILIS_KEYWORD_APPEND */
	NULL, /* SUBTILIS_KEYWORD_ASC */
	NULL, /* SUBTILIS_KEYWORD_ASN */
	NULL, /* SUBTILIS_KEYWORD_ATN */
	NULL, /* SUBTILIS_KEYWORD_BEAT */
	NULL, /* SUBTILIS_KEYWORD_BEATS */
	NULL, /* SUBTILIS_KEYWORD_BGET_HASH */
	subtilis_parser_bput, /* SUBTILIS_KEYWORD_BPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_BY */
	NULL, /* SUBTILIS_KEYWORD_CALL */
	NULL, /* SUBTILIS_KEYWORD_CASE */
	NULL, /* SUBTILIS_KEYWORD_CHR_STR */
	subtilis_parser_circle, /* SUBTILIS_KEYWORD_CIRCLE */
	subtilis_parser_clg, /* SUBTILIS_KEYWORD_CLG */
	subtilis_parser_close, /* SUBTILIS_KEYWORD_CLOSE_HASH */
	subtilis_parser_cls, /* SUBTILIS_KEYWORD_CLS */
	subtilis_parser_colour, /* SUBTILIS_KEYWORD_COLOR */
	subtilis_parser_colour, /* SUBTILIS_KEYWORD_COLOUR */
	subtilis_parser_copy, /* SUBTILIS_KEYWORD_COPY */
	NULL, /* SUBTILIS_KEYWORD_COS */
	NULL, /* SUBTILIS_KEYWORD_COUNT */
	subtilis_parser_def, /* SUBTILIS_KEYWORD_DEF */
	NULL, /* SUBTILIS_KEYWORD_DEG */
	prv_dim, /* SUBTILIS_KEYWORD_DIM */
	NULL, /* SUBTILIS_KEYWORD_DIV */
	subtilis_parser_draw, /* SUBTILIS_KEYWORD_DRAW */
	NULL, /* SUBTILIS_KEYWORD_ELLIPSE */
	NULL, /* SUBTILIS_KEYWORD_ELSE */
	prv_end, /* SUBTILIS_KEYWORD_END */
	NULL, /* SUBTILIS_KEYWORD_ENDCASE */
	NULL, /* SUBTILIS_KEYWORD_ENDERROR */
	NULL, /* SUBTILIS_KEYWORD_ENDIF */
	subtilis_parser_endproc, /* SUBTILIS_KEYWORD_ENDPROC */
	NULL, /* SUBTILIS_KEYWORD_ENDRANGE */
	NULL, /* SUBTILIS_KEYWORD_ENDTRY */
	NULL, /* SUBTILIS_KEYWORD_ENDWHILE */
	NULL, /* SUBTILIS_KEYWORD_EOF_HASH */
	NULL, /* SUBTILIS_KEYWORD_EOR */
	NULL, /* SUBTILIS_KEYWORD_ERL */
	NULL, /* SUBTILIS_KEYWORD_ERR */
	subtilis_parser_error, /* SUBTILIS_KEYWORD_ERROR */
	NULL, /* SUBTILIS_KEYWORD_EVAL */
	NULL, /* SUBTILIS_KEYWORD_EXP */
	NULL, /* SUBTILIS_KEYWORD_EXT_HASH */
	NULL, /* SUBTILIS_KEYWORD_FALSE */
	subtilis_parser_fill, /* SUBTILIS_KEYWORD_FILL */
	subtilis_parser_for, /* SUBTILIS_KEYWORD_FOR */
	subtilis_parser_gcol, /* SUBTILIS_KEYWORD_GCOL */
	NULL, /* SUBTILIS_KEYWORD_GET */
	NULL, /* SUBTILIS_KEYWORD_GET_HASH */
	NULL, /* SUBTILIS_KEYWORD_GET_STR */
	NULL, /* SUBTILIS_KEYWORD_GET_STR_HASH */
	NULL, /* SUBTILIS_KEYWORD_HEAP_FREE */
	subtilis_parser_if, /* SUBTILIS_KEYWORD_IF */
	NULL, /* SUBTILIS_KEYWORD_INKEY */
	NULL, /* SUBTILIS_KEYWORD_INKEY_STR */
	NULL, /* SUBTILIS_KEYWORD_INPUT */
	NULL, /* SUBTILIS_KEYWORD_INPUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_INSTR */
	NULL, /* SUBTILIS_KEYWORD_INT */
	NULL, /* SUBTILIS_KEYWORD_INTZ */
	subtilis_parser_left_str, /* SUBTILIS_KEYWORD_LEFT_STR */
	NULL, /* SUBTILIS_KEYWORD_LEN */
	subtilis_parser_let, /* SUBTILIS_KEYWORD_LET */
	subtilis_parser_line, /* SUBTILIS_KEYWORD_LINE */
	NULL, /* SUBTILIS_KEYWORD_LN */
	subtilis_parser_local, /* SUBTILIS_KEYWORD_LOCAL */
	NULL, /* SUBTILIS_KEYWORD_LOG */
	subtilis_parser_mid_str, /* SUBTILIS_KEYWORD_MID_STR */
	NULL, /* SUBTILIS_KEYWORD_MOD */
	subtilis_parser_mode, /* SUBTILIS_KEYWORD_MODE */
	NULL, /* SUBTILIS_KEYWORD_MOUSE */
	subtilis_parser_move, /* SUBTILIS_KEYWORD_MOVE */
	NULL, /* SUBTILIS_KEYWORD_NEXT */
	NULL, /* SUBTILIS_KEYWORD_NOT */
	NULL, /* SUBTILIS_KEYWORD_OF */
	subtilis_parser_off, /* SUBTILIS_KEYWORD_OFF */
	subtilis_parser_on, /* SUBTILIS_KEYWORD_ON */
	subtilis_parser_onerror, /* SUBTILIS_KEYWORD_ONERROR */
	NULL, /* SUBTILIS_KEYWORD_OPENIN */
	NULL, /* SUBTILIS_KEYWORD_OPENOUT */
	NULL, /* SUBTILIS_KEYWORD_OPENUP */
	NULL, /* SUBTILIS_KEYWORD_OR */
	subtilis_parser_origin, /* SUBTILIS_KEYWORD_ORIGIN */
	subtilis_parser_oscli, /* SUBTILIS_KEYWORD_OSCLI */
	NULL, /* SUBTILIS_KEYWORD_OTHERWISE */
	NULL, /* SUBTILIS_KEYWORD_PI */
	subtilis_parser_plot, /* SUBTILIS_KEYWORD_PLOT */
	subtilis_parser_point, /* SUBTILIS_KEYWORD_POINT */
	NULL, /* SUBTILIS_KEYWORD_POS */
	subtilis_parser_print, /* SUBTILIS_KEYWORD_PRINT */
	NULL, /* SUBTILIS_KEYWORD_PRINT_HASH */
	subtilis_parser_set_ptr, /* SUBTILIS_KEYWORD_PTR_HASH */
	subtilis_parser_put_hash, /* SUBTILIS_KEYWORD_PUT_HASH */
	NULL, /* SUBTILIS_KEYWORD_QUIT */
	NULL, /* SUBTILIS_KEYWORD_RAD */
	subtilis_parser_range, /* SUBTILIS_KEYWORD_RANGE */
	subtilis_parser_rectangle, /* SUBTILIS_KEYWORD_RECTANGLE */
	subtilis_parser_repeat, /* SUBTILIS_KEYWORD_REPEAT */
	NULL, /* SUBTILIS_KEYWORD_REPORT */
	NULL, /* SUBTILIS_KEYWORD_REPORT_STR */
	NULL, /* SUBTILIS_KEYWORD_RETURN */
	subtilis_parser_right_str, /* SUBTILIS_KEYWORD_RIGHT_STR */
	NULL, /* SUBTILIS_KEYWORD_RND */
	NULL, /* SUBTILIS_KEYWORD_SGN */
	NULL, /* SUBTILIS_KEYWORD_SIN */
	NULL, /* SUBTILIS_KEYWORD_SOUND */
	NULL, /* SUBTILIS_KEYWORD_SPC */
	NULL, /* SUBTILIS_KEYWORD_SQR */
	NULL, /* SUBTILIS_KEYWORD_STEP */
	NULL, /* SUBTILIS_KEYWORD_STEREO */
	NULL, /* SUBTILIS_KEYWORD_STOP */
	NULL, /* SUBTILIS_KEYWORD_STRING_STR */
	NULL, /* SUBTILIS_KEYWORD_STRS */
	NULL, /* SUBTILIS_KEYWORD_SUM */
	NULL, /* SUBTILIS_KEYWORD_SUMLEN */
	NULL, /* SUBTILIS_KEYWORD_SWAP */
	subtilis_parser_sys, /* SUBTILIS_KEYWORD_SYS */
	NULL, /* SUBTILIS_KEYWORD_TAB */
	NULL, /* SUBTILIS_KEYWORD_TAN */
	NULL, /* SUBTILIS_KEYWORD_TEMPO */
	NULL, /* SUBTILIS_KEYWORD_THEN */
	NULL, /* SUBTILIS_KEYWORD_TIME */
	NULL, /* SUBTILIS_KEYWORD_TIME_STR */
	NULL, /* SUBTILIS_KEYWORD_TINT */
	NULL, /* SUBTILIS_KEYWORD_TO */
	NULL, /* SUBTILIS_KEYWORD_TRUE */
	subtilis_parser_try, /* SUBTILIS_KEYWORD_TRY */
	subtilis_parser_try_one, /* SUBTILIS_KEYWORD_TRY */
	NULL, /* SUBTILIS_KEYWORD_UNTIL */
	NULL, /* SUBTILIS_KEYWORD_USR */
	NULL, /* SUBTILIS_KEYWORD_VAL */
	subtilis_parser_vdu, /* SUBTILIS_KEYWORD_VDU */
	NULL, /* SUBTILIS_KEYWORD_VOICES */
	NULL, /* SUBTILIS_KEYWORD_VPOS */
	subtilis_parser_wait, /* SUBTILIS_KEYWORD_WAIT */
	NULL, /* SUBTILIS_KEYWORD_WHEN */
	subtilis_parser_while, /* SUBTILIS_KEYWORD_WHILE */
	NULL, /* SUBTILIS_KEYWORD_WIDTH */
};

/* clang-format on */
