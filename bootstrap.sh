#!/bin/sh

# Fail on any errors
set -euo pipefail

# generate ./configure and other autotools files
libtoolize --install --copy --force --automake
autopoint --force
aclocal
autoconf --force
autoheader --force
automake --add-missing --copy --force-missing
