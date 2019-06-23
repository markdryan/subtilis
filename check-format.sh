#!/bin/bash
set -e
CLANG_FORMAT=clang-format
if [ -f /usr/bin/clang-format-3.9 ]; then
    CLANG_FORMAT=/usr/bin/clang-format-3.9
fi
$CLANG_FORMAT --version
for fn in *.[ch] common/*.[ch]; do
    echo ${fn}
    $CLANG_FORMAT -style=file ${fn} | diff ${fn}  -
done
