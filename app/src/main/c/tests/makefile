.PHONY: default test clean

CC ?= clang
OPTIMIZE ?= -Og
CFLAGS_EXTRA ?=

CFLAGS = -g -Wall -Wextra -Wpedantic $(OPTIMIZE)

BIN = test
BUILD_PREFIX = build
OUT = $(BUILD_PREFIX)/$(BIN)

LISTS=testlist.txt

default:
	$(CC) $$(find . -type f -name '*.c' | tr '\n' ' ') $$(awk '{print "../" $$0 ".c"}' $(LISTS) | tr '\n' ' ') -o $(OUT) $(CFLAGS) $(CFLAGS_EXTRA)

test: default
	./$(OUT)

clean:
	rm -r $(OUT) $(OUT).dSYM/
