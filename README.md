# NCAP

### Dependencies

- [raylib](https://github.com/raysan5/raylib) (tested on `>=5.5`)
- [FFmpeg](https://github.com/FFmpeg/FFmpeg) `>=7.0`
    - `libavutil`
    - `libavformat`
    - `libavcodec`
- [c-dsa](https://github.com/M-Y-Sun/c-dsa) `>=0.1.0`

### Supported ABIs

See the [supported ABIs](https://developer.android.com/ndk/guides/abis#sa) page on the Android NDK documentation.

- `armeabi-v7a` (ARM)
- `arm64-v8a` (AArch64)
- `x86_64` (x86-64)

TODO: support x86 (libav compile issues)
