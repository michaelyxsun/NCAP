#include <stdio.h>
#include <string.h>

#include "test.h"

#include "../strvec.h"

size_t passcnt = 0;
size_t failcnt = 0;

int
main (void)
{
    strvec_t sq;
    assert_fatal (strvec_init (&sq) == STRQUEUE_OK,
                  "strvec_init == STRQUEUE_OK", exit);

    const char  *str1 = "foobar";
    const size_t len1 = strlen (str1);
    assert_nonfatal (strvec_pushb (&sq, str1, len1) == STRQUEUE_OK,
                     "strvec_push == STRQUEUE_OK");
    assert_nonfatal (memcmp (str1, strvec_front (&sq), len1) == 0,
                     "front doesn't match pushed");
    assert_nonfatal (memcmp (str1, strvec_back (&sq), len1) == 0,
                     "back doesn't match pushed");

    const char  *str2 = "everyone should use zig build";
    const size_t len2 = strlen (str2);
    assert_nonfatal (strvec_pushb (&sq, str2, len2) == STRQUEUE_OK,
                     "strvec_push == STRQUEUE_OK");
    assert_nonfatal (memcmp (str1, strvec_front (&sq), len1) == 0,
                     "front doesn't match first pushed");
    assert_nonfatal (memcmp (str2, strvec_back (&sq), len2) == 0,
                     "back doesn't match most recently pushed");

    strvec_popb (&sq);

    assert_nonfatal (memcmp (str1, strvec_front (&sq), len1) == 0,
                     "front doesn't match first pushed after pop");
    assert_nonfatal (memcmp (str1, strvec_back (&sq), len1) == 0,
                     "back doesn't match first pushed after pop");

deinit:
    strvec_deinit (&sq);

exit:
    report ();

    return 0;
}
