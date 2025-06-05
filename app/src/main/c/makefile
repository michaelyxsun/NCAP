FILE ?= native-lib.c

COMPILEDB = compile_commands.json

AAUDIO_INCL = $$ANDROID_HOME/ndk/27.0.12077973/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/aaudio

default:
	clang $(FILE) -o main $$(pkg-config --libs --cflags raylib) -I$(AAUDIO_INCL) # -MJ $(COMPILEDB)
