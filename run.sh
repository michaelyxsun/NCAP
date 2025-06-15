#!/usr/bin/env bash

cd "$(dirname "$0")" || exit

# usage: ./run.sh <device id> <abi>-<releasemode>
#
# allowed abis are as follows:
# - universal
# - armeabi-v7a
# - arm64-v8a
# - x86_64

./gradlew build &&
    adb -s "$1" install "app/build/outputs/apk/debug/app-$2-debug.apk" &&
    ./log.sh "$1"
