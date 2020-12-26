# Getting Started

## Building Subtilis

Subtilis can be built and run on modern computers as a cross compiler or directly on a RiscOS 3 or RiscOS 4 machine such as a RiscPC.  To build Subtilis for Linux or MacOS you'll need a C99 compiler and a make tool.  Once you have these installed simply type

```
git clone git@github.com:markdryan/subtilis.git
make -j
```

You will end up with two binaries called subtro and subptd.  subtro compiles Subtilis programs for RiscOS 3 and RiscOS4 while subptd targets the native ARM mode of PiTube direct.

The compilers accept no options (yet) and only accept one parameter, a path to a Subtilis program.  To build your first Subtilis program for RiscOS type

```
./subtro examples/circle_shrink
```

This will create a file called RunImage in your local directory.  RunImage is a static, absolute binary.  Copy this binary to an Archimedes, set the file type to Absolute and then click on it to run it.  There are no shared libraries to worry about.

To build your first Subtilis program for the Native ARM mode of PiTube direct, type

```
./subtptd examples/banner
```

The PiTubeDirect binaries are compiled to run at 0000F000.  To run, copy them on your Acorn 8-bit of choice, and type

```
*LOAD RunImage 0000F000
*GO 0000F000
```

You may need to rename the binary so that it has a shorter name to load and execute it.

## Building Subtilis for RiscOS

Subtilis on RiscOS is built using the DDE.  To build it simply copy the source files to your RiscPC or Archimedes and click on the Mk obey file.  It can take 5-10 minutes to build on a RiscPC.
 


