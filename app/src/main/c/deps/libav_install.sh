#!/bin/sh

cd FFmpeg || exit

TOOLCHAIN=/Users/msun/Library/Android/sdk/ndk/27.0.12077973/toolchains/llvm/prebuilt/darwin-x86_64

# $1 is ABI     Example: armeabi-v7a
# $2 is ARCH    Example: aarch64
# $3 is CC      Example: armv7a-linux-androideabi26-clang
install() {
    printf 'installing for ABI '\`'%s'\'' and architecture '\`'%s'\''...\n' "$1" "$2"

    PREFIX="../libav/$1"
    CC="$TOOLCHAIN/bin/$3"

    ./configure \
        --cc="$CC" \
        --ld="$CC" \
        --nm="$TOOLCHAIN/bin/llvm-nm" \
        --ar="$TOOLCHAIN/bin/llvm-ar" \
        --strip="$TOOLCHAIN/bin/llvm-strip" \
        --ranlib="$TOOLCHAIN/bin/llvm-ranlib" \
        --target-os=android \
        --arch="$2" \
        --extra-cflags="--gcc-toolchain=$TOOLCHAIN" \
        --sysroot="$TOOLCHAIN/sysroot" \
        --enable-cross-compile \
        --disable-static \
        --enable-shared \
        --disable-programs \
        --disable-doc \
        --disable-avdevice \
        --disable-swscale \
        --disable-avfilter \
        --disable-network \
        --disable-encoders \
        --disable-muxers \
        --disable-bsfs \
        --disable-indevs \
        --disable-outdevs \
        --disable-filters \
        --enable-protocol=file \
        --prefix="$PREFIX" \
        --shlibdir="$PREFIX" \
        --logfile=configure.log &&
        make clean &&
        make install -j
}

install armeabi-v7a arm armv7a-linux-androideabi26-clang
install arm64-v8a aarch64 aarch64-linux-android26-clang
install x86_64 x86_64 x86_86-linux-android26-clang
