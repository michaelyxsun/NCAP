#pragma once

#ifndef STRQUEUE_H
#define STRQUEUE_H

#include <stddef.h>

typedef struct strvec_struct {
    size_t cap;
    size_t siz;
    char *_Nullable *_Nullable ptr;
} strvec_t;

#define STRQUEUE_OK    0
#define STRQUEUE_ERR   -1
#define STRQUEUE_ENULL -2

/** sets errno */
extern int strvec_init (strvec_t *_Nonnull this);

extern void strvec_deinit (strvec_t *_Nonnull this);

extern int strvec_pushb (strvec_t *_Nonnull this, const char *_Nonnull str,
                         size_t len);

extern void strvec_popb (strvec_t *_Nonnull this);

/**
 * signature:
 *
 * `char *strvec_front (strvec_t *_Nonnull this);`
 */
#define strvec_front(thisaddr) ((thisaddr)->ptr[0])

/**
 * signature:
 *
 * `char *strvec_back (strvec_t *_Nonnull this);`
 */
#define strvec_back(thisaddr) ((thisaddr)->ptr[(thisaddr)->siz - 1])

extern void strvec_sort (strvec_t *_Nonnull this);

#endif // !STRQUEUE_H
