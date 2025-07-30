#!/bin/sh

find . -name 'test_*' -exec sh -c 'make test TARG=$(echo {} | cut -c 8- | rev | cut -c 3- | rev) CFLAGS_EXTRA=-DNCAP_ISTEST' \;
