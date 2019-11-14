#!/bin/sh

which lcov 1>/dev/null 2>&1
if [ $? != 0 ]
then
    echo "You need to have lcov installed in order to generate the test coverage report"
    exit 1
fi

# Creates output binary folder
rm -rf bin 
mkdir bin
cd bin

# Configure project with cmake
cmake ..

# Compile project
make -j$(nproc)
