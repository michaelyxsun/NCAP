# Dependency Files

### `libav_install.sh`

Install script for FFmpeg libav. Edit the TOOLCHAIN variable to point to the local ndk toolchain.

### `raylib_config.h`

Custom build options for raylib. Copy this to `src/config.h` in the raylib submodule directory (probably `raylib/src/config.h`).
Note that it must be renamed to `config.h` for it to preprocess properly.

### `cdsa_install.sh`

Edit the TOOLCHAIN variable to point to the local ndk toolchain.
