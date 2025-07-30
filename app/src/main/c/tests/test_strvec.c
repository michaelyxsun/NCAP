#include <string.h>

#include "test.c"

#include "../strvec.h"

int
main (void)
{
    strvec_t sq;
    assert_fatal (strvec_init (&sq) == STRQUEUE_OK,
                  "strvec_init == STRQUEUE_OK", exit);
    assert_nonfatal (sq.siz == 0, "siz == 0");

    const char  *str1 = "foobar";
    const size_t len1 = strlen (str1);
    assert_nonfatal (strvec_pushb (&sq, str1, len1) == STRQUEUE_OK,
                     "strvec_push == STRQUEUE_OK");
    assert_nonfatal (sq.siz == 1, "siz == 1 after first push");
    assert_nonfatal (memcmp (str1, strvec_front (&sq), len1) == 0,
                     "front doesn't match pushed");
    assert_nonfatal (memcmp (str1, strvec_back (&sq), len1) == 0,
                     "back doesn't match pushed");

    const char  *str2 = "everyone should use zig build";
    const size_t len2 = strlen (str2);
    assert_nonfatal (strvec_pushb (&sq, str2, len2) == STRQUEUE_OK,
                     "strvec_push == STRQUEUE_OK");
    assert_nonfatal (sq.siz == 2, "siz == 2 after second push");
    assert_nonfatal (memcmp (str1, strvec_front (&sq), len1) == 0,
                     "front doesn't match first pushed");
    assert_nonfatal (memcmp (str2, strvec_back (&sq), len2) == 0,
                     "back doesn't match most recently pushed");

    strvec_popb (&sq);

    assert_nonfatal (sq.siz == 1, "siz == 1 after pop");
    assert_nonfatal (memcmp (str1, strvec_front (&sq), len1) == 0,
                     "front doesn't match first pushed after pop");
    assert_nonfatal (memcmp (str1, strvec_back (&sq), len1) == 0,
                     "back doesn't match first pushed after pop");

    strvec_pushb (&sq, "aa", 2);
    strvec_pushb (&sq, "bb", 2);
    strvec_sort (&sq);

    assert_nonfatal (memcmp ("aa", strvec_front (&sq), 2) == 0,
                     "front should be \"aa\"");
    assert_nonfatal (memcmp ("bb", sq.ptr[1], 2) == 0,
                     "pos 1 should be \"bb\"");
    assert_nonfatal (memcmp (str1, sq.ptr[2], len1) == 0,
                     "pos 2 should be \"foobar\"");

deinit:
    strvec_deinit (&sq);

exit:
    report ();

    return 0;
}
