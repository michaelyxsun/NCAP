#pragma once

#ifndef STRQUEUE_H
#define STRQUEUE_H

#include <stddef.h>

typedef struct strqueue_struct {
    size_t cap;
    size_t siz;
    char **ptr;
} strqueue_t;

#define STRQUEUE_OK    0
#define STRQUEUE_ERR   -1
#define STRQUEUE_ENULL -2

/** sets errno */
extern int strqueue_init (strqueue_t *this);

extern void strqueue_deinit (strqueue_t *this);

extern int strqueue_push (strqueue_t *this, const char *str, size_t len);

extern void strqueue_pop (strqueue_t *this);

/**
 * signature:
 *
 * char *strqueue_front (strqueue_t *this);
 */
#define strqueue_front(thisaddr) ((thisaddr)->ptr[(thisaddr)->siz - 1])

#endif // !STRQUEUE_H
