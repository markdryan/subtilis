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
