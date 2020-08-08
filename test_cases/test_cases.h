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

#ifndef __SUBTILIS_TEST_CASES_H
#define __SUBTILIS_TEST_CASES_H

#include <stdbool.h>

struct subtilis_test_case_t_ {
	const char *name;
	const char *source;
	const char *result;
	bool mem_leaks_ok;
};

typedef struct subtilis_test_case_t_ subtilis_test_case_t;

typedef enum {
	SUBTILIS_TEST_CASE_ID_SUBTRACTION,
	SUBTILIS_TEST_CASE_ID_DIVISION,
	SUBTILIS_TEST_CASE_ID_MODULO,
	SUBTILIS_TEST_CASE_ID_MULTIPLICATION,
	SUBTILIS_TEST_CASE_ID_ADDITION,
	SUBTILIS_TEST_CASE_ID_NESTED_EXP,
	SUBTILIS_TEST_CASE_ID_UNARY_MINUS,
	SUBTILIS_TEST_CASE_ID_AND,
	SUBTILIS_TEST_CASE_ID_OR,
	SUBTILIS_TEST_CASE_ID_EOR,
	SUBTILIS_TEST_CASE_ID_NOT,
	SUBTILIS_TEST_CASE_ID_EQ,
	SUBTILIS_TEST_CASE_ID_NEQ,
	SUBTILIS_TEST_CASE_ID_GT,
	SUBTILIS_TEST_CASE_ID_LTE,
	SUBTILIS_TEST_CASE_ID_LT,
	SUBTILIS_TEST_CASE_ID_GTE,
	SUBTILIS_TEST_CASE_ID_IF,
	SUBTILIS_TEST_CASE_ID_WHILE,
	SUBTILIS_TEST_CASE_ID_IF_AND_WHILE,
	SUBTILIS_TEST_CASE_ID_EQ_AND_NEQ_BR_IMM,
	SUBTILIS_TEST_CASE_ID_EQ_AND_NEQ_BR,
	SUBTILIS_TEST_CASE_ID_BASIC_PROC,
	SUBTILIS_TEST_CASE_ID_LOCAL_PROC,
	SUBTILIS_TEST_CASE_ID_PROC_ARGS,
	SUBTILIS_TEST_CASE_ID_FN_FACT,
	SUBTILIS_TEST_CASE_ID_FN_FACT_NO_LET,
	SUBTILIS_TEST_CASE_ID_ABS,
	SUBTILIS_TEST_CASE_ID_FPA_SMALL,
	SUBTILIS_TEST_CASE_ID_FPA_LOGICAL,
	SUBTILIS_TEST_CASE_ID_FPA_IF,
	SUBTILIS_TEST_CASE_ID_FPA_REPEAT,
	SUBTILIS_TEST_CASE_ID_INT_REG_ALLOC_BASIC,
	SUBTILIS_TEST_CASE_ID_FPA_REG_ALLOC_BASIC,
	SUBTILIS_TEST_CASE_ID_FPA_SAVE,
	SUBTILIS_TEST_CASE_ID_INT_SAVE,
	SUBTILIS_TEST_CASE_ID_BRANCH_SAVE,
	SUBTILIS_TEST_CASE_ID_TIME,
	SUBTILIS_TEST_CASE_ID_SIN_COS,
	SUBTILIS_TEST_CASE_ID_PI,
	SUBTILIS_TEST_CASE_ID_SQR,
	SUBTILIS_TEST_CASE_ID_MIXED_ARGS,
	SUBTILIS_TEST_CASE_ID_VDU,
	SUBTILIS_TEST_CASE_ID_VOID_FN,
	SUBTILIS_TEST_CASE_ID_ASSIGN_FN,
	SUBTILIS_TEST_CASE_ID_FOR_BASIC_INT,
	SUBTILIS_TEST_CASE_ID_FOR_BASIC_REAL,
	SUBTILIS_TEST_CASE_ID_FOR_STEP_INT_CONST,
	SUBTILIS_TEST_CASE_ID_FOR_STEP_REAL_CONST,
	SUBTILIS_TEST_CASE_ID_FOR_STEP_INT_VAR,
	SUBTILIS_TEST_CASE_ID_FOR_STEP_REAL_VAR,
	SUBTILIS_TEST_CASE_ID_FOR_MOD_STEP,
	SUBTILIS_TEST_CASE_ID_POINT_TINT,
	SUBTILIS_TEST_CASE_ID_REG_ALLOC_BUSTER,
	SUBTILIS_TEST_CASE_ID_SAVE_THIRD,
	SUBTILIS_TEST_CASE_ID_SAVE_USELESS_MOVE,
	SUBTILIS_TEST_CASE_ID_SAVE_FIXED_REG_ARG,
	SUBTILIS_TEST_CASE_ID_CMP_IMM,
	SUBTILIS_TEST_CASE_ID_ASSIGNMENT_OPS,
	SUBTILIS_TEST_CASE_ID_INT,
	SUBTILIS_TEST_CASE_ID_TRIG,
	SUBTILIS_TEST_CASE_ID_LOG,
	SUBTILIS_TEST_CASE_ID_RND_INT,
	SUBTILIS_TEST_CASE_ID_RND_REAL,
	SUBTILIS_TEST_CASE_ID_PRINTFP,
	SUBTILIS_TEST_CASE_ID_END,
	SUBTILIS_TEST_CASE_ID_ERROR_ENDPROC_EARLY,
	SUBTILIS_TEST_CASE_ID_ERROR_HANDLED,
	SUBTILIS_TEST_CASE_ID_ERROR_HANDLED_FN,
	SUBTILIS_TEST_CASE_ID_ERROR_MAIN,
	SUBTILIS_TEST_CASE_ID_ERROR_MAIN_END,
	SUBTILIS_TEST_CASE_ID_ERROR_NESTED,
	SUBTILIS_TEST_CASE_ID_ERROR_NESTED_ENDFN_EARLY,
	SUBTILIS_TEST_CASE_ID_ERROR_NESTED_ENDPROC_EARLY,
	SUBTILIS_TEST_CASE_ID_ERROR_NESTED_FN,
	SUBTILIS_TEST_CASE_ID_ERROR_THROW_FN,
	SUBTILIS_TEST_CASE_ID_ERROR_ERROR_IN_MAIN,
	SUBTILIS_TEST_CASE_ID_ERROR_DIV_BY_ZERO,
	SUBTILIS_TEST_CASE_ID_ERROR_LOGRANGE,
	SUBTILIS_TEST_CASE_ID_END_IN_PROC,
	SUBTILIS_TEST_CASE_ID_ARRAY_FOR_INT,
	SUBTILIS_TEST_CASE_ID_ARRAY_FOR_DOUBLE,
	SUBTILIS_TEST_CASE_ID_ARRAY2D_INT_CONST,
	SUBTILIS_TEST_CASE_ID_ARRAY2D_INT_VAR,
	SUBTILIS_TEST_CASE_ID_ARRAY_BAD_INDEX_VAR,
	SUBTILIS_TEST_CASE_ID_ARRAY2D_BAD_INDEX_VAR,
	SUBTILIS_TEST_CASE_ID_ARRAY2D_INT_ZERO,
	SUBTILIS_TEST_CASE_ID_ARRAY2D_REAL_ZERO,
	SUBTILIS_TEST_CASE_ID_ARRAY1D_INT_PLUS_EQUALS,
	SUBTILIS_TEST_CASE_ID_ARRAY1D_INT_MINUS_EQUALS,
	SUBTILIS_TEST_CASE_ID_ARRAY1D_REAL_PLUS_EQUALS,
	SUBTILIS_TEST_CASE_ID_ARRAY1D_REAL_MINUS_EQUALS,
	SUBTILIS_TEST_CASE_ID_ARRAY2D_INT_PLUS_EQUALS,
	SUBTILIS_TEST_CASE_ID_FOR_ARRAY_INT_VAR,
	SUBTILIS_TEST_CASE_ID_FOR_ARRAY_REAL_VAR,
	SUBTILIS_TEST_CASE_ID_FOR_ARRAY_INT_VAR_STEP,
	SUBTILIS_TEST_CASE_ID_FOR_ARRAY_REAL_VAR_STEP,
	SUBTILIS_TEST_CASE_ID_FOR_ARRAY_INT_VAR_STEP_VAR,
	SUBTILIS_TEST_CASE_ID_DIM_OOM,
	SUBTILIS_TEST_CASE_ID_LOCAL_DIM_OOM,
	SUBTILIS_TEST_CASE_ID_GLOBAL_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_IF,
	SUBTILIS_TEST_CASE_ID_LOCAL_LOOP_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_PROC_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_IF_FN_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_PROC_NESTED_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_ON_ERROR_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_PROC_ON_ERROR_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_PROC_RETURN_DEREF,
	SUBTILIS_TEST_CASE_ID_LOCAL_FN_RETURN_DEREF,
	SUBTILIS_TEST_CASE_ID_ERROR_DEREF,
	SUBTILIS_TEST_CASE_GLOBAL_SHADOW,
	SUBTILIS_TEST_CASE_FOR_GLOBAL_SHADOW,
	SUBTILIS_TEST_CASE_REM,
	SUBTILIS_TEST_CASE_ARRAY_GLOBAL_ASSIGN,
	SUBTILIS_TEST_CASE_ARRAY_LOCAL_ASSIGN,
	SUBTILIS_TEST_CASE_ARRAY_GLOBAL_LOCAL_ASSIGN,
	SUBTILIS_TEST_CASE_ARRAY_INT_ARG,
	SUBTILIS_TEST_CASE_ARRAY_TWO_INT_ARG,
	SUBTILIS_TEST_CASE_ARRAY_INT_ARG_FIFTH,
	SUBTILIS_TEST_CASE_ARRAY_REAL_ARG,
	SUBTILIS_TEST_CASE_ARRAY_TWO_REAL_ARG,
	SUBTILIS_TEST_CASE_ARRAY_REAL_ARG_FIFTH,
	SUBTILIS_TEST_CASE_ARRAY_GLOBAL_ARG,
	SUBTILIS_TEST_CASE_ARRAY_CONSTANT_DIM,
	SUBTILIS_TEST_CASE_ARRAY_DYNAMIC_DIM,
	SUBTILIS_TEST_CASE_ARRAY_DYNAMIC_DIM_ZERO,
	SUBTILIS_TEST_CASE_ARRAY_BAD_DYNAMIC_DIM,
	SUBTILIS_TEST_CASE_ARRAY_ASSIGN_REF,
	SUBTILIS_TEST_CASE_ARRAY_ASSIGN_RETURN_REF,
	SUBTILIS_TEST_CASE_ARRAY_ASSIGN_RETURN_ARR,
	SUBTILIS_TEST_CASE_ARRAY_TEMPORARIES,
	SUBTILIS_TEST_CASE_ARRAY_ASSIGN_LOCAL_REF,
	SUBTILIS_TEST_CASE_ARRAY_ASSIGN_LOCAL_RETURN_REF,
	SUBTILIS_TEST_CASE_ARRAY_DOUBLE_REF,
	SUBTILIS_TEST_CASE_SGN,
	SUBTILIS_TEST_CASE_ARRAY_DIM_MIXED,
	SUBTILIS_TEST_CASE_NESTED_BLOCKS,
	SUBTILIS_TEST_CASE_LOCAL_INITIALISE_FROM_LOCAL,
	SUBTILIS_TEST_CASE_FN_LOCAL_ARRAY,
	SUBTILIS_TEST_CASE_ID_DYN_ARRAY_1D_TOO_MANY_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_DYN_ARRAY_2D_TOO_MANY_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_ARRAY_1D_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_ARRAY_3D_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_DYN_ARRAY_1D_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_ARRAY_1D_DBL_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_DYN_ARRAY_1D_DBL_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_CONST_ARRAY_3D_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_ARRAY_3D_MIXED_INITIALISERS,
	SUBTILIS_TEST_CASE_ID_REG_ALLOC_SWI,
	SUBTILIS_TEST_CASE_ID_STRING_BASIC,
	SUBTILIS_TEST_CASE_ID_PRINT_SEMI_COLON,
	SUBTILIS_TEST_CASE_ID_CHR_STR,
	SUBTILIS_TEST_CASE_ID_ASC,
	SUBTILIS_TEST_CASE_ID_LEN,
	SUBTILIS_TEST_CASE_ID_SPC,
	SUBTILIS_TEST_CASE_ID_TAB_ONE_ARG,
	SUBTILIS_TEST_CASE_ID_STRING_ARGS,
	SUBTILIS_TEST_CASE_ID_FN_STRING,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_FN,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_FN2,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_FN3,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_1_EL,
	SUBTILIS_TEST_CASE_ID_RELATED_FOR_LOOPS,
	SUBTILIS_TEST_CASE_ID_POW,
	SUBTILIS_TEST_CASE_ID_STR_EQ,
	SUBTILIS_TEST_CASE_ID_STR_NEQ,
	SUBTILIS_TEST_CASE_ID_STR_LT,
	SUBTILIS_TEST_CASE_ID_STR_LTE,
	SUBTILIS_TEST_CASE_ID_STR_GT,
	SUBTILIS_TEST_CASE_ID_STR_GTE,
	SUBTILIS_TEST_CASE_ID_LEFT_STR,
	SUBTILIS_TEST_CASE_ID_RIGHT_STR,
	SUBTILIS_TEST_CASE_ID_RIGHT_STR2,
	SUBTILIS_TEST_CASE_ID_MID$_STR,
	SUBTILIS_TEST_CASE_ID_MID$_STR2,
	SUBTILIS_TEST_CASE_ID_MID$_STR3,
	SUBTILIS_TEST_CASE_ID_NESTED_ARRAY_REF,
	SUBTILIS_TEST_CASE_ID_STRING_STR,
	SUBTILIS_TEST_CASE_ID_STR_STR,
	SUBTILIS_TEST_CASE_ID_STR_ADD1,
	SUBTILIS_TEST_CASE_ID_STR_EXP,
	SUBTILIS_TEST_CASE_ID_HEAP_BUSTER,
	SUBTILIS_TEST_CASE_ID_STRING_ADD_EQUALS,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_ADD_EQUALS,
	SUBTILIS_TEST_CASE_ID_LEFT_STR_STATEMENT,
	SUBTILIS_TEST_CASE_ID_RIGHT_STR_STATEMENT,
	SUBTILIS_TEST_CASE_ID_MID_STR_STATEMENT,
	SUBTILIS_TEST_CASE_ID_MIN_INT32,
	SUBTILIS_TEST_CASE_ID_MAX,
} subtilis_test_case_id_t;

extern const subtilis_test_case_t test_cases[];

#endif
