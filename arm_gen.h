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
void subtilis_arm_gen_storeoi32(subtilis_ir_program_t *p, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_loadoi32(subtilis_ir_program_t *p, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_label(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_if_lt(subtilis_ir_program_t *p, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_jump(subtilis_ir_program_t *p, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_arm_gen_jmpc(subtilis_ir_program_t *p, size_t start,
			   void *user_data, subtilis_error_t *err);
#endif
