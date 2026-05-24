#!/bin/sh
#change current directory to this file

SCRIPT_PATH="$(dirname "$0")"
cd "$SCRIPT_PATH" || exit 1

cmake -G Xcode \
      -DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo \
	  -B _build/xcode-mac \
	  .
