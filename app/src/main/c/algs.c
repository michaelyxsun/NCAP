#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "algs.h"

// TODO(michaelyxsun): inline asm
void
memswp (void *restrict p1, void *restrict p2, size_t width)
{
    void    *tmp;
    uint64_t u64tmp;

    switch (width) {
        case 1:
            u64tmp         = *(uint8_t *)p1;
            *(uint8_t *)p1 = *(uint8_t *)p2;
            *(uint8_t *)p2 = u64tmp;
            break;
        case 2:
            u64tmp          = *(uint16_t *)p1;
            *(uint16_t *)p1 = *(uint16_t *)p2;
            *(uint16_t *)p2 = u64tmp;
            break;
        case 4:
            u64tmp          = *(uint32_t *)p1;
            *(uint32_t *)p1 = *(uint32_t *)p2;
            *(uint32_t *)p2 = u64tmp;
            break;
        case 8:
            u64tmp          = *(uint64_t *)p1;
            *(uint64_t *)p1 = *(uint64_t *)p2;
            *(uint64_t *)p2 = u64tmp;
            break;
        default:
            tmp = malloc (width);
            memcpy (tmp, p1, width);
            memcpy (p1, p2, width);
            memcpy (p2, tmp, width);
            free (tmp);
    }
}

/**
 * fisher yates.
 *
 * shuffles length `len` array `arr` with `width` byte elements.
 */
void
shuffle (void *arr, size_t width, size_t len)
{
    if (width == 0)
        return;

    for (size_t i = len; i > 0; --i) {
        memswp ((uint8_t *)arr + (i - 1) * width,
                (uint8_t *)arr + (rand () % i) * width, width);
    }
}
