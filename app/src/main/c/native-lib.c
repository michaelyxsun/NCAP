#include <errno.h>
#include <pthread.h>

#include "audio.h"
#include "render.h"
#include "util.h"

static const char *FILENAME = "native-lib.c";

static void *
tfn_audio_play (void *errstat)
{
    const char *fn_in  = "/sdcard/Download/audio.mp3";
    const char *fn_out = "/sdcard/Download/audio.wav";

    libav_cvt_wav (fn_in, fn_out);

    // *(int *)errstat = audio_play ("/sdcard/Download/out.wav");
    *(int *)errstat = audio_play (fn_out);
    pthread_exit (NULL);
}

int
main (void)
{
    pthread_t audio_tid;
    int       stat;
    logi ("%s: Spawning audio_play thread...", FILENAME);
    pthread_create (&audio_tid, NULL, tfn_audio_play, &stat);

    logi ("%s: Initializing window...", FILENAME);
    render ();

    logi ("%s: Joining thread...", FILENAME);
    pthread_join (audio_tid, NULL);

    logi ("%s: audio_play returned a status code of %d", FILENAME, stat);

    return 0;
}
