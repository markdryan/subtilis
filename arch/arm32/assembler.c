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

#include "assembler.h"

/* clang-format off */
subtilis_arm_section_t *subtilis_arm_asm_parse(
	subtilis_lexer_t *l, subtilis_token_t *t, subtilis_arm_op_pool_t *pool,
	subtilis_type_section_t *stype, const subtilis_settings_t *set,
	subtilis_arm_swi_info_t *swi_info, subtilis_error_t *err)
/* clang-format on */
{
	subtilis_arm_section_t *arm_s;

	arm_s = subtilis_arm_section_new(pool, stype, 0, 0, 0, 0, set, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return NULL;

	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_AL, false, 15, 14,
				 err);

	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	return arm_s;

cleanup:

	subtilis_arm_section_delete(arm_s);

	return NULL;
}
