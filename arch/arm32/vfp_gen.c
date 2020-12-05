/*
 * Copyright (c) 2020 Mark Ryan
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

#include <limits.h>

#include "../../common/error_codes.h"
#include "arm_core.h"
#include "arm_gen.h"
#include "arm_vfp_dist.h"
#include "vfp_alloc.h"
#include "vfp_gen.h"

void subtilis_vfp_gen_movr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_movir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_movri32(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_movrrdi32(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_movi32r(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_callr(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_addr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_addir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_subr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_subir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_rsubir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_mulr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_mulir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_divr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_divir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_rdivir(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_storeor(subtilis_ir_section_t *s, size_t start,
			      void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_loador(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_retr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_retir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_lt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_lte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_neq_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_eq_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_gt_imm(subtilis_ir_section_t *s, size_t start,
				void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_gte_imm(subtilis_ir_section_t *s, size_t start,
				 void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_lt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_lte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_eq(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_neq(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_gt(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_if_gte(subtilis_ir_section_t *s, size_t start,
			     void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_gtir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_ltir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_eqir(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_neqir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_gteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_lteir(subtilis_ir_section_t *s, size_t start,
			    void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_gtr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_ltr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_gter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_lter(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_eqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_neqr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_sin(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_cos(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_tan(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_asn(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_acs(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_atn(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_sqr(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_log(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_ln(subtilis_ir_section_t *s, size_t start,
			 void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_absr(subtilis_ir_section_t *s, size_t start,
			   void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_pow(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_exp(subtilis_ir_section_t *s, size_t start,
			  void *user_data, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_gen_preamble(subtilis_arm_section_t *arm_s,
			       subtilis_error_t *err)
{
}

size_t subtilis_vfp_preserve_regs(subtilis_arm_section_t *arm_s,
				  int save_real_start, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
	return INT_MAX;
}

size_t subtilis_vfp_restore_regs(subtilis_arm_section_t *arm_s,
				 int save_real_start, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
	return INT_MAX;
}

void subtilis_vfp_preserve_update(subtilis_arm_section_t *arm_s,
				  subtilis_arm_call_site_t *call_site,
				  size_t real_regs_saved, size_t real_regs_used,
				  subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_update_offsets(subtilis_arm_section_t *arm_s,
				 subtilis_arm_call_site_t *call_site,
				 size_t bytes_saved, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_store_double(subtilis_arm_section_t *arm_s,
			       subtilis_arm_reg_t dest, subtilis_arm_reg_t base,
			       size_t offset, subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_mov_reg(subtilis_arm_section_t *arm_s,
			  subtilis_arm_reg_t dest, subtilis_arm_reg_t src,
			  subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

bool subtilis_vfp_is_fixed(size_t reg) { return false; }

/* clang-format off */
void subtilis_vfp_insert_stran_spill_imm(subtilis_arm_section_t *s,
					 subtilis_arm_op_t *current,
					 subtilis_arm_instr_type_t itype,
					 subtilis_arm_ccode_type_t ccode,
					 subtilis_arm_reg_t dest,
					 subtilis_arm_reg_t base,
					 subtilis_arm_reg_t spill_reg,
					 int32_t offset, subtilis_error_t *err)
{
	/* clang-format on */

	subtilis_error_set_assertion_failed(err);
}

void subtilis_vfp_insert_stran_imm(subtilis_arm_section_t *s,
				   subtilis_arm_op_t *current,
				   subtilis_arm_instr_type_t itype,
				   subtilis_arm_ccode_type_t ccode,
				   subtilis_arm_reg_t dest,
				   subtilis_arm_reg_t base, int32_t offset,
				   subtilis_error_t *err)
{
	subtilis_error_set_assertion_failed(err);
}

void subtilis_arm_vfp_if_init(subtilis_arm_fp_if_t *fp_if)
{
	fp_if->max_regs = 16; /* TODO: needs constant */

	/* TODO: These are wrong */
	fp_if->max_offset = 1023;
	fp_if->store_type = SUBTILIS_FPA_INSTR_STF;
	fp_if->load_type = SUBTILIS_FPA_INSTR_LDF;

	fp_if->preamble_fn = subtilis_vfp_gen_preamble;
	fp_if->preserve_regs_fn = subtilis_vfp_preserve_regs;
	fp_if->restore_regs_fn = subtilis_vfp_restore_regs;
	fp_if->update_regs_fn = subtilis_vfp_preserve_update;
	fp_if->update_offs_fn = subtilis_vfp_update_offsets;
	fp_if->store_dbl_fn = subtilis_vfp_store_double;
	fp_if->mov_reg_fn = subtilis_vfp_mov_reg;
	fp_if->spill_imm_fn = subtilis_vfp_insert_stran_spill_imm;
	fp_if->stran_imm_fn = subtilis_vfp_insert_stran_imm;
	fp_if->is_fixed_fn = subtilis_vfp_is_fixed;
	fp_if->init_dist_walker_fn = subtilis_init_vfp_dist_walker;
	fp_if->init_used_walker_fn = subtilis_init_vfp_used_walker;
	fp_if->init_real_alloc_fn = subtilis_vfp_alloc_init_walker;
}
