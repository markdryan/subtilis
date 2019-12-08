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

#include "bad_test_cases.h"

/* clang-format off */
const subtilis_bad_test_case_t bad_test_cases[] = {
	{ "already_defined_local",
	  "LOCAL A%\n"
	  "LOCAL A%\n",
	  SUBTILIS_ERROR_ALREADY_DEFINED,
	},
	{ "already_defined_nested",
	  "LOCAL A%\n"
	  "IF TRUE THEN\n"
	  "    LOCAL A%\n"
	  "ENDIF\n",
	  SUBTILIS_ERROR_ALREADY_DEFINED,
	},
	{ "already_defined_ass_op",
	  "A% := 1\n"
	  "IF TRUE THEN\n"
	  "    A% := 2\n"
	  "ENDIF\n",
	  SUBTILIS_ERROR_ALREADY_DEFINED,
	},
	{ "already_defined_for",
	  "A% := 1\n"
	  "FOR A% := 0 TO 10\n"
	  "NEXT\n",
	  SUBTILIS_ERROR_ALREADY_DEFINED,
	},
	{ "already_defined_dim_global",
	  "DIM A%(1)\n"
	  "DIM A%(1)\n",
	  SUBTILIS_ERROR_ALREADY_DEFINED,
	},
	{ "already_defined_dim_local",
	  "LOCAL DIM A%(1)\n"
	  "IF TRUE THEN\n"
	  "    LOCAL DIM A%(1)\n"
	  "ENDIF\n",
	  SUBTILIS_ERROR_ALREADY_DEFINED,
	},
	{ "unknown_procedure",
	  "PROCmissing\n"
	  "DEF PROCmiss\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_UNKNOWN_PROCEDURE,
	},
	{ "unknown_function",
	  "PRINT FNBad\n"
	  "DEF FNBad\n",
	  SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	},
	{ "not_keyword",
	  "+\n",
	  SUBTILIS_ERROR_KEYWORD_EXPECTED,
	},
	{ "not_terminated_if",
	  "IF TRUE THEN\n",
	  SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	},
	{ "not_terminated_while",
	  "WHILE TRUE\n",
	  SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	},
	{ "not_terminated_repeat",
	  "REPEAT\n",
	  SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	},
	{ "not_terminated_proc",
	  "PROCMark\n"
	  "DEF PROCMark\n",
	  SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	},
	{ "not_terminated_fn",
	  "PRINT FNMark\n"
	  "DEF FNMark\n",
	  SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	},
	{ "not_terminated_on_error",
	  "ONERROR\n",
	  SUBTILIS_ERROR_COMPOUND_NOT_TERM,
	},
	{ "proc_too_many_args1",
	  "PROCNoArgs(1)\n"
	  "DEF PROCNoArgs\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_BAD_ARG_COUNT,
	},
	{ "proc_too_many_args2",
	  "PROCOneArgs(1,2)\n"
	  "DEF PROCOneArgs(a%)\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_BAD_ARG_COUNT,
	},
	{ "fn_too_many_args1",
	  "PRINT FNNoArgs(1)\n"
	  "DEF FNNoArgs\n"
	  "<-0\n",
	  SUBTILIS_ERROR_BAD_ARG_COUNT,
	},
	{ "fn_too_many_args2",
	  "PRINT FNOneArgs(1,2)\n"
	  "DEF FNOneArgs(a%)\n"
	  "<-0\n",
	  SUBTILIS_ERROR_BAD_ARG_COUNT,
	},
	{ "if_no_then",
	  "IF TRUE\n",
	  SUBTILIS_ERROR_EXPECTED,
	},
	{ "endproc_in_main",
	  "ENDPROC\n",
	  SUBTILIS_ERROR_ENDPROC_IN_MAIN,
	},
	{ "return_in_main",
	  "<-0\n",
	  SUBTILIS_ERROR_RETURN_IN_MAIN,
	},
	{ "missing_bracket",
	  "x := (10 + 2\n",
	  SUBTILIS_ERROR_RIGHT_BKT_EXPECTED,
	},
	{ "useless_statement",
	  "END\n"
	  "PRINT 1\n",
	  SUBTILIS_ERROR_USELESS_STATEMENT,
	},
	{ "useless_statement",
	  "PROCMark\n"
	  "DEF PROCMark\n"
	  "  IF TRUE THEN\n"
	  "      ENDPROC\n"
	  "      PRINT 1\n"
	  "  ENDIF\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_USELESS_STATEMENT,
	},
	{ "nested_procedure",
	  "PROCMark\n"
	  "DEF PROCMark\n"
	  "  DEF PROCMark2\n"
	  "  ENDPROC\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_NESTED_PROCEDURE,
	},
	{ "nested_function",
	  "PROCMark\n"
	  "DEF PROCMark\n"
	  "  DEF FNMark2\n"
	  "  <-0\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_NESTED_PROCEDURE,
	},
	{ "nested_handler",
	  "ONERROR\n"
	  "  ONERROR\n"
	  "  ENDERROR\n"
	  "ENDERROR\n",
	  SUBTILIS_ERROR_NESTED_HANDLER,
	},
	{ "dim_in_proc",
	  "PROCMark\n"
	  "DEF PROCMark\n"
	  "  DIM a%(10)\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_DIM_IN_PROC,
	},
	{ "bad_dim",
	  "DIM a%(0)\n",
	  SUBTILIS_ERROR_BAD_DIM,
	},
	{ "not_array",
	  "a%:=0\n"
	  "PRINT a%(0)\n",
	  SUBTILIS_ERROR_NOT_ARRAY,
	},
	{ "bad_index",
	  "DIM a%(10)\n"
	  "PRINT a%(11)\n",
	  SUBTILIS_ERROR_BAD_INDEX,
	},
	{ "bad_index_count",
	  "DIM a%(10)\n"
	  "PRINT a%(11,12)\n",
	  SUBTILIS_ERROR_BAD_INDEX_COUNT,
	},
	{ "zero_step",
	  "FOR I%=0 TO 10 STEP 0\n"
	  "NEXT\n",
	  SUBTILIS_ERROR_ZERO_STEP,
	},
	{ "exp_expected",
	  "PRINT PRINT\n",
	  SUBTILIS_ERROR_EXP_EXPECTED,
	},
	{ "id_expected",
	  "LET = 10\n",
	  SUBTILIS_ERROR_ID_EXPECTED,
	},
	{ "bad_proc_name",
	  "DEF PROCMark%\n"
	  "ENDPROC\n",
	  SUBTILIS_ERROR_BAD_PROC_NAME
	},
	{ "too_many_blocks",
	  "IF TRUE THEN\n"
	  " IF TRUE THEN\n"
	  "  IF TRUE THEN\n"
	  "   IF TRUE THEN\n"
	  "    IF TRUE THEN\n"
	  "     IF TRUE THEN\n"
	  "      IF TRUE THEN\n"
	  "       IF TRUE THEN\n"
	  "        IF TRUE THEN\n"
	  "         IF TRUE THEN\n"
	  "          IF TRUE THEN\n"
	  "           IF TRUE THEN\n"
	  "            IF TRUE THEN\n"
	  "             IF TRUE THEN\n"
	  "              IF TRUE THEN\n"
	  "               IF TRUE THEN\n"
	  "                IF TRUE THEN\n"
	  "                 IF TRUE THEN\n"
	  "                  IF TRUE THEN\n"
	  "                   IF TRUE THEN\n"
	  "                    IF TRUE THEN\n"
	  "                     IF TRUE THEN\n"
	  "                      IF TRUE THEN\n"
	  "                       IF TRUE THEN\n"
	  "                        IF TRUE THEN\n"
	  "                         IF TRUE THEN\n"
	  "                          IF TRUE THEN\n"
	  "                           IF TRUE THEN\n"
	  "                            IF TRUE THEN\n"
	  "                             IF TRUE THEN\n"
	  "                              IF TRUE THEN\n"
	  "                               IF TRUE THEN\n"
	  "                                IF TRUE THEN\n"
	  "                                 x% := 0\n"
	  "                                ENDIF\n"
	  "                               ENDIF\n"
	  "                              ENDIF\n"
	  "                             ENDIF\n"
	  "                            ENDIF\n"
	  "                           ENDIF\n"
	  "                          ENDIF\n"
	  "                         ENDIF\n"
	  "                        ENDIF\n"
	  "                       ENDIF\n"
	  "                      ENDIF\n"
	  "                     ENDIF\n"
	  "                    ENDIF\n"
	  "                   ENDIF\n"
	  "                  ENDIF\n"
	  "                 ENDIF\n"
	  "                ENDIF\n"
	  "               ENDIF\n"
	  "              ENDIF\n"
	  "             ENDIF\n"
	  "            ENDIF\n"
	  "           ENDIF\n"
	  "          ENDIF\n"
	  "         ENDIF\n"
	  "        ENDIF\n"
	  "       ENDIF\n"
	  "      ENDIF\n"
	  "     ENDIF\n"
	  "    ENDIF\n"
	  "   ENDIF\n"
	  "  ENDIF\n"
	  " ENDIF\n"
	  "ENDIF\n",
	  SUBTILIS_ERROR_TOO_MANY_BLOCKS,
	},
	{
	"div_by_zero",
	"PRINT 10 DIV 0\n",
	SUBTILIS_ERROR_DIVIDE_BY_ZERO
	},
	{
	"divide_by_zero",
	"PRINT 10 / 0\n",
	SUBTILIS_ERROR_DIVIDE_BY_ZERO
	},
	{
	"missing_square_bracket",
	"VDU [30\n",
	SUBTILIS_ERROR_EXPECTED
	},
	{
	"array_assign_mismatch",
	"DIM a%(1)\n"
	"DIM b%(1,1)\n"
	"a%() = b%()\n",
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH
	},
	{
	"array_assign_mismatch2",
	"DIM a%(1)\n"
	"DIM b(1)\n"
	"a%() = b()\n",
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH
	},
	{
	"array_plus_assign",
	"DIM a%(1)\n"
	"DIM b%(1)\n"
	"a%() += b()\n",
	SUBTILIS_ERROR_ASSIGNMENT_OP_EXPECTED
	},
	{
	"array_bad_arg_int",
	"PROCArr(1)\n"
	"def PROCArr(a%(1))\n"
	"endproc\n",
	SUBTILIS_ERROR_BAD_ARG_TYPE
	},
	{
	"array_bad_arg_int",
	"PROCArr(1.0)\n"
	"def PROCArr(a%(1))\n"
	"endproc\n",
	SUBTILIS_ERROR_BAD_ARG_TYPE
	},
	{
	"array_bad_fn_bad_assign",
	"a%() = FNArr%\n"
	"def FNArr%\n"
	"<-0\n",
	SUBTILIS_ERROR_UNKNOWN_VARIABLE
	},
	{
	"array_dim_too_many_args",
	"DIM a%(10)\n"
	"print dim(a%(),1,2)\n",
	SUBTILIS_ERROR_RIGHT_BKT_EXPECTED,
	},
	{
	"array_dim_no_args",
	"DIM a%(10)\n"
	"print dim()\n",
	SUBTILIS_ERROR_EXP_EXPECTED,
	},
	{
	"array_dim_zero_arg",
	"DIM a%(10)\n"
	"print dim(a%(),0)\n",
	SUBTILIS_ERROR_BAD_INDEX,
	},
	{
	"array_dim_arg_too_big",
	"DIM a%(10)\n"
	"print dim(a%(),11)\n",
	SUBTILIS_ERROR_BAD_INDEX,
	},
};

/* clang-format on */