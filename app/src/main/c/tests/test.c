#include <assert.h>
#include <stddef.h>
#include <stdio.h>

size_t passcnt = 0;
size_t failcnt = 0;

#define assert_fatal(cond, msg, label)                                        \
    if (!(cond)) {                                                            \
        ++failcnt;                                                            \
        fputs ("Fatal assertion failed:\t", stderr);                          \
        fputs (msg "\n", stderr);                                             \
        goto label;                                                           \
    } else {                                                                  \
        ++passcnt;                                                            \
    }

#define assert_nonfatal(cond, msg)                                            \
    if (!(cond)) {                                                            \
        ++failcnt;                                                            \
        fputs ("Nonfatal assertion failed:\t", stderr);                       \
        fputs (msg "\n", stderr);                                             \
    } else {                                                                  \
        ++passcnt;                                                            \
    }

#define report()                                                              \
    printf ("\n------REPORT------\npasses:\t%zu\nfails:\t%zu\ntotal:\t%zu\n", \
            passcnt, failcnt, passcnt + failcnt)
