#include "aaudio_bind.c"

void
audio_init (void)
{
    audio_isplay = false;
}

bool
audio_isplaying (void)
{
    return audio_isplay;
}

int
audio_resume (void)
{
    int pth_ret;

    logv ("resuming audio...");

    if ((pth_ret = pthread_mutex_lock (&audio_mx)) != 0)
        return pth_ret;

    audio_isplay = true;
    pth_ret      = pthread_cond_signal (&audio_cv);

    pthread_mutex_unlock (&audio_mx);

    return pth_ret;
}

int
audio_pause (void)
{
    int pth_ret;

    logv ("pausing audio...");

    if ((pth_ret = pthread_mutex_lock (&audio_mx)) != 0)
        return pth_ret;

    audio_isplay = false;
    pth_ret      = pthread_cond_signal (&audio_cv);

    pthread_mutex_unlock (&audio_mx);

    return pth_ret;
}
