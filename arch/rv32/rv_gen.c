/*
 * Copyright (c) 2023 Mark Ryan
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

#include <limits.h>

#include "rv_gen.h"

void subtilis_rv_gen_movii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_rv_reg_t dest;
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t op2 = instr->operands[1].integer;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);

	subtilis_rv_section_add_li(rv_s, dest, op2, err);
}

void subtilis_rv_gen_storeoi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	subtilis_ir_inst_t *instr = &s->ops[start]->op.instr;
	int32_t offset = instr->operands[2].integer;
	subtilis_rv_reg_t dest;
	subtilis_rv_reg_t base;

	dest = subtilis_rv_ir_to_rv_reg(instr->operands[0].reg);
	base = subtilis_rv_ir_to_rv_reg(instr->operands[1].reg);

	/*
	 * Now this will have to be modified to support offsets larger
	 * than 12 bits.
	 */

	subtilis_rv_section_add_sw(rv_s, base, dest, offset, err);
}

void subtilis_rv_gen_label(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_rv_section_t *rv_s = user_data;
	size_t label = s->ops[start]->op.label;

	subtilis_rv_section_add_label(rv_s, label, err);
}


