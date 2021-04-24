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
	  "DIM a%(10, -1)\n",
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
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH
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
	{
	"for_not_numeric",
	"local dim a%(10)\n"
	"for i% := 0 to a%()\n"
	"next\n",
	SUBTILIS_ERROR_NUMERIC_EXP_EXPECTED,
	},
	{
	"array_arg_dim_mismatch",
	"dim b%(1,1)\n"
	"PROCArr(b%())\n"
	"def PROCArr(a%(1))\n"
	"endproc\n",
	SUBTILIS_ERROR_BAD_ARG_TYPE
	},
	{
	"array_1d_too_many_initialisers",
	"dim b%(5)\n"
	"b%() = 0, 1, 2, 3, 4, 5, 6\n",
	SUBTILIS_ERROR_BAD_ELEMENT_COUNT
	},
	{
	"array_2d_too_many_initialisers",
	"dim b%(2,2)\n"
	"b%() = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9\n",
	SUBTILIS_ERROR_BAD_ELEMENT_COUNT
	},
	{
	"array_var_initialiser",
	"dim b%(5)\n"
	"a% := 2\n"
	"b%() = 0, 1, a%, 3, 4, 5, 6\n",
	SUBTILIS_ERROR_CONST_EXPRESSION_EXPECTED,
	},
	{
	"array_get_dim_not_array",
	"dim a%(10)\n"
	"print dim(a%)\n",
	SUBTILIS_ERROR_NOT_ARRAY,
	},
	{
	"chr$",
	"print chr$(\"Hello\")\n",
	SUBTILIS_ERROR_INTEGER_EXPECTED,
	},
	{
	"asc",
	"print asc(1)\n",
	SUBTILIS_ERROR_STRING_EXPECTED,
	},
	{
	"len",
	"print asc(1.0)\n",
	SUBTILIS_ERROR_STRING_EXPECTED,
	},
	{
	"string_array_mismatch",
	"dim a$(10)\n"
	"a$() = 1, 2, 3\n",
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH,
	},
	{
	"int_array_mismatch",
	"dim a%(10)\n"
	"a%() = \"hello\", \"world\"n",
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH,
	},
	{
	"str_str_mismatch",
	"a$ = str$~(\"hello\")\n",
	SUBTILIS_ERROR_INTEGER_EXPECTED,
	},
	{
	"str_add_mismatch",
	"a$ := \"hello\"\n"
	"print a$ + 10\n",
	SUBTILIS_ERROR_STRING_EXPECTED,
	},
	{
	"left_str_zero_arg",
	"a$ := \"hello\"\n"
	"left$() = \"aa\"\n",
	SUBTILIS_ERROR_STRING_EXPECTED,
	},
	{
	"right_str_zero_arg",
	"a$ := \"hello\"\n"
	"right$() = \"aa\"\n",
	SUBTILIS_ERROR_STRING_EXPECTED,
	},
	{
	"mid_str_zero_arg",
	"a$ := \"hello\"\n"
	"mid$() = \"aa\"\n",
	SUBTILIS_ERROR_STRING_EXPECTED,
	},
	{
	"mid_str_one_arg",
	"a$ := \"hello\"\n"
	"mid$(a$) = \"aa\"\n",
	SUBTILIS_ERROR_NUMERIC_EXP_EXPECTED,
	},
	{
	"left_str_const_str",
	"left$(\"hello\") = \"aa\"\n",
	SUBTILIS_ERROR_STRING_VARIABLE_EXPECTED,
	},
	{
	"right_str_const_str",
	"right$(\"hello\") = \"aa\"\n",
	SUBTILIS_ERROR_STRING_VARIABLE_EXPECTED,
	},
	{
	"mid_str_const_str",
	"mid$(\"hello\", 1) = \"aa\"\n",
	SUBTILIS_ERROR_STRING_VARIABLE_EXPECTED,
	},
	{
	"print_hex_not_num",
	"a$ := \"hello\"\n"
	"print ~a$\n",
	SUBTILIS_ERROR_INTEGER_EXPECTED,
	},
	{"val_too_long",
	 "a=val(\"-30000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "0000000000000000000000000000000000000000000000\")\n",
	 SUBTILIS_ERROR_NUMBER_TOO_LONG,
	},
	{"val_bad_base",
	"print val(\"2\", 11)\n",
	SUBTILIS_ERROR_BAD_VAL_ARG,
	},
	{"defproc1",
	 "PROCMark\n"
	 "DEFPROCMark\n"
	 "ENDPROC\n",
	 SUBTILIS_ERROR_DEFPROC_SHOULD_BE_DEF_PROC,
	},
	{"defproc2",
	 "PROCMark(10)\n"
	 "a% := 1\n"
	 "DEFPROCMark(a%)\n"
	 "ENDPROC\n",
	 SUBTILIS_ERROR_DEFPROC_SHOULD_BE_DEF_PROC,
	},
	{"defproc3",
	 "PROCMark(10)\n"
	 "DEFPROCMark(a%)\n"
	 "ENDPROC\n",
	 SUBTILIS_ERROR_DEFPROC_SHOULD_BE_DEF_PROC,
	},
	{"deffn1",
	 "PRINT FNMark\n"
	 "DEFFNMark\n"
	 "<-10\n",
	 SUBTILIS_ERROR_DEFFN_SHOULD_BE_DEF_FN,
	},
	{"deffn2",
	 "PRINT FNMark(10)\n"
	 "a% := 0\n"
	 "DEFFNMark(a%)\n"
	 "<-a%\n",
	 SUBTILIS_ERROR_DEFFN_SHOULD_BE_DEF_FN,
	},
	{"deffn3",
	 "PRINT FNMark(10)\n"
	 "DEFFNMark(a%)\n"
	 "<-a%\n",
	 SUBTILIS_ERROR_DEFFN_SHOULD_BE_DEF_FN,
	},
	{"error_in_handler",
	 "onerror local dim a%(10) enderror\n",
	 SUBTILIS_ERROR_ERROR_IN_HANDLER,
	},
	{"handler_in try",
	 "try onerror enderror endtry\n",
	 SUBTILIS_ERROR_HANDLER_IN_TRY,
	},
	{"intz_dbl_to_int",
	"print intz(10.0)\n",
	SUBTILIS_ERROR_BAD_ZERO_EXTEND,
	},
	{"array_reference_redefine",
	"a&() := FNInita&(1)()\n"
	"a&() := FNUpdatea&(1)(a&())\n"
	"def FNInita&(1)\n"
	"local dim a&(1)\n"
	"<-a&()\n",
	SUBTILIS_ERROR_ALREADY_DEFINED,
	},
	{"assign_array_to_numeric",
	"a& := FNInita&(1)()\n"
	"def FNInita&(1)\n"
	"local dim a&(1)\n"
	"<-a&()\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"local_obscures_global1",
	"dim a&(1)\n"
	"a& := 10\n"
	"print a&\n"
	"print a&(0)\n",
	SUBTILIS_ERROR_LOCAL_OBSCURES_GLOBAL,
	},
	{"local_obscures_global2",
	"dim a&(1)\n"
	"local a& = 10\n"
	"print a&\n"
	"print a&(0)\n",
	SUBTILIS_ERROR_LOCAL_OBSCURES_GLOBAL,
	},
	{
	"get_hash_array_strings",
	"dim a$(10)\n"
	"x% := get# 0, a$()\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{
	"put_hash_array_int",
	"a% = 10\n"
	"x% := get# 0, a%\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"copy_bad_src",
	"a% = &12345678\n"
	"dim b&(9)\n"
	"copy b&(), a%",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"copy_bad_dst",
	"dim b&(9)\n"
	"copy \"mark\", b&()",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"copy_string_array_mismatch",
	"dim b&(9)\n"
	"dim a$(9)\n"
	"copy(a$(), b&())",
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH,
	},
	{"copy_string_array_mismatch_2",
	"dim b&(9)\n"
	"dim a$(9)\n"
	"copy(b&(), a$())",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"local_array_in_main",
	"dim a%(-1)\n"
	"b%() := FNEmpty%(1)()\n"
	"PROCPrint\n"
	"\n"
	"def FNEmpty%(1)\n"
	"  local dim a%()\n"
	"<-a%()\n"
	"\n"
	"def PROCPrint\n"
	"  print dim(b%(),1)\n"
	"endproc\n",
	SUBTILIS_ERROR_UNKNOWN_VARIABLE,
	},
	{"append_to_fixed_array",
	"dim a%(10)\n"
	"append a%(), 10\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"append_string_to_int_array",
	"dim a%()\n"
	"append a%(), \"hello\"\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"copy_temp",
	"a$ = \"aa\"\n"
	"copy(a$+\"1\", \"a\")\n",
	SUBTILIS_ERROR_TEMPORARY_NOT_ALLOWED,
	},
	{"get_hash_temp",
	"a$ = \"aa\"\n"
	"x$ := get# 0, a$+\"1\"\n",
	SUBTILIS_ERROR_TEMPORARY_NOT_ALLOWED,
	},
};

/* clang-format on */
