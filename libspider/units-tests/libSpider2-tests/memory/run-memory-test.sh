#!/bin/sh

# Run test
./test-memory --gtest_output="xml:../report-memory.xml"

# Generate html report
mv CMakeFiles/runtime.dir/main.gcda .
mv CMakeFiles/runtime.dir/main.gcno .
lcov --base-directory . --directory . -c -o memory_test.info
lcov --remove memory_test.info "/usr*" -o memory_test.info # remove output for external libraries
rm -rf ../report
genhtml -o ../report -t "libspider2 memory test coverage" --num-spaces 4 memory_test.info
# Clean work space
cd .. && rm -rf bin
