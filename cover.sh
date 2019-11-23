#!/bin/bash

make clean
CC=clang CFLAGS="-fprofile-instr-generate -fcoverage-mapping" make -j8 check
llvm-profdata merge -sparse default.profraw -o default.profdata
llvm-cov report  ./unit_tests -instr-profile=default.profdata 
