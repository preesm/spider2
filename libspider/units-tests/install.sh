#!/bin/sh
sudo apt update
sudo apt install libgtest-dev lcov
cd /usr/src/gtest
cmake CMakeLists.txt
make
cp *.a /usr/lib
