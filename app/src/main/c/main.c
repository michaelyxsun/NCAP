#include <jni.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "logging.h"
#include "properties.h"
#include "render.h"

static const char *FILENAME = "main.c";

static ANativeActivity *activity;

static void *
tfn_audio_play (void *errstat)
{
    pthread_mutex_lock (&render_mx);

    while (render_ready == false)
        pthread_cond_wait (&render_cv, &render_mx);

    pthread_mutex_unlock (&render_mx);

    logi ("audio_play thread recieved signal");

    const char *fn_in = "/sdcard/Download/audio.m4a";

    const char  *intdata_path     = activity->internalDataPath;
    const size_t intdata_path_len = strlen (intdata_path);
    const size_t malloc_siz = intdata_path_len + AUDIO_CACHE_FILE_LEN + 2;
    char        *fn_out     = malloc (malloc_siz);
    memcpy (fn_out, intdata_path, intdata_path_len);
    memcpy (fn_out + intdata_path_len, "/" AUDIO_CACHE_FILE,
            AUDIO_CACHE_FILE_LEN + 1);
    *(fn_out + malloc_siz - 1) = '\0';

    logif ("converting `%s' to WAV file `%s'...", fn_in, fn_out);

    int *e = (int *)errstat;

    if ((*e = libav_cvt_cwav (fn_in, fn_out)) != NCAP_OK) {
        logef ("ERROR: libav_cvt_wav failed with code %d\n", *e);
        goto exit;
    }

    logi ("playing audio...");

    if ((*e = audio_play (fn_out)) != NCAP_OK) {
        logef ("ERROR: libav_cvt_wav failed with code %d\n", *e);
        goto exit;
    }

exit:
    free (fn_out);
    pthread_exit (NULL);
}

int
main (void)
{
    activity = GetAndroidApp ()->activity;

    pthread_t audio_tid;
    int       stat;

    pthread_create (&audio_tid, NULL, tfn_audio_play, &stat);
    logi ("spawned audio_play thread");

    render ();

    logi ("joining threads...");

    pthread_join (audio_tid, NULL);

    logdf ("audio_play thread joined with a status code of %d...", stat);
}
