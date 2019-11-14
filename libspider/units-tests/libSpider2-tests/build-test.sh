#!/bin/sh

if [ "$#" -ne 1 ]; then
    echo "Expecting one argument."
    echo "usage: ./build-test.sh FOLDER_NAME"
    exit 1
fi

# Creates output binary folder
rm -rf $1/bin 
mkdir $1/bin
cd $1/bin

# Configure project with cmake
cmake ..

# Compile project
make -j$(nproc) 

# Reset folder position
cd -
