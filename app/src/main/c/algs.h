#pragma once

#ifndef ALGS_H
#define ALGS_H

#include <stddef.h>

extern void memswp (void *p1, void *p2, size_t width);

/**
 * fisher yates.
 *
 * `arr` is a length `len` array of `width` byte elements
 */
extern void shuffle (void *arr, size_t width, size_t len);

#endif // ALGS_H
