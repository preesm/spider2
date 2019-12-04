#!/bin/bash

rm -rf ./bin/*
cd bin
# Generating the Makefile
# Run cmake gui to debug cmake problem
cmake .. -G "CodeBlocks - Unix Makefiles"

