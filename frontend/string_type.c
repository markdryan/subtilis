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

#include "string_type.h"
#include "reference_type.h"
#include "type_if.h"

/*
 * Each string variable occupies  a fixed amount of space either on the stack or
 * in global memory.  This memory is laid out as follows.
 *
 *  ----------------------------------
 * | Size in Bytes     |           0 |
 *  ----------------------------------
 * | Pointer to Data   |           4 |
 *  ----------------------------------
 */

#define SUBTIILIS_STRING_SIZE_OFF SUBTIILIS_REFERENCE_SIZE_OFF
#define SUBTIILIS_STRING_DATA_OFF SUBTIILIS_REFERENCE_DATA_OFF

size_t subtilis_string_type_size(const subtilis_type_t *type)
{
	size_t size;

	/*
	 * TODO: Size appropriately for 64 bit builds.
	 */

	/*
	 * We need, on 32 bit builds,
	 * 4 bytes for the pointer
	 * 4 bytes for the size
	 */

	size = sizeof(int32_t) + sizeof(int32_t);

	return size;
}

void subtilis_string_type_zero_ref(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   size_t mem_reg, size_t loc,
				   subtilis_error_t *err)
{
	subtilis_ir_operand_t op1;
	subtilis_ir_operand_t op2;
	subtilis_exp_t *zero = NULL;

	zero = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	zero = subtilis_type_if_exp_to_var(p, zero, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	op1.reg = mem_reg,
	op2.integer = loc + SUBTIILIS_STRING_SIZE_OFF;
	subtilis_ir_section_add_instr_reg(
	    p->current, SUBTILIS_OP_INSTR_STOREO_I32, zero->exp.ir_op,
	    op1, op2, err);

cleanup:

	subtilis_exp_delete(zero);
}

void subtilis_string_type_init_ref(subtilis_parser_t *p,
				   const subtilis_type_t *type,
				   size_t mem_reg, size_t loc,
				   subtilis_exp_t *e,
				   subtilis_error_t *err)
{
	subtilis_reference_type_init_ref(p, mem_reg, loc, e->exp.ir_op.reg,
					 err);
}

