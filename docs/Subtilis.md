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

### Subtilis permits lower case keywords

Keywords can be either upper or lower case in Subtilis.  Mixed case identifiers
are always treated like normal identifiers.  For example,

```
for FoR = 0 TO 10
  PRINT FoR
next
```

Here FoR is treated as an identifier and not a keyword and so the program compiles
without issue, although it looks a bit odd.

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
been declared LOCAL, the compiler will try to create a new global variable.  However,
Subtilis is much more restrictive than BBC Basic when creating global variables.
Global variables can only be created at the top level of the main procedure.  So although
they can be read from and written to anywhere in the program, they cannot be declared,
i.e., defined for the first time, in a user defined function or procedure or in a 
compound statement such as IF.

So for example,

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

and

```
REPEAT
    X% = 1
UNTIL TRUE
```

will not.  While this is quite restrictive, it shouldn't be too much of a problem
as you almost always want a local variable when programming in Subtilis.  Not
only are local variables necessary to isolate the various different parts of
your program, they are also assigned to registers where as global variables are
not.  Operations involving local variables are consequently, much faster than
the equivalent operations involving global variables.  One side effect of these
limitations can be noted when using the FOR statement inside a procedure.
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

or 

```
DEF PROCFor
    FOR LOCAL I% = 1 TO 10
        PRINT I%
    NEXT
ENDPROC
```

which will be much faster, will generate less code and won't interfere with
other parts of your code.  Note the programs in the two examples above
are equivalent.  In both cases the local variable I% is declared in the
outermost scope of the procedure, i.e., is accessible outside of the FOR
loop.

LOCAL variables can be created inside the main function, so code like this,

```
LOCAL I%   
FOR I% = 1 TO 10
    PRINT I%
NEXT
```

is allowed and recommended in Subtilis.

The LOCAL keyword can be used anywhere inside a function or procedure.  Local variables,
are scoped.  Local variables defined in a block are only avaialable within that block.
Local variables defined at the top level of a function or a procedure are available
in all subsequent blocks in that function or procedure.  Local variables shadow global
variables of the same name but they do not shadow local variables.  It is an error to
attempt to redefine an existing local variable in a nested compound statement.

Variables must be defined before they are used.  BBC BASIC permits code such
as

```
X% += 1
Y% = Y% + 1
```

where neither X% or Y% have been previously defined.  While such behaviour would be
possible in Subtilis, it has been explicitly disabled as it's very weird.

Currently the LOCAL statement can only be used to define a single variable.
However, it can be used to initialise the variable.  Thus in Subtilis you can
write

```
LOCAL y% = 10 MOD 3
```

If no initialiser is provided the variable is initialised to it's zero value,
0 for numbers and array elements and "" for strings.  Thus the statement

```
LOCAL x%
```

declares a local variable, x%, and initialises it to 0.

### The := assignment operator

Subtilis introduces the ':=' operator which creates a new local variable and
assigns a value to that variable.

```
x% := 1
```

is equivalent to writing

```
LOCAL x% = 1
```

only more concise.  As with the LOCAL keyword, the ':=' operator can be
used inside a FOR loop to declare a local, e.g.,

```
FOR i% := 0 TO 10
NEXT
```


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

ENDPROC or <- cannot be used in the main function but END can.  END can also
be called from inside a function or a procedure.

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

Keywords that act like functions, e.g., LEN, ASC, COS, SIN and RND, are parsed like
functions, without requiring the FN prefix.  This means that their parameters must be
enclosed within brackets even if there is only one parameter.  In BBC BASIC you could
write SINA but in Subtilis you must write SIN(A)

### Error Handling

Error handling in a Subtilis is a little different from BBC Basic.  When an error
occurs inside a procedure of function, one of three things happen.

1. The program will jump to the nearest approoriate error handler defined in
   the procedure or function.
2. If there are no appropriate error handlers and we're not in the main procedure,
   the procedure or function will return, and we check for an error handler in the
   calling procedure or function, executing steps 1, 2 or 3 as appropriate.
3. If there are no appropriate error handlers and we're in the main procedure
   the process will exit.

Error handlers are defined using the ONERROR statement.  They are compound blocks
that are terminated by ENDERROR.  An error handler can contain any number of
statements and can call functions or procedures, although special rules apply
in such cases.  For example, the following code

```
ONERROR
    PRINT 0
ENDERROR

PRINT 1

ERROR 29
```

 will print

```
1
0
```

When control is passed to an error handler normal execution of that function ceases.
It is not possible to return to the statement that caused the error, or indeed the
subsequent statement.  Once the error handler has finished excuting control will
transfer to any other available error handlers in the same function or control will
return to the calling function.  By default, errors are not consumed by error handlers.
The error will still be present in the calling function.  This behaviour can be
overriding by explicitly returning from the error handler using ENDPROC or <-
as appropriate.  For example, the following code will print

```
PROCFail
PRINT 2

DEF PROCFail
    ONERROR
        PRINT 0
    ENDERROR

    PRINT 1

    ERROR 29
ENDPROC
```

will print
```
1
0
```

This is because the error has not been consumed by PROCFail and it has been propegated
to the main procedure.  As this procedure does not have an error handler, we exit the
program.  However, if we modify the code to add an ENDPROC after PRINT 0, e.g.,

```
PROCFail
PRINT 2

DEF PROCFail
    ONERROR
        PRINT 0
	ENDPROC
    ENDERROR

    PRINT 1

    ERROR 29
ENDPROC
```

we will get

```
1
0
2
```

Procedures or functions, including the main procedure, can have multiple error handlers.
These handlers are lexically scoped.  When an error handler finishes executing control
will pass to the previously declared error handler in the same scope.  If there
are no more error handlers the procedure or function or program will exit.

For example,

```
ONERROR
        PRINT 0
ENDERROR

FOR I% = 0 TO 10
    ONERROR
       PRINT 1
    ENDERROR

    ERROR 29
NEXT
```

will print

```
1
0
```

while

```
ONERROR
        PRINT 0
ENDERROR

FOR I% = 0 TO 10
    ONERROR
       PRINT 1
    ENDERROR
NEXT

ERROR 29
```

will just print

```
0
```

As we have seen, error handlers are chained by default.  This behaviour can be
overridden by including an END, ERROR, ENDPROC or <- in an error handler.  When this
is done the error handler will cause the program to return to the calling
function ignoring any other error handlers declared in the procedure that
generated the error.  For example,

```
ONERROR
        PRINT 0
ENDERROR

FOR I% = 0 TO 10
    ONERROR
       PRINT 1
       END
    ENDERROR

    ERROR 29
NEXT
```

will simply print

```
1
```

Procedures and functions can be called inside an error handler but they cannot
generate errors.  Any errors that they do generate are discarded.  ERROR can be
called from inside an error handler.  The main motivation for doing this would
be to alter the code of the error being returned.  Currently, the ERROR keyword
only accepts a number as we don't yet have any strings in Subtilis.

When an error is propegated from a function, the return value of that function
is set to the default value for the type.  This is sort of irrelvant as that
value cannot be accessed by the program.

### Arrays

Arrays variables can be declared in two different ways in Subtilis.

1. Using the DIM keyword as in BBC Basic
2. Using an array reference

Unlike BBC Basic, arrays are always reference types in Subtilis.  In BBC Basic
arrays behave like value types in some cases and reference types (when passed to
procedures) in other cases.  In Subtilis arrays are always reference types.  When
you delcare an array with the DIM statement two separate things happen. You allocate
space on the heap to store the array and you initialise a reference to that array
which can be used to refer to it there after.

The DIM keyword works in mostly the same way as it does in BBC Basic with the exception that
arrays are currently limited to a maximum of 10 dimensions, an arbitrarily chosen limit, which may change.
Arrays can be declared inside functions but they need to be delcared local.

For example,

```
DEF PROCAlloc
    DIM a%(10000000)

    PRINT 1
ENDPROC
```

will not compile.  The reason is that new global variables cannot be created inside a function
or a procedure  (that isn't the main function).

You need to write

```
DEF PROCAlloc
    LOCAL DIM a%(10000000)

    PRINT 1
ENDPROC
```

An array's type includes the following pieces of information.

1. The type of the elements it stores.
2. The number of dimensions.  This is fixed at compile time.
3. The size of each dimensions.

Dimension sizes may or may not be known at compile time.  It's better if they are as this
allows the compiler to do more checking at compile time, rather than runtime.  Currently,
the statement DIM a%(b%, c%) will create a two dimensional array whose dimensions are dynamic,
and not known at compile time.

Array references are simply defined using the normal variable creation mechanism.  The names
of array reference variables must conform to a specific pattern.  You have an identifier,
followed by a type symbol (except for reals) followed by opening and closing round brackets,
e.g.,

a%() or f()

The name of the reference only contains partial type information.  We can tell that it is an
array and what the underlying element type is but not how many dimensions that array has, or
what the sizes of the array may be.  This information is associated with the reference when
it is first created.  By default an array reference assumes the type of its source.

For example,

```
DIM a%(10, 10)
b%() := a%()
```

In this example we create a new array reference b%().  The type of this variable is a reference
to a two dimensional integer array of dimensions 10 and 10.  The example also shows how to
take a reference to an array that has already been declared.  The reference is formed from the
array's name followed by opening and closing round brackets, e.g., a%().

Any attempt to assign an array of other dimensions to b% would fail, e.g.,

```
DIM c%(10, 20)
b%() = c%()
```

would result in a compile error.

*The following three paragrapghs describe functionality that is not yet
 implemented. It is not currently possible to explicitly specify the dimensions
 of an array reference.  We probably want to be able to do this, particularly to
 be able to create a reference that has all dynamic dimensions.*

It is however possible to explicitly define the dimensions of a reference when it is created.
This can be done by specifying a constant number of dimensions followed by an optional number of
constants, one for each dimension.  Specifying a dimension of 0, will result in the dimension
being treated as a dynamic dimension.

For example,

```
DIM a%(10, 10)
DIM c%(10, 20)
b%(2, 10, 0) = a%()
b%() = c%()
```

would generate no compile errors as the second dimension of the array refernce is dynamic and
is not known until run-time.  It is not necessary to specify values for all the dimensions.
The compiler will assume that any dimensions for which values are not specified are dynamic,
for example,

```
DIM a%(10, 10)
DIM c%(10, 20)
DIM d%(100, 20)
b%(2) = a%()
b%() = c%()
b%() = d%()
```

The array reference b% is a two dimensional integer array reference with two dynamic dimensions.

It is an error to specify dimensions for an array reference that has already been created, e.g.,

```
b%(2) = a%()
b%(2) = a%()
```

The second assignment statement will result in an error.

All arrays are currently allocated on the heap and are reference counted.  This is likely to
change at some point in the future once we have escape analysis, but for the time being
there's a small overhead in declaring arrays.

Arrays can be passed to functions and procedures and returned from functions.  The syntax of array
parameter declaration is similar to that of array reference declaration with the exception that
the number of dimensions is not optional, for example.

```
DEF FNDouble%(1)(a%(1))
   LOCAL i%
   FOR i% = 0 TO DIM(a%(1))
     a%(i%) += a%(i%)
   NEXT
<-a%()
```

That may look a bit weird but that's how it currently works.

*Note the following feature is not currently supported.  You can't specify explicit dimension
sizes for either the arguments or the return types of a function.*

The dimension of that array can be specified as well so that it is known at compile time, e.g.,

```
DEF FNDouble%(1,10)(a%(1,10))
```

which looks even weirder.  You've got to put the return type information somewhere.

When the function is invoked the number of dimensions of the return type must also be specified.  For example,

```
DIM a%(10)
b%() = FNDouble%(1)(a%())
```

### Array initialisers

Array initialisers are supported but with one restriction.  The values in the
initialiser list must be constants.  So,

```
dim a%(4)
a%() = 1, 2, 3, 4, 5
```

works fine, where as,

```
dim a%(4)
b% := 2
a%() = 1, b%, 3, 4, 5
```

will not compile.

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

### Zero argument functions that return arrays

Functions and procedures with zero arguments can be currently invoked with and
without round brackets, e.g., the following two statements are both valid.

```
PROCDo
PROCDo()
```

There's one exception however.  The round brackets are mandatory for no argument
functions that return arrays, e.g.,

```
a%() := FNArr(1)()
def FNArr(1)
   dim a%(10)
<-a%()
```

This is a little inconsistent but going forward we will probably make the round
brackets mandatory for all function calls.  Once we implement functions and
procedures as first class types we will need a way to distinguish between
function calls and function pointers.

### Temporary arrays cannot be indexed

There's no technical reason that this isn't supported apart from the fact that
it would make the code hard to read, but it's not currently possible to directly
index an array returned from a function.  It needs to be assigned to an array or
an array reference first.  So the follow code,

```
PRINT FNDouble%(1)(b%())(1)
```

will not compile, and to be honest, the langauge is all the better for this restriction.

## Unimplemented Language Features

### Assembler

There's no assembler, either inline or otherwise.  I have most of the code for this so it
will be implemented at some point.

### Other missing items

Here's a list of other language features that are currently not implemented but which will be at some point

* The @% variable
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
