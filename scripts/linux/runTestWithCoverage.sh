#!/bin/sh
set -e

which gcovr 1>/dev/null 2>&1
if [ $? != 0 ]
then
    echo "You need to have gcovr installed in order to generate the test coverage report"
    exit 1
fi

# Warning user about the all test
echo "Running all-spider2-test target to get full coverage.."

# Run test
./scripts/linux/runTest.sh all

# Generate html report
cd bin
mkdir coverage
gcovr -r .. -e 'test/.*' --html --html-details -s -o ./coverage/coverage.html
