#!/bin/bash

set -e
set -u
set -x

mkdir build || true

pushd build

cmake ..
make

test/test-creator > input 2> output.expected
cat input | ./check-parenthesis > output.actual || true
diff -q output.expected output.actual && echo "OK" || echo "Fail"

popd

