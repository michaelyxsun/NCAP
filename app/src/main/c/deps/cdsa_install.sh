#!/usr/bin/env bash

cd c-dsa || exit

TOOLCHAIN=/Users/msun/Library/Android/sdk/ndk/27.0.12077973/toolchains/llvm/prebuilt/darwin-x86_64
CFLAGS="--gcc-toolchain=$TOOLCHAIN --sysroot=$TOOLCHAIN/sysroot"

makecmd() {
    target=$1-linux-android26-clang
    echo "make a install CC=$TOOLCHAIN/bin/$target PREFIX=lib/$1 EXTRA_CFLAGS=\"$CFLAGS --target=$2\""
}

eval "$(makecmd armeabi-v7a armv7-linux-androideabi)"
eval "$(makecmd arm64-v8a aarch64-linux-android)"
eval "$(makecmd x86_64 x86_64-linux-android)"
