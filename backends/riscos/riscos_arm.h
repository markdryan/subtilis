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

#ifndef __SUBTILIS_RISCOS_ARM_H
#define __SUBTILIS_RISCOS_ARM_H

#include "../../arch/arm32/arm_core.h"
#include "../../common/backend_caps.h"

typedef void (*subtilis_riscos_fp_preamble_t)(subtilis_arm_section_t *arm_s,
					      subtilis_error_t *err);

/* clang-format off */
subtilis_arm_prog_t *
subtilis_riscos_generate(
	subtilis_arm_op_pool_t *pool, subtilis_ir_prog_t *p,
	const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals,
	subtilis_riscos_fp_preamble_t fp_premable,
	subtilis_error_t *err);
/* clang-format on */

#define SUBTILIS_RISCOS_PRINT_BUFFER 0
#define SUBTILIS_RISCOS_PRINT_BUFFER_SIZE 32
#define SUBTILIS_RISCOS_RUNTIME_SIZE SUBTILIS_RISCOS_PRINT_BUFFER_SIZE

void subtilis_riscos_arm_printstr(subtilis_ir_section_t *s, size_t start,
				  void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_printnl(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_modei32(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_plot(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_at(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_pos(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_vpos(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_gcol(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_origin(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_gettime(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);

void subtilis_riscos_arm_cls(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_clg(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_on(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_off(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_wait(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_get(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_get_to(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_inkey(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_os_byte_id(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_vdui(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_vdu(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_point(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_tint(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_end(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_testesc(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_realloc(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_ref(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_getref(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_riscos_tcol(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_riscos_palette(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_i32_to_dec(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_i32_to_hex(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_heap_free_space(subtilis_ir_section_t *s, size_t start,
					 void *user_data,
					 subtilis_error_t *err);
void subtilis_riscos_arm_block_free_space(subtilis_ir_section_t *s,
					  size_t start, void *user_data,
					  subtilis_error_t *err);

#define SUBTILIS_RISCOS_ARM_CAPS                                               \
	(SUBTILIS_BACKEND_HAVE_I32_TO_DEC | SUBTILIS_BACKEND_HAVE_I32_TO_HEX)

#endif
