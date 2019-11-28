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
    echo "usage: ./run-test.sh TEST_NAME"
    exit 1
fi

# Creates output binary folder
rm -rf bin 
mkdir bin
cd bin

# Configure project with cmake
cmake ..

# Compile project
mkdir $1
cmake --build . --target $1-spider2.0-tests -- -j$(nproc) 

# Run test
./$1-spider2.0-tests --gtest_output="xml:../report-$1-spider2.0-tests.xml"

# Generate html report
cp -R ../../../bin/CMakeFiles/Spider2.dir/libspider/*.gcda .
cp -R ../../../bin/CMakeFiles/Spider2.dir/libspider/**/*.gcda .
cp -R ../../../bin/CMakeFiles/Spider2.dir/libspider/**/**/*.gcda .
cp -R ../../../bin/CMakeFiles/Spider2.dir/libspider/*.gcno .
cp -R ../../../bin/CMakeFiles/Spider2.dir/libspider/**/*.gcno .
cp -R ../../../bin/CMakeFiles/Spider2.dir/libspider/**/**/*.gcno .
mv CMakeFiles/$1-spider2.0-tests.dir/source/*.gcda .
mv CMakeFiles/$1-spider2.0-tests.dir/source/*.gcno .
lcov -c -d . -o $1-spider2.0-tests_test.info
lcov --remove $1-spider2.0-tests_test.info "/usr*" -o $1-spider2.0-tests_test.info # remove output for external libraries
rm -rf ../report
genhtml -o ../report -t "libspider2 $1 test coverage" --num-spaces 4 $1-spider2.0-tests_test.info

# Clean work space
rm -rf ../bin

# Reset directory 
cd -
