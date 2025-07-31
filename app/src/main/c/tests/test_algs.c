#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "test.c"

#include "../algs.c"

int
main (void)
{
    const int base[] = { 0, 1, 2, 3, 4, 5 };
    int      *arr    = malloc (sizeof base);
    memcpy (arr, base, sizeof base);

    const size_t width = sizeof base[0];
    const size_t len   = sizeof base / width;

    srand (88888888);

    size_t iters = 8;

    while (iters--) {
        shuffle (arr, width, len);
        assert_nonfatal (
            memcmp (arr, base, sizeof base) != 0,
            "shuffled array should not be equal to the starting array");
    }

    free (arr);

    report ();

    return 0;
}
