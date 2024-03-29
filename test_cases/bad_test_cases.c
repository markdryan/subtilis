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
	  SUBTILIS_ERROR_EXPECTED,
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
	SUBTILIS_ERROR_EXPECTED
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
	"a%() := FNArr%\n"
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
	"array_bad_string_initialiser1",
	"dim b$(1)\n"
	"b$() = \"hello\", 5\n",
	SUBTILIS_ERROR_CONST_STRING_EXPECTED,
	},
	{
	"array_bad_string_initialiser1",
	"dim b$(1)\n"
	"local a$\n"
	"b$() = a$, \"hello\"\n",
	SUBTILIS_ERROR_CONST_STRING_EXPECTED,
	},
	{
	"array_fn_string_initialiser1",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = 5\n",
	SUBTILIS_ERROR_FN_TYPE_MISMATCH,
	},
	{
	"array_fn_string_initialiser2",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = def FN%(a%) <-0, 5\n",
	SUBTILIS_ERROR_FN_TYPE_MISMATCH,
	},
	{
	"array_fn_string_initialiser3",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = 5, def FN%(a%) <-0\n",
	SUBTILIS_ERROR_FN_TYPE_MISMATCH,
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
	"x% := get#(0, a$())\n",
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
	"dim a%(1)\n"
	"b%() := FNEmpty%(1)()\n"
	"PROCPrint\n"
	"\n"
	"def FNEmpty%(1)\n"
	"  local dim a%(1)\n"
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
	"dim a%{}\n"
	"append a%(), \"hello\"\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"get_hash_temp",
	"a$ = \"aa\"\n"
	"x$ := get#(0, a$+\"1\")\n",
	SUBTILIS_ERROR_TEMPORARY_NOT_ALLOWED,
	},
	{"index_vector_as_array",
	"dim a%{10}\n"
	"print a%(1)\n",
	SUBTILIS_ERROR_NOT_ARRAY,
	},
	{"index_array_as_vector",
	"dim a%(10)\n"
	"print a%{1}\n",
	SUBTILIS_ERROR_NOT_VECTOR,
	},
	{"assign_vector_to_array",
	"dim a%{10}\n"
	"b%() := a%{}\n",
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH,
	},
	{"assign_array_to_vector",
	"dim a%(10)\n"
	"b%{} := a%()\n",
	SUBTILIS_ERROR_ARRAY_TYPE_MISMATCH,
	},
	{"negative_array_dim",
	"dim a%(-1)\n",
	SUBTILIS_ERROR_BAD_DIM,
	},
	{"range_out_of_scope",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"range b% := a%()\n"
	"    print b%\n"
	"endrange\n"
	"print b%\n",
	SUBTILIS_ERROR_UNKNOWN_VARIABLE,
	},
	{"range_not_array_or_vector",
	"range a& := \"hello\"\n"
	"  print a&\n"
	"endrange\n",
	SUBTILIS_ERROR_NOT_ARRAY_OR_VECTOR,
	},
	{"range_type_mismatch",
	"dim b(7)\n"
	"range a& := b()\n"
	"print a&\n"
	"endrange\n",
	SUBTILIS_ERROR_RANGE_TYPE_MISMATCH,
	},
	{"range_array_in_range",
	"dim a&(5)\n"
	"a&() = 1,2,3,4,5,6\n"
	"dim c&(0)\n"
	"range c&(0) := a&()\n"
	"  print b&\n"
	"endrange\n",
	SUBTILIS_ERROR_ARRAY_IN_RANGE,
	},
	{"range_too_many_vars",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"local c%\n"
	"range b%, c%, d% = a%()\n"
	"endrange\n",
	SUBTILIS_ERROR_RANGE_BAD_VAR_COUNT,
	},
	{"range_bad_index_type",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"local b%\n"
	"range b%, c = a%()\n"
	"  print c\n"
	"endrange\n",
	SUBTILIS_ERROR_INTEGER_VARIABLE_EXPECTED,
	},
	{"range_array_for_index_var",
	"dim a%{5}\n"
	"dim c%(0)\n"
	"range b%, c%(0) = a%{}\n"
	"endrange\n",
	SUBTILIS_ERROR_ARRAY_IN_RANGE,
	},
	{"for_out_of_scope",
	"for i% := 0 to 10\n"
	"next\n"
	"print i%\n",
	SUBTILIS_ERROR_UNKNOWN_VARIABLE,
	},
	{"assign_func_to_proc",
	"type PROCMap(a%)\n"
	"a@PROCMap = def FN(a%) <-a%\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"assign_proc_missing_args",
	"type PROCMap(a%, b%)\n"
	"a@PROCMap = def PROC(a%) endproc\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"assign_fn_type_mismatch",
	"type FNMap%\n"
	"a@FNMap = def FN <-0.0\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"bad_lambda_name1",
	"type FNMap\n"
	"a@FNMap = def FNu <-0.0\n",
	SUBTILIS_ERROR_BAD_LAMBDA_NAME
	},
	{"bad_lambda_name2",
	"type FNMap%\n"
	"a@FNMap = def FNu% <-0\n",
	SUBTILIS_ERROR_BAD_LAMBDA_NAME
	},
	{"assign_fn_type_mismatch2",
	"type PROCEmpty\n"
	"local a@PROCEmpty = 1\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"assign_fn_type_mismatch3",
	"type PROCEmpty\n"
	"a@PROCEmpty = 1\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"proc_addr_undefined",
	"type PROCEmpty\n"
	"a@PROCEmpty = !PROCBoring\n"
	"a@PROCEmpty()\n",
	SUBTILIS_ERROR_UNKNOWN_PROCEDURE,
	},
	{"proc_addr_wrong_args",
	"type FNStr$(a$, b%)\n"
	"local a@FNStr = !FNgetStr$\n"
	"print a@FNStr(\"hello \", 3)\n"
	"def FNgetStr$(b%)\n"
	"<-string$(b%, \"hello\")\n",
	SUBTILIS_ERROR_BAD_ARG_COUNT,
	},
	{"proc_ptr_in_expression",
	"type PROCMark\n"
	"local a@PROCMark\n"
	"print a@PROCMark()\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"bad_function_ptr_param1",
	"type FNMap$(a%)\n"
	"PROCMapper(10, !FNstr$)\n"
	"def FNstr$(a$) <- a$ + a$\n"
	"def PROCMapper(s%, a@FNMap)\n"
	"  print a@FNMap(s%)\n"
	"endproc\n",
	SUBTILIS_ERROR_BAD_ARG_TYPE,
	},
	{"bad_function_ptr_param2",
	"type FNMap%(a$)\n"
	"PROCMapper(\"hello\", !FNstr$)\n"
	"def FNstr$(a$) <- a$ + a$\n"
	"def PROCMapper(s$, a@FNMap)\n"
	"  print a@FNMap(s$)\n"
	"endproc\n",
	SUBTILIS_ERROR_BAD_ARG_TYPE,
	},
	{"bad_function_for",
	"type PROCVoid\n"
	"for a@PROCVoid = def PROC endproc to def PROC endproc\n"
	"next\n",
	SUBTILIS_ERROR_NUMERIC_EXPECTED,
	},
	{"vector_slice_bad_index1",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b%{} := a%{2 : 1}\n",
	SUBTILIS_ERROR_BAD_SLICE,
	},
	{"vector_slice_bad_index2",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b%{} := a%{-3 : -1}\n",
	SUBTILIS_ERROR_BAD_SLICE,
	},
	{"vector_slice_bad_index3",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b%{} := a%{-1 : 0}\n",
	SUBTILIS_ERROR_BAD_SLICE,
	},
	{"array_empty_slice",
	"dim a%(10)\n"
	"a%() = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b%() := a%(3:3)\n",
	SUBTILIS_ERROR_BAD_SLICE,
	},
	{"array_2d_slice",
	"local dim a%(10,10)\n"
	"b%() := a%(1:2)\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"swap_numeric",
	"local a%\n"
	"local b\n"
	"swap a%, b\n",
	SUBTILIS_ERROR_SWAP_TYPE_MISMATCH,
	},
	{"swap_numeric_constant",
	"local a%\n"
	"swap a%, 10\n",
	SUBTILIS_ERROR_LVALUE_EXPECTED,
	},
	{"swap_string_constant",
	"local a$\n"
	"swap a$, \"hello\"\n",
	SUBTILIS_ERROR_LVALUE_EXPECTED,
	},
	{"swap_fn_mismatch",
	"type FNOne(a%)\n"
	"type FNTwo(a$)\n"
	"local a@FNOne\n"
	"local b@FNTwo\n"
	"swap a@FNOne, b@FNTwo\n",
	SUBTILIS_ERROR_SWAP_TYPE_MISMATCH,
	},
	{"swap_array_mismatch",
	"dim a%(1)\n"
	"dim b%(1,1)\n"
	"swap a%(), b%()\n",
	SUBTILIS_ERROR_SWAP_TYPE_MISMATCH,
	},
	{"swap_array_mismatch2",
	"dim a%(1)\n"
	"dim b%(2)\n"
	"swap a%(), b%()\n",
	SUBTILIS_ERROR_SWAP_TYPE_MISMATCH,
	},
	{"swap_array_slice",
	 "c% := 10\n"
	 "dim a%(c%)\n"
	 "dim b%(c%)\n"
	 "swap a%(), b%(5:6)\n",
	 SUBTILIS_ERROR_LVALUE_EXPECTED,
	},
	{"swap_vector_slice",
	 "dim a%{10}\n"
	 "dim b%{10}\n"
	 "swap a%{}, b%{5:6}\n",
	 SUBTILIS_ERROR_LVALUE_EXPECTED,
	},
	{"swap_string",
	 "a$ := \"hello\"\n"
	 "b$ := \"world\"\n"
	 "swap b$, left$(a$)\n",
	 SUBTILIS_ERROR_LVALUE_EXPECTED,
	},
	{"rec_var_dim",
	 "a% = 10\n"
	 "type RECData (\n"
	 "dim arr%(a%)\n"
	 ")\n",
	 SUBTILIS_ERROR_CONST_INTEGER_EXPECTED
	},
	{"rec_bad_keyword",
	 "type RECData ( draw arr%(a%) )\n",
	 SUBTILIS_ERROR_EXPECTED
	},
	{"rec_bad_dim",
	 "type RECData ( dim a%() )\n",
	 SUBTILIS_ERROR_CONST_INTEGER_EXPECTED
	},
	{"rec_id_expected",
	 "type RECData ( a%, b% )\n",
	 SUBTILIS_ERROR_ID_EXPECTED
	},
	{"rec_type_mismatch",
	"type RECData ( a% )\n"
	"type RECData2 ( a% )\n"
	"local a@RECData\n"
	"local b@RECData2\n"
	"a@RECData = b@RECData2\n",
	SUBTILIS_ERROR_BAD_CONVERSION
	},
	{"rec_type_already_defined",
	"type RECData ( a% )\n"
	"type RECData ( a% )\n",
	SUBTILIS_ERROR_ALREADY_DEFINED
	},
	{"rec_field_already_defined",
	 "type RECData ( a% a% )\n",
	SUBTILIS_ERROR_ALREADY_DEFINED
	},
	{"rec_field_array_reference",
	 "type RECData ( a%() )\n",
	SUBTILIS_ERROR_ID_EXPECTED
	},
	{"rec_field_vector_reference",
	 "type RECData ( a%{} )\n",
	SUBTILIS_ERROR_ID_EXPECTED
	},
	{"rec_no_fields",
	 "type RECa2 ( )\n",
	 SUBTILIS_ERROR_EMPTY_REC
	},
	{"rec_type_in_proc",
	 "def PROCMark\n"
	 "  type RECa2 ( a% )\n"
	 "endproc\n",
	SUBTILIS_ERROR_TYPE_NOT_AT_TOP_LEVEL
	},
	{"rec_type_init_too_many_fields",
	 "type RECa2 ( a% )\n"
	 "local a@RECa2 = ( 10, 11)\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"rec_type_init_wrong_type",
	 "type RECa2 ( dim a%(1) )\n"
	 "local a@RECa2 = ( \"hello\" )\n",
	 SUBTILIS_ERROR_NOT_ARRAY_OR_VECTOR,
	},
	{"rec_type_init_missing_r_bracket",
	"type RECa2 ( a% )\n"
	"local a@RECa2 = ( 10\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"rec_type_init_missing_exp",
	 "type RECa2 ( a% b% )\n"
	 "local a@RECa2 = ( 10, )\n",
	 SUBTILIS_ERROR_EXP_EXPECTED,
	},
	{"vector_fn_mismatch",
	 "type PROCfn(a%)\n"
	 "type PROCfn2(a%, b%)\n"
	 "dim a@PROCfn{1}\n"
	 "dim a@PROCfn2{1}\n"
	 "a@PROCfn{} = a@PROCfn2{}\n",
	 SUBTILIS_ERROR_FN_TYPE_MISMATCH,
	},
	{"array_fn_mismatch",
	 "type PROCfn(a%)\n"
	 "type PROCfn2(a%, b%)\n"
	 "dim a@PROCfn(1)\n"
	 "dim a@PROCfn2(1)\n"
	 "a@PROCfn() = a@PROCfn2()\n",
	 SUBTILIS_ERROR_FN_TYPE_MISMATCH,
	},
	{"range_rec_type",
	"type RECscalar (\n"
	"     a%\n"
	"     b\n"
	")\n"
	"type RECscalar2 (\n"
	"     z\n"
	"     f\n"
	")\n"
	"\n"
	"dim a@RECscalar(1)\n"
	"range v@RECscalar2 = a@RECscalar()\n"
	"    print v@RECscalar2.a%\n"
	"    print v@RECscalar2.b\n"
	"endrange\n",
	SUBTILIS_ERROR_RANGE_TYPE_MISMATCH,
	},
	{"rec_proc_assign",
	"type PROCMark(a$)\n"
	"type RECmixed ( b@PROCMark )\n"
	"dim a@RECmixed(1)\n"
	"a@RECmixed(0).b@PROCMark = def PROC(a$) print \"Hello\" + a$ endproc\n"
	"b@PROCMark = a@RECmixed(0).b@PROCMark(\" everyone in\")\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"type_rec_proc_assign",
	"type PROCMark\n"
	"type RECmixed ( b@PROCMark )\n"
	"dim a@RECmixed(1)\n"
	"a@RECmixed(0).b@PROCMark =\n"
	"  def PROC(a$) print \"Hello\" + a$ endproc\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"type_rec_proc_assign_partial",
	"type PROCMark\n"
	"type RECmixed ( b@PROCMark )\n"
	"dim a@RECmixed(1)\n"
	"a@RECmixed(0).b@PROCMark = !PROClater\n"
	"def PROClater(a$) endproc\n",
	SUBTILIS_ERROR_BAD_ARG_COUNT,
	},
	{"rec_type_init",
	"type RECmixed ( a% b% )\n"
	"local c@RECmixed = (10, 11)\n"
	"local d@RECmixed = 1.11\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"nested_array_ref_assign_to_el",
	"dim a%(10)\n"
	"repeat\n"
	"  b%(0) := a%()\n"
	"until false\n",
	SUBTILIS_ERROR_BAD_INDEX,
	},
	{"array_rec_assign_bad_type",
	"type RECtest(a)\n"
	"dim a@RECtest(10)\n"
	"a@RECtest(0) = 10\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"array_curly",
	"dim a%{1}\n"
	"a%(0) = 1\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"vector_round",
	"dim a%(1)\n"
	"a%{0} = 1\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"array_field_curly",
	"type RECar ( dim a%(1) )\n"
	"a@RECar = ()\n"
	"a@RECar.a%{0} = 10\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"put_rec_non_scalar",
	"type PROCdo(a%)\n"
	"type RECref(\n"
	"    a@PROCdo\n"
	")\n"
	"\n"
	"local dim a@RECref(1)\n"
	"put# 0, a@RECref()\n",
	SUBTILIS_ERROR_EXPECTED,
	},
	{"assign_string_to_string_vector",
	"local dim lines${}\n"
	"\n"
	"lines$ = FNGetLine$(lines${})\n"
	"\n"
	"def FNGetLine$(lines${})\n"
	"   line$ := \"\"\n"
	"<- line$\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"assign_rec_to_existing_vec_rec",
	"type RECop ( op% )\n"
	"\n"
	"local dim r@RECop{}\n"
	"r@RECop = FNGetRec@RECop{}()\n"
	"def FNGetRec@RECop{}\n"
	"  local dim a@RECop{}\n"
	"<-a@RECop{}\n",
	SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"global_array_after_proc",
	"PROCdo\n"
	"dim tokens$(1)\n"
	"tokens$() = \"Hello\", \"World\"\n"
	"\n"
	"def PROCdo\n"
	"for i% := 0 to dim(tokens$(),1)\n"
	"  print tokens$(i%)\n"
	"next\n"
	"endproc\n",
	SUBTILIS_ERROR_GLOBAL_AFTER_PROC,
	},
	{"global_array_ref_after_proc",
	"dim tokens$(1)\n"
	"tokens$() = \"Hello\", \"World\"\n"
	"PROCdo\n"
	"tok$() = tokens$()\n"
	"def PROCdo\n"
	"for i% := 0 to dim(tok$(),1)\n"
	"  print tok$(i%)\n"
	"next\n"
	"endproc\n",
	SUBTILIS_ERROR_GLOBAL_AFTER_PROC,
	},
	{"global_int_after_proc",
	"PROCA\n"
	"a%=12\n"
	"def PROCA\n"
	"  print a%\n"
	"endproc\n",
	SUBTILIS_ERROR_GLOBAL_AFTER_PROC,
	},
	{"global_proc_ref_after_proc",
	"type RECstr (a$)\n"
	"type PROCVoid\n"
	"a@PROCVoid=!PROCA\n"
	"a@PROCVoid()\n"
	"a@RECstr = (\"hello\")\n"
	"def PROCA\n"
	"  print a@RECstr.a$\n"
	"endproc\n",
	SUBTILIS_ERROR_GLOBAL_AFTER_PROC,
	},
	{"global_for_after_proc",
	"PROCdo\n"
	"for i% = 1 to 10 next\n"
	"def PROCdo\n"
	"  print i%\n"
	"endproc\n",
	SUBTILIS_ERROR_GLOBAL_AFTER_PROC,
	},
	{"global_ranger_after_proc",
	"dim a%(10)\n"
	"PROCdo\n"
	"range ~, i% = a%()\n"
	"endrange\n"
	"def PROCdo\n"
	"  print i%\n"
	"endproc\n",
	SUBTILIS_ERROR_GLOBAL_AFTER_PROC,
	},
	{"append_bad_type",
	 "dim a%{}\n"
	 "append(a%{}, 1, \"aa\")\n",
	 SUBTILIS_ERROR_INTEGER_EXPECTED,
	},
	{"append_array_array_gran",
	 "dim a%{2}\n"
	 "dim b%{2}\n"
	 "append(a%{}, b%{}, 16)\n",
	 SUBTILIS_ERROR_TOO_MANY_ARGS,
	},
	{"append_bad_gran",
	 "dim a%{2}\n"
	 "append(a%{}, 1, -1)\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
};

/* clang-format on */
