#!/bin/sh

errorexit() {
    echo $1
    exit 1
}

# prepare locales to make date format reproducible
export LC_ALL=en_US
export LANG=en
export LINGUAS=en

# run the test
./testsms > testsms.log

# check if output differs from what it should be
diff testsms.log testsms-output.txt
