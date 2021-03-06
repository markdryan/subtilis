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
	 "dim a(c%)\n"
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
	 "a$() = \"Mark\""
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
	"  f% := openout \"Markus\"\n"
	"  onerror\n"
	"    tryone close# f%\n"
	"  enderror\n"
	"\n"
	"  print eof# f%\n"
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
	"  g% := openin \"Markus\"\n"
	"\n"
	"  onerror\n"
	"    tryone close# g%\n"
	"  enderror\n"
	"\n"
	"  print eof# g%\n"
	"\n"
	"  b$ := \"\"\n"
	"  local a%\n"
	"  while (tryone a% = bget# g%) = 0\n"
	"     b$ += chr$(a%)\n"
	"  endwhile\n"
	"\n"
	"  print eof# g%\n"
	"\n"
	"  if not eof# g% then\n"
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
	"print ext# f%\n"
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
	"print ptr# f%\n"
	"onerror\n"
	"  print \"whoops\"\n"
	"enderror\n"
	"ptr# f%, 15\n"
	"print ptr# f%\n"
	"b$ := \"\"\n"
	"local a%\n"
	"while (tryone a% = bget# f%) = 0\n"
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
	"   print ptr# f%\n"
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
};

/* clang-format on */
