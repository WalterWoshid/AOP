#!/bin/sh
# Any subsequent(*) commands which fail will cause the shell script to exit immediately
set -e

make test "$@"

# You can also run the tests with a custom php.ini like this:
# php run-tests.php -c /etc/php/8.1/cli/php.ini