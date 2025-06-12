#!/usr/bin/env bash

# move this into your ffmpeg directory

CORES=8
TOOLCHAIN=/Users/msun/Library/Android/sdk/ndk/27.0.12077973/toolchains/llvm/prebuilt/darwin-x86_64

###### arch options ######

# ABI=armeabi-v7a
# ARCH=arm
# CC="$TOOLCHAIN/bin/armv7a-linux-androideabi26-clang"

# ABI=arm64-v8a
# ARCH=aarch64
# CC="$TOOLCHAIN/bin/aarch64-linux-android26-clang"

ABI=x86_64
ARCH=x86_64
CC="$TOOLCHAIN/bin/x86_64-linux-android26-clang"

### this one doesnt work ###
# ABI=x86
# ARCH=i686
# CC="$TOOLCHAIN/bin/i686-linux-android26-clang"

###### end arch options ######

PREFIX="../libav/$ABI"

# --as="$CC" \

./configure \
    --cc="$CC" \
    --ld="$CC" \
    --nm="$TOOLCHAIN/bin/llvm-nm" \
    --ar="$TOOLCHAIN/bin/llvm-ar" \
    --strip="$TOOLCHAIN/bin/llvm-strip" \
    --ranlib="$TOOLCHAIN/bin/llvm-ranlib" \
    --target-os=android \
    --arch="$ARCH" \
    --extra-cflags="--gcc-toolchain=$TOOLCHAIN" \
    --sysroot="$TOOLCHAIN/sysroot" \
    --enable-cross-compile \
    --disable-static \
    --enable-shared \
    --disable-programs \
    --disable-doc \
    --disable-encoders \
    --enable-protocol=file \
    --enable-pic \
    --prefix="$PREFIX" \
    --shlibdir="$PREFIX" \
    --logfile=configure.log &&
    make clean &&
    make install -j "$CORES"
