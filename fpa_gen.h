/*
 * Copyright (c) 2018 Mark Ryan
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

#ifndef __SUBTILIS_FPA_GEN_H
#define __SUBTILIS_FPA_GEN_H

#include "ir.h"

void subtilis_fpa_gen_movr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_movir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_movri32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_movrrdi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_movi32r(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_callr(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_addr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_addir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_subr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_subir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_rsubir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_mulr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_mulir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_divr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_divir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_rdivir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_storeor(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_loador(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_retr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_retir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);

void subtilis_fpa_gen_if_lt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_lte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_neq_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_eq_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_gt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_gte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_lt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_lte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_eq(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_neq(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_gt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_if_gte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_gtir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_ltir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_eqir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_neqir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_gteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_lteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_gtr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_ltr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_gter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_lter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_eqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_neqr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_sin(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_cos(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_fpa_gen_sqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);

#endif
