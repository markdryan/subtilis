# Subtilis

## Differences from BBC BASIC V

Although Subtilis resembles and is inspired by BBC BASIC it is not compatible
with BBC BASIC.  The most striking difference is that it cannot parse tokenized
BASIC files.  Currently, it only understands ASCII files although at some point
in the future this will probably be extended to add UTF-8 support, for strings
at least.  This means you can't compile an existing BBC BASIC program with
Subtilis.  It will need to be converted to ASCII first.  Further, modifications
will likely be needed as described in this document.

### BBC BASIC Features that Subtilis will never implement

#### Editing and environmental commands

None of the editing or environmental instructions such as LIST, RENUMBER, NEW, OLD
will be implemented as they simply don't make sense for a compiled language.

#### DATA, READ, RESTORE

DATA, READ and RESTORE seem to be a little anachronistic and will not be implemented.
They'll be replaced by structures and composite literals.

#### EVAL

EVAL will probably never be implemented as it would require run time type information
which Subtilis does not generate and will probably never generate (to keep binaries small).

#### Line numbers, GOTO and GOSUB

Subtilis does not use line numbers.  Trying to compile a program with line numbers
will result in an error.  As there are no line numbers there are also no GOTOs
(explicit or implicit) and no GOSUBs.  GOTO and GOSUB will probably never be
implemented.  GOSUB is essentially useless.  While GOTO can be useful for writing
cleanup code and for handling errors, there are better ways to do these sort of
things today, e.g, ON ERROR or something like Go's defer statement.  Subtilis will
probably get a defer statement at some point in the future.

### Newlines are not part of the grammar

BBC BASIC uses newlines to terminate statements, e.g.,

```
IF X% = 10 THEN PRINT "TEN"
```

Subtilis just treats newlines as normal whitespace, i.e., they play no part in
Subtilis's grammar.  This has a number of implications.

####  All IF statements need to be terminated by an ENDIF

So the code above would be written as

```
IF X% = 10 THEN
    PRINT "TEN"
ENDIF
```

in Subtilis.

#### Subtilis uses <- to return values from a function rather than =

For example,

```
DEF FNSUM%(a%, b%)
   LOCAL c%
   c% = a% + b%
<- c%
```

The reason for this is that 

```
DEF FNSUM%(a%, b%)
   LOCAL c%
   c% = a% + b%
=c%
```

will not compile in Subtilis.  As newlines are not part of the grammar, the parser
will see

```
c% = a% + b% = c%
```

as a single statement, i.e., assigning the result of a boolean expression to c%.  The
function will then be unterminated and a parse error will result.  Another way to
fix this would be to use '==' as the equality operator as in other languages.

#### VDU statements ending in ';' need to be enclosed in square brackets

This is for the same reason that cannot use '=' to return values from a function.
Consider the following code.

```
VDU 19, 2, 2; 0;
X% = 10
```

BBC BASIC would view this code as consisting of two different statements.  Subtilis
however, sees

```
VDU 19, 2, 2; 0; X% = 10
```

So an extra byte, either 0 or -1 would be written to the VDU stream depending on the result
of the expression X% = 10 which is viewed as a boolean expression rather than an assignment.

Subtilis hacks around this issue by allowing the user to enclose the VDU statements in
square brackets.  So in Subtilis we would write

```
VDU [19, 2, 2; 0;]
X% = 10
```

### Program layout

Each Subtilis program will consist of multiple files (currently, only one file is supported).  All
but one of these files can contain only functions and procedures.  One of the files can
however contain some basic statements that occur before, after or in between function and procedure
definitions.  These basic statements are actually part of the main function although no function is
explicitly defined.  They are the first statements that will be executed when the program is run.
There is no need to insert and END statement before the first procedure of function definition.
In fact the END statement isn't currently implemented.

For example

```
DEF FNFac%(X%)
    LOCAL ret%
    IF X% < 1 THEN
        ret% = 1
    ELSE
        ret% = X% * FNFac%(X%-1)
    ENDIF
<- ret%

PRINT FNFac%(6)
```

The first statement to execute in this program is the line PRINT FNFac%(6).  Note, there's no
need to place function definitions before there call site.  For example,

```
PRINT FNFac%(6)

DEF FNFac%(X%)
    LOCAL ret%
    IF X% < 1 THEN
        ret% = 1
    ELSE
        ret% = X% * FNFac%(X%-1)
    ENDIF
<- ret%
```

works just as well.  Again note the lack of an END statement.

### Local and Global Variables

When a new variable is defined using var = or LET var = and that variable has not
been declared LOCAL, a new global variable is created.  However, global variables
can only be created in the main procedure.  They can be modified once created inside
other procedures.  So for example,

```
X% = 10
PROCSet

DEF PROCSet
    X% = 11
ENDPROC
```

will compile, where as

```
PROCSet

DEF PROCSet
    X% = 11
ENDPROC
```

will not.  This is done on purpose to prevent users from accidentally declaring a
global variable when they really want a local variable.  You almost always want a
local variable in Subtilis as local variables are assigned to registers where as
global variables are not.  Operations involving local variables are consequently,
much faster than the equivalent operations involving global variables.  One side
affect of this limitation can be noted when using the FOR statement inside a function.
For example,

```
DEF PROCFor
    FOR I% = 1 TO 10
        PRINT I%
    NEXT
ENDPROC
```

will not compile in Subtilis as the code tries to create a new global variable
inside a function which is not permitted.  Instead you need to write,

```
DEF PROCFor
    LOCAL I%   
    FOR I% = 1 TO 10
        PRINT I%
    NEXT
ENDPROC
```

which will be much faster and will generate less code.

LOCAL variables can be created inside the main function, so code like this,

```
LOCAL I%   
FOR I% = 1 TO 10
    PRINT I%
NEXT
```

is allowed and recommended in Subtilis.  In future, I'll probably get rid of the
LOCAL keyword and make all variable declarations local by default which will solve
this issue.  Anyone actually wanting a global variable would need to declare it
in advance using a GLOBAL keyword.

### Function and Procedure definition

The DEF keyword must be followed by a space.  So DEFPROCTest will not compile in
Subtilis.  You need to write DEF PROCTest instead.

The return type of a function needs to be included in the function name.  So

```
DEF FNTest
    LOCAL X%
<-X%
```

defines a function that returns a real value where as

```
DEF FNTest%
    LOCAL X%
<-X%
```

defines a function that returns an integer.  In the case of the first function,
the compiler will generate extra code to convert the value of X%, 0, to a real
number.

ENDPROC or <- cannot be used in the main function.  There's currently no way
to terminate the main function early, although END will be implemented at
some point.

### Real numbers

Real numbers are currently defined to be 64 bit in Subtilis and are very slow
when using the FPE on RiscOS.  Presumably they're nice and fast if you have an
FPA unit, but these co-processors are quite rare.

### NEXT

NEXT does not take any parameters in Subtilis.  So you cannot write code like

```
FOR I% = 0 TO 10
    FOR J% = 0 TO 10
        PRINT I%*J%
NEXT J%, I%
```

Instead you need to write

```
FOR I% = 0 TO 10
    FOR J% = 0 TO 10
        PRINT I%*J%
    NEXT
NEXT
```

### Keywords that act like Functions

Keywords that act like functions, e.g., COS, SIN and RND, are parsed like functions,
without requiring the FN prefix.  This means that their parameters must be enclosed
within brackets even if there is only one parameter.  In BBC BASIC you could write SINA
but in Subtilis you must write SIN(A)

## Current Issues with the Grammar

### Function like keywords returning integer values

Function like keywords returning integer values such as GET, INKEY and RND don't end
in a % which is a little weird, especially seeing as how there are string equivalents
of some of these functions that end with a $.  Looking at the name of the keyword you'd expect GET to
return a real value when in fact it returns an integer.

### RND

The lack of type information in some of the keywords that return values is particularly
troublesome in the case of the RND keyword.  The issue here is that the return type of
the RND keyword can differ according to the value of the argument it is passed.  For
example, RND(5) returns an integer where are RND(1) returns a floating point value.
As the value cannot always be known at compile time, for example RND(A%), we have a problem.
Subtilis resolves the problem, rather unsatisfactorily at present, as follows.

* RND(0) and RND(1) return floating point values
* RND(constant > 1) and RND(constant < 0) return integers
* RND(A%), where A% is a variable, always returns a floating point number, even if A%=2 at
runtime.

Which is really weird.  RND(2) and A%=2 RND(A%) return two different types.  This sort of
works as you're probably going to use the return value in an integer context most of the
time, and so Subtilis will convert the floating point number to an integer for you,
for example.

```
B% = RND(A%)
```

where A% > 1, will work as intended, albeit slightly more inefficiently than expected.

```
PRINT RND(A%)
```

on the other hand will print out a floating point variable, e.g, 10.0.

This is all very broken and will have to change.  We'll probably end up with RND returning
floating point numbers and RND% returning integers.

## Unimplemented Language Features

### Constant Pooling

Currently, the ARM backend does not support constant pooling.  Any constants used by a function
or procedure are placed at the end of that function or procedure.  If the location of the
instruction that uses the constant is too far away from that constant, 1024 bytes for floating
point constants and 4096 bytes for real numbers, the function cannot be compiled and the
compiler will assert.  If this happens you need to split your functions up.  This will get
fixed at some point, once I figure out how.

### Assembler

There's no assembler, either inline or otherwise.  I have most of the code for this so it
will be implemented at some point.

### Other missing items

Here's a list of other language features that are currently not implemented but which will be at some point

* The @% variable
* Error Handling
* Arrays
* Strings
* File Handling
* CASE OF
* SOUND
* OSCLI and *
* TAB
* RETURN for passing arguments by reference to procedures and functions
* POINT TO

There are also some enhancements that will need to be added to the language to make it
more palatable to the modern programmer.

* Lower case keywords
* Structures
* Functions and procedures as first class types
* Closures
* Maps
* Defer

## Tooling

The tooling is very basic and needs a huge amount of work

* There's no linker so we're limited to a single source file right now.
* There's no error recovery so you only get a single error message before the compiler bombs out.
* There's no optimizer
* No assembler
* The compiler is too slow.  It takes 13 seconds to compile a very simple program on the A3000 (8 Mhz ARM2).

## Source code structure

The source code structure is very weird right now with all the code placed in the top level directory.
The reason for this is that there are some BASIC programs that copy the code to and from RiscOS machines
over NFS.  When the code is copied to the RiscOS machines it is copied in a way that is pleasing to the
RiscOS C compilers, e.g., all the C files are placed in a folder called C.  Changing the layout of the
source code would require changes to these BASIC files.  It's going to happen at some point but it hasn't
happened yet.
