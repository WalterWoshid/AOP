#!/bin/sh
# Any subsequent(*) commands which fail will cause the shell script to exit immediately
set -e

# Install the dependencies
sudo apt-get update && apt-get install php8.1-dev