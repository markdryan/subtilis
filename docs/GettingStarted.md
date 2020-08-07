# Getting Started

## Building Subtilis

Subtilis can be run on modern computers as a cross compiler or directly on a RiscOS 3 or RiscOS4 machine such as a RiscPC.  To build Subtilis for Linux or MacOS simply type.

```
git clone git@github.com:markdryan/subtilis.git
make -j
```

You will end up with a binary called basicc (this name will change at some point)

The basicc program accepts no options (yet) and only accepts one parameter, a path to a Subtilis program.  To build your first Subtilis program type

```
./basicc examples/circle_shrink
```

This will create a file called RunImage in your local directory.  RunImage is a static, absolute binary.  Copy this binary to an Archimedes, set the file type to Absolute and then click on it to run it.  There are no shared libraries to worry about.

## Building Subtilis for RiscOS

Subtilis on RiscOS is built using the DDE.  To build it simple copy the source files to your RiscPC or Archimedes and click on the Mk obey file.  It can take 5-10 minutes to build on a RiscPC.
 


