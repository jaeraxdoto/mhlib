#!/bin/bash
# Basic functionality test
cd demo-maxsat
rm -f test.out test.log
./maxsat seed 3 tciter 1000 oname test || exit 1
cat test.out test.log || exit 1
grep "best objective.*764" test.out
