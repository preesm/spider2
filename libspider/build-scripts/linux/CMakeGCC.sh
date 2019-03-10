#!/bin/bash

cd ../..
rm -rf bin

mkdir bin
mkdir bin/make
cd bin/make
# Generating the Makefile
# Run cmake gui to debug cmake problem
cmake ../..

make VERBOSE=1 -j$(nproc)

cp ../../libspider/spider/spider.h ./
