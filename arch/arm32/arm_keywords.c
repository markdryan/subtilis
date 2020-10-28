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

#include <stdbool.h>

#include "arm_keywords.h"

/* clang-format off */
const subtilis_keyword_t subtilis_arm_keywords_list[] = {
	{"ALIGN",     SUBTILIS_ARM_KEYWORD_ALIGN,    true},
	{"AND",       SUBTILIS_ARM_KEYWORD_AND,      true},
	{"ASR",       SUBTILIS_ARM_KEYWORD_ASR,      true},
	{"DIV",       SUBTILIS_ARM_KEYWORD_DIV,      true},
	{"EOR",       SUBTILIS_ARM_KEYWORD_EOR,      true},
	{"EQUB",      SUBTILIS_ARM_KEYWORD_EQUB,     true},
	{"EQUD",      SUBTILIS_ARM_KEYWORD_EQUD,     true},
	{"EQUDBL",    SUBTILIS_ARM_KEYWORD_EQUDBL,   true},
	{"EQUDBLR",   SUBTILIS_ARM_KEYWORD_EQUDBLR,  true},
	{"EQUW",      SUBTILIS_ARM_KEYWORD_EQUW,     true},
	{"FALSE",     SUBTILIS_ARM_KEYWORD_FALSE,    true},
	{"FN",        SUBTILIS_KEYWORD_FN,           true},
	{"LSL",       SUBTILIS_ARM_KEYWORD_LSL,      true},
	{"LSR",       SUBTILIS_ARM_KEYWORD_LSR,      true},
	{"MOD",       SUBTILIS_ARM_KEYWORD_MOD,      true},
	{"NOT",       SUBTILIS_ARM_KEYWORD_NOT,      true},
	{"OR",        SUBTILIS_ARM_KEYWORD_OR,       true},
	{"PROC",      SUBTILIS_KEYWORD_PROC,         true},
	{"REM",       SUBTILIS_KEYWORD_REM,          true},
	{"ROR",       SUBTILIS_ARM_KEYWORD_ROR,      true},
	{"RRX",       SUBTILIS_ARM_KEYWORD_RRX,      true},
	{"TRUE",      SUBTILIS_ARM_KEYWORD_TRUE,     true},
	{"align",     SUBTILIS_ARM_KEYWORD_ALIGN,    true},
	{"asr",       SUBTILIS_ARM_KEYWORD_ASR,      true},
	{"and",       SUBTILIS_ARM_KEYWORD_AND,      true},
	{"div",       SUBTILIS_ARM_KEYWORD_DIV,      true},
	{"eor",       SUBTILIS_ARM_KEYWORD_EOR,      true},
	{"equb",      SUBTILIS_ARM_KEYWORD_EQUB,     true},
	{"equd",      SUBTILIS_ARM_KEYWORD_EQUD,     true},
	{"equdbl",    SUBTILIS_ARM_KEYWORD_EQUDBL,   true},
	{"equdblr",   SUBTILIS_ARM_KEYWORD_EQUDBLR,  true},
	{"equw",      SUBTILIS_ARM_KEYWORD_EQUW,     true},
	{"false",     SUBTILIS_ARM_KEYWORD_FALSE,    true},
	{"lsl",       SUBTILIS_ARM_KEYWORD_LSL,      true},
	{"lsr",       SUBTILIS_ARM_KEYWORD_LSR,      true},
	{"mod",       SUBTILIS_ARM_KEYWORD_MOD,      true},
	{"not",       SUBTILIS_ARM_KEYWORD_NOT,      true},
	{"or",        SUBTILIS_ARM_KEYWORD_OR,       true},
	{"rem",       SUBTILIS_KEYWORD_REM,          true},
	{"ror",       SUBTILIS_ARM_KEYWORD_ROR,      true},
	{"rrx",       SUBTILIS_ARM_KEYWORD_RRX,      true},
	{"true",      SUBTILIS_ARM_KEYWORD_TRUE,     true},
};

/* clang-format on */
