#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strvec.h"

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
strvec_init (strvec_t *this)
{
    this->cap = 1;
    this->siz = 0;
    this->ptr = malloc (sizeof (char *));
    return this->ptr == NULL ? STRQUEUE_ENULL : STRQUEUE_OK;
}

void
strvec_deinit (strvec_t *this)
{
    while (this->siz)
        free (this->ptr[--this->siz]);

    free (this->ptr);
}

int
strvec_pushb (strvec_t *this, const char *restrict str, size_t len)
{
    if (this->siz == this->cap)
        expand (this);

    char *p = this->ptr[this->siz++] = malloc (len + 1);

    if (p == NULL)
        return STRQUEUE_ENULL;

    memcpy (p, str, len + 1);
    return STRQUEUE_OK;
}

void
strvec_popb (strvec_t *this)
{
    free (this->ptr[--this->siz]);

    if (this->cap >= this->siz << 1)
        shrink (this)
}

static int
cmp (const void *plhs, const void *prhs)
{
    return strcmp (*(char **)plhs, *(char **)prhs);
}

void
strvec_sort (strvec_t *this)
{
    qsort (this->ptr, this->siz, sizeof this->ptr[0], cmp);
}
