#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Expecting one argument."
    echo "usage: ./scripts/linux/CMakeGCC.sh BUILD_TYPE"
    exit 1
fi

rm -rf bin/*
cd bin

# Generating the Makefile
# Run cmake gui to debug cmake problem
cmake .. -DCMAKE_BUILD_TYPE=$1

# Compile project
cmake --build . --target spider2 -- -j$(nproc) VERBOSE=1

