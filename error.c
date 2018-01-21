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

	/* SUBTILIS_ERROR_NOT_SUPPORTED */
	{"Keyword %s not supported.\n", 1},

	/* SUBTILIS_ID_EXPECTED */
	{"Identifer expected found %s.\n", 1},

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
	{"Compund statement not terminated.\n", 0},

	/* SUBTILIS_ERROR_WALKER_FAILED */
	{"Walker failed.\n", 0},

	/* SUBTILIS_ERROR_ALREADY_DEFINED */
	{"Already defined.\n", 0},

	/* SUBTILIS_ERROR_NESTED_PROCEDURE */
	{"Nested procedures or functions are not allowed.\n", 0},
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

void subtilis_error_set_basic(subtilis_error_t *e, subtilis_error_type_t type,
			      const char *subtilis_file,
			      unsigned int subtilis_line)
{
	subtilis_error_set_full(e, type, "", "", "", 0, subtilis_file,
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
