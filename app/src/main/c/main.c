#include <jni.h>
#include <pthread.h>
#include <unistd.h>

// #include <android_native_app_glue.h>

#include "audio.h"
#include "logging.h"
#include "properties.h"
#include "render.h"

static const char *FILENAME = "main.c";

static void *
tfn_audio_play (void *errstat)
{
    const char *fn_in = "/sdcard/Download/audio.m4a";
    // const char *fn_out = "/sdcard/Download/audio.wav";
    const char *fn_out = intfile ("audio.wav");

    logi ("%s: converting `%s' to WAV file `%s'...", FILENAME, fn_in, fn_out);

    int *e = (int *)errstat;

    if ((*e = libav_cvt_wav (fn_in, fn_out)) != NCAP_OK) {
        loge ("%s: ERROR: libav_cvt_wav failed with code %d\n", FILENAME, *e);
        pthread_exit (NULL);
    }

    logi ("%s: playing audio...", FILENAME);

    if ((*e = audio_play (fn_out)) != NCAP_OK) {
        loge ("%s: ERROR: libav_cvt_wav failed with code %d\n", FILENAME, *e);
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
    logi ("%s: Spawning audio_play thread...", FILENAME);
    pthread_create (&audio_tid, NULL, tfn_audio_play, &stat);

    logi ("%s: Initializing window...", FILENAME);
    render ();

    logi ("%s: Joining thread...", FILENAME);
    pthread_join (audio_tid, NULL);

    logi ("%s: tfn_audio_play returned a status code of %d", FILENAME, stat);
}
