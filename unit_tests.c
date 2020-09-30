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

#include <locale.h>

#include "arch/arm32/arm_core_test.h"
#include "arch/arm32/arm_reg_alloc_test.h"
#include "arch/arm32/fpa_test.h"
#include "backends/riscos/arm_test.h"
#include "common/bitset_test.h"
#include "frontend/ir_test.h"
#include "frontend/lexer_test.h"
#include "frontend/parser_test.h"
#include "frontend/symbol_table_test.h"

int main(int argc, char *argv[])
{
	int failure = 0;

	setlocale(LC_ALL, "C");

	failure |= lexer_test();
	failure |= symbol_table_test();
	failure |= parser_test();
	failure |= ir_test();
	failure |= arm_core_test();
	failure |= arm_reg_alloc_test();
	failure |= arm_test();
	failure |= fpa_test();
	failure |= bitset_test();

	return failure;
}
