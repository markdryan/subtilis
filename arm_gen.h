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

#ifndef __SUBTILIS_ARM_GEN_H
#define __SUBTILIS_ARM_GEN_H

#include "ir.h"

void subtilis_arm_gen_movii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_addii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_subii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_rsubii32(subtilis_ir_program_t *p, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_mulii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_addi32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_subi32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_muli32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_storeoi32(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_loadoi32(subtilis_ir_program_t *p, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_label(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_lt_imm(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_lte_imm(subtilis_ir_program_t *p, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_gt_imm(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_gte_imm(subtilis_ir_program_t *p, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_lt(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_lte(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_gt(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_gte(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_jump(subtilis_ir_program_t *p, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_jmpc(subtilis_ir_program_t *p, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_eori32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_ori32(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_andi32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_eorii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_orii32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_andii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_gtii32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_ltii32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_gteii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_lteii32(subtilis_ir_program_t *p, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_gti32(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_lti32(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_gtei32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_ltei32(subtilis_ir_program_t *p, size_t start,
			     void *user_data, subtilis_error_t *err);
#endif
