.PHONY: default test clean

TARG ?= main

CC ?= clang
OPTIMIZE ?=
CFLAGS_EXTRA ?=

CFLAGS = -g -Wall -Wextra -Wpedantic $(OPTIMIZE)

BIN = test
BUILD_PREFIX = build
OUT = $(BUILD_PREFIX)/$(BIN)

default:
	$(CC) test_$(TARG).c -o $(OUT) $(CFLAGS) $(CFLAGS_EXTRA) -g

test: default
	./$(OUT)

clean:
	rm -r $(OUT) $(OUT).dSYM/
