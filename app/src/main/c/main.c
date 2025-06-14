#include <jni.h>
#include <pthread.h>

// #include <android_native_app_glue.h>

#include "audio.h"
#include "logging.h"
#include "properties.h"
#include "render.h"

static const char *FILENAME = "main.c";

static void *
tfn_audio_play (void *errstat)
{
    pthread_mutex_lock (&render_mx);

    while (render_ready == false)
        pthread_cond_wait (&render_cv, &render_mx);

    pthread_mutex_unlock (&render_mx);

    logi ("%s: %s: audio_play thread recieved signal", FILENAME, __func__);

    const char *fn_in = "/sdcard/Download/audio.m4a";
    // const char *fn_out = "/sdcard/Download/audio.wav";
    const char *fn_out = intfile ("audio.wav");

    logi ("%s: %s: converting `%s' to WAV file `%s'...", FILENAME, __func__,
          fn_in, fn_out);

    int *e = (int *)errstat;

    if ((*e = libav_cvt_wav (fn_in, fn_out)) != NCAP_OK) {
        loge ("%s: %s: ERROR: libav_cvt_wav failed with code %d\n", FILENAME,
              __func__, *e);
        pthread_exit (NULL);
    }

    logi ("%s: %s: playing audio...", FILENAME, __func__);

    if ((*e = audio_play (fn_out)) != NCAP_OK) {
        loge ("%s: %s: ERROR: libav_cvt_wav failed with code %d\n", FILENAME,
              __func__, *e);
        pthread_exit (NULL);
    }

    pthread_exit (NULL);
}

int
main (void)
// void
// android_main (struct android_app *state)
{
    pthread_t audio_tid;
    int       stat;

    pthread_create (&audio_tid, NULL, tfn_audio_play, &stat);
    logi ("%s: %s: spawned audio_play thread", FILENAME, __func__);

    render ();

    logi ("%s: %s: joining threads...", FILENAME, __func__);

    pthread_join (audio_tid, NULL);

    logd ("%s: %s: audio_play thread joined with a status code of %d...",
          FILENAME, __func__, stat);
}
