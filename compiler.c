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
#include <stdio.h>

#include "arch/arm32/arm_encode.h"
#include "arch/arm32/arm_keywords.h"
#include "arch/arm32/fpa_gen.h"
#include "backends/riscos/riscos_arm2.h"
#include "backends/riscos_common/riscos_arm.h"
#include "common/error.h"
#include "common/lexer.h"
#include "frontend/basic_keywords.h"
#include "frontend/parser.h"

static void prv_set_prog_size(uint8_t *code, size_t bytes_written,
			      subtilis_error_t *err)
{
	if (bytes_written < 8) {
		subtilis_error_set_assertion_failed(err);
		return;
	}
	((uint32_t *)code)[1] =
	    SUBTILIS_RISCOS_ARM2_PROGRAM_START + (int32_t)bytes_written;
}

int main(int argc, char *argv[])
{
	subtilis_error_t err;
	subtilis_stream_t s;
	subtilis_settings_t settings;
	subtilis_backend_t backend;
	subtilis_lexer_t *l = NULL;
	subtilis_parser_t *p = NULL;
	subtilis_arm_prog_t *arm_p = NULL;
	subtilis_arm_op_pool_t *pool = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage: basicc file\n");
		return 1;
	}

	setlocale(LC_ALL, "C");

	subtilis_error_init(&err);
	subtilis_stream_from_file(&s, argv[1], &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	l = subtilis_lexer_new(&s, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
			       subtilis_keywords_list, SUBTILIS_KEYWORD_TOKENS,
			       subtilis_arm_keywords_list,
			       SUBTILIS_ARM_KEYWORD_TOKENS, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	settings.handle_escapes = true;
	settings.ignore_graphics_errors = true;
	settings.check_mem_leaks = false;

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	backend.caps = SUBTILIS_RISCOS_ARM_CAPS;
	backend.sys_trans = subtilis_riscos_arm2_sys_trans;
	backend.sys_check = subtilis_riscos_arm2_sys_check;
	backend.backend_data = pool;
	backend.asm_parse = subtilis_riscos_arm2_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	p = subtilis_parser_new(l, &backend, &settings, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	subtilis_ir_prog_dump(p->prog);

	arm_p = subtilis_riscos_generate(
	    pool, p->prog, riscos_arm2_rules, riscos_arm2_rules_count,
	    p->st->max_allocated, subtilis_fpa_gen_preamble,
	    SUBTILIS_RISCOS_ARM2_PROGRAM_START, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	printf("\n\n");
	//	subtilis_arm_prog_dump(arm_p);

	subtilis_arm_encode(arm_p, "RunImage", prv_set_prog_size, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_prog_delete(arm_p);
	subtilis_arm_op_pool_delete(pool);
	subtilis_parser_delete(p);
	subtilis_lexer_delete(l, &err);

	return 0;

cleanup:
	subtilis_arm_prog_delete(arm_p);
	subtilis_arm_op_pool_delete(pool);
	subtilis_parser_delete(p);
	if (l)
		subtilis_lexer_delete(l, &err);
	else
		s.close(s.handle, &err);

fail:
	subtilis_error_fprintf(stderr, &err, true);

	return 1;
}
