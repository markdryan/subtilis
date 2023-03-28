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

#include <locale.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "arch/rv32/rv_encode.h"
#include "arch/rv32/rv_keywords.h"
#include "arch/rv32/rv32_core.h"
#include "backends/rv_bare/rv_bare.h"

#include "common/error.h"
#include "common/lexer.h"
#include "frontend/basic_keywords.h"
#include "frontend/parser.h"

#define SUBTILIS_RV_GEN_PROGRAM_START 0
#define SUBTILIS_RV_GEN_ARM_CAPS SUBTILIS_RV_CAPS

static const char shstrtab[] = {
	0, '.', 't', 'e', 'x', 't', 0,
	'.','r','i','s','c','v','.','a','t','t','r','i','b','u','t','e','s', 0,
	'.','s','h','s','t','r','t','a','b', 0
};
static uint32_t align_adjust;

static void prv_add_elf_header(FILE *fp, size_t bytes_written,
			       subtilis_error_t *err)
{
	uint8_t elf_header[] = {
		0x7f, 'E', 'L', 'F',
		1, // 32 bit
		1, // little endian
		1, // Version
		0, // Linux
		0, // ABI Version
		0, 0, 0, 0, 0, 0, 0, // Reserved
		2, 0, // ETYPE
		0xf3, 0, // Machine
		1, 0, 0, 0, // Version
		0x74, 0x0, 0x1, 0, // Entry Point
		0x34, 0, 0, 0, // Program header table offset
		0, 0, 0, 0,  // Section header offset
		0, 0, 0, 0, // flags
		52, 0, // Size of this header
		32, 0, // Size of program header entry
		2, 0, // Number of program header entries
		40, 0, // Size of section header entry
		4, 0, // Number of section heaader table entries
		3, 0, // Index of section that contains section names
	};

	uint8_t rv_prog_header[] = {
		3, 0, 0, 0x70, // Type RISCV_ATTRIBUT
		0, 0, 0, 0,   // segment offset
		0, 0, 0, 0,    // virtual address
		0, 0, 0, 0,    // phys address
		0x1f, 0, 0, 0, // size in file
		0, 0, 0, 0,    // size in mem
		4, 0, 0, 0,    // flags
		1, 0, 0, 0,    // alignment
	};

	uint8_t code_prog_header[] = {
		1, 0, 0, 0, // Type LOAD
		0, 0, 0, 0, // segment offset
		0, 0, 1, 0, // virtual address
		0, 0, 1, 0, // phys address
		0, 0, 0, 0, // size in file
		0, 0, 0, 0, // size in mem
		5, 0, 0, 0, // flags
		0, 0x10, 0, 0, // alignment
	};

	uint32_t prog_seg_size = sizeof(elf_header) +
		sizeof(rv_prog_header) + sizeof(code_prog_header) +
		bytes_written;

	uint32_t section_headers = prog_seg_size + 0x1f + sizeof(shstrtab);
	uint32_t rem = section_headers & 3;
	if (rem)
		section_headers = (section_headers - rem) + 4;

	align_adjust = 4 - rem;

	memcpy(&elf_header[32], &section_headers, sizeof(uint32_t));

	memcpy(&rv_prog_header[4], &prog_seg_size, sizeof(uint32_t));
	memcpy(&code_prog_header[16], &prog_seg_size, sizeof(uint32_t));
	memcpy(&code_prog_header[20], &prog_seg_size, sizeof(uint32_t));

	if (fwrite(elf_header, 1, sizeof(elf_header), fp) <
	    sizeof(elf_header)) {
		subtilis_error_set_file_write(err);
		return;
	}

	if (fwrite(rv_prog_header, 1, sizeof(rv_prog_header), fp) <
	    sizeof(rv_prog_header)) {
		subtilis_error_set_file_write(err);
		return;
	}

	if (fwrite(code_prog_header, 1, sizeof(code_prog_header), fp) <
	    sizeof(code_prog_header))
		subtilis_error_set_file_write(err);
}

static void prv_add_elf_tail(FILE *fp, size_t bytes_written,
			     subtilis_error_t *err)
{
	uint8_t padding[] = {0, 0, 0, 0};

	uint8_t rv_section[] = {
		0x41, 0x1e, 0x00, 0x00,
		0x00, 0x72, 0x69, 0x73,
		0x63, 0x76, 0x00, 0x01,
		0x14, 0x00, 0x00, 0x00,
		0x05, 0x72, 0x76, 0x33,
		0x32, 0x69, 0x32, 0x70,
		0x30, 0x5f, 0x6d, 0x32,
		0x70, 0x30, 0x00,
	};

	uint8_t null_section_header[] = {
		0, 0, 0, 0,    // Offset to name in shstrtab
		0, 0, 0, 0,    // PROGBITS
		0, 0, 0, 0,    // alloc, execute
		0, 0, 0, 0, // virtual address
		0, 0, 0, 0, // start of section offset
		0, 0, 0, 0,    // size of section
		0, 0, 0, 0,    // link
		0, 0, 0, 0,    // info
		0, 0, 0, 0,    // alignment
		0, 0, 0, 0,    // entsize
	};

	uint8_t text_section_header[] = {
		1, 0, 0, 0,    // Offset to name in shstrtab
		1, 0, 0, 0,    // PROGBITS
		6, 0, 0, 0,    // alloc, execute
		0x74, 0, 1, 0, // virtual address
		0x74, 0, 0, 0, // start of section offset
		0, 0, 0, 0,    // size of section
		0, 0, 0, 0,    // link
		0, 0, 0, 0,    // info
		4, 0, 0, 0,    // alignment
		0, 0, 0, 0,    // entsize
	};

	uint8_t rv_section_header[] = {
		7, 0, 0, 0,       // Offset to name in shstrtab
		3, 0, 0, 0x70,  // RISCV_ATTRIBUTE
		0, 0, 0, 0,       // flags
		0, 0, 0, 0,       // virtual address
		0, 0, 0, 0,       // start of section offset
		0x1f, 0, 0, 0,    // size of section
		0, 0, 0, 0,       // link
		0, 0, 0, 0,       // info
		1, 0, 0, 0,       // alignment
		0, 0, 0, 0,       // entsize
	};

	uint8_t strtab_section_header[] = {
		25, 0, 0, 0,      // Offset to name in shstrtab
		3, 0, 0, 0,       // STRTAB
		0, 0, 0, 0,       // flags
		0, 0, 0, 0,       // virtual address
		0, 0, 0, 0,       // start of section offset
		0, 0, 0, 0,       // size of section
		0, 0, 0, 0,       // link
		0, 0, 0, 0,       // info
		1, 0, 0, 0,       // alignment
		0, 0, 0, 0,       // entsize
	};

	uint32_t rv_section_header_start = 0x74 + bytes_written;
	uint32_t strtab_section_header_start = rv_section_header_start + 0x1f;
	uint32_t strtab_section_header_size = sizeof(shstrtab);

	memcpy(&text_section_header[20], &bytes_written, sizeof(uint32_t));
	memcpy(&rv_section_header[16], &rv_section_header_start,
	       sizeof(uint32_t));
	memcpy(&strtab_section_header[16], &strtab_section_header_start,
	       sizeof(uint32_t));
	memcpy(&strtab_section_header[20], &strtab_section_header_size,
	       sizeof(uint32_t));

	if (fwrite(rv_section, 1, sizeof(rv_section), fp) <
	    sizeof(rv_section)) {
		subtilis_error_set_file_write(err);
		return;
	}

	if (fwrite(shstrtab, 1, sizeof(shstrtab), fp) <
	    sizeof(shstrtab)) {
		subtilis_error_set_file_write(err);
		return;
	}

	if (align_adjust > 0) {
		if (fwrite(padding, 1, align_adjust,fp) < align_adjust) {
			subtilis_error_set_file_write(err);
			return;
		}
	}

	if (fwrite(null_section_header, 1, sizeof(text_section_header),fp) <
	    sizeof(text_section_header)) {
		subtilis_error_set_file_write(err);
		return;
	}

	if (fwrite(text_section_header, 1, sizeof(text_section_header),fp) <
	    sizeof(text_section_header)) {
		subtilis_error_set_file_write(err);
		return;
	}

	if (fwrite(rv_section_header, 1, sizeof(rv_section_header),fp) <
	    sizeof(rv_section_header)) {
		subtilis_error_set_file_write(err);
		return;
	}

	if (fwrite(strtab_section_header, 1, sizeof(strtab_section_header),fp) <
	    sizeof(strtab_section_header)) {
		subtilis_error_set_file_write(err);
		return;
	}
}

int main(int argc, char *argv[])
{
	subtilis_error_t err;
	subtilis_stream_t s;
	subtilis_settings_t settings;
	subtilis_backend_t backend;
	subtilis_lexer_t *l = NULL;
	subtilis_parser_t *p = NULL;
	subtilis_rv_prog_t *rv_p = NULL;
	subtilis_rv_op_pool_t *pool = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage: subtrv file\n");
		return 1;
	}

	setlocale(LC_ALL, "C");

	subtilis_error_init(&err);
	subtilis_stream_from_file(&s, argv[1], &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto fail;

	l = subtilis_lexer_new(&s, SUBTILIS_CONFIG_LEXER_BUF_SIZE,
			       subtilis_keywords_list, SUBTILIS_KEYWORD_TOKENS,
			       subtilis_rv_keywords_list,
			       SUBTILIS_RV_KEYWORD_TOKENS, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	settings.handle_escapes = false;
	settings.ignore_graphics_errors = true;
	settings.check_mem_leaks = false;

	pool = subtilis_rv_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	backend.caps = SUBTILIS_RV_GEN_ARM_CAPS;
	backend.sys_trans = subtilis_rv_bare_sys_trans;
	backend.sys_check = subtilis_rv_bare_sys_check;
	backend.backend_data = pool;

	p = subtilis_parser_new(l, &backend, &settings, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_ir_prog_dump(p->prog);

	rv_p = subtilis_rv_bare_generate(
		pool, p->prog, riscos_rv_bare_rules, riscos_rv_bare_rules_count,
		p->st->max_allocated, SUBTILIS_RV_PROGRAM_START,  &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	printf("\n\n");
	//	subtilis_arm_prog_dump(arm_p);

	subtilis_rv_encode(rv_p, "a.out", prv_add_elf_header, prv_add_elf_tail,
			   &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_rv_prog_delete(rv_p);
	subtilis_rv_op_pool_delete(pool);
	subtilis_parser_delete(p);
	subtilis_lexer_delete(l, &err);

	return 0;

cleanup:
	subtilis_rv_prog_delete(rv_p);
	subtilis_rv_op_pool_delete(pool);
	subtilis_parser_delete(p);
	if (l)
		subtilis_lexer_delete(l, &err);
	else
		s.close(s.handle, &err);

fail:
	subtilis_error_fprintf(stderr, &err, true);

	return 1;
}
