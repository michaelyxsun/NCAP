#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strqueue.h"

#define bytecap(this) (this->cap * sizeof (char *))

#define expand(this)                                                          \
    do {                                                                      \
        this->cap <<= 1;                                                      \
        this->ptr = realloc (this->ptr, bytecap (this));                      \
    } while (0);

#define shrink(this)                                                          \
    do {                                                                      \
        this->cap >>= 1;                                                      \
        this->ptr = realloc (this->ptr, bytecap (this));                      \
    } while (0);

int
strqueue_init (strqueue_t *this)
{
    this->cap = 1;
    this->siz = 0;
    this->ptr = malloc (sizeof (char *));

    puts ("malloced this->ptr");

    return this->ptr == NULL ? STRQUEUE_ENULL : STRQUEUE_OK;
}

void
strqueue_deinit (strqueue_t *this)
{
    while (this->siz--)
        free (*this->ptr--);

    free (this->ptr + 1);
    puts ("freed this->ptr");
}

int
strqueue_push (strqueue_t *this, const char *restrict str, size_t len)
{
    if (this->siz == this->cap)
        expand (this);

    char *p = this->ptr[this->siz++] = malloc (len);

    if (p == NULL)
        return STRQUEUE_ENULL;

    memcpy (p, str, len);
    return STRQUEUE_OK;
}

void
strqueue_pop (strqueue_t *this)
{
    free (this->ptr[--this->siz]);

    if (this->cap >= this->siz << 1)
        shrink (this)
}
