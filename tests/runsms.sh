#!/bin/sh

errorexit() {
    echo $1
    exit 1
}

# run the test
./testsms > testsms.log

# check if output differs from what it should be
diff testsms.log testsms-output.txt
