#!/bin/sh

which lcov 1>/dev/null 2>&1
if [ $? != 0 ]
then
    echo "You need to have lcov installed in order to generate the test coverage report"
    exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "Expecting one argument."
    echo "usage: ./run-test.sh FOLDER_NAME"
    exit 1
fi

# Move to folder
cd $1/bin

# Run test
./test-$1 --gtest_output="xml:../report-$1.xml"

# Generate html report
mv CMakeFiles/runtime.dir/main.gcda .
mv CMakeFiles/runtime.dir/main.gcno .
rm -f $1/$1_test.info
lcov --base-directory . --directory . -c -o $1_test.info
lcov --remove $1_test.info "/usr*" -o $1_test.info # remove output for external libraries
rm -rf ../report

# Clean work space
rm -rf ../bin

# Reset directory 
cd -
