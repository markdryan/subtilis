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

#include "rv_opcodes.h"

const rv_opcode_t rv_opcodes[] = {
	{ 0x13, 0x0, 0x0},           /* SUBTILIS_RV_ADDI */
	{ 0x13, 0x2, 0x0},           /* SUBTILIS_RV_SLTI */
	{ 0x13, 0x3, 0x0},           /* SUBTILIS_RV_SLTIU */
	{ 0x13, 0x7, 0x0},           /* SUBTILIS_RV_ANDI */
	{ 0x13, 0x6, 0x0},           /* SUBTILIS_RV_ORI */
	{ 0x13, 0x4, 0x0},           /* SUBTILIS_RV_XORI */
	{ 0x13, 0x1, 0x0},           /* SUBTILIS_RV_SLLI */
	{ 0x13, 0x5, 0x0},           /* SUBTILIS_RV_SRLI */
	{ 0x13, 0x5, 0x20},          /* SUBTILIS_RV_SRAI */
	{ 0x37, 0x0, 0x0},           /* SUBTILIS_RV_LUI */
	{ 0x17, 0x0, 0x0},           /* SUBTILIS_RV_AUIPC */
	{ 0x33, 0x0, 0x0},           /* SUBTILIS_RV_ADD */
	{ 0x33, 0x2, 0x0},           /* SUBTILIS_RV_SLT */
	{ 0x33, 0x3, 0x0},           /* SUBTILIS_RV_SLTU */
	{ 0x33, 0x7, 0x0},           /* SUBTILIS_RV_AND */
	{ 0x33, 0x6, 0x0},           /* SUBTILIS_RV_OR */
	{ 0x33, 0x4, 0x0},           /* SUBTILIS_RV_XOR */
	{ 0x33, 0x1, 0x0},           /* SUBTILIS_RV_SLL */
	{ 0x33, 0x5, 0x0},           /* SUBTILIS_RV_SRL */
	{ 0x33, 0x0, 0x20},          /* SUBTILIS_RV_SUB */
	{ 0x33, 0x5, 0x20},          /* SUBTILIS_RV_SRA */
	{ 0x6f, 0x0, 0x0},           /* SUBTILIS_RV_JAL */
	{ 0x67, 0x0, 0x0},           /* SUBTILIS_RV_JALR */
	{ 0x63, 0x0, 0x0},           /* SUBTILIS_RV_BEQ */
	{ 0x63, 0x1, 0x0},           /* SUBTILIS_RV_BNE */
	{ 0x63, 0x4, 0x0},           /* SUBTILIS_RV_BLT */
	{ 0x63, 0x6, 0x0},           /* SUBTILIS_RV_BLTU */
	{ 0x63, 0x5, 0x0},           /* SUBTILIS_RV_BGE */
	{ 0x63, 0x7, 0x0},           /* SUBTILIS_RV_BGEU */
	{ 0x3,  0x2, 0x0},           /* SUBTILIS_RV_LW */
	{ 0x3,  0x1, 0x0},           /* SUBTILIS_RV_LH */
	{ 0x3,  0x5, 0x0},           /* SUBTILIS_RV_LHU */
	{ 0x3,  0x0, 0x0},           /* SUBTILIS_RV_LB */
	{ 0x3,  0x4, 0x0},           /* SUBTILIS_RV_LBU */
	{ 0x23, 0x2, 0x0},           /* SUBTILIS_RV_SW */
	{ 0x23, 0x1, 0x0},           /* SUBTILIS_RV_SH */
	{ 0x23, 0x0, 0x0},           /* SUBTILIS_RV_SB */
	{ 0xf,  0x0, 0x0},           /* SUBTILIS_RV_FENCE */
	{ 0x73, 0x0, 0x0},           /* SUBTILIS_RV_ECALL */
	{ 0x73, 0x0, 0x0},           /* SUBTILIS_RV_EBREAK */
	{ 0x33, 0x0, 0x1},           /* SUBTILIS_RV_MUL */
	{ 0x33, 0x1, 0x1},           /* SUBTILIS_RV_MULH */
	{ 0x33, 0x2, 0x1},           /* SUBTILIS_RV_MULHSU */
	{ 0x33, 0x3, 0x1},           /* SUBTILIS_RV_MULHU */
	{ 0x33, 0x4, 0x1},           /* SUBTILIS_RV_DIV */
	{ 0x33, 0x5, 0x1},           /* SUBTILIS_RV_DIVU */
	{ 0x33, 0x6, 0x1},           /* SUBTILIS_RV_REM */
	{ 0x33, 0x7, 0x1},           /* SUBTILIS_RV_REMU */
	{ 0x7,  0x2, 0x0},           /* SUBTILIS_RV_FLW */
	{ 0x27, 0x2, 0x0},           /* SUBTILIS_RV_FSW */
	{ 0x43, 0x0, 0x0, true},     /* SUBTILIS_RV_FMADD_S */
	{ 0x47, 0x0, 0x0, true},     /* SUBTILIS_RV_FMSUB_S */
	{ 0x4b, 0x0, 0x0, true},     /* SUBTILIS_RV_FNMSUB_S */
	{ 0x4f, 0x0, 0x0, true},     /* SUBTILIS_RV_FNMADD_S */
	{ 0x53, 0x0, 0x0, true},     /* SUBTILIS_RV_FADD_S */
	{ 0x53, 0x0, 0x4, true},     /* SUBTILIS_RV_FSUB_S */
	{ 0x53, 0x0, 0x8, true},     /* SUBTILIS_RV_FMUL_S */
	{ 0x53, 0x0, 0xc, true},     /* SUBTILIS_RV_FDIV_S */
	{ 0x53, 0x0, 0x2c, true},    /* SUBTILIS_RV_FSQRT_S */
	{ 0x53, 0x0, 0x10, false},   /* SUBTILIS_RV_FSGNJ_S */
	{ 0x53, 0x1, 0x10, false},   /* SUBTILIS_RV_FSGNJN_S */
	{ 0x53, 0x2, 0x10, false},   /* SUBTILIS_RV_FSGNJX_S */
	{ 0x53, 0x0, 0x14, false},   /* SUBTILIS_RV_FMIN_S */
	{ 0x53, 0x1, 0x14, false},   /* SUBTILIS_RV_FMAX_S */
	{ 0x53, 0x0, 0x60, true},    /* SUBTILIS_RV_FCVT_W_S */
	{ 0x53, 0x0, 0x60, true},    /* SUBTILIS_RV_FCVT_WU_S */
	{ 0x53, 0x0, 0x70, false},   /* SUBTILIS_RV_FMV_X_W */
	{ 0x53, 0x2, 0x50, false},   /* SUBTILIS_RV_FEQ_S */
	{ 0x53, 0x1, 0x50, false},   /* SUBTILIS_RV_FLT_S */
	{ 0x53, 0x0, 0x50, false},   /* SUBTILIS_RV_FLE_S */
	{ 0x53, 0x1, 0x70, false},   /* SUBTILIS_RV_FCLASS_S */
	{ 0x53, 0x0, 0x68, true},    /* SUBTILIS_RV_FCVT_S_W */
	{ 0x53, 0x0, 0x68, true},    /* SUBTILIS_RV_FCVT_S_WU */
	{ 0x53, 0x0, 0x78, false},   /* SUBTILIS_RV_FMV_W_X */

	{ 0x7,  0x3, 0x0},           /* SUBTILIS_RV_FLD */
	{ 0x27, 0x3, 0x0},           /* SUBTILIS_RV_FSD */
	{ 0x43, 0x0, 0x0, true},     /* SUBTILIS_RV_FMADD_D */
	{ 0x47, 0x0, 0x0, true},     /* SUBTILIS_RV_FMSUB_D */
	{ 0x4b, 0x0, 0x0, true},     /* SUBTILIS_RV_FNMSUB_D */
	{ 0x4f, 0x0, 0x0, true},     /* SUBTILIS_RV_FNMADD_D */
	{ 0x53, 0x0, 0x1, true},     /* SUBTILIS_RV_FADD_D */
	{ 0x53, 0x0, 0x5, true},     /* SUBTILIS_RV_FSUB_D */
	{ 0x53, 0x0, 0x9, true},     /* SUBTILIS_RV_FMUL_D */
	{ 0x53, 0x0, 0xd, true},     /* SUBTILIS_RV_FDIV_D */
	{ 0x53, 0x0, 0x2d, true},    /* SUBTILIS_RV_FSQRT_D */
	{ 0x53, 0x0, 0x11, false},   /* SUBTILIS_RV_FSGNJ_D */
	{ 0x53, 0x1, 0x11, false},   /* SUBTILIS_RV_FSGNJN_D */
	{ 0x53, 0x2, 0x11, false},   /* SUBTILIS_RV_FSGNJX_D */
	{ 0x53, 0x0, 0x15, false},   /* SUBTILIS_RV_FMIN_D */
	{ 0x53, 0x1, 0x15, false},   /* SUBTILIS_RV_FMAX_D */
	{ 0x53, 0x0, 0x20, true},    /* SUBTILIS_RV_FCVT_S_D */
	{ 0x53, 0x0, 0x21, true},    /* SUBTILIS_RV_FCVT_D_S */
	{ 0x53, 0x2, 0x51, false},   /* SUBTILIS_RV_FEQ_D */
	{ 0x53, 0x1, 0x51, false},   /* SUBTILIS_RV_FLT_D */
	{ 0x53, 0x0, 0x51, false},   /* SUBTILIS_RV_FLE_D */
	{ 0x53, 0x1, 0x71, false},   /* SUBTILIS_RV_FCLASS_D */
	{ 0x53, 0x1, 0x61, true},    /* SUBTILIS_RV_FCVT_W_D */
	{ 0x53, 0x0, 0x61, true},    /* SUBTILIS_RV_FCVT_WU_D */
	{ 0x53, 0x0, 0x69, true},    /* SUBTILIS_RV_FCVT_D_W */
	{ 0x53, 0x0, 0x69, true},    /* SUBTILIS_RV_FCVT_D_WU */
};

const size_t rv_opcode_len = sizeof(rv_opcodes) / sizeof(rv_opcode_t);
