#include <stdio.h>
#include <string.h>

#include "test.h"

#include "../strqueue.h"

size_t passcnt = 0;
size_t failcnt = 0;

int
main (void)
{
    strqueue_t sq;
    assert_fatal (strqueue_init (&sq) == STRQUEUE_OK,
                  "strqueue_init == STRQUEUE_OK", exit);

    const char  *str1 = "foobar";
    const size_t len1 = strlen (str1);
    assert_nonfatal (strqueue_push (&sq, str1, len1) == STRQUEUE_OK,
                     "strqueue_push == STRQUEUE_OK");
    assert_nonfatal (memcmp (str1, strqueue_front (&sq), len1) == 0,
                     "front doesn't match pushed");

    const char  *str2 = "everyone should use zig build";
    const size_t len2 = strlen (str2);
    assert_nonfatal (strqueue_push (&sq, str2, len2) == STRQUEUE_OK,
                     "strqueue_push == STRQUEUE_OK");
    assert_nonfatal (memcmp (str2, strqueue_front (&sq), len2) == 0,
                     "front doesn't match most recently pushed");

    strqueue_pop (&sq);

    assert_nonfatal (memcmp (str1, strqueue_front (&sq), len1) == 0,
                     "front doesn't match first pushed after pop");

deinit:
    strqueue_deinit (&sq);

exit:
    printf ("\n------REPORT------\npasses:\t%zu\nfails:\t%zu\ntotal:\t%zu\n",
            passcnt, failcnt, passcnt + failcnt);

    return 0;
}
