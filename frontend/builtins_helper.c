/*
 * Copyright (c) 2021 Mark Ryan
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

#include "builtins_helper.h"
#include "parser_exp.h"
#include "reference_type.h"
#include "type_if.h"

void subtilis_builtin_memset_i32(subtilis_parser_t *p, size_t base_reg,
				 size_t size_reg, size_t val_reg,
				 subtilis_error_t *err)
{
	subtilis_ir_arg_t *args;
	char *name = NULL;
	static const char memset[] = "_memseti32";

	name = malloc(sizeof(memset));
	if (!name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(name, memset);

	args = malloc(sizeof(*args) * 3);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = base_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = size_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = val_reg;

	(void)subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MEMSETI32, NULL,
				    args, &subtilis_type_void, 3, true, err);
}

void subtilis_builtin_memset_i8(subtilis_parser_t *p, size_t base_reg,
				size_t size_reg, size_t val_reg,
				subtilis_error_t *err)
{
	subtilis_ir_arg_t *args;
	char *name = NULL;
	static const char memset[] = "_memseti8";

	name = malloc(sizeof(memset));
	if (!name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(name, memset);

	args = malloc(sizeof(*args) * 3);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = base_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = size_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = val_reg;

	(void)subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MEMSETI8, NULL,
				    args, &subtilis_type_void, 3, true, err);
}

void subtilis_builtin_memset_i64(subtilis_parser_t *p, size_t base_reg,
				 size_t size_reg, size_t val_reg_low,
				 size_t val_reg_high, subtilis_error_t *err)
{
	subtilis_ir_arg_t *args;
	char *name = NULL;
	static const char memset[] = "_memseti64";

	name = malloc(sizeof(memset));
	if (!name) {
		subtilis_error_set_oom(err);
		return;
	}
	strcpy(name, memset);

	args = malloc(sizeof(*args) * 4);
	if (!args) {
		free(name);
		subtilis_error_set_oom(err);
		return;
	}

	args[0].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[0].reg = base_reg;
	args[1].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[1].reg = size_reg;
	args[2].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[2].reg = val_reg_low;
	args[3].type = SUBTILIS_IR_REG_TYPE_INTEGER;
	args[3].reg = val_reg_high;

	(void)subtilis_exp_add_call(p, name, SUBTILIS_BUILTINS_MEMSETI64, NULL,
				    args, &subtilis_type_void, 4, true, err);
}

void subtilis_builtin_bzero_reg(subtilis_parser_t *p, size_t base_reg,
				size_t loc, size_t size_reg,
				subtilis_error_t *err)
{
	subtilis_exp_t *val_exp = NULL;

	val_exp = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val_exp = subtilis_type_if_exp_to_var(p, val_exp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	base_reg = subtilis_reference_get_pointer(p, base_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_builtin_memset_i8(p, base_reg, size_reg,
				   val_exp->exp.ir_op.reg, err);

cleanup:

	subtilis_exp_delete(val_exp);
}

void subtilis_builtin_bzero(subtilis_parser_t *p, size_t base_reg, size_t loc,
			    size_t size, subtilis_error_t *err)
{
	subtilis_exp_t *size_exp = NULL;
	subtilis_exp_t *val_exp = NULL;

	size_exp = subtilis_exp_new_int32((int32_t)size, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	size_exp = subtilis_type_if_exp_to_var(p, size_exp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	val_exp = subtilis_exp_new_int32(0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	val_exp = subtilis_type_if_exp_to_var(p, val_exp, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	base_reg = subtilis_reference_get_pointer(p, base_reg, loc, err);
	if (err->type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (size & 3)
		subtilis_builtin_memset_i8(p, base_reg, size_exp->exp.ir_op.reg,
					   val_exp->exp.ir_op.reg, err);
	else
		subtilis_builtin_memset_i32(p, base_reg,
					    size_exp->exp.ir_op.reg,
					    val_exp->exp.ir_op.reg, err);
cleanup:

	subtilis_exp_delete(val_exp);
	subtilis_exp_delete(size_exp);
}
