#!/bin/sh
# Any subsequent(*) commands which fail will cause the shell script to exit immediately
set -e

./bin/build.sh
./bin/install.sh