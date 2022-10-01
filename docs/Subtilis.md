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

EVAL will probably never be implemented, at least in the form that it appears in
BBC BASIC, as this would require run time type information which Subtilis does not
generate and will probably never generate (to keep binaries small).  A limited version
of EVAL may be possible where variables that can be accessed by the EVAL expression
are specified at compile time, but this will need a lot of thought and isn't a
priority.

#### Line numbers, GOTO and GOSUB

Subtilis does not use line numbers.  Trying to compile a program with line numbers
will result in an error.  As there are no line numbers there are also no GOTOs
(explicit or implicit) and no GOSUBs.  GOTO and GOSUB will probably never be
implemented.  GOSUB is essentially useless.  While GOTO can be useful for writing
cleanup code and for handling errors, there are better ways to do these sort of
things today, e.g, ON ERROR.

#### The '*' method of executing OS commands

This can't be done in Subtilis as it relies on newlines being part of the grammar
which they're not.  OSCLI is implemented however.

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
There is no need to insert an END statement before the first procedure of function definition.

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

When a new variable is defined using var = or LET var = or DIM and that variable has not
been declared LOCAL, the compiler will try to create a new global variable.  However,
Subtilis is much more restrictive than BBC BASIC when creating global variables.
Global variables can only be created at the top level of the main procedure before any
procedures or functions have been called.  So although they can be read from and written
to anywhere in the program, they cannot be declared,
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

or

```
DEF PROCFor
    FOR I% := 1 TO 10
        PRINT I%
    NEXT
ENDPROC
```


which will be much faster, will generate less code and won't interfere with
other parts of your code.  Note the programs in the three examples above
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

The restriction that global variables must be defined before any procedures or functions
have been called is also quite restrictive.  For example,

```
PROCA
a% = 10
```

will not compile.  Neither, unfortunately, will

```
a% = FNCompute%
```

This restriction is temporary and will be removed at some point in the future,
but for the time being it remains in place and ensures that global variables
are not accessed inside functions and procedures before they are initialised.

The LOCAL keyword can be used anywhere inside a function or procedure.  Local variables,
are scoped.  Local variables defined in a block are only available within that block.
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

Local variables declared at the top level of the main function are not allowed
to shadow global variables.  For example,

```
dim a&(1)
a& := 10
```

will not compile.

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

### Variables Created by FOR and RANGE

The FOR and RANGE statements are fairly unique in Subtilis as they are the only
statements, apart from DIM and LET that can create new variables.  When a FOR
or a RANGE statement appears at the top level of the main function they can be
used to create global variabes.  For example,

```
for a% = 0 to 10 next
```

is equivalent to 

```
a% = 0
for a% = 0 to 10 next
```

However, when FOR or RANGE is used to create local variables, these variables are
local to the loop.  They cease to exist once the loop goes out of scope.  Thus

```
range a% := b%{}
endrange
print a%
```

will not compile as a% ceases to exist once the loop has finished.


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

### SWAP

SWAP can be used to swap two variables of the same type.  It can be used for any type of variable
and works mostly as expected with the exception that both l-values must be of the
exact same type.  For example, BBC BASIC V permits,

```
SWAP a%, b
```

but Subtilis does not.  This will result in an error when tried in a Subtilis program.
Another implication of this restricition is that it is not possible to swap arrays that
have the same element type but have different dimensions.  For example, in Subtilis you
cannot do

```
dim a%(1)
dim b%(1, 1)
swap a%(), b%()
```

This restriction also applies to arrays with the same number of dimensions if the size of
the arrays are known to the compiler.  For example,

```
dim a%(10)
dim b%(9)
swap a%(), b%()
```

will not compile as both a%() and b%() have a different type.  This restriction does not
apply to vectors as the size of a vector is not encoded within its type.

You can however, swap elements of an array or vector with their scalar counterparts as
both l-values are of the same type, e.g.,

```
a% := 10
local dim b%(0)
b%() = 7
swap b%(0), a%
```

### Keywords that act like Functions

Keywords that act like functions, e.g., LEN, ASC, COS, SIN and RND, are parsed like
functions, without requiring the FN prefix.  This means that their parameters must be
enclosed within brackets even if there is only one parameter.  In BBC BASIC you could
write SINA but in Subtilis you must write SIN(A)

### Error Handling

Error handling in a Subtilis is a little different from BBC BASIC.  When an error
occurs inside a procedure of function, one of three things happen.

1. The program will jump to the nearest appropriate error handler defined in
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
subsequent statement.  Once the error handler has finished executing control will
transfer to any other available error handlers in the same function or control will
return to the calling function.  By default, errors are not consumed by error handlers.
The error will still be present in the calling function.  This behaviour can be
overridden by explicitly returning from the error handler using ENDPROC or <-
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

ERROR can be called from inside an error handler.  The main motivation for doing this would
be to alter the code of the error being returned.  Currently, the ERROR keyword
only accepts a number.  Long term it will probably accept a structure when
structures have been added to the language.

When an error is propagated from a function, the return value of that function
is set to the default value for the type.  This is sort of irrelvant as that
value cannot be accessed by the program.

It's possible to trap an error and to prevent it from terminating the execution of the current
function or indeed the program.  This can be done using the try keyword either in expression
or statement form.  The try expression is a compound statement that prevents any errors that
occur within in from propagating to the nearest error handler.  If an error occurs inside a try
block, execution of that block terminates and error value is returned. If the try block completes
successfully the expression evaluates to 0.  The try expression is terminated by an endtry.

For example,

```
x% := try
    PROCOne
    PROCTwo
    error 18
    PROCThree
endtry
print x%
```

Assuming PROCOne and PROCTwo complete without error, the try statement will abort execution in its
third line and will return the value 18.  This value will be stored in x% and printed to the screen.

The try statement is very similar to the try expression except that it returns no value.  Any error
generated during the execution of the try statement is thrown away.  The try statement is useful
if you don't care about the error code, which is often the case when writing error handlers.

One place where it is important to use try blocks is in error handlers.  The compiler will not let
an error, other than those generated by an explicit call to ERROR, be propegated from an
error handler.  To avoid the compile error a try statement is needed.

For example,

```
onerror
    local dim a%(10)
enderror
```

will not compile, as dim allocates memory which can fail.  

```
onerror
    try
        local dim a%(10)
    endtry
enderror
```

will compile.

Error handlers however, can not appear in try blocks, e.g.,

```
try
    onerror
        local dim a%(10)
    enderror
endtry
```

will not compile.

Sometimes you only want to trap the error from a single statement.  This
can be done using the try/endtry statements, but typing endtry for a
single statement is tedious.  For this reason, Subtilis also supports
the tryone keyword, which allows you to trap the error from a single
statement, and does not require a block termination statement, i.e., there
is no endtryone.  Note that the statement can be a compound statement so
you can write code like:

```
if tryone for i% := 0 to 10 PROCMayFail next then
    print "Failed"
endif
```

### Strings

Strings work much in the same way as they do in BBC BASIC with the exception that there
is no limit on the length of strings apart from the memory made avaiable to the
program.

Strings in Subtilis have value semantics as they do in BBC BASIC.  However, strings are
actually implemented as copy on write reference types in Subtilis.  Assigning one string
to another does not the copy source's data.  It simply increases its reference count.  This makes
assigning one string to another and passing and returning strings to and from functions
nice and quick.  There is a price to pay when the string is modified however.  When
modifying a string using += or the statement form of left$, mid$ or right string$, a
copy of the target string may need to be made.  The copy only happens if the reference
count of the underlying string is > 1, but even in this case there is still a price to be
paid as the reference count check needs to be done at runtime, so there's a small performance
penalty.  Also, as we need to build these checks into our compiled code, language constructs that
modify strings tend to generate more code than constructs that read from strings.

### Arrays

Arrays variables can be declared in two different ways in Subtilis.

1. Using the DIM keyword as in BBC BASIC
2. Using an array reference

Unlike BBC BASIC, arrays are always reference types in Subtilis.  In BBC BASIC
arrays behave like value types in some cases and reference types (when passed to
procedures) in other cases.  In Subtilis arrays are always reference types.  When
you declare an array with the DIM statement two separate things happen. You allocate
space on the heap to store the array and you initialise a reference to that array
which can be used to refer to it there after.

The DIM keyword works in mostly the same way as it does in BBC BASIC with the exception that
arrays are currently limited to a maximum of 10 dimensions, an arbitrarily chosen limit, which may change.
Arrays can be declared inside functions but they need to be declared local.

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

*The following three paragraphs describe functionality that is not yet
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

would generate no compile errors as the second dimension of the array reference is dynamic and
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

### Array Memory Management and Arrays as arguments

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

Both forms of array initialisers are supported.

```
dim a%(4)
a%() = 11;
```

Sets all 5 elements of a%() to 11 and 

```
dim a%(4)
a%() = 1, 2, 3, 4, 5
```

Sets the elements of a%() to 1, 2, 3, 4, 5.

There's one restriction when using the second form of array initialisation.  The values in the
initialiser list must be constants.  So,

```
dim a%(4)
b% := 2
a%() = 1, b%, 3, 4, 5
```

will not compile.  This restriction only applies when multiple elements are specified in the 
initialiser list.  

```
dim a%(4)
v% := 11
a%() = v%
```

Compiles and works as expected.

### Vectors

Subtilis supports vectors.  Vectors are similar to one dimensional arrays with 3 notable exceptions.

1. The are defined, referenced and indexed using curly brackets {}, rather than round brackets.
2. A vector can contain 0 elements.
3. The size of a vector can be increased after it has been defined using the append statement.

Vectors are declared using the DIM keyword, e.g.,

```
dim a%{10}
```

declares a vector containing 11 elements.  Zero element vectors can be created in two ways.

1. By omitting the dimension size between the {} in the dim statement
2. By specifying a negative size, e.g., -1

For example, the following code snipped declares two zero element vectors

```
dim a&{}
dim b${-1}
```

Vectors can be initialised in the same way that arrays are initialised.  Attempting to initialise
all the values of a zero element vector to the same value has no effect, e.g.,

```
dim a%{}
a%{} = 1
```

will do nothing.

Vectors can be indexed like normal arrays, e.g., using {} instead of ()

```
dim a%{1}
a%{0} = 10
a%{1} = 11
```

Note that because a vector can contain zero elements, it's generally not safe to iterate through a
vector using a for loop as for loops in BASIC behave like repeat loops instead of while loops, i.e.,
one iteration of the loop is always performed.

Vectors can be passed to and returned from functions.  This is demonstrated in the following example
which returns the sum of the individual elements of two vectors of ints in a new vector.

```
local dim a%{10}
local dim b%{5}

a%{} = 1
b%{} = 2

c%{} := FNAddVectors%{}(a%{}, b%{})
i% := 0
while i% <= dim(c%{}, 1)
  print c%{i%}
  i% += 1
endwhile

def FNAddVectors%{}(a%{}, b%{})
  a_len% := dim(a%{}, 1)
  b_len% := dim(b%{}, 1)
  if b_len% < a_len% then
    a_len% = b_len%
  endif
  local dim ret%{a_len%}
  i% := 0
  while i% <= a_len%
    ret%{i%} = a%{i%} + b%{i%} 
    i% += 1
  endwhile
<-ret%{}
```

The size of a vector can be determined using the dim statement as shown
in the example above.

It's also possible to create vector references, e.g.,

```
local dim a&{}
b&{} := a&{}
```

Vectors can be used pretty much anywhere arrays can be used, e.g., in the put#, get# and copy statements.

It's not currently possible to assign an array reference to a vector reference and vice-versa.
This may be permitted in the future if it turns out that there's a need for this sort of thing.


### Copying

Arrays and vectors are reference types.  When you assign one array to another you really just copy
the pointer to the data and some additional array meta data.  The actual contents
of the array are not copied.  For example,

```
dim a%(10)
dim b%(10)
a%() = 10
b%() = a%()
a%(0) = 11
print b%(0)
```

Will print 11.  This is because a%() and b%() point to the same underlying data.  Sometimes
you want to actually duplicate data rather than references.  This could be done using a for
loop for example

```
for i% := 0 to 10
  a%(i%) = b%(i%)
next
```

but this currently generates very inefficient code, as a bounds check is performed on every
array access.  A better way to do this is with the copy statement, which eliminates most of
the bounds checks, for example

```
dim a%(10)
dim b%(10)
a%() = 10
copy(b%(), a%())
a%(0) = 11
print b%(0)
```

will print 10.

The copy statement is flexible and allows you to mix types but there are some rules.

1. The first argument must be an array or vector reference or a non-constant string.
2. The second argument must be an array or vector reference, a non-constant string or a constant string.
3. The types of both arguments do not need to match as long as both arguments are either an array of a numeric type or a string.
4. When the first argument is an array or vector of strings, the second argument must also be an array or vector of strings.
5. Unless you are copying arrays or vectors of strings, the copy statement does a memcpy.  Thus there is no type conversion.  You can copy from
   an array of bytes or ints into an array of real numbers, but unless the data in the array of bytes contains valid binary representations.  Some restrictions may be introduced here.  Not sure how useful it is to copy an integer or a string into a real.
   of floating point numbers, the contents of the real array will not be meaningful.
6. The sizes and dimensions of the arguments do not need to match.  The amount of data copied is 
   min(size of first arg in bytes, size of second arguments in bytes).
7. If the size the second array is not a multiple of the element size of the first array, the final element copied into the destination array will be partially written.  (This may change.  Not sure how useful this is.)
8. If the first argument contains more data, n bytes, than the second argument k bytes, only the first k bytes of the
destination array will be overwritten.  The final n-k bytes of the first argument will be unmodified.


Here's an example of copying a string into a byte array

```
dim a&(5)
a&() = 33
copy(a&(), "hello")
for i% := 0 to dim(a&(),1)
  print chr$(a&(i%));
next
print ""
```

The example will print "hello!"

Two forms of the copy statement are provided.  The statement form, which is depicted in the examples
above, and an expression form.  The expression form evaluates to its first argument, the expression
form is useful when the destination is a temporary, e.g., a value returned from a function or keyword,
e.g.,

```
print copy(FNNewString$, "hello world")
```

### Appending

Subtilis provides a new keyword, called append that allows the programmer to append elements to
a string or a vector.  As with the copy statement,  expression and statement variants of the keyword are provided.
Append takes two parameters.

The first parameter must be a vector or a string.  If the first parameter is a string, the second
parameter must also be a string, e.g.,

```
append(a$, "hello")
```

which is equivalent to writing

```
a$ += "hello"
```

If the first argument is a vector the type second argument can be either the type of elements
stored in the vector, or it can be a vector or array (of any dimension), providing it contains
the same type of elements as the first argument.  For example,

```
local dim a${}
local dim b$(2)

b$() = "to", "the", "world"
append(a${}, "hello")
append(a${}, b$())

print a${0}
print a${1}
print a${2}
print a${3}
```

will print "hello to the world".

### Slices

It is possible to slice vectors and one-dimensional arrays to create new collections that contain
a portion of the elements of the original collection.  Slicing a vector creates
a new vector and slicing a 1 dimensional array creates a new 1d array.  Slicing is performed using the ':'
operator, which is preceded and followed by two optional integer values.  The first value specifies
the index of the first element that is to be part of the new slice.  The second value specifies the
first element that will not be part of the new slice.  Each of these integers are optional.
If the first integer is missing the compiler assumes a default value of 0.  If the second integer is
missing the compiler assumes a default of the number of elements in the collection that is being sliced.

For example, in the following code snippet

```
local dim a%{10}
a%{} = 1,2,3,4,5,6,7,8,9,10,11
b%{} := a%{1:3}
c%{} := a%{:3}
d%{} := a%{7:}
e%{} := a%{:}
```

The vector b%{} contains two elements 2 and 3, c%{} contains 3 elements 1,2 and 3, d%{}
contains 4 elements 8, 9, 10 and 11, and e% contains the same elements as a%.  In fact,

```
e%{} := a%{:}
```

is equivalent to writing
```
e%{} := a%{}
```

When slicing a vector it is possible to create an empty vector, for example

```
local dim a%{10}
a%{} = 1,2,3,4,5,6,7,8,9,10,11
print dim(a%{1:1}, 1)
```

will output -1, indicating that the new slice contains 0 elements.  It is not possible to create a
zero element array when slicing, as the language does not allow zero element arrays.  So if we try
to rewrite the previous example using arrays instead of vectors we'll get a compile error, e.g.,

```
local dim a%(10)
a%() = 1,2,3,4,5,6,7,8,9,10,11
print dim(a%(1:1), 1)
```

will not compile.

When slicing vectors the first index must always be less than or equal to the second index.  When slicing
arrays the first index must be less than the second index.  In all cases however, the second index must be
no greater than the number of elements in the array or vector.  The constraints are enforced by the language,
at compile time where possible and at run-time otherwise.  For example,

```
def PROC_slicer(b%)
    local dim a%(10)
    print dim(a%(b%:b%), 1)
endproc
```

will generate a runtime error (as we're trying to create an array slice with 0 elements).

When a vector or an array is sliced, a new container is created that points to the data of its parent.  The
block of data holding the actual elements is now shared between the original collection and the slice, much the
same way as it is when you initialise one array reference with the contents of another array.  If you then
write to the slice, the contents of the original collection will also change.  However, it should be noted that
vectors are copy on append (assuming that their elements are referred to by another vector), so if you slice a
vector and then append to the new slice, the slice and the original vector will no longer share the same set
of elements.  Modifying the elements of the slice will have no effect on the vector.  For example,

```
dim a%{5}
a%{} = 1,2,3,4,5,6

b%{} := a%{1 : 3}
b%{0} = 100
b%{1} = 101

range v% := a%{}
  print v%;
  print " ";
endrange
print ""

append(b%{}, 102)
b%{0} = 99
range v% := a%{}
  print v%;
  print " ";
endrange
print ""

range v% := b%{}
  print v%;
  print " ";
endrange
print ""
```

Will print

```
1 100 101 4 5 6
1 100 101 4 5 6
99 101 102
```

Slices can be used in combination with the copy statement to build up arbitrary structures in memory
containing mixed types.  This is useful when you need to have precise control over the layout of data
in memory, for example when programming the WIMP is RiscOS.  For example, a procedure to display an
error dialog in Subtilis could be written like this:

```
def PROCdisplayError(e$)
    local dim eblock&(len(e$) + 4)
    eblock&(0) = 255
    copy(eblock&(4:), e$)
    sys "Wimp_ReportError", eblock&(), 1, app$
endproc
```

Note how we use the copy statement in conjunction with a slice of the byte buffer to copy the contents
of e$ to the buffer starting at the 5th element (index 4).


### Function Pointers, Lambdas and Higher Order Functions

Subtilis allows the address of existing functions to be taken, stored in a variable and
passed to and returned from functions.  It also allows the creation of lambda or unnamed
functions.  Before a lambda function or the address of a function or procedure can be assigned
to a variable or passed to and from a function a new type must be created to represent the type
of the variable.  Types are created with the new *type* keyword.  The *type* keyword is followed
by a function prototype.  For example,

```
type FNReduce%(a%, b%)
```

Defines a new type called *FNReduce* which represents functions that take two integer input
parameters and that return an integer.  The names of the parameters is unimportant in the type
statement, but the types of the parameters are.  Note that,

```
type FNReduce%(a%, b%)
```

and

```
type FNReduce%(ONE%, TWO%)
```

are identical.  Also note that the name of the type does not include the return type of the function, i.e.,
the name of the type we just defined is *FNReduce* and not *FNReduce%*.

Instances of user defined types are declared using the **@** operator.  A variable name is followed by **@** which is
followed by the type name.  So to create a variable that points to a function that takes two integer arguments
and that returns an integer one might type.

```
local a@FNReduce
```

It's also possible to create user defined types that store pointers to procedures, simply by replacing FN
with PROC, e.g.,

```
type PROCDoSomething(a$)
local b@PROCDoSomething
```

Is a type that represents a procedure that takes a single string argument.


As with all other variables, function pointers that are not explicitly initialised are set to their zero
value.  What that means, differs depending on the type of the function.  The zero value for any user defined
procedure type is a procedure that does nothing.   The zero value for functions type that return numeric types
is a pointer to a function that returns the zero value of the appropriate type.  The zero values for strings
and vectors are pointers to functions that return empty strings and vectors, respectively.  The zero value for a
function returning an array is a special case.  It's a pointer to a function that returns an array of the
correct dimensions that contains a single element.  This is because Subtilis requires that all arrays have
at least one element.  

To invoke a function through a function pointer, the name of the function needs to be precedeed by all the
relevant arguments in brackets.  Note the brackets are mandatory, even if the procedure or function type
accepts no arguments.

So for example,

```
print a@FNReduce(1,1)
b@PROCDoSomething()
```

will print out 0, the value returned by the expression a@FNReduce(1,1) which invokes the zero value
for function pointers returning integers.

Variables of function pointers can be initialised in two ways.  They can be assigned the pointer of a
name function or procedure declared anywhere in the source file, or they can be initialised with a lambda function.

The **!** operator is used to take the address of a named function.  You simply prefix the function name with the
**!** operator.  Note that in Subtilis, the function name contains the full return type of the function, unlike the
type name.  So to initialise a@FNReduce to point to a named function we might do something like this.

```
type FNReduce%(a%, b%)
a@FNReduce := !FNAdd%
print a@FNReduce(1, 2)

def FNAdd%(a%, b%) <- a% + b%
```

Lambda functions are function or procedure definitions without a name that can be used in an expression
context.  For example, let's create another instance of FNReduce and assign it a lambda function.

```
b@FNReduce = def FN%(a%, b%) <- a%*b%
print b@FNReduce(1,2)
```

This code snippet should output 2.

Arrays and vectors of function pointers in the same way as they are for all the other types.
For example,

```
type PROCprinter(a$)
dim vec@PROCprinter{1}
vec@PROCprinter{} = def PROC(a$) print "hello " + a$ endproc

append(vec@PROCprinter{}, def PROC(a$) print "goodbye " + a$ endproc)

range a@PROCprinter := vec@PROCprinter{}
    a@PROCprinter("Subtilis")
endrange
```

should output

```
hello Subtilis
hello Subtilis
goodbye Subtilis
```

Note *hello Subtilis* is output twice as the vector initially contained two elements and both of
those elements were initialised to point to the first lambda function.

Function pointers can be passed to functions and returned from them.  For example,

```
type FNReduce%(a%, b%)
dim vec@FNReduce{1}
vec@FNReduce{} = FNMakeAdder@FNReduce, FNMakeMultiplier@FNReduce

range a@FNReduce := vec@FNReduce{}
    PROCReducer(10, 10, a@FNReduce)
endrange

def FNMakeAdder@FNReduce <- def FN%(a%, b%) <- a% + b%
def FNMakeMultiplier@FNReduce <- def FN%(a%, b%) <- a% * b%

def PROCReducer(a%, b%, fn@FNReduce)
    print fn@FNReduce(a%, b%)
endproc
```

*FNMakeAdder@FNReduce* and *FNMakeMultiplier@FNReduce* both return lambda functions and *PROCReducer*
accepts a function pointer as an argument.

It's possible to create recursive types using the type keyword.  For example,
a type can be created that represents a pointer to a procedure that accepts a function pointer as an argument.
If we wanted to take the address of PROCReducer from the previous example, we could do
so as follows.

```
type PROCHigherOrder(a%, b%, fn@FNReduce)
ho@PROCHigherOrder = !PROCReducer
ho@PROCHigherOrder(100, 3, def FN%(a%, b%) <- a% div b%)
```

As a final note, Subtilis does not support any form of closure and probably never will.

### RECords

#### Type and variable creation

Subtilis supports user defined types called records.  Before a record can be used a type
for that record must be created.  This can be done using the type keyword, also used to
create new types for function pointers.  The type keyword is followed by a record type
name which must begin with the string "REC" followed by a valid identifier name.  The
record name is the followed by one or more white space separated field declarations
enclosed by round brackets.  For example,

```
type RECPoint ( x% y% )
```

creates a new record type called RECPoint that contains two integer fields, named x% and y%.
An instance of our new type can be created using the @ operator, e.g.,

```
local a@RECPoint
```

will create a new variable called a<!-- -->@RECPoint.  When a record is created in this way all of
its fields are initialised to their zero value, so in this case, the two fields, as they
are integers, will both have been initialised to zero.

The fields can be accessed (read from and written to) using the '.' operator.  For example,

```
print a@RECPoint.x%
print a@RECPoint.y%
a@RECPoint.x% = 100
a@RECPoint.y% = 200
print a@RECPoint.x%
print a@RECPoint.y%
```

should output

```
0
0
100
200
```

#### Initialisation lists

All the fields of a record can be set all at once using an initialisation list.  This can be done when
the variable is created or at a later stage.  An initialisation list is a comma separated list
of values (which can be constants or variables) surrounded by round brackets.  For example, to create and
initialise a new instance of RECPoint, we might type

```
a@RECPoint := (100, 200)
```

The number of elements of the initialisation list must not exceed the number of fields of the record variable
being initialised or assigned to.  It is possible to specify fewer elements in an initialisation list than there
are fields in the record.  In this case the fields that aren't specified are set to their zero-value.  For example,

```
a@RECPoint = (10)
```

will set a<!-- -->@RECPoint.x to 10 and a<!-- -->@RECPoint.y to 0.  It is even possible to specify an empty initialisation list,
which will set all the fields of the record variable to their zero value.  This is useful when declaring global
record variables, as Subtilis provides no equivalent of the local statement for global variables.  Thus the
statement

```
glob@RECPoint = ()
```

appearing in the top level of the program will create a new global variable of type RECPoint and will set both
fields to zero.

#### Nested records

The fields of a record can be of any type, including other records, strings or arrays.  There's one exception,
recursive record definitions are not allow (without using record pointers).  For example,

```
type RECShape (
  name$
  name@RECPoint
  dim vertices@RECPoint{3}
)
```

defines a record that contains a string, coordinates for the placement of that string and a vector of points
that describe a shape.  All the fields of records that contain complex types such as other records or arrays
can be initialised using an initialisation list.  Extra sets of round brackets are required to denote the contents
of a nested type or array or vector.  For example, our shape might be fully initialised as follows.

```
a@RECShape := ( "square", ( 100, 450), ( (100, 0 ), (100, 100), (100, 400), (400, 400)))
```

#### Layout and Copy Semantics

Records are not reference types.  They are value types.  When you assign one reference variable
to another the entire contents of the source variable are copied, e.g.,

```
a@RECPoint = b@RECPoint
```

copies 8 bytes of data, two integer variables.  Global record variables are placed in the main global data
area and local record variables are placed on the stack.  Records may contain fields that are reference
values, such as strings and arrays.  The meta data for reference fields are contained within the record
variable itself, but the data for those fields are stored on the heap and reference counted.  The compiler
ensures that all reference fields of a record variable are dereferenced when the record goes out of scope,
or have their reference counts increased when the variable is copied.

All fields in a record are aligned by their required alignment.  Byte fields are 1 byte aligned, integer
fields are 4 byte aligned, real numbers are 8 byte aligned, and referenced types are aligned by the size
of a pointer (which on RiscOS is 4 bytes).  The alignment of the record variable itself is determined by
its field with the highest alignment.  For example,

```
type RECalign ( a& b& c)
```

will define a record variable that is 16 bytes in size and is 8 byte aligned.  Field a& is 8 byte aligned,
as it is the first field in the structure, and occurs at offset 0 from the start of any variable defined
using this type.  Field b& is 1 byte aligned and occurs at offset 1.  Field c is 8 byte aligned and occurs
at offset 8.  As variables of type RECalign are required to be 8 byte aligned, field c will also be 8 byte
aligned.

Record variables can be used in almost all places that a variable of one of the standard types can be used.
They can be passed to procedures and functions and returned from functions.  They can be used with the copy,
append and swap keywords.  For example, the following program defines a function that adds the fields of
two records together and returns the results in a new record.


```
type RECScalar ( a% b c& )

a@RECScalar = ( 1, 10.0, 3 )
@RECScalar = ( 4, 10.0, 5 )
c@RECScalar = FNAdd@RECScalar(a@RECScalar, b@RECScalar)
print c@RECScalar.a%
print c@RECScalar.b"
print c@RECScalar.c&

def FNAdd@RECScalar(a@RECScalar, b@RECScalar)
    a@RECScalar.a% += b@RECScalar.a%
    a@RECScalar.b += b@RECScalar.b
    a@RECScalar.c& += b@RECScalar.c&
<-a@RECScalar
```

The program will print

```
5
20
8
```

Note that record variables are value types, they are passed to and returned from functions
as value types, i.e., they are copied, at least conceptually.  It should be possible for
the compiler to elide some of the copies in certain cases, but at the time of writing
(August 2022) it can't currently do this.

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

### VAL

VAL like RND is a little compiler unfriendly.  You don't always know at compile time
what the type returned by the keyword is going to be so you have to assume that it's
a real.  For this reason VAL(a$) in Subtilis converts from a string to a floating
point number using floating point arithmetic and so will be slow on most Archimedes.
For this reason, Subtilis introduces a variant on the VAL keyword which takes a
second argument.  The second argument denotes the base of the number in string
representation and when present the compiler always converts from a string to
an int using integer arthimetic.  Valid values for the base are 2-10 and 16.  For
example

```
print val(256, 10)
print val(100, 16)
```

will print

```
256
256
```


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
an array reference first.  So the following code,

```
PRINT FNDouble%(1)(b%())(1)
```

will not compile, and to be honest, the language is all the better for this restriction.

### SYS

Subtilis supports the SYS statement which can be used to make system calls.  There are however,
some restrictions and peculiarities in the implementation.  The main restriction is that the
first argument to the SYS statement, the ID of the system call, must be a constant, known
at compile time.  It can be a string or an integer, but it must be a constant.  Without this
restriction it would be necessary to use self modifying code to invoke the system call on
32 bit ARM and this is messy as different CPUs, supported by RiscOS have different requirements,
related to self modifying code.  In order to be able to create a single binary that will run
on all RiscOS 3 and 4 machines I've imposed this restriction.  There are some benefits of
using constants for the call id however.

1. The compiler can perform the translation of string name to integer at compile time, e.g.,
SYS "OS_Write0" generates the exact same code as SYS 2.

2. The compiler knows which registers are read and modified by which system calls.  The is
convenient as it means the compiler only needs to preserve and restore the registers that
are modified by a given system call.  It doesn't need to save and restore all registers.  This
makes certain system calls that don't corrupt many registers nice and fast.

3. The compiler will also generate an error if you try to initialise a register that it
knows the system call doesn't use, e.g.,

```
SYS "OS_Write0", "hello", "world"
```

will not compile as the compiler knows that OS_Write0 only expects a single input argument.

The compiler will accept any type (variable or constant) as an input argument although some types
are modified before being passed to the system call.  Reals are cast to integers and strings are
zero-terminated and may be copied, RECords are passed by reference.  The only valid type for an output argument is an integer variable.

Some system calls expect buffers to be passed as arguments.  There's no way in Subtilis to create a
buffer at the moment, so you need to use integer arrays or records, e.g.,

```
type RECeigenv ( x% y% )

local DIM block%(2)
local a@RECeigenv
block%(0)=4
block%(1)=5
block%(2)=-1
SYS "OS_ReadVduVariables",block%(),a@RECeigenv
print a@RECeigenv.x%; print " "; print a@RECeigenv.y%
```

A word of caution.  As in BBC BASIC there's no obligation to provide values for all the registers
used by a system call.  For example, you can do,

```
SYS "OS_SWINumberFromString",,"OS_SpriteOp" TO sys_call_id%
```

Here, no value is specified for R0, as the SYS call doesn't use R0.  In BBC BASIC, R0 would
be initialised to 0 before the sys call is made.  This doesn't happen in Subtilis.  R0 is not
modified by the compiler before the call is made.   If you want to ensure that a register is
set to 0 before making a system call (not necessary in this example) you must explicitly pass
the value of 0 in the SYS argument list. BBC BASIC programs also seem to get away without
providing zero values for the trailing registers used by a system call, so you need to be careful
when porting programs that make SYS calls from BBC BASIC to Subtilis.  Look up each system call
and make sure you're passing all the parameters it needs even if those parameters only need to be
set to 0.  If the registers used by a system call contain unitialised data your program will behave
in mysterious ways.

In RiscOS a program can place an X in front of a system call name to indicate that it would like
to handle any error generated by the system call.  When you do this and the system call fails,
the error will be handled by Subtilis's normal error handling facility.  For example, the following
call to XOS_ConvertInteger4 will fail as it needs to write 5 bytes and we only specify a length of
2.  The failure will be trapped by the error handler.

```
dim a%(1)
onerror
  print "Should end up here"
enderror
local free%
sys "XOS_ConvertInteger4", 1999, a%(), 2 to ,,free%
print "Shouldn't get here"
```

If you don't want this behaviour you need to use the ';' syntax to read the processor's flags
register, e.g.,

```
dim a%(1)
onerror
  print "Shouldn't end up here"
enderror
local free%
local flags%
sys "XOS_ConvertInteger4", 1999, a%(), 2 to ,,free% ; flags%
print "Should get here and here are the flags ";
print flags%
```

This feature is highly platform specific.  In particular, the compiler will generate different
code for this for the RiscOS and PiTube Direct (doesn't exist yet) backends.  On ARM backends
only the N, Z, C and V flags will be returned in the top 4 bytes of the flags% variable.

It's not currently possible to invoke a system call that the compiler doesn't know about.  Long term
this feature will be added but will be restricted to using a constant integer as the syscall id.
To make matters worse, Subtilis doesn't yet know about all the existing RiscOS system calls.  Many,
e.g., Wimp calls, still need to be added.

## File Handling

Works for the most part as you would expect.  There's one syntactic different between BBC BASIC
and Subtilis though.  The ',' operator is used in place of the '=' operator in the PTR# statement.
The reason is that the use of '=' is ambiguous, for example when the compiler sees,

```
ptr# a%=10
```

It assumes the file handle to be determined not by a% but rather by the expression a%=10.  So for
this reason, in Subtilis, we write.

```
ptr# a%, 10
```

None of the file handling keywords buffer the input.  The requests are sent directly to the OS.
For this reason, Subtilis does not implement GET$#.  It would be very inefficient.  A getline
function will be provided once we have a standard library.

## New keywords

### HEAPFREE

Returns the number of free bytes available in the heap.  Only makes sense on platforms where the
amount of memory allocated to a program is fixed when it's run.  On platforms where there's
no memory restriction on an application, the maximum integer value is returned.

### GET# and PUT#

Can be used to read and write blocks of data from and to a file.

GET# takes two comma separated parameters, a file handle and a buffer of some sorts, and returns the number of items read.
The buffer can be an array containing numeric elements or a string or a record containing numeric types.  The size of the
array or string determines the maximum number of bytes that are actually read, e.g.,

```
buf$ := string$(32, " ")
chars_read% := get#(handle%, buf$)
```

will read up to 32 bytes into buf$.  If there are fewer than 32 bytes in the file, the number of bytes actually read
will be returned, but no error will be generated.  As get# can be used in an expression, its parameters are bracketed.

Note the return value from get# is not the number of bytes read but the number of complete objects read.  For example,
assuming that there were 15 bytes left in the file pointed to by handle# and the following code was executed,

```
dim buf%(10)
ints_read% := get#(handle%, buf%())
```

ints_read% would be set to 3 and not 15 as only 3 complete integers were read.




PUT# writes a buffer to a file.  The buffer can be a scalar array or a string.  The number of bytes written is
determined by the size of the array or string in bytes.  For example,

```
dim a&(32)
put# handle%, a&()
```

will write 32 bytes to the file pointed to by handle%.  An error will be generated if not all 32 bytes can be
written.

### INTZ

Zero extends from a byte variable to an integer.  See the byte type below for more information

### RANGE

The RANGE keyword introduces a new convenience looping construct for iterating through the elements
of an array or a vector.  The syntax is

```
range [local] (var|~) [,index]* (:=|=) array or vector
endrange
```

*array or vector* is an array or vector reference.  It can be of any type and any dimension.
*var* is a variable.  Its type must match the element type of the *array or vector*.  It must
cannot be an element of an array or vector, e.g., a$(0).
The *var* variable can be followed by one or more optional *index* variables.  They do not need to
be present, but if they are, the number of index variables provided must match the number of
dimensions of the *array or vector*.

Here's an example

```
dim a%(5)
a%() = 1,2,3,4,5,6
range b%, c% := a%()
  print b%;
  print ", ";
  print c%
endrange
```

This code will print

```
1, 0
2, 1
3, 2
4, 3
5, 4
6, 5
```

A '~' can be specified in place of the var variable.  This signifies to the compiler that we want
to execute the loop body once for each element in the collection, but that we don't actually care
about the elements in the collection.  Using the ~ in place of a variable name allows the compiler
to generate more efficient code as it doesn't need to generate code to copy an element out of a
collection into a variable that won't be used.  For example,

```
dim b%(3)
range ~, a% = b%()
    print a%
endrange
```

will print

```
0
1
2
3
```

If the local keyword is provided or the := assigment operator is used, then new local variables will be
created for *var* and the *index* variables.  These variables will go out of scope when the endrange
keyword is reached.  If neither local or := is specified, the *var* and the *index* are assumed
to be existing variables.  There's one exception here.  If the range loop is used in the top level of
the main function, new global variables will be created for any of the var or index variables that do
not exist..  These variables will be valid for the entire program.

For example,

```
dim a%(5)
a%() = 1,2,3,4,5,6

range b%, c% = a%()
endrange
print b%
print c%
```

will print

```
6
6
```

Range should be used in preference to for when iterating through arrays and vectors.  Bounds checking
is eliminated for assignment to *var* so

```
range v% := range a%()
  rem do something with v%
endrange
```

should be more efficient and generate less code than

```
for i% := 0 to dim(a%(), 1)
  v% := a%(i%)
  rem do something with v%
next
```

In addition, it is better to use range loops when dealing with vectors as vectors can be empty,
thus

```
dim a%{}
range v% := a%{}
  print v%
endrange
```

will do nothing, whereas

```
dim a%{}
for i% := 0 to dim(a%{},1)
  print a%{i%}
next
```

will generate a runtime error as it will try to access the first element of the vector which does not exist.
The body of a for loop is always executed at least once.

### OSARGS$

OSARG$ returns a string containing the commandline used to launch the running program.

## New Scalar Types

### Byte

Subtilis supports a byte type.  This is a signed 8 bit integer.  There are no byte constants only byte variables.  Byte variables are declared with the '&' type suffix, for example

```
a& := -1
```

declares a new byte variable and assigns it the value -1.  As byte is a signed type, byte variables are sign extended when converted to other types or printed out.  Use the INTZ keyword to zero extend a byte value instead, e.g.,

```
a& := -1
print intz(a&)
```

outputs 255 and not -1.

## Assembler

Subtilis implements an assembler, as all good BASIC implementations should.  It differs considerably from the assembler in BBC BASIC, however.  Assembly happens at compile time rather than at runtime and the syntax of Subtilis's assembler is significantly different to that of BBC BASIC.  Assembly code is included directly inside Subtilis source files and not in separate assembly only files.  A function or a procedure must be written entirely on one of the two languages.  It is not possible to mix BASIC and assembly in the same function or procedure.  Inline assembly is not permitted.

Assembly language functions and procedures are defined in the same way as BASIC functions, using the DEF PROC or DEF FN keywords.  The bodies of the functions are different, however.  The assembly language directly follows the DEF statement and is enclosed in square brackets.  Assembly language functions or procedures do not end with <- or ENDPROC.  Here's a brief example that returns the integer value 1.

```
def FNOne%
[
	MOV R0, 1
	MOV PC, R14
]
```

The function can be called from BASIC using the normal function calling syntax, e.g.,

```
PRINT FNOne%
```

As Assembly language is really a platform specific feature the assemblers (currently, there's only one) are documented separately.

* [The 32 bit ARM assembler](https://github.com/markdryan/subtilis/blob/master/docs/ARM32Asm.md)

## Unimplemented Language Features

### Other missing items

Here's a list of other language features that are currently not implemented but which will be at some point

* The @% variable
* CASE OF
* SOUND
* RETURN for passing arguments by reference to procedures and functions
* POINT TO
* INPUT
* INPUT# and PRINT#
* INSTR
* The vector operations on arrays are not implemented but will be

There are also some enhancements that will need to be added to the language to make it
more palatable to the modern programmer.

* Maps
* Allow the results of an expression to be discarded, e.g. ~= FN@RECv()
* PUT# and GET# should be able to write and read single variables (and not just arrays and vectors)
* Optimisation for append of constant strings.  Currently, it generates a temporary and appends the temporary.
* It also does this when assiging a constant string to an element in a vector.
* In a related note it would be nice to be able to append REC literals directly, without having to manually create a temporary variable first.
* Maybe some sort of way to specify the initial allocation size of a vector, to avoid reallocs, e.g., dim a${} reserve 64.


## Tooling

The tooling is very basic and needs a huge amount of work

* There's no linker so we're limited to a single source file right now.
* There's no error recovery so you only get a single error message before the compiler bombs out.
* There's no optimizer
* The compiler is too slow.  It takes 13 seconds to compile a very simple program on the A3000 (8 Mhz ARM2).


