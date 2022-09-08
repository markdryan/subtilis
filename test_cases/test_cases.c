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

#include "test_cases.h"

/* clang-format off */
const subtilis_test_case_t test_cases[] = {
	{ "subtraction",
	  "LET b% = 100 - 5\n"
	  "LET c% = 10 - b% -10\n"
	  "LET d% = c% - b% - 1\n"
	  "LET e% = d% - 1030\n"
	  "PRINT d%\n"
	  "PRINT e%\n",
	  "-191\n-1221\n"},
	{ "division",
	  "LOCAL f%\n"
	  "LET b% = 100 DIV 5\n"
	  "LET c% = 1000 DIV b% DIV 10\n"
	  "LET d% = b% DIV c% DIV 2\n"
	  "LET e% = c% DIV -2\n"
	  "LET f% = -b% DIV -c%\n"
	  "LET g% = -1\n"
	  "PRINT d%\n"
	  "PRINT e%\n"
	  "PRINT f%\n"
	  "PRINT g% DIV 2\n",
	  "2\n-2\n4\n0\n"},
	{ "mod",
	  "LOCAL b% = 100 MOD 6\n"
	  "LET c% = 6 MOD b%\n"
	  "LET d% = b% MOD c%\n"
	  "LET e% = -(c% * 10) MOD 3\n"
	  "LET f% = 34\n"
	  "LET g% = f% MOD 4\n"
	  "LET h% = -f% MOD 4\n"
	  "PRINT b%\n"
	  "PRINT c%\n"
	  "PRINT d%\n"
	  "PRINT e%\n"
	  "PRINT g%\n"
	  "PRINT h%\n",
	  "4\n2\n0\n-2\n2\n-2\n"},
	{ "multiplication",
	  "LET b% = 1 * 10\n"
	  "LET c% = 10 * b% * 5\n"
	  "LET d% = c% * 2 * b%\n"
	  "PRINT d%\n",
	  "10000\n"},
	{ "addition",
	  "LET b% = &ff + 10\n"
	  "LET c% = &10 + b% + 5\n"
	  "LET d% = c% + 2 + b%\n"
	  "PRINT d%\n",
	  "553\n"},
	{ "expression",
	  "LET a% = (1 + 1)\n"
	  "LET b% = ((((10 - a%) -5) * 10))\n"
	  "PRINT b%",
	  "30\n"},
	{ "unary_minus",
	  "LET b% = -110\n"
	  "LET c% = -b%\n"
	  "LET d% = 10 - -(c% - 0)\n"
	  "PRINT d%\n"
	  "PRINT -&ffffffff\n"
	  "PRINT -b% + c%\n",
	  "120\n1\n220\n"},
	{ "and",
	  "LET b% = -1\n"
	  "PRINT b% AND -1 AND TRUE AND b%"
	  "LET c% = b% AND FALSE\n"
	  "PRINT c%\n"
	  "PRINT 255 AND 128\n",
	  "-1\n0\n128\n"},
	{ "or",
	  "LET b% = -1\n"
	  "PRINT b% OR -1 OR TRUE OR b%\n"
	  "LET c% = b% OR FALSE\n"
	  "PRINT c%\n"
	  "PRINT FALSE OR FALSE\n"
	  "LET d% = FALSE\n"
	  "PRINT d% OR FALSE\n"
	  "PRINT 64 OR 128\n",
	  "-1\n-1\n0\n0\n192\n"},
	{ "eor",
	  "LET b% = -1\n"
	  "PRINT b% EOR -1 EOR TRUE EOR b%\n"
	  "LET c% = b% EOR FALSE\n"
	  "PRINT c%\n"
	  "PRINT FALSE EOR FALSE\n"
	  "PRINT TRUE EOR TRUE\n"
	  "LET d% = FALSE\n"
	  "PRINT d% EOR FALSE\n"
	  "PRINT 192 EOR 128\n",
	  "0\n-1\n0\n0\n0\n64\n"},
	{ "not",
	  "LET b% = -1\n"
	  "PRINT NOT b%\n"
	  "PRINT NOT NOT b%\n"
	  "PRINT NOT TRUE\n"
	  "PRINT NOT &fffffff0\n"
	  "LET c% = &fffffff0\n"
	  "PRINT NOT c% AND &f\n",
	  "0\n-1\n0\n15\n15\n"},
	{ "eq",
	  "LET b% = &ff\n"
	  "PRINT 10 = 5 + 5\n"
	  "PRINT b% = 255\n"
	  "PRINT b% = 254\n"
	  "LET c% = 255\n"
	  "PRINT c% = b%\n"
	  "LET c% = b% = 254\n"
	  "PRINT c%\n"
	  "PRINT COS(0) = 1\n",
	  "-1\n-1\n0\n-1\n0\n-1\n"},
	{ "neq",
	  "LET b% = &ff\n"
	  "PRINT 10 <> 5 + 5\n"
	  "PRINT b% <> 255\n"
	  "PRINT b% <> 254\n"
	  "LET c% = 255\n"
	  "PRINT c% <> b%\n"
	  "LET c% = b% <> 254\n"
	  "PRINT c%\n"
	  "PRINT COS(0) <> 1\n",
	  "0\n0\n-1\n0\n-1\n0\n"},
	{ "gt",
	  "LET b% = &ff\n"
	  "PRINT 10 > 5 + 5\n"
	  "PRINT b% > 255\n"
	  "PRINT 256 > b%\n"
	  "LET c% = 255\n"
	  "PRINT c% > b%\n"
	  "LET c% = b% > 254\n"
	  "PRINT c%\n",
	  "0\n0\n-1\n0\n-1\n"},
	{ "lte",
	  "LET b% = &ff\n"
	  "PRINT 10 <= 5 + 5\n"
	  "PRINT b% <= 255\n"
	  "PRINT 256 <= b%\n"
	  "LET c% = 255\n"
	  "PRINT c% <= b% +1\n"
	  "LET c% = b% <= 254\n"
	  "PRINT c%\n",
	  "-1\n-1\n0\n-1\n0\n"},
	{ "lt",
	  "LET b% = &ff\n"
	  "PRINT 10 < 5 + 5\n"
	  "PRINT b% < 255\n"
	  "PRINT 256 < b%\n"
	  "LET c% = 255\n"
	  "PRINT c% < b%\n"
	  "LET c% = 254 < b%\n"
	  "PRINT c%\n",
	  "0\n0\n0\n0\n-1\n"},
	{ "gte",
	  "LET b% = &ff\n"
	  "PRINT 10 >= 5 + 5\n"
	  "PRINT b% >= 255\n"
	  "PRINT 256 >= b%\n"
	  "LET c% = 255\n"
	  "PRINT c% >= b%\n"
	  "LET c% = 254 >= b%\n"
	  "PRINT c%\n",
	  "-1\n-1\n-1\n-1\n0\n"},
	{ "if",
	  "IF 10 >= 5 + 5 THEN\n"
	  "  PRINT TRUE\n"
	  "ELSE\n"
	  "  PRINT FALSE\n"
	  "ENDIF\n"
	  "PRINT 32\n"
	  "LET b% = &ff\n"
	  "IF b% > 255 THEN PRINT 100 ENDIF\n"
	  "IF b% <= 255 THEN\n"
	  "  PRINT 200\n"
	  "  PRINT 300\n"
	  "ENDIF\n",
	  "-1\n32\n200\n300\n"},
	{ "while",
	  "LET i% = 0\n"
	  "WHILE i% < 5\n"
	  "  PRINT i%\n"
	  "  LET i%=i%+1\n"
	  "ENDWHILE\n",
	  "0\n1\n2\n3\n4\n"},
	{ "if_and_while",
	  "LET i% = 0\n"
	  "WHILE i% < 6\n"
	  "  IF i% AND 1 THEN\n"
	  "    PRINT FALSE\n"
	  "  ELSE\n"
	  "    PRINT TRUE\n"
	  "  ENDIF\n"
	  "  LET i%=i%+1\n"
	  "ENDWHILE\n",
	  "-1\n0\n-1\n0\n-1\n0\n"},
	{ "eq_and_neq_br_imm",
	  "LET i% = 0\n"
	  "WHILE i% <> 6\n"
	  "  LET i%=i%+1\n"
	  "ENDWHILE\n"
	  "PRINT i%\n"
	  "IF i% = 6 THEN\n"
	  "  PRINT TRUE\n"
	  "ELSE\n"
	  "  PRINT FALSE\n"
	  "ENDIF\n",
	  "6\n-1\n"},
	{ "eq_and_neq_br",
	  "LET i% = 0\n"
	  "LET j% = 6\n"
	  "WHILE i% <> j%\n"
	  "  LET i%=i%+1\n"
	  "ENDWHILE\n"
	  "PRINT i%\n"
	  "IF i% = j% THEN\n"
	  "  PRINT TRUE\n"
	  "ELSE\n"
	  "  PRINT FALSE\n"
	  "ENDIF\n",
	  "6\n-1\n"},
	{ "basic_proc",
	  "LET i% = 0\n"
	  "PROCInci\n"
	  "PROCInci\n"
	  "PRINT i%\n"
	  "DEF PROCInci"
	  "  LET i%=i%+1\n"
	  "ENDPROC\n",
	  "2\n"},
	{ "local",
	  "LOCAL i%= 5\n"
	  "LET i%=i%+1\n"
	  "PROCInci\n"
	  "PRINT i%\n"
	  "DEF PROCInci"
	  "  LOCAL i%\n"
	  "  LET i%=i%+1\n"
	  "  PRINT i%\n"
	  "ENDPROC\n",
	  "1\n6\n"},
	{ "proc_args",
	  "LOCAL i%\n"
	  "LET i% = 1\n"
	  "LET j% = 2\n"
	  "PROCAdd(i%, j%, 10, 11, 12)\n"
	  "DEF PROCAdd(a%, b%, c%, d%, e%)\n"
	  "PRINT a% + b% + c% +d% + e%\n"
	  "ENDPROC\n",
	  "36\n"},
	{ "fn_fact",
	  "LOCAL x%\n"
	  "LET x% = FNFac%(4)\n"
	  "PRINT x%\n"
	  "DEF FNFac%(a%)\n"
	  "    IF a% <= 1 THEN\n"
	  "       <- 1\n"
	  "    ENDIF\n"
	  "<- a% * FNFac%(a%-1)\n",
	  "24\n"},
	{ "fn_fact_no_let",
	  "LOCAL x%\n"
	  "x% = FNFac%(4)\n"
	  "PRINT x%\n"
	  "DEF FNFac%(a%)\n"
	  "  LOCAL res%\n"
	  "  IF a% <= 1 THEN\n"
	  "    res% = 1\n"
	  "  ELSE\n"
	  "    res% = a% * FNFac%(a%-1)\n"
	  "  ENDIF\n"
	  "<-res%\n",
	  "24\n"},
	{ "abs",
	  "LOCAL A\n"
	  "LOCAL x%\n"
	  "\n"
	  "LET x%=-10\n"
	  "PRINT ABS(x%)\n"
	  "PRINT ABS(10)\n"
	  "PRINT INT(ABS(10.0))\n"
	  "PRINT INT(ABS(-10.0))\n"
	  "A = 10\n"
	  "PRINT INT(ABS(A))\n"
	  "A = -10\n"
	  "PRINT INT(ABS(A))\n"
	  "\n"
	  "PRINT ABS(-1)\n"
	  "X% = &ffffffff\n"
	  "PRINT ABS(X%)\n"
	  "\n"
	  "PRINT ABS(&80000000)\n"
	  "X% = &80000000\n"
	  "PRINT ABS(X%)\n",
	  "10\n10\n10\n10\n10\n10\n1\n1\n-2147483648\n-2147483648\n"},
	{ "fpa_small",
	  "LOCAL x\n"
	  "LOCAL y\n"
	  "LOCAL a\n"
	  "LOCAL b\n"
	  "LOCAL c\n"
	  "LOCAL d\n"
	  "\n"
	  "LET x = 2.0\n"
	  "LET y = 3.14\n"
	  "LET z = (x+x) / 2\n"
	  "LET a = z - x -10.0\n"
	  "LET b = a * 10\n"
	  "\n"
	  "PROCa(a,b,x,y,z)\n"
	  "\n"
	  "LET c = b * a\n"
	  "LET d = 2 / c\n"
	  "LET e% = d\n"
	  "LET f = e%\n"
	  "LET h = -y\n"
	  "LET e% = z\n"
	  "PRINT e%\n"
	  "\n"
	  "DEF PROCa(a,b,c,d,e)\n"
	  "LOCAL a%\n"
	  "LET a% = a + b + c + d + e\n"
	  "PRINT a%\n"
	  "ENDPROC\n",
	  "-102\n2\n"},
	{ "fpa_logical",
	  "LOCAL a = 3.14\n"
	  "LET b = 17.6666\n"
	  "LET c = 3.14\n"
	  "PRINT a < b\n"
	  "PRINT a > b\n"
	  "PRINT a = b\n"
	  "PRINT a = c\n"
	  "PRINT a <> b\n"
	  "PRINT a <> c\n"
	  "PRINT a < 17.6666\n"
	  "PRINT a > 17.6666\n"
	  "PRINT a = 17.6666\n"
	  "PRINT a = 3.14\n"
	  "PRINT a <> 17.6666\n"
	  "PRINT a <> 3.14\n"
	  "PRINT 3.14 < b\n"
	  "PRINT 3.14 > b\n"
	  "PRINT 3.14 = b\n"
	  "PRINT 3.14 = c\n"
	  "PRINT 3.14 <> b\n"
	  "PRINT 3.14 <> c\n"
	  "PRINT a <> 2.0\n"
	  "PRINT a OR b\n"
	  "PRINT a AND b\n"
	  "PRINT a EOR b\n"
	  "PRINT NOT a\n"
	  "PRINT a OR 17.6666\n"
	  "PRINT a AND 17.6666\n"
	  "PRINT a EOR 17.6666\n"
	  "PRINT NOT 3.14\n",
	  "-1\n0\n0\n-1\n-1\n0\n-1\n0\n0\n-1\n-1\n0\n-1\n0\n0\n"
	  "-1\n-1\n0\n-1\n19\n1\n18\n-4\n19\n1\n18\n-4\n"},
	{ "fpa_if",
	  "LOCAL a\n"
	  "LET a = 3.14\n"
	  "LET B% = 11\n"
	  "IF B% > a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF 10 < a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF 10 <= a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "PROCIf2\n"
	  "DEF PROCIf2\n"
	  "LOCAL a\n"
	  "a = 3.14\n"
	  "IF 11 = a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF 3.14 = a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF 3.14 <> a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF 3.14 >= a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "PROCTestVars(a, 11.0)\n"
	  "ENDPROC\n"
	  "DEF PROCTestVars(a,b)\n"
	  "IF a < b THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF a <= a THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF a > b THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF b >= b THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF a = b THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "IF a <> b THEN\n"
	  "PRINT TRUE\n"
	  "ELSE\n"
	  "PRINT FALSE\n"
	  "ENDIF\n"
	  "PRINT a < b\n"
	  "PRINT a <= a\n"
	  "PRINT a > b\n"
	  "PRINT b >= b\n"
	  "PRINT a = b\n"
	  "PRINT a <> b\n"
	  "ENDPROC\n",
	  "-1\n0\n0\n0\n-1\n0\n-1\n-1\n-1\n0\n-1\n0\n"
	  "-1\n-1\n-1\n0\n-1\n0\n-1\n"},
	{ "repeat",
	  "LET i% = 0\n"
	  "REPEAT\n"
	  "  PRINT i%\n"
	  "  LET i%=i%+1\n"
	  "UNTIL i%=5\n",
	  "0\n1\n2\n3\n4\n"},
	{ "int_reg_alloc_basic",
	  "LOCAL a%\n"
	  "LOCAL b%\n"
	  "LOCAL c%\n"
	  "LOCAL d%\n"
	  "LOCAL e%\n"
	  "LOCAL f%\n"
	  "LOCAL i%\n"
	  "LOCAL g%\n"
	  "LOCAL h%\n"
	  "LOCAL k%\n"
	  "LOCAL l%\n"
	  "LOCAL m%\n"
	  "LOCAL n%\n"
	  "LOCAL o%\n"
	  "\n"
	  "LET a% = 0\n"
	  "LET b% = 1\n"
	  "LET c% = 2\n"
	  "LET d% = 3\n"
	  "LET e% = 4\n"
	  "LET f% = 5\n"
	  "LET k% = 6\n"
	  "LET l% = 7\n"
	  "LET n% = 8\n"
	  "\n"
	  "LET g% = a% + b% + c% + d% + e% + f% + i% + k% + l% + n%\n"
	  "LET o% = g%\n"
	  "LET h% = a% + b% + c% + d% + e% + f% + i% + k% + l% + n%\n"
	  "LET m% = a% + b% + c% + d% + e% + f% + i% + k% + l% + n%\n"
	  "PRINT g%\n"
	  "PRINT h%\n"
	  "PRINT m%\n"
	  "PRINT o%\n"
	  "LET i% = i% + 1\n",
	  "36\n36\n36\n36\n"},
	{"fpa_reg_alloc_basic",
	 "LOCAL a\n"
	 "LOCAL b\n"
	 "LOCAL c\n"
	 "LOCAL d\n"
	 "LOCAL e\n"
	 "LOCAL f\n"
	 "LOCAL g\n"
	 "LOCAL h\n"
	 "LOCAL i\n"
	 "LOCAL m\n"
	 "\n"
	 "LET a = 0\n"
	 "LET b = 1\n"
	 "LET c = 2\n"
	 "LET d = 3\n"
	 "LET e = 4\n"
	 "LET i = 5\n"
	 "\n"
	 "LET a = a + 1\n"
	 "LET b = b + 1\n"
	 "LET c = c + 1\n"
	 "LET d = d + 1\n"
	 "LET e = e + 1\n"
	 "LET i = i + 1\n"
	 "\n"
	 "LET g = a + b + c + d + e + i\n"
	 "LET h = a + b + c + d + e + i\n"
	 "LET m = a + b + c + d + e + i\n"
	 "LET a% = g\n"
	 "PRINT a%\n"
	 "LET a% = h\n"
	 "PRINT a%\n"
	 "LET a% = m\n"
	 "PRINT a%\n"
	 "LET i = i + 1\n",
	 "21\n21\n21\n"},
	{"fp_save",
	"LOCAL a\n"
	"LOCAL b\n"
	"LOCAL c\n"
	"LOCAL d\n"
	"LOCAL e\n"
	"LOCAL f\n"
	"\n"
	"LET a = 1\n"
	"LET b = 2\n"
	"LET c = 3\n"
	"LET d = 4\n"
	"LET e = 4\n"
	"LET f = 5\n"
	"\n"
	"PROCOverwrite(f)\n"
	"\n"
	"LET a% = a + b + c + d\n"
	"PRINT a%\n"
	"\n"
	"DEF PROCOverwrite(a)\n"
	"LOCAL b\n"
	"LOCAL c\n"
	"LOCAL a%\n"
	"\n"
	"LET a = 10\n"
	"LET b = 20\n"
	"LET c = 30\n"
	"LET a% = a + b + c\n"
	"PRINT a%\n"
	"ENDPROC\n",
	 "60\n10\n"
	},
	{"int_save",
	 "LOCAL a%\n"
	 "LOCAL b%\n"
	 "LOCAL c%\n"
	 "LOCAL d%\n"
	 "LOCAL e%\n"
	 "LOCAL f%\n"
	 "\n"
	 "LET a% = 1\n"
	 "LET b% = 2\n"
	 "LET c% = 3\n"
	 "LET d% = 4\n"
	 "LET e% = 4\n"
	 "LET f% = 5\n"
	 "\n"
	 "PROCOverwrite(f%)\n"
	 "\n"
	 "LET a% = a% + b% + c% + d%\n"
	 "PRINT a%\n"
	 "\n"
	 "DEF PROCOverwrite(a%)\n"
	 "LOCAL b%\n"
	 "LOCAL c%\n"
	 "\n"
	 "LET a% = 10\n"
	 "LET b% = 20\n"
	 "LET c% = 30\n"
	 "LET a% = a% + b% + c%\n"
	 "PRINT a%\n"
	 "ENDPROC\n",
	 "60\n10\n"},
	{"branch_save",
	 "LOCAL a%\n"
	 "LOCAL c%\n"
	 "WHILE c% < 2\n"
	 "PRINT a%\n"
	 "PROCa\n"
	 "LET c% = c% + 1\n"
	 "ENDWHILE\n"
	 "DEF PROCa\n"
	 "LOCAL b%\n"
	 "LET b% = 2\n"
	 "ENDPROC\n",
	 "0\n0\n"},
	{"time",
	 "LOCAL a%\n"
	 "LET a% = TIME\n"
	 "PRINT TRUE\n",
	 "-1\n"},
	{"cos_and_sin",
	 "LOCAL a\n"
	 "PROCCheck(SIN(0), 0)\n"
	 "PROCCheck(COS(0), 1)\n"
	 "LET a = 0\n"
	 "LET b = 30\n"
	 "LET c = 60\n"
	 "PROCCheck(SIN(a), 0)\n"
	 "PROCCheck(COS(a), 1)\n"
	 "PROCCheck(SIN(RAD(30)), 0.5)\n"
	 "PROCCheck(COS(RAD(60)), 0.5)\n"
	 "LET a = RAD(b)\n"
	 "PROCCheck(SIN(a), 0.5)\n"
	 "LET a = RAD(c)\n"
	 "PROCCheck(COS(a), 0.5)\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n",
	 "-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n"},
	{"pi",
	 "PROCCheck(2 * PI , RAD(360))\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n",
	 "-1\n"},
	{"sqr",
	 "PROCCheck(SQR(2), 1.414)\n"
	 "LET A=2\n"
	 "PROCCheck(SQR(A), 1.414)\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n",
	 "-1\n-1\n"},
	{"mixed_args",
	 "PROCAdd(10, 10.0)\n"
	 "DEF PROCAdd(A%, A)\n"
	 "LOCAL R%\n"
	 "LET R% = A% + A\n"
	 "PRINT R%\n"
	 "ENDPROC",
	 "20\n"},
	{"vdu",
	 "LOCAL C%\n"
	 "LOCAL D%\n"
	 "VDU 115, 117, 98, 116, 105, 108, 105, 115\n"
	 "VDU [32]\n"
	 "VDU [&7573;&7462;&6C69;&7369;]\n"
	 "LET C% = 98\n"
	 "LET D% = &7573\n"
	 "VDU 32\n"
	 "VDU D%; C%, 116, 105, 108, 105, 115\n",
	 "subtilis subtilis subtilis"},
	{"void_fn",
	 "LOCAL C%\n"
	 "LET C%=FNA\n"
	 "PRINT C%\n"
	 "PROCB\n"
	 "DEF PROCB\n"
	 "PRINT FNA%\n"
	 "ENDPROC\n"
	 "DEF FNA%\n"
	 "<--1\n"
	 "DEF FNA\n"
	 "<-1.0\n",
	 "1\n-1\n"},
	{"assign_fn",
	 "LOCAL C%\n"
	 "PRINT FNA%\n"
	 "DEF FNA%\n"
	 "LOCAL A%\n"
	 "A%= 10\n"
	 "<-A%\n",
	 "10\n"},
	{"for_basic_int",
	 "LOCAL I%\n"
	 "FOR I% = 1 TO 4\n"
	 "PRINT I%\n"
	 "NEXT\n"
	 "FOR I% = 1 TO 0\n"
	 "PRINT I%\n"
	 "NEXT\n"
	 "FOR A% = 1 TO 4\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "FOR A% = 1 TO 0\n"
	 "PRINT A%\n"
	 "NEXT\n",
	 "1\n2\n3\n4\n1\n1\n2\n3\n4\n1\n"},
	{"for_basic_real",
	 "LOCAL I\n"
	 "LOCAL A%\n"
	 "FOR I = 1 TO 4\n"
	 "A%=I\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "FOR I = 1 TO 0\n"
	 "A%=I\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "FOR A = 1 TO 4\n"
	 "A%=A\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "FOR A = 1 TO 0\n"
	 "A%=A\n"
	 "PRINT A%\n"
	 "NEXT\n",
	 "1\n2\n3\n4\n1\n1\n2\n3\n4\n1\n"},
	{"for_step_int_const",
	 "FOR A% = 1 TO 10 STEP 2\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "FOR A% = 10 TO 0 STEP -2\n"
	 "PRINT A%\n"
	 "NEXT\n",
	 "1\n3\n5\n7\n9\n10\n8\n6\n4\n2\n0\n"},
	{"for_step_real_const",
	 "LOCAL A\n"
	 "LOCAL A%\n"
	 "FOR A = 1 TO 10 STEP 2\n"
	 "A%=A\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "FOR I = 10 TO 0 STEP -2\n"
	 "A%=I\n"
	 "PRINT A%\n"
	 "NEXT\n",
	 "1\n3\n5\n7\n9\n10\n8\n6\n4\n2\n0\n"},
	{"for_step_int_var",
	 "LOCAL S%\n"
	 "LOCAL A%\n"
	 "S% = 2\n"
	 "FOR A% = 0 TO 10 STEP S%\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "S% = -2\n"
	 "FOR A% = -1 TO -10 STEP S%\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "FOR A% = 0 TO -10 STEP S%\n"
	 "PRINT A%\n"
	 "NEXT\n",
	 "0\n2\n4\n6\n8\n10\n-1\n-3\n-5\n-7\n-9\n0\n-2\n-4\n-6\n-8\n-10\n"},
	{"for_step_real_var",
	 "LOCAL S\n"
	 "LOCAL A\n"
	 "LOCAL A%\n"
	 "S = 1.5\n"
	 "FOR A = 0 TO 10 STEP S\n"
	 "A% = A\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "S = 2.0\n"
	 "FOR A = -1 TO -10 STEP -S\n"
	 "A% = A\n"
	 "PRINT A%\n"
	 "NEXT\n"
	 "S = -2.0\n"
	 "FOR A = 0 TO -10 STEP S\n"
	 "A% = A\n"
	 "PRINT A%\n"
	 "NEXT\n",
	 "0\n1\n3\n4\n6\n7\n9\n-1\n-3\n-5\n-7\n-9\n0\n-2\n-4\n-6\n-8\n-10\n"},
	{"for_mod_step",
	 "LOCAL A%\n"
	 "LOCAL S%\n"
	 "LOCAL A\n"
	 "LOCAL S\n"
	 "S% = 2\n"
	 "FOR A% = 0 TO 10 STEP S%\n"
	 "PRINT A%\n"
	 "S%=S%+1\n"
	 "NEXT\n"
	 "S = 2\n"
	 "FOR A = 0 TO 10 STEP S\n"
	 "A% = A\n"
	 "PRINT A%\n"
	 "S=S+1\n"
	 "NEXT\n",
	 "0\n2\n4\n6\n8\n10\n0\n2\n4\n6\n8\n10\n"},
	{"point_tint",
	 "CLG\n"
	 "PRINT POINT(0,0)\n"
	 "PRINT TINT(0,0)\n",
	 "0\n0\n"},
	{"reg_alloc_buster",
	 "PROCTWELVE(1,2,3,4,5,6,7.0,8.0,9.0,10.0,11.0,12.0)\n"
	 "DEF PROCTWELVE(A%, B%, C%, D%, E%, F%, A, B, C, D, E, F)\n"
	 "  LOCAL Z%\n"
	 "  LOCAL G\n"
	 "  LOCAL H\n"
	 "  LOCAL I\n"
	 "\n"
	 "  PRINT A%\n"
	 "  PRINT B%\n"
	 "  PRINT C%\n"
	 "  PRINT D%\n"
	 "  PRINT E%\n"
	 "  PRINT F%\n"
	 "  Z% = A\n"
	 "  PRINT Z%\n"
	 "  Z% = B\n"
	 "  PRINT Z%\n"
	 "  Z% = C\n"
	 "  PRINT Z%\n"
	 "  Z% = D\n"
	 "  PRINT Z%\n"
	 "  Z% = E\n"
	 "  PRINT Z%\n"
	 "  Z% = F\n"
	 "  PRINT Z%\n"
	 "  G = F + 1\n"
	 "  H = G + 1\n"
	 "  I = H + 1\n"
	 "  Z% = G\n"
	 "  PRINT Z%\n"
	 "  Z% = H\n"
	 "  PRINT Z%\n"
	 "  Z% = I\n"
	 "  PRINT Z%\n"
	 "  Z% = A\n"
	 "  PRINT Z%\n"
	 "  Z% = B\n"
	 "  PRINT Z%\n"
	 "  Z% = C\n"
	 "  PRINT Z%\n"
	 "  Z% = D\n"
	 "  PRINT Z%\n"
	 "  Z% = E\n"
	 "  PRINT Z%\n"
	 "  Z% = F\n"
	 "ENDPROC\n",
	 "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n7\n8\n9\n10\n11\n"
	},
	{"save_third",
	 "PROCA(1,2,3)\n"
	 "PROCF(1.0,2.0,3.0)\n"
	 "DEF PROCA(A%, B%, C%)\n"
	 "  PROCB(A%, B%)\n"
	 "  PRINT C%\n"
	 "ENDPROC\n"
	 "DEF PROCB(A%, B%)\n"
	 "    PRINT A%\n"
	 "    PRINT B%\n"
	 "ENDPROC\n"
	 "DEF PROCF(A, B, C)\n"
	 "    LOCAL A%\n"
	 "    PROCG\n"
	 "    A% = C\n"
	 "    PRINT A%\n"
	 "ENDPROC\n"
	 "DEF PROCG\n"
	 "    PROCH(4.0, 5.0, 6.0)\n"
	 "ENDPROC\n"
	 "DEF PROCH(A, B, C)\n"
	 "ENDPROC\n",
	 "1\n2\n3\n3\n"
	},
	{"save_useless_move",
	 "PROCA(10)\n"
	 "\n"
	 "DEF PROCA(A%)\n"
	 "    PROCB\n"
	 "    PRINT A%\n"
	 "ENDPROC\n"
	 "\n"
	 "DEF PROCB\n"
	 "    GCOL 0, 1\n"
	 "ENDPROC\n",
	 "10\n"
	},
	{"save_fixed_reg_arg",
	 "PROCFOR(2)\n"
	 "\n"
	 "DEF PROCFOR(S%)\n"
	 "  LOCAL I%\n"
	 "\n"
	 "  GCOL 0, 2\n"
	 "  FOR I% = -10 TO 10 STEP S%\n"
	 "    PRINT I%\n"
	 "  NEXT\n"
	 "ENDPROC\n",
	 "-10\n-8\n-6\n-4\n-2\n0\n2\n4\n6\n8\n10\n"
	},
	{ "cmp_imm",
	"FOR X% = 0 TO 1300 STEP 1300\n"
	"    PRINT X%\n"
	"NEXT",
	"0\n1300\n"
	},
	{ "assignment_ops",
	  "LOCAL A%\n"
	  "LOCAL B%\n"
	  "LOCAL B\n"
	  "\n"
	  "A% = 10\n"
	  "A% += 1\n"
	  "PRINT A%\n"
	  "A% -= 10\n"
	  "PRINT A%\n"
	  "\n"
	  "B% = 10\n"
	  "A% += B%\n"
	  "PRINT A%\n"
	  "A% -= B%\n"
	  "PRINT A%\n"
	  "\n"
	  "A% += 10.0\n"
	  "PRINT A%\n"
	  "A% -= 10.0\n"
	  "PRINT A%\n"
	  "\n"
	  "B = 10.0\n"
	  "A% += B\n"
	  "PRINT A%\n"
	  "A% -= B\n"
	  "PRINT A%\n"
	  "\n"
	  "C = 1\n"
	  "C += 10.0\n"
	  "A% = C\n"
	  "PRINT A%\n"
	  "\n"
	  "C -= 10\n"
	  "A% = C\n"
	  "PRINT A%\n"
	  "\n"
	  "C += B\n"
	  "A% = C\n"
	  "PRINT A%\n"
	  "\n"
	  "C -= B\n"
	  "A% = C\n"
	  "PRINT A%\n"
	  "\n"
	  "C += B%\n"
	  "A% = C\n"
	  "PRINT A%\n"
	  "C -= B%\n"
	  "A% = C\n"
	  "PRINT A%\n",
	  "11\n1\n11\n1\n11\n1\n11\n1\n11\n1\n11\n1\n11\n1\n"
	},
	{ "int",
	  "PRINT INT(1.5)\n"
	  "PRINT INT(-1.5)\n"
	  "A = 1.5\n"
	  "\n"
	  "PRINT INT(A)\n"
	  "PRINT INT(-A)\n",
	  "1\n-2\n1\n-2\n"
	},
	{ "trig",
	  "PRINT INT(TAN(RAD(45)))\n"
	  "A = RAD(45)\n"
	  "PRINT INT(TAN(A))\n"
	  "\n"
	  "A = ASN(0.5) - 0.5236\n"
	  "PRINT NOT (A > 0.001 OR A < - 0.001)\n"
	  "\n"
	  "A = 0.5\n"
	  "A = ASN(A) - 0.5236\n"
	  "PRINT NOT (A > 0.001 OR A < - 0.001)\n"
	  "\n"
	  "A = ACS(0.3) - 1.2661\n"
	  "PRINT NOT (A > 0.001 OR A < - 0.001)\n"
	  "\n"
	  "A = 0.3\n"
	  "A = ACS(A) - 1.2661\n"
	  "PRINT NOT (A > 0.001 OR A < - 0.001)\n"
	  "\n"
	  "A = ATN(0.8) - 0.6747\n"
	  "PRINT NOT (A > 0.001 OR A < - 0.001)\n"
	  "\n"
	  "A = 0.8\n"
	  "A = ATN(A) - 0.6747\n"
	  "PRINT NOT (A > 0.001 OR A < - 0.001)\n",
	  "1\n1\n-1\n-1\n-1\n-1\n-1\n-1\n"
	},
	{"log",
	 "PRINT INT(LOG(1000))\n"
	 "A = 1000\n"
	 "PRINT INT(LOG(A))\n"
	 "\n"
	 "A = LN(10)\n"
	 "A -= 2.30258509\n"
	 "PRINT NOT (A > 0.001 OR A < - 0.001)\n"
	 "\n"
	 "A = 10\n"
	 "A = LN(A) - 2.30258509\n"
	 "PRINT NOT (A > 0.001 OR A < - 0.001)\n",
	 "3\n3\n-1\n-1\n"},
	{"rnd_int",
	 "X%=100\n"
	 "PRINT INT(RND(-X%))\n"
	 "FOR I% = 0 TO 5\n"
	 "PRINT INT(RND(X%))\n"
	 "NEXT\n"
	 "PRINT RND(-100)\n"
	 "FOR I% = 0 TO 5\n"
	 "PRINT RND(100)\n"
	 "NEXT\n",
	 "100\n24\n99\n74\n1\n16\n23\n100\n24\n99\n74\n1\n16\n23\n"},
	{"rnd_real",
	 "X = RND(1)\n"
	 "PRINT X >= 0 AND X <= 1\n"
	 "Y = RND(0)\n"
	 "PRINT X = Y\n"
	 "I = 1\n"
	 "X = RND(I)"
	 "PRINT X >= 0 AND X <= 1\n"
	 "I = 0\n"
	 "Y = RND(I)\n"
	 "PRINT X = Y\n",
	 "-1\n-1\n-1\n-1\n"},
	{"printfp",
	 "PRINT PI\n"
	 "PRINT -PI\n"
	 "PRINT 3.5\n",
	 "3.1428571428\n-3.1428571428\n3.5\n"},
	{"end",
	 "FOR I% = 0 TO 10\n"
	 "  IF I% = 5 THEN\n"
	 "    END\n"
	 "  ENDIF\n"
	 "  PRINT I%\n"
	 "NEXT\n",
	 "0\n1\n2\n3\n4\n"},
	{"error_endproc_early",
	 "PRINT 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 1\n"
	 "ENDERROR\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 2\n"
	 "END\n"
	 "ENDERROR\n"
	 "\n"
	 "ERROR 3\n",
	 "0\n2\n",
	 true},
	{"error_handled",
	 "PRINT 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDERROR\n"
	 "\n"
	 "PROCFail\n"
	 "\n"
	 "PRINT 5\n"
	 "\n"
	 "DEF PROCFail\n"
	 "PRINT 1\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 3\n"
	 "ENDPROC\n"
	 "ENDERROR\n"
	 "\n"
	 "PROCFail2\n"
	 "ENDPROC\n"
	 "\n"
	 "DEF PROCFail2\n"
	 "PRINT 2\n"
	 "ERROR 4\n"
	 "ENDPROC\n",
	 "0\n1\n2\n3\n5\n"},
	{"error_handled_fn",
	 "PRINT 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT FNFail%\n"
	 "\n"
	 "PRINT 6\n"
	 "\n"
	 "DEF FNFail%\n"
	 "PRINT 1\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 3\n"
	 "<-4\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT FNFail2%\n"
	 "<-0\n"
	 "\n"
	 "DEF FNFail2%\n"
	 "PRINT 2\n"
	 "ERROR 5\n"
	 "<-0\n",
	 "0\n1\n2\n3\n4\n6\n",
	},
	{"error_main",
	"PRINT 0\n"
	"\n"
	"ONERROR\n"
	"PRINT ERR\n"
	"ENDERROR\n"
	"\n"
	"ONERROR\n"
	"PRINT 1\n"
	"ENDERROR\n"
	"\n"
	"ERROR 2\n",
	"0\n1\n2\n"},
	{"main_end",
	 "PRINT 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDERROR\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 1\n"
	 "END\n"
	 "ENDERROR\n"
	 "\n"
	 "ERROR 2\n",
	 "0\n1\n",
	true},
	{"error_nested",
	"PRINT 0\n"
	"\n"
	"ONERROR\n"
	"PRINT ERR\n"
	"ENDERROR\n"
	"\n"
	"PROCFail\n"
	"\n"
	"DEF PROCFail\n"
	"PRINT 1\n"
	"\n"
	"ONERROR\n"
	"PRINT 3\n"
	"ENDERROR\n"
	"\n"
	"PROCFail2\n"
	"ENDPROC\n"
	"\n"
	"DEF PROCFail2\n"
	"PRINT 2\n"
	"ERROR 4\n"
	"ENDPROC\n",
	"0\n1\n2\n3\n4\n"},
	{"error_nested_endfn_early",
	 "PRINT 0\n"
	 "\n"
	 "PRINT FNNested%\n"
	 "\n"
	 "PRINT 1\n"
	 "\n"
	 "DEF FNNested%\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 1\n"
	 "ENDERROR\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 2\n"
	 "<-0\n"
	 "ENDERROR\n"
	 "\n"
	 "ERROR 3\n"
	 "<-0\n",
	 "0\n2\n0\n1\n"},
	{"error_nested_endproc_early",
	 "PRINT 0\n"
	 "\n"
	 "PROCNested\n"
	 "\n"
	 "PRINT 1\n"
	 "\n"
	 "DEF PROCNested\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 1\n"
	 "ENDERROR\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 2\n"
	 "ENDPROC\n"
	 "ENDERROR\n"
	 "\n"
	 "ERROR 3\n"
	 "ENDPROC\n",
	 "0\n2\n1\n"},
	{"error_nested_fn",
	 "PRINT 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT FNFail%\n"
	 "\n"
	 "DEF FNFail%\n"
	 "PRINT 1\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 3\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT FNFail2%\n"
	 "<-1\n"
	 "\n"
	 "DEF FNFail2%\n"
	 "PRINT 2\n"
	 "ERROR 4\n"
	 "<-1\n",
	 "0\n1\n2\n3\n4\n"},
	{"error_throw_fn",
	 "PRINT 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT FNFail%\n"
	 "\n"
	 "PRINT 5\n"
	 "\n"
	 "DEF FNFail%\n"
	 "PRINT 1\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT 3\n"
	 "ERROR 6\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT FNFail2%\n"
	 "<-0\n"
	 "\n"
	 "DEF FNFail2%\n"
	 "PRINT 2\n"
	 "ERROR 4\n"
	 "<-0\n",
	 "0\n1\n2\n3\n6\n"},
	{"error_error_in_main",
	 "ONERROR\n"
	 "PRINT 0\n"
	 "ENDERROR\n"
	 "\n"
	 "FOR I% = 0 TO 10\n"
	 "ONERROR\n"
	 "PRINT 1\n"
	 "ERROR 1\n"
	 "ENDERROR\n"
	 "ERROR 29\n"
	 "NEXT\n",
	 "1\n"},
	{"dividebyzero",
	 "PROCDIVByZero\n"
	 "PROCDivideByZero\n"
	 "PROCRDivideByZero\n"
	 "\n"
	 "DEF PROCDIVByZero\n"
	 "LOCAL A%\n"
	 "LOCAL B%\n"
	 "\n"
	 "A% = 100\n"
	 "B% = 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDPROC\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT A% DIV B%\n"
	 "PRINT &ff\n"
	 "ENDPROC\n"
	 "\n"
	 "DEF PROCDivideByZero\n"
	 "LOCAL A\n"
	 "LOCAL B\n"
	 "\n"
	 "A = 100\n"
	 "B = 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDPROC\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT A / B\n"
	 "PRINT &ff\n"
	 "ENDPROC\n"
	 "\n"
	 "DEF PROCRDivideByZero\n"
	 "LOCAL A\n"
	 "LOCAL B\n"
	 "\n"
	 "A = 100\n"
	 "B = 0\n"
	 "\n"
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDPROC\n"
	 "ENDERROR\n"
	 "\n"
	 "PRINT 10 / B\n"
	 "PRINT &ff\n"
	 "ENDPROC\n",
	 "18\n18\n18\n"},
	{ "logrange",
	"PROCLogZero\n"
	"PROCLnNeg\n"
	"A = 10\n"
	"PRINT 105 / A\n"
	"\n"
	"DEF PROCLogZero\n"
	"LOCAL A\n"
	"A = 0\n"
	"ONERROR\n"
	"        PRINT ERR\n"
	"ENDPROC\n"
	"ENDERROR\n"
	"\n"
	"PRINT LOG(A)\n"
	"ENDPROC\n"
	"\n"
	"DEF PROCLnNeg\n"
	"LOCAL A\n"
	"A = -1\n"
	"ONERROR\n"
	"        PRINT ERR\n"
	"ENDPROC\n"
	"ENDERROR\n"
	"\n"
	"PRINT LN(A)\n"
	"ENDPROC\n",
	"22\n22\n10.5\n"},
	{"end_in_proc",
	 "PRINT TRUE\n"
	 "PROCEnd\n"
	 "PRINT TRUE\n"
	 "DEF PROCEnd\n"
	 "END\n"
	 "ENDPROC\n",
	 "-1\n",
	 true},
	{"array_int_simple",
	 "LOCAL i%\n"
	 "DIM a%(10)\n"
	 "FOR i% = 0 TO 10\n"
	 "    a%(i%) = i%\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 10\n"
	 "    PRINT a%(i%)\n"
	 "NEXT\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"},
	{"array_double_simple",
	 "LOCAL i\n"
	 "DIM a(10)\n"
	 "FOR i% = 0 TO 10\n"
	 "    a(i%) = i% + 0.5\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 10\n"
	 "    PRINT a(i%)\n"
	 "NEXT\n",
	 "0.5\n1.5\n2.5\n3.5\n4.5\n5.5\n6.5\n7.5\n8.5\n9.5\n10.5\n"},
	{"array2D_int_const",
	 "LOCAL i%\n"
	 "LOCAL j%\n"
	 "LOCAL c%\n"
	 "DIM a%(2,2)\n"
	 "FOR i% = 0 TO 2\n"
	 "    FOR j% = 0 TO 2\n"
	 "        a%(i%,j%) = c%\n"
	 "        c% += 1\n"
	 "    NEXT\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 2\n"
	 "    FOR j% = 0 TO 2\n"
	 "        PRINT a%(i%,j%)\n"
	 "    NEXT\n"
	 "NEXT\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n"},
	{"array2D_int_var",
	 "LOCAL i% = 2\n"
	 "LOCAL j% = 2\n"
	 "LOCAL c%\n"
	 "DIM a%(i%, j%)\n"
	 "FOR i% = 0 TO 2\n"
	 "    FOR j% = 0 TO 2\n"
	 "        a%(i%,j%) = c%\n"
	 "        c% += 1\n"
	 "    NEXT\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 2\n"
	 "    FOR j% = 0 TO 2\n"
	 "        PRINT a%(i%,j%)\n"
	 "    NEXT\n"
	 "NEXT\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n"},
	{"array_bad_index_var",
	 "ONERROR\n"
	 "    PRINT ERR\n"
	 "ENDERROR\n"
	 "DIM a%(2)\n"
	 "i%=3\n"
	 "PRINT a%(i%)\n",
	 "10\n"},
	{"array2d_bad_index_var",
	 "ONERROR\n"
	 "    PRINT ERR\n"
	 "ENDERROR\n"
	 "DIM a%(2,2)\n"
	 "i%=3\n"
	 "PRINT a%(i%,0)\n",
	 "10\n"},
	{"array2d_int_zero",
	 "DIM a%(1024)\n"
	 "FOR i% = 0 TO 1024\n"
	 "  IF a%(i%) <> 0 THEN\n"
	 "      PRINT 0\n"
	 "      END\n"
	 "  ENDIF\n"
	 "NEXT\n"
	 "PRINT 1\n",
	 "1\n"},
	{"array2d_int_real",
	 "DIM a(1024)\n"
	 "FOR i% = 0 TO 1024\n"
	 "  IF a(i%) <> 0.0 THEN\n"
	 "      PRINT 0\n"
	 "      END\n"
	 "  ENDIF\n"
	 "NEXT\n"
	 "PRINT 1\n",
	 "1\n"},
	{"array1d_int_plus_equals",
	 "DIM a%(8)\n"
	 "FOR i% = 0 TO 8\n"
	 "  a%(i%) = 1\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  a%(i%) += i%\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  PRINT a%(i%)\n"
	 "NEXT\n",
	 "1\n2\n3\n4\n5\n6\n7\n8\n9\n"},
	{"array1d_int_minus_equals",
	 "DIM a%(8)\n"
	 "FOR i% = 0 TO 8\n"
	 "  a%(i%) = 10\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  a%(i%) -= i%\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  PRINT a%(i%)\n"
	 "NEXT\n",
	 "10\n9\n8\n7\n6\n5\n4\n3\n2\n"},
	{"array1d_real_plus_equals",
	 "DIM a(8)\n"
	 "FOR i% = 0 TO 8\n"
	 "  a(i%) = 1.5\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  a(i%) += i%\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  PRINT a(i%)\n"
	 "NEXT\n",
	 "1.5\n2.5\n3.5\n4.5\n5.5\n6.5\n7.5\n8.5\n9.5\n"},
	{"array1d_real_minus_equals",
	 "DIM a(8)\n"
	 "FOR i% = 0 TO 8\n"
	 "  a(i%) = 10.5\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  a(i%) -= i%\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 8\n"
	 "  PRINT a(i%)\n"
	 "NEXT\n",
	 "10.5\n9.5\n8.5\n7.5\n6.5\n5.5\n4.5\n3.5\n2.5\n"},
	{"array2d_int_plus_equals",
	 "DIM a%(2,2)\n"
	 "LOCAL i%\n"
	 "LOCAL j%\n"
	 "FOR i% = 0 TO 2\n"
	 "  FOR j% = 0 TO 2\n"
	 "    a%(i%, j%) = 1\n"
	 "  NEXT\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 2\n"
	 "  FOR j% = 0 TO 2\n"
	 "    a%(i%, j%) += (i%*3) + j%\n"
	 "  NEXT\n"
	 "NEXT\n"
	 "FOR i% = 0 TO 2\n"
	 "  FOR j% = 0 TO 2\n"
	 "    PRINT a%(i%, j%)\n"
	 "  NEXT\n"
	 "NEXT\n",
	 "1\n2\n3\n4\n5\n6\n7\n8\n9\n"},
	{"for_array_int_var",
	 "DIM a%(2)\n"
	 "FOR a%(0) = 0 TO 10\n"
	 "  PRINT a%(0)\n"
	 "NEXT\n"
	 "PRINT a%(0)\n"
	 "PRINT a%(1)\n"
	 "PRINT a%(2)\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n0\n0\n"},
	{"for_array_real_var",
	 "DIM a(2)\n"
	 "FOR a(0) = 0.5 TO 10.5\n"
	 "  PRINT a(0)\n"
	 "NEXT\n"
	 "PRINT a(0)\n"
	 "PRINT a(1)\n"
	 "PRINT a(2)\n",
	 "0.5\n1.5\n2.5\n3.5\n4.5\n5.5\n6.5\n7.5\n8.5\n9.5\n10.5\n11.5\n0\n0\n"
	},
	{"for_array_int_var_step",
	 "DIM a%(2)\n"
	 "FOR a%(0) = 0 TO 10 STEP 2\n"
	 "  PRINT a%(0)\n"
	 "NEXT\n"
	 "PRINT a%(0)\n"
	 "PRINT a%(1)\n"
	 "PRINT a%(2)\n",
	 "0\n2\n4\n6\n8\n10\n12\n0\n0\n"},
	{"for_array_real_var",
	 "DIM a(2)\n"
	 "FOR a(0) = 0.5 TO 10.5 STEP 2\n"
	 "  PRINT a(0)\n"
	 "NEXT\n"
	 "PRINT a(0)\n"
	 "PRINT a(1)\n"
	 "PRINT a(2)\n",
	 "0.5\n2.5\n4.5\n6.5\n8.5\n10.5\n12.5\n0\n0\n"
	},
	{"for_array_int_var_step_var",
	 "LOCAL st% = 2\n"
	 "DIM a%(2)\n"
	 "FOR a%(0) = 0 TO 10 STEP st%\n"
	 "  PRINT a%(0)\n"
	 "NEXT\n"
	 "PRINT a%(0)\n"
	 "PRINT a%(1)\n"
	 "PRINT a%(2)\n",
	 "0\n2\n4\n6\n8\n10\n12\n0\n0\n"},
	{"array_dim_oom",
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDERROR\n"
	 "DIM a%(10000000)\n"
	 "PRINT 1\n",
	 "11\n"},
	{"array_local_dim_oom",
	 "ONERROR\n"
	 "PRINT ERR\n"
	 "ENDERROR\n"
	 "PROCAlloc\n"
	 "PRINT 2\n"
	 "DEF PROCAlloc\n"
	 "LOCAL DIM a%(10000000)\n"
	 "PRINT 1\n"
	 "ENDPROC\n",
	 "11\n"},
	{"global_deref",
	 "DIM c(100)\n",
	 ""
	},
	{"local_deref",
	 "LOCAL DIM c(100)\n",
	 ""
	},
	{"local_if_deref",
	 "IF TRUE THEN\n"
	 "    LOCAL DIM c(100)\n"
	 "ENDIF",
	 ""
	},
	{"local_loop_deref",
	 "FOR I% = 0 TO 3\n"
	 "    LOCAL DIM c(100)\n"
	 "NEXT",
	 ""
	},
	{"local_proc_deref",
	 "PROCP\n"
	 "DEF PROCP\n"
	 "  LOCAL DIM c(100)\n"
	 "ENDPROC\n",
	 ""
	},
	{"local_if_fn_deref",
	 "PRINT FNF\n"
	 "DEF FNF\n"
	 "  IF TRUE THEN\n"
	 "    LOCAL DIM c(100)\n"
	 "  ENDIF"
	 "<-0\n",
	 "0\n"
	},
	{"local_proc_nested_deref",
	 "PROCP\n"
	 "DEF PROCP\n"
	 "  LOCAL DIM a(10)\n"
	 "    REPEAT\n"
	 "        LOCAL DIM b(10)\n"
	 "        IF TRUE THEN\n"
	 "            LOCAL DIM c(10)\n"
	 "        ENDIF\n"
	 "    UNTIL TRUE\n"
	 "ENDPROC\n",
	 ""
	},
	{"local_on_error_deref",
	 "ONERROR\n"
	 "    TRY\n"
	 "        LOCAL DIM c(100)\n"
	 "    ENDTRY\n"
	 "ENDERROR\n"
	 "ERROR 1\n",
	 ""
	},
	{"local_proc_on_error_deref",
	 "PROCP\n"
	 "DEF PROCP\n"
	 "    ONERROR\n"
	 "        TRY\n"
	 "            LOCAL DIM c(100)\n"
	 "        ENDTRY\n"
	 "    ENDERROR\n"
	 "    ERROR 1\n"
	 "ENDPROC\n",
	 ""
	},
	{"local_proc_return_deref",
	 "PROCP\n"
	 "DEF PROCP\n"
	 "    LOCAL DIM c(100)\n"
	 "    IF TRUE THEN\n"
	 "         LOCAL DIM d(10)\n"
	 "         ENDPROC\n"
	 "    ENDIF\n"
	 "ENDPROC\n",
	 ""
	},
	{"local_proc_return_deref",
	 "PRINT FNF\n"
	 "DEF FNF\n"
	 "    LOCAL DIM c(100)\n"
	 "    IF TRUE THEN\n"
	 "         LOCAL DIM d(10)\n"
	 "         <-0\n"
	 "    ENDIF\n"
	 "<-0\n",
	 "0\n"
	},
	{"error_deref",
	 "ONERROR\n"
	 "  PRINT ERR\n"
	 "ENDERROR\n"
	 "PROCP\n"
	 "DEF PROCP\n"
	 "  LOCAL DIM a%(10)\n"
	 "  IF TRUE THEN\n"
	 "    LOCAL DIM b%(10)\n"
	 "    REPEAT\n"
	 "      LOCAL DIM c%(10)\n"
	 "      ERROR 1\n"
	 "    UNTIL TRUE\n"
	 "  ENDIF\n"
	 "ENDPROC\n",
	 "1\n"},
	{"global_shadow",
	 "x% = 1\n"
	 "if true then\n"
	 "  x% := 2\n"
	 "  print x%\n"
	 "endif\n"
	 "print x%\n",
	 "2\n1\n"
	},
	{"for_global_shadow",
	 "i% = 0\n"
	 "if true then\n"
	 "  for i% := 0 to 10\n"
	 "    print i%\n"
	 "  next\n"
	 "endif\n"
	 "print i%\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n0\n"
	},
	{"rem",
	 "rem Copyright Mark Ryan (c) 2019\n"
	 "rem\n"
	 "for i% := 0 to 10 REM Loop 10 times\n"
	 "  print i%\n"
	 "next\n"
	 "rem the end\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"array_global_assign",
	 "dim a%(10)\n"
	 "dim b%(10)\n"
	 "for i% := 0 to 10\n"
	 "  a%(i%) = i%\n"
	 "next\n"
	 "b%() = a%()\n"
	 "for i% = 0 to 10\n"
	 "  print b%(i%)\n"
	 "next\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"array_local_assign",
	 "local dim a%(10)\n"
	 "local dim b%(10)\n"
	 "for i% := 0 to 10\n"
	 "  a%(i%) = i%\n"
	 "next\n"
	 "b%() = a%()\n"
	 "for i% = 0 to 10\n"
	 "  print b%(i%)\n"
	 "next\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"array_global_local_assign",
	 "dim b%(10)\n"
	 "PROCAssign\n"
	 "DEF PROCAssign\n"
	 "  local dim a%(10)\n"
	 "  for i% := 0 to 10\n"
	 "    a%(i%) = i%\n"
	 "  next\n"
	 "  b%() = a%()\n"
	 "ENDPROC\n"
	 "for i% = 0 to 10\n"
	 "  print b%(i%)\n"
	 "next\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"array_int_arg",
	 "local dim a%(10)\n"
	 "for i% := 0 to 10\n"
	 "  a%(i%) = i%\n"
	 "next\n"
	 "PROCArr(a%())\n"
	 "def PROCArr(b%(1))\n"
	 "for i% := 0 to 10\n"
	 "    print b%(i%)\n"
	 "next\n"
	 "endproc\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"array_two_int_arg",
	 "local dim a%(10)\n"
	 "local dim b%(10)\n"
	 "for i% := 0 to 10\n"
	 "  a%(i%) = i%\n"
	 "  b%(i%) = i% * 2\n"
	 "next\n"
	 "PROCSum(a%(), b%())\n"
	 "def PROCSum(c%(1), d%(1))\n"
	 "for i% := 0 to 10\n"
	 "print c%(i%) + d%(i%)\n"
	 "next\n"
	 "endproc\n",
	 "0\n3\n6\n9\n12\n15\n18\n21\n24\n27\n30\n"
	},
	{"array_int_arg_fifth",
	 "local dim a%(10)\n"
	 "for i% := 0 to 10\n"
	 "  a%(i%) = i%\n"
	 "next\n"
	 "PROCArr(1,2,3,4,a%())\n"
	 "def PROCArr(a%, b%, c%, d%, e%(1))\n"
	 "num% := a% + b% + c% + d%\n"
	 "for i% := 0 to 10\n"
	 "    num% += e%(i%)\n"
	 "next\n"
	 "print num%\n"
	 "endproc\n",
	 "65\n",
	},
	{"array_real_arg",
	 "local dim a(10)\n"
	 "for i% := 0 to 10\n"
	 "  a(i%) = i%\n"
	 "next\n"
	 "PROCArr(a())\n"
	 "def PROCArr(b(1))\n"
	 "for i% := 0 to 10\n"
	 "    print b(i%)\n"
	 "next\n"
	 "endproc\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"array_two_real_arg",
	 "local dim a(10)\n"
	 "local dim b(10)\n"
	 "for i% := 0 to 10\n"
	 "  a(i%) = i%\n"
	 "  b(i%) = i% * 2\n"
	 "next\n"
	 "PROCSum(a(), b())\n"
	 "def PROCSum(c(1), d(1))\n"
	 "for i% := 0 to 10\n"
	 "print c(i%) + d(i%)\n"
	 "next\n"
	 "endproc\n",
	 "0\n3\n6\n9\n12\n15\n18\n21\n24\n27\n30\n"
	},
	{"array_real_arg_fifth",
	 "local dim a(10)\n"
	 "for i% := 0 to 10\n"
	 "  a(i%) = i%\n"
	 "next\n"
	 "PROCArr(1,2,3,4,a())\n"
	 "def PROCArr(a%, b%, c%, d%, e(1))\n"
	 "num% := a% + b% + c% + d%\n"
	 "for i% := 0 to 10\n"
	 "    num% += e(i%)\n"
	 "next\n"
	 "print num%\n"
	 "endproc\n",
	 "65\n",
	},
	{"array_global_arg",
	 "dim a%(10)\n"
	 "for i% := 0 to 10\n"
	 "  a%(i%) = i%\n"
	 "next\n"
	 "PROCArr(a%())\n"
	 "def PROCArr(b%(1))\n"
	 "for i% := 0 to 10\n"
	 "    print b%(i%)\n"
	 "next\n"
	 "endproc\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"array_constant_dim",
	 "local dim a%(2,4,6)\n"
	 "print dim(a%())\n"
	 "print dim(a%(), 3)\n",
	 "3\n6\n",
	},
	{"array_dynamic_dim",
	 "local d% = 3\n"
	 "local dim a%(2,4,6)\n"
	 "print dim(a%(), d%)\n",
	 "6\n",
	},
	{"array_dynamic_dim_zero",
	 "onerror\n"
	 "  print err\n"
	 "enderror\n"
	 "local d%\n"
	 "local dim a%(2,4,6)\n"
	 "print dim(a%(), d%)\n",
	 "10\n",
	},
	{"array_bad_dynamic_dim",
	 "onerror\n"
	 "  print err\n"
	 "enderror\n"
	 "d% := 4\n"
	 "local dim a%(2,4,6)\n"
	 "print dim(a%(), d%)\n",
	 "10\n",
	},
	{"array_return_ref_fn",
	 "a() = FNArr(1)()\n"
	 "for i% = 0 to dim(a(),1)\n"
	 "  print a(i%)\n"
	 "next\n"
	 "\n"
	 "def FNArr(1)\n"
	 "  local dim a(10)\n"
	 "  for i% := 0 to 10\n"
	 "    a(i%) = i%\n"
	 "  next\n"
	 "<- a()\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n",
	},
	{"array_assign_ref",
	 "dim a%(10,10)\n"
	 "b%() := a%()\n"
	 "print dim(b%())\n"
	 "print dim(b%(), 1)\n",
	 "2\n10\n",
	},
	{"array_return_fn",
	 "c% := 10\n"
	 "dim a(c%)\n"
	 "a() = FNArr(1)()\n"
	 "for i% = 0 to dim(a(),1)\n"
	 "  print a(i%)\n"
	 "next\n"
	 "\n"
	 "def FNArr(1)\n"
	 "  local dim a(10)\n"
	 "  for i% := 0 to 10\n"
	 "    a(i%) = i%\n"
	 "  next\n"
	 "<- a()\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n",
	},
	{"array_temporaries",
	 "a%() := FNAddArray%(1)(FNFillA%(1)(), FNFillB%(1)())\n"
	 "for i% := 0 to 10\n"
	 "print a%(i%)\n"
	 "next\n"
	 "def FNFillA%(1)\n"
	 "local dim a%(10)\n"
	 "for i% := 0 to 10\n"
	 "a%(i%) = i%\n"
	 "next\n"
	 "<-a%()\n"
	 "def FNFillB%(1)\n"
	 "local dim a%(10)\n"
	 "for i% := 0 to 10\n"
	 "a%(i%) = i% * 2\n"
	 "next\n"
	 "<-a%()\n"
	 "def FNAddArray%(1)(a%(1), b%(1))\n"
	 "local dim c%(10)\n"
	 "for i% := 0 to dim(a%(),1)\n"
	 "c%(i%) = a%(i%) + b%(i%)\n"
	 "next\n"
	 "<-c%()\n",
	 "0\n3\n6\n9\n12\n15\n18\n21\n24\n27\n30\n"
	},
	{"array_assign_local_ref",
	 "dim a%(10,10)\n"
	 "local b%() = a%()\n"
	 "print dim(b%())\n"
	 "print dim(b%(), 1)\n",
	 "2\n10\n",
	},
	{"array_local_return_fn",
	 "c% := 10\n"
	 "local a() = FNArr(1)()\n"
	 "for i% = 0 to dim(a(),1)\n"
	 "  print a(i%)\n"
	 "next\n"
	 "\n"
	 "def FNArr(1)\n"
	 "  local dim a(10)\n"
	 "  for i% := 0 to 10\n"
	 "    a(i%) = i%\n"
	 "  next\n"
	 "<- a()\n",
	 "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n",
	},
	{"array_double_ref",
	  "local dim a%(10)\n"
	  "for i% = 0 to 10\n"
	  "  a%(i%) = i%\n"
	  "next\n"
	  "b%() = a%()\n"
	  "c%() = b%()\n"
	  "for i% = 0 to 10\n"
	  "  print c%(i%)\n"
	  "next\n",
	  "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n",
	},
	{"sgn",
	 "print SGN(11)\n"
	 "print SGN(0)\n"
	 "print SGN(-11)\n"
	 "a% := 11\n"
	 "b% := 0\n"
	 "c% := -111\n"
	 "print(sgn(a%))\n"
	 "print(sgn(b%))\n"
	 "print(sgn(c%))\n"
	 "\n"
	 "print SGN(11.0)\n"
	 "print SGN(0.0)\n"
	 "print SGN(-11.0)\n"
	 "a := 11.0\n"
	 "b := 0.0\n"
	 "c := -111.0\n"
	 "print(sgn(a))\n"
	 "print(sgn(b))\n"
	 "print(sgn(c))\n",
	 "1\n0\n-1\n1\n0\n-1\n1\n0\n-1\n1\n0\n-1\n",
	},
	{"array_dim_mixed",
	 "a% := 3\n"
	 "local dim b%(a%,3)\n"
	 "print b%(1,3)\n",
	 "0\n",
	},
	{"nested_blocks",
	 "if true then\n"
	 "  if true then\n"
	 "    a% := 1\n"
	 "    print a%\n"
	 "  endif\n"
	 "endif\n"
	 "if true then\n"
	 "  if true then\n"
	 "    a% := 1\n"
	 "    print a%\n"
	 "  endif\n"
	 "endif\n",
	 "1\n1\n",
	},
	{"local_initialise_from_local",
	 "q := 512\n"
	 "f := q\n"
	 "f += 1\n"
	 "print q\n",
	 "512\n",
	},
	{"fn_local_array",
	 "PRINT FNLocal%(7)\n"
	 "def FNLocal%(n%)\n"
	 "  local dim c%(10)\n"
	 "<-n%\n",
	 "7\n"
	},
	{
	"dyn_array_1d_too_many_initialisers",
	"a% = 5\n"
	"dim b%(a%)\n"
	"onerror print err enderror\n"
	"b%() = 0, 1, 2, 3, 4, 5, 6\n",
	"10\n"
	},
	{
	"dyn_array_2d_too_many_initialisers",
	"a% = 2\n"
	"b% = 2\n"
	"dim c%(a%, b%)\n"
	"onerror print err enderror\n"
	"c%() = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9\n",
	"10\n"
	},
	{
	"array_1_initialisers",
	"dim a%(4)\n"
	"a%() = 1, 2, 3, 4, 5\n"
	"print a%(0)\n"
	"print a%(1)\n"
	"print a%(2)\n"
	"print a%(3)\n"
	"print a%(4)\n",
	"1\n2\n3\n4\n5\n"
	},
	{
	"array_3d_initialisers",
	"dim a%(1,1,1)\n"
	"a%() = 1, 2, 3, 4, 5, 6, 7, 8\n"
	"print a%(0,0,0)\n"
	"print a%(0,0,1)\n"
	"print a%(0,1,0)\n"
	"print a%(0,1,1)\n"
	"print a%(1,0,0)\n"
	"print a%(1,0,1)\n"
	"print a%(1,1,0)\n"
	"print a%(1,1,1)\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n"
	},
	{
	"dyn_array_1d_initialisers",
	"b% := 10\n"
	"dim a%(b%)\n"
	"a%() = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11\n"
	"for i% := 0 to 10 print a%(i%) next\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n"
	},
	{
	"array_1d_dbl_initialisers",
	"dim a(4)\n"
	"a() = 1, 2, 3, 4, 5\n"
	"print a(0)\n"
	"print a(1)\n"
	"print a(2)\n"
	"print a(3)\n"
	"print a(4)\n",
	"1\n2\n3\n4\n5\n"
	},
	{
	"dyn_array_1d_dbl_initialisers",
	"b% := 10\n"
	"dim a(b%)\n"
	"a() = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11\n"
	"for i% := 0 to 10 print a(i%) next\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n"
	},
	{
	"const_array_3d_initialisers",
	"dim a%(1,4,3)\n"
	"a%() = 1, 2, 3, 4,\n"
	"5, 6, 7, 8,\n"
	"9, 10, 11, 12,\n"
	"13, 14, 15, 16,\n"
	"17, 18, 19, 20,\n"
	"21, 22, 23, 24,\n"
	"25, 26, 27, 28,\n"
	"29, 30, 31, 32,\n"
	"33, 34, 35, 36,\n"
	"27, 38, 39, 40\n"
	"\n"
	"for i% := 0 to dim(a%(),1)\n"
	"  for j% := 0 to dim(a%(),2)\n"
	"    for k% := 0 to dim(a%(),3)\n"
	"      print a%(i%, j%, k%)\n"
	"    next\n"
	"  next\n"
	"next\n",
	"1\n2\n3\n4\n"
	"5\n6\n7\n8\n"
	"9\n10\n11\n12\n"
	"13\n14\n15\n16\n"
	"17\n18\n19\n20\n"
	"21\n22\n23\n24\n"
	"25\n26\n27\n28\n"
	"29\n30\n31\n32\n"
	"33\n34\n35\n36\n"
	"27\n38\n39\n40\n"
	},
	{
	"array_3d_mixed_initialisers",
	"b% := 1\n"
	"dim a%(b%,b%,b%)\n"
	"a%() = 1, 2, 3, 4, 5, 6, 7, 8\n"
	"for i% := 0 to dim(a%(), 1)\n"
	"  for j% := 0 to  dim(a%(), 2)\n"
	"    for k% := 0 to dim(a%(), 3)\n"
	"      print a%(i%, j%, k%)\n"
	"    next\n"
	"  next\n"
	"next\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n"
	},
	{
	"reg_alloc_swi",
	"x% := 640\n"
	"y% := 512\n"
	"a% := 768\n"
	"b% := 768\n"
	"v% := 32\n"
	"w% := 24\n"
	"c% := 16\n"
	"d% := -28\n"
	"dim e%(7)\n"
	"if x% > 1280 or x% < 0 then v% = -v% endif\n"
	"if y% > 1024 or y% < 0 then w% = -w% endif\n"
	"a% += c%\n"
	"b% += d%\n"
	"print x%\n"
	"print y%\n"
	"print a%\n"
	"print b%\n"
	"print v%\n"
	"print w%\n"
	"print c%\n"
	"print d%\n",
	"640\n512\n784\n740\n32\n24\n16\n-28\n",
	},
	{
	"string_basic",
	"print \"hello world\"\n"
	"local a$ = \"from BASIC\"\n"
	"print a$\n"
	"print \"\"\n"
	"c$ := a$\n"
	"print c$\n"
	"a$ = \"new value for a$\"\n"
	"print a$\n"
	"local d$\n"
	"a$ = d$\n"
	"print a$\n"
	"glob$ = \"Hello global\"\n"
	"print glob$\n"
	"glob$ = d$\n"
	"print glob$\n",
	"hello world\nfrom BASIC\n\nfrom BASIC\nnew value for a$\n\n"
	"Hello global\n\n"
	},
	{
	"print_semi_colon",
	"print \"hello\";\n"
	"print 1;\n"
	"print 2\n",
	"hello12\n"
	},
	{"chr$",
	 "print chr$(33)\n"
	 "a% := 33\n"
	 "print chr$(a%)\n"
	 "a = 33\n"
	 "print chr$(a)\n",
	 "!\n!\n!\n"
	},
	{"asc",
	 "print asc(\"01\")\n"
	 "a$ = \"01\"\n"
	 "print asc(a$)\n"
	 "b$ := \"\"\n"
	 "print asc(\"\")\n"
	 "print asc(b$)\n",
	 "48\n48\n-1\n-1\n"
	},
	{"len",
	 "print len(\"Hello world\")\n"
	 "print len(\"\")\n"
	 "a$ = \"01\"\n"
	 "b$ := \"\"\n"
	 "print len(b$)\n"
	 "print len(a$)\n",
	 "11\n0\n0\n2\n"
	},
	{"spc",
	 "print spc(10) \"Hello world\"\n"
	 "a% := 10\n"
	 "print spc(a%) \"Hello world\"\n",
	 "          Hello world\n"
	 "          Hello world\n",
	},
	{"tab_one_arg",
	 "print tab(10) \"Hello world\"\n"
	 "a% := 10\n"
	 "print spc(a%) \"Hello world\"\n",
	 "          Hello world\n"
	 "          Hello world\n",
	},
	{"string_args",
	 "a$ := \"hello\"\n"
	 "PROCmark(a$, \" world\")\n"
	 "DEF PROCmark(b$, c$)\n"
	 "print b$;\n"
	 "print c$\n"
	 "ENDPROC\n",
	 "hello world\n",
	},
	{"fn_string",
	 "print FNHello$;\n"
	 "print FNWorld$\n"
	 "DEF FNHello$\n"
	 "<-\"hello\"\n"
	 "DEF FNWorld$\n"
	 "a$ := \" world\"\n"
	 "<-a$\n",
	 "hello world\n",
	},
	{"string_array",
	 "dim a$(10)\n"
	 "a$() = \"Mark\", \"you\", \"really\", \"are\", \"very\", \"cool\","
	 "\"and\", \"have\", \"written\", \"a\", \"great compiler\"\n"
	 "a$(5) = \"old\"\n"
	 "for i% = 0 to 9\n"
	 "  print a$(i%);\n"
	 "  print \" \";\n"
	 "next\n"
	 "print a$(i%)\n",
	 "Mark you really are very old and have written a great compiler\n"
	},
	{"string_array_fn",
	 "a$() := FNMark$(1)()\n"
	 "dim ab%(1)\n"
	 "for i% = 0 to dim(a$(),1)-1\n"
	 "  print a$(i%);\n"
	 "  print \" \";\n"
	 "next\n"
	 "print a$(i%)\n"
	 "DEF FNMark$(1)\n"
	 "local dim a$(11)\n"
	 "a$() = \"Mark\", \"you\", \"really\", \"are\", \"very\", \"cool\","
	 "\"and\", \"have\", \"\", \"written\", \"a\", \"great compiler\"\n"
	 "a$(5) = \"old\"\n"
	 "<-a$()\n",
	 "Mark you really are very old and have  written a great compiler\n"
	},
	{"string_array_fn2",
	 "a$() := FNHello$(1)()\n"
	 "for i% := 0 to 10\n"
	 "  print a$(i%)\n"
	 "next\n"
	 "DEF FNHello$(1)\n"
	 "    local dim a$(10)\n"
	 "    for i% := 0 to 10\n"
	 "      a$(i%) = \"hello\"\n"
	 "    next\n"
	 "    b$() := a$()\n"
	 "<-b$()\n",
	 "hello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\n"
	 "hello\nhello\n",
	},
	{"string_array_fn3",
	 "a$() = FNHello$(1)()\n"
	 "for i% = 0 to 10\n"
	 "print a$(i%)\n"
	 "next\n"
	 "\n"
	 "def FNHello$(1)\n"
	 "local dim a$(10)\n"
	 "hello$ := \"hello\"\n"
	 "for i% := 0 to 10\n"
	 "a$(i%) = hello$\n"
	 "next\n"
	 "<-a$()\n",
	 "hello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\n"
	 "hello\nhello\n",
	},
	{"string_array_1_el",
	 "dim a$(1)\n"
	 "a$(0) = \"Mark\""
	 "print a$(0)\n",
	 "Mark\n",
	},
	{"related_for_loops",
	 "FOR Y% := 2 TO 4\n"
	 "  FOR X% := Y% TO 4\n"
	 "    PRINT X%;\n"
	 "    PRINT \" \";\n"
	 "    PRINT Y%\n"
	 "  NEXT\n"
	 "NEXT\n",
	 "2 2\n3 2\n4 2\n3 3\n4 3\n4 4\n",
	},
	{"pow",
	 "print 10 ^ -1\n"
	 "print 10 ^ 0\n"
	 "print 10 ^ 2\n"
	 "print 10.0 ^ 0\n"
	 "print 10.0 ^ 2\n"
	 "print 10.0 ^ 2.0\n"
	 "print 10 ^ 2.0\n"
	 "print 10 ^ 1\n"
	 "print 10 ^ 1.0\n"
	 "a% := 10\n"
	 "b% := 0\n"
	 "print a% ^ b%\n"
	 "b% = 2\n"
	 "print a% ^ b%\n"
	 "b% = -1\n"
	 "print a% ^ b%\n"
	 "a% = 16\n"
	 "b = 0.5\n"
	 "print a% ^ b\n"
	 "a = 16.0\n"
	 "print a ^ b\n"
	 "b% = 2\n"
	 "print a ^ b%\n"
	 "print a ^ 0.0\n"
	 "print a ^ 0\n"
	 "print a ^ 1\n"
	 "print a% ^ 1\n",
	 "0.1\n1\n100\n1\n100\n100\n100\n10\n10\n1\n100\n0.1\n4\n4\n256\n1\n1\n16\n16\n",
	},
	{"str_eq",
	 "PRINT \"hello\" = \"hello\""
	 "PRINT \"world\" = \"hello\""
	 "PRINT \"world\" = \"a\""
	 "a$ = \"hello\""
	 "PRINT a$ = \"hello\""
	 "PRINT a$ = \"world\""
	 "PRINT a$ = \"a\""
	 "PRINT \"hello\" = a$"
	 "PRINT \"world\" = a$"
	 "PRINT \"a\" = a$"
	 "b$ = \"world\""
	 "c$ = \"hello\""
	 "d$ = \"a\""
	 "PRINT a$ = a$"
	 "PRINT a$ = \"hell\""
	 "PRINT a$ = b$"
	 "PRINT a$ = c$"
	 "PRINT a$ = d$"
	 "long1$ =\"abcdefghijklmnopqrstuvwxyz\""
	 "long2$ =\"abcdefghijklmnopqrstuvwxyz\""
	 "long3$ =\"aacdefghijklmnopqrstuvwxyz\""
	 "long4$ =\"abcdefghijklmnopprstuvwxyz\""
	 "long5$ =\"abcdefghijklmnopprstuvwxyy\""
	 "PRINT long1$ = long2$"
	 "PRINT long1$ = long3$"
	 "PRINT long1$ = long4$"
	 "PRINT long1$ = long5$",
	 "-1\n0\n0\n-1\n0\n0\n-1\n0\n0\n-1\n0\n0\n-1\n0\n-1\n0\n0\n0\n",
	},
	{"str_neq",
	 "PRINT \"hello\" <> \"hello\""
	 "PRINT \"world\" <> \"hello\""
	 "PRINT \"world\" <> \"a\""
	 "a$ = \"hello\""
	 "PRINT a$ <> \"hello\""
	 "PRINT a$ <> \"world\""
	 "PRINT a$ <> \"a\""
	 "PRINT \"hello\" <> a$"
	 "PRINT \"world\" <> a$"
	 "PRINT \"a\" <> a$"
	 "b$ = \"world\""
	 "c$ = \"hello\""
	 "d$ = \"a\""
	 "PRINT a$ <> a$"
	 "PRINT a$ <> \"hell\""
	 "PRINT a$ <> b$"
	 "PRINT a$ <> c$"
	 "PRINT a$ <> d$"
	 "long1$ =\"abcdefghijklmnopqrstuvwxyz\""
	 "long2$ =\"abcdefghijklmnopqrstuvwxyz\""
	 "long3$ =\"aacdefghijklmnopqrstuvwxyz\""
	 "long4$ =\"abcdefghijklmnopprstuvwxyz\""
	 "long5$ =\"abcdefghijklmnopprstuvwxyy\""
	 "PRINT long1$ <> long2$"
	 "PRINT long1$ <> long3$"
	 "PRINT long1$ <> long4$"
	 "PRINT long1$ <> long5$",
	 "0\n-1\n-1\n0\n-1\n-1\n0\n-1\n-1\n0\n-1\n-1\n0\n-1\n0\n-1\n-1\n-1\n",
	},
	{"str_lt",
	 "print \"hello\" < \"hello\"\n"
	 "print \"world\" < \"hello\"\n"
	 "print \"world\" < \"a\"\n"
	 "print \"a\" < \"world\"\n"
	 "a$ = \"hello\"\n"
	 "print a$ < \"hello\"\n"
	 "print a$ < \"world\"\n"
	 "b$ = \"world\"\n"
	 "print a$ < b$\n"
	 "print b$ < a$\n"
	 "print a$ < a$\n"
	 "long1$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long2$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long3$ =\"aacdefghijklmnopqrstuvwxyz\"\n"
	 "long4$ =\"abcdefghijklmnopqrstuvwxyy\"\n"
	 "print long1$ < long2$\n"
	 "print long1$ < long3$\n"
	 "print long4$ < long1$\n"
	 "print \"\" < \"\"\n"
	 "e$ := \"\"\n"
	 "print e$ < \"\"\n"
	 "print \"\" < e$\n"
	 "f$ := \"\"\n"
	 "print e$ < f$\n"
	 "print \"\" < a$\n",
	 "0\n0\n0\n-1\n0\n-1\n-1\n0\n0\n0\n0\n-1\n0\n0\n0\n0\n-1\n"
	},
	{"str_lte",
	 "print \"hello\" <= \"hello\"\n"
	 "print \"world\" <= \"hello\"\n"
	 "print \"world\" <= \"a\"\n"
	 "print \"a\" <= \"world\"\n"
	 "a$ = \"hello\"\n"
	 "print a$ <= \"hello\"\n"
	 "print a$ <= \"world\"\n"
	 "b$ = \"world\"\n"
	 "print a$ <= b$\n"
	 "print b$ <= a$\n"
	 "print a$ <= a$\n"
	 "long1$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long2$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long3$ =\"aacdefghijklmnopqrstuvwxyz\"\n"
	 "long4$ =\"abcdefghijklmnopqrstuvwxyy\"\n"
	 "print long1$ <= long2$\n"
	 "print long1$ <= long3$\n"
	 "print long4$ <= long1$\n"
	 "print \"\" <= \"\"\n"
	 "e$ := \"\"\n"
	 "print e$ <= \"\"\n"
	 "print \"\" <= e$\n"
	 "f$ := \"\"\n"
	 "print e$ <= f$\n"
	 "print \"\" <= a$\n",
	 "-1\n0\n0\n-1\n-1\n-1\n-1\n0\n-1\n-1\n0\n-1\n-1\n-1\n-1\n-1\n-1\n"
	},
	{"str_gt",
	 "print \"hello\" > \"hello\"\n"
	 "print \"world\" > \"hello\"\n"
	 "print \"world\" > \"a\"\n"
	 "print \"a\" > \"world\"\n"
	 "a$ = \"hello\"\n"
	 "print a$ > \"hello\"\n"
	 "print a$ > \"world\"\n"
	 "b$ = \"world\"\n"
	 "print a$ > b$\n"
	 "print b$ > a$\n"
	 "print a$ > a$\n"
	 "long1$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long2$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long3$ =\"aacdefghijklmnopqrstuvwxyz\"\n"
	 "long4$ =\"abcdefghijklmnopqrstuvwxyy\"\n"
	 "print long1$ > long2$\n"
	 "print long1$ > long3$\n"
	 "print long4$ > long1$\n"
	 "print \"\" > \"\"\n"
	 "e$ := \"\"\n"
	 "print e$ > \"\"\n"
	 "print \"\" > e$\n"
	 "f$ := \"\"\n"
	 "print e$ > f$\n"
	 "print \"\" > a$\n",
	 "0\n-1\n-1\n0\n0\n0\n0\n-1\n0\n0\n-1\n0\n0\n0\n0\n0\n0\n"
	},
	{"str_gte",
	 "print \"hello\" >= \"hello\"\n"
	 "print \"world\" >= \"hello\"\n"
	 "print \"world\" >= \"a\"\n"
	 "print \"a\" >= \"world\"\n"
	 "a$ = \"hello\"\n"
	 "print a$ >= \"hello\"\n"
	 "print a$ >= \"world\"\n"
	 "b$ = \"world\"\n"
	 "print a$ >= b$\n"
	 "print b$ >= a$\n"
	 "print a$ >= a$\n"
	 "long1$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long2$ =\"abcdefghijklmnopqrstuvwxyz\"\n"
	 "long3$ =\"aacdefghijklmnopqrstuvwxyz\"\n"
	 "long4$ =\"abcdefghijklmnopqrstuvwxyy\"\n"
	 "print long1$ >= long2$\n"
	 "print long1$ >= long3$\n"
	 "print long4$ >= long1$\n"
	 "print \"\" >= \"\"\n"
	 "e$ := \"\"\n"
	 "print e$ >= \"\"\n"
	 "print \"\" >= e$\n"
	 "f$ := \"\"\n"
	 "print e$ >= f$\n"
	 "print \"\" >= a$\n",
	 "-1\n-1\n-1\n0\n-1\n0\n0\n-1\n-1\n-1\n-1\n0\n-1\n-1\n-1\n-1\n0\n",
	},
	{"left_str",
	 "a$ = \"hello\"\n"
	 "a% = 2\n"
	 "print left$(a$)\n"
	 "print left$(a$,1)\n"
	 "print left$(a$,a%)\n"
	 "print left$(\"\",10)\n"
	 "print left$(\"hello\",10)\n"
	 "print left$(a$,10)\n"
	 "a%=10\n"
	 "print left$(a$,a%)\n"
	 "\n"
	 "print left$(\"hello\",0)\n"
	 "print left$(a$,0)\n"
	 "a% = 0\n"
	 "print left$(a$,a%)\n"
	 "\n"
	 "print left$(a$,-1)\n"
	 "a%=-1\n"
	 "print left$(a$,a%)\n"
	 "a$ = \"\"\n"
	 "print left$(a$,1)\n"
	 "a% = 0\n"
	 "print left$(\"hello\",a%)\n"
	 "a% = 2\n"
	 "print left$(\"hello\",a%)\n"
	 "print left$(\"hello\",-1)\n"
	 "a%=-1\n"
	 "print left$(\"hello\",a%)\n"
	 "print left$(\"hello\")\n",
	 "h\nh\nhe\n\nhello\nhello\nhello\n\n\n\nhello\nhello\n\n"
	 "\nhe\nhello\nhello\nh\n",
	},
	{"right_str",
	 "print right$(\"hello\")\n"
	 "print right$(\"hello\", 1)\n"
	 "print right$(\"hello\", 3)\n"
	 "print right$(\"hello\", -1)\n"
	 "print right$(\"hello\", 1000)\n"
	 "print right$(\"hello\", 0)\n"
	 "\n"
	 "a% := 1\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = 3\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = -1\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = 1000\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = 0\n"
	 "print right$(\"hello\", a%)\n"
	 "\n"
	 "a$ = \"hello\"\n"
	 "print right$(a$)\n"
	 "print right$(a$, 1)\n"
	 "print right$(a$, 3)\n"
	 "print right$(a$, -1)\n"
	 "print right$(a$, 1000)\n"
	 "print right$(a$, 0)\n"
	 "\n"
	 "a% = 1\n"
	 "print right$(a$, a%)\n"
	 "a% = 3\n"
	 "print right$(a$, a%)\n"
	 "a% = -1\n"
	 "print right$(a$, a%)\n"
	 "a% = 1000\n"
	 "print right$(a$, a%)\n"
	 "a% = 0\n"
	 "print right$(a$, a%)\n",
	 "o\no\nllo\nhello\nhello\n\no\nllo\nhello\nhello\n\n"
	 "o\no\nllo\nhello\nhello\n\no\nllo\nhello\nhello\n\n",
	},
	{"right_str2",
	 "a% := 1\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = 3\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = -1\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = 1000\n"
	 "print right$(\"hello\", a%)\n"
	 "a% = 0\n"
	 "print right$(\"hello\", a%)\n"
	 "\n",
	 "o\nllo\nhello\nhello\n\n"
	},
	{"mid_str",
	 "print mid$(\"hello\", 0)\n"
	 "print mid$(\"hello\", 1,-1)\n"
	 "print mid$(\"hello\", -1)\n"
	 "print mid$(\"hello\", 1,1)\n"
	 "print mid$(\"hello\", 1,4)\n"
	 "print mid$(\"hello\", 1,3)\n"
	 "print mid$(\"hello\", 2,4)\n"
	 "print mid$(\"hello\", 2,10)\n"
	 "print mid$(\"hello\", 0,0)\n"
	 "print mid$(\"hello\", 0,-1)\n"
	 "b% := 0\n"
	 "print mid$(\"hello\", 2, b%)\n"
	 "b% = -1\n"
	 "print mid$(\"hello\", 2, b%)\n"
	 "b% = 1\n"
	 "print mid$(\"hello\", 2, b%)\n"
	 "b% = 2\n"
	 "print mid$(\"hello\", 2, b%)\n"
	 "b% = 10\n"
	 "print mid$(\"hello\", 2, b%)\n",
	 "hello\nhello\n\nh\nhell\nhel\nello\nello\n\nhello\n\nello\n"
	 "e\nel\nello\n",
	},
	{"mid_str2",
	 "a% := 0\n"
	 "print mid$(\"hello\",a%)\n"
	 "a% = 1\n"
	 "print mid$(\"hello\",a%)\n"
	 "a% = 2\n"
	 "print mid$(\"hello\",a%)\n"
	 "a% = 0\n"
	 "print mid$(\"hello\",a%,1)\n"
	 "a% = 1\n"
	 "print mid$(\"hello\",a%, 2)\n"
	 "a% = 2\n"
	 "print mid$(\"hello\",a%, 3)\n"
	 "b% = 0\n"
	 "print mid$(\"hello\",a%,b%)\n"
	 "b% = 1\n"
	 "print mid$(\"hello\",a%,b%)\n"
	 "b% = 3\n"
	 "print mid$(\"hello\",a%,b%)\n"
	 "b% = -1\n"
	 "print mid$(\"hello\",a%,b%)\n"
	 "b% = 100\n"
	 "print mid$(\"hello\",a%,b%)\n",
	 "hello\nhello\nello\nh\nhe\nell\n\ne\nell\nello\nello\n",
	},
	{"mid_str3",
	 "a$ = \"hello\"\n"
	 "a% := 0\n"
	 "print mid$(a$,a%)\n"
	 "a% = 1\n"
	 "print mid$(a$,a%)\n"
	 "a% = 2\n"
	 "print mid$(a$,a%)\n"
	 "a% = 0\n"
	 "print mid$(a$,a%,1)\n"
	 "a% = 1\n"
	 "print mid$(a$,a%, 2)\n"
	 "a% = 2\n"
	 "print mid$(a$,a%, 3)\n"
	 "b% = 0\n"
	 "print mid$(a$,a%,b%)\n"
	 "b% = 1\n"
	 "print mid$(a$,a%,b%)\n"
	 "b% = 3\n"
	 "print mid$(a$,a%,b%)\n"
	 "b% = -1\n"
	 "print mid$(a$,a%,b%)\n"
	 "b% = 100\n"
	 "print mid$(a$,a%,b%)\n"

	 "print mid$(a$, -1,1)\n"
	 "print mid$(a$, 0,1)\n"
	 "print mid$(a$, 1, 2)\n"
	 "print mid$(a$, 2, 3)\n",
	 "hello\nhello\nello\nh\nhe\nell\n\ne\nell\nello\nello\n\nh\nhe\nell\n",
	},
	{"nested_array_ref",
	 "dim a%(10)\n"
	 "a%() = 1, 2, 3\n"
	 "for i% = 0 to dim(a%(),1)\n"
	 "  b%() := a%()\n"
	 "  print b%(i%)\n"
	 "next\n",
	 "1\n2\n3\n0\n0\n0\n0\n0\n0\n0\n0\n",
	},
	{"string_str",
	 "print string$(0,\"aa\")\n"
	 "print string$(-1,\"aa\")\n"
	 "print string$(1,\"aa\")\n"
	 "print string$(4,\"aa\")\n"
	 "a% := 0\n"
	 "print string$(a%,\"aa\")\n"
	 "a% = -1\n"
	 "print string$(a%,\"aa\")\n"
	 "a% = 1\n"
	 "print string$(a%,\"aa\")\n"
	 "a% = 4\n"
	 "print string$(a%,\"aa\")\n"
	 "a$ = \"aa\"\n"
	 "print string$(0, a$)\n"
	 "print string$(-1, a$)\n"
	 "print string$(1, a$)\n"
	 "print string$(4, a$)\n"
	 "a% = 0\n"
	 "print string$(a%, a$)\n"
	 "a% = -1\n"
	 "print string$(a%, a$)\n"
	 "a% = 1\n"
	 "print string$(a%, a$)\n"
	 "a% = 4\n"
	 "print string$(a%, a$)\n"
	 "print string$(4, \"a\")\n"
	 "print string$(a%, \"a\")\n"
	 "a$ = \"a\"\n"
	 "print string$(4, a$)\n"
	 "print string$(a%, a$)\n"
	 "print string$(4.0, a$)\n"
	 "a := 4.0\n"
	 "print string$(a, a$)\n",
	 "\n\naa\naaaaaaaa\n\n\naa\naaaaaaaa\n"
	 "\n\naa\naaaaaaaa\n\n\naa\naaaaaaaa\n"
	 "aaaa\naaaa\naaaa\naaaa\naaaa\naaaa\n",
	},
	{"str_str",
	 "print str$(10)\n"
	 "print str$(-10)\n"
	 "print str$(2147483647)\n"
	 "print str$(&80000000)\n"
	 "print str$(3.1457)\n"
	 "print str$(-3.1457)\n"
	 "a% := 10\n"
	 "print str$(a%)\n"
	 "a% = -10\n"
	 "print str$(a%)\n"
	 "a% = 2147483647\n"
	 "print str$(2147483647)\n"
	 "a% = &80000000\n"
	 "print str$(a%)\n"
	 "a := 3.1457\n"
	 "print str$(a)\n"
	 "a = -3.1457\n"
	 "print str$(a)\n"
	 "print str$~(&ffffffff)\n"
	 "print str$~(4000)\n"
	 "print str$~(4000.6666)\n"
	 "a% = &ffffffff\n"
	 "print str$~(a%)\n"
	 "a% = 4000\n"
	 "print str$~(a%)\n"
	 "a = 4000.6666\n"
	 "print str$~(a)\n",
	 "10\n-10\n2147483647\n-2147483648\n3.1457\n-3.1457\n"
	 "10\n-10\n2147483647\n-2147483648\n3.1457\n-3.1457\n"
	 "FFFFFFFF\nFA0\nFA0\nFFFFFFFF\nFA0\nFA0\n"
	},
	{"str_add_1",
	 "print \"hello\" + \" \" + \"world\"\n"
	 "a$ := \"hello\"\n"
	 "b$ := \"world\"\n"
	 "c$ := \"\"\n"
	 "print a$ + b$\n"
	 "print a$ + c$\n"
	 "print c$ + b$\n"
	 "print \"\" + a$\n"
	 "print b$ + \"\"\n"
	 "print \"hello\" + b$\n"
	 "print a$ + \" \" + \"world\"\n",
	 "hello world\nhelloworld\nhello\nworld\nhello\nworld\n"
	 "helloworld\nhello world\n",
	},
	{"exp",
	 "print exp(1)\n"
	 "a% = 1\n"
	 "print exp(a%)\n"
	 "print exp(0)\n"
	 "a = 0\n"
	 "print exp(a)\n",
	 "2.7182818284\n2.7182818284\n1\n1\n"
	},
	{"heap_buster",
	 "for i% := 1 to 10000\n"
	 "  PROCAlloc\n"
	 "next\n"
	 "\n"
	 "def PROCAlloc\n"
	 "  onerror\n"
	 "  endproc\n"
	 "enderror\n"
	 "\n"
	 "  size% := rnd(64*256)\n"
	 "  local dim ar(size%)\n"
	 "endproc\n",
	 ""},
	{"string_add_equals",
	 "a$ := \"hel\"\n"
	 "a$ += \"lo\"\n"
	 "b$ = a$\n"
	 "a$ += \" world\"\n"
	 "print a$\n"
	 "print b$\n"
	 "\n"
	 "c$ = \"\"\n"
	 "b$ += c$\n"
	 "print b$\n"
	 "\n"
	 "c$ += a$\n"
	 "print c$\n"
	 "\n"
	 "b$ += \"\"\n"
	 "print b$\n"
	 "\n"
	 "a$ += a$\n"
	 "print a$\n",
	 "hello world\nhello\nhello\nhello world\nhello\n"
	 "hello worldhello world\n"
	},
	{
	"string_array_add_equals",
	"dim a$(10)\n"
	"\n"
	"b$ := \"hello\"\n"
	"a$(0) += \" world\"\n"
	"a$(1) = \"hello\"\n"
	"a$(1) += a$(0)\n"
	"a$(0) += \"\"\n"
	"\n"
	"print a$(0)\n"
	"print a$(1)\n",
	" world\nhello world\n"
	},
	{"left_string_statement",
	 "a$ := \"hello mark\"\n"
	 "left$(a$) = \"HE\"\n"
	 "print a$\n"
	 "left$(a$,0) = \"HEL\"\n"
	 "print a$\n"
	 "left$(a$,-1) = \"HEL\"\n"
	 "print a$\n"
	 "left$(a$,PI) = \"he\"\n"
	 "print a$\n"
	 "b$ := \"HE\"\n"
	 "a$ = \"hello mark\"\n"
	 "left$(a$) = b$\n"
	 "print a$\n"
	 "b% := 0\n"
	 "left$(a$,b%) = b$\n"
	 "print a$\n"
	 "b$ = \"HEL\"\n"
	 "b := -1.1\n"
	 "left$(a$,b) = b$\n"
	 "print a$\n"
	 "b% = 3\n"
	 "b$ = \"he\"\n"
	 "left$(a$,b%) = b$\n"
	 "print a$\n"
	 "a$ = \"hello mark\"\n"
	 "c$ = a$\n"
	 "left$(a$,3) = \"HEL\"\n"
	 "print a$\n"
	 "print c$\n"
	 "left$(a$) = \"\"\n"
	 "print a$\n"
	 "a$ = \"\"\n"
	 "left$(a$) = b$\n"
	 "print a$\n",
	 "HEllo mark\nHEllo mark\nHELlo mark\nheLlo mark\n"
	 "HEllo mark\nHEllo mark\nHELlo mark\nheLlo mark\n"
	 "HELlo mark\nhello mark\nHELlo mark\n\n"
	},
	{"right_string_statement",
	 "a$ := \"hello mark\"\n"
	 "right$(a$) = \"RK\"\n"
	 "print a$\n"
	 "\n"
	 "right$(a$,0) = \"ARK\"\n"
	 "print a$\n"
	 "\n"
	 "right$(a$,-1) = \"ARK\"\n"
	 "print a$\n"
	 "\n"
	 "right$(a$,PI) = \"rk\"\n"
	 "print a$\n"
	 "\n"
	 "b$ := \"RK\"\n"
	 "\n"
	 "a$ = \"hello mark\"\n"
	 "right$(a$) = b$\n"
	 "print a$\n"
	 "\n"
	 "b% := 0\n"
	 "right$(a$,b%) = b$\n"
	 "print a$\n"
	 "\n"
	 "b$ = \"ARK\"\n"
	 "b = -1\n"
	 "right$(a$,b) = b$\n"
	 "print a$\n"
	 "\n"
	 "a$ = \"hello mark\"\n"
	 "b% = 3\n"
	 "right$(a$,b%) = b$\n"
	 "print a$\n"
	 "\n"
	 "a$ = \"hello mark\"\n"
	 "c$ = a$\n"
	 "right$(a$,3) = \"ARK\"\n"
	 "print a$\n"
	 "print c$\n",
	 "hello maRK\nhello maRK\nhello mARK\nhello mArk\n"
	 "hello maRK\nhello maRK\nhello mARK\nhello mARK\n"
	 "hello mARK\nhello mark\n"
	},
	{"mid_string_statement",
	 "a$ := \"hello mark\"\n"
	 "mid$(a$,2) = \"LLO\"\n"
	 "print a$\n"
	 "\n"
	 "mid$(a$,0) = \"hello mark\"\n"
	 "print a$\n"
	 "\n"
	 "mid$(a$,1, 6) = \"ELLO MARK\"\n"
	 "print a$\n"
	 "\n"
	 "mid$(a$,1, -1) = \"ELLO MARK\"\n"
	 "print a$\n"
	 "\n"
	 "a$ = \"hello mark\"\n"
	 "b$ := \"LLO\"\n"
	 "b% := 2\n"
	 "mid$(a$, b%) = b$\n"
	 "print a$\n"
	 "\n"
	 "b% = 0\n"
	 "b$ = \"hello mark\"\n"
	 "mid$(a$, b%) = b$\n"
	 "print a$\n"
	 "\n"
	 "b% = 1\n"
	 "c% := 6\n"
	 "b$ = \"ELLO MARK\"\n"
	 "mid$(a$, b%, c%) = b$\n"
	 "print a$\n"
	 "\n"
	 "b% = 1\n"
	 "c% = -1\n"
	 "mid$(a$, b%, c%) = b$\n"
	 "print a$\n"
	 "\n"
	 "d$ := a$\n"
	 "b$ = \"H\"\n"
	 "b% = 0\n"
	 "c% = 100\n"
	 "mid$(a$, b%, c%) = b$\n"
	 "print a$\n"
	 "\n"
	 "a$ = \"hello mark\"\n"
	 "mid$(a$,2.1) = \"LLO\"\n"
	 "print a$\n"
	 "\n"
	 "a$ = \"hello mark\"\n"
	 "\n"
	 "b := 1.1\n"
	 "c := 6.2\n"
	 "b$ = \"ELLO MARK\"\n"
	 "mid$(a$, b, c) = b$\n"
	 "print a$\n"
	 "\n"
	 "mid$(a$,1) = \"\"\n"
	 "print a$\n"
	 "\n"
	 "e$ := \"\"\n"
	 "mid$(a$,1) = e$\n"
	 "print a$\n",
	 "heLLO mark\nhello mark\nhELLO Mark\nhELLO MARK\n"
	 "heLLO mark\nhello mark\nhELLO Mark\nhELLO MARK\n"
	 "HELLO MARK\nheLLO mark\nhELLO Mark\nhELLO Mark\n"
	 "hELLO Mark\n"
	},
	{"min_int32",
	 "a% = -2147483648\n"
	 "print a%\n",
	 "-2147483648\n"
	},
	{"print_hex",
	 "print ~100\n"
	 "a% = 100\n"
	 "print ~a%\n"
	 "a% = 0\n"
	 "print ~a%\n"
	 "print ~15.8\n"
	 "a = 15.8\n"
	 "print ~a\n",
	 "64\n64\n0\nF\nF\n"
	},
	{"good_val",
	 "PROCCheck(val(\"-12.23\"), -12.23)\n"
	 "PROCCheck(val(\"-12.23i\"), -12.23)\n"
	 "print val(\"-12.\")\n"
	 "print val(\"+1s\")\n"
	 "print val(\"3230\")\n"
	 "\n"
	 "a$ = \"-12.23\"\n"
	 "PROCCheck(val(a$), -12.23)\n"
	 "a$ = \"-12.23i\"\n"
	 "PROCCheck(val(a$), -12.23)\n"
	 "a$ = \"-12.\"\n"
	 "b = val(a$)\n"
	 "print b\n"
	 "a$ = \"+1s\"\n"
	 "b = val(a$)\n"
	 "print b\n"
	 "a$ = \"3230\"\n"
	 "b = val(a$)\n"
	 "print b\n"
	 "\n"
	 "DEF PROCCheck(a, e)\n"
	 "LET a = e - a\n"
	 "IF a < 0.0 THEN LET a = -a ENDIF\n"
	 "PRINT a < 0.001\n"
	 "ENDPROC\n",
	 "-1\n-1\n-12\n1\n3230\n-1\n-1\n-12\n1\n3230\n"
	},
	{"val_bad_plus",
	 "onerror\n"
	 "  print err\n"
	 "enderror\n"
	 "a$ := \"30000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "0000000000000000000000000000000000000000000000\"\n"
	 "print val(a$)\n",
	 "20\n",
	},
	{"val_bad_minus",
	 "onerror\n"
	 "  print err\n"
	 "enderror\n"
	 "a$ := \"-30000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "00000000000000000000000000000000000000000000000000000000000000000000"
	 "0000000000000000000000000000000000000000000000\"\n"
	 "print val(a$)\n",
	 "20\n",
	},
	{"int_val",
	 "print val(\"1234\", 10)\n"
	 "a% = 10\n"
	"a$ = \"1234\"\n"
	 "print val(a$, 10)\n"
	 "a$ = \"-1234f\"\n"
	 "print val(a$, 10)\n"
	 "a$ = \"-10000001\"\n"
	 "print val(a$, 2)\n"
	 "a$ = \"-10000\"\n"
	 "print val(a$, 10)\n"
	 "a$ = \"2147483647\"\n"
	 "print val(a$, a%)\n"
	 "a$ = \"-2147483648\"\n"
	 "print val(a$, a%)\n"
	 "\n"
	 "PROCOverflow(\"2147483648\")\n"
	 "PROCOverflow(\"-2147483649\")\n"
	 "\n"
	 "def PROCOverflow(a$)\n"
	 "\n"
	 "onerror\n"
	 "print err\n"
	 "endproc\n"
	 "enderror\n"
	 "\n"
	 "print val(a$,10)\n"
	 "\n"
	 "endproc\n",
	 "1234\n1234\n-1234\n-129\n-10000\n2147483647\n-2147483648\n20\n20\n"
	},
	{"hex_val",
	 "print val(\"1ff\", 16)\n"
	 "a$ = \"1ff\"\n"
	 "print val(a$,16)\n"
	 "a% = 16\n"
	 "print val(a$,a%)\n"
	 "a% = 16\n"
	 "print val(\"1ff?\",a%)\n"
	 "a% = 16\n"
	 "print val(\"7fffffff\",a%)\n"
	 "a% = 16\n"
	 "print val(\"-1ff\",a%)\n"
	 "a% = 16\n"
	 "print val(\"-80000000\",a%)\n"
	 "print val(\"-100\",a%)\n"
	 "\n"
	 "PROCOverflow(\"80000000\")\n"
	 "PROCOverflow(\"-80000001\")\n"
	 "\n"
	 "def PROCOverflow(a$)\n"
	 "\n"
	 "onerror\n"
	 "print err\n"
	 "endproc\n"
	 "enderror\n"
	 "\n"
	 "print val(a$, 16)\n"
	 "\n"
	 "endproc\n",
	"511\n511\n511\n511\n2147483647\n-511\n-2147483648\n-256\n20\n20\n"
	},
	{"val_bad_base",
	 "PROCBadBase(0)\n"
	 "PROCBadBase(1)\n"
	 "PROCBadBase(11)\n"
	 "PROCBadBase(17)\n"
	 "def PROCBadBase(a%)\n"
	 "\n"
	 "onerror\n"
	 "print err\n"
	 "endproc\n"
	 "enderror\n"
	 "\n"
	 "print val(\"1\", a%)\n"
	 "\n"
	 "endproc\n",
	 "31\n31\n31\n31\n"
	},
	{"string_null_assign",
	 "a$ = \"122\"\n"
	 "a$ = \"\"\n"
	 "a$ = \"-2147483648\"\n"
	 "print a$\n",
	 "-2147483648\n"
	},
	{"string_null_init",
	 "a$ = \"\"\n"
	 "a$ = \"hello\"\n"
	 "print a$\n",
	 "hello\n"
	},
	{
	"plot_as_int_array",
	"dim plot%(1)\n"
	"plot%() = 1, 2\n"
	"print plot%(0)\n"
	"print plot%(1)\n",
	"1\n2\n",
	},
	{
	"plot_as_string_array",
	"dim plot$(1)\n"
	"plot$() = \"hello\", \"world\"\n"
	"print plot$(0)\n"
	"print plot$(1)\n",
	"hello\nworld\n",
	},
	{
	"plot_as_int",
	"plot% := 1\n"
	"print plot%\n",
	"1\n",
	},
	{
	"plot_as_string",
	"plot$ := \"hello\"\n"
	"print plot$\n",
	"hello\n",
	},
	{
	"nested_try_exp",
	"print try\n"
	"x% := try\n"
	"error 14\n"
	"endtry\n"
	"print \"failed with error: \";\n"
	"print x%\n"
	"endtry\n"
	"print \"I should be here\"\n",
	"failed with error: 14\n0\nI should be here\n",
	},
	{"handler_after_try",
	"print try\n"
	"endtry\n"
	"onerror\n"
	"print \"in handler\"\n"
	"enderror\n"
	"error 10\n",
	"0\nin handler\n",
	},
	{
	"try_in_onerror",
	"onerror\n"
	"try\n"
	"local dim a%(10)\n"
	"print a%(0)\n"
	"endtry\n"
	"enderror\n"
	"print\"hello\"\n"
	"error 10\n",
	"hello\n0\n"
	},
	{"try_back2back",
	"try\n"
	"print 1\n"
	"error 2\n"
	"endtry\n"
	"try\n"
	"print 3\n"
	"error 4\n"
	"endtry\n",
	"1\n3\n",
	},
	{
	"nested_try_exp_in_proc",
	"PROCTry\n"
	"print \"and here\"\n"
	"def PROCTry\n"
	"print try\n"
	"x% := try\n"
	"error 14\n"
	"endtry\n"
	"print \"failed with error: \";\n"
	"print x%\n"
	"endtry\n"
	"print \"I should be here\"\n"
	"endproc",
	"failed with error: 14\n0\nI should be here\nand here\n",
	},
	{
	"tryone",
	"a% := 0\n"
	"while tryone PROCFail(a%)\n"
	"a% += 1\n"
	"endwhile\n"
	"print \"finished\"\n"
	"def PROCFail(a%)\n"
	"PRINT a%\n"
	"if a% < 5 then\n"
	"      error a%+1\n"
	"endif\n"
	"endproc\n",
	"0\n1\n2\n3\n4\n5\nfinished\n",
	},
	{"bget_bput_eof",
	"PROCWriteFile\n"
	"PROCReadFile\n"
	"\n"
	"def PROCWriteFile\n"
	"  f% := openout(\"markus\")\n"
	"  onerror\n"
	"    tryone close# f%\n"
	"  enderror\n"
	"\n"
	"  print eof#(f%)\n"
	"\n"
	"  a$ := \"hello world!\"\n"
	"\n"
	"  for i% := 1 to len(a$)\n"
	"    bput# f%, asc(mid$(a$,i%,1))\n"
	"  next\n"
	"\n"
	"  onerror error err enderror\n"
	"\n"
	"  close# f%\n"
	"endproc\n"
	"\n"
	"def PROCReadFile\n"
	"  g% := openin(\"markus\")\n"
	"\n"
	"  onerror\n"
	"    tryone close# g%\n"
	"  enderror\n"
	"\n"
	"  print eof#(g%)\n"
	"\n"
	"  b$ := \"\"\n"
	"  local a%\n"
	"  while (tryone a% = bget#(g%)) = 0\n"
	"     b$ += chr$(a%)\n"
	"  endwhile\n"
	"\n"
	"  print eof#(g%)\n"
	"\n"
	"  if not eof#(g%) then\n"
	"    print \"eof error\"\n"
	"     error err\n"
	"  endif\n"
	"\n"
	"  tryone close# g%\n"
	"\n"
	"  print b$\n"
	"endproc\n",
	"0\n0\n-1\nhello world!\n"
	},
	{"ext_test",
	"PROCCreateFile(\"markus\", string$(33, \"!\"))\n"
	"f% = openin(\"markus\")\n"
	"onerror tryone close# f% enderror\n"
	"print ext#(f%)\n"
	"close# f%\n"
	"\n"
	"def PROCCreateFile(name$, data$)\n"
	"  f% := openout(name$)\n"
	"  onerror tryone close# f% enderror\n"
	"  for i% := 1 to len(data$)\n"
	"    bput# f%, asc(mid$(data$,i%,1))\n"
	"  next\n"
	"  close# f%\n"
	"endproc\n",
	"33\n",
	},
	{"ptr_test",
	"PROCCreateFile(\"markus\", string$(16, \"!\") + string$(16,\"@\"))\n"
	"f% = openin(\"markus\")\n"
	"print ptr#(f%)\n"
	"onerror\n"
	"  print \"whoops\"\n"
	"enderror\n"
	"ptr# f%, 15\n"
	"print ptr#(f%)\n"
	"b$ := \"\"\n"
	"local a%\n"
	"while (tryone a% = bget#(f%)) = 0\n"
	"  b$ += chr$(a%)\n"
	"endwhile\n"
	"print b$\n"
	"close# f%\n"
	"def PROCCreateFile(name$, data$)\n"
	"  f% := openout(name$)\n"
	"  onerror tryone close# f% enderror\n"
	"  for i% := 1 to len(data$)\n"
	"    bput# f%, asc(mid$(data$,i%,1))\n"
	"   next\n"
	"   print ptr#(f%)\n"
	"   close# f%\n"
	"endproc\n",
	"32\n0\n15\n!@@@@@@@@@@@@@@@@\n",
	},
	{"int_byte_global",
	"b& = -10\n"
	"a% = 66\n"
	"print a% * b&\n"
	"print 66 * b&\n"
	"print a% + b&\n"
	"print 66 + b&\n"
	"print a% - b&\n"
	"print 66 - b&\n"
	"a% = 64\n"
	"b& = 4\n"
	"print a% << b&\n"
	"print 64 << b&\n"
	"print a% >> b&\n"
	"print 64 >> b&\n"
	"a% = -64\n"
	"print a% >>> b&\n"
	"print -64 >>> b&\n"
	"print a% DIV b&\n"
	"print a% MOD b&\n"
	"b& = -64"
	"c& = 64"
	"print a% = b&\n"
	"print a% = c&\n"
	"print a% <> b&\n"
	"print a% <> c&\n"
	"print -64 = b&\n"
	"print -64 = c&\n"
	"print -64 <> b&\n"
	"print -64 <> c&\n"
	"print a% <= b&\n"
	"print a% < b&\n"
	"print a% > b&\n"
	"print a% >= b&\n"
	"print a% <= c&\n"
	"print a% < c&\n"
	"print a% > c&\n"
	"print a% >= c&\n"
	"print -64 <= b&\n"
	"print -64 < b&\n"
	"print -64 > b&\n"
	"print -64 >= b&\n"
	"print -64 <= c&\n"
	"print -64 < c&\n"
	"print -64 > c&\n"
	"print -64 >= c&\n",
	"-660\n-660\n56\n56\n76\n76\n1024\n1024\n4\n4\n-4\n-4\n-16\n0\n-1\n0\n"
	"0\n-1\n-1\n0\n0\n-1\n-1\n0\n0\n-1\n-1\n-1\n0\n0\n-1\n0\n0\n-1\n-1\n-1\n0\n0\n"
	},
	{"byte_int_local",
	"b% := -10\n"
	"a& := 66\n"
	"print a& * b%\n"
	"print a& + b%\n"
	"print a& - b%\n"
	"a& = 64\n"
	"b% = 4\n"
	"print a& << b%\n"
	"print a& >> b%\n"
	"a& = -64\n"
	"print a& >>> b%\n"
	"print a& DIV b%\n"
	"print a& MOD b%\n"
	"b% = -64"
	"c% = 64"
	"print a& = b%\n"
	"print a& = c%\n"
	"print a& <> b%\n"
	"print a& <> c%\n"
	"print a& <= b%\n"
	"print a& < b%\n"
	"print a& > b%\n"
	"print a& >= b%\n"
	"print a& <= c%\n"
	"print a& < c%\n"
	"print a& > c%\n"
	"print a& >= c%\n",
	"-660\n56\n76\n0\n4\n-4\n-16\n0\n-1\n0\n0\n-1\n-1\n0\n0\n-1\n-1\n-1\n0\n0\n"
	},
	{"byte_byte_local",
	"b& := -64\n"
	"a& := 64\n"
	"c& := 200\n"
	"print b& + a&\n"
	"print c& + a&\n"
	"b& = 3\n"
	"c& = 128\n"
	"print a& * b&\n"
	"print b& * c&\n"
	"b& = 1\n"
	"print a& << b&\n"
	"print a& >> b&\n"
	"print a& >>> b&\n"
	"print (a& << b&) >>> b&\n"
	"print c& div a&\n"
	"b& = 3\n"
	"print a& mod b&\n"
	"a& = -64\n"
	"b& = -64\n"
	"c& = 64\n"
	"print a& = b&\n"
	"print a& = c&\n"
	"print a& <> b&\n"
	"print a& <> c&\n"
	"print a& <= b&\n"
	"print a& < b&\n"
	"print a& > b&\n"
	"print a& >= b&\n"
	"print a& <= c&\n"
	"print a& < c&\n"
	"print a& > c&\n"
	"print a& >= c&\n",
	"0\n8\n-64\n-128\n-128\n32\n32\n-64\n-2\n1\n-1\n0\n0\n-1\n-1\n0\n0\n"
	"-1\n-1\n-1\n0\n0\n",
	},
	{"byte_fn",
	"print FNByteSum&(100, 100)\n"
	"a& = 140\n"
	"b& = 130\n"
	"print FNByteSum&(a&, b&)\n"
	"print FNIntSum%(a&, b&)\n"
	"print FNByteSumDBL&(a&, b&)\n"
	"print FNDBLSum(a&, b&)\n"
	"c = 140\n"
	"d = 130\n"
	"print FNByteSum&(c, d)\n"
	"def FNByteSum&(a&, b&)\n"
	"<- a& + b&\n"
	"def FNIntSum%(a%, b%)\n"
	"<- a% + b%\n"
	"def FNByteSumDBL&(a, b)\n"
	"<- a + b\n"
	"def FNDBLSum(a, b)\n"
	"<- a + b\n",
	"-56\n14\n-242\n14\n-242\n14\n",
	},
	{"byte_array_conversion",
	"DIM a%(1)\n"
	"DIM b(1)\n"
	"byte& = -77\n"
	"a%(0) = byte&\n"
	"b(0) = byte&\n"
	"print a%(0)\n"
	"print b(0)\n",
	"-77\n-77\n",
	},
	{"byte_chr$",
	"a% := 33 + 256\n"
	"b& := a%\n"
	"print chr$(b&)\n",
	"!\n",
	},
	{"intz_byte_int",
	"a& := TRUE\n"
	"print intz(a&)\n",
	"255\n",
	},
	{"array_byte",
	"a&() := FNInita&(1)()\n"
	"print a&(0)\n"
	"print a&(1)\n"
	"a&() = FNUpdatea&(1)(a&())\n"
	"print a&(0)\n"
	"print a&(1)\n"
	"\n"
	"def FNInita&(1)\n"
	"  local dim a&(1)\n"
	"  a&(0) = 127\n"
	"  a&(1) = 128\n"
	"<-a&()\n"
	"\n"
	"def FNUpdatea&(1)(a&(1))\n"
	"  a&(0) += 1\n"
	"  a&(1) += 1\n"
	"<-a&()\n",
	"127\n-128\n-128\n-127\n",
	},
	{"array_byte_init",
	"dim a&(1)\n"
	"a&() = 1, 2\n"
	"print a&(0)\n"
	"print a&(1)\n",
	"1\n2\n",
	},
	{"string_array_fn4",
	"a$() := FNInita$(1)()\n"
	"print a$(0)\n"
	"print a$(1)\n"
	"a$() = FNUpdatea$(1)(a$())\n"
	"print a$(0)\n"
	"print a$(1)\n"
	"\n"
	"def FNInita$(1)\n"
	"  local dim a$(1)\n"
	"  a$(0) = \"hello\"\n"
	"  a$(1) = \"world\"\n"
	"<-a$()\n"
	"\n"
	"def FNUpdatea$(1)(a$(1))\n"
	"  a$(0) += \" !\"\n"
	"  a$(1) += \" *\"\n"
	"<-a$()\n",
	"hello\nworld\nhello !\nworld *\n",
	},
	{"string_array_set",
	"dim a$(9)\n"
	"a$() = \"hello\"\n"
	"for i% := 0 to dim(a$(), 1)\n"
	"print a$(i%)\n"
	"next\n",
	"hello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\n"
	},
	{"int_array_set",
	"dim a%(9)\n"
	"a%() = 255\n"
	"for i% := 0 to dim(a%(), 1)\n"
	"print a%(i%)\n"
	"next\n",
	"255\n255\n255\n255\n255\n255\n255\n255\n255\n255\n"
	},
	{"byte_array_set",
	"dim a&(9)\n"
	"a&() = 255\n"
	"for i% := 0 to dim(a&(), 1)\n"
	"print a&(i%)\n"
	"next\n",
	"-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n",
	},
	{"real_array_set",
	"dim a(9)\n"
	"a() = 255\n"
	"for i% := 0 to dim(a(), 1)\n"
	"print a(i%)\n"
	"next\n",
	"255\n255\n255\n255\n255\n255\n255\n255\n255\n255\n"
	},
	{"string_2darray_set",
	"dim a$(2,2)\n"
	"a$() = \"hello\"\n"
	"for i% := 0 to dim(a$(), 1)\n"
	"  for j% := 0 to dim(a$(), 2)\n"
	"    print a$(i%, j%)\n"
	"  next\n"
	"next\n",
	"hello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\n"
	},
	{"int_2darray_set",
	"dim a%(2,2)\n"
	"a%() = 255\n"
	"for i% := 0 to dim(a%(), 1)\n"
	"  for j% := 0 to dim(a%(), 2)\n"
	"    print a%(i%, j%)\n"
	"  next\n"
	"next\n",
	"255\n255\n255\n255\n255\n255\n255\n255\n255\n"
	},
	{"byte_2darray_set",
	"dim a&(2,2)\n"
	"a&() = 255\n"
	"for i% := 0 to dim(a&(), 1)\n"
	"  for j% := 0 to dim(a&(), 2)\n"
	"    print a&(i%, j%)\n"
	"  next\n"
	"next\n",
	"-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n",
	},
	{"real_2darray_set",
	"dim a(2,2)\n"
	"a() = 255\n"
	"for i% := 0 to dim(a(), 1)\n"
	"  for j% := 0 to dim(a(), 2)\n"
	"    print a(i%, j%)\n"
	"  next\n"
	"next\n",
	"255\n255\n255\n255\n255\n255\n255\n255\n255\n"
	},
	{"string_array_set_var",
	"dim a$(9)\n"
	"s$ = \"hello\"\n"
	"a$() = s$\n"
	"for i% := 0 to dim(a$(), 1)\n"
	"print a$(i%)\n"
	"next\n",
	"hello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\nhello\n"
	},
	{"int_array_set_var",
	"dim a%(9)\n"
	"v% := 255\n"
	"a%() = v%\n"
	"for i% := 0 to dim(a%(), 1)\n"
	"print a%(i%)\n"
	"next\n",
	"255\n255\n255\n255\n255\n255\n255\n255\n255\n255\n"
	},
	{"byte_array_set_var",
	"dim a&(9)\n"
	"v& = 255\n"
	"a&() = v&\n"
	"for i% := 0 to dim(a&(), 1)\n"
	"print a&(i%)\n"
	"next\n",
	"-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n-1\n",
	},
	{"real_array_set_var",
	"dim a(9)\n"
	"v := 255\n"
	"a() = v\n"
	"for i% := 0 to dim(a(), 1)\n"
	"print a(i%)\n"
	"next\n",
	"255\n255\n255\n255\n255\n255\n255\n255\n255\n255\n"
	},
	{"block_get_put_string",
	"PROCCreateFile\n"
	"print FNReadFile$;\n"
	"\n"
	"def PROCCreateFile\n"
	"    a$ := string$(6, \"Hello Mark\" + chr$(10))\n"
	"    f% := openout(\"markus\")\n"
	"    onerror\n"
	"      tryone close# f%\n"
	"    enderror\n"
	"\n"
	"    put# f%, a$\n"
	"\n"
	"    close# f%\n"
	"endproc\n"
	"\n"
	"def FNReadFile$\n"
	"    f% := openin(\"markus\")\n"
	"    onerror\n"
	"      tryone close# f%\n"
	"    enderror\n"
	"\n"
	"    s$ := string$(ext#(f%), \" \")\n"
	"    read% := get#(f%, s$)\n"
	"\n"
	"    tryone close# f%\n"
	"<-s$\n",
	"Hello Mark\nHello Mark\nHello Mark\nHello Mark\nHello Mark\n"
	"Hello Mark\n",
	},
	{"block_get_put_array",
	"PROCCreateFile\n"
	"a%() := FNReadFile%(1)()\n"
	"for i% := 0 to dim(a%(),1)\n"
	"    print a%(i%)\n"
	"next\n"
	"\n"
	"def PROCCreateFile\n"
	"    local dim a%(9)\n"
	"    a%() = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10\n"
	"    f% := openout(\"markus\")\n"
	"    onerror\n"
	"      tryone close# f%\n"
	"    enderror\n"
	"\n"
	"    put# f%, a%()\n"
	"\n"
	"    close# f%\n"
	"endproc\n"
	"\n"
	"def FNReadFile%(1)\n"
	"    f% := openin(\"markus\")\n"
	"    onerror\n"
	"      tryone close# f%\n"
	"    enderror\n"
	"\n"
	"    local dim a%(10)\n"
	"    read% := get#(f%, a%())\n"
	"\n"
	"    tryone close# f%\n"
	"<-a%()\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n0\n",
	},
	{"copy_byte_string",
	"a$ := \"Hello world\"\n"
	"dim b&(7)\n"
	"copy(b&(), a$)\n"
	"for i% := 0 to dim(b&(), 1)\n"
	"  print chr$(b&(i%));\n"
	"next\n"
	"print \"\"\n",
	"Hello wo\n",
	},
	{"copy_byte_int",
	"dim a%(99)\n"
	"a%() = &12345678\n"
	"c% := 9\n"
	"dim b&(c%)\n"
	"copy(b&(), a%())\n"
	"for i% := 0 to dim(b&(),1)\n"
	"  print ~b&(i%)\n"
	"next\n",
	"78\n56\n34\n12\n78\n56\n34\n12\n78\n56\n",
	},
	{"copy_byte_const_string",
	"dim b&(7)\n"
	"copy(b&(), \"Hello world\")\n"
	"for i% := 0 to dim(b&(), 1)\n"
	"  print chr$(b&(i%));\n"
	"next\n"
	"print \"\"\n",
	"Hello wo\n",
	},
	{"copy_byte_empty_string",
	"a$ := \"\"\n"
	"copy(a$, \"Hello\")\n"
	"print a$\n",
	"\n",
	},
	{"copy_string_string_woc",
	"a$ := \"Hello Mark\"\n"
	"b$ := a$\n"
	"copy(a$, \"Seeya\")\n"
	"print a$\n"
	"print b$\n",
	"Seeya Mark\nHello Mark\n",
	},
	{"copy_string_array",
	"a$() = FNCopy$(1)()\n"
	"for i% := 0 to dim(a$(),1)\n"
	"  print a$(i%)\n"
	"next\n"
	"\n"
	"def FNCopy$(1)\n"
	"  local dim a$(9)\n"
	"  a$() = \"Hello\"\n"
	"  local dim b$(6)\n"
	"  b$() =  \"goodbye\"\n"
	"  copy(b$(), a$())\n"
	"<- b$()\n",
	"Hello\nHello\nHello\nHello\nHello\nHello\nHello\n"
	},
	{"copy_string_array_to_empty",
	"a$() = FNCopy$(1)()\n"
	"for i% := 0 to dim(a$(),1)\n"
	"  print a$(i%)\n"
	"next\n"
	"\n"
	"def FNCopy$(1)\n"
	"  local dim a$(9)\n"
	"  a$() = \"Hello\"\n"
	"  local dim b$(6)\n"
	"  copy(b$(), a$())\n"
	"<- b$()\n",
	"Hello\nHello\nHello\nHello\nHello\nHello\nHello\n"
	},
	{"bget_cow",
	"PROCWriteFile\n"
	"f% := openin(\"markus\")\n"
	"onerror\n"
	"  tryone close# f%\n"
	"enderror\n"
	"\n"
	"s$ := string$(ext#(f%), \"!\")\n"
	"b$ := s$\n"
	"read% := get#(f%, s$)\n"
	"print s$\n"
	"print b$\n"
	"close# f%\n"
	"def PROCWriteFile\n"
	"  f% := openout(\"markus\")\n"
	"  onerror\n"
	"    tryone close# f%\n"
	"  enderror\n"
	"\n"
	"  a$ := \"Hello markus\"\n"
	"\n"
	"  put# f%, a$\n"
	"  close# f%\n"
	"endproc\n",
	"Hello markus\n!!!!!!!!!!!!\n"
	},
	{"string_str_char_0_len",
	"a% := 0\n"
	"s$ := string$(a%, \"!\")\n"
	"print s$\n",
	"\n",
	},
	{"zero_len_int_vector",
	"dim a%{-1}\n"
	"\n"
	"PROCEmpty(a%{})\n"
	"\n"
	"b%{} := FNEmpty%{}()\n"
	"print dim(b%{})\n"
	"print dim(b%{},1)\n"
	"\n"
	"def PROCEmpty(a%{})\n"
	"  print dim(a%{})\n"
	"  print dim(a%{},1)\n"
	"endproc\n"
	"\n"
	"def FNEmpty%{}\n"
	"  local dim a%{}\n"
	"<-a%{}\n",
	"1\n-1\n1\n-1\n"},
	{"zero_len_int_vector_assign",
	"dim a%{}\n"
	"b%{} := a%{}\n"
	"print dim(b%{},1)\n",
	"-1\n",
	},
	{"zero_len_string_vector",
	"dim a${-1}\n"
	"\n"
	"PROCEmpty(a${})\n"
	"\n"
	"b${} := FNEmpty${}()\n"
	"print dim(b${})\n"
	"print dim(b${},1)\n"
	"\n"
	"def PROCEmpty(a${})\n"
	"  print dim(a${})\n"
	"  print dim(a${},1)\n"
	"endproc\n"
	"\n"
	"def FNEmpty${}\n"
	"  local dim a${}\n"
	"<-a${}\n",
	"1\n-1\n1\n-1\n"},
	{"zero_len_string_vector_assign",
	"dim a${}\n"
	"b${} := a${}\n"
	"print dim(b${},1)\n",
	"-1\n",
	},
	{"append_int_vector",
	"dim a%{}\n"
	"for i% := 0 to 9\n"
	"  append(a%{}, i%)\n"
	"next\n"
	"append(a%{}, 10)\n"
	"print dim(a%{},1)\n"
	"for i% = 0 to dim(a%{},1)\n"
	"  print a%{i%}\n"
	"next\n",
	"10\n0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n",
	},
	{"append_real_vector",
	"dim a{}\n"
	"for i := 0 to 4.5 step .5\n"
	"  append(a{}, i)\n"
	"next\n"
	"append(a{}, 5.0)\n"
	"print dim(a{},1)\n"
	"for i% = 0 to dim(a{},1)\n"
	"  print a{i%}\n"
	"next\n",
	"10\n0\n0.5\n1\n1.5\n2\n2.5\n3\n3.5\n4\n4.5\n5\n",
	},
	{"append_byte_vector",
	"dim a&{}\n"
	"for i% := 0 to 9\n"
	"  append(a&{}, i%)\n"
	"next\n"
	"append(a&{}, 10)\n"
	"print dim(a&{},1)\n"
	"for i% = 0 to dim(a&{},1)\n"
	"  print a&{i%}\n"
	"next\n",
	"10\n0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n",
	},
	{"append_string",
	"a$ := \"\"\n"
	"for i% := asc(\"0\") to asc(\"9\")\n"
	"  append(a$, chr$(i%))\n"
	"next\n"
	"\n"
	"print a$\n",
	"0123456789\n"
	},
	{"append_string_vector_const",
	"local dim a${}\n"
	"\n"
	"append(a${}, \"hello\")\n"
	"append(a${}, \"world\")\n"
	"for i% = 0 to dim(a${}, 1)\n"
	"  print a${i%}\n"
	"next\n",
	"hello\nworld\n",
	},
	{"append_string_vector",
	"local dim a${}\n"
	"\n"
	"for i% := asc(\"a\") to asc(\"z\")\n"
	"  append(a${}, chr$(i%))\n"
	"next\n"
	"for i% = 0 to dim(a${}, 1)\n"
	"  print a${i%}\n"
	"next\n",
	"a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\nn\no\np\nq\nr\ns\nt\nu\nv\nw\nx\ny\nz\n",
	},
	{"read_empty_file",
	"close# openout(\"markus\")  rem creates an empty file\n"
	"print dim(FNReadFile&{}(\"markus\"), 1)\n"
	"\n"
	"def FNReadFile&{}(a$)\n"
	"  f% := openin(a$)\n"
	"  onerror tryone close# f% enderror\n"
	"  local dim a&{ext#(f%) - 1}\n"
	"  if dim(a&{}, 1) > -1 then\n"
	"      read% := get#(f%, a&{})\n"
	"  endif\n"
	"  close# f%\n"
	"<- a&{}\n",
	"-1\n",
	},
	{"append_vector_array_ints",
	"c% := 7\n"
	"local dim a&{c%}\n"
	"a&{} = 1\n"
	"local dim b&(7)\n"
	"b&() = 2,3,4,5,6,7,8,9\n"
	"c&{} := a&{}\n"
	"append(a&{}, b&())\n"
	"for i% := 0 to dim(a&{},1)\n"
	"  print a&{i%}\n"
	"next\n"
	"for i% = 0 to dim(c&{},1)\n"
	"  print c&{i%}\n"
	"next\n",
	"1\n1\n1\n1\n1\n1\n1\n1\n2\n3\n4\n5\n6\n7\n8\n9\n1\n1\n1\n1\n1\n1\n1\n1\n",
	},
	{"append_vector_array_reals",
	"d% := 7\n"
	"local dim a{d%}\n"
	"a{} = 1\n"
	"local dim b(7)\n"
	"b() = 2,3,4,5,6,7,8,9\n"
	"c{} := a{}\n"
	"append(a{}, b())\n"
	"for i% := 0 to dim(a{},1)\n"
	"  print a{i%}\n"
	"next\n"
	"for i% = 0 to dim(c{},1)\n"
	"  print c{i%}\n"
	"next\n",
	"1\n1\n1\n1\n1\n1\n1\n1\n2\n3\n4\n5\n6\n7\n8\n9\n1\n1\n1\n1\n1\n1\n1\n1\n",
	},
	{"append_vector_array_bytes",
	"local dim a&{7}\n"
	"a&{} = 1\n"
	"local dim b&(2,2)\n"
	"b&() = 2,3,4,5,6,7,8,9,10\n"
	"c&{} := a&{}\n"
	"append(a&{}, b&())\n"
	"for i% := 0 to dim(a&{},1)\n"
	"  print a&{i%}\n"
	"next\n"
	"for i% = 0 to dim(c&{},1)\n"
	"  print c&{i%}\n"
	"next\n",
	"1\n1\n1\n1\n1\n1\n1\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n1\n1\n1\n1\n1\n1\n1\n1\n",
	},
	{"append_vector_array_strings",
	"c% := 4\n"
	"dim a${c%}\n"
	"a${} = \"i\", \"ii\", \"iii\", \"iv\", \"v\"\n"
	"dim b$(4)\n"
	"b$() = \"vi\", \"vii\", \"viii\", \"ix\", \"x\"\n"
	"append(a${}, b$())\n"
	"for i% := 0 to dim(a${}, 1)\n"
	"  print a${i%}\n"
	"next\n",
	"i\nii\niii\niv\nv\nvi\nvii\nviii\nix\nx\n",
	},
	{"append_vector_vector_empty",
	"dim a%{}\n"
	"b% := 5\n"
	"dim c%{b%}\n"
	"c%{} = 100\n"
	"append(c%{}, a%{})\n"
	"print dim(c%{}, 1)\n",
	"5\n",
	},
	{"copy_exp",
	"local dim b&(10)\n"
	"b&() = asc(\"!\")\n"
	"a% := 10\n"
	"print copy(string$(a%, \" \"), b&())\n",
	"!!!!!!!!!!\n",
	},
	{"append_exp",
	"a% := 10\n"
	"print append(string$(a%,\"*\"), string$(10,\"!\"))\n",
	"**********!!!!!!!!!!\n",
	},
	{"get_hash_len_test",
	"PROCWriteFile(\"markus\")\n"
	"f% := openin(\"markus\")\n"
	"local dim a%(10)\n"
	"print get#(f%, a%())\n"
	"close# f%\n"
	"\n"
	"def PROCWriteFile(a$)\n"
	"  local dim a%(9)\n"
	"  a%() = 1,2,3,4,5,6,7,8,9,10\n"
	"  f% := openout(a$)\n"
	"  onerror\n"
	"    tryone close# f%\n"
	"  enderror\n"
	"  put# f%, a%()\n"
	"  close# f%\n"
	"endproc\n",
	"10\n",
	},
	{"memset_vector",
	"local dim b&{10}\n"
	"b&{} = 66\n"
	"for i% := 0 to dim(b&{},1)\n"
	"print b&{i%}\n"
	"next\n",
	"66\n66\n66\n66\n66\n66\n66\n66\n66\n66\n66\n",
	},
	{"vector_bad_init",
	"onerror print err enderror\n"
	"dim a%{}\n"
	"a%{} = 1,1\n",
	"10\n",
	},
	{"memset_empty_vector",
	"dim a%{}\n"
	"a%{} = 1\n"
	"dim b{}\n"
	"b{} = 1\n"
	"dim c&{}\n"
	"c&{} = 1\n"
	"dim d${}\n"
	"d${} = \"1\"\n"
	"print dim(a%{}, 1)"
	"print dim(b{}, 1)"
	"print dim(c&{}, 1)"
	"print dim(d${}, 1)",
	"-1\n-1\n-1\n-1\n",
	},
	{"vector_ref_test",
	"local dim a%{4}\n"
	"local b%{} = a%{}\n"
	"c%{} := a%{}\n"
	"a%{} = 1, 2, 3\n"
	"for i% := 0 to dim(c%{},1)\n"
	"print c%{i%}\n"
	"next\n",
	"1\n2\n3\n0\n0\n"
	},
	{"vector_copy",
	"dim a%{10}\n"
	"dim b%(11)\n"
	"dim c%{10}\n"
	"c%{} = 11\n"
	"b%() = 10\n"
	"copy(a%{}, b%())\n"
	"for i% := 0 to dim(a%{}, 1)\n"
	"print a%{i%}\n"
	"next\n"
	"copy(a%{}, c%{})\n"
	"for i% = 0 to dim(a%{}, 1)\n"
	"print a%{i%}\n"
	"next\n",
	"10\n10\n10\n10\n10\n10\n10\n10\n10\n10\n10\n"
	"11\n11\n11\n11\n11\n11\n11\n11\n11\n11\n11\n",
	},
	{"vector_as_param",
	"dim a%{}\n"
	"PROCPassVector(a%{})\n"
	"def PROCPassVector(a%{})\n"
	"print dim(a%{}, 1)\n"
	"endproc\n",
	"-1\n",
	},
	{"vector_as_func",
	"local a%{} = FNMakeVector%{}\n"
	"for i% := 0 to dim(a%{}, 1)\n"
	"print a%{i%}\n"
	"next\n"
	"def FNMakeVector%{}\n"
	"local dim a%{9}\n"
	"a%{} = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9\n"
	"<-a%{}\n",
	"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n",
	},
	{"negative_array_dim",
	"onerror print err enderror\n"
	"b% := -1\n"
	"dim a%(b%)\n",
	"10\n",
	},
	{"range_int_local_new",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"range b% := a%()\n"
	"    print b%\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n",
	},
	{"range_int_local",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"local b%\n"
	"range b% = a%()\n"
	"    print b%\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n",
	},
	{"range_int_global_new",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"range b% = a%()\n"
	"    print b%\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n",
	},
	{"range_int_global",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"b% = 0\n"
	"range b% = a%()\n"
	"    print b%\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n",
	},
	{"range_string_global_new",
	"dim a$(5)\n"
	"a$() = \"hello\", \"world\", \"will\", \"this\", \"work\", \"yes\"\n"
	"range b$ = a$()\n"
	"    print b$\n"
	"endrange\n",
	"hello\nworld\nwill\nthis\nwork\nyes\n",
	},
	{"range_string_global",
	"dim a$(5)\n"
	"a$() = \"hello\", \"world\", \"will\", \"this\", \"work\", \"yes\"\n"
	"b$ = \"\""
	"range b$ = a$()\n"
	"    print b$\n"
	"endrange\n",
	"hello\nworld\nwill\nthis\nwork\nyes\n",
	},
	{"range_string_local_new",
	"dim a$(5)\n"
	"a$() = \"hello\", \"world\", \"will\", \"this\", \"work\", \"yes\"\n"
	"range b$ := a$()\n"
	"    print b$\n"
	"endrange\n",
	"hello\nworld\nwill\nthis\nwork\nyes\n",
	},
	{"range_string_local",
	"dim a$(5)\n"
	"a$() = \"hello\", \"world\", \"will\", \"this\", \"work\", \"yes\"\n"
	"b$ := \"\"\n"
	"range b$ = a$()\n"
	"    print b$\n"
	"endrange\n",
	"hello\nworld\nwill\nthis\nwork\nyes\n",
	},
	{"range_byte",
	"dim a&(5)\n"
	"a&() = 1,2,3,4,5,6\n"
	"range b& := a&()\n"
	"print b&\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n"
	},
	{"range_real",
	"dim a(5)\n"
	"a() = 1,2,3,4,5,6\n"
	"range b := a()\n"
	"print b\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n"
	},
	{"range_vector",
	"dim a{5}\n"
	"a{} = 1,2,3,4,5,6\n"
	"range b := a{}\n"
	"print b\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n"
	},
	{"range_2d_array",
	"dim a%(1,2)\n"
	"a%() = 1,2,3,4,5,6\n"
	"range b% := a%()\n"
	"  print b%\n"
	"endrange\n",
	"1\n2\n3\n4\n5\n6\n"
	},
	{"range_1d_array_var",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"local b%\n"
	"range b%, c% = a%()\n"
	"  print b%;\n"
	"  print \", \";\n"
	"  print c%\n"
	"endrange\n",
	"1, 0\n2, 1\n3, 2\n4, 3\n5, 4\n6, 5\n",
	},
	{"range_1d_array_var_global",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"range b%, c% = a%()\n"
	"  print b%;\n"
	"  print \", \";\n"
	"  print c%\n"
	"endrange\n"
	"print b%\n"
	"print c%\n",
	"1, 0\n2, 1\n3, 2\n4, 3\n5, 4\n6, 5\n6\n6\n",
	},
	{"range_1d_array_var_local",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"range local b%, c% = a%()\n"
	"  print b%;\n"
	"  print \", \";\n"
	"  print c%\n"
	"endrange\n",
	"1, 0\n2, 1\n3, 2\n4, 3\n5, 4\n6, 5\n",
	},
	{"range_1d_array_var_local_index",
	"dim a%(5)\n"
	"a%() = 1,2,3,4,5,6\n"
	"local c%\n"
	"range b%, c% = a%()\n"
	"  print b%;\n"
	"  print \", \";\n"
	"  print c%\n"
	"endrange\n",
	"1, 0\n2, 1\n3, 2\n4, 3\n5, 4\n6, 5\n",
	},
	{"range_var_vector",
	"dim a%{5}\n"
	"a%{} = 1,2,3,4,5,6\n"
	"local b%\n"
	"range b%, c% = a%{}\n"
	"  print b%;\n"
	"  print \", \";\n"
	"  print c%\n"
	"endrange\n",
	"1, 0\n2, 1\n3, 2\n4, 3\n5, 4\n6, 5\n",
	},
	{"range_3d_string_array",
	"dim a$(1,1,1)\n"
	"a$() = \"her\", \"beauty\", \"fairly\", \"took\", \"my\", \"breath\", \"away\", \".\"\n"
	"range b$, c%, d%, e% := a$()\n"
	"  print b$;\n"
	"  print \" ( \";\n"
	"  print c%;\n"
	"  print \", \";\n"
	"  print d%;\n"
	"  print \", \";\n"
	"  print e%;\n"
	"  print \")\"\n"
	"endrange\n",
	"her ( 0, 0, 0)\n"
	"beauty ( 0, 0, 1)\n"
	"fairly ( 0, 1, 0)\n"
	"took ( 0, 1, 1)\n"
	"my ( 1, 0, 0)\n"
	"breath ( 1, 0, 1)\n"
	"away ( 1, 1, 0)\n"
	". ( 1, 1, 1)\n",
	},
	{"for_vector_var",
	"dim a%{0}\n"
	"for a%{0} = 0 to 4\n"
	"  print a%{0}\n"
	"next\n"
	"print a%{0}\n",
	"0\n1\n2\n3\n4\n5\n",
	},
	{"fn_names",
	"def FNa <-0.0\n"
	"def FNa& <- 2\n"
	"def FNa% <-1\n"
	"def FNa$ <- \"hello\"\n"
	"def FNa%(1) local dim a%(10) <- a%()\n"
	"def FNa%{} local dim a%{4} <-a%{}\n"
	"print FNa\n"
	"print FNa&\n"
	"print FNa%\n"
	"print FNa$\n"
	"print dim(FNa%(1)(),1)\n"
	"print dim(FNa%{}, 1)\n",
	"0\n2\n1\nhello\n10\n4\n",
	},
	{"lambda_basic",
	"type PROCMap(a%)\n"
	"type PROCNoArgs\n"
	"type FNInt%\n"
	"type FNStr$\n"
	"type FNReal\n"
	"type FNByte&\n"
	"e@PROCMap := def PROC(a%)\n"
	"  print a%\n"
	"endproc\n"
	"f@PROCNoArgs := def PROC\n"
	"  print \"no args\"\n"
	"endproc\n"
	"i@FNInt := def FN% <- 1\n"
	"j@FNStr := def FN$ <- \"Hello World\"\n"
	"k@FNReal := def FN <- 10.0\n"
	"l@FNByte := def FN& <- 127\n"
	"e@PROCMap(10.1)\n"
	"f@PROCNoArgs()\n"
	"print i@FNInt()\n"
	"print j@FNStr()\n"
	"print k@FNReal()\n"
	"print l@FNByte()\n",
	"10\nno args\n1\nHello World\n10\n127\n",
	},
	{"lambda_string_array_fn",
	"type FN_getStrings$(1)(len%)\n"
	"a@FN_getStrings = def FN$(1)(len%)\n"
	"  local dim a$(len%)\n"
	"   for i% := 0 to dim (a$(),1)\n"
	"    a$(i%) = str$(i%)\n"
	"   next\n"
	"<-a$()\n"
	"strs$() := a@FN_getStrings(10)\n"
	"range a$ := strs$()\n"
	"  print a$\n"
	"endrange\n",
	"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n",
	},
	{"lambda_alias",
	"type FNByte&\n"
	"l@FNByte = def FN& <- 127\n"
	"print l@FNByte()\n"
	"g@FNByte := l@FNByte\n"
	"h@FNByte = l@FNByte\n"
	"print g@FNByte()\n"
	"print h@FNByte()\n",
	"127\n127\n127\n",
	},
	{"lambda_nested1",
	"type PROCEmpty\n"
	"for i% = 0 to 9\n"
	"  a@PROCEmpty := def PROC print \"empty\" endproc\n"
	"  a@PROCEmpty()\n"
	"next\n",
	"empty\nempty\nempty\nempty\nempty\nempty\nempty\nempty\nempty\nempty\n",
	},
	{"lambda_in_proc",
	"type FNHello$\n"
	"def PROCPrint\n"
	"  a@FNHello := def FN$ <- \"HELLO\"\n"
	"  print a@FNHello()\n"
	"endproc\n"
	"PROCPrint\n",
	"HELLO\n",
	},
	{"lambda_in_error_handler",
	"type PROCEmpty\n"
	"onerror\n"
	"  a@PROCEmpty := def PROC print \"empty\" endproc\n"
	"  tryone a@PROCEmpty()\n"
	"enderror\n"
	"error 1\n",
	"empty\n",
	},
	{"call_addr_local",
	"type PROCEmpty\n"
	"local a@PROCEmpty = !PROCBoring\n"
	"a@PROCEmpty()\n"
	"def PROCBoring\n"
	"  print \"boring\"\n"
	"endproc\n",
	"boring\n"
	},
	{"call_addr_global",
	"type PROCEmpty\n"
	"a@PROCEmpty = !PROCBoring\n"
	"a@PROCEmpty()\n"
	"def PROCBoring\n"
	"  print \"boring\"\n"
	"endproc\n",
	"boring\n"
	},
	{"call_addr_fn",
	"type FNStr$\n"
	"local a@FNStr = !FNgetStr$\n"
	"print a@FNStr()\n"
	"def FNgetStr$\n"
	"<-\"hello\"\n",
	"hello\n",
	},
	{"call_addr_fn_params",
	"type FNStr$(a$, b%)\n"
	"local a@FNStr = !FNgetStr$\n"
	"print a@FNStr(\"hello\", 3)\n"
	"def FNgetStr$(a$, b%)\n"
	"<-string$(b%, a$)\n",
	"hellohellohello\n",
	},
	{"call_addr_on_error",
	"type PROCEmpty\n"
	"onerror\n"
	"local a@PROCEmpty = !PROCBoring\n"
	"tryone a@PROCEmpty()\n"
	"enderror\n"
	"error 1\n"
	"def PROCBoring\n"
	"  print \"boring\"\n"
	"endproc\n",
	"boring\n",
	},
	{"dim_fn_init1",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = !FNCube%, def FN%(a%) <- a% * 2, def FN%(a%) <- a% * a%\n"
	"range c@FNMap := a@FNMap()\n"
	"print c@FNMap(10)\n"
	"endrange\n"
	"def FNCube%(a%)\n"
	"<- a% * a% * a%\n",
	"1000\n20\n100\n",
	},
	{"dim_fn_init2",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = def FN%(a%) <- a% * 2, def FN%(a%) <- a% * a%,!FNCube%\n"
	"range c@FNMap := a@FNMap()\n"
	"print c@FNMap(10)\n"
	"endrange\n"
	"def FNCube%(a%)\n"
	"<- a% * a% * a%\n",
	"20\n100\n1000\n",
	},
	{"dim_fn_init3",
	"type FNMap%(a%)\n"
	"dim a@FNMap(3)\n"
	"a@FNMap() = def FN%(a%) <- a% * 2, def FN%(a%) <- a% * a%,!FNCube%\n"
	"range c@FNMap := a@FNMap()\n"
	"print c@FNMap(10)\n"
	"endrange\n"
	"def FNCube%(a%)\n"
	"<- a% * a% * a%\n",
	"20\n100\n1000\n0\n",
	},
	{"dim_fn_set1",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = def FN%(a%) <- a% * 2\n"
	"range c@FNMap := a@FNMap()\n"
	"print c@FNMap(10)\n"
	"endrange\n",
	"20\n20\n20\n",
	},
	{"dim_fn_set2",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = !FNCube%\n"
	"range c@FNMap := a@FNMap()\n"
	"print c@FNMap(10)\n"
	"endrange\n"
	"def FNCube%(a%)\n"
	"<- a% * a% * a%\n",
	"1000\n1000\n1000\n",
	},
	{"dim_fn_zero",
	"type FNp&\n"
	"type PROCdoer\n"
	"type FNi%\n"
	"type FNr\n"
	"local l@FNp\n"
	"local m@FNp\n"
	"local o@FNi\n"
	"local p@FNr\n"
	"local p@PROCdoer\n"
	"print l@FNp()\n"
	"print m@FNp()\n"
	"print o@FNi()\n"
	"print p@FNr()\n"
	"p@PROCdoer()\n",
	"0\n0\n0\n0\n",
	},
	{"dim_fn_zero_args",
	"type FNp&(a%)\n"
	"type PROCdoer(a$)\n"
	"type FNi%(a)\n"
	"type FNr(a&)\n"
	"local l@FNp\n"
	"local m@FNp\n"
	"local o@FNi\n"
	"local p@FNr\n"
	"local p@PROCdoer\n"
	"print l@FNp(10)\n"
	"print m@FNp(11)\n"
	"print o@FNi(3.14)\n"
	"print p@FNr(&ff)\n"
	"p@PROCdoer(\"hello\")\n",
	"0\n0\n0\n0\n",
	},
	{"dim_fn_zero_ref",
	"type FNStr$(a$)\n"
	"type FNia%(2)\n"
	"type FNba&(1)\n"
	"type FNra(1)\n"
	"type FNhello$(1)(a$)\n"
	"type FNAr${}\n"
	"type FNiv%{}\n"
	"type FNbv&{}\n"
	"type FNrv{}\n"
	"local m@FNra\n"
	"local n@FNba\n"
	"local o@FNia\n"
	"local p@FNStr\n"
	"local q@FNhello\n"
	"local r@FNAr\n"
	"local s@FNiv\n"
	"local t@FNbv\n"
	"local u@FNrv\n"
	"print len(p@FNStr(\"hI\"))\n"
	"print dim(m@FNra(),1)\n"
	"print dim(n@FNba(),1)\n"
	"print dim(o@FNia(),1)\n"
	"print dim(q@FNhello(\"hI\"),1)\n"
	"print dim(r@FNAr(),1)\n"
	"print dim(s@FNiv(),1)\n"
	"print dim(t@FNbv(),1)\n"
	"print dim(u@FNrv(),1)\n",
	"0\n1\n1\n1\n1\n-1\n-1\n-1\n-1\n",
	},
	{"fn_map",
	"dim a{10}\n"
	"\n"
	"range v, i% := a{}\n"
	"  a{i%} = i%\n"
	"endrange\n"
	"\n"
	"type FNMapper(a)\n"
	"\n"
	"PROC_map(a{}, def FN(a) <- a*a)\n"
	"\n"
	"range v, i% := a{}\n"
	"  print v;\n"
	"  if i% < dim(a{}, 1) then print \" \"; endif\n"
	"endrange\n"
	"print \"\"\n"
	"\n"
	"def PROC_map(a{}, f@FNMapper)\n"
	"  range v, i% := a{}\n"
	"    a{i%} = f@FNMapper(v)\n"
	"  endrange\n"
	"endproc\n",
	"0 1 4 9 16 25 36 49 64 81 100\n",
	},
	{"fn_ret_fn",
	"type PROCMaker(a%)\n"
	"\n"
	"def FNMakeMaker@PROCMaker\n"
	"<- def PROC(a%)\n"
	"    for i% := 1 TO a%\n"
	"       print \"maker\"\n"
	"    next\n"
	"  endproc\n"
	"a@PROCMaker = FNMakeMaker@PROCMaker()\n"
	"a@PROCMaker(4)\n",
	"maker\nmaker\nmaker\nmaker\n",
	},
	{"fn_ret_ar_fn",
	"type PROCMaker(a%)\n"
	"\n"
	"def FNMakeMaker@PROCMaker(1)\n"
	"  local dim a@PROCMaker(2)\n"
	"  a@PROCMaker() = def PROC(a%) PROCLooper(a%, \"one\") endproc,\n"
	"    def PROC(a%) PROCLooper(a%, \"two\") endproc,\n"
	"    def PROC(a%) PROCLooper(a%, \"three\") endproc\n"
	"<- a@PROCMaker()\n"
	"\n"
	"def PROCLooper(a%, b$)\n"
	"  for i% := 1 TO a%\n"
	"     print b$\n"
	"  next\n"
	"endproc\n"
	"\n"
	"a@PROCMaker() := FNMakeMaker@PROCMaker(1)()\n"
	"print \"array returned with \";\n"
	"print dim(a@PROCMaker(),1 );\n"
	"print \" elements\"\n"
	"range fn@PROCMaker, i% := a@PROCMaker()\n"
	"   fn@PROCMaker(i% + 1)\n"
	"endrange\n",
	"array returned with 2 elements\none\ntwo\ntwo\nthree\nthree\nthree\n"
	},
	{"assign_fn_array",
	"type PROCArgs(a%, b%)\n"
	"dim a@PROCArgs(1)\n"
	"a@PROCArgs() = !PROCOne\n"
	"a@PROCArgs(1) = !PROCTwo\n"
	"range v@PROCArgs := a@PROCArgs()\n"
	"  v@PROCArgs(1,2)\n"
	"endrange\n"
	"dim vec@PROCArgs{1}\n"
	"vec@PROCArgs{} = !PROCOne\n"
	"vec@PROCArgs{1} = !PROCTwo\n"
	"append(vec@PROCArgs{}, !PROCThree)\n"
	"range v@PROCArgs := vec@PROCArgs{}\n"
	"  v@PROCArgs(1,2)\n"
	"endrange\n"
	"def PROCOne(a%, b%) print \"one\" endproc\n"
	"def PROCTwo(a%, b%) print \"two\" endproc\n"
	"def PROCThree(a%, b%) print \"three\" endproc\n",
	"one\ntwo\none\ntwo\nthree\n",
	},
	{"recursive_type",
	"type PROC_dummy\n"
	"type FNfn@PROC_dummy(a%)\n"
	"\n"
	"local a@FNfn\n"
	"local n@PROC_dummy\n"
	"\n"
	"a@FNfn = def FN@PROC_dummy(a%) <- def PROC print \"dummy\" endproc\n"
	"n@PROC_dummy = a@FNfn(10)\n"
	"n@PROC_dummy()\n",
	"dummy\n",
	},
	{"recursive_type2",
	"type PROC_dummy(a%)\n"
	"type PROChigher(a%, b@PROC_dummy, c@PROC_dummy)\n"
	"\n"
	"local a@PROChigher = def PROC(a%, b@PROC_dummy, c@PROC_dummy)\n"
	"    b@PROC_dummy(a%)\n"
	"    c@PROC_dummy(a%)\n"
	"endproc\n"
	"\n"
	"local b@PROC_dummy = !PROCup\n"
	"local c@PROC_dummy = !PROCdown\n"
	"a@PROChigher(5, b@PROC_dummy, c@PROC_dummy)\n"
	"a@PROChigher(5, !PROCup, !PROCdown)\n"
	"\n"
	"def PROCup(a%)\n"
	"  for i% := 1 to a% print i% next\n"
	"endproc\n"
	"\n"
	"def PROCdown(a%)\n"
	"  for i% := a% to 1 step -1 print i% next\n"
	"endproc\n",
	"1\n2\n3\n4\n5\n5\n4\n3\n2\n1\n1\n2\n3\n4\n5\n5\n4\n3\n2\n1\n"
	},
	{"dim_fn_print",
	"type FNMap%(a%)\n"
	"dim a@FNMap(1)\n"
	"a@FNMap(0) = def FN%(a%) <- a% * 2\n"
	"a@FNMap(1) = def FN%(a%) <- a% * a%\n"
	"print a@FNMap(0)(10)\n"
	"print a@FNMap(1)(10)\n"
	"range c@FNMap := a@FNMap()\n"
	"print c@FNMap(10)\n"
	"endrange\n",
	"20\n100\n20\n100\n",
	},
	{"function_ptr_param",
	"type FNMap$(a$)\n"
	"PROCMapper(\"hello\", !FNstr$)\n"
	"def FNstr$(a$) <- a$ + a$\n"
	"def PROCMapper(s$, a@FNMap)\n"
	"  print a@FNMap(s$)\n"
	"endproc\n",
	"hellohello\n",
	},
	{"mid_str_cow",
	"a$ := \"hello mark\"\n"
	"b$ := mid$(a$, 4, 4)\n"
	"left$(b$,1) = \"O\"\n"
	"print a$\n"
	"print b$\n",
	"hello mark\nOo m\n",
	},
	{"fn_zero_dim",
	"type FNMap%(a%)\n"
	"dim a@FNMap(2)\n"
	"a@FNMap() = def FN%(a%) <- a% * 2, def FN%(a%) <- a% * 3\n"
	"range c@FNMap := a@FNMap()\n"
	"  print c@FNMap(10)\n"
	"endrange\n",
	"20\n30\n0\n",
	},
	{"vector_slice_int",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"range v% := a%{1 : 3}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%{1 : 3}, 1)\n",
	"2\n3\n1\n",
	},
	{"vector_slice_const_var",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b% := 1\n"
	"c% := 3\n"
	"range v% := a%{1 : c%}\n"
	"  print v%\n"
	"endrange\n"
	"range v% := a%{b% : 3}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%{1 : c%})\n"
	"print dim(a%{b% : 3})\n",
	"2\n3\n2\n3\n1\n1\n",
	},
	{"vector_slice_const_whole",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"range v% := a%{0 : 11}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%{0 : 11}, 1)\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n10\n",
	},
	{"vector_slice_var_whole",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b% := 0\n"
	"range v% := a%{b% : dim(a%{}, 1)+1}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%{b% : dim(a%{}, 1)+1}, 1)\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n10\n",
	},
	{"vector_slice_reference",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"range v% := a%{:}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%{0:}, 1)\n",
	"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n10\n",
	},
	{"vector_slice_partial",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"range v% := a%{8 : }\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%{8 : }, 1)\n",
	"9\n10\n11\n2\n",
	},
	{"vector_slice_empty_const",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b%{} := a%{1 : 1}\n"
	"range v% := b%{}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(b%{}, 1)\n",
	"-1\n",
	},
	{"vector_slice_empty_var",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"d% := 1\n"
	"b%{} := a%{d% : d%}\n"
	"range v% := b%{}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(b%{}, 1)\n",
	"-1\n",
	},
	{"vector_slice_int_var",
	"dim a%{10}\n"
	"a%{} = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b% := 1\n"
	"c% := 3\n"
	"range v% := a%{b% : c%}\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%{b% : c%}, 1)\n",
	"2\n3\n1\n",
	},
	{"vector_slice_string",
	"dim a${5}\n"
	"a${} = \"one\", \"two\", \"three\", \"four\", \"five\"\n"
	"range s$ := a${1 : 3}\n"
	"  print s$\n"
	"endrange\n",
	"two\nthree\n",
	},
	{"vector_slice_return",
	"a${} := FNSlice${}()\n"
	"range x$ := a${}\n"
	"  print x$\n"
	"endrange\n"
	"def FNSlice${}\n"
	"  local dim a${2}\n"
	"a${} = \"one\", \"two\", \"three\"\n"
	"<-a${0:1}\n",
	"one\n",
	},
	{"slice_append",
	"dim a%{5}\n"
	"a%{} = 1,2,3,4,5,6\n"
	"b%{} := a%{1 : 3}\n"
	"b%{0} = 100\n"
	"b%{1} = 101\n"
	"range v% := a%{}\n"
	"  print v%\n"
	"endrange\n"
	"print \"\"\n"
	"range v% := b%{}\n"
	"print v%\n"
	"endrange\n"
	"print \"\"\n"
	"append(b%{}, 102)\n"
	"range v% := a%{}\n"
	"  print v%\n"
	"endrange\n"
	"print \"\"\n"
	"range v% := b%{}\n"
	"  print v%\n"
	"endrange\n",
	"1\n100\n101\n4\n5\n6\n\n"
	"100\n101\n\n"
	"1\n100\n101\n4\n5\n6\n\n"
	"100\n101\n102\n",
	},
	{"copy_into_slice",
	"dim a&{10}\n"
	"copy(a&{2 : 7}, \"hello\")\n"
	"range v& := a&{}\n"
	"  print v&\n"
	"endrange\n",
	"0\n0\n104\n101\n108\n108\n111\n0\n0\n0\n0\n",
	},
	{"array_slice_const",
	"dim a%(10)\n"
	"a%() = 1,2,3,4,5,6,7,8,9,10,11\n"
	"range v% := a%(:3)\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%(:3), 1)\n",
	"1\n2\n3\n2\n",
	},
	{"array_slice_empty_var",
	"dim a%(10)\n"
	"a%() = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b% := 0\n"
	"onerror print err enderror\n"
	"print dim(a%(b%:b%), 1)\n",
	"10\n",
	},
	{"array_slice_int_var",
	"dim a%(10)\n"
	"a%() = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b% := 1\n"
	"c% := 3\n"
	"range v% := a%(b% : c%)\n"
	"  print v%\n"
	"endrange\n"
	"print dim(a%(b% : c%), 1)\n",
	"2\n3\n1\n",
	},
	{"array_slice_real_var",
	"dim a(10)\n"
	"a() = 1,2,3,4,5,6,7,8,9,10,11\n"
	"b% := 1\n"
	"c% := 3\n"
	"range v := a(b% : c%)\n"
	"  print v\n"
	"endrange\n"
	"print dim(a(b% : c%), 1)\n",
	"2\n3\n1\n",
	},
	{"swap_proc",
	 "type PROCa(a%)\n"
	 "dim c@PROCa(0)\n"
	 "local a@PROCa = def PROC(a%) print a%+a% endproc\n"
	 "local b@PROCa = def PROC(a%) print a%*a% endproc\n"
	 "c@PROCa(0) = def PROC(a%) print a% + a% + a% endproc\n"
	 "swap a@PROCa, b@PROCa\n"
	 "a@PROCa(4)\n"
	 "b@PROCa(4)\n"
	 "swap b@PROCa, c@PROCa(0)\n"
	 "b@PROCa(4)\n"
	 "rem get around a bug in the parser\n"
	 "d@PROCa = c@PROCa(0)\n"
	 "d@PROCa(4)\n",
	 "16\n8\n12\n8\n",
	},
	{"swap_int",
	 "a% := 1\n"
	 "b% := 2\n"
	 "swap a%, b%\n"
	 "print a%\n"
	 "print b%\n"
	 "c% = 3\n"
	 "swap b%, c%\n"
	 "print b%\n"
	 "print c%\n"
	 "d% = 4\n"
	 "swap c%, d%\n"
	 "print c%\n"
	 "print d%\n"
	 "dim aa%(0)\n"
	 "aa%(0) = 5\n"
	 "swap aa%(0), d%\n"
	 "print d%\n"
	 "print aa%(0)\n"
	 "dim av%{0}\n"
	 "av%{0} = 6\n"
	 "swap av%{0}, aa%(0)\n"
	 "print aa%(0)\n"
	 "print av%{0}\n",
	 "2\n1\n3\n1\n4\n1\n5\n1\n6\n1\n",
	},
	{"swap_byte",
	 "a& := 1\n"
	 "b& := 2\n"
	 "swap a&, b&\n"
	 "print a&\n"
	 "print b&\n"
	 "c& = 3\n"
	 "swap b&, c&\n"
	 "print b&\n"
	 "print c&\n"
	 "d& = 4\n"
	 "swap c&, d&\n"
	 "print c&\n"
	 "print d&\n"
	 "dim aa&(0)\n"
	 "aa&(0) = 5\n"
	 "swap aa&(0), d&\n"
	 "print d&\n"
	 "print aa&(0)\n"
	 "dim av&{0}\n"
	 "av&{0} = 6\n"
	 "swap av&{0}, aa&(0)\n"
	 "print aa&(0)\n"
	 "print av&{0}\n",
	 "2\n1\n3\n1\n4\n1\n5\n1\n6\n1\n",
	},
	{"swap_real",
	 "a := 1\n"
	 "b := 2\n"
	 "swap a, b\n"
	 "print a\n"
	 "print b\n"
	 "c = 3\n"
	 "swap b, c\n"
	 "print b\n"
	 "print c\n"
	 "d = 4\n"
	 "swap c, d\n"
	 "print c\n"
	 "print d\n"
	 "dim aa(0)\n"
	 "aa(0) = 5\n"
	 "swap aa(0), d\n"
	 "print d\n"
	 "print aa(0)\n"
	 "dim av{0}\n"
	 "av{0} = 6\n"
	 "swap av{0}, aa(0)\n"
	 "print aa(0)\n"
	 "print av{0}\n",
	 "2\n1\n3\n1\n4\n1\n5\n1\n6\n1\n",
	},
	{"swap_string",
	 "a$ := \"hello\"\n"
	 "b$ := \"world\"\n"
	 "swap a$, b$\n"
	 "print a$\n"
	 "print b$\n"
	 "c$ = \"Monday\"\n"
	 "swap b$, c$\n"
	 "print b$\n"
	 "print c$\n"
	 "d$ = \"Tuesday\"\n"
	 "swap c$, d$\n"
	 "print c$\n"
	 "print d$\n"
	 "dim e$(5), f$(0)\n"
	 "e$(4) = \"Thursday\"\n"
	 "f$(0) = \"Wednesday\"\n"
	 "swap d$, f$(0)\n"
	 "print d$\n"
	 "print f$(0)\n"
	 "a% := 2\n"
	 "swap f$(0), e$(a% * 2)\n"
	 "print f$(0)\n"
	 "print e$(a% + a%)\n",
	 "world\nhello\nMonday\nhello\nTuesday\nhello\nWednesday\nhello\n"
	 "Thursday\nhello\n",
	},
	{"swap_array",
	 "dim a&(0), b&(0)\n"
	 "a&() = 1\n"
	 "b&() = 2\n"
	 "swap a&(), b&()\n"
	 "print a&(0)\n"
	 "print b&(0)\n"
	 "local dim a%(0), b%(0)\n"
	 "a%() = 1\n"
	 "b%() = 2\n"
	 "swap a%(), b%()\n"
	 "print a%(0)\n"
	 "print b%(0)\n"
	 "local dim c(1,1)\n"
	 "dim d(1,1)\n"
	 "c() = 3\n"
	 "d() = 4\n"
	 "swap c(), d()\n"
	 "print c(1,1)\n"
	 "print d(1,1)\n"
	 "dim e$(0), f$(0)\n"
	 "e$(0) = \"hello\""
	 "f$(0) = \"world\""
	 "swap e$(), f$()\n"
	 "print e$(0)\n"
	 "print f$(0)\n"
	 "type FNStr$\n"
	 "dim g@FNStr(0), h@FNStr(0)\n"
	 "g@FNStr(0) = def FN$ <- \"hello\"\n"
	 "h@FNStr(0) = def FN$ <- \"world\"\n"
	 "swap g@FNStr(), h@FNStr()\n"
	 "print g@FNStr(0)()\n"
	 "print h@FNStr(0)()\n",
	 "2\n1\n2\n1\n4\n3\nworld\nhello\nworld\nhello\n"
	},
	{"swap_vector",
	 "dim a&{0}, b&{0}\n"
	 "a&{} = 1\n"
	 "b&{} = 2\n"
	 "swap a&{}, b&{}\n"
	 "print a&{0}\n"
	 "print b&{0}\n"
	 "local dim a%{0}, b%{0}\n"
	 "a%{} = 1\n"
	 "b%{} = 2\n"
	 "swap a%{}, b%{}\n"
	 "print a%{0}\n"
	 "print b%{0}\n"
	 "dim e${0}, f${0}\n"
	 "e${0} = \"hello\""
	 "f${0} = \"world\""
	 "swap e${}, f${}\n"
	 "print e${0}\n"
	 "print f${0}\n"
	 "type FNStr$\n"
	 "dim g@FNStr{0}, h@FNStr{0}\n"
	 "g@FNStr{0} = def FN$ <- \"hello\"\n"
	 "h@FNStr{0} = def FN$ <- \"world\"\n"
	 "swap g@FNStr{}, h@FNStr{}\n"
	 "print g@FNStr{0}()\n"
	 "print h@FNStr{0}()\n",
	 "2\n1\n2\n1\nworld\nhello\nworld\nhello\n"
	},
	{"fn_arr_deref",
	 "type FNMark%(1)\n"
	 "local a@FNMark = def FN%(1) local dim a%(0) a%() = 66 <-a%()\n"
	 "print a@FNMark()(0)\n",
	 "66\n"
	},
	{"fn_vec_deref",
	"type FNMark%{}\n"
	"local a@FNMark = def FN%{} local dim a%{0} a%{} = 66 <-a%{}\n"
	"print a@FNMark(){0}\n",
	"66\n"
	},
	{"rec_assign_copy",
	"type FNptr%(a%)\n"
	"type RECData (\n"
	"     x&\n"
	"     y%\n"
	"     dim a%{1}\n"
	"     a@FNptr\n"
	")\n"
	"type RECPoint (\n"
	"     a\n"
	"     b$\n"
	"     h@RECData\n"
	"     dim a$(1)\n"
	")\n"
	"\n"
	"local a@RECPoint\n"
	"\n"
	"a@RECPoint.a = 2.0\n"
	"a@RECPoint.b$ = \"Hello World!\"\n"
	"a@RECPoint.b$ += \" and goodbye\"\n"
	"a@RECPoint.h@RECData.x& = -1\n"
	"a@RECPoint.h@RECData.y% = 12\n"
	"a@RECPoint.h@RECData.y% += 1\n"
	"a@RECPoint.h@RECData.y% -= 10\n"
	"a@RECPoint.h@RECData.a%{0} = 11\n"
	"a@RECPoint.h@RECData.a%{0} += 1\n"
	"a@RECPoint.h@RECData.a%{0} -= 2\n"
	"a@RECPoint.h@RECData.a@FNptr = def FN%(a%) <- a% * a%\n"
	"a@RECPoint.a$(0) = \"BASIC\"\n"
	"a@RECPoint.a$(0) += \" is cool\"\n"
	"\n"
	"print a@RECPoint.a\n"
	"print a@RECPoint.b$\n"
	"print a@RECPoint.h@RECData.x&\n"
	"print a@RECPoint.h@RECData.y%\n"
	"print a@RECPoint.h@RECData.a%{0}\n"
	"print a@RECPoint.h@RECData.a@FNptr(10)\n"
	"print a@RECPoint.a$(0)\n"
	"\n"
	"local b@RECPoint\n"
	"b@RECPoint = a@RECPoint\n"
	"print b@RECPoint.a\n"
	"print b@RECPoint.b$\n"
	"print a@RECPoint.h@RECData.x&\n"
	"print b@RECPoint.h@RECData.y%\n"
	"print b@RECPoint.h@RECData.a%{0}\n"
	"print b@RECPoint.h@RECData.a@FNptr(10)\n"
	"print b@RECPoint.a$(0)\n",
	"2\n"
	"Hello World! and goodbye\n"
	"-1\n"
	"3\n"
	"10\n"
	"100\n"
	"BASIC is cool\n"
	"2\n"
	"Hello World! and goodbye\n"
	"-1\n"
	"3\n"
	"10\n"
	"100\n"
	"BASIC is cool\n"
	},
	{"rec_zero",
	 "type FNPtr\n"
	 "\n"
	 "type RECInner (\n"
	 "     dim a%(1)\n"
	 "     dim b{1}\n"
	 ")\n"
	 "\n"
	 "type RECData (\n"
	 "     a%\n"
	 "     c$\n"
	 "     e&\n"
	 "     d@FNPtr\n"
	 "     f\n"
	 "     dim g$(1)\n"
	 "     h@RECInner\n"
	 ")\n"
	 "\n"
	 "local a@RECData\n"
	 "\n"
	 "print a@RECData.a%\n"
	 "print len(a@RECData.c$)\n"
	 "print a@RECData.e&\n"
	 "print a@RECData.d@FNPtr()\n"
	 "print a@RECData.f\n"
	 "print len(a@RECData.g$(0))\n"
	 "print a@RECData.h@RECInner.a%(0)\n"
	 "print a@RECData.h@RECInner.b{0}\n",
	 "0\n0\n0\n0\n0\n0\n0\n0\n",
	},
	{"rec_zero_init",
	 "type FNPtr\n"
	 "\n"
	 "type RECInner (\n"
	 "     dim a%(1)\n"
	 "     dim b{1}\n"
	 ")\n"
	 "\n"
	 "type RECData (\n"
	 "     a%\n"
	 "     c$\n"
	 "     e&\n"
	 "     d@FNPtr\n"
	 "     f\n"
	 "     dim g$(1)\n"
	 "     h@RECInner\n"
	 ")\n"
	 "\n"
	 "local a@RECData = ()\n"
	 "\n"
	 "print a@RECData.a%\n"
	 "print len(a@RECData.c$)\n"
	 "print a@RECData.e&\n"
	 "print a@RECData.d@FNPtr()\n"
	 "print a@RECData.f\n"
	 "print len(a@RECData.g$(0))\n"
	 "print a@RECData.h@RECInner.a%(0)\n"
	 "print a@RECData.h@RECInner.b{0}\n",
	 "0\n0\n0\n0\n0\n0\n0\n0\n",
	},
	{"rec_init",
	"type RECa1 (\n"
	"     a%\n"
	"     b$\n"
	"     dim ar${1}\n"
	")\n"
	"\n"
	"type RECa2 (\n"
	"     a@RECa1\n"
	"     dim br${1}\n"
	"     dim nums%{2}\n"
	"     dim reals{2}\n"
	"     d%\n"
	")\n"
	"\n"
	"local a% = 10\n"
	"local dim ar${1}\n"
	"ar${} = \"hello\", \"world\"\n"
	"local a@RECa2 = ( (a%, \"inner\", ar${}), ( \"goodbye\" ),\n"
	"( 1, 2, 3 ), ( 1.5, 2.5, 3.5 ), 1000 )\n"
	"\n"
	"print a@RECa2.a@RECa1.a%\n"
	"print a@RECa2.a@RECa1.b$\n"
	"\n"
	"range v$ := a@RECa2.a@RECa1.ar${}\n"
	"  print v$\n"
	"endrange\n"
	"\n"
	"range v$ := a@RECa2.br${}\n"
	"  print v$\n"
	"endrange\n"
	"\n"
	"range v% := a@RECa2.nums%{}\n"
	"  print v%\n"
	"endrange\n"
	"\n"
	"range v := a@RECa2.reals{}\n"
	"  print v\n"
	"endrange\n"
	"\n"
	"print a@RECa2.d%\n",
	"10\ninner\nhello\nworld\ngoodbye\ngoodbye\n1\n2\n3\n1.5\n2.5\n3.5\n"
	"1000\n",
	},
	{"rec_partial_init",
	"type RECInner (\n"
	"     b\n"
	"     dim a%(2)\n"
	"     f%\n"
	")\n"
	"\n"
	"type RECmark (\n"
	"     a%\n"
	"     a@RECInner\n"
	"     c$\n"
	"     d&\n"
	")\n"
	"\n"
	"local a@RECmark = ( 10, (2.0, ( 1, 2 )))\n"
	"print a@RECmark.a%\n"
	"print a@RECmark.a@RECInner.b\n"
	"print a@RECmark.a@RECInner.a%(0);\n"
	"range v% := a@RECmark.a@RECInner.a%(1:)\n"
	"  print \" \";\n"
	"  print v%;\n"
	"endrange\n"
	"print \"\"\n"
	"print a@RECmark.a@RECInner.f%\n"
	"print a@RECmark.c$\n"
	"print a@RECmark.d&\n",
	"10\n2\n1 2 0\n0\n\n0\n",
	},
	{"rec_partial_init_global",
	"type RECInner (\n"
	"     b\n"
	"     dim a%(2)\n"
	"     f%\n"
	")\n"
	"\n"
	"type RECmark (\n"
	"     a%\n"
	"     a@RECInner\n"
	"     c$\n"
	"     d&\n"
	")\n"
	"\n"
	"a@RECmark = ( 10, (2.0, ( 1, 2 )))\n"
	"print a@RECmark.a%\n"
	"print a@RECmark.a@RECInner.b\n"
	"print a@RECmark.a@RECInner.a%(0);\n"
	"range v% := a@RECmark.a@RECInner.a%(1:)\n"
	"  print \" \";\n"
	"  print v%;\n"
	"endrange\n"
	"print \"\"\n"
	"print a@RECmark.a@RECInner.f%\n"
	"print a@RECmark.c$\n"
	"print a@RECmark.d&\n",
	"10\n2\n1 2 0\n0\n\n0\n",
	},
	{"rec_zero_init_local_global",
	 "type RECmark ( a% b$ dim c%{} )\n"
	 "a@RECmark = ()\n"
	 "b@RECmark := ()\n"
	 "local c@RECmark = ()\n"
	 "print a@RECmark.a%\n"
	 "print len(a@RECmark.b$)\n"
	 "print dim(a@RECmark.c%{}, 1)\n"
	 "print b@RECmark.a%\n"
	 "print len(b@RECmark.b$)\n"
	 "print dim(b@RECmark.c%{}, 1)\n"
	 "print c@RECmark.a%\n"
	 "print len(c@RECmark.b$)\n"
	 "print dim(c@RECmark.c%{}, 1)\n",
	 "0\n0\n-1\n0\n0\n-1\n0\n0\n-1\n",
	},
	{"dim_arr_rec",
	"type FNCompute%(a)\n"
	"type RECa2 (\n"
	"  a%\n"
	"  x&\n"
	"  y\n"
	"  dim nums%(10)\n"
	"  dim s$(10)\n"
	"  dim b&(4)\n"
	")\n"
	"type RECa1 (\n"
	"  a@RECa2\n"
	"  a$\n"
	"  dim nums(20)\n"
	"  dim fns@FNCompute(5)\n"
	")\n"
	"\n"
	"dim a@RECa1(10)\n"
	"a@RECa1(1).a@RECa2.a% = 10\n"
	"a@RECa1(10).a@RECa2.y = 3.14\n"
	"a@RECa1(10).a@RECa2.nums%(2) = 49\n"
	"a@RECa1(10).a@RECa2.s$() = \"BASIC\"\n"
	"a@RECa1(10).a@RECa2.b&() = 88\n"
	"a@RECa1(1).a$ =  \"hello\"\n"
	"a@RECa1(1).nums(11) =  2.14\n"
	"a@RECa1(0).fns@FNCompute(5) = def FN%(a) <- a * 2\n"
	"print a@RECa1(1).a@RECa2.a%\n"
	"print a@RECa1(10).a@RECa2.y\n"
	"print a@RECa1(10).a@RECa2.nums%(2)\n"
	"print a@RECa1(10).a@RECa2.s$(3)\n"
	"print a@RECa1(10).a@RECa2.b&(3)\n"
	"print a@RECa1(1).a$\n"
	"print a@RECa1(1).nums(11)\n"
	"print a@RECa1(0).fns@FNCompute(5)(10)\n"
	"print a@RECa1(10).fns@FNCompute(5)(10)\n",
	"10\n3.14\n49\nBASIC\n88\nhello\n2.14\n20\n0\n",
	},
	{"dim_vec_rec",
	"type FNCompute%(a)\n"
	"type RECa2 (\n"
	"  a%\n"
	"  x&\n"
	"  y\n"
	"  dim nums%{10}\n"
	"  dim s${10}\n"
	"  dim b&{4}\n"
	")\n"
	"\n"
	"type RECa1 (\n"
	"  a@RECa2\n"
	"  a$\n"
	"  dim nums{20}\n"
	"  dim fns@FNCompute{5}\n"
	")\n"
	"\n"
	"dim a@RECa1{10}\n"
	"a@RECa1{1}.a@RECa2.a% = 10\n"
	"a@RECa1{10}.a@RECa2.y = 3.14\n"
	"a@RECa1{10}.a@RECa2.nums%{2} = 49\n"
	"a@RECa1{10}.a@RECa2.s${} = \"BASIC\"\n"
	"a@RECa1{10}.a@RECa2.b&{} = 88\n"
	"a@RECa1{1}.a$ =  \"hello\"\n"
	"a@RECa1{1}.nums{11} =  2.14\n"
	"a@RECa1{0}.fns@FNCompute{5} = def FN%(a) <- a * 2\n"
	"print a@RECa1{1}.a@RECa2.a%\n"
	"print a@RECa1{10}.a@RECa2.y\n"
	"print a@RECa1{10}.a@RECa2.nums%{2}\n"
	"print a@RECa1{10}.a@RECa2.s${3}\n"
	"print a@RECa1{10}.a@RECa2.b&{3}\n"
	"print a@RECa1{1}.a$\n"
	"print a@RECa1{1}.nums{11}\n"
	"print a@RECa1{0}.fns@FNCompute{5}(10)\n"
	"print a@RECa1{10}.fns@FNCompute{5}(10)\n",
	"10\n3.14\n49\nBASIC\n88\nhello\n2.14\n20\n0\n",
	},
	{"rec_copy_scalar",
	"type RECa1(\n"
	"     x%\n"
	"     y%\n"
	"     a\n"
	"     b&\n"
	")\n"
	"\n"
	"dim a@RECa1(3)\n"
	"\n"
	"for i% := 0 to dim(a@RECa1(),1)\n"
	"  a@RECa1(i%).x% = i%\n"
	"  a@RECa1(i%).y% = i% + 1\n"
	"  a@RECa1(i%).a = i%*3\n"
	"  a@RECa1(i%).b& = i% * 2\n"
	"next\n"
	"\n"
	"dim b@RECa1(3)\n"
	"copy(b@RECa1(), a@RECa1())\n"
	"for i% := 0 to dim(b@RECa1(),1)\n"
	"  print b@RECa1(i%).x%\n"
	"  print b@RECa1(i%).y%\n"
	"  print b@RECa1(i%).a\n"
	"  print b@RECa1(i%).b&\n"
	"  print \"\"\n"
	"next\n",
	"0\n1\n0\n0\n\n1\n2\n3\n2\n\n2\n3\n6\n4\n\n3\n4\n9\n6\n\n",
	},
	{"rec_copy_non_scalar",
	"type RECref (\n"
	"     dim a%(10)\n"
	")\n"
	"\n"
	"dim a@RECref(1)\n"
	"for i% := 0 to dim(a@RECref(0).a%(), 1)\n"
	"    a@RECref(0).a%(i%) = i%\n"
	"next\n"
	"\n"
	"dim b@RECref(1)\n"
	"copy(b@RECref(), a@RECref())\n"
	"range v% := b@RECref(0).a%()\n"
	"      print v%\n"
	"endrange\n",
	"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"
	},
	{"range_global_empty_slice",
	"dim a{1}\n"
	"range x = a{0:0}\n"
	"  print x\n"
	"endrange\n"
	"print x\n",
	"0\n",
	},
	{"range_rec_local_new",
	"type RECscalar (\n"
	"     a%\n"
	"     b\n"
	")\n"
	"\n"
	"dim a@RECscalar(1)\n"
	"for i% := 0 to dim(a@RECscalar(), 1)\n"
	"    a@RECscalar(i%).a% = 1 * (i% + 1)\n"
	"    a@RECscalar(i%).b = 2 * (i% + 1)\n"
	"next\n"
	"\n"
	"range v@RECscalar := a@RECscalar()\n"
	"    print v@RECscalar.a%\n"
	"    print v@RECscalar.b\n"
	 "endrange\n",
	 "1\n2\n2\n4\n",
	},
	{"range_rec_global_new",
	"type RECscalar (\n"
	"     a%\n"
	"     b\n"
	")\n"
	"\n"
	"dim a@RECscalar(1)\n"
	"for i% := 0 to dim(a@RECscalar(), 1)\n"
	"    a@RECscalar(i%).a% = 1 * (i% + 1)\n"
	"    a@RECscalar(i%).b = 2 * (i% + 1)\n"
	"next\n"
	"\n"
	"range v@RECscalar = a@RECscalar()\n"
	"    print v@RECscalar.a%\n"
	"    print v@RECscalar.b\n"
	"endrange\n"
	"print v@RECscalar.a%\n"
	"print v@RECscalar.b\n",
	 "1\n2\n2\n4\n2\n4\n",
	},
	{"range_tilde_global",
	 "dim b%(4)\n"
	"range ~, a% = b%()\n"
	"    print a%\n"
	"endrange\n",
	"0\n1\n2\n3\n4\n",
	},
	{"range_tilde_local",
	"type RECel (\n"
	"     a$\n"
	"     b%\n"
	"     d\n"
	"     dim c(10)\n"
	")\n"
	"dim a@RECel(4)\n"
	"range v@RECel, i% := a@RECel()\n"
	"  print a@RECel(i%).b%\n"
	"endrange\n",
	"0\n0\n0\n0\n0\n",
	},
	{"rec_proc_field_call",
	"type PROCMark(a%, b%)\n"
	"type RECmixed ( b@PROCMark )\n"
	"dim a@RECmixed(1)\n"
	"a@RECmixed(0).b@PROCMark = def PROC(a%, b%) print a% + b% endproc\n"
	"a@RECmixed(0).b@PROCMark(10, 11)\n",
	"21\n",
	},
	{"array_proc_call",
	"type PROCMark\n"
	"dim a@PROCMark(1)\n"
	"a@PROCMark(0) = def PROC print \"Hello\" endproc\n"
	"a@PROCMark(0)()\n",
	"Hello\n",
	},
	{"append_rec",
	"type RECScalar (a% b c&)\n"
	"dim a@RECScalar{0}\n"
	"local b@RECScalar = ( 1, 2, 3 )\n"
	"append(a@RECScalar{}, b@RECScalar)\n"
	"range ~, i% := a@RECScalar{}\n"
	"  print a@RECScalar{i%}.a%\n"
	"  print a@RECScalar{i%}.b\n"
	"  print a@RECScalar{i%}.c&\n"
	"endrange\n",
	"0\n0\n0\n1\n2\n3\n",
	},
	{"rec_init_copy",
	"type RECmixed ( a% b% )\n"
	"local c@RECmixed = (10, 11)\n"
	"local d@RECmixed = c@RECmixed\n"
	"print d@RECmixed.a%\n"
	"print d@RECmixed.b%\n",
	"10\n11\n",
	},
	{"rec_reset",
	"type RECref ( dim v%{1} )\n"
	"type RECScalar (\n"
	"     a%\n"
	"     b$\n"
	"     dim c%(1)\n"
	"     d@RECref\n"
	")\n"
	"\n"
	"local a@RECScalar = ( 10, \"hello\", ( 1, 2), ( (3, 4)))\n"
	"a@RECScalar = (100, \"goodbye\", ( 10, 20), (( 2 )))\n"
	"\n"
	"print a@RECScalar.a%\n"
	"print a@RECScalar.b$\n"
	"print a@RECScalar.c%(0)\n"
	"print a@RECScalar.c%(1)\n"
	"print a@RECScalar.d@RECref.v%{0}\n"
	"print a@RECScalar.d@RECref.v%{1}\n"
	"\n"
	"a@RECScalar = (100, \"goodbye\", ( 10, 20), ())\n"
	"print a@RECScalar.d@RECref.v%{0}\n"
	"print a@RECScalar.d@RECref.v%{1}\n",
	"100\ngoodbye\n10\n20\n2\n2\n0\n0\n",
	},
	{"rec_reset_partial",
	"type RECref ( dim v%{1} )\n"
	"type RECScalar (\n"
	"     dim f(1)\n"
	"     a%\n"
	"     b$\n"
	"     c\n"
	"     d$\n"
	"     e@RECref\n"
	")\n"
	"\n"
	"local a@RECScalar = ( ( 7, 8), 10, \"hello\", 3.14, \"ignore me\","
	"( ( 1, 1 ) ) )\n"
	"append(a@RECScalar.e@RECref.v%{}, 2)\n"
	"a@RECScalar = ( (9, 10), 100, \"goodbye\" )\n"
	"\n"
	"print a@RECScalar.f(0)\n"
	"print a@RECScalar.f(1)\n"
	"print a@RECScalar.a%\n"
	"print a@RECScalar.b$\n"
	"print a@RECScalar.c\n"
	"print a@RECScalar.d$\n"
	"print dim(a@RECScalar.e@RECref.v%{}, 1)\n"
	"print a@RECScalar.e@RECref.v%{0}\n"
	"print a@RECScalar.e@RECref.v%{1}\n",
	"9\n10\n100\ngoodbye\n0\n\n1\n0\n0\n",
	},
	{"array_rec_reset",
	"type RECScalar ( a% b$ c& )\n"
	"dim b@RECScalar(2)\n"
	"b@RECScalar(0) = ( 1, \"hello\", 3 )\n"
	"b@RECScalar(0) = ( 4, \"goodbye\", 6 )\n"
	"print b@RECScalar(0).a%\n"
	"print b@RECScalar(0).b$\n"
	"print b@RECScalar(0).c&\n",
	"4\ngoodbye\n6\n",
	},
	{"field_rec_reset",
	"type RECNested (\n"
	"   a$\n"
	"   dim b%(1)\n"
	")\n"
	"type RECScalar (\n"
	"     a%\n"
	"     b\n"
	"     c&\n"
	"     d@RECNested\n"
	")\n"
	"a@RECScalar := ()\n"
	"b$ = \"hello\"\n"
	"dim c%(1)\n"
	"c%() = 1, 2\n"
	"a@RECScalar.d@RECNested = (b$, c%())\n"
	"print a@RECScalar.a%\n"
	"print a@RECScalar.b\n"
	"print a@RECScalar.c&\n"
	"print a@RECScalar.d@RECNested.a$\n"
	"print a@RECScalar.d@RECNested.b%(0)\n"
	"print a@RECScalar.d@RECNested.b%(1)\n",
	"0\n0\n0\nhello\n1\n2\n",
	},
	{"rec_empty_reset",
	"type RECs (\n"
	"   dim a%(1)\n"
	"   s$\n"
	"   b\n"
	")\n"
	"\n"
	"a@RECs = ()\n"
	"a@RECs.a%() = 1\n"
	"a@RECs.s$ = \"Subtilis BASIC\"\n"
	"a@RECs.b = 3.14\n"
	"a@RECs = ()\n"
	"print a@RECs.a%(0)\n"
	"print a@RECs.a%(1)\n"
	"print a@RECs.s$\n"
	"print a@RECs.b\n",
	"0\n0\n\n0\n",
	},
	{"fn_array_ref_assign",
	"type FNtest%(a)\n"
	"dim a@FNtest(10)\n"
	"a@FNtest() = def FN%(a) <- a+ 1.0, def FN%(a) <-a+ 2.0\n"
	"b@FNtest() = a@FNtest()\n"
	"print b@FNtest(0)(1)\n"
	"print b@FNtest(1)(1)\n",
	"2\n3\n",
	},
	{"rec_array_ref_assign",
	"type RECtest(a)\n"
	"dim a@RECtest(10)\n"
	"dim b@RECtest(10)\n"
	"a@RECtest(0).a = 10\n"
	"b@RECtest() = a@RECtest()\n"
	"print a@RECtest(0).a\n",
	"10\n",
	},
	{"rec_append_array_scalar",
	"type RECScalar (\n"
	"     a%\n"
	"     b\n"
	"     c&\n"
	")\n"
	"\n"
	"dim a@RECScalar{0}\n"
	"dim b@RECScalar(2)\n"
	"\n"
	"a@RECScalar{0} = ( -1, -2, -3 )\n"
	"b@RECScalar(0) = ( 1, 2, 3 )\n"
	"b@RECScalar(1) = ( 4, 5, 6 )\n"
	"b@RECScalar(2) = ( 7, 8, 9 )\n"
	"append(a@RECScalar{}, b@RECScalar())\n"
	"\n"
	"range ~, i% := a@RECScalar{}\n"
	"  print a@RECScalar{i%}.a%\n"
	"  print a@RECScalar{i%}.b\n"
	"  print a@RECScalar{i%}.c&\n"
	"endrange\n",
	"-1\n-2\n-3\n1\n2\n3\n4\n5\n6\n7\n8\n9\n",
	},
	{"rec_append_array_ref",
	"type RECref ( a$ )\n"
	"\n"
	"type RECouter (\n"
	"     a%\n"
	"     b\n"
	"     d@RECref\n"
	"     c&\n"
	")\n"
	"\n"
	"dim a@RECouter{0}\n"
	"dim b@RECouter(2)\n"
	"\n"
	"a@RECouter{0} = ( -1, -2, (\"one\"), -3 )\n"
	"b@RECouter(0) = ( 1, 2, (\"two\"), 3 )\n"
	"b@RECouter(1) = ( 4, 5, (\"three\"), 6 )\n"
	"b@RECouter(2) = ( 7, 8, (\"four\"), 9 )\n"
	"append(a@RECouter{}, b@RECouter())\n"
	"\n"
	"range ~, i% := a@RECouter{}\n"
	"  print a@RECouter{i%}.a%\n"
	"  print a@RECouter{i%}.b\n"
	"  print a@RECouter{i%}.d@RECref.a$\n"
	"  print a@RECouter{i%}.c&\n"
	"endrange\n",
	"-1\n-2\none\n-3\n1\n2\ntwo\n3\n4\n5\nthree\n6\n7\n8\nfour\n9\n",
	},
	{"rec_ref_fn",
	"type RECRef (a% b$ c&)\n"
	"\n"
	"a@RECRef = ( 1, \"hello\", 3)\n"
	"b@RECRef = ( 4, \" goodbye\", 5)\n"
	"c@RECRef := FNAdd@RECRef(a@RECRef, b@RECRef)\n"
	"print c@RECRef.a%\n"
	"print c@RECRef.b$\n"
	"print c@RECRef.c&\n"
	"\n"
	"def FNAdd@RECRef(a@RECRef, b@RECRef)\n"
	"    a@RECRef.a% += b@RECRef.a%\n"
	"    a@RECRef.b$ += b@RECRef.b$\n"
	"    a@RECRef.c& += b@RECRef.c&\n"
	"<-a@RECRef\n",
	"5\nhello goodbye\n8\n",
	},
	{"rec_scalar_fn",
	"type RECScalar ( a% b c& )\n"
	"\n"
	"a@RECScalar = ( 1, 10.0, 3)\n"
	"b@RECScalar = ( 4, 10.0, 5)\n"
	"c@RECScalar = FNAdd@RECScalar(a@RECScalar, b@RECScalar)\n"
	"print c@RECScalar.a%\n"
	"print c@RECScalar.b\n"
	"print c@RECScalar.c&\n"
	"\n"
	"def FNAdd@RECScalar(a@RECScalar, b@RECScalar)\n"
	"    a@RECScalar.a% += b@RECScalar.a%\n"
	"    a@RECScalar.b += b@RECScalar.b\n"
	"    a@RECScalar.c& += b@RECScalar.c&\n"
	"<-a@RECScalar\n",
	"5\n20\n8\n",
	},
	{"rec_ref_fn_lambda",
	"type RECRef (\n"
	"     a%\n"
	"     b$\n"
	"     c&\n"
	")\n"
	"type FNPtr@RECRef(a@RECRef, b@RECRef)\n"
	"fn@FNPtr = def FN@RECRef(a@RECRef, b@RECRef)\n"
	"    a@RECRef.a% += b@RECRef.a%\n"
	"    a@RECRef.b$ += b@RECRef.b$\n"
	"    a@RECRef.c& += b@RECRef.c&\n"
	"<-a@RECRef\n"
	"\n"
	"a@RECRef = ( 1, \"hello\", 3)\n"
	"b@RECRef = ( 4, \" goodbye\", 5)\n"
	"c@RECRef := fn@FNPtr(a@RECRef, b@RECRef)\n"
	"print c@RECRef.a%\n"
	"print c@RECRef.b$\n"
	"print c@RECRef.c&\n",
	"5\nhello goodbye\n8\n",
	},
	{"rec_ref_fn_ptr",
	"type RECRef (\n"
	"     a%\n"
	"     b$\n"
	"     c&\n"
	")\n"
	"type FNPtr@RECRef(a@RECRef, b@RECRef)\n"
	"fn@FNPtr = !FNAdd@RECRef\n"
	"\n"
	"a@RECRef = ( 1, \"hello\", 3)\n"
	"b@RECRef = ( 4, \" goodbye\", 5)\n"
	"c@RECRef := fn@FNPtr(a@RECRef, b@RECRef)\n"
	"print c@RECRef.a%\n"
	"print c@RECRef.b$\n"
	"print c@RECRef.c&\n"
	"\n"
	"def FNAdd@RECRef(a@RECRef, b@RECRef)\n"
	"    a@RECRef.a% += b@RECRef.a%\n"
	"    a@RECRef.b$ += b@RECRef.b$\n"
	"    a@RECRef.c& += b@RECRef.c&\n"
	"<-a@RECRef\n",
	"5\nhello goodbye\n8\n",
	},
	{"rec_scalar_vec_fn",
	"type RECar ( a% )\n"
	"dim a@RECar(9)\n"
	"dim b@RECar(9)\n"
	"for i% := 0 to 9\n"
	"  a@RECar(i%).a% = i%\n"
	"  b@RECar(i%).a% = 10 + i%\n"
	"next\n"
	"c@RECar{} := FNappend@RECar{}(a@RECar(), b@RECar())\n"
	"range v@RECar := c@RECar{}\n"
	"  print v@RECar.a%\n"
	"endrange\n"
	"def FNappend@RECar{}(a@RECar(1), b@RECar(1))\n"
	"    local dim c@RECar{}\n"
	"    range v@RECar := a@RECar()\n"
	"        append(c@RECar{}, v@RECar)\n"
	"    endrange\n"
	"    range v@RECar := b@RECar()\n"
	"        append(c@RECar{}, v@RECar)\n"
	"    endrange\n"
	"<-c@RECar{}\n",
	"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n"
	"10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n",
	},
	{"rec_fn_return_zero_rec_ref",
	"type RECScalar (\n"
	"     a%\n"
	"     b$\n"
	"     c&\n"
	")\n"
	"\n"
	"a@RECScalar := FNAdd@RECScalar()\n"
	"print a@RECScalar.a%\n"
	"print a@RECScalar.b$\n"
	"print a@RECScalar.c&\n"
	"\n"
	"def FNAdd@RECScalar\n"
	"    local a@RECScalar\n"
	"<-a@RECScalar\n",
	"0\n\n0\n",
	},
	{"rec_fn_return_rec_multiple_ref",
	"type RECScalar (\n"
	"     dim a%(1)\n"
	"     b$\n"
	"     dim c$(2)\n"
	")\n"
	"\n"
	"a@RECScalar := FNAdd@RECScalar()\n"
	"local b@RECScalar = ( ( 3, 4),\n"
	"\"quiet\", (\"but\", \"not\", \"raining\" ))\n"
	"print a@RECScalar.a%(0)\n"
	"print a@RECScalar.a%(1)\n"
	"print a@RECScalar.b$\n"
	"print a@RECScalar.c$(0)\n"
	"print a@RECScalar.c$(1)\n"
	"print a@RECScalar.c$(2)\n"
	"\n"
	"def FNAdd@RECScalar\n"
	"    local a@RECScalar = ( ( 1, 2), \"hello\",\n"
	"    (\"goodbye\", \"forever\" ))\n"
	"<-a@RECScalar\n",
	"1\n2\nhello\ngoodbye\nforever\n\n",
	},
	{"rec_fn_return_rec_nested_ref",
	"type RECinner (\n"
	"     dim a%(1)\n"
	"     b$\n"
	"     dim c$(2)\n"
	"     d$\n"
	")\n"
	"\n"
	"type RECref (\n"
	"     a@RECinner\n"
	"     name$\n"
	")\n"
	"\n"
	"a@RECref := FNAdd@RECref()\n"
	"print a@RECref.name$\n"
	"print a@RECref.a@RECinner.a%(0)\n"
	"print a@RECref.a@RECinner.a%(1)\n"
	"print a@RECref.a@RECinner.b$\n"
	"print a@RECref.a@RECinner.c$(0)\n"
	"print a@RECref.a@RECinner.c$(1)\n"
	"print a@RECref.a@RECinner.c$(2)\n"
	"print a@RECref.a@RECinner.d$\n"
	"\n"
	"def FNAdd@RECref\n"
	"    local a@RECref = ( ( ( 1, 2), \"hello\",\n"
	"       (\"goodbye\", \"forever\" ), \"whoops\"), \"outer\")\n"
	"<-a@RECref\n",
	"outer\n1\n2\nhello\ngoodbye\nforever\n\nwhoops\n",
	},
	{"rec_swap_three_bytes",
	"type RECref (\n"
	"     a&\n"
	"     b&\n"
	"     c&\n"
	")\n"
	"\n"
	"\n"
	"a@RECref := (1, 2, 3)\n"
	"b@RECref := (7, 8, 9)\n"
	"\n"
	"swap a@RECref, b@RECref\n"
	"\n"
	"print a@RECref.a&\n"
	"print a@RECref.b&\n"
	"print a@RECref.c&\n"
	"print b@RECref.a&\n"
	"print b@RECref.b&\n"
	"print b@RECref.c&\n",
	"7\n8\n9\n1\n2\n3\n",
	},
	{"rec_swap",
	"type RECref (\n"
	"     a%\n"
	"     b$\n"
	"     c&\n"
	")\n"
	"\n"
	"a@RECref := (1, \"two\", 3)\n"
	"b@RECref := (7, \"eight\", 9)\n"
	"\n"
	"swap a@RECref, b@RECref\n"
	"\n"
	"print a@RECref.a%\n"
	"print a@RECref.b$\n"
	"print a@RECref.c&\n"
	"print b@RECref.a%\n"
	"print b@RECref.b$\n"
	"print b@RECref.c&\n",
	"7\neight\n9\n1\ntwo\n3\n",
	},
	{"rec_array_swap",
	"type RECref (\n"
	"     a%\n"
	"     b$\n"
	"     c&\n"
	")\n"
	"\n"
	"dim a@RECref(0)\n"
	"dim b@RECref(0)\n"
	"\n"
	"a@RECref(0) = (1, \"two\", 3)\n"
	"b@RECref(0) = (7, \"eight\", 9)\n"
	"\n"
	"swap a@RECref(), b@RECref()\n"
	"\n"
	"print a@RECref(0).a%\n"
	"print a@RECref(0).b$\n"
	"print a@RECref(0).c&\n"
	"print b@RECref(0).a%\n"
	"print b@RECref(0).b$\n"
	"print b@RECref(0).c&\n",
	"7\neight\n9\n1\ntwo\n3\n",
	},
	{"rec_array_put_get",
	"type RECinner (\n"
	"     a%\n"
	")\n"
	"\n"
	"type RECscalar (\n"
	"     a%\n"
	"     b\n"
	"     c@RECinner\n"
	"     d&\n"
	")\n"
	"\n"
	"PROCWriteFile\n"
	"PROCReadFile\n"
	"def PROCWriteFile\n"
	"    f% := openout(\"markus\")\n"
	"    onerror\n"
	"      tryone close# f%\n"
	"    enderror\n"
	"    local dim a@RECscalar(1)\n"
	"    a@RECscalar(0) = ( 1, 3.14, ( 2 ), &ff )\n"
	"    a@RECscalar(1) = ( 2, 99, ( 3 ), &1f )\n"
	"    put# f%, a@RECscalar()\n"
	"    onerror error err enderror\n"
	"    close# f%\n"
	"endproc\n"
	"def PROCReadFile\n"
	"    f% := openin(\"markus\")\n"
	"    onerror\n"
	"      tryone close# f%\n"
	"    enderror\n"
	"    local dim a@RECscalar(1)\n"
	"    read% := get#(f%, a@RECscalar())\n"
	"    range ~, i% := a@RECscalar()\n"
	"        print a@RECscalar(i%).a%\n"
	"        print a@RECscalar(i%).b\n"
	"        print a@RECscalar(i%).c@RECinner.a%\n"
	"        print ~a@RECscalar(i%).d&\n"
	"    endrange\n"
	"    tryone close# f%\n"
	"endproc\n",
	"1\n3.14\n2\nFF\n2\n99\n3\n1F\n",
	},
	{"copy_array_fn",
	"type PROCdo(a%)\n"
	"\n"
	"dim a@PROCdo(1)\n"
	"a@PROCdo() = def PROC(a%) print a% endproc,\n"
	"def PROC(a%) for i% := 1 to a% print 1 next endproc\n"
	"dim b@PROCdo(1)\n"
	"copy(b@PROCdo(), a@PROCdo())\n"
	"\n"
	"range c@PROCdo := b@PROCdo()\n"
	"      c@PROCdo(4)\n"
	"endrange\n",
	"4\n1\n1\n1\n1\n",
	},
	{"copy_array_rec_fn",
	"type PROCdo(a%)\n"
	"type RECscalar (\n"
	"    a@PROCdo\n"
	")\n"
	"\n"
	"dim a@RECscalar(1)\n"
	"\n"
	"a@RECscalar(0) = (def PROC(a%) print a% endproc)\n"
	"a@RECscalar(1) = (def PROC(a%) for i% := 1 to a% print 1 next\n"
	"endproc)\n"
	"\n"
	"dim b@RECscalar(1)\n"
	"copy(b@RECscalar(), a@RECscalar())\n"
	"\n"
	"range c@RECscalar := b@RECscalar()\n"
	"      c@RECscalar.a@PROCdo(4)\n"
	"endrange\n",
	"4\n1\n1\n1\n1\n",
	},
	{"mid_str_alias",
	"c$ = \"hello mark\"\n"
	"\n"
	"rem alias c$ to force an alloc\n"
	"\n"
	"a$ = c$\n"
	"b$ := \"LLO\"\n"
	"mid$(a$, 2) = b$\n"
	"print a$\n"
	"\n"
	"b$ = \"hello mark\"\n"
	"mid$(a$, 2) = b$\n"
	"print a$\n",
	"heLLO mark\n"
	"hehello ma\n",
	},
	{"assign_empty_vector_from_tmp",
	"local dim lines${}\n"
	"lines${} = FNConsumeBuffer${}(lines${})\n"
	"print dim(lines${}, 1)\n"
	"def FNConsumeBuffer${}(lines${})\n"
	"<- lines${}",
	"-1\n",
	},
	{"append_vector_string_return",
	"lines${} := FNGetLines${}()\n"
	"range s$ := lines${}\n"
	"  print s$\n"
	"endrange\n"
	"\n"
	"def FNGetLines${}\n"
	"    local dim lines${}\n"
	"    for i% := 0 to 1\n"
	"        lines${} = FNFillBuffer${}(lines${})\n"
	"    next\n"
	"<-lines${}\n"
	"\n"
	"def FNFillBuffer${}(lines${})\n"
	"      s$ := string$(16, \"a\")\n"
	"      append(lines${}, s$)\n"
	"<-lines${}\n",
	"aaaaaaaaaaaaaaaa\n"
	"aaaaaaaaaaaaaaaa\n"
	},
	{"append_vector_rec_ref_return",
	"type RECstr ( a$ )\n"
	"\n"
	"lines@RECstr{} := FNGetLines@RECstr{}()\n"
	"range s@RECstr := lines@RECstr{}\n"
	"  print s@RECstr.a$\n"
	"endrange\n"
	"\n"
	"def FNGetLines@RECstr{}\n"
	"    local dim lines@RECstr{}\n"
	"    for i% := 0 to 1\n"
	"        lines@RECstr{} = FNFillBuffer@RECstr{}(lines@RECstr{})\n"
	"    next\n"
	"<-lines@RECstr{}\n"
	"\n"
	"def FNFillBuffer@RECstr{}(lines@RECstr{})\n"
	"      local a@RECstr = ( string$(16, \"a\") )\n"
	"      append(lines@RECstr{}, a@RECstr)\n"
	"<-lines@RECstr{}\n",
	"aaaaaaaaaaaaaaaa\naaaaaaaaaaaaaaaa\n",
	},
	{"append_vector_rec_noref_return",
	"type RECint ( a% )\n"
	"\n"
	"lines@RECint{} := FNGetLines@RECint{}()\n"
	"range s@RECint := lines@RECint{}\n"
	"  print s@RECint.a%\n"
	"endrange\n"
	"\n"
	"def FNGetLines@RECint{}\n"
	"    local dim lines@RECint{}\n"
	"    for i% := 0 to 1\n"
	"        lines@RECint{} = FNFillBuffer@RECint{}(lines@RECint{})\n"
	"    next\n"
	"<-lines@RECint{}\n"
	"\n"
	"def FNFillBuffer@RECint{}(lines@RECint{})\n"
	"      local a@RECint = ( 16 )\n"
	"      append(lines@RECint{}, a@RECint)\n"
	"<-lines@RECint{}\n",
	"16\n16\n",
	},
	{"reset_vector_string_slice",
	"local dim lines${5}\n"
	"lines${} = \"one\", \"two\", \"three\", \"four\", \"five\", \"six\"\n"
	"\n"
	"def FNSliceReset${}(a${})\n"
	"  a${} = \"hello\", \"goodbye\"\n"
	"<-a${}\n"
	"\n"
	"lines2${} = FNSliceReset${}(lines${1:3})\n"
	"range s$ := lines2${}\n"
	"  print s$\n"
	"endrange\n",
	"hello\ngoodbye\n",
	},
	{"set_vector_string_slice",
	"local dim lines${5}\n"
	"lines${} = \"one\", \"two\", \"three\", \"four\", \"five\", \"six\"\n"
	"\n"
	"def FNSliceReset${}(a${})\n"
	"  a${} = \"hello\"\n"
	"<-a${}\n"
	"\n"
	"lines2${} = FNSliceReset${}(lines${1:3})\n"
	"range s$ := lines2${}\n"
	"  print s$\n"
	"endrange\n",
	"hello\nhello\n",
	},
	{"append_vector_string_slice",
	"local dim lines${5}\n"
	"lines${} = \"one\", \"two\", \"three\", \"four\", \"five\", \"six\"\n"
	"\n"
	"def FNSliceAppend${}(a${})\n"
	"  append(a${}, \"three\")\n"
	"<-a${}\n"
	"\n"
	"lines2${} = FNSliceAppend${}(lines${1:3})\n"
	"range s$ := lines2${}\n"
	"  print s$\n"
	"endrange\n",
	"two\nthree\nthree\n",
	},
	{"append_vector_slice_rec",
	"type RECStr ( a$ )\n"
	"\n"
	"local dim lines@RECStr{5}\n"
	"lines@RECStr{0}.a$ = \"one\"\n"
	"lines@RECStr{1}.a$ = \"two\"\n"
	"lines@RECStr{2}.a$ = \"three\"\n"
	"lines@RECStr{3}.a$ = \"four\"\n"
	"lines@RECStr{4}.a$ = \"five\"\n"
	"lines@RECStr{5}.a$ = \"six\"\n"
	"\n"
	"def FNSliceReset@RECStr{}(a@RECStr{})\n"
	"  local b@RECStr = ( \"hello\" )\n"
	"  append(a@RECStr{}, b@RECStr)\n"
	"<-a@RECStr{}\n"
	"\n"
	"lines2@RECStr{} = FNSliceReset@RECStr{}(lines@RECStr{1:3})\n"
	"range a@RECStr := lines2@RECStr{}\n"
	"  print a@RECStr.a$\n"
	"endrange\n",
	"two\nthree\nhello\n",
	},
	{"append_vector_array_string_return",
	"lines${} := FNGetLines${}()\n"
	"range s$ := lines${}\n"
	"  print s$\n"
	"endrange\n"
	"\n"
	"def FNGetLines${}\n"
	"    local dim lines${}\n"
	"    for i% := 0 to 1\n"
	"        lines${} = FNFillBuffer${}(lines${})\n"
	"    next\n"
	"<-lines${}\n"
	"\n"
	"def FNFillBuffer${}(lines${})\n"
	"      local dim a$(1)\n"
	"      a$() = string$(16, \"a\"), string$(16, \"b\")\n"
	"      append(lines${}, a$())\n"
	"<-lines${}\n",
	"aaaaaaaaaaaaaaaa\nbbbbbbbbbbbbbbbb\n"
	"aaaaaaaaaaaaaaaa\nbbbbbbbbbbbbbbbb\n",
	},
	{"append_vector_array_rec_ref_return",
	"type RECStr ( s$ )\n"
	"lines@RECStr{} := FNGetLines@RECStr{}()\n"
	"range a@RECStr := lines@RECStr{}\n"
	"  print a@RECStr.s$\n"
	"endrange\n"
	"\n"
	"def FNGetLines@RECStr{}\n"
	"    local dim lines@RECStr{}\n"
	"    for i% := 0 to 1\n"
	"        lines@RECStr{} = FNFillBuffer@RECStr{}(lines@RECStr{})\n"
	"    next\n"
	"<-lines@RECStr{}\n"
	"\n"
	"def FNFillBuffer@RECStr{}(lines@RECStr{})\n"
	"      local dim a@RECStr(1)\n"
	"      a@RECStr(0) = ( string$(16, \"a\") )\n"
	"      a@RECStr(1) = ( string$(16, \"b\") )\n"
	"      append(lines@RECStr{}, a@RECStr())\n"
	"<-lines@RECStr{}\n",
	"aaaaaaaaaaaaaaaa\nbbbbbbbbbbbbbbbb\n"
	"aaaaaaaaaaaaaaaa\nbbbbbbbbbbbbbbbb\n",
	},
	{"string_nc_c_1/4_byte_eq_neq",
	"a$ := \"hello\" + chr$(10)\n"
	"print right$(a$) <> chr$(10)  rem 0\n"
	"print right$(a$) <> chr$(11)  rem -1\n"
	"print right$(a$) = chr$(10)   rem -1\n"
	"print right$(a$) = chr$(11)   rem 0\n"
	"b$ := \"ello\"\n"
	"print b$ <> \"ello\"            rem 0\n"
	"print b$ <> \"elld\"            rem -1\n"
	"print b$ = \"ello\"             rem -1\n"
	"print b$ = \"ellp\"             rem 0\n",
	"0\n-1\n-1\n0\n0\n-1\n-1\n0\n",
	},
	{"right_str_non_const_full_in_block",
	"local dim av${}\n"
	"local existing%\n"
	"a$ := \"a\"\n"
	"if 1 then\n"
	"  existing% = right$(a$) <> chr$(10)\n"
	"endif\n"
	"append(av${}, a$)\n"
	"print av${0}\n",
	"a\n",
	},
	{"left_str_non_const_full_in_block",
	"local dim av${}\n"
	"local existing%\n"
	"a$ := \"a\"\n"
	"if 1 then\n"
	"  existing% = left$(a$) <> chr$(10)\n"
	"endif\n"
	"append(av${}, a$)\n"
	"print av${0}\n",
	"a\n",
	},
	{"mid_str_non_const_full_in_block",
	"local dim av${}\n"
	"local existing%\n"
	"a$ := \"a\"\n"
	"if 1 then\n"
	"  existing% = mid$(a$,1) <> chr$(10)\n"
	"endif\n"
	"append(av${}, a$)\n"
	"print av${0}\n",
	"a\n",
	},
	{"append_assign_to_self_int",
	"local dim a%{1}\n"
	"a%{} = append(a%{}, 1)\n"
	"print dim(a%{}, 1)\n",
	"2\n",
	},
	{"append_assign_to_self_string",
	"local dim a${1}\n"
	"a${} = append(a${}, \"string\")\n"
	"print dim(a${}, 1)\n",
	"2\n",
	},
	{"append_assign_to_self_nested",
	"local dim a%{1}\n"
	"for i% := 1 to 10\n"
	"  a%{} = append(a%{}, 1)\n"
	"next\n"
	"print dim(a%{}, 1)\n",
	"11\n",
	},
	{"append_assign_to_self_empty",
	"local dim a%{}\n"
	"local dim b%{}\n"
	"a%{} = append(a%{}, b%{})\n"
	"print dim(a%{}, 1)\n",
	 "-1\n",
	},
	{"osargs",
	"print osargs$\n",
	"unit_test\n",
	},
	{"mid_str_var",
	"list$ := \"hello world\"\n"
	"start% := 4\n"
	"end% := 20\n"
	"print mid$(list$,start%, end%)\n"
	"print start%\n"
	"print end%\n",
	"lo world\n4\n20\n",
	},
};

/* clang-format on */
