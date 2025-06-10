#!/usr/bin/env bash

# pass the emulator name

files="$(find app/src/main/c -name '*.c' -depth 1 -exec basename {} \; | tr '\n' '|' | rev | cut -c2- | rev | sed 's/\./\\\./g')"
echo "grep for files $files"

adb -s "$1" logcat -c
adb -s "$1" logcat | grep -E "$files"
