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

#include <stdbool.h>

#include "rv_keywords.h"

/* clang-format off */
const subtilis_keyword_t subtilis_rv_keywords_list[] = {
	{"ABS",       SUBTILIS_RV_KEYWORD_ABS,         true},
	{"ACS",       SUBTILIS_RV_KEYWORD_ACS,         true},
	{"ALIGN",     SUBTILIS_RV_KEYWORD_ALIGN,       true},
	{"AND",       SUBTILIS_RV_KEYWORD_AND,         true},
	{"ASC",       SUBTILIS_RV_KEYWORD_ASC,         true},
	{"ASN",       SUBTILIS_RV_KEYWORD_ASN,         true},
	{"ATN",       SUBTILIS_RV_KEYWORD_ATN,         true},
	{"CHR$",      SUBTILIS_RV_KEYWORD_CHR_STR,     true},
	{"COS",       SUBTILIS_RV_KEYWORD_COS,         true},
	{"DEF",       SUBTILIS_RV_KEYWORD_DEF,         true},
	{"DIV",       SUBTILIS_RV_KEYWORD_DIV,         true},
	{"EOR",       SUBTILIS_RV_KEYWORD_EOR,         true},
	{"EQUB",      SUBTILIS_RV_KEYWORD_EQUB,        true},
	{"EQUD",      SUBTILIS_RV_KEYWORD_EQUD,        true},
	{"EQUDBL",    SUBTILIS_RV_KEYWORD_EQUDBL,      true},
	{"EQUDBLR",   SUBTILIS_RV_KEYWORD_EQUDBLR,     true},
	{"EQUF",      SUBTILIS_RV_KEYWORD_EQUF,        true},
	{"EQUS",      SUBTILIS_RV_KEYWORD_EQUS,        true},
	{"EQUW",      SUBTILIS_RV_KEYWORD_EQUW,        true},
	{"EXP",       SUBTILIS_RV_KEYWORD_EXP,         true},
	{"FALSE",     SUBTILIS_RV_KEYWORD_FALSE,       true},
	{"FOR",       SUBTILIS_RV_KEYWORD_FOR,         true},
	{"FN",        SUBTILIS_KEYWORD_FN,              true},
	{"INT",       SUBTILIS_RV_KEYWORD_INT,         true},
	{"LEFT$",     SUBTILIS_RV_KEYWORD_LEFT_STR,    true},
	{"LEN",       SUBTILIS_RV_KEYWORD_LEN,         true},
	{"LN",        SUBTILIS_RV_KEYWORD_LN,          true},
	{"LOG",       SUBTILIS_RV_KEYWORD_LOG,         true},
	{"MID$",      SUBTILIS_RV_KEYWORD_MID_STR,     true},
	{"MOD",       SUBTILIS_RV_KEYWORD_MOD,         true},
	{"NEXT",      SUBTILIS_RV_KEYWORD_NEXT,        true},
	{"NOT",       SUBTILIS_RV_KEYWORD_NOT,         true},
	{"OR",        SUBTILIS_RV_KEYWORD_OR,          true},
	{"PI",        SUBTILIS_RV_KEYWORD_PI,          true},
	{"PROC",      SUBTILIS_KEYWORD_PROC,            true},
	{"RAD",       SUBTILIS_RV_KEYWORD_RAD,         true},
	{"REC",       SUBTILIS_KEYWORD_REC,             true},
	{"REM",       SUBTILIS_KEYWORD_REM,             true},
	{"RIGHT$",    SUBTILIS_RV_KEYWORD_RIGHT_STR,   true},
	{"SGN",       SUBTILIS_RV_KEYWORD_SGN,         true},
	{"SIN",       SUBTILIS_RV_KEYWORD_SIN,         true},
	{"SQR",       SUBTILIS_RV_KEYWORD_SQR,         true},
	{"STR$",      SUBTILIS_RV_KEYWORD_STR_STR,     true},
	{"STRING$",   SUBTILIS_RV_KEYWORD_STRING_STR,  true},
	{"TAN",       SUBTILIS_RV_KEYWORD_TAN,         true},
	{"TO",        SUBTILIS_RV_KEYWORD_TO,          true},
	{"TRUE",      SUBTILIS_RV_KEYWORD_TRUE,        true},
	{"abs",       SUBTILIS_RV_KEYWORD_ABS,         true},
	{"acs",       SUBTILIS_RV_KEYWORD_ACS,         true},
	{"align",     SUBTILIS_RV_KEYWORD_ALIGN,       true},
	{"and",       SUBTILIS_RV_KEYWORD_AND,         true},
	{"asc",       SUBTILIS_RV_KEYWORD_ASC,         true},
	{"asn",       SUBTILIS_RV_KEYWORD_ASN,         true},
	{"atn",       SUBTILIS_RV_KEYWORD_ATN,         true},
	{"chr$",      SUBTILIS_RV_KEYWORD_CHR_STR,     true},
	{"cos",       SUBTILIS_RV_KEYWORD_COS,         true},
	{"def",       SUBTILIS_RV_KEYWORD_DEF,         true},
	{"div",       SUBTILIS_RV_KEYWORD_DIV,         true},
	{"eor",       SUBTILIS_RV_KEYWORD_EOR,         true},
	{"equb",      SUBTILIS_RV_KEYWORD_EQUB,        true},
	{"equd",      SUBTILIS_RV_KEYWORD_EQUD,        true},
	{"equdbl",    SUBTILIS_RV_KEYWORD_EQUDBL,      true},
	{"equdblr",   SUBTILIS_RV_KEYWORD_EQUDBLR,     true},
	{"equf",      SUBTILIS_RV_KEYWORD_EQUF,        true},
	{"equs",      SUBTILIS_RV_KEYWORD_EQUS,        true},
	{"equw",      SUBTILIS_RV_KEYWORD_EQUW,        true},
	{"exp",       SUBTILIS_RV_KEYWORD_EXP,         true},
	{"false",     SUBTILIS_RV_KEYWORD_FALSE,       true},
	{"for",       SUBTILIS_RV_KEYWORD_FOR,         true},
	{"int",       SUBTILIS_RV_KEYWORD_INT,         true},
	{"left$",     SUBTILIS_RV_KEYWORD_LEFT_STR,    true},
	{"len",       SUBTILIS_RV_KEYWORD_LEN,         true},
	{"ln",        SUBTILIS_RV_KEYWORD_LN,          true},
	{"log",       SUBTILIS_RV_KEYWORD_LOG,         true},
	{"mid$",      SUBTILIS_RV_KEYWORD_MID_STR,     true},
	{"mod",       SUBTILIS_RV_KEYWORD_MOD,         true},
	{"next",      SUBTILIS_RV_KEYWORD_NEXT,        true},
	{"not",       SUBTILIS_RV_KEYWORD_NOT,         true},
	{"or",        SUBTILIS_RV_KEYWORD_OR,          true},
	{"pi",        SUBTILIS_RV_KEYWORD_PI,          true},
	{"rad",       SUBTILIS_RV_KEYWORD_RAD,         true},
	{"rem",       SUBTILIS_KEYWORD_REM,             true},
	{"right$",    SUBTILIS_RV_KEYWORD_RIGHT_STR,   true},
	{"sgn",       SUBTILIS_RV_KEYWORD_SGN,         true},
	{"sin",       SUBTILIS_RV_KEYWORD_SIN,         true},
	{"sqr",       SUBTILIS_RV_KEYWORD_SQR,         true},
	{"str$",      SUBTILIS_RV_KEYWORD_STR_STR,     true},
	{"string$",   SUBTILIS_RV_KEYWORD_STRING_STR,  true},
	{"tan",       SUBTILIS_RV_KEYWORD_TAN,         true},
	{"to",        SUBTILIS_RV_KEYWORD_TO,          true},
	{"true",      SUBTILIS_RV_KEYWORD_TRUE,        true},
};

/* clang-format on */
