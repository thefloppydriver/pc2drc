#! /bin/sh

srcdir=$(dirname $(dirname $0))
$srcdir/tools/cpplint.py $(find $srcdir -name "*.cpp" -or -name "*.h")
