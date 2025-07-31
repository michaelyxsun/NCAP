#pragma once

#ifndef ALGS_H
#define ALGS_H

#include <stddef.h>

extern void memswp (void *_Nonnull restrict p1, void *_Nonnull restrict p2,
                    size_t width);

/**
 * fisher yates.
 *
 * `arr` is a length `len` array of `width` byte elements
 */
extern void shuffle (void *_Nonnull arr, size_t width, size_t len);

#endif // ALGS_H
