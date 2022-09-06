/*
 * Copyright (c) 2020 Mark Ryan
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

#include "../../arch/arm32/arm_disass.h"
#include "../../arch/arm32/arm_encode.h"
#include "../../arch/arm32/arm_keywords.h"
#include "../../arch/arm32/arm_vm.h"
#include "../../arch/arm32/vfp_gen.h"
#include "../../frontend/parser_test.h"
#include "../../test_cases/bad_test_cases.h"
#include "../../test_cases/test_cases.h"
#include "../ptd/ptd.h"
#include "../riscos_common/riscos_arm.h"
#include "ptd_test.h"

static int prv_test_example(subtilis_lexer_t *l, subtilis_parser_t *p,
			    subtilis_error_type_t expected_err,
			    const char *expected, bool mem_leaks_ok)
{
	subtilis_error_t err;
	subtilis_buffer_t b;
	size_t code_size;
	subtilis_arm_fp_if_t fp_if;
	int retval = 1;
	subtilis_arm_op_pool_t *pool = NULL;
	subtilis_arm_prog_t *arm_p = NULL;
	subtilis_arm_vm_t *vm = NULL;
	uint8_t *code = NULL;
	char *argv[2] = {"./runptd", "unit_test"};

	subtilis_error_init(&err);
	subtilis_buffer_init(&b, 1024);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->backend.backend_data = pool;

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		if (err.type == expected_err)
			retval = 0;
		goto cleanup;
	}

	//	subtilis_ir_prog_dump(p->prog);
	subtilis_arm_vfp_if_init(&fp_if);

	arm_p = subtilis_riscos_generate(
	    pool, p->prog, ptd_rules, ptd_rules_count, p->st->max_allocated,
	    &fp_if, SUBTILIS_PTD_PROGRAM_START, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	subtilis_arm_prog_dump(arm_p);

	code = subtilis_arm_encode_buf(arm_p, &code_size, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		if (err.type == expected_err)
			retval = 0;
		goto cleanup;
	}

	/* Insert heap start */

	if (code_size < 8) {
		subtilis_error_set_assertion_failed(&err);
		goto cleanup;
	}

	((uint32_t *)code)[1] = SUBTILIS_PTD_PROGRAM_START + code_size;

	//	for (size_t i = 0; i < code_size; i++) {
	//		printf("0x%x\n",code[i]);
	///	}
	vm = subtilis_arm_vm_new(code, code_size, 512 * 1024,
				 SUBTILIS_PTD_PROGRAM_START, true, 2, argv,
				 &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_vm_run(vm, &b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_buffer_zero_terminate(&b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (strcmp(subtilis_buffer_get_string(&b), expected)) {
		printf("%s expected got %s\n", expected,
		       subtilis_buffer_get_string(&b));
		retval = 1;
		goto cleanup;
	}

	retval = 0;

cleanup:

	if (retval != 0) {
		subtilis_error_fprintf(stdout, &err, true);
	} else if (err.type != expected_err) {
		fprintf(stderr, "expected error %u got %u\n", expected_err,
			err.type);
		retval = 1;
	}

	subtilis_arm_vm_delete(vm);
	free(code);
	subtilis_arm_prog_delete(arm_p);
	subtilis_arm_op_pool_delete(pool);
	subtilis_buffer_free(&b);

	return retval;
}

static int prv_test_ptd_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	subtilis_backend_t backend;
	int ret = 0;

	backend.caps = SUBTILIS_PTD_CAPS;
	backend.sys_trans = subtilis_ptd_sys_trans;
	backend.sys_check = subtilis_ptd_sys_check;
	backend.backend_data = NULL;
	backend.asm_parse = subtilis_ptd_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	for (i = 0; i < SUBTILIS_TEST_CASE_ID_MAX; i++) {
		test = &test_cases[i];

		switch (i) {
		case SUBTILIS_TEST_CASE_ID_SIN_COS:
		case SUBTILIS_TEST_CASE_ID_POINT_TINT:
		case SUBTILIS_TEST_CASE_ID_TRIG:
		case SUBTILIS_TEST_CASE_ID_LOG:
		case SUBTILIS_TEST_CASE_ID_ERROR_LOGRANGE:
		case SUBTILIS_TEST_CASE_ID_POW:
		case SUBTILIS_TEST_CASE_ID_STR_EXP:
			continue;
		default:
			break;
		}

		printf("ptd_%s", test->name);
		pass = parser_test_wrapper(
		    test->source, &backend, prv_test_example,
		    subtilis_arm_keywords_list, SUBTILIS_ARM_KEYWORD_TOKENS,
		    SUBTILIS_ERROR_OK, test->result, test->mem_leaks_ok);
		ret |= pass;
	}

	return ret;
}

/* clang-format off */
static const subtilis_test_case_t riscos_vfp_test_cases[] = {
	{
	"assembler_vfp_loop",
	"PROCFPLoop(10)\n"
	"\n"
	"def PROCFPLoop(a)\n"
	"[\n"
	"    MOV R0, 33\n"
	"    ADR R1, value\n"
	"    FLDD D1, [R1]"
	"start:\n"
	"   SWI \"OS_WriteC\"\n"
	"   FSUBD D0, D0, D1\n"
	"   FCMPZD D0\n"
	"   FMSTAT\n"
	"   BGT start\n"
	"   MOV PC, R14\n"
	"value:"
	"   EQUDBL 0.5\n"
	"]\n",
	"!!!!!!!!!!!!!!!!!!!!",
	},
	{"assembler_vfp_sqr",
	"PROCCheck(SQR(2), FNSQR(2))\n"
	"PROCCheck(SQR(2), FNSQRFixed)\n"
	"DEF FNSQR(a) [ FSQRTD D0, D0 MOV PC, R14 ]\n"
	"DEF FNSQRFixed\n"
	"[\n"
	"  ADR R0, value\n"
	"  FLDD D0, [R0]\n"
	"  MOV PC, R14\n"
	"value: EQUDBL SQR(2)\n"
	"]\n"
	"DEF PROCCheck(a, e)\n"
	"LET a = e - a\n"
	"IF a < 0.0 THEN LET a = -a ENDIF\n"
	"PRINT a < 0.001\n"
	"ENDPROC\n",
	"-1\n-1\n"},
	{"assembler_vfp_abs",
	"PROCCheck(FNAbs(-1), 1)\n"
	"PROCCheck(FNAbsFixed, 1)\n"
	"DEF FNAbs(a) [ FABSD D0, D0 MOV PC, R14 ]\n"
	"DEF FNAbsFixed\n"
	"[\n"
	"  ADR R0, value\n"
	"  FLDD D0, [R0]\n"
	"  MOV PC, R14\n"
	"value: EQUDBL ABS(-1)\n"
	"]\n"
	"DEF PROCCheck(a, e)\n"
	"LET a = e - a\n"
	"IF a < 0.0 THEN LET a = -a ENDIF\n"
	"PRINT a < 0.001\n"
	"ENDPROC\n",
	"-1\n-1\n"},
	{"assembler_vfp_conv",
	"PRINT FNFPAConv%(10)\n"
	"def FNFPAConv%(a%)\n"
	"[\n"
	"FMSR S0, R0\n"
	"FSITOD D0, S0\n"
	"ADR R0, value1\n"
	"FLDD D1, [R0]\n"
	"ADR R0, value2\n"
	"FLDD D2, [R0]\n"
	"FMULD D0, D0, D1\n"
	"FDIVD D0, D0, D2\n"
	"FTOSID S0, D0\n"
	"FMRS R0, S0\n"
	"MOV PC, R14\n"
	"value1:\n"
	"EQUDBL 2.0"
	"value2:\n"
	"EQUDBL 5.0"
	"]\n",
	"4\n"},
	{"assembler_vfp_array",
	"local dim a(9)\n"
	"PROCInitRealArray(a())\n"
	"for i% := 0 to dim(a(),1)\n"
	"print a(i%)\n"
	"next\n"
	"\n"
	"def PROCInitRealArray(a(1))\n"
	"[\n"
	"def array=R1\n"
	"LDR array, [R0, 4]\n"
	"ADR R0, nums\n"
	"SUB R0, R0, 8\n"
	"loop:\n"
	"\n"
	"FLDD D0, [R0, 8]!\n"
	"FCMPZD D0\n"
	"FMRX R15, FPSCR\n"
	"MOVEQ PC, R14\n"
	"FSTD D0, [array], 8\n"
	"B loop\n"
	"nums:\n"
	"EQUDBL 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0.0\n"
	"]\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"},
	{
	"assembler_vfp_cptran",
	"PRINT FNWFSRFS%(7)\n"
	"def FNWFSRFS%(a%)\n"
	"[\n"
	"FMXR FPSCR, R0\n"
	"FMRX R0, FPSCR\n"
	"MOV PC, R14\n"
	"]\n",
	"7\n",
	},
	{
	"assembler_vfp_double_reg_tran",
	"dim a%(1)\n"
	"PROCDblToInts(329, a%())\n"
	"print FNIntsToDnl(a%())\n"
	"def PROCDblToInts(num, a%(1))\n"
	"[\n"
	"	LDR R1, [R0, 4]\n"
	"	FMRRD R2, R3, D0\n"
	"	STR R2, [R0]!\n"
	"	STR R3, [R0]\n"
	"	MOV PC, R14\n"
	"]\n"
	"def FNIntsToDnl(a%(1))\n"
	"[\n"
	"  LDR R1, [R0, 4]\n"
	"  LDR R2, [R0]!\n"
	"  LDR R3, [R0]\n"
	"  FMDRR D0, R2, R3\n"
	"  MOV PC, R14\n"
	"]\n",
	"329\n",
	},
	{
	"assembler_vfp_float_loop",
	"PROCFPLoop(10)\n"
	"\n"
	"def PROCFPLoop(a)\n"
	"[\n"
	"  MOV R0, 33\n"
	"  FCVTSD S0, D0\n"
	"  ADR R1, value\n"
	"  FLDS S1, [R1]\n"
	"start:\n"
	"  SWI \"OS_WriteC\"\n"
	"  FSUBS S0, S0, S1\n"
	"  FCMPZS S0\n"
	"  FMSTAT\n"
	"  BGT start\n"
	"  MOV PC, R14\n"
	"value:\n"
	"  EQUF 0.5\n"
	"]\n",
	"!!!!!!!!!!!!!!!!!!!!",
	},
	{"assembler_vfp_float_sqr",
	"PROCCheck(SQR(2), FNSQR(2))\n"
	"PROCCheck(SQR(2), FNSQRFixed)\n"
	"DEF FNSQR(a)\n"
	"[\n"
	"  FCVTSD S0, D0\n"
	"  FSQRTS S0, S0\n"
	"  FCVTDS D0, S0\n"
	"  MOV PC, R14\n"
	"]\n"
	"DEF FNSQRFixed\n"
	"[\n"
	"  ADR R0, value\n"
	"  FLDS S0, [R0]\n"
	"  FCVTDS D0, S0\n"
	"  MOV PC, R14\n"
	"value: EQUF SQR(2)\n"
	"]\n"
	"DEF PROCCheck(a, e)\n"
	"LET a = e - a\n"
	"IF a < 0.0 THEN LET a = -a ENDIF\n"
	"PRINT a < 0.001\n"
	"ENDPROC\n",
	"-1\n-1\n"},
	{"assembler_vfp_float_abs",
	"PROCCheck(FNAbs(-1), 1)\n"
	"PROCCheck(FNAbsFixed, 1)\n"
	"DEF FNAbs(a)\n"
	"[\n"
	"  FCVTSD S0, D0\n"
	"  FABSS S0, S0\n"
	"  FCVTDS D0, S0\n"
	"  MOV PC, R14\n"
	"]\n"
	"DEF FNAbsFixed\n"
	"[\n"
	"  ADR R0, value\n"
	"  FLDS S0, [R0]\n"
	"  FCVTDS D0, S0\n"
	"  MOV PC, R14\n"
	"value: EQUF ABS(-1)\n"
	"]\n"
	"DEF PROCCheck(a, e)\n"
	"LET a = e - a\n"
	"IF a < 0.0 THEN LET a = -a ENDIF\n"
	"PRINT a < 0.001\n"
	"ENDPROC\n",
	"-1\n-1\n"},
	{"assembler_vfp_conv",
	"PRINT FNFPAConv%(10)\n"
	"def FNFPAConv%(a%)\n"
	"[\n"
	"FMSR S0, R0\n"
	"ADR R0, value1\n"
	"FLDS S1, [R0]\n"
	"ADR R0, value2\n"
	"FLDS S2, [R0]\n"
	"FMULS S0, S0, S1\n"
	"FDIVS S0, S0, S2\n"
	"FMRS R0, S0\n"
	"MOV PC, R14\n"
	"value1:\n"
	"EQUF 2.0"
	"value2:\n"
	"EQUF 5.0"
	"]\n",
	"4\n"},
	{"assembler_stran_misc",
	"print FNSignE%\n"
	"print FNSignEH%\n"
	"print FNUnsignH%\n"
	"print FNDword%\n"
	"\n"
	"dim a%(1)\n"
	"a%(0) = &ffff0000\n"
	"PROCStoreH(a%())\n"
	"print ~a%(0)\n"
	"\n"
	"dim b%(1)\n"
	"PROCStoreD(b%())\n"
	"print b%(0)\n"
	"print b%(1)\n"
	"\n"
	"b%() = 0, 0\n"
	"\n"
	"print FNPreSignE%\n"
	"print FNPreSignEH%\n"
	"print FNPreUnsignH%\n"
	"print FNPreDword%\n"
	"\n"
	"a%(0) = &ffff0000\n"
	"PROCPreStoreH(a%())\n"
	"print ~a%(0)\n"
	"\n"
	"PROCPreStoreD(b%())\n"
	"print b%(0)\n"
	"print b%(1)\n"
	"\n"
	"print FNSignENoOffset%\n"
	"\n"
	"\n"
	"def FNSignE%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRSB R0,[R1],0\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUB -33\n"
	"]\n"
	"\n"
	"def FNSignEH%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRSH R0,[R1],0\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUW -1000\n"
	"]\n"
	"\n"
	"def FNUnsignH%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRH R0,[R1],0\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUW -1\n"
	"]\n"
	"\n"
	"def FNDword%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRD R0,[R1],0\n"
	"ADD R0, R0, R1\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUD 1\n"
	"EQUD 2\n"
	"]\n"
	"\n"
	"def PROCStoreH(a%(1))\n"
	"[\n"
	"LDR R0, [R0, 4]\n"
	"ADR R1, value\n"
	"LDRH R1, [R1], 0\n"
	"STRH R1,[R0],0\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUW &ffff\n"
	"]\n"
	"\n"
	"def PROCStoreD(a%(1))\n"
	"[\n"
	"LDR R0, [R0, 4]\n"
	"MOV R2, 1\n"
	"MOV R3, 2\n"
	"STRD R2, [R0], 0\n"
	"MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNPreSignE%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRSB R0,[R1, 0]\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUB -33\n"
	"]\n"
	"\n"
	"def FNPreSignEH%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRSH R0,[R1, 0]\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUW -1000\n"
	"]\n"
	"\n"
	"def FNPreUnsignH%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRH R0,[R1, 0]\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUW -1\n"
	"]\n"
	"\n"
	"def FNPreDword%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRD R0,[R1, 0]\n"
	"ADD R0, R0, R1\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUD 1\n"
	"EQUD 2\n"
	"]\n"
	"\n"
	"def PROCPreStoreH(a%(1))\n"
	"[\n"
	"LDR R0, [R0, 4]\n"
	"ADR R1, value\n"
	"LDRH R1, [R1, 0]\n"
	"STRH R1,[R0, 0]\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUW &ffff\n"
	"]\n"
	"\n"
	"def PROCPreStoreD(a%(1))\n"
	"[\n"
	"LDR R0, [R0, 4]\n"
	"MOV R2, 1\n"
	"MOV R3, 2\n"
	"STRD R2, [R0, 0]\n"
	"MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSignENoOffset%\n"
	"[\n"
	"ADR R1, value\n"
	"LDRSB R0,[R1]\n"
	"MOV PC, R14\n"
	"value:\n"
	"EQUB -33\n"
	"]\n",
	"-33\n-1000\n65535\n3\nFFFFFFFF\n1\n2\n-33\n-1000\n65535\n3\nFFFFFFFF\n1\n2\n-33\n",
	},
	{"assembler_simd",
	"print ~FNQADD16%(&10004fff, &10004fff)\n"
	"print ~FNQADD8%(&10104f4f, &10104f4f)\n"
	"print ~FNQADDSUBX%(&4fff8000, &4fff4fff)\n"
	"print ~FNQSUB16%(&80001000, &10001000)\n"
	"print ~FNQSUB8%(&80801010, &10101010)\n"
	"print ~FNQSUBADDX%(&80004fff, &4fff4fff)\n"
	"print ~FNSADD16%(&7fff1010,&00011010)\n"
	"print ~FNSADD8%(&7f7f1010,&01011010)\n"
	"print ~FNSADDSUBX%(&7fff1010,&10100001)\n"
	"print ~FNSSUB16%(&80001010,&00011010)\n"
	"print ~FNSSUB8%(&80801010,&01011010)\n"
	"print ~FNSSUBADDX%(&80001010,&10100001)\n"
	"print ~FNSHADD16%(&ffff1111,&ffff1111)\n"
	"print ~FNSHADD8%(&ffff2121,&ffff2121)\n"
	"print ~FNSHADDSUBX%(&11114444,&22221111)\n"
	"print ~FNSHSUB16%(&66664444,&22222222)\n"
	"print ~FNSHSUB8%(&44444444,&22222222)\n"
	"print ~FNSHSUBADDX%(&11114444,&22221111)\n"
	"print ~FNUADD16%(&7fff1010,&00011010)\n"
	"print ~FNUADD8%(&7f7f1010,&01011010)\n"
	"print ~FNUADDSUBX%(&7fff1010,&10100001)\n"
	"print ~FNUSUB16%(&80001010,&00011010)\n"
	"print ~FNUSUB8%(&80801010,&01011010)\n"
	"print ~FNUSUBADDX%(&80001010,&10100001)\n"
	"print ~FNUHADD16%(&ffff1111,&00031111)\n"
	"print ~FNUHADD8%(&ffff1111,&03031111)\n"
	"print ~FNUHADDSUBX%(&ffff4444,&22220003)\n"
	"print ~FNUHSUB16%(&0000ffff,&22221111)\n"
	"print ~FNUHSUB8%(&0000ffff,&22221111)\n"
	"print ~FNUHSUBADDX%(&0000ffff,&11112222)\n"
	"print ~FNUQADD16%(&1000efff, &10004fff)\n"
	"print ~FNUQADD8%(&1010efef, &10104f4f)\n"
	"print ~FNUQADDSUBX%(&efff8000, &4fff4fff)\n"
	"print ~FNUQSUB16%(&80001000, &10001000)\n"
	"print ~FNUQSUB8%(&80801010, &10101010)\n"
	"print ~FNUQSUBADDX%(&8000efff, &4fff4fff)\n"
	"\n"
	"def FNQADD16%(a%, b%)\n"
	"[\n"
	"	QADD16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNQADD8%(a%, b%)\n"
	"[\n"
	"	QADD8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNQADDSUBX%(a%, b%)\n"
	"[\n"
	"	QADDSUBX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNQSUB16%(a%, b%)\n"
	"[\n"
	"	QSUB16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNQSUB8%(a%, b%)\n"
	"[\n"
	"	QSUB8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNQSUBADDX%(a%, b%)\n"
	"[\n"
	"	QSUBADDX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSADD16%(a%, b%)\n"
	"[\n"
	"	SADD16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSADD8%(a%, b%)\n"
	"[\n"
	"	SADD8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSADDSUBX%(a%, b%)\n"
	"[\n"
	"	SADDSUBX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSSUB16%(a%, b%)\n"
	"[\n"
	"	SSUB16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSSUB8%(a%, b%)\n"
	"[\n"
	"	SSUB8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSSUBADDX%(a%, b%)\n"
	"[\n"
	"	SSUBADDX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSHADD16%(a%, b%)\n"
	"[\n"
	"	SHADD16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSHADD8%(a%, b%)\n"
	"[\n"
	"	SHADD8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSHADDSUBX%(a%, b%)\n"
	"[\n"
	"	SHADDSUBX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSHSUB16%(a%, b%)\n"
	"[\n"
	"	SHSUB16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSHSUB8%(a%, b%)\n"
	"[\n"
	"	SHSUB8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSHSUBADDX%(a%, b%)\n"
	"[\n"
	"	SHSUBADDX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUADD16%(a%, b%)\n"
	"[\n"
	"	UADD16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUADD8%(a%, b%)\n"
	"[\n"
	"	UADD8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUADDSUBX%(a%, b%)\n"
	"[\n"
	"	UADDSUBX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUSUB16%(a%, b%)\n"
	"[\n"
	"	USUB16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUSUB8%(a%, b%)\n"
	"[\n"
	"	USUB8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUSUBADDX%(a%, b%)\n"
	"[\n"
	"	USUBADDX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUHADD16%(a%, b%)\n"
	"[\n"
	"	UHADD16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUHADD8%(a%, b%)\n"
	"[\n"
	"	UHADD8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUHADDSUBX%(a%, b%)\n"
	"[\n"
	"	UHADDSUBX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUHSUB16%(a%, b%)\n"
	"[\n"
	"	UHSUB16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUHSUB8%(a%, b%)\n"
	"[\n"
	"	UHSUB8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUHSUBADDX%(a%, b%)\n"
	"[\n"
	"	UHSUBADDX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUQADD16%(a%, b%)\n"
	"[\n"
	"	UQADD16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUQADD8%(a%, b%)\n"
	"[\n"
	"	UQADD8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUQADDSUBX%(a%, b%)\n"
	"[\n"
	"	UQADDSUBX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUQSUB16%(a%, b%)\n"
	"[\n"
	"	UQSUB16 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUQSUB8%(a%, b%)\n"
	"[\n"
	"	UQSUB8 R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNUQSUBADDX%(a%, b%)\n"
	"[\n"
	"	UQSUBADDX R0, R0, R1\n"
	"	MOV PC, R14\n"
	"]\n",
	"20007FFF\n20207F7F\n7FFF8000\n80000000\n80800000\n80007FFF\n"
	"80002020\n80802020\n80000000\n7FFF0000\n7F7F0000\n7FFF2020\n"
	"FFFF1111\nFFFF2121\n11111111\n22221111\n11111111\n3333\n"
	"80002020\n80802020\n80000000\n7FFF0000\n7F7F0000\n7FFF2020\n"
	"80011111\n81811111\n80011111\nEEEF7777\nEFEF7777\nEEEF8888\n"
	"2000FFFF\n2020FFFF\nFFFF3001\n70000000\n70700000\n3001FFFF\n"
	},
	{"msr_mrs",
	"print ~FNMSR%\n"
	"print ~FNMSR2%\n"
	"def FNMSR%\n"
	"[\n"
	"MSR CPSR_cxsf, &f << 28\n"
	"MRS R0, CPSR\n"
	"MOV PC, R14\n"
	"]\n"
	"def FNMSR2%\n"
	"[\n"
	"MOV R1, &f << 28\n"
	"MSR CPSR_cxsf, R1\n"
	"MRS R0, CPSR\n"
	"MOV PC, R14\n"
	"]\n",
	"F0000000\nF0000000\n"
	},
	{"signx",
	"a& = TRUE\n"
	"print FNSignX%(a&)\n"
	"print ~FNSignX16%(&000100ff)\n"
	"print ~FNSignX16%(&00ff0001)\n"
	"print FNSignXH%(&8000)\n"
	"print FNSignXROR%(&80ff)\n"
	"print FNSignXHROR%(&ffff0000)\n"
	"print ~FNSignX16ROR%(&00ff0001)\n"
	"\n"
	"\n"
	"def FNSignX%(a&)\n"
	"[\n"
	"	SXTB R0, R0\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSignX16%(a%)\n"
	"[\n"
	"	SXTB16 R0, R0\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSignXH%(a%)\n"
	"[\n"
	"	SXTH R0, R0\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSignXROR%(a%)\n"
	"[\n"
	"	SXTB R0, R0, ROR 8\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSignXHROR%(a%)\n"
	"[\n"
	"	SXTH R0, R0, ROR 16\n"
	"	MOV PC, R14\n"
	"]\n"
	"\n"
	"def FNSignX16ROR%(a%)\n"
	"[\n"
	"	SXTB16 R0, R0, ROR 16\n"
	"	MOV PC, R14\n"
	"]\n",
	"-1\n1FFFF\nFFFF0001\n-32768\n-128\n-1\n1FFFF\n",
	},
};

/* clang-format on */

static int prv_test_riscos_vfp_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	subtilis_backend_t backend;
	int ret = 0;

	backend.caps = SUBTILIS_PTD_CAPS;
	backend.sys_trans = subtilis_ptd_sys_trans;
	backend.sys_check = subtilis_ptd_sys_check;
	backend.backend_data = NULL;
	backend.asm_parse = subtilis_ptd_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	for (i = 0;
	     i < sizeof(riscos_vfp_test_cases) / sizeof(subtilis_test_case_t);
	     i++) {
		test = &riscos_vfp_test_cases[i];
		printf("ptd_%s", test->name);
		pass = parser_test_wrapper(
		    test->source, &backend, prv_test_example,
		    subtilis_arm_keywords_list, SUBTILIS_ARM_KEYWORD_TOKENS,
		    SUBTILIS_ERROR_OK, test->result, test->mem_leaks_ok);
		ret |= pass;
	}

	return ret;
}

int ptd_test(void)
{
	int ret = 0;

	ret |= prv_test_ptd_examples();
	ret |= prv_test_riscos_vfp_examples();

	return ret;
}
