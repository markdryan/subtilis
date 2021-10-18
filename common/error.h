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

#ifndef __SUBTILIS_ERROR_H
#define __SUBTILIS_ERROR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"

typedef enum {
	SUBTILIS_ERROR_OK,
	SUBTILIS_ERROR_OOM,
	SUBTILIS_ERROR_OPEN,
	SUBTILIS_ERROR_READ,
	SUBTILIS_ERROR_WRITE,
	SUBTILIS_ERROR_CLOSE,
	SUBTILIS_ERROR_UNTERMINATED_STRING,
	SUBTILIS_ERROR_STRING_TOO_LONG,
	SUBTILIS_ERROR_NUMBER_TOO_LONG,
	SUBTILIS_ERROR_IDENTIFIER_TOO_LONG,
	SUBTILIS_ERROR_UNKNOWN_TOKEN,
	SUBTILIS_ERROR_BAD_PROC_NAME,
	SUBTILIS_ERROR_BAD_FN_NAME,
	SUBTILIS_ERROR_BAD_LAMBDA_NAME,
	SUBTILIS_ERROR_BAD_TYPE_NAME,
	SUBTILIS_ERROR_ASSERTION_FAILED,
	SUBTILIS_ERROR_KEYWORD_EXPECTED,
	SUBTILIS_ERROR_KEYWORD_UNEXPECTED,
	SUBTILIS_ERROR_ID_EXPECTED,
	SUBTILIS_ERROR_NOT_SUPPORTED,
	SUBTILIS_ERROR_ASSIGNMENT_OP_EXPECTED,
	SUBTILIS_ERROR_EXP_EXPECTED,
	SUBTILIS_ERROR_RIGHT_BKT_EXPECTED,
	SUBTILIS_ERROR_INTEGER_EXPECTED,
	SUBTILIS_ERROR_BYTE_EXPECTED,
	SUBTILIS_ERROR_BAD_EXPRESSION,
	SUBTILIS_ERROR_DIVIDE_BY_ZERO,
	SUBTILIS_ERROR_UNKNOWN_VARIABLE,
	SUBTILIS_ERROR_INTEGER_EXP_EXPECTED,
	SUBTILIS_ERROR_EXPECTED,
	SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	SUBTILIS_ERROR_WALKER_FAILED,
	SUBTILIS_ERROR_ALREADY_DEFINED,
	SUBTILIS_ERROR_NESTED_PROCEDURE,
	SUBTILIS_ERROR_PROCEDURE_EXPECTED,
	SUBTILIS_ERROR_FUNCTION_EXPECTED,
	SUBTILIS_ERROR_UNKNOWN_PROCEDURE,
	SUBTILIS_ERROR_UNKNOWN_FUNCTION,
	SUBTILIS_ERROR_UNKNOWN_TYPE,
	SUBTILIS_ERROR_BAD_INSTRUCTION,
	SUBTILIS_ERROR_BAD_ARG_COUNT,
	SUBTILIS_ERROR_TOO_MANY_ARGS,
	SUBTILIS_ERROR_BAD_ARG_TYPE,
	SUBTILIS_ERROR_ZERO_STEP,
	SUBTILIS_ERROR_NUMERIC_EXPECTED,
	SUBTILIS_ERROR_ENDPROC_IN_MAIN,
	SUBTILIS_ERROR_USELESS_STATEMENT,
	SUBTILIS_ERROR_RETURN_IN_MAIN,
	SUBTILIS_ERROR_RETURN_IN_PROC,
	SUBTILIS_ERROR_NESTED_HANDLER,
	SUBTILIS_ERROR_HANDLER_IN_TRY,
	SUBTILIS_ERROR_ERROR_IN_HANDLER,
	SUBTILIS_ERROR_ENDPROC_IN_FN,
	SUBTILIS_ERROR_RETURN_EXPECTED,
	SUBTILIS_ERROR_TOO_MANY_DIMS,
	SUBTILIS_ERROR_DIM_IN_PROC,
	SUBTILIS_ERROR_BAD_DIM,
	SUBTILIS_ERROR_BAD_INDEX,
	SUBTILIS_ERROR_BAD_INDEX_COUNT,
	SUBTILIS_ERROR_NOT_ARRAY,
	SUBTILIS_ERROR_NOT_VECTOR,
	SUBTILIS_ERROR_NOT_ARRAY_OR_VECTOR,
	SUBTILIS_ERROR_VARIABLE_BAD_LEVEL,
	SUBTILIS_ERROR_BAD_ERROR,
	SUBTILIS_ERROR_TOO_MANY_BLOCKS,
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH,
	SUBTILIS_ERROR_FN_TYPE_MISMATCH,
	SUBTILIS_ERROR_CONST_INTEGER_EXPECTED,
	SUBTILIS_ERROR_NUMERIC_EXP_EXPECTED,
	SUBTILIS_ERROR_BAD_CONVERSION,
	SUBTILIS_ERROR_BAD_ZERO_EXTEND,
	SUBTILIS_ERROR_BAD_ELEMENT_COUNT,
	SUBTILIS_ERROR_CONST_EXPRESSION_EXPECTED,
	SUBTILIS_ERROR_STRING_EXPECTED,
	SUBTILIS_ERROR_CONST_STRING_EXPECTED,
	SUBTILIS_ERROR_STRING_VARIABLE_EXPECTED,
	SUBTILIS_ERROR_BAD_VAL_ARG,
	SUBTILIS_ERROR_SYS_BAD_ARGS,
	SUBTILIS_ERROR_SYS_TOO_MANY_ARGS,
	SUBTILIS_ERROR_INTEGER_VARIABLE_EXPECTED,
	SUBTILIS_ERROR_DEFPROC_SHOULD_BE_DEF_PROC,
	SUBTILIS_ERROR_DEFFN_SHOULD_BE_DEF_FN,
	SUBTILIS_ERROR_LOCAL_OBSCURES_GLOBAL,
	SUBTILIS_ERROR_TEMPORARY_NOT_ALLOWED,
	SUBTILIS_ERROR_RANGE_TYPE_MISMATCH,
	SUBTILIS_ERROR_ARRAY_IN_RANGE,
	SUBTILIS_ERROR_RANGE_BAD_VAR_COUNT,
	SUBTILIS_ERROR_TYPE_NOT_AT_TOP_LEVEL,
	SUBTILIS_ERROR_ASS_BAD_REG,
	SUBTILIS_ERROR_ASS_INTEGER_ENCODE,
	SUBTILIS_ERROR_SYS_CALL_UNKNOWN,
	SUBTILIS_ERROR_ASS_BAD_OFFSET,
	SUBTILIS_ERROR_ASS_BAD_RANGE,
	SUBTILIS_ERROR_ASS_BAD_ALIGNMENT,
	SUBTILIS_ERROR_ASS_KEYWORD_BAD_USE,
	SUBTILIS_ERROR_ASS_INT_TOO_BIG,
	SUBTILIS_ERROR_ASS_MISSING_LABEL,
	SUBTILIS_ERROR_ASS_BAD_ADR,
	SUBTILIS_ERROR_ASS_BAD_ALIGN,
	SUBTILIS_ERROR_ASS_BAD_REAL_IMM,
	SUBTILIS_ERROR_ASS_LABEL_MISSING_COLON,
} subtilis_error_type_t;

struct _subtilis_error_t {
	subtilis_error_type_t type;
	char data1[SUBTILIS_CONFIG_ERROR_LEN];
	char data2[SUBTILIS_CONFIG_ERROR_LEN];
	char file[SUBTILIS_CONFIG_PATH_MAX];
	unsigned int line;
	char subtilis_file[SUBTILIS_CONFIG_PATH_MAX];
	unsigned int subtilis_line;
};

typedef struct _subtilis_error_t subtilis_error_t;

/* TODO: Line numbers are not being correctly recorded in some error messages */

void subtilis_error_init(subtilis_error_t *e);
#define subtilis_error_set0(e, t, f, l)                                        \
	subtilis_error_set_full(e, t, "", "", f, l, __FILE__, __LINE__)
#define subtilis_error_set1(e, t, d1, f, l)                                    \
	subtilis_error_set_full(e, t, d1, "", f, l, __FILE__, __LINE__)
#define subtilis_error_set2(e, t, d1, d2, f, l)                                \
	subtilis_error_set_full(e, t, d1, d2, f, l, __FILE__, __LINE__)

#define subtilis_error_set_oom(e)                                              \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_OOM, __FILE__, __LINE__)
#define subtilis_error_set_file_open(e, path)                                  \
	subtilis_error_set_errno(e, SUBTILIS_ERROR_OPEN, path, __FILE__,       \
				 __LINE__)
#define subtilis_error_set_file_read(e)                                        \
	subtilis_error_set_errno(e, SUBTILIS_ERROR_READ, "", __FILE__, __LINE__)
#define subtilis_error_set_file_write(e)                                       \
	subtilis_error_set_errno(e, SUBTILIS_ERROR_WRITE, "", __FILE__,        \
				 __LINE__)
#define subtilis_error_set_file_close(e)                                       \
	subtilis_error_set_errno(e, SUBTILIS_ERROR_CLOSE, "", __FILE__,        \
				 __LINE__)
#define subtilis_error_set_unterminated_string(e, str, file, line)             \
	subtilis_error_set1(e, SUBTILIS_ERROR_UNTERMINATED_STRING, str, file,  \
			    line)
#define subtilis_error_set_string_too_long(e, str, file, line)                 \
	subtilis_error_set1(e, SUBTILIS_ERROR_STRING_TOO_LONG, str, file, line)
#define subtilis_error_set_number_too_long(e, str, file, line)                 \
	subtilis_error_set1(e, SUBTILIS_ERROR_NUMBER_TOO_LONG, str, file, line)
#define subtilis_error_set_identifier_too_long(e, str, file, line)             \
	subtilis_error_set1(e, SUBTILIS_ERROR_IDENTIFIER_TOO_LONG, str, file,  \
			    line)
#define subtilis_error_set_unknown_token(e, str, file, line)                   \
	subtilis_error_set1(e, SUBTILIS_ERROR_UNKNOWN_TOKEN, str, file, line)
#define subtilis_error_set_bad_proc_name(e, str, file, line)                   \
	subtilis_error_set1(e, SUBTILIS_ERROR_BAD_PROC_NAME, str, file, line)
#define subtilis_error_set_bad_fn_name(e, str, file, line)                     \
	subtilis_error_set1(e, SUBTILIS_ERROR_BAD_FN_NAME, str, file, line)
#define subtilis_error_set_bad_lambda_name(e, str, file, line)                 \
	subtilis_error_set1(e, SUBTILIS_ERROR_BAD_LAMBDA_NAME, str, file, line)
#define subtilis_error_set_bad_type_name(e, str, file, line)                   \
	subtilis_error_set1(e, SUBTILIS_ERROR_BAD_TYPE_NAME, str, file, line)
#define subtilis_error_set_assertion_failed(e)                                 \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_ASSERTION_FAILED, __FILE__, \
				 __LINE__)
#define subtilis_error_set_keyword_expected(e, str, file, line)                \
	subtilis_error_set1(e, SUBTILIS_ERROR_KEYWORD_EXPECTED, str, file, line)
#define subtilis_error_set_keyword_unexpected(e, str, file, line)              \
	subtilis_error_set1(e, SUBTILIS_ERROR_KEYWORD_UNEXPECTED, str, file,   \
			    line)
#define subtilis_error_set_not_supported(e, str, file, line)                   \
	subtilis_error_set1(e, SUBTILIS_ERROR_NOT_SUPPORTED, str, file, line)
#define subtilis_error_set_id_expected(e, str, file, line)                     \
	subtilis_error_set1(e, SUBTILIS_ERROR_ID_EXPECTED, str, file, line)
#define subtilis_error_set_assignment_op_expected(e, str, file, line)          \
	subtilis_error_set1(e, SUBTILIS_ERROR_ASSIGNMENT_OP_EXPECTED, str,     \
			    file, line)
#define subtilis_error_set_exp_expected(e, str, file, line)                    \
	subtilis_error_set1(e, SUBTILIS_ERROR_EXP_EXPECTED, str, file, line)
#define subtilis_error_set_right_bkt_expected(e, str, file, line)              \
	subtilis_error_set1(e, SUBTILIS_ERROR_RIGHT_BKT_EXPECTED, str, file,   \
			    line)
#define subtilis_error_set_integer_expected(e, str, file, line)                \
	subtilis_error_set1(e, SUBTILIS_ERROR_INTEGER_EXPECTED, str, file, line)
#define subtilis_error_set_byte_expected(e, str, file, line)                   \
	subtilis_error_set1(e, SUBTILIS_ERROR_BYTE_EXPECTED, str, file, line)
#define subtilis_error_set_bad_expression(e, file, line)                       \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_BAD_EXPRESSION, file,      \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_divide_by_zero(e, file, line)                       \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_DIVIDE_BY_ZERO, file,      \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_unknown_variable(e, str, file, line)                \
	subtilis_error_set1(e, SUBTILIS_ERROR_UNKNOWN_VARIABLE, str, file, line)
#define subtilis_error_set_integer_exp_expected(e, file, line)                 \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_INTEGER_EXP_EXPECTED, file, \
				 line)
#define subtilis_error_set_expected(e, exp, found, file, line)                 \
	subtilis_error_set2(e, SUBTILIS_ERROR_EXPECTED, exp, found, file, line)
#define subtilis_error_set_compund_not_term(e, file, line)                     \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_COMPOUND_NOT_TERM, file,   \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_walker_failed(e)                                    \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_WALKER_FAILED, __FILE__,    \
				 __LINE__)
#define subtilis_error_set_already_defined(e, name, file, line)                \
	subtilis_error_set1(e, SUBTILIS_ERROR_ALREADY_DEFINED, name, file, line)
#define subtilis_error_set_nested_procedure(e, file, line)                     \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_NESTED_PROCEDURE, file, line)
#define subtilis_error_set_procedure_expected(e, str, file, line)              \
	subtilis_error_set1(e, SUBTILIS_ERROR_PROCEDURE_EXPECTED, str, file,   \
			    line)
#define subtilis_error_set_function_expected(e, str, file, line)               \
	subtilis_error_set1(e, SUBTILIS_ERROR_FUNCTION_EXPECTED, str, file,    \
			    line)
#define subtilis_error_set_unknown_procedure(e, str, file, line)               \
	subtilis_error_set1(e, SUBTILIS_ERROR_UNKNOWN_PROCEDURE, str, file,    \
			    line)
#define subtilis_error_set_unknown_function(e, str, file, line)                \
	subtilis_error_set1(e, SUBTILIS_ERROR_UNKNOWN_FUNCTION, str, file, line)
#define subtilis_error_set_unknown_type(e, str, file, line)                    \
	subtilis_error_set1(e, SUBTILIS_ERROR_UNKNOWN_TYPE, str, file, line)
#define subtilis_error_set_bad_instruction(e, num)                             \
	subtilis_error_set_hex(e, SUBTILIS_ERROR_BAD_INSTRUCTION, num,         \
			       __FILE__, __LINE__)
#define subtilis_error_set_bad_arg_count(e, num1, num2, file, line)            \
	subtilis_error_set_int(e, SUBTILIS_ERROR_BAD_ARG_COUNT, num1, num2,    \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_too_many_args(e, num1, num2, file, line)            \
	subtilis_error_set_int(e, SUBTILIS_ERROR_TOO_MANY_ARGS, num1, num2,    \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_numeric_expected(e, id, file, line)                 \
	subtilis_error_set1(e, SUBTILIS_ERROR_NUMERIC_EXPECTED, id, file, line)
#define subtilis_error_set_zero_step(e, file, line)                            \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_ZERO_STEP, file, line)
#define subtilis_error_set_proc_in_main(e, file, line)                         \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_ENDPROC_IN_MAIN, file,     \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_useless_statement(e, file, line)                    \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_USELESS_STATEMENT, file,   \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_return_in_main(e, file, line)                       \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_RETURN_IN_MAIN, file,      \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_return_in_proc(e, file, line)                       \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_RETURN_IN_PROC, file,      \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_nested_handler(e, file, line)                       \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_NESTED_HANDLER, file, line)
#define subtilis_error_set_handler_in_try(e, file, line)                       \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_HANDLER_IN_TRY, file, line)
#define subtilis_error_set_error_handler(e, file, line)                        \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_ERROR_IN_HANDLER, file, line)
#define subtilis_error_set_proc_in_fn(e, file, line)                           \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_ENDPROC_IN_FN, file, line, \
				  __FILE__, __LINE__)
#define subtilis_error_return_expected(e, file, line)                          \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_RETURN_EXPECTED, file,     \
				  line, __FILE__, __LINE__)
#define subtilis_error_too_many_dims(e, str, file, line)                       \
	subtilis_error_set1(e, SUBTILIS_ERROR_TOO_MANY_DIMS, str, file, line)
#define subtilis_error_dim_in_proc(e, file, line)                              \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_DIM_IN_PROC, file, line,   \
				  __FILE__, __LINE__)
#define subtilis_error_bad_dim(e, str, file, line)                             \
	subtilis_error_set1(e, SUBTILIS_ERROR_BAD_DIM, str, file, line)
#define subtilis_error_bad_index(e, str, file, line)                           \
	subtilis_error_set1(e, SUBTILIS_ERROR_BAD_INDEX, str, file, line)
#define subtilis_error_bad_index_count(e, str, file, line)                     \
	subtilis_error_set1(e, SUBTILIS_ERROR_BAD_INDEX_COUNT, str, file, line)
#define subtilis_error_not_array(e, str, file, line)                           \
	subtilis_error_set1(e, SUBTILIS_ERROR_NOT_ARRAY, str, file, line)
#define subtilis_error_not_vector(e, str, file, line)                          \
	subtilis_error_set1(e, SUBTILIS_ERROR_NOT_VECTOR, str, file, line)
#define subtilis_error_not_array_or_vector(e, str, file, line)                 \
	subtilis_error_set1(e, SUBTILIS_ERROR_NOT_ARRAY_OR_VECTOR, str, file,  \
			    line)
#define subtilis_error_variable_bad_level(e, str, file, line)                  \
	subtilis_error_set1(e, SUBTILIS_ERROR_VARIABLE_BAD_LEVEL, str, file,   \
			    line)
#define subtilis_error_set_bad_error(e, num1, num2, file, line)                \
	subtilis_error_set_int(e, SUBTILIS_ERROR_BAD_ERROR, num1, num2, file,  \
			       line, __FILE__, __LINE__)
#define subtilis_error_set_too_many_blocks(e, file, line)                      \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_TOO_MANY_BLOCKS, file,     \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_array_type_mismatch(e, file, line)                  \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH, file, \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_fn_type_mismatch(e, file, line)                     \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_FN_TYPE_MISMATCH, file,    \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_const_integer_expected(e, file, line)               \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_CONST_INTEGER_EXPECTED,    \
				  file, line, __FILE__, __LINE__)
#define subtilis_error_set_numeric_exp_expected(e, file, line)                 \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_NUMERIC_EXP_EXPECTED,      \
				  file, line, __FILE__, __LINE__)
#define subtilis_error_set_bad_conversion(e, from, to, file, line)             \
	subtilis_error_set2(e, SUBTILIS_ERROR_BAD_CONVERSION, from, to, file,  \
			    line)
#define subtilis_error_set_bad_zero_extend(e, from, to, file, line)            \
	subtilis_error_set2(e, SUBTILIS_ERROR_BAD_ZERO_EXTEND, from, to, file, \
			    line)
#define subtilis_error_bad_element_count(e, file, line)                        \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_BAD_ELEMENT_COUNT, file,   \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_const_expression_expected(e, file, line)            \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_CONST_EXPRESSION_EXPECTED, \
				  file, line, __FILE__, __LINE__)
#define subtilis_error_set_string_expected(e, file, line)                      \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_STRING_EXPECTED, file,     \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_const_string_expected(e, file, line)                \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_CONST_STRING_EXPECTED,     \
				  file, line, __FILE__, __LINE__)
#define subtilis_error_set_string_variable_expected(e, file, line)             \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_STRING_VARIABLE_EXPECTED,  \
				  file, line, __FILE__, __LINE__)
#define subtilis_error_set_bad_val_arg(e, file, line)                          \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_BAD_VAL_ARG, file, line,   \
				  __FILE__, __LINE__)
#define subtilis_error_set_sys_bad_args(e, file, line)                         \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_SYS_BAD_ARGS, file, line,  \
				  __FILE__, __LINE__)
#define subtilis_error_set_sys_max_too_many_args(e, file, line)                \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_SYS_TOO_MANY_ARGS, file,   \
				  line, __FILE__, __LINE__)
#define subtilis_error_set_integer_variable_expected(e, found, file, line)     \
	subtilis_error_set1(e, SUBTILIS_ERROR_INTEGER_VARIABLE_EXPECTED,       \
			    found, file, line)
#define subtilis_error_set_defproc_should_be_def_proc(e, file, line)           \
	subtilis_error_set_syntax(e,                                           \
				  SUBTILIS_ERROR_DEFPROC_SHOULD_BE_DEF_PROC,   \
				  file, line, __FILE__, __LINE__)
#define subtilis_error_set_deffn_should_be_def_fn(e, file, line)               \
	subtilis_error_set_syntax(e, SUBTILIS_ERROR_DEFFN_SHOULD_BE_DEF_FN,    \
				  file, line, __FILE__, __LINE__)
#define subtilis_error_set_local_obscures_global(e, var_name, file, line)      \
	subtilis_error_set1(e, SUBTILIS_ERROR_LOCAL_OBSCURES_GLOBAL, var_name, \
			    file, line)
#define subtilis_error_set_temporary_not_allowed(e, context, file, line)       \
	subtilis_error_set1(e, SUBTILIS_ERROR_TEMPORARY_NOT_ALLOWED, context,  \
			    file, line)
#define subtilis_error_set_range_type_mismatch(e, scalar, col, file, line)     \
	subtilis_error_set2(e, SUBTILIS_ERROR_RANGE_TYPE_MISMATCH, scalar,     \
			    col, file, line)
#define subtilis_error_set_array_in_range(e, var_name, file, line)             \
	subtilis_error_set1(e, SUBTILIS_ERROR_ARRAY_IN_RANGE, var_name, file,  \
			    line)
#define subtilis_error_set_range_bad_var_count(e, exp, got, file, line)        \
	subtilis_error_set_int(e, SUBTILIS_ERROR_RANGE_BAD_VAR_COUNT, exp,     \
			       got, file, line, __FILE__, __LINE__)
#define subtilis_error_set_type_not_at_top_level(e, str, file, line)           \
	subtilis_error_set1(e, SUBTILIS_ERROR_TYPE_NOT_AT_TOP_LEVEL, str,      \
			    file, line)
#define subtilis_error_set_ass_bad_reg(e, reg, file, line)                     \
	subtilis_error_set1(e, SUBTILIS_ERROR_ASS_BAD_REG, reg, file, line)
#define subtilis_error_set_ass_integer_encode(e, num, file, line)              \
	subtilis_error_set_int(e, SUBTILIS_ERROR_ASS_INTEGER_ENCODE, num, 0,   \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_sys_call_unknown(e, str, file, line)                \
	subtilis_error_set1(e, SUBTILIS_ERROR_SYS_CALL_UNKNOWN, str, file, line)
#define subtilis_error_set_ass_bad_offset(e, offset, file, line)               \
	subtilis_error_set_int(e, SUBTILIS_ERROR_ASS_BAD_OFFSET, offset, 0,    \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_ass_bad_range(e, num1, num2, file, line)            \
	subtilis_error_set_int(e, SUBTILIS_ERROR_ASS_BAD_RANGE, num1, num2,    \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_ass_bad_alignment(e)                                \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_ASS_BAD_ALIGNMENT,          \
				 __FILE__, __LINE__)
#define subtilis_error_set_ass_keyword_bad_use(e, name, file, line)            \
	subtilis_error_set1(e, SUBTILIS_ERROR_ASS_KEYWORD_BAD_USE, name, file, \
			    line)
#define subtilis_error_set_ass_integer_too_big(e, num, file, line)             \
	subtilis_error_set_int(e, SUBTILIS_ERROR_ASS_INT_TOO_BIG, num, 0,      \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_ass_unknown_label(e, str, file, line)               \
	subtilis_error_set1(e, SUBTILIS_ERROR_ASS_MISSING_LABEL, str, file,    \
			    line)
#define subtilis_error_set_ass_bad_adr(e)                                      \
	subtilis_error_set_basic(e, SUBTILIS_ERROR_ASS_BAD_ADR, __FILE__,      \
				 __LINE__)
#define subtilis_error_set_ass_bad_align(e, align, file, line)                 \
	subtilis_error_set_int(e, SUBTILIS_ERROR_ASS_BAD_ALIGN, align, 0,      \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_ass_bad_real_imm(e, num, file, line)                \
	subtilis_error_set_dbl(e, SUBTILIS_ERROR_ASS_BAD_REAL_IMM, num, 0.0,   \
			       file, line, __FILE__, __LINE__)
#define subtilis_error_set_label_missing_colon(e, name, file, line)            \
	subtilis_error_set1(e, SUBTILIS_ERROR_ASS_LABEL_MISSING_COLON, name,   \
			    file, line)

void subtilis_error_set_full(subtilis_error_t *e, subtilis_error_type_t type,
			     const char *data1, const char *data2,
			     const char *file, unsigned int line,
			     const char *subtilis_file,
			     unsigned int subtilis_line);
void subtilis_error_set_int(subtilis_error_t *e, subtilis_error_type_t type,
			    int data1, int data2, const char *file,
			    unsigned int line, const char *subtilis_file,
			    unsigned int subtilis_line);
void subtilis_error_set_dbl(subtilis_error_t *e, subtilis_error_type_t type,
			    double data1, double data2, const char *file,
			    unsigned int line, const char *subtilis_file,
			    unsigned int subtilis_line);
void subtilis_error_set_basic(subtilis_error_t *e, subtilis_error_type_t type,
			      const char *subtilis_file,
			      unsigned int subtilis_line);
void subtilis_error_set_syntax(subtilis_error_t *e, subtilis_error_type_t type,
			       const char *file, unsigned int line,
			       const char *subtilis_file,
			       unsigned int subtilis_line);
void subtilis_error_set_hex(subtilis_error_t *e, subtilis_error_type_t type,
			    uint32_t num, const char *subtilis_file,
			    unsigned int subtilis_line);
void subtilis_error_set_errno(subtilis_error_t *e, subtilis_error_type_t type,
			      const char *data1, const char *subtilis_file,
			      unsigned int subtilis_line);
void subtilis_error_fprintf(FILE *f, subtilis_error_t *e, bool verbose);

void subtilis_error_set_bad_arg_type(subtilis_error_t *e, size_t arg,
				     const char *expected, const char *got,
				     const char *file, unsigned int line,
				     const char *subtilis_file,
				     unsigned int subtilis_line);

#endif
