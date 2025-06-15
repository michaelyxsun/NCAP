#!/usr/bin/env bash

cd "$(dirname "$0")" || exit

# pass the emulator name

files="$(find app/src/main/c -name '*.c' -depth 1 -exec basename {} \; | tr '\n' '|' | sed 's/\./\\\./g') F "
echo "grep for files $files"

adb -s "$1" logcat -c
adb -s "$1" logcat | grep -E "$files"
