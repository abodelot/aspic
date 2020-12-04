#!/bin/sh

C_RED="\e[1;91;7m"
C_GREEN="\e[1;92;7m"
C_NONE="\e[0m"

result=0

for i in $(find ./tests -name "*.ac" -type f | sort); do
    # Hide valgrind output
    # Trigger an error if valgrind detected an error (regardless of the test result)
    if valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 --exit-on-first-error=yes ./aspic $i > /dev/null 2>&1; then
        echo ${C_GREEN} PASS ${C_NONE} $i
    else
        echo ${C_RED} FAIL ${C_NONE} $i
        result=1
    fi
done

exit $result
