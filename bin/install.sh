#!/bin/sh
# Any subsequent(*) commands which fail will cause the shell script to exit immediately
set -e

# Find additional php ini files
find_ini=$(php --ini | grep 'Scan for additional .ini files in: ' | cut -d' ' -f7)

# Install the package
if [ "$(id -u)" -eq 0 ]; then
  make install
else
  echo "Root privileges are required to install the package"
  sudo make install
fi

# Add the extension to php.ini
if [ "$find_ini" ]; then
  echo "Installing extension in:          $find_ini/20-aop.ini"
  sudo sh -c "echo 'extension=aop.so' > '$find_ini'/20-aop.ini"
fi