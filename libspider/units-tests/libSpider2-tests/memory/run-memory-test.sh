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

# Run test
./test-memory 

# Generate html report
mv CMakeFiles/runtime.dir/main.gcda .
mv CMakeFiles/runtime.dir/main.gcno .
lcov --base-directory . --directory . -c -o memory_test.info
lcov --remove memory_test.info "/usr*" -o memory_test.info # remove output for external libraries
rm -rf ../report
genhtml -o ../report -t "libspider2 memory test coverage" --num-spaces 4 memory_test.info
# Clean work space
cd .. && rm -rf bin
