#!/bin/sh
# Create the links expected by SConstruct/SCons.
cd "$(dirname "$0")"

ln -sfn ../../rt-thread rt-thread
ln -sfn ../../libraries libraries
ln -sfn ../libs libs
ln -sfn ../../tools tools
