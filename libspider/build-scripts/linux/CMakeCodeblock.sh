#!/bin/bash

cd ../..
rm -rf bin

mkdir bin
cd bin
# Generating the Makefile
# Run cmake gui to debug cmake problem
cmake ../ -G "CodeBlocks - Unix Makefiles"

