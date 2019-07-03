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
./test-expression-parser --gtest_output="xml:../report-expression-parser.xml"

# Generate html report
mv CMakeFiles/runtime.dir/main.gcda .
mv CMakeFiles/runtime.dir/main.gcno .
lcov --base-directory . --directory . -c -o expression-parser_test.info
lcov --remove expression-parser_test.info "/usr*" -o expression-parser_test.info # remove output for external libraries
rm -rf ../report
genhtml -o ../report -t "libspider2 containers test coverage" --num-spaces 4 expression-parser_test.info
# Clean work space
cd .. && rm -rf bin
