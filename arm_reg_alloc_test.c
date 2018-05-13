/*
 * Copyright (c) 2018 Mark Ryan
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

#include "arm_reg_alloc.h"

static subtilis_arm_section_t *prv_create_section(subtilis_arm_op_pool_t *pool,
						  subtilis_error_t *err)
{
	subtilis_arm_section_t *s;
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;

	dest.type = SUBTILIS_ARM_REG_FLOATING;
	op1.type = SUBTILIS_ARM_REG_FLOATING;
	op2.type = SUBTILIS_ARM_REG_FLOATING;

	s = subtilis_arm_section_new(pool, 0, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	dest.num = 0;
	op1.num = 1;
	op2.num = 8;
	subtilis_arm_add_mul(s, SUBTILIS_ARM_CCODE_AL, false, dest, op1, op2,
			     err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	dest.num = 3;
	op1.num = 4;
	subtilis_arm_add_cmp(s, SUBTILIS_ARM_INSTR_CMP, SUBTILIS_ARM_CCODE_AL,
			     dest, op1, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	dest.num = 8;
	op2.num = 4;
	subtilis_arm_add_mov_reg(s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	dest.num = 4;
	op2.num = 10;
	subtilis_arm_add_mvn_reg(s, SUBTILIS_ARM_CCODE_AL, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto on_error;

	return s;

on_error:

	subtilis_arm_section_delete(s);
	return NULL;
}

static int prv_test_arm_regs_used_before(void)
{
	subtilis_error_t err;
	subtilis_arm_section_t *s = NULL;
	subtilis_arm_op_pool_t *pool = NULL;
	int retval = 1;
	size_t reg_list;
	size_t op_ptr;
	size_t i;
	size_t expected[] = {
	    0x0, 0x0, 0x100, 0x110,
	};

	subtilis_error_init(&err);

	printf("arm_reg_used_before");

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto on_error;

	s = prv_create_section(pool, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto on_error;

	op_ptr = s->first_op;
	for (i = 0; i < sizeof(expected) / sizeof(size_t); i++) {
		reg_list =
		    subtilis_arm_regs_used_before(s, &pool->ops[op_ptr], &err);
		if (err.type != SUBTILIS_ERROR_OK)
			goto on_error;
		if (reg_list != expected[i]) {
			printf("Expected 0x%zx got 0x%zx\n", expected[i],
			       reg_list);
			goto on_error;
		}
		op_ptr = pool->ops[op_ptr].next;
	}

	retval = 0;

on_error:

	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

static int prv_test_arm_regs_used_after(void)
{
	subtilis_error_t err;
	subtilis_arm_section_t *s = NULL;
	subtilis_arm_op_pool_t *pool = NULL;
	int retval = 1;
	size_t reg_list;
	size_t op_ptr;
	size_t i;
	size_t expected[] = {
	    0x510, 0x410, 0x410, 0x400,
	};

	subtilis_error_init(&err);

	printf("arm_reg_used_after");

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto on_error;

	s = prv_create_section(pool, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto on_error;

	op_ptr = s->first_op;
	for (i = 0; i < sizeof(expected) / sizeof(size_t); i++) {
		reg_list =
		    subtilis_arm_regs_used_after(s, &pool->ops[op_ptr], &err);
		if (err.type != SUBTILIS_ERROR_OK)
			goto on_error;
		if (reg_list != expected[i]) {
			printf("Expected 0x%zx got 0x%zx\n", expected[i],
			       reg_list);
			goto on_error;
		}
		op_ptr = pool->ops[op_ptr].next;
	}

	retval = 0;

on_error:

	subtilis_arm_section_delete(s);
	subtilis_arm_op_pool_delete(pool);

	if (retval == 0)
		printf(": [OK]\n");
	else
		printf(": [FAIL]\n");

	return retval;
}

int arm_reg_alloc_test(void)
{
	int retval;

	retval = prv_test_arm_regs_used_before();
	retval |= prv_test_arm_regs_used_after();

	return retval;
}