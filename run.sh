#!/usr/bin/env bash

./gradlew build &&
    adb -s "$1" install app/build/outputs/apk/debug/app-debug.apk &&
    ./log.sh "$1"
