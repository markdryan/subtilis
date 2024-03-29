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
	SUBTILIS_TEST_CASE_ID_PRINT_HEX,
	SUBTILIS_TEST_CASE_ID_VAL_GOOD,
	SUBTILIS_TEST_CASE_ID_VAL_BAD,
	SUBTILIS_TEST_CASE_ID_VAL_MINUS,
	SUBTILIS_TEST_CASE_ID_INT_VAL,
	SUBTILIS_TEST_CASE_ID_HEX_VAL,
	SUBTILIS_TEST_CASE_ID_VAL_BAD_BASE,
	SUBTILIS_TEST_CASE_ID_STR_NULL_ASSIGN,
	SUBTILIS_TEST_CASE_ID_STR_NULL_INIT,
	SUBTILIS_TEST_CASE_ID_PLOT_AS_INT_ARRAY,
	SUBTILIS_TEST_CASE_ID_PLOT_AS_STRING_ARRAY,
	SUBTILIS_TEST_CASE_ID_PLOT_AS_INT,
	SUBTILIS_TEST_CASE_ID_PLOT_AS_STRING,
	SUBTILIS_TEST_CASE_ID_NESTED_TRY_EXP,
	SUBTILIS_TEST_CASE_ID_HANDLER_AFTER_TRY,
	SUBTILIS_TEST_CASE_ID_TRY_IN_ONERROR,
	SUBTILIS_TEST_CASE_ID_TRY_BACK2BACK,
	SUBTILIS_TEST_CASE_ID_NESTED_TRY_EXP_IN_PROC,
	SUBTILIS_TEST_CASE_ID_TRYONE,
	SUBTILIS_TEST_CASE_ID_BGET_BPUT_EOF,
	SUBTILIS_TEST_CASE_ID_EXT,
	SUBTILIS_TEST_CASE_ID_PTR,
	SUBTILIS_TEST_CASE_ID_INT_BYTE_GLOBAL,
	SUBTILIS_TEST_CASE_ID_BYTE_INT_LOCAL,
	SUBTILIS_TEST_CASE_ID_BYTE_BYTE_LOCAL,
	SUBTILIS_TEST_CASE_ID_BYTE_FN,
	SUBTILIS_TEST_CASE_ID_BYTE_ARRAY_CONV,
	SUBTILIS_TEST_CASE_ID_BYTE_CHR_STR,
	SUBTILIS_TEST_CASE_ID_INTZ_BYTE_INT,
	SUBTILIS_TEST_CASE_ID_ARRAY_BYTE,
	SUBTILIS_TEST_CASE_ID_ARRAY_BYTE_INIT,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_FN4,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_SET,
	SUBTILIS_TEST_CASE_ID_INT_ARRAY_SET,
	SUBTILIS_TEST_CASE_ID_BYTE_ARRAY_SET,
	SUBTILIS_TEST_CASE_ID_REAL_ARRAY_SET,
	SUBTILIS_TEST_CASE_ID_STRING_2DARRAY_SET,
	SUBTILIS_TEST_CASE_ID_INT_2DARRAY_SET,
	SUBTILIS_TEST_CASE_ID_BYTE_2DARRAY_SET,
	SUBTILIS_TEST_CASE_ID_REAL_2DARRAY_SET,
	SUBTILIS_TEST_CASE_ID_STRING_ARRAY_SET_VAR,
	SUBTILIS_TEST_CASE_ID_INT_ARRAY_SET_VAR,
	SUBTILIS_TEST_CASE_ID_BYTE_ARRAY_SET_VAR,
	SUBTILIS_TEST_CASE_ID_REAL_ARRAY_SET_VAR,
	SUBTILIS_TEST_CASE_ID_BLOCK_SET_PUT_STR,
	SUBTILIS_TEST_CASE_ID_BLOCK_SET_PUT_ARRAY,
	SUBTILIS_TEST_CASE_ID_BLOCK_COPY_BYTE_STRING,
	SUBTILIS_TEST_CASE_ID_BLOCK_COPY_BYTE_INT,
	SUBTILIS_TEST_CASE_ID_BLOCK_COPY_BYTE_CONST_STRING,
	SUBTILIS_TEST_CASE_ID_BLOCK_COPY_BYTE_EMPTY_STRING,
	SUBTILIS_TEST_CASE_ID_BLOCK_COPY_STRING_STRING_WOC,
	SUBTILIS_TEST_CASE_ID_BLOCK_COPY_STRING_ARRAY,
	SUBTILIS_TEST_CASE_ID_BLOCK_COPY_STRING_ARRAY_TO_EMPTY,
	SUBTILIS_TEST_CASE_ID_BGET_COW,
	SUBTILIS_TEST_CASE_ID_STRING_STR_CHAR_LEN_0,
	SUBTILIS_TEST_CASE_ID_ZERO_LEN_INT_VECTOR,
	SUBTILIS_TEST_CASE_ID_ZERO_LEN_INT_VECTOR_ASSIGN,
	SUBTILIS_TEST_CASE_ID_ZERO_LEN_STRING_VECTOR,
	SUBTILIS_TEST_CASE_ID_ZERO_LEN_STRING_VECTOR_ASSIGN,
	SUBTILIS_TEST_CASE_ID_APPEND_INT_VECTOR,
	SUBTILIS_TEST_CASE_ID_APPEND_REAL_VECTOR,
	SUBTILIS_TEST_CASE_ID_APPEND_BYTE_VECTOR,
	SUBTILIS_TEST_CASE_ID_APPEND_STRING,
	SUBTILIS_TEST_CASE_ID_APPEND_STRING_VECTOR_CONST,
	SUBTILIS_TEST_CASE_ID_APPEND_STRING_VECTOR,
	SUBTILIS_TEST_CASE_ID_READ_EMPTY_FILE,
	SUBTILIS_TEST_CASE_ID_VECTOR_ARRAY_ARRAY_INTS,
	SUBTILIS_TEST_CASE_ID_VECTOR_ARRAY_ARRAY_REALS,
	SUBTILIS_TEST_CASE_ID_VECTOR_ARRAY_ARRAY_BYTES,
	SUBTILIS_TEST_CASE_ID_VECTOR_ARRAY_ARRAY_STRINGS,
	SUBTILIS_TEST_CASE_ID_VECTOR_ARRAY_ARRAY_EMPTY,
	SUBTILIS_TEST_CASE_ID_COPY_EXP,
	SUBTILIS_TEST_CASE_ID_APPEND_EXP,
	SUBTILIS_TEST_CASE_ID_GET_HASH_LEN_TEST,
	SUBTILIS_TEST_CASE_ID_MEMSET_VECTOR,
	SUBTILIS_TEST_CASE_ID_VECTOR_BAD_INIT,
	SUBTILIS_TEST_CASE_ID_MEMSET_EMPTY_VECTOR,
	SUBTILIS_TEST_CASE_ID_VECTOR_REF_TEST,
	SUBTILIS_TEST_CASE_ID_VECTOR_COPY,
	SUBTILIS_TEST_CASE_ID_VECTOR_AS_PARAM,
	SUBTILIS_TEST_CASE_ID_VECTOR_AS_FUNC,
	SUBTILIS_TEST_CASE_ID_NEGATIVE_ARRAY_DIM,
	SUBTILIS_TEST_CASE_ID_RANGE_INT_LOCAL_NEW,
	SUBTILIS_TEST_CASE_ID_RANGE_INT_LOCAL,
	SUBTILIS_TEST_CASE_ID_RANGE_INT_GLOBAL_NEW,
	SUBTILIS_TEST_CASE_ID_RANGE_INT_GLOBAL,
	SUBTILIS_TEST_CASE_ID_RANGE_STRING_GLOBAL_NEW,
	SUBTILIS_TEST_CASE_ID_RANGE_STRING_GLOBAL,
	SUBTILIS_TEST_CASE_ID_RANGE_STRING_LOCAL_NEW,
	SUBTILIS_TEST_CASE_ID_RANGE_STRING_LOCAL,
	SUBTILIS_TEST_CASE_ID_RANGE_BYTE,
	SUBTILIS_TEST_CASE_ID_RANGE_REAL,
	SUBTILIS_TEST_CASE_ID_RANGE_VECTOR,
	SUBTILIS_TEST_CASE_ID_RANGE_2D_ARRAY,
	SUBTILIS_TEST_CASE_ID_RANGE_1D_ARRAY_VAR,
	SUBTILIS_TEST_CASE_ID_RANGE_1D_ARRAY_VAR_GLOBAL,
	SUBTILIS_TEST_CASE_ID_RANGE_1D_ARRAY_VAR_LOCAL,
	SUBTILIS_TEST_CASE_ID_RANGE_1D_ARRAY_LOCAL_INDEX,
	SUBTILIS_TEST_CASE_ID_RANGE_VECTOR_VAR,
	SUBTILIS_TEST_CASE_ID_RANGE_3D_STRING,
	SUBTILIS_TEST_CASE_ID_FOR_VECTOR_VAR,
	SUBTILIS_TEST_CASE_ID_FN_NAMES,
	SUBTILIS_TEST_CASE_ID_LAMBDA_BASIC,
	SUBTILIS_TEST_CASE_ID_LAMBDA_STRING_ARRAY_FN,
	SUBTILIS_TEST_CASE_ID_LAMBDA_ALIAS,
	SUBTILIS_TEST_CASE_ID_LAMBDA_NESTED,
	SUBTILIS_TEST_CASE_ID_LAMBDA_IN_PROC,
	SUBTILIS_TEST_CASE_ID_LAMBDA_IN_ERROR_HANDLER,
	SUBTILIS_TEST_CASE_ID_CALL_ADDR_LOCAL,
	SUBTILIS_TEST_CASE_ID_CALL_ADDR_GLOBAL,
	SUBTILIS_TEST_CASE_ID_CALL_ADDR_FN,
	SUBTILIS_TEST_CASE_ID_CALL_ADDR_FN_PARAMS,
	SUBTILIS_TEST_CASE_ID_CALL_ADDR_ON_ERROR,
	SUBTILIS_TEST_CASE_ID_DIM_FN_INIT1,
	SUBTILIS_TEST_CASE_ID_DIM_FN_INIT2,
	SUBTILIS_TEST_CASE_ID_DIM_FN_INIT3,
	SUBTILIS_TEST_CASE_ID_DIM_FN_SET1,
	SUBTILIS_TEST_CASE_ID_DIM_FN_SET2,
	SUBTILIS_TEST_CASE_ID_DIM_FN_ZERO,
	SUBTILIS_TEST_CASE_ID_DIM_FN_ZERO_ARGS,
	SUBTILIS_TEST_CASE_ID_DIM_FN_ZERO_REF,
	SUBTILIS_TEST_CASE_ID_FN_MAP,
	SUBTILIS_TEST_CASE_ID_FN_RET_FN,
	SUBTILIS_TEST_CASE_ID_FN_RET_AR_FN,
	SUBTILIS_TEST_CASE_ID_ASSIGN_FN_ARRAY,
	SUBTILIS_TEST_CASE_ID_RECURSIVE_TYPE_1,
	SUBTILIS_TEST_CASE_ID_RECURSIVE_TYPE_2,
	SUBTILIS_TEST_CASE_ID_DIM_FN_PRINT,
	SUBTILIS_TEST_CASE_ID_FUNCTION_PTR_PARAM,
	SUBTILIS_TEST_CASE_ID_MID_STR_COW,
	SUBTILIS_TEST_CASE_ID_FN_ZERO_DIM,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_INT,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_CONST_VAR,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_CONST_WHOLE,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_VAR_WHOLE,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_REFERENCE,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_PARTIAL,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_EMPTY_CONST,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_EMPTY_VAR,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_INT_VAR,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_STRING,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_RETURN,
	SUBTILIS_TEST_CASE_ID_VECTOR_SLICE_APPEND,
	SUBTILIS_TEST_CASE_ID_COPY_INTO_SLICE,
	SUBTILIS_TEST_CASE_ID_ARRAY_SLICE_CONST,
	SUBTILIS_TEST_CASE_ID_ARRAY_SLICE_EMPTY_VAR,
	SUBTILIS_TEST_CASE_ID_ARRAY_SLICE_INT_VAR,
	SUBTILIS_TEST_CASE_ID_ARRAY_SLICE_REAL_VAR,
	SUBTILIS_TEST_CASE_ID_SWAP_PROC,
	SUBTILIS_TEST_CASE_ID_SWAP_INT,
	SUBTILIS_TEST_CASE_ID_SWAP_BYTE,
	SUBTILIS_TEST_CASE_ID_SWAP_REAL,
	SUBTILIS_TEST_CASE_ID_SWAP_STRING,
	SUBTILIS_TEST_CASE_ID_SWAP_ARRAY,
	SUBTILIS_TEST_CASE_ID_SWAP_VECTOR,
	SUBTILIS_TEST_CASE_ID_FN_ARRAY_DEREF,
	SUBTILIS_TEST_CASE_ID_FN_VECTOR_DEREF,
	SUBTILIS_TEST_CASE_ID_REC_ASSIGN_COPY,
	SUBTILIS_TEST_CASE_ID_REC_ZERO,
	SUBTILIS_TEST_CASE_ID_REC_ZERO_INIT,
	SUBTILIS_TEST_CASE_ID_REC_INIT,
	SUBTILIS_TEST_CASE_ID_REC_PARTIAL_INIT,
	SUBTILIS_TEST_CASE_ID_REC_PARTIAL_INIT_GLOBAL,
	SUBTILIS_TEST_CASE_ID_REC_ZERO_INIT_LOCAL_GLOBAL,
	SUBTILIS_TEST_CASE_ID_DIM_ARR_REC,
	SUBTILIS_TEST_CASE_ID_DIM_VEC_REC,
	SUBTILIS_TEST_CASE_ID_REC_COPY_SCALAR,
	SUBTILIS_TEST_CASE_ID_REC_COPY_NON_SCALAR,
	SUBTILIS_TEST_CASE_ID_RANGE_GBL_EMPTY_SLICE,
	SUBTILIS_TEST_CASE_ID_RANGE_REC_LOCAL_NEW,
	SUBTILIS_TEST_CASE_ID_RANGE_REC_GLOBAL_NEW,
	SUBTILIS_TEST_CASE_ID_RANGE_TILDE_LOCAL,
	SUBTILIS_TEST_CASE_ID_RANGE_TILDE_GLOBAL,
	SUBTILIS_TEST_CASE_ID_REC_PROC_FIELD_CALL,
	SUBTILIS_TEST_CASE_ID_ARRAY_PROC_CALL,
	SUBTILIS_TEST_CASE_ID_APPEND_REC,
	SUBTILIS_TEST_CASE_ID_REC_INIT_COPY,
	SUBTILIS_TEST_CASE_ID_REC_RESET,
	SUBTILIS_TEST_CASE_ID_REC_RESET_PARTIAL,
	SUBTILIS_TEST_CASE_ID_ARRAY_REC_RESET,
	SUBTILIS_TEST_CASE_ID_FIELD_REC_RESET,
	SUBTILIS_TEST_CASE_ID_REC_EMPTY_RESET,
	SUBTILIS_TEST_CASE_ID_FN_ARRAY_REF_ASSIGN,
	SUBTILIS_TEST_CASE_ID_REC_ARRAY_REF_ASSIGN,
	SUBTILIS_TEST_CASE_ID_REC_APPEND_ARRAY_SCALAR,
	SUBTILIS_TEST_CASE_ID_REC_APPEND_ARRAY_REF,
	SUBTILIS_TEST_CASE_ID_REC_REF_FN,
	SUBTILIS_TEST_CASE_ID_REC_SCALAR_FN,
	SUBTILIS_TEST_CASE_ID_REC_REF_LAMBDA_FN,
	SUBTILIS_TEST_CASE_ID_REC_REF_FN_PTR,
	SUBTILIS_TEST_CASE_ID_REC_SCALAR_VEC_FN,
	SUBTILIS_TEST_CASE_ID_REC_FN_RETURN_ZERO_REC_REF,
	SUBTILIS_TEST_CASE_ID_REC_FN_RETURN_REC_MULTIPLE_REF,
	SUBTILIS_TEST_CASE_ID_REC_FN_RETURN_REC_NESTED_REF,
	SUBTILIS_TEST_CASE_ID_REC_SWAP_THREE_BYTES,
	SUBTILIS_TEST_CASE_ID_REC_SWAP,
	SUBTILIS_TEST_CASE_ID_REC_ARRAY_SWAP,
	SUBTILIS_TEST_CASE_ID_REC_ARRAY_PUT_GET,
	SUBTILIS_TEST_CASE_ID_COPY_ARRAY_FN,
	SUBTILIS_TEST_CASE_ID_COPY_ARRAY_REC_FN,
	SUBTILIS_TEST_CASE_ID_MID_STR_ALIAS_FN,
	SUBTILIS_TEST_CASE_ID_ASSIGN_EMPTY_VECTOR_FROM_TMP,
	SUBTILIS_TEST_CASE_ID_APPEND_VECTOR_STRING_RETURN,
	SUBTILIS_TEST_CASE_ID_APPEND_VECTOR_REC_REF_RETURN,
	SUBTILIS_TEST_CASE_ID_APPEND_VECTOR_REC_NOREF_RETURN,
	SUBTILIS_TEST_CASE_ID_RESET_VECTOR_STRING_SLICE,
	SUBTILIS_TEST_CASE_ID_SET_VECTOR_STRING_SLICE,
	SUBTILIS_TEST_CASE_ID_APPEND_VECTOR_STRING_SLICE,
	SUBTILIS_TEST_CASE_ID_APPEND_VECTOR_REC_SLICE,
	SUBTILIS_TEST_CASE_ID_APPEND_VECTOR_ARRAY_STRING_RETURN,
	SUBTILIS_TEST_CASE_ID_APPEND_VECTOR_ARRAY_REC_REF_RETURN,
	SUBTILIS_TEST_CASE_ID_STRING_1_4_BYTE_EQ_NEQ,
	SUBTILIS_TEST_CASE_ID_RIGHT_STR_NON_CONST_FULL_IN_BLOCK,
	SUBTILIS_TEST_CASE_ID_LEFT_STR_NON_CONST_FULL_IN_BLOCK,
	SUBTILIS_TEST_CASE_ID_MID_STR_NON_CONST_FULL_IN_BLOCK,
	SUBTILIS_TEST_CASE_ID_APPEND_ASSIGN_TO_SELF_INT,
	SUBTILIS_TEST_CASE_ID_APPEND_ASSIGN_TO_SELF_STRING,
	SUBTILIS_TEST_CASE_ID_APPEND_ASSIGN_TO_SELF_NESTED,
	SUBTILIS_TEST_CASE_ID_APPEND_ASSIGN_TO_SELF_EMPTY,
	SUBTILIS_TEST_CASE_ID_OS_ARGS,
	SUBTILIS_TEST_CASE_ID_MID_STR_VAR,
	SUBTILIS_TEST_CASE_ID_MEMCMP_2,
	SUBTILIS_TEST_CASE_ID_PASS_RETURN_EMPTY_STRING,
	SUBTILIS_TEST_CASE_ID_RETURN_REC_VECTOR_GLOBAL,
	SUBTILIS_TEST_CASE_ID_APPEND_GRAN,
	SUBTILIS_TEST_CASE_ID_APPEND_BAD_GRAN,
	SUBTILIS_TEST_CASE_ID_SWAP_REC_FIELD,
	SUBTILIS_TEST_CASE_ID_MAX,
} subtilis_test_case_id_t;

extern const subtilis_test_case_t test_cases[];

#endif
