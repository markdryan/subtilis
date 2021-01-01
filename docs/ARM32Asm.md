# The 32 bit ARM Assembler

## Supported Instructions

Subtilis supports all the ARM2 instructions in addition to most of the FPA instructions.  Four FPA instructions are not currently supported;  WFC, RFC, LFM and SFM.  They will be implemented at some point.

## Calling Convention

Assembly language functions are free to corrupt any registers except for R12 and R13.  R12 and R13 must be restored to the values that they had when the function was entered.  Subtilis uses a full descending stack and the stack is pointed to by R13.  STMFD R13!, {R12} and LDMFD R13!, {R12} can be used to preserve and restore R12 inside an assembly language function.

Subtilis passes the first four integer parameters in  R0-R3 and the first four floating pointer parameters in F0-F3.  Subsequent parameters are passed on the stack.  Strings and Arrays are passed by pointer in integer registers.

For example, in the following procedure

```
def PROC(a, b%, c(), d)
[
]
```

F0 = a, R0 = b%, R1 = c(), F1 = d

Arguments that are passed via the stack are first grouped by type and then pushed onto the stack in the order in which they appear in the function or procedure definition.  Floating point arguments are pushed first, followed by integer arguments.  As Subtilis uses a full descending stack one needs to subtract from the stack pointer to access these arguments.  For example,

```
PRINT FNArgs%(1, 2, 3, 4, 5, 10)

def FNArgs%(a%, b%, c%, d%, e%, f%)
[
	LDR R0, [R13,-4]
	MOV PC, R14
]
```

The following code prints out 10.  If we modify the offset in the LDR statement to be -8, the value printed will be 5.

The following example should help to illustrate how stack arguments are laid out in memory.  The function accepts 5 floating point arguments and 5 integer arguments.  Only 8 of these arguments can be passed via registers.  The parameters 10 and 5.0 are passed on the stack.  The value 5.0 is pushed first (even though it is the last parameter passed to the function) followed by the integer 10.  To access the value of the 'er' parameter we need to use an offset of -12 (8 for the double and 4 for the integer).  The example then prints out 5.

```
PRINT FNArgs(1, 2, 3, 4, 1.0, 2.0, 3.0, 4.0, 10, 5.0)

def FNArgs(a%, b%, c%, d%, ar, br, cr, dr, e%, er)
[
	LDFD F0, [R13,-12]
	MOV PC, R14
]
```

### Return values

Values are returned from functions in either the R0 or the F0 registers.

## Calling functions from inside an assembly function

This is currently not possible although it will be added at some point.

## Expression

The Subtilis assembler supports a full expression syntax that implements many of BASIC's built-in functions and operators.  The assembler, like Subtilis itself, is strongly typed.  Each expression can be one of 6 different types:

1. A constant string, enclosed in quotes, e.g., "hello"
2. An integer number, e.g., 10
3. A floating point number, e.g., 3.14
4. An integer register.  Valid values are R0-R15 and PC.
5. A floating point register.  Valid values are F0-F7.
6. An identifier, e.g., label1.

As in BASIC, an implicit conversion happens when an integer expression is used where a floating point number is expected, and vice versa.  Registers can be converted to integers using the INT keyword, e.g, INT(R1) produces an integer expression of value 1.

The following operators are supported:

* EOR, OR, AND, NOT, TRUE, FALSE
* =, <>, >, <=, <, >=, <<, >>, >>>
* +, -
* *, DIV, /, MOD, ^

The precedence and associativity of the operators is identical to Subtilis.

The following built-in functions are supported:

* INT
* SIN, COS, TAN, ACS, ASN, ATN
* SQR, EXP, LOG, LN
* RAD, ABS, SGN, PI
* CHR$, ASC, LEN, LEFT$, RIGHT$, MID$, STRING$, STR$ (currently STR$~ isn't supported).

## Labels

Labels are identifiers which are not keywords or mnemomics.  They must be followed by a ':'.

## DEF

The DEF keyword can be used to define constants anywhere within an assembly language function.  The constants are local to the function in which they are defined.  They are visible from the
point of definition to the end of the current scope, which is normally the end of the function unless, the DEF statement is used inside a FOR loop.  The DEF keyword is used as follows

```
DEF <identifier> '=' <expression>
```

e.g.,

```
DEF angle = 60
DEF scale = COS(RAD(angle)) * PI
```

## Case

Currently, all ARM instructions must be entered in upper case.  All assembler directives and built-in functions however, e.g., def, for, exp, can be entered in either upper and lower (but not mixed) case.

## FOR

The assembler provides a simple looping construct in the form of BASIC's FOR statement.  The syntax of the FOR statement is as follows.

```
FOR <identifier> '=' <expression> TO <expression>
    (<instruction>|<label>|<directive>)
NEXT
```

Here are some examples

```
FOR i = 1 TO 10
    EQUD i
NEXT
```

```
FOR i = R0 TO R5
    MOV i, INT(i)
NEXT
```

The looping variable must be previously undefined.  The variable exists as a constant set to the final value of the loop variable when the loop exits.  This means that the variable cannot be re-used as the loop variable of another loop in the same scope.

Loops can be nested up to 32 levels.

The STEP keyword isn't currently supported but will be added at some point in the future.

## EQU statements

There are 6 assembler directives that can be used to store data directly in the assembled code.  These are

* EQUB    - Stores a byte
* EQUD    - Stores a 32 bit integer
* EQUF    - Stores a 32 bit float
* EQUDBL  - Stores a 64 bit double
* EQUDBLR - Stores a 64 bit double in which the two 32 bit words are reversed.  This is the format expected by the FPA instructions.
* EQUS    - Stores a string followed by a zero, e.g., EQUS "hi", stores 3 bytes.
* EQUW    - Stores two bytes.

The syntax of all the EQU statements is identical.

```
EQU <expression>
```

## ALIGN

The ALIGN statement can be used to insert 0s into the code so that the statement that follows the ALIGN directive will be aligned to the chosen boundary.  This is sometimes needed if you use one of the EQU directives to insert data whose size is not a multiple of 4 bytes.  For example,


```
def PROCPrint
[
	SWI "OS_WriteS"
	EQUS "hi"
	MOV PC, R14
]
```

This function will probably crash as the MOV PC, R14 instruction to return from the procedure is not word aligned.  To fix the issue an ALIGN statement is needed, e.g.,

```
def PROCPrint
[
	SWI "OS_WriteS"
	EQUS "hi"
	ALIGN 4
	MOV PC, R14
]
```

The syntax of the ALIGN directive is

```
ALIGN <expression>
```

The value of the expression must be a power of 2, > 0 and <= 1024.

## Layout of Subtilis data structures

Strings and arrays have a similar layout in memory.  Strings are laid out as follows:


|                   | Byte offset |
| ------------------|-------------|
| Size in Bytes     |           0 |
| Pointer to Data   |           4 |


Strings are not zero terminated in Subtilis.

Arrays are laid out as follows:


|                   | Byte offset |
|-------------------|-------------|
| Size in Bytes     |           0 |
| Pointer to Data   |           4 |
| Destructor        |           8 |
| Size of DIM 1     |          12 |
| Size of DIM n     | 8 + (n * 4) |


The size of the array object is determined by the number of dimensions it has that are not known at compile time.  When arrays are passed to functions any compile time information related to the size of its dimensions does not get passed to the function.  Therefore, if you pass a two dimensional array to a procedure the array object accessible inside the procedure will consist of 20 bytes, the final 8 bytes of which will hold the size of the two dimensions.

When a procedure is called with an array or a string as an argument, it's actually a pointer to a block of memory laid out in the manner described above, that is passed the procedure.  As an example, consider a simple function for computing the length of a string

```
def FNStrlen%(a$)
[
	LDR R0, [R0]
	MOV PC, R14
]
```

The following function returns the number of elements in a 1 dimensional array.

```
def FNArraySize%(a%(1))
[
	LDR R0, [R0, 12]
	ADD R0, R0, 1
	MOV PC, R14
]
```
