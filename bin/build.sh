#!/bin/sh
# Any subsequent(*) commands which fail will cause the shell script to exit immediately
set -e

# Prepare the package, development tools for php
phpize --clean
phpize

# Compile the package
./configure
make clean
make