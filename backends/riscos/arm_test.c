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

#include <stdlib.h>
#include <string.h>

#include "../../arch/arm32/arm_disass.h"
#include "../../arch/arm32/arm_encode.h"
#include "../../arch/arm32/arm_keywords.h"
#include "../../arch/arm32/arm_vm.h"
#include "../../arch/arm32/fpa_gen.h"
#include "../../frontend/parser_test.h"
#include "../../test_cases/bad_test_cases.h"
#include "../../test_cases/test_cases.h"
#include "../riscos_common/riscos_arm.h"
#include "arm_test.h"
#include "riscos_arm2.h"

/* clang-format off */
static const subtilis_test_case_t riscos_arm_test_cases[] = {
	{"sys_write0_str",
	 "sys \"OS_Write0\", \"hello world\"\n"
	 "print \"\"\n",
	 "hello world\n",
	},
	{"sys_write0_int",
	 "sys 2, \"hello world\"\n"
	 "print \"\"\n",
	 "hello world\n",
	},
	{"sys_write0_str",
	 "a$ = \"hello world\"\n"
	 "sys \"OS_Write0\", \"hello world\"\n"
	 "print \"\"\n",
	 "hello world\n",
	},
	{"sys_get_env",
	 "a% := 1\n"
	 "b% = 0\n"
	 "c% = 0\n"
	 "sys \"OS_GetEnv\" to a%, b%, c%\n"
	 "print b% > 0\n"
	 "sys \"OS_GetEnv\" to b%, a%, c%\n"
	 "print a% > 0\n",
	 "-1\n-1\n",
	},
	{"sys_write_c",
	 "sys 256+33\n"
	 "print \"\"\n",
	 "!\n"
	},
	{"sys_os_convert_integer",
	 "dim a%(10)\n"
	 "local free%\n"
	 "sys \"OS_ConvertInteger4\", 1999, a%(), 44 to ,,free%\n"
	 "sys \"OS_Write0\", a%()\n"
	 "print \"\""
	 "print free%\n",
	 "1999\n40\n",
	},
	{
	"sys_failed",
	"dim a%(1)\n"
	"onerror print err enderror\n"
	"local free%\n"
	"sys \"XOS_ConvertInteger4\", 1999, a%(), 2 to ,,free%\n"
	"print \"Shouldn't get here\"\n",
	"484\n",
	},
	{"riscos_int",
	 "PRINT FNInt%\n"
	 "def FNInt%\n"
	 "[\n"
	 "def float = 3.14\n"

	 "MOV R0, INT(float)\n"
	 "MOV PC, R14\n"
	 "]\n",
	 "3\n"
	},
	{"assembler_banner",
	 "PROCbanner\n"
	 "def PROCbanner\n"
	 "[\n"
	 "def screen_width = 40\n"
	 "def header = STRING$(screen_width,\"*\")\n"
	 "def gap = STRING$(13,\" \")\n"
	 "\n"
	 "ADR R0, header_label\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, message\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, header_label\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "MOV PC, R14\n"
	 "\n"
	 "header_label:\n"
	 "EQUS header\n"
	 "message:\n"
	 "EQUS \"*\" + gap + \"Hello World!\" + gap + \"*\"\n"
	 "]\n",
	 "****************************************\n"
	 "*             Hello World!             *\n"
	 "****************************************\n"
	},
	{"assembler_adr",
	"PROCprint\n"
	"def PROCprint\n"
	"[\n"
	"    B start\n"
	"three:\n"
	"    EQUS \"123\"\n"
	"start:\n"
	"    ADR R0, hello_world\n"
	"    SWI \"OS_Write0\"\n"
	"    ADR R0, three\n"
	"    SWI \"OS_Write0\"\n"
	"    MOV PC, R14\n"
	"hello_world:\n"
	"    EQUS \"Hello  World\"\n"
	"]\n",
	"Hello  World123",
	},
	{"assembler_string",
	 "PRINT FNLen%\n"
	 "PROCChrStr\n"
	 "PRINT FNASC%\n"
	 "PROCLeftStr\n"
	 "PROCRightStr\n"
	 "PROCStrStr\n"
	 "PROCStringStr\n"
	 "PROCMidStr\n"
	 "\n"
	 "def FNLen%\n"
	 "[\n"
	 "def one = \"1\"\n"
	 "MOV R0, LEN(one)\n"
	 "MOV PC, R14\n"
	 "]\n"
	 "\n"
	 "def PROCChrStr\n"
	 "[\n"
	 "def str = chr$(33)\n"
	 "ADR R0, string\n"
	 "SWI \"OS_Write0\"\n"
	 "MOV PC, R14\n"
	 "string:\n"
	 "EQUS str\n"
	 "]\n"
	 "\n"
	 "def FNASC%\n"
	 "[\n"
	 "MOV R0, ASC(\"!\")\n"
	 "MOV PC, R14\n"
	 "]\n"
	 "\n"
	 "def PROCLeftStr\n"
	 "[\n"
	 "def message = \"Hello World\"\n"
	 "ADR R0, label1\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label2\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "MOV PC, R14\n"
	 "label1:\n"
	 "EQUS LEFT$(message, 7)\n"
	 "label2:\n"
	 "EQUS LEFT$(message)\n"
	 "]\n"
	 "\n"
	 "def PROCRightStr\n"
	 "[\n"
	 "def message = \"Hello World\"\n"
	 "ADR R0, label1\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label2\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "MOV PC, R14\n"
	 "label1:\n"
	 "EQUS right$(message, 7)\n"
	 "label2:\n"
	 "EQUS right$(message)\n"
	 "]\n"
	 "\n"
	 "def PROCStrStr\n"
	 "[\n"
	 "ADR R0, label1\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "MOV PC, R14\n"
	 "label1:\n"
	 "EQUS STR$(PI)\n"
	 "]\n"
	 "\n"
	 "def PROCStringStr\n"
	 "[\n"
	 "ADR R0, label1\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "MOV PC, R14\n"
	 "label1:\n"
	 "EQUS STRING$(5,\"*\")+\"==\"+STRING$(5,\"*\")\n"
	 "]\n"
	 "\n"
	 "def PROCMidStr\n"
	 "[\n"
	 "ADR R0, label1\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label2\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label3\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label4\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label5\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label6\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label7\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "ADR R0, label8\n"
	 "SWI \"OS_Write0\"\n"
	 "SWI \"OS_NewLine\"\n"
	 "MOV PC, R14\n"
	 "label1:\n"
	 "EQUS MID$(\"Hello World\", 5)\n"
	 "label2:\n"
	 "EQUS MID$(\"Hello World\", 5, 3)\n"
	 "label3:\n"
	 "EQUS MID$(\"Hello World\", 5, 0)\n"
	 "label4:\n"
	 "EQUS MID$(\"Hello World\", 5, -1)\n"
	 "label5:\n"
	 "EQUS MID$(\"Hello World\", 5, 6)\n"
	 "label6:\n"
	 "EQUS MID$(\"Hello World\", -1)\n"
	 "label7:\n"
	 "EQUS MID$(\"Hello World\", 0)\n"
	 "label8:\n"
	 "EQUS MID$(\"Hello World\", 0, -1)\n"
	 "]\n",
	 "1\n!33\nHello W\nH\no World\nd\n3.142857\n*****==*****\n"
	 " World\n Wo\n\n World\n World\n\nHello World\nHello World\n",
	},
	{"assembler_mul",
	 "print FNMul%(10)\n"
	 "print FNMla%(10, 10)\n"
	 "\n"
	 "def FNMul%(a%)\n"
	 "[\n"
	 "MOV R1, 3\n"
	 "MUL R2, R0, R1\n"
	 "MOV R0, R2\n"
	 "MOV PC, R14\n"
	 "]\n"
	 "\n"
	 "def FNMla%(a%, b%)\n"
	 "[\n"
	 "MOV R3, 3\n"
	 "MLA R2, R0, R3, R1\n"
	 "MOV R0, R2\n"
	 "MOV PC, R14\n"
	 "]\n",
	 "30\n40\n",
	},
	{ "assembler_align",
	  "PROCPrint\n"
	  "\n"
	  "def PROCPrint\n"
	  "[\n"
	  "ADR R0, str\n"
	  "SWI \"OS_Write0\"\n"
	  "B end\n"
	  "str:\n"
	  "EQUS \"A\"\n"
	  "ALIGN 4\n"
	  "end:\n"
	  "MOV PC, R14\n"
	  "]\n",
	  "A",
	},
	{ "assembler_for",
	  "PRINT FNLoop%\n"
	  "def FNLoop%\n"
	  "[\n"
	  "MOV R0, 0\n"
	  "MOV R4, 0\n"
	  "ADR R3, nums\n"
	  "start:\n"
	  "LDR R2, [R3, R4, LSL 2]\n"
	  "ADD R0, R0, R2\n"
	  "ADD R4, R4, 1\n"
	  "CMP R4, 10\n"
	  "BLT start\n"
	  "MOV PC, R14\n"
	  "nums:\n"
	  "for i = 1 to 10\n"
	  "EQUD i\n"
	  "next\n"
	  "]\n",
	  "55\n",
	},
	{ "assembler_bl",
	  "PRINT FNFac%(6)\n"
	  "def FNFac%(a%)\n"
	  "[\n"
	  "fac:\n"
	  "CMP R0, 1\n"
	  "MOVLE PC, R14\n"
	  "MOV R1, R0\n"
	  "STMFD R13!, {R1, R14}\n"
	  "SUB R0, R0, 1\n"
	  "BL fac\n"
	  "LDMFD R13!, {R1, R14}\n"
	  "MUL R0, R1, R0\n"
	  "MOV PC, R14\n"
	  "]\n"
	  "\n",
	  "720\n",
	},
	{ "assembler_int_exp",
	  "a% := ((((10 * 10 + 10 DIV 4) MOD 7) << 2) >> 3) - 5\n"
	  "PRINT FNIntEXP% = a%\n"
	  "def FNIntEXP%\n"
	  "[\n"
	  "def intexp = ((((10 * 10 + 10 DIV 4) MOD 7) << 2) >> 3) - 5\n"
	  "ADR R0, label\n"
	  "LDR R0, [R0]\n"
	  "MOV PC, R14\n"
	  "label: EQUD intexp\n"
	  "]\n",
	  "-1\n"
	},
	{ "assembler_logical",
	  "a% := (\"h\" <> \"H\") AND NOT (5 >= 10) EOR (\"hello\" = \"hello\")\n"
	  "PRINT a% = FNLogicalEXP%\n"
	  "def FNLogicalEXP%\n"
	  "[\n"
	  "ADR R0, label\n"
	  "LDR R0, [R0]\n"
	  "MOV PC, R14\n"
	  "label:\n"
	  "EQUD (\"h\" <> \"H\") AND NOT (5 >= 10) EOR (\"hello\" = \"hello\")\n"
	  "]\n",
	  "-1\n"
	},
	{ "assembler_logical_float",
	  "a% := (2.0 > -10.0) AND  (1.0 <= 1.0) AND NOT (5.0 >= 10.0)\n"
	  "PRINT a% = FNLogicalEXP%\n"
	  "def FNLogicalEXP%\n"
	  "[\n"
	  "ADR R0, label\n"
	  "LDR R0, [R0]\n"
	  "MOV PC, R14\n"
	  "label:\n"
	  "EQUD (2.0 > -10.0) AND  (1.0 <= 1.0) AND NOT (5.0 >= 10.0)\n"
	  "]\n",
	  "-1\n"
	},
	{ "assembler_byte_transfer",
	  "a$ := \"hello  \"\n"
	  "PROCTransmute(a$)\n"
	  "PRINT a$\n"
	  "def PROCTransmute(a$)\n"
	  "[\n"
	  "LDR R2, [R0, 4]\n"
	  "ADR R0, label\n"
	  "MOV R1, 0\n"
	  "loop:\n"
	  "LDRB R3, [R0, R1]\n"
	  "STRB R3, [R2], 1\n"
	  "ADD R1, R1, 1\n"
	  "CMP R3, 0\n"
	  "BNE loop\n"
	  "MOV PC, R14\n"
	  "label: EQUB ASC(\"G\"), ASC(\"o\"), ASC(\"o\"), ASC(\"d\")\n"
	  "EQUB ASC(\"b\"), ASC(\"y\"), ASC(\"e\"), 0\n"
	  "]\n",
	  "Goodbye\n",
	},
	{"assembler_equw",
	 "PROCEQUW\n"
	 "def PROCEQUW\n"
	 "[\n"
	 "ADR R1, label\n"
	 "for i = 1 to 4\n"
	 "LDRB R0, [R1], 1\n"
	 "SWI \"OS_WriteC\"\n"
	 "next\n"
	 "MOV PC, R14\n"
	 "label:\n"
	 "EQUW &2021, &2022\n"
	 "]\n",
	 "! \" "
	},
	{"assembler_status",
	 "PROCStatus\n"
	 "def PROCStatus\n"
	 "[\n"
	 "MOV R0, ASC(\"!\")\n"
	 "MOV R1, 10\n"
	 "loop: SWI \"OS_WriteC\"\n"
	 "SUBS R1, R1, 1\n"
	 "BNE loop\n"
	 "MOV PC, R14\n"
	 "]\n",
	 "!!!!!!!!!!",
	},
	{"assembler_mixed_loop",
	 "PROCMixedLoop\n"
	 "def PROCMixedLoop\n"
	 "[\n"
	 "ADR R1, label\n"
	 "loop:\n"
	 "LDR R0, [R1], 4\n"
	 "CMP R0, 0\n"
	 "MOVEQ PC, R14\n"
	 "ADD R0, R0, ASC(\"0\")\n"
	 "SWI \"OS_WriteC\"\n"
	 "B loop\n"
	 "label: for i = 1.4 to 9 EQUD i next\n"
	 "EQUD 0\n"
	 "]\n",
	 "12345678",
	},
	{ "assembler_mtran",
	  "LOCAL DIM a%(4)\n"
	  "a%() = ASC(\"H\"), ASC(\"E\"), ASC(\"L\"), ASC(\"L\"), ASC(\"O\")\n"
	  "PROCPrint(a%())\n"
	  "def PROCPrint(a%(1))\n"
	  "[\n"
	  "LDR R1, [R0, 4]\n"
	  "LDMIA R1, {R2-R5, R6}\n"
	  "for i = R2 TO R6\n"
	  "MOV R0, i\n"
	  "SWI \"OS_WriteC\"\n"
	  "next\n"
	  "MOV PC, R14\n"
	  "]\n",
	  "HELLO",
	},
	{ "assembler_op2_shift",
	  "PRINT FNOP2Shift%(2)\n"
	  "def FNOP2Shift%(a%)\n"
	  "[\n"
	  "  MOV R1, 10\n"
	  "  MOV R1, R1, LSR 2\n"
	  "  MOV R0, R1, LSL R0\n"
	  "  MOV PC, R14\n"
	  "]\n",
	  "8\n",
	},
};

static const subtilis_test_case_t riscos_fpa_test_cases[] = {
	{"assembler_fpa_loop",
	"PROCFPLoop(10)\n"
	"\n"
	"def PROCFPLoop(a)\n"
	"[\n"
	"    MOV R0, 33\n"
	"start:\n"
	"   SWI \"OS_WriteC\"\n"
	"   SUFD F0, F0, 0.5\n"
	"   CMF F0, 0\n"
	"   BGT start\n"
	"   MOV PC, R14\n"
	"]\n",
	"!!!!!!!!!!!!!!!!!!!!",
	},
	{ "assembler_fpa_cos_and_sin",
	 "LOCAL a\n"
	 "PROCCheck(FNSIN(0), 0)\n"
	 "PROCCheck(FNCOS(0), 1)\n"
	 "LET a = 0\n"
	 "LET b = 30\n"
	 "LET c = 60\n"
	 "PROCCheck(FNSIN(a), 0)\n"
	 "PROCCheck(FNCOS(a), 1)\n"
	 "PROCCheck(FNSIN(RAD(30)), 0.5)\n"
	 "PROCCheck(FNCOS(RAD(60)), 0.5)\n"
	 "LET a = RAD(b)\n"
	 "PROCCheck(FNSIN(a), 0.5)\n"
	 "LET a = RAD(c)\n"
	 "PROCCheck(FNCOS(a), 0.5)\n"
	 "PROCCheck(FNCOS60, COS(RAD(60)))\n"
	 "PROCCheck(FNSIN60, SIN(RAD(60)))\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n"
	  "DEF FNSIN(a) [ SINDM F0, F0 MOV PC, R14 ]\n"
	  "DEF FNCOS(a) [ COSD F0, F0 MOV PC, R14 ]\n"
	  "DEF FNCOS60\n"
	  "[\n"
	  "  ADR R0, value\n"
	  "  LDFD F0, [R0]\n"
	  "  MOV PC, R14\n"
	  "value: EQUDBLR COS(RAD(60))\n"
	  "]\n"
	  "DEF FNSIN60\n"
	  "[\n"
	  "  ADR R0, value\n"
	  "  LDFD F0, [R0]\n"
	  "  MOV PC, R14\n"
	  "value: EQUDBLR SIN(RAD(60))\n"
	  "]\n",
	 "-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n"},
	{ "assembler_fpa_trig",
	  "PRINT INT(FNTAN(RAD(45)))\n"
	  "A = RAD(45)\n"
	  "PRINT INT(FNTAN(A))\n"
	  "\n"
	  "PROCCheck(FNASN(0.5),  0.5236)\n"
	  "\n"
	  "A = 0.5\n"
	  "PROCCheck(FNASN(A),  0.5236)\n"
	  "\n"
	  "PROCCheck(FNACS(0.3), 1.2661)\n"
	  "\n"
	  "A = 0.3\n"
	  "PROCCheck(FNACS(A), 1.2661)\n"
	  "\n"
	  "PROCCheck(FNATN(0.8), 0.6747)\n"
	  "\n"
	  "A = 0.8\n"
	  "PROCCheck(FNATN(A), 0.6747)\n"
	  "\n"
	  "PROCCheck(FNTANFixed, TAN(RAD(45)))\n"
	  "PROCCheck(FNASNFixed, ASN(0.5))\n"
	  "PROCCheck(FNACSFixed, ACS(0.3))\n"
	  "PROCCheck(FNATNFixed, ATN(0.8))\n"
	  "PROCCheck(FNPIFixed, PI)\n"
	  "DEF PROCCheck(a, e)\n"
	  "LET a = e - a\n"
	  "IF a < 0.0 THEN LET a = -a ENDIF\n"
	  "PRINT a < 0.001\n"
	  "ENDPROC\n"
	  "DEF FNTAN(a) [ TANDM F0, F0 MOV PC, R14 ]\n"
	  "DEF FNASN(a) [ ASND F0, F0 MOV PC, R14 ]\n"
	  "DEF FNACS(a) [ ACSDM F0, F0 MOV PC, R14 ]\n"
	  "DEF FNATN(a) [ ATND F0, F0 MOV PC, R14 ]\n"
	  "DEF FNTANFixed\n"
	  "[\n"
	  "  ADR R0, value\n"
	  "  LDFD F0, [R0]\n"
	  "  MOV PC, R14\n"
	  "value: EQUDBLR TAN(RAD(45))\n"
	  "]\n"
	  "DEF FNASNFixed\n"
	  "[\n"
	  "  ADR R0, value\n"
	  "  LDFD F0, [R0]\n"
	  "  MOV PC, R14\n"
	  "value: EQUDBLR ASN(0.5)\n"
	  "]\n"
	  "DEF FNACSFixed\n"
	  "[\n"
	  "  ADR R0, value\n"
	  "  LDFD F0, [R0]\n"
	  "  MOV PC, R14\n"
	  "value: EQUDBLR ACS(0.3)\n"
	  "]\n"
	  "DEF FNATNFixed\n"
	  "[\n"
	  "  ADR R0, value\n"
	  "  LDFD F0, [R0]\n"
	  "  MOV PC, R14\n"
	  "value: EQUDBLR ATN(0.8)\n"
	  "]\n"
	  "DEF FNPIFixed\n"
	  "[\n"
	  "  ADR R0, value\n"
	  "  LDFD F0, [R0]\n"
	  "  MOV PC, R14\n"
	  "value: EQUDBLR PI\n"
	  "]\n",
	  "1\n1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n"
	},
	{"assembler_fpa_sqr",
	 "PROCCheck(SQR(2), FNSQR(2))\n"
	 "PROCCheck(SQR(2), FNSQRFixed)\n"
	 "DEF FNSQR(a) [ SQTDM F0, F0 MOV PC, R14 ]\n"
	 "DEF FNSQRFixed\n"
	 "[\n"
	 "  ADR R0, value\n"
	 "  LDFD F0, [R0]\n"
	 "  MOV PC, R14\n"
	 "value: EQUDBLR SQR(2)\n"
	 "]\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n",
	 "-1\n-1\n"},
	{"assembler_fpa_log",
	 "PRINT INT(FNLog(1000))\n"
	 "PROCCheck(FNLn(10), 2.30258509)\n"
	 "PRINT FNLogFixed\n"
	 "PROCCheck(FNLnFixed, 2.30258509)\n"
	 "\n"
	 "DEF FNLog(a) [ LOGDM F0, F0 MOV PC, R14 ]\n"
	 "DEF FNLn(a) [ LGNDM F0, F0 MOV PC, R14 ]\n"
	 "\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n"
	 "DEF FNLogFixed\n"
	 "[\n"
	 "  ADR R0, value\n"
	 "  LDFD F0, [R0]\n"
	 "  MOV PC, R14\n"
	 "value: EQUDBLR LOG(1000)\n"
	 "]\n"
	 "DEF FNLnFixed\n"
	 "[\n"
	 "  def constant = LN(10)\n"
	 "  ADR R0, value\n"
	 "  LDFD F0, [R0]\n"
	 "  MOV PC, R14\n"
	 "value: EQUDBLR constant\n"
	 "]\n",
	 "3\n-1\n3\n-1\n"},
	{"assembler_fpa_exp",
	 "PROCCheck(FNExp(1), 2.7182818284)\n"
	 "PROCCheck(FNExpFixed, 2.7182818284)\n"
	 "DEF FNExp(a) [ EXPDM F0, F0 MOV PC, R14 ]\n"
	 "DEF FNExpFixed\n"
	 "[\n"
	 "  ADR R0, value\n"
	 "  LDFD F0, [R0]\n"
	 "  MOV PC, R14\n"
	 "value: EQUDBLR EXP(1)\n"
	 "]\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n",
	 "-1\n-1\n"},
	{"assembler_fpa_abs",
	 "PROCCheck(FNAbs(-1), 1)\n"
	 "PROCCheck(FNAbsFixed, 1)\n"
	 "DEF FNAbs(a) [ ABSDM F0, F0 MOV PC, R14 ]\n"
	 "DEF FNAbsFixed\n"
	 "[\n"
	 "  ADR R0, value\n"
	 "  LDFD F0, [R0]\n"
	 "  MOV PC, R14\n"
	 "value: EQUDBLR ABS(-1)\n"
	 "]\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n",
	 "-1\n-1\n"},
	{ "assembler_real_exp",
	  "a := PI * 360 / exp(1) + 10 - 5\n"
	  "b := FNRealEXP\n"
	  "PRINT abs(a-b) < 0.001\n"
	  "def FNRealEXP\n"
	  "[\n"
	  "ADR R0, label\n"
	  "LDFD F0, [R0]\n"
	  "MOV PC, R14\n"
	  "label: EQUDBLR PI * 360 / exp(1) + 10 - 5\n"
	  "]\n",
	  "-1\n"
	},
	{ "assembler_fpa_conv",
	  "PRINT FNFPAConv%(10)\n"
	  "def FNFPAConv%(a%)\n"
	  "[\n"
	  "FLTDM F0, R0\n"
	  "MUFDM F0, F0, 2\n"
	  "MVFD F1, 5\n"
	  "DVFD F0, F0, F1\n"
	  "FIXM R0, F0\n"
	  "MOV PC, R14\n"
	  "]\n",
	  "4\n"
	},
	{ "assembler_fpa_array",
	  "local dim a(9)\n"
	  "PROCInitRealArray(a())\n"
	  "for i% := 0 to dim(a(),1)\n"
	  "print a(i%)\n"
	  "next\n"
	  "\n"
	  "def PROCInitRealArray(a(1))\n"
	  "[\n"
	  "def array=R1\n"
	  "LDR array, [R0, 4]\n"
	  "ADR R0, nums\n"
	  "SUB R0, R0, 8\n"
	  "loop:\n"
	  "\n"
	  "LDFD F0, [R0, 8]!\n"
	  "CMF F0, 0.0\n"
	  "MOVEQ PC, R14\n"
	  "STFD F0, [array], 8\n"
	  "B loop\n"
	  "nums:\n"
	  "EQUDBLR 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0.0\n"
	  "]\n",
	  "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"assembler_fpa_cptran",
	 "PRINT FNWFSRFS%(7)\n"
	 "def FNWFSRFS%(a%)\n"
	 "[\n"
	 "WFS R0\n"
	 "RFS R0\n"
	 "MOV PC, R14\n"
	 "]\n",
	 "7\n",
	}
};

static const subtilis_bad_test_case_t riscos_arm_bad_test_cases[] = {
	{"sys_non_const_id",
	 "a$ = \"hello\"\n"
	 "b$ = \"OS_Write0\"\n"
	 "sys b$, a$",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"sys_bad_out_type",
	 "local a%\n"
	 "local b%\n"
	 "local c\n"
	 "sys \"OS_GetEnv\" to a%, b%, c\n",
	 SUBTILIS_ERROR_INTEGER_VARIABLE_EXPECTED,
	},
	{"sys_bad_out_type_2",
	 "local a%\n"
	 "local b%\n"
	 "local c\n"
	 "sys \"OS_GetEnv\" to a%, b%, c\n",
	 SUBTILIS_ERROR_INTEGER_VARIABLE_EXPECTED,
	},
	{"sys_bad_out_type_3",
	 "local a%\n"
	 "local b%\n"
	 "local c$\n"
	 "sys \"OS_GetEnv\" to a%, b%, c$\n",
	 SUBTILIS_ERROR_INTEGER_VARIABLE_EXPECTED,
	},
	{"sys_bad_missing_out_arg",
	 "sys \"OS_GetEnv\" to\n",
	 SUBTILIS_ERROR_UNKNOWN_VARIABLE,
	},
	{"sys_bad_missing_out_arg_2",
	 "sys \"OS_GetEnv\" to ,,\n",
	 SUBTILIS_ERROR_UNKNOWN_VARIABLE,
	},
	{"sys_no_in_args",
	 "sys \"OS_GetEnv\" ,,,\n",
	 SUBTILIS_ERROR_EXP_EXPECTED,
	},
	{"sys_invalid_in_reg",
	 "sys \"OS_Write0\" ,,,\"hello\"\n",
	 SUBTILIS_ERROR_SYS_BAD_ARGS,
	},
	{"sys_invalid_out_reg",
	 "local a%\n"
	 "sys \"OS_GetEnv\" to ,,,a%\n",
	 SUBTILIS_ERROR_SYS_BAD_ARGS,
	},
	{"sys_int_as_array",
	 "local a%\n"
	 "local free%\n"
	 "sys \"OS_ConvertInteger4\", 1999, a%(), 44 to ,,free%\n",
	 SUBTILIS_ERROR_NOT_ARRAY,
	},
	{"assembler_mov_missing_comma",
	 "def PROCBad [ MOV PC ]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_mov_missing_arg",
	 "def PROCBad [ MOV PC, ]\n",
	 SUBTILIS_ERROR_EXP_EXPECTED,
	},
	{"assembler_bad_int_reg",
	 "def PROCBad [ MOV R0, R16]\n",
	 SUBTILIS_ERROR_ASS_BAD_REG,
	},
	{"assembler_bad_fp_reg",
	 "def PROCBad [ MVFD F0, F8]\n",
	 SUBTILIS_ERROR_ASS_BAD_REG,
	},
	{"assembler_bad_def",
	 "def PROCBad [ def x 10]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_label",
	 "def PROCBad [ B unknown ]\n",
	 SUBTILIS_ERROR_ASS_MISSING_LABEL,
	},
	{"assembler_bad_adr",
	 "def PROCBad\n"
	 "[\n"
	 "ADR R0, label\n"
	 "for i = 1 to 512 EQUDBL 0.0 next\n"
	 "label: equd 0\n"
	 "]\n",
	 SUBTILIS_ERROR_ASS_BAD_ADR,
	},
	{"assembler_bad_encode",
	 "def PROCBad [ MOV R0, 1023 MOV PC, R14 ]\n",
	 SUBTILIS_ERROR_ASS_INTEGER_ENCODE,
	},
	{"assembler_bad_align",
	 "def PROCBad [ ALIGN 3 ]\n",
	 SUBTILIS_ERROR_ASS_BAD_ALIGN,
	},
	{"assembler_bad_fpa_imm",
	 "def PROCBad [ MVFD F0, 11 MOV PC, R14 ]\n",
	 SUBTILIS_ERROR_ASS_BAD_REAL_IMM,
	},
	{"assembler_bad_reg_type1",
	 "def PROCBad [ MVFD R0, 11 MOV PC, R14 ]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_reg_type2",
	 "def PROCBad [ MOV F0, 11 MOV PC, R14 ]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_for_mismatch1",
	 "def PROCBad [ for i = 0 to R5 EQUD 0 next ]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_for_mismatch2",
	 "def PROCBad [ for i = F0 to R5 EQUD 0 next ]\n",
	 SUBTILIS_ERROR_BAD_CONVERSION,
	},
	{"assembler_bad_range1",
	 "def PROCBad [ STMFD R0!, {R1-}]\n",
	 SUBTILIS_ERROR_ASS_BAD_REG,
	},
	{"assembler_bad_range2",
	 "def PROCBad [ STMFD R0!, {R7-R1}]\n",
	 SUBTILIS_ERROR_ASS_BAD_RANGE,
	},
	{"assembler_bad_range3",
	 "def PROCBad [ STMFD R0!, {R1,}]\n",
	 SUBTILIS_ERROR_ASS_BAD_REG,
	},
	{"assembler_bad_adr1",
	 "def PROCBad [ ADR 0, label label ]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_adr2",
	 "def PROCBad [ ADR R15, label label ]\n",
	 SUBTILIS_ERROR_ASS_BAD_REG,
	},
	{"assembler_bad_adr3",
	 "def PROCBad [ ADR R1, R0 label ]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_adr4",
	 "def PROCBad [ ADR R1 ]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_adr4",
	 "def PROCBad [ ADR R1, ]\n",
	 SUBTILIS_ERROR_EXP_EXPECTED,
	},
	{"assembler_bad_mtran1",
	 "def PROCBad [ STMIA ]\n",
	 SUBTILIS_ERROR_EXP_EXPECTED,
	},
	{"assembler_bad_mtran2",
	 "def PROCBad [ STMIA R0]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_mtran3",
	 "def PROCBad [ STMIA R0!]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_mtran4",
	 "def PROCBad [ STMIA R0!,]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_mtran5",
	 "def PROCBad [ STMIA R0!,{]\n",
	 SUBTILIS_ERROR_ASS_BAD_REG,
	},
	{"assembler_bad_mtran6",
	 "def PROCBad [ STMIA R0!,{R0]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_mtran7",
	 "def PROCBad [ STMIA R0! R0]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
	{"assembler_bad_mtran8",
	 "def PROCBad [ STMIA R0!, R0 R0]\n",
	 SUBTILIS_ERROR_EXPECTED,
	},
};

/* clang-format on */

static int prv_test_example(subtilis_lexer_t *l, subtilis_parser_t *p,
			    subtilis_error_type_t expected_err,
			    const char *expected, bool mem_leaks_ok)
{
	subtilis_error_t err;
	subtilis_buffer_t b;
	size_t code_size;
	subtilis_arm_fp_if_t fp_if;
	int retval = 1;
	subtilis_arm_op_pool_t *pool = NULL;
	subtilis_arm_prog_t *arm_p = NULL;
	subtilis_arm_vm_t *vm = NULL;
	uint8_t *code = NULL;

	subtilis_error_init(&err);
	subtilis_buffer_init(&b, 1024);

	pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	p->backend.backend_data = pool;

	subtilis_parse(p, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		if (err.type == expected_err)
			retval = 0;
		goto cleanup;
	}

	//	subtilis_ir_prog_dump(p->prog);
	subtilis_arm_fpa_if_init(&fp_if);

	arm_p = subtilis_riscos_generate(
	    pool, p->prog, riscos_arm2_rules, riscos_arm2_rules_count,
	    p->st->max_allocated, &fp_if, SUBTILIS_RISCOS_ARM2_PROGRAM_START,
	    &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	//	subtilis_arm_prog_dump(arm_p);

	code = subtilis_arm_encode_buf(arm_p, &code_size, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		if (err.type == expected_err)
			retval = 0;
		goto cleanup;
	}

	/* Insert heap start */

	if (code_size < 8) {
		subtilis_error_set_assertion_failed(&err);
		goto cleanup;
	}

	((uint32_t *)code)[1] = SUBTILIS_RISCOS_ARM2_PROGRAM_START + code_size;

	//	for (size_t i = 0; i < code_size; i++) {
	//		printf("0x%x\n",code[i]);
	///	}
	vm = subtilis_arm_vm_new(code, code_size, 512 * 1024,
				 SUBTILIS_RISCOS_ARM2_PROGRAM_START, false,
				 &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_arm_vm_run(vm, &b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	subtilis_buffer_zero_terminate(&b, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	if (strcmp(subtilis_buffer_get_string(&b), expected)) {
		printf("%s expected got %s\n", expected,
		       subtilis_buffer_get_string(&b));
		retval = 1;
		goto cleanup;
	}

	retval = 0;

cleanup:

	if (retval != 0) {
		subtilis_error_fprintf(stdout, &err, true);
	} else if (err.type != expected_err) {
		fprintf(stderr, "expected error %u got %u\n", expected_err,
			err.type);
		retval = 1;
	}

	subtilis_arm_vm_delete(vm);
	free(code);
	subtilis_arm_prog_delete(arm_p);
	subtilis_arm_op_pool_delete(pool);
	subtilis_buffer_free(&b);

	return retval;
}

static int prv_test_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	subtilis_backend_t backend;
	int ret = 0;

	backend.caps = SUBTILIS_RISCOS_ARM_CAPS;
	backend.sys_trans = subtilis_riscos_arm2_sys_trans;
	backend.sys_check = subtilis_riscos_arm2_sys_check;
	backend.backend_data = NULL;
	backend.asm_parse = subtilis_riscos_arm2_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	for (i = 0; i < SUBTILIS_TEST_CASE_ID_MAX; i++) {
		test = &test_cases[i];
		printf("arm_%s", test->name);
		pass = parser_test_wrapper(
		    test->source, &backend, prv_test_example,
		    subtilis_arm_keywords_list, SUBTILIS_ARM_KEYWORD_TOKENS,
		    SUBTILIS_ERROR_OK, test->result, test->mem_leaks_ok);
		ret |= pass;
	}

	return ret;
}

static int prv_test_riscos_arm_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	subtilis_backend_t backend;
	int ret = 0;

	backend.caps = SUBTILIS_RISCOS_ARM_CAPS;
	backend.sys_trans = subtilis_riscos_arm2_sys_trans;
	backend.sys_check = subtilis_riscos_arm2_sys_check;
	backend.backend_data = NULL;
	backend.asm_parse = subtilis_riscos_arm2_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	for (i = 0;
	     i < sizeof(riscos_arm_test_cases) / sizeof(subtilis_test_case_t);
	     i++) {
		test = &riscos_arm_test_cases[i];
		printf("arm_%s", test->name);
		pass = parser_test_wrapper(
		    test->source, &backend, prv_test_example,
		    subtilis_arm_keywords_list, SUBTILIS_ARM_KEYWORD_TOKENS,
		    SUBTILIS_ERROR_OK, test->result, test->mem_leaks_ok);
		ret |= pass;
	}

	return ret;
}

static int prv_test_riscos_fpa_examples(void)
{
	size_t i;
	int pass;
	const subtilis_test_case_t *test;
	subtilis_backend_t backend;
	int ret = 0;

	backend.caps = SUBTILIS_RISCOS_ARM_CAPS;
	backend.sys_trans = subtilis_riscos_arm2_sys_trans;
	backend.sys_check = subtilis_riscos_arm2_sys_check;
	backend.backend_data = NULL;
	backend.asm_parse = subtilis_riscos_arm2_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	for (i = 0;
	     i < sizeof(riscos_fpa_test_cases) / sizeof(subtilis_test_case_t);
	     i++) {
		test = &riscos_fpa_test_cases[i];
		printf("arm_%s", test->name);
		pass = parser_test_wrapper(
		    test->source, &backend, prv_test_example,
		    subtilis_arm_keywords_list, SUBTILIS_ARM_KEYWORD_TOKENS,
		    SUBTILIS_ERROR_OK, test->result, test->mem_leaks_ok);
		ret |= pass;
	}

	return ret;
}

static int prv_test_bad_cases(void)
{
	size_t i;
	subtilis_backend_t backend;
	const subtilis_bad_test_case_t *test;
	int retval = 0;

	backend.caps = SUBTILIS_RISCOS_ARM_CAPS;
	backend.sys_trans = subtilis_riscos_arm2_sys_trans;
	backend.sys_check = subtilis_riscos_arm2_sys_check;
	backend.backend_data = NULL;
	backend.asm_parse = subtilis_riscos_arm2_asm_parse;
	backend.asm_free = subtilis_riscos_asm_free;

	for (i = 0; i < sizeof(riscos_arm_bad_test_cases) /
			    sizeof(riscos_arm_bad_test_cases[0]);
	     i++) {
		test = &riscos_arm_bad_test_cases[i];
		printf("arm_bad_%s", test->name);
		retval |= parser_test_wrapper(
		    test->source, &backend, prv_test_example,
		    subtilis_arm_keywords_list, SUBTILIS_ARM_KEYWORD_TOKENS,
		    test->err, "", false);
	}

	return retval;
}

/* clang-format off */
static const uint32_t prv_expected_code[] = {
	0x01A00001, /* MOVEQ R0, r1 */
	0x11F00001, /* MVNSNE R0, r1 */
	0xC1500001, /* CMPGT R0, R1 */
	0xB0000192, /* MULLT R0, R2, R1 */
	0x25920010, /* LDRCS R0, [R2, #16] */
	0x2F0000DC, /* SWICS &DC */
	0x4AFFFFFE, /* BMI #-8 */
	0xE8370001, /* LDMFA R7!, {R0} */
	0xE9A70001, /* STMFA R7!, {R0} */
	0xE9B001F8, /* LDMED R0!, {R3-R8} */
	0xE82001F8, /* STMED R0!, {R3-R8} */
	0xE8B18000, /* LDMFD R1!, {r15} */
	0xE9214000, /* STMFD R1!, {r14} */
	0xE91D000F, /* LDMEA R13, {r0-r3} */
	0xE88D000F, /* STMEA R13, {r0-r3} */
	0x26820101, /* STRCS R0, [R2], R1, LSL #2 */
	0xE1A00251, /* MOV R0, R1, ASR R2 */
};

/* clang-format on */

static void prv_add_ops(subtilis_arm_section_t *arm_s, subtilis_error_t *err)
{
	subtilis_arm_reg_t dest;
	subtilis_arm_reg_t op1;
	subtilis_arm_reg_t op2;
	subtilis_arm_instr_t *instr;
	subtilis_arm_data_instr_t *datai;
	subtilis_arm_stran_instr_t *stran;

	/* MOVEQ R0, r1 */
	dest = 0;
	op1 = 2;
	op2 = 1;
	subtilis_arm_add_mov_reg(arm_s, SUBTILIS_ARM_CCODE_EQ, false, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* MVNSNE R0, r1 */
	subtilis_arm_add_mvn_reg(arm_s, SUBTILIS_ARM_CCODE_NE, true, dest, op2,
				 err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* CMPGT R0, R1 */
	subtilis_arm_add_cmp(arm_s, SUBTILIS_ARM_INSTR_CMP,
			     SUBTILIS_ARM_CCODE_GT, dest, op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* MULLT R0, R2, R1 */
	subtilis_arm_add_mul(arm_s, SUBTILIS_ARM_CCODE_LT, false, dest, op1,
			     op2, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDRCS R0, [R2, #16] */
	subtilis_arm_add_stran_imm(arm_s, SUBTILIS_ARM_INSTR_LDR,
				   SUBTILIS_ARM_CCODE_CS, dest, op1, 16, false,
				   err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* SWICS &DC */
	subtilis_arm_add_swi(arm_s, SUBTILIS_ARM_CCODE_CS, 0xdc, 0, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	subtilis_arm_section_add_label(arm_s, 0, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* BMI label_0 */
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_B, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	instr->operands.br.ccode = SUBTILIS_ARM_CCODE_MI;
	instr->operands.br.target.label = 0;

	/* LDMFA R7!, {R0} */
	dest = 7;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1,
			       SUBTILIS_ARM_MTRAN_FA, true, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMFA R7!, {R0} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1,
			       SUBTILIS_ARM_MTRAN_FA, true, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDMED R0!, {r3-r8} */
	dest = 0;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1F8,
			       SUBTILIS_ARM_MTRAN_ED, true, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMED R0!, {r3-r8} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x1F8,
			       SUBTILIS_ARM_MTRAN_ED, true, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDMFD R1!, {r15} */
	dest = 1;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x8000,
			       SUBTILIS_ARM_MTRAN_FD, true, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMFD R1!, {r14} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0x4000,
			       SUBTILIS_ARM_MTRAN_FD, true, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* LDMEA R13, {r0-r3} */
	dest = 13;
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_LDM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0xf,
			       SUBTILIS_ARM_MTRAN_EA, false, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STMEA R13, {r0-r3} */
	subtilis_arm_add_mtran(arm_s, SUBTILIS_ARM_INSTR_STM,
			       SUBTILIS_ARM_CCODE_AL, dest, 0xf,
			       SUBTILIS_ARM_MTRAN_EA, false, false, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	/* STRCS R0, [R2], R1, LSL #2 */
	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_STR, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;

	stran = &instr->operands.stran;
	stran->ccode = SUBTILIS_ARM_CCODE_CS;
	stran->dest = 0;
	stran->base = 2;
	stran->offset.type = SUBTILIS_ARM_OP2_SHIFTED;
	stran->offset.op.shift.shift.integer = 2;
	stran->offset.op.shift.reg = 1;
	stran->offset.op.shift.type = SUBTILIS_ARM_SHIFT_LSL;
	stran->pre_indexed = false;
	stran->write_back = false;
	stran->subtract = false;

	/* MOV R0, R1, ASR R2 */

	instr =
	    subtilis_arm_section_add_instr(arm_s, SUBTILIS_ARM_INSTR_MOV, err);
	if (err->type != SUBTILIS_ERROR_OK)
		return;
	dest = 0;
	op1 = 1;
	datai = &instr->operands.data;
	datai->status = false;
	datai->ccode = SUBTILIS_ARM_CCODE_AL;
	datai->dest = dest;
	datai->op2.type = SUBTILIS_ARM_OP2_SHIFTED;
	datai->op2.op.shift.reg = op1;
	datai->op2.op.shift.type = SUBTILIS_ARM_SHIFT_ASR;
	datai->op2.op.shift.shift.reg = 2;
	datai->op2.op.shift.shift_reg = true;
}

static int prv_test_encode(void)
{
	subtilis_error_t err;
	size_t i;
	size_t code_size;
	int retval = 1;
	subtilis_arm_op_pool_t *op_pool = NULL;
	subtilis_string_pool_t *string_pool = NULL;
	subtilis_constant_pool_t *const_pool = NULL;
	subtilis_arm_section_t *arm_s = NULL;
	subtilis_arm_prog_t *arm_p = NULL;
	uint32_t *code = NULL;
	subtilis_type_section_t *stype = NULL;

	printf("arm_test_encode");

	subtilis_error_init(&err);
	op_pool = subtilis_arm_op_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	string_pool = subtilis_string_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	const_pool = subtilis_constant_pool_new(&err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	arm_p = subtilis_arm_prog_new(1, op_pool, string_pool, const_pool, NULL,
				      NULL, SUBTILIS_RISCOS_ARM2_PROGRAM_START,
				      &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	stype = subtilis_type_section_new(&subtilis_type_void, 0, NULL, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	/* TODO: Figure out why 16 is being passed here for reg_counter */
	arm_s = subtilis_arm_prog_section_new(arm_p, stype, 16, 0, 0, 0, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	prv_add_ops(arm_s, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	code = (uint32_t *)subtilis_arm_encode_buf(arm_p, &code_size, &err);
	if (err.type != SUBTILIS_ERROR_OK)
		goto cleanup;

	for (i = 0; i < code_size / 4; i++)
		if (code[i] != prv_expected_code[i]) {
			fprintf(stderr, "%zu: expected 0x%x got 0x%x\n", i,
				prv_expected_code[i], code[i]);
			goto cleanup;
		}

	retval = 0;

cleanup:
	if ((retval == 1) || (err.type != SUBTILIS_ERROR_OK)) {
		printf(": [FAIL]\n");
		subtilis_error_fprintf(stdout, &err, true);
	} else {
		printf(": [OK]\n");
	}

	free(code);
	subtilis_arm_prog_delete(arm_p);
	subtilis_type_section_delete(stype);
	subtilis_constant_pool_delete(const_pool);
	subtilis_string_pool_delete(string_pool);
	subtilis_arm_op_pool_delete(op_pool);

	return retval;
}

static int prv_test_disass_data(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_data_instr_t *datai;

	subtilis_error_init(&err);

	/* MOVEQ R0, r1 */

	subtilis_arm_disass(&instr, prv_expected_code[0], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_MOV) {
		fprintf(stderr, "[0] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_MOV, instr.type);
		return 1;
	}

	datai = &instr.operands.data;
	if (datai->ccode != SUBTILIS_ARM_CCODE_EQ) {
		fprintf(stderr, "[0] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_EQ, datai->ccode);
		return 1;
	}

	if (datai->status) {
		fprintf(stderr, "[0] status should not be set\n");
		return 1;
	}

	if ((datai->dest != 0) || (datai->op2.type != SUBTILIS_ARM_OP2_REG) ||
	    (datai->op2.op.reg != 1)) {
		fprintf(stderr, "[0] bad register values\n");
		return 1;
	}

	/* MVNSNE R0, r1 */
	subtilis_arm_disass(&instr, prv_expected_code[1], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_MVN) {
		fprintf(stderr, "[1] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_MVN, instr.type);
		return 1;
	}
	datai = &instr.operands.data;

	if (datai->ccode != SUBTILIS_ARM_CCODE_NE) {
		fprintf(stderr, "[1] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_NE, datai->ccode);
		return 1;
	}

	if (!datai->status) {
		fprintf(stderr, "[1] status should be set\n");
		return 1;
	}

	if ((datai->dest != 0) || (datai->op2.type != SUBTILIS_ARM_OP2_REG) ||
	    (datai->op2.op.reg != 1)) {
		fprintf(stderr, "[1] bad register values\n");
		return 1;
	}

	/* CMPGT R0, R1 */
	subtilis_arm_disass(&instr, prv_expected_code[2], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_CMP) {
		fprintf(stderr, "[2] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_CMP, instr.type);
		return 1;
	}
	datai = &instr.operands.data;

	if (datai->ccode != SUBTILIS_ARM_CCODE_GT) {
		fprintf(stderr, "[2] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_GT, datai->ccode);
		return 1;
	}

	if (!datai->status) {
		fprintf(stderr, "[2] status should be set\n");
		return 1;
	}

	if ((datai->dest != 0) || (datai->op2.type != SUBTILIS_ARM_OP2_REG) ||
	    (datai->op2.op.reg != 1)) {
		fprintf(stderr, "[2] bad register values\n");
		return 1;
	}

	return 0;
}

static int prv_test_disass_mul(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_mul_instr_t *mul;

	subtilis_error_init(&err);

	subtilis_arm_disass(&instr, prv_expected_code[3], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	/* MULLT R0, R2, R1 */

	if (instr.type != SUBTILIS_ARM_INSTR_MUL) {
		fprintf(stderr, "[3] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_MUL, instr.type);
		return 1;
	}

	mul = &instr.operands.mul;
	if (mul->ccode != SUBTILIS_ARM_CCODE_LT) {
		fprintf(stderr, "[3] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_LT, mul->ccode);
		return 1;
	}

	if (mul->status) {
		fprintf(stderr, "[3] status should not be set\n");
		return 1;
	}

	if ((mul->dest != 0) || (mul->rs != 1) || (mul->rm != 2)) {
		fprintf(stderr, "[3] bad register values\n");
		return 1;
	}

	return 0;
}

static int prv_test_disass_stran(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_stran_instr_t *stran;

	subtilis_error_init(&err);

	/* LDRCS R0, [R2, #16] */
	subtilis_arm_disass(&instr, prv_expected_code[4], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_LDR) {
		fprintf(stderr, "[4] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_LDR, instr.type);
		return 1;
	}
	stran = &instr.operands.stran;

	if (stran->ccode != SUBTILIS_ARM_CCODE_CS) {
		fprintf(stderr, "[4] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_CS, stran->ccode);
		return 1;
	}

	if (!stran->pre_indexed) {
		fprintf(stderr, "[4] pre indexed expected\n");
		return 1;
	}

	if (stran->write_back) {
		fprintf(stderr, "[4] write back not expected\n");
		return 1;
	}

	if (stran->subtract) {
		fprintf(stderr, "[4] subtract not expected\n");
		return 1;
	}

	if ((stran->dest != 0) || (stran->base != 2)) {
		fprintf(stderr, "[4] bad register values\n");
		return 1;
	}

	if ((stran->offset.type != SUBTILIS_ARM_OP2_I32) ||
	    (stran->offset.op.integer != 16)) {
		fprintf(stderr, "[4] bad  values\n");
		return 1;
	}

	/* STRCS R0, [R2], R1, LSL #2 */
	subtilis_arm_disass(&instr, prv_expected_code[15], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != SUBTILIS_ARM_INSTR_STR) {
		fprintf(stderr, "[15] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_STR, instr.type);
		return 1;
	}
	stran = &instr.operands.stran;

	if (stran->ccode != SUBTILIS_ARM_CCODE_CS) {
		fprintf(stderr, "[15] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_CS, stran->ccode);
		return 1;
	}

	if (stran->pre_indexed) {
		fprintf(stderr, "[15] pre indexed not expected\n");
		return 1;
	}

	if (stran->subtract) {
		fprintf(stderr, "[15] subtract not expected\n");
		return 1;
	}

	if ((stran->dest != 0) || (stran->base != 2)) {
		fprintf(stderr, "[15] bad register values\n");
		return 1;
	}

	if ((stran->offset.type != SUBTILIS_ARM_OP2_SHIFTED) ||
	    (stran->offset.op.shift.reg != 1) ||
	    (stran->offset.op.shift.shift.integer != 2) ||
	    (stran->offset.op.shift.type != SUBTILIS_ARM_SHIFT_LSL)) {
		fprintf(stderr, "[15] bad  values\n");
		return 1;
	}

	return 0;
}

static int prv_test_disass_swi(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_swi_instr_t *swi;

	subtilis_error_init(&err);

	subtilis_arm_disass(&instr, prv_expected_code[5], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	/* SWICS &DC */

	if (instr.type != SUBTILIS_ARM_INSTR_SWI) {
		fprintf(stderr, "[5] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_SWI, instr.type);
		return 1;
	}

	swi = &instr.operands.swi;
	if (swi->ccode != SUBTILIS_ARM_CCODE_CS) {
		fprintf(stderr, "[5] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_CS, swi->ccode);
		return 1;
	}

	if (swi->code != 0xdc) {
		fprintf(stderr, "[5] code should be 0xdc found 0x%zx\n",
			swi->code);
		return 1;
	}

	return 0;
}

static int prv_test_disass_b(void)
{
	subtilis_error_t err;
	subtilis_arm_instr_t instr;
	subtilis_arm_br_instr_t *br;

	subtilis_error_init(&err);

	subtilis_arm_disass(&instr, prv_expected_code[6], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	/* BMI #-8 */

	if (instr.type != SUBTILIS_ARM_INSTR_B) {
		fprintf(stderr, "[6] expected instr 0x%x got 0x%x\n",
			SUBTILIS_ARM_INSTR_B, instr.type);
		return 1;
	}

	br = &instr.operands.br;
	if (br->ccode != SUBTILIS_ARM_CCODE_MI) {
		fprintf(stderr, "[6] expected ccode 0x%x got 0x%x\n",
			SUBTILIS_ARM_CCODE_MI, br->ccode);
		return 1;
	}

	if (br->link) {
		fprintf(stderr, "[6] link should not be set\n");
		return 1;
	}

	if (br->target.offset != -2) {
		fprintf(stderr, "[6] offset of -2 expected got %d\n",
			br->target.offset);
		return 1;
	}

	return 0;
}

static int prv_check_mtran(size_t index, bool write_back,
			   subtilis_arm_instr_type_t type,
			   subtilis_arm_mtran_type_t mtype, size_t reg_num,
			   size_t reg_list)
{
	subtilis_arm_instr_t instr;
	subtilis_arm_mtran_instr_t *mtran;
	subtilis_error_t err;

	subtilis_error_init(&err);
	subtilis_arm_disass(&instr, prv_expected_code[index], false, &err);
	if (err.type != SUBTILIS_ERROR_OK) {
		subtilis_error_fprintf(stdout, &err, true);
		return 1;
	}

	if (instr.type != type) {
		fprintf(stderr, "[%zu] expected instr 0x%x got 0x%x\n", index,
			type, instr.type);
		return 1;
	}

	mtran = &instr.operands.mtran;
	if (mtran->ccode != SUBTILIS_ARM_CCODE_AL) {
		fprintf(stderr, "[%zu] expected ccode 0x%x got 0x%x\n", index,
			SUBTILIS_ARM_CCODE_AL, mtran->ccode);
		return 1;
	}

	if (mtran->write_back != write_back) {
		fprintf(stderr, "[%zu] write back expected\n", index);
		return 1;
	}

	if (mtran->type != mtype) {
		fprintf(stderr, "[%zu] expected mtran type 0x%x got 0x%x\n",
			index, mtype, mtran->type);
		return 1;
	}

	if (mtran->op0 != reg_num) {
		fprintf(stderr, "[%zu] bad register value\n", index);
		return 1;
	}

	if (mtran->reg_list != reg_list) {
		fprintf(stderr, "[%zu] bad register values\n", index);
		return 1;
	}

	return 0;
}

static int prv_test_disass_mtran(void)
{
	int retval;

	/* LDMFA R7!, {R0} */
	retval = prv_check_mtran(7, true, SUBTILIS_ARM_INSTR_LDM,
				 SUBTILIS_ARM_MTRAN_DA, 7, 1);

	/* STMFA R7!, {R0} */
	retval |= prv_check_mtran(8, true, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_IB, 7, 1);

	/* LDMED R0!, {R3-R8} */
	retval |= prv_check_mtran(9, true, SUBTILIS_ARM_INSTR_LDM,
				  SUBTILIS_ARM_MTRAN_IB, 0, 0x1f8);

	/* STMED R0!, {R3-R8} */
	retval |= prv_check_mtran(10, true, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_DA, 0, 0x1f8);

	/* LDMFD R1!, {r15} */
	retval |= prv_check_mtran(11, true, SUBTILIS_ARM_INSTR_LDM,
				  SUBTILIS_ARM_MTRAN_IA, 1, 0x8000);

	/* STMFD R1!, {r14} */
	retval |= prv_check_mtran(12, true, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_DB, 1, 0x4000);

	/* LDMEA R13, {r0-r3} */
	retval |= prv_check_mtran(13, false, SUBTILIS_ARM_INSTR_LDM,
				  SUBTILIS_ARM_MTRAN_DB, 13, 0xf);

	/* STMEA R13, {r0-r3} */
	retval |= prv_check_mtran(14, false, SUBTILIS_ARM_INSTR_STM,
				  SUBTILIS_ARM_MTRAN_IA, 13, 0xf);

	return retval;
}

static int prv_test_disass(void)
{
	int retval;

	printf("arm_test_disass");

	retval = prv_test_disass_data();
	retval |= prv_test_disass_mul();
	retval |= prv_test_disass_swi();
	retval |= prv_test_disass_b();
	retval |= prv_test_disass_stran();
	retval |= prv_test_disass_mtran();

	if (retval == 1)
		printf(": [FAIL]\n");
	else
		printf(": [OK]\n");

	return retval;
}

int arm_test(void)
{
	int res = 0;

	res = prv_test_examples();
	res |= prv_test_encode();
	res |= prv_test_disass();
	res |= prv_test_riscos_arm_examples();
	res |= prv_test_riscos_fpa_examples();
	res |= prv_test_bad_cases();

	return res;
}
