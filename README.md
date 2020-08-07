# Subtilis

[![Build Status](https://travis-ci.org/markdryan/subtilis.svg?branch=master)](https://travis-ci.org/markdryan/subtilis)


A BASIC compiler for retro computers.

BASIC is over 50 years old and may not be the most respected of computer
languages, but it, at least the BBC variant of BASIC, is not that bad really.
It's just missing a few things, like structures, maps, memory management, a
standard library, function pointers, vectorization and a decent
compiler and tool chain.  There's a suspicion that adding these features to the
language in a natural way without compromising its simplicity, might result in
quite a nice programming environment.  At least this is the theory to be put to
the test in the Subtilis project.

The tentative plan for the project is to first create a compiler for a subset of
the existing BBC BASIC V features and then figure out a way of adding modern
constructs to the lanuage.  To start with there will probably only be two target
OSes, RISCOS 3 and RISCOS 4, and one CPU family, ARM 2 or greater.  Ultimately
the goal is to create a backend for the native ARM processor mode of PiTubeDirect.

Subtilis is not a BBC BASIC compiler and it never will be.  It ressembles and is
inspired by BBC BASIC, but it will not compile BBC BASIC programs.  It is likely
to diverge more and more from BBC BASIC as its development continues and new
features are added.  It should, however, be fairly easy to modify existing BBC
BASIC programs so that they can be compiled with Subtilis.

Please see the Subtilis [documentation](https://github.com/markdryan/subtilis/blob/master/docs/Subtilis.md) for more details.

Subtilis is still very much in its infancy and is not recommended for use by anyone
other than it's author.  Nevertheless, if you want to play around with it, please visit
the [GettingStarted](https://github.com/markdryan/subtilis/blob/master/docs/GettingStarted.md) page.