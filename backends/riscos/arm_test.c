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

#include "../../arch/arm32/arm_disass.h"
#include "../../arch/arm32/arm_encode.h"
#include "../../arch/arm32/arm_vm.h"
#include "../../arch/arm32/fpa_gen.h"
#include "../../frontend/parser_test.h"
#include "../../test_cases/test_cases.h"
#include "arm_test.h"
#include "riscos_arm.h"
#include "riscos_arm2.h"

static int prv_test_example(subtilis_lexer_t *l, subtilis_parser_t *p,
			    const char *expected)
{
	subtilis_error_t err;
	subtilis_buffer_t b;
	size_t code_size;
	int retval = 1;
	subtilis_arm_op_pool_t *pool = NULL;
	subtilis_arm_prog_t *arm_p = NULL;
	subtilis_arm_vm_t *vm = NULL;
	uint32_t *code = NULL;

	subtilis_error_init(&err);
	subtilis_buffer_init(&b, 1024);

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stderr, &err, true);
		return 1;
	}

	//	subtilis_ir_prog_dump(p->prog);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_p = subtilis_riscos_generate(
	    pool, p->prog, riscos_arm2_rules, riscos_arm2_rules_count,
	    p->st->allocated, subtilis_fpa_gen_preamble, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	subtilis_arm_prog_dump(arm_p);

	code = subtilis_arm_encode_buf(arm_p, &code_size, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	for (size_t i = 0; i < code_size; i++) {
	//		printf("0x%x\n",code[i]);
	///	}
	vm = subtilis_arm_vm_new(code, code_size, 16 * 1024, &err);
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

	if (err.type != SUBTILIS_ERROR_OK)
		subtilis_error_fprintf(stdout, &err, true);
	subtilis_arm_vm_delete(vm);
	free(code);
	subtilis_arm_prog_delete(arm_p);
	subtilis_arm_op_pool_delete(pool);
	subtilis_buffer_free(&b);

	return retval;
}

static int prv_test_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	int ret = 0;

	for (i = 0; i < SUBTILIS_TEST_CASE_ID_MAX; i++) {
		test = &test_cases[i];
		printf("arm_%s", test->name);
		pass =
		    parser_test_wrapper(test->source, SUBTILIS_RISCOS_ARM_CAPS,
					prv_test_example, test->result);
		ret |= pass;
	}

	return ret;
}

/* clang-format off */
static const uint32_t prv_expected_code[] = {
	0x01A00001, /* MOVEQ R0, r1 */
	0x11F00001, /* MVNSNE R0, r1 */
	0xC1500001, /* CMPGT R0, R1 */
	0xB0000192, /* MULLT R0, R2, R1 */
	0x25920010, /* LDRCS R0, [R2, #16] */
	0x2F0000DC, /* SWICS &DC */
	0x4AFFFFFE, /* BMI #-8 */
	0xE8370001, /* LDMFA R7!, {R0} */
	0xE9A70001, /* STMFA R7!, {R0} */
	0xE9B001F8, /* LDMED R0!, {R3-R8} */
	0xE82001F8, /* STMED R0!, {R3-R8} */
	0xE8B18000, /* LDMFD R1!, {r15} */
	0xE9214000, /* STMFD R1!, {r14} */
	0xE91D000F, /* LDMEA R13, {r0-r3} */
	0xE88D000F, /* STMEA R13, {r0-r3} */
	0x26820101, /* STRCS R0, [R2], R1, LSL #2 */
	0xE1A00251, /* MOV R0, R1, ASR R2 */
};

/* clang-format on */

static void prv_add_ops(subtilis_arm_section_t *arm_s, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_stran_instr_t *stran;

	/* MOVEQ R0, r1 */
	dest = 0;
	op1 = 2;
	op2 = 1;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_EQ, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* MVNSNE R0, r1 */
	subtilis_arm_add_mvn_reg(arm_s, SUBTILIS_ARM_CCODE_NE, true, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* CMPGT R0, R1 */
	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_GT, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* MULLT R0, R2, R1 */
	subtilis_arm_add_mul(arm_s, SUBTILIS_ARM_CCODE_LT, false, dest, op1,
			     op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDRCS R0, [R2, #16] */
	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_CS, dest, op1, 16, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* SWICS &DC */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_CS, 0xdc, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* BMI label_0 */
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.br.ccode = SUBTILIS_ARM_CCODE_MI;
	instr->operands.br.target.label = 0;

	/* LDMFA R7!, {R0} */
	dest = 7;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1,
			       SUBTILIS_ARM_MTRAN_FA, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMFA R7!, {R0} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1,
			       SUBTILIS_ARM_MTRAN_FA, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDMED R0!, {r3-r8} */
	dest = 0;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1F8,
			       SUBTILIS_ARM_MTRAN_ED, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMED R0!, {r3-r8} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1F8,
			       SUBTILIS_ARM_MTRAN_ED, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDMFD R1!, {r15} */
	dest = 1;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x8000,
			       SUBTILIS_ARM_MTRAN_FD, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMFD R1!, {r14} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x4000,
			       SUBTILIS_ARM_MTRAN_FD, true, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDMEA R13, {r0-r3} */
	dest = 13;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0xf,
			       SUBTILIS_ARM_MTRAN_EA, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMEA R13, {r0-r3} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0xf,
			       SUBTILIS_ARM_MTRAN_EA, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STRCS R0, [R2], R1, LSL #2 */
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_CS;
	stran->dest = 0;
	stran->base = 2;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.reg = 1;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->pre_indexed = false;
	stran->write_back = false;
	stran->subtract = false;

	/* MOV R0, R1, ASR R2 */

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dest = 0;
	op1 = 1;
	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = dest;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.reg = op1;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_ASR;
	datai->op2.op.shift.shift.reg = 2;
	datai->op2.op.shift.shift_reg = true;
}

static int prv_test_encode(void)
{
	subtilis_error_t err;
	size_t i;
	size_t code_size;
	int retval = 1;
	subtilis_arm_op_pool_t *op_pool = NULL;
	subtilis_string_pool_t *string_pool = NULL;
	subtilis_arm_section_t *arm_s = NULL;
	subtilis_arm_prog_t *arm_p = NULL;
	uint32_t *code = NULL;
	subtilis_type_section_t *stype = NULL;

	printf("arm_test_encode");

	subtilis_error_init(&err);
	op_pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	string_pool = subtilis_string_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_p = subtilis_arm_prog_new(1, op_pool, string_pool, false, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	stype = subtilis_type_section_new(SUBTILIS_TYPE_VOID, 0, NULL, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* TODO: Figure out why 16 is being passed here for reg_counter */
	arm_s = subtilis_arm_prog_section_new(arm_p, stype, 16, 0, 0, 0, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_add_ops(arm_s, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	code = subtilis_arm_encode_buf(arm_p, &code_size, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 0; i < code_size; i++)
		if (code[i] != prv_expected_code[i]) {
			fprintf(stderr, "%zu: expected 0x%x got 0x%x\n", i,
				prv_expected_code[i], code[i]);
			goto cleanup;
		}

	retval = 0;

cleanup:
	if ((retval == 1) || (err.type != SUBTILIS_ERROR_OK)) {
		printf(": [FAIL]\n");
		subtilis_error_fprintf(stdout, &err, true);
	} else {
		printf(": [OK]\n");
	}

	free(code);
	subtilis_arm_prog_delete(arm_p);
	subtilis_type_section_delete(stype);
	subtilis_string_pool_delete(string_pool);
	subtilis_arm_op_pool_delete(op_pool);

	return retval;
}

static int prv_test_disass_data(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_data_instr_t *datai;

	subtilis_error_init(&err);

	/* MOVEQ R0, r1 */

	subtilis_arm_disass(&instr, prv_expected_code[0], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_MOV) {
		fprintf(stderr, "[0] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_MOV, instr.type);
		return 1;
	}

	datai = &instr.operands.data;
	if (datai->ccode != SUBTILIS_ARM_CCODE_EQ) {
		fprintf(stderr, "[0] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_EQ, datai->ccode);
		return 1;
	}

	if (datai->status) {
		fprintf(stderr, "[0] status should not be set\n");
		return 1;
	}

	if ((datai->dest != 0) || (datai->op2.type != SUBTILIS_ARM_OP2_REG) ||
	    (datai->op2.op.reg != 1)) {
		fprintf(stderr, "[0] bad register values\n");
		return 1;
	}

	/* MVNSNE R0, r1 */
	subtilis_arm_disass(&instr, prv_expected_code[1], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_MVN) {
		fprintf(stderr, "[1] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_MVN, instr.type);
		return 1;
	}
	datai = &instr.operands.data;

	if (datai->ccode != SUBTILIS_ARM_CCODE_NE) {
		fprintf(stderr, "[1] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_NE, datai->ccode);
		return 1;
	}

	if (!datai->status) {
		fprintf(stderr, "[1] status should be set\n");
		return 1;
	}

	if ((datai->dest != 0) || (datai->op2.type != SUBTILIS_ARM_OP2_REG) ||
	    (datai->op2.op.reg != 1)) {
		fprintf(stderr, "[1] bad register values\n");
		return 1;
	}

	/* CMPGT R0, R1 */
	subtilis_arm_disass(&instr, prv_expected_code[2], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_CMP) {
		fprintf(stderr, "[2] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_CMP, instr.type);
		return 1;
	}
	datai = &instr.operands.data;

	if (datai->ccode != SUBTILIS_ARM_CCODE_GT) {
		fprintf(stderr, "[2] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_GT, datai->ccode);
		return 1;
	}

	if (!datai->status) {
		fprintf(stderr, "[2] status should be set\n");
		return 1;
	}

	if ((datai->dest != 0) || (datai->op2.type != SUBTILIS_ARM_OP2_REG) ||
	    (datai->op2.op.reg != 1)) {
		fprintf(stderr, "[2] bad register values\n");
		return 1;
	}

	return 0;
}

static int prv_test_disass_mul(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_mul_instr_t *mul;

	subtilis_error_init(&err);

	subtilis_arm_disass(&instr, prv_expected_code[3], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	/* MULLT R0, R2, R1 */

	if (instr.type != SUBTILIS_ARM_INSTR_MUL) {
		fprintf(stderr, "[3] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_MUL, instr.type);
		return 1;
	}

	mul = &instr.operands.mul;
	if (mul->ccode != SUBTILIS_ARM_CCODE_LT) {
		fprintf(stderr, "[3] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_LT, mul->ccode);
		return 1;
	}

	if (mul->status) {
		fprintf(stderr, "[3] status should not be set\n");
		return 1;
	}

	if ((mul->dest != 0) || (mul->rs != 2) || (mul->rm != 1)) {
		fprintf(stderr, "[3] bad register values\n");
		return 1;
	}

	return 0;
}

static int prv_test_disass_stran(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_stran_instr_t *stran;

	subtilis_error_init(&err);

	/* LDRCS R0, [R2, #16] */
	subtilis_arm_disass(&instr, prv_expected_code[4], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_LDR) {
		fprintf(stderr, "[4] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_LDR, instr.type);
		return 1;
	}
	stran = &instr.operands.stran;

	if (stran->ccode != SUBTILIS_ARM_CCODE_CS) {
		fprintf(stderr, "[4] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_CS, stran->ccode);
		return 1;
	}

	if (!stran->pre_indexed) {
		fprintf(stderr, "[4] pre indexed expected\n");
		return 1;
	}

	if (stran->write_back) {
		fprintf(stderr, "[4] write back not expected\n");
		return 1;
	}

	if (stran->subtract) {
		fprintf(stderr, "[4] subtract not expected\n");
		return 1;
	}

	if ((stran->dest != 0) || (stran->base != 2)) {
		fprintf(stderr, "[4] bad register values\n");
		return 1;
	}

	if ((stran->offset.type != SUBTILIS_ARM_OP2_I32) ||
	    (stran->offset.op.integer != 16)) {
		fprintf(stderr, "[4] bad  values\n");
		return 1;
	}

	/* STRCS R0, [R2], R1, LSL #2 */
	subtilis_arm_disass(&instr, prv_expected_code[15], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_STR) {
		fprintf(stderr, "[15] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_STR, instr.type);
		return 1;
	}
	stran = &instr.operands.stran;

	if (stran->ccode != SUBTILIS_ARM_CCODE_CS) {
		fprintf(stderr, "[15] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_CS, stran->ccode);
		return 1;
	}

	if (stran->pre_indexed) {
		fprintf(stderr, "[15] pre indexed not expected\n");
		return 1;
	}

	if (stran->subtract) {
		fprintf(stderr, "[15] subtract not expected\n");
		return 1;
	}

	if ((stran->dest != 0) || (stran->base != 2)) {
		fprintf(stderr, "[15] bad register values\n");
		return 1;
	}

	if ((stran->offset.type != SUBTILIS_ARM_OP2_SHIFTED) ||
	    (stran->offset.op.shift.reg != 1) ||
	    (stran->offset.op.shift.shift.integer != 2) ||
	    (stran->offset.op.shift.type != SUBTILIS_ARM_SHIFT_LSL)) {
		fprintf(stderr, "[15] bad  values\n");
		return 1;
	}

	return 0;
}

static int prv_test_disass_swi(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_swi_instr_t *swi;

	subtilis_error_init(&err);

	subtilis_arm_disass(&instr, prv_expected_code[5], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	/* SWICS &DC */

	if (instr.type != SUBTILIS_ARM_INSTR_SWI) {
		fprintf(stderr, "[5] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_SWI, instr.type);
		return 1;
	}

	swi = &instr.operands.swi;
	if (swi->ccode != SUBTILIS_ARM_CCODE_CS) {
		fprintf(stderr, "[5] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_CS, swi->ccode);
		return 1;
	}

	if (swi->code != 0xdc) {
		fprintf(stderr, "[5] code should be 0xdc found 0x%zx\n",
			swi->code);
		return 1;
	}

	return 0;
}

static int prv_test_disass_b(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_br_instr_t *br;

	subtilis_error_init(&err);

	subtilis_arm_disass(&instr, prv_expected_code[6], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	/* BMI #-8 */

	if (instr.type != SUBTILIS_ARM_INSTR_B) {
		fprintf(stderr, "[6] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_B, instr.type);
		return 1;
	}

	br = &instr.operands.br;
	if (br->ccode != SUBTILIS_ARM_CCODE_MI) {
		fprintf(stderr, "[6] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_MI, br->ccode);
		return 1;
	}

	if (br->link) {
		fprintf(stderr, "[6] link should not be set\n");
		return 1;
	}

	if (br->target.offset != -2) {
		fprintf(stderr, "[6] offset of -2 expected got %d\n",
			br->target.offset);
		return 1;
	}

	return 0;
}

static int prv_check_mtran(size_t index, bool write_back,
			   subtilis_arm_instr_type_t type,
			   subtilis_arm_mtran_type_t mtype, size_t reg_num,
			   size_t reg_list)
{
	subtilis_arm_instr_t instr;
	subtilis_arm_mtran_instr_t *mtran;
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_arm_disass(&instr, prv_expected_code[index], &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != type) {
		fprintf(stderr, "[%zu] expected instr 0x%x got 0x%x\n", index,
			type, instr.type);
		return 1;
	}

	mtran = &instr.operands.mtran;
	if (mtran->ccode != SUBTILIS_ARM_CCODE_AL) {
		fprintf(stderr, "[%zu] expected ccode 0x%x got 0x%x\n", index,
			SUBTILIS_ARM_CCODE_AL, mtran->ccode);
		return 1;
	}

	if (mtran->write_back != write_back) {
		fprintf(stderr, "[%zu] write back expected\n", index);
		return 1;
	}

	if (mtran->type != mtype) {
		fprintf(stderr, "[%zu] expected mtran type 0x%x got 0x%x\n",
			index, mtype, mtran->type);
		return 1;
	}

	if (mtran->op0 != reg_num) {
		fprintf(stderr, "[%zu] bad register value\n", index);
		return 1;
	}

	if (mtran->reg_list != reg_list) {
		fprintf(stderr, "[%zu] bad register values\n", index);
		return 1;
	}

	return 0;
}

static int prv_test_disass_mtran(void)
{
	int retval;

	/* LDMFA R7!, {R0} */
	retval = prv_check_mtran(7, true, SUBTILIS_ARM_INSTR_LDM,
				 SUBTILIS_ARM_MTRAN_DA, 7, 1);

	/* STMFA R7!, {R0} */
	retval |= prv_check_mtran(8, true, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_IB, 7, 1);

	/* LDMED R0!, {R3-R8} */
	retval |= prv_check_mtran(9, true, SUBTILIS_ARM_INSTR_LDM,
				  SUBTILIS_ARM_MTRAN_IB, 0, 0x1f8);

	/* STMED R0!, {R3-R8} */
	retval |= prv_check_mtran(10, true, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_DA, 0, 0x1f8);

	/* LDMFD R1!, {r15} */
	retval |= prv_check_mtran(11, true, SUBTILIS_ARM_INSTR_LDM,
				  SUBTILIS_ARM_MTRAN_IA, 1, 0x8000);

	/* STMFD R1!, {r14} */
	retval |= prv_check_mtran(12, true, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_DB, 1, 0x4000);

	/* LDMEA R13, {r0-r3} */
	retval |= prv_check_mtran(13, false, SUBTILIS_ARM_INSTR_LDM,
				  SUBTILIS_ARM_MTRAN_DB, 13, 0xf);

	/* STMEA R13, {r0-r3} */
	retval |= prv_check_mtran(14, false, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_IA, 13, 0xf);

	return retval;
}

static int prv_test_disass(void)
{
	int retval;

	printf("arm_test_disass");

	retval = prv_test_disass_data();
	retval |= prv_test_disass_mul();
	retval |= prv_test_disass_swi();
	retval |= prv_test_disass_b();
	retval |= prv_test_disass_stran();
	retval |= prv_test_disass_mtran();

	if (retval == 1)
		printf(": [FAIL]\n");
	else
		printf(": [OK]\n");

	return retval;
}

int arm_test(void)
{
	int res = 0;

	res = prv_test_examples();
	res |= prv_test_encode();
	res |= prv_test_disass();

	return res;
}
