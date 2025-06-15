# Dependency Files

### `libav_install.sh`

Install script for FFmpeg libav. For development, move the file into the FFmpeg submodule directory (probably `FFmpeg/libav_install.sh`) and run.

### `raylib_config.h`

Custom build options for raylib. Copy this to `src/config.h` in the raylib submodule directory (probably `raylib/src/config.h`).
Note that it must be renamed to `config.h` for it to preprocess properly.
