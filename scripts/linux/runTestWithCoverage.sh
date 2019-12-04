#!/bin/sh
set -e

which gcovr 1>/dev/null 2>&1
if [ $? != 0 ]
then
    echo "You need to have gcovr installed in order to generate the test coverage report"
    exit 1
fi

# Run test
echo "Running all-spider2-test target to get full coverage.."
./scripts/linux/runTest.sh all

# Collect .gcda and .gcno files
cd bin
rm -rf gcov_files
mkdir gcov_files
find ./libspider/CMakeFiles/spider2.dir/ -name '*.gcda' -exec cp -prv '{}' './gcov_files/' ';'
find ./libspider/CMakeFiles/spider2.dir/ -name '*.gcno' -exec cp -prv '{}' './gcov_files/' ';'
find ./test/CMakeFiles/all-spider2-test.dir/ -name '*.gcda' -exec cp -prv '{}' './gcov_files/' ';'
find ./test/CMakeFiles/all-spider2-test.dir/ -name '*.gcno' -exec cp -prv '{}' './gcov_files/' ';'

# Generate html report
lcov -c -d ./gcov_files -o all-spider2-test.info
lcov --remove all-spider2-test.info "/usr*" -o all-spider2-test.info # Remove output for external libraries
rm -rf coverage
mkdir coverage
genhtml -o coverage -t "libspider2 test coverage" --num-spaces 4 all-spider2-test.info 

# Clean tmp folder and files
rm -rf gcov_files
rm all-spider2-test.info
