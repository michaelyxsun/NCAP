#!/usr/bin/env bash

# pass the emulator name

adb logcat -c
adb -s "$1" logcat | grep -E 'native-lib\.c|audio\.c'
