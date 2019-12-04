#!/bin/sh
sudo apt update
sudo apt install libgtest-dev lcov
cd /usr/src/gtest
sudo su
cmake CMakeLists.txt
make -j$(nproc)
cp *.a /usr/lib
