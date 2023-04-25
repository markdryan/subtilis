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

#ifndef __SUBTILIS_RV_GEN_H
#define __SUBTILIS_RV_GEN_H

#include "../../common/ir.h"
#include "rv32_core.h"

void subtilis_rv_gen_mov(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_movii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_addii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_subii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_rsubii32(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_mulii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_addi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_subi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_muli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_divi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_divii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_modi32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_storeoi8(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_storeoi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_loadoi8(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_loadoi32(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_label(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_lt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_lte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_neq_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_eq_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_gt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_gte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_lt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_lte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_eq(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_neq(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_gt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_gte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
/*
void subtilis_rv_gen_if_err(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_if_err_rev(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err);
*/
void subtilis_rv_gen_jump(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_jmpc(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_jmpc_rev(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_jmpc_no_label(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err);
/*
void subtilis_rv_gen_cmovi32_gti32(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_cmovi32_lti32(subtilis_ir_section_t *s, size_t start,
				    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_cmovi32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
*/
void subtilis_rv_gen_eori32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_ori32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_mvni32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_andi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_eorii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_orii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_andii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_gtii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_ltii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_eqii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_neqii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_gteii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_lteii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_gti32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_lti32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_gtei32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_ltei32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_eqi32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_neqi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_call(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_calli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
/*
void subtilis_rv_gen_call(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_call_gen(subtilis_ir_section_t *s, size_t start,
			       void *user_data,
			       subtilis_rv_br_link_type_t link_type,
			       bool indirect, subtilis_error_t *err);
void subtilis_rv_gen_calli32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
*/
void subtilis_rv_gen_call_ptr(subtilis_ir_section_t *s, size_t start,
			       void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_calli32_ptr(subtilis_ir_section_t *s, size_t start,
				  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_ret(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_reti32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_retii32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_eqii32(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err);


void subtilis_rv_gen_lsli32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_lslii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_lsri32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_lsrii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_asri32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_asrii32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
/*
void subtilis_rv_gen_pushi32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_popi32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
*/


void subtilis_rv_gen_movr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_movi32r(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_movri32(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_absr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_addr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_addir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_mulir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_neqir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_subr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
void subtilis_rv_gen_subir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err);

void subtilis_rv_gen_lca(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err);
/*
void subtilis_rv_gen_memseti32(subtilis_ir_section_t *s,
				subtilis_rv_section_t *rv_s,
				subtilis_error_t *err);
void subtilis_rv_gen_sete(subtilis_rv_section_t *rv_s,
			   subtilis_ir_section_t *s,
			   subtilis_rv_ccode_type_t ccode, size_t reg,
			   int32_t error_code, subtilis_error_t *err);
void subtilis_rv_gen_sete_load_reg(subtilis_rv_section_t *rv_s,
				    subtilis_ir_section_t *s,
				    subtilis_rv_ccode_type_t ccode, size_t reg,
				    subtilis_error_t *err);
void subtilis_rv_gen_signx8to32_helper(subtilis_rv_section_t *rv_s,
					subtilis_rv_reg_t dest,
					subtilis_rv_reg_t src,
					subtilis_error_t *err);
*/
void subtilis_rv_gen_get_proc_addr(subtilis_ir_section_t *s, size_t start,
				   void *user_data, subtilis_error_t *err);


/*
 * Updates all the stack adjustment statements that occur before REM statements
 * with the correct immediate value once it is known after register allocation
 * has completed.
 */
void subtilis_rv_restore_stack(subtilis_rv_section_t *rv_s,
			       size_t stack_space, subtilis_error_t *err);


#endif
