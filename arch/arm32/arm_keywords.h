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

#ifndef __SUBTILIS_ARM_KEYWORDS_H__
#define __SUBTILIS_ARM_KEYWORDS_H__

#include "../../common/keywords.h"

/* clang-format off */
enum {
	SUBTILIS_ARM_KEYWORD_ALIGN = SUBTILIS_KEYWORD_COMMON_MAX,
	SUBTILIS_ARM_KEYWORD_AND,
	SUBTILIS_ARM_KEYWORD_DIV,
	SUBTILIS_ARM_KEYWORD_EOR,
	SUBTILIS_ARM_KEYWORD_FALSE,
	SUBTILIS_ARM_KEYWORD_MOD,
	SUBTILIS_ARM_KEYWORD_NOT,
	SUBTILIS_ARM_KEYWORD_OR,
	SUBTILIS_ARM_KEYWORD_TRUE,

	// ARM specific

	SUBTILIS_ARM_KEYWORD_ASR,
	SUBTILIS_ARM_KEYWORD_LSL,
	SUBTILIS_ARM_KEYWORD_LSR,
	SUBTILIS_ARM_KEYWORD_ROR,
	SUBTILIS_ARM_KEYWORD_RRX,
	SUBTILIS_ARM_KEYWORD_EQUB,
	SUBTILIS_ARM_KEYWORD_EQUD,
	SUBTILIS_ARM_KEYWORD_EQUDBL,
	SUBTILIS_ARM_KEYWORD_EQUDBLR,
	SUBTILIS_ARM_KEYWORD_EQUW,
	SUBTILIS_ARM_KEYWORD_EQUS,

	SUBTILIS_ARM_KEYWORD_MAX
};

/* clang-format on */

/*
 * PROC and FN are upper case only.
 */

#define SUBTILIS_ARM_KEYWORD_TOKENS ((SUBTILIS_ARM_KEYWORD_MAX * 2) - 2)

/* clang-format off */
extern const
subtilis_keyword_t subtilis_arm_keywords_list[SUBTILIS_ARM_KEYWORD_TOKENS];

/* clang-format on */

#endif
