FILE ?= native-lib.c

default:
	clang $(FILE) -o main $$(pkg-config --libs --cflags raylib)
