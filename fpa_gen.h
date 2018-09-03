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

#endif
