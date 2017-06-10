#!/bin/bash

make clean
CC=clang CFLAGS="-fprofile-instr-generate -fcoverage-mapping" make check
llvm-profdata merge -sparse default.profraw -o default.profdata
llvm-cov report  ./lexer_test -instr-profile=default.profdata 
