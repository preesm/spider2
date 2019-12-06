#!/bin/sh
set -e

which lcov 1>/dev/null 2>&1
if [ $? != 0 ]
then
    echo "You need to have lcov installed in order to generate the test coverage report"
    exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "Expecting one argument."
    echo "usage: ./scripts/linux/runTest.sh TEST_NAME"
    exit 1
fi

# Configure project with cmake
cd bin
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Compile project
cmake --build . --target spider2 -- -j$(nproc)
cmake --build . --target $1-spider2-test -- -j$(nproc)

# Run test
./bin/$1-spider2-test --gtest_output="xml:./report-$1-spider2-test.xml"
