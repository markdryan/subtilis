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
#include <stdio.h>
#include <string.h>

#include "error.h"

struct _subtilis_error_desc_t {
	const char *msg;
	unsigned int args;
};

typedef struct _subtilis_error_desc_t subtilis_error_desc_t;

/* clang-format off */
static subtilis_error_desc_t prv_errors[] = {
	/* SUBTILIS_ERROR_OK */
	{"", 0},

	/* SUBTILIS_ERROR_OOM */
	{"Memory allocation error.\n", 0},

	/* SUBTILIS_ERROR_OPEN */
	{"Failed to open %s, err: %s.\n", 2},

	/* SUBTILIS_ERROR_READ */
	{"Failed to read from file, err: %s.\n", 1},

	/* SUBTILIS_ERROR_WRITE */
	{"Failed to write to file, err: %s.\n", 1},

	/* SUBTILIS_ERROR_CLOSE */
	{"Failed to close file, err: %s.\n", 1},

	/* SUBTILIS_ERROR_UNTERMINATED_STRING */
	{"Unterminated string: %s.\n", 1},

	/* SUBTILIS_ERROR_STRING_TOO_LONG */
	{"String too long: %s.\n", 1},

	/* SUBTILIS_ERROR_NUMBER_TOO_LONG */
	{"Number too long: %s.\n", 1},

	/* SUBTILIS_ERROR_IDENTIFIER_TOO_LONG */
	{"Identifier too long: %s.\n", 1},

	/* SUBTILIS_ERROR_UNKNOWN_TOKEN */
	{"Unknown token: %s.\n", 1},

	/* SUBTILIS_ERROR_BAD_PROC_NAME */
	{"Bad procedure name: %s.\n", 1},

	/* SUBTILIS_ERROR_BAD_FN_NAME */
	{"Bad function name: %s.\n", 1},

	/* SUBTILIS_ERROR_ASSERTION_FAILED */
	{"Assertion failed.\n", 0},

	/* SUBTILIS_ERROR_KEYWORD_EXPECTED */
	{"Keyword expected, found %s.\n", 1},

	/* SUBTILIS_ID_EXPECTED */
	{"Identifer expected found %s.\n", 1},

	/* SUBTILIS_ERROR_NOT_SUPPORTED */
	{"%s not supported.\n", 1},

	/* SUBTILIS_ERROR_ASSIGNMENT_OP_EXPECTED */
	{"Assignment operator (=, +=, -=)  expected found %s.\n", 1},

	/* SUBTILIS_ERROR_EXP_EXPECTED */
	{"Expression  expected found %s.\n", 1},

	/* SUBTILIS_ERROR_RIGHT_BKT_EXPECTED */
	{")  expected found %s.\n", 1},

	/* SUBTILIS_ERROR_INTEGER_EXPECTED */
	{"Integer expected found %s.\n", 1},

	/* SUBTILIS_ERROR_BAD_EXPRESSION */
	{"Bad expression.\n", 0},

	/* SUBTILIS_ERROR_DIVIDE_BY_ZERO */
	{"Divide by zero.\n", 0},

	/* SUBTILIS_ERROR_UNKNOWN_VARIABLE */
	{"Unknown variable %s.\n", 1},

	/* SUBTILIS_ERROR_INTEGER_EXP_EXPECTED */
	{"Integer expression expected .\n", 0},

	/* SUBTILIS_ERROR_EXPECTED */
	{"%s expected, found %s .\n", 2},

	/* SUBTILIS_COMPOUND_NOT_TERM */
	{"Compound statement not terminated.\n", 0},

	/* SUBTILIS_ERROR_WALKER_FAILED */
	{"Walker failed.\n", 0},

	/* SUBTILIS_ERROR_ALREADY_DEFINED */
	{"%s has already been defined.\n", 1},

	/* SUBTILIS_ERROR_NESTED_PROCEDURE */
	{"Nested procedures or functions are not allowed.\n", 0},

	/* SUBTILIS_ERROR_PROCEDURE_EXPECTED */
	{"%s is a procedure not a function\n", 1},

	/* SUBTILIS_ERROR_FUNCTION_EXPECTED */
	{"%s is a function not a procedure\n", 1},

	/* SUBTILIS_ERROR_UNKNOWN_PROCEDURE */
	{"Unknown procedure %s\n", 1},

	/* SUBTILIS_ERROR_UNKNOWN_FUNCTION */
	{"Unknown function %s\n", 1},

	/* SUBTILIS_ERROR_BAD_INSTRUCTION */
	{"Bad instruction %s\n", 1},

	/* SUBTILIS_ERROR_BAD_ARG_COUNT */
	{"Wrong number of arguments.  Found %s expected %s\n", 2},

	/* SUBTILIS_ERROR_BAD_ARG_TYPE */
	{"Argument %s of wrong type : %s\n", 2},

	/* SUBTILIS_ERROR_ZERO_STEP */
	{"step must not be zero\n"},

	/* SUBTILIS_ERROR_NUMERIC_EXPECTED */
	{"%s must be a numeric variable\n", 1},

	/* SUBTILIS_ERROR_ENDPROC_IN_MAIN */
	{"ENDPROC is not allowed in the main function\n"},

	/* SUBTILIS_ERROR_USELESS_STATEMENT */
	{"Statement will never be executed\n"},

	/* SUBTILIS_ERROR_RETURN_IN_MAIN */
	{"<- is not allowed in the main function\n"},

	/* SUBTILIS_ERROR_RETURN_IN_PROC */
	{"<- is not allowed in a procedure\n"},

	/* SUBTILIS_ERROR_NESTED_HANDLER */
	{"Nested error handlers are not allowed\n", 0},

	/* SUBTILIS_ERROR_ENDPROC_IN_FN */
	{"ENDPROC is not allowed in functions\n"},

	/* SUBTILIS_ERROR_RETURN_EXPECTED */
	{"The top most error handler in a function must return a value\n"},

	/* SUBTILIS_ERROR_TOO_MANY_DIMS */
	{"Too many dimensions specified for array %s\n", 1},

	/* SUBTILIS_ERROR_DIM_IN_PROC */
	{"Cannot create global array in a procedure.  Use LOCAL DIM\n", 0},

	/* SUBTILIS_ERROR_BAD_DIM */
	{"Invalid dimensions specified for array %s\n", 1},

	/* SUBTILIS_ERROR_BAD_INDEX */
	{"Invalid index specified for array %s\n", 1},

	/* SUBTILIS_ERROR_BAD_INDEX_COUNT */
	{"Incorrect number of indices specified for array %s\n", 1},

	/* SUBTILIS_ERROR_NOT_ARRAY */
	{"%s is not an array\n", 1},

	/* SUBTILIS_ERROR_VARIABLE_BAD_LEVEL */
	{"global variable %s must be declared at the top level\n", 1},

	/* SUBTILIS_ERROR_BAD_ERROR */
	{"Bad error, expected %s got %s\n", 2},

	/* SUBTILIS_ERROR_TOO_MANY_BLOCKS */
	{"Too many blocks\n", 0},

	/* SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH */
	{"Arrays are not compatible\n", 0},

	/* SUBTILIS_ERROR_CONST_INTEGER_EXPECTED */
	{"Constant integer expected\n", 0},

	/* SUBTILIS_ERROR_NUMERIC_EXP_EXPECTED */
	{"Numeric expression expected\n", 0},

	/* SUBTILIS_ERROR_BAD_CONVERSION */
	{"Unable to convert from %s to %s\n", 2},

	/* SUBTILIS_ERROR_BAD_ELEMENT_COUNT */
	{"Bad element count\n", 0},

	/* SUBTILIS_ERROR_CONST_EXPRESSION_EXPECTED */
	{"Constant expression expected\n", 0},

	/* SUBTILIS_ERROR_STRING_EXPECTED */
	{"String expected\n", 0},

	/* SUBTILIS_ERROR_CONST_STRING_EXPECTED */
	{"Constant string expected\n", 0},

	/* SUBTILIS_ERROR_STRING_VARIABLE_EXPECTED */
	{"String variable expected\n", 0},

	/* SUBTILIS_ERROR_BAD_VAL_ARG */
	{"The second value to VAL must be either 16 or >= 2 and <= 10\n", 0},
};

/* clang-format on */

void subtilis_error_init(subtilis_error_t *e)
{
	e->type = SUBTILIS_ERROR_OK;
	e->data1[0] = 0;
	e->data2[0] = 0;
	e->file[0] = 0;
	e->line = 0;
	e->subtilis_file[0] = 0;
	e->subtilis_line = 0;
}

void subtilis_error_set_full(subtilis_error_t *e, subtilis_error_type_t type,
			     const char *data1, const char *data2,
			     const char *file, unsigned int line,
			     const char *subtilis_file,
			     unsigned int subtilis_line)
{
	e->type = type;
	strncpy(e->data1, data1, SUBTILIS_CONFIG_ERROR_LEN);
	e->data1[SUBTILIS_CONFIG_ERROR_LEN - 1] = 0;
	strncpy(e->data2, data2, SUBTILIS_CONFIG_ERROR_LEN);
	e->data2[SUBTILIS_CONFIG_ERROR_LEN - 1] = 0;
	strncpy(e->file, file, SUBTILIS_CONFIG_PATH_MAX);
	e->file[SUBTILIS_CONFIG_PATH_MAX - 1] = 0;
	e->line = line;
	strncpy(e->subtilis_file, subtilis_file, SUBTILIS_CONFIG_PATH_MAX);
	e->subtilis_file[SUBTILIS_CONFIG_PATH_MAX - 1] = 0;
	e->subtilis_line = subtilis_line;
}

void subtilis_error_set_int(subtilis_error_t *e, subtilis_error_type_t type,
			    int data1, int data2, const char *file,
			    unsigned int line, const char *subtilis_file,
			    unsigned int subtilis_line)
{
	char num1[32];
	char num2[32];

	snprintf(num1, sizeof(num1), "%d", data1);
	snprintf(num2, sizeof(num2), "%d", data2);

	subtilis_error_set_full(e, type, num1, num2, file, line, subtilis_file,
				subtilis_line);
}

void subtilis_error_set_basic(subtilis_error_t *e, subtilis_error_type_t type,
			      const char *subtilis_file,
			      unsigned int subtilis_line)
{
	subtilis_error_set_full(e, type, "", "", "", 0, subtilis_file,
				subtilis_line);
}

void subtilis_error_set_syntax(subtilis_error_t *e, subtilis_error_type_t type,
			       const char *file, unsigned int line,
			       const char *subtilis_file,
			       unsigned int subtilis_line)
{
	subtilis_error_set_full(e, type, "", "", file, line, subtilis_file,
				subtilis_line);
}

void subtilis_error_set_errno(subtilis_error_t *e, subtilis_error_type_t type,
			      const char *data1, const char *subtilis_file,
			      unsigned int subtilis_line)
{
	char buf[32];

	sprintf(buf, "%d", errno);
	subtilis_error_set_full(e, type, data1, buf, "", 0, subtilis_file,
				subtilis_line);
}

void subtilis_error_fprintf(FILE *f, subtilis_error_t *e, bool verbose)
{
	int args;

	if (e->type == SUBTILIS_ERROR_OK)
		return;
	if (strlen(e->file) > 0)
		fprintf(f, "%s: %d: ", e->file, e->line);

	args = prv_errors[e->type].args;
	if (args == 0)
		fprintf(f, "%s", prv_errors[e->type].msg);
	else if (args == 1)
		fprintf(f, prv_errors[e->type].msg, e->data1);
	else if (args == 2)
		fprintf(f, prv_errors[e->type].msg, e->data1, e->data2);
	if (verbose)
		fprintf(f, "  generated by %s:%d\n", e->subtilis_file,
			e->subtilis_line);
}

void subtilis_error_set_hex(subtilis_error_t *e, subtilis_error_type_t type,
			    uint32_t num, const char *subtilis_file,
			    unsigned int subtilis_line)
{
	char buf[32];

	sprintf(buf, "0x%x", num);
	subtilis_error_set1(e, type, buf, subtilis_file, subtilis_line);
}

void subtilis_error_set_bad_arg_type(subtilis_error_t *e, size_t arg,
				     const char *expected, const char *got,
				     const char *file, unsigned int line,
				     const char *subtilis_file,
				     unsigned int subtilis_line)
{
	char arg_num[32];
	char msg[SUBTILIS_CONFIG_ERROR_LEN];

	snprintf(arg_num, sizeof(arg_num), "%zu", arg);
	snprintf(msg, sizeof(msg), "expected %s got %s", expected, got);

	subtilis_error_set_full(e, SUBTILIS_ERROR_BAD_ARG_TYPE, arg_num, msg,
				file, line, subtilis_file, subtilis_line);
}
