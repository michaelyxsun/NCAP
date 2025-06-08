#!/usr/bin/env bash

# move this into the FFmpeg directory

PREFIX=../libav
CORES=4
TOOLCHAIN=/Users/msun/Library/Android/sdk/ndk/27.0.12077973/toolchains/llvm/prebuilt/darwin-x86_64
CC="$TOOLCHAIN/bin/aarch64-linux-android26-clang"
# CC="$TOOLCHAIN/bin/armv7a-linux-androideabi26-clang"

# --as="$CC" \

./configure \
    --cc="$CC" \
    --nm="$TOOLCHAIN/bin/llvm-nm" \
    --ar="$TOOLCHAIN/bin/llvm-ar" \
    --strip="$TOOLCHAIN/bin/llvm-strip" \
    --ranlib="$TOOLCHAIN/bin/llvm-ranlib" \
    --target-os=android \
    --arch=arm64 \
    --extra-cflags="--gcc-toolchain=$TOOLCHAIN" \
    --sysroot="$TOOLCHAIN/sysroot" \
    --enable-cross-compile \
    --disable-static \
    --enable-shared \
    --disable-programs \
    --disable-doc \
    --enable-protocol=file \
    --enable-pic \
    --prefix="$PREFIX" \
    --shlibdir="$PREFIX" \
    --logfile=configure.log &&
    make install -j "$CORES"
