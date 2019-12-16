#!/bin/sh
set -e

if [ "$#" -ne 2 ]; then
    echo "Expecting one argument."
    echo "usage:   ./scripts/linux/lcovRemoveTest.sh FILE_NAME FILE_PATH"
    echo "example: ./scripts/linux/lcovRemoveTest.sh \"GenericAllocator.cpp\" \"../libspider/memory/dynamic-allocators/\""
    exit 1
fi

file=$1
path=$2
echo "Searching for LCOV_IGNORE comment in $file.."
line_ignore=$(grep -n "LCOV_IGNORE" $path$file | cut -d : -f 1)  # Line number of non testable error in given $file
if [ ! -z "$line_ignore" ]
then
    echo "LCOV_IGNORE flag in file: $file --> FOUND."
    for line in $line_ignore
    do
        line=$(($line + 1))
        line_file=$(grep -n -m 1 "$file" all-spider2-test.info | cut -d : -f 1) # Line where the report of $file start
        line_zero=$(($line_file + $(tail -n +${line_file} all-spider2-test.info | grep -n -m 1 "DA:${line}" | cut -d : -f 1) - 1)) # Line of the corresponding coverage info
        line_orig_shift=$(($line - 2)) # Line just before the one of the test in $file
        value=$(tail -n +${line_file} all-spider2-test.info | grep -m 1 "${line_orig_shift}," | cut -d , -f 2) # Value attributed by LCOV for the line before
        sed -i "${line_zero}s/DA:${line},0/DA:${line},${value}/g" all-spider2-test.info # Replace the non-covered value by the one of the line before (if the line was not covered).
    done
fi


