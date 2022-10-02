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
#include "../../arch/arm32/arm_swi.h"
#include "../../common/backend_caps.h"

/* clang-format off */
subtilis_arm_prog_t *
subtilis_riscos_generate(
	subtilis_arm_op_pool_t *pool, subtilis_ir_prog_t *p,
	const subtilis_ir_rule_raw_t *rules_raw,
	size_t rule_count, size_t globals,
	const subtilis_arm_fp_if_t *fp_if,
	int32_t start_address, subtilis_error_t *err);
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
void subtilis_riscos_arm_point_tint(subtilis_ir_section_t *s, size_t start,
				    void *user_data, size_t res_reg,
				    subtilis_error_t *err);
void subtilis_riscos_handle_graphics_error(subtilis_arm_section_t *arm_s,
					   subtilis_ir_section_t *s,
					   subtilis_error_t *err);
void subtilis_riscos_arm_point(subtilis_ir_section_t *s, size_t start,
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
void subtilis_riscos_arm_block_adjust(subtilis_ir_section_t *s, size_t start,
				      void *user_data, subtilis_error_t *err);
void subtilis_riscos_arm_syscall(subtilis_ir_section_t *s, size_t start,
				 void *user_data,
				 const subtilis_arm_swi_t *swi_list,
				 size_t swi_count, subtilis_error_t *err);
void subtilis_riscos_openout(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_openup(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_riscos_openin(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_riscos_close(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_riscos_bget(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_riscos_bput(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_riscos_block_get(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_riscos_block_put(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_riscos_eof(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_error_t *err);
void subtilis_riscos_ext(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_error_t *err);
void subtilis_riscos_get_ptr(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_set_ptr(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_riscos_signx8to32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
bool subtilis_riscos_sys_check(size_t call_id, uint32_t *in_regs,
			       uint32_t *out_regs, bool *handle_errors,
			       const subtilis_arm_swi_t *swi_list,
			       size_t swi_count);
void subtilis_riscos_oscli(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_riscos_osargs(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_riscos_asm_free(void *asm_code);

#endif
