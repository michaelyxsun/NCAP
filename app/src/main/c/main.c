#include <dirent.h>
#include <errno.h>
#include <jni.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "config.h"
#include "logging.h"
#include "properties.h"
#include "render.h"
#include "strvec.h"

static const char *FILENAME = "main.c";

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 128
#endif

static ANativeActivity *activity;

static const struct timespec retry_ts
    = { .tv_sec = 0, .tv_nsec = 250000000 }; // 250 ms

static void
path_concat (char *dst, const char *restrict prefix, const char *restrict file)
{
    const size_t pfxlen     = strlen (prefix);
    const size_t flen       = strlen (file);
    const size_t malloc_siz = pfxlen + flen + 2;

    memcpy (dst, prefix, pfxlen);
    dst[pfxlen] = '/';
    memcpy (dst + pfxlen + 1, file, flen + 1);
    dst[malloc_siz - 1] = '\0';
}

static int
load_dir (strvec_t *sv, const char *path)
{
    struct dirent *dir;
    DIR           *dp = opendir (path);

    if (dp == NULL) {
        logef ("opendir failed for path `%s': %s", path, strerror (errno));
        return -1;
    }

    while ((dir = readdir (dp)) != NULL) {
        if (dir->d_type != DT_REG) {
            logvf ("skipping file `%s': not a DT_REG file", dir->d_name);
            continue;
        }

        strvec_pushb (sv, dir->d_name, strlen (dir->d_name));
        logvf ("pushed file `%s'", dir->d_name);
    }

    if (closedir (dp) != 0) {
        logef ("ERROR: closedir failed: %s", strerror (errno));
        return -1;
    }

    logdf ("closed DIR pointer to path `%s'", path);

    if (sv->siz == 0) {
        logw ("WARN: read no files");
        return -1;
    }

    strvec_sort (sv);

    return 0;
}

struct audio_play_args_t {
    const char *const prefix;
    strvec_t *const   sv;
    int               errstat;
};

static void *
tfn_audio_play (void *args_vp)
{
    int ret;

    logd ("waiting for render ready...");

    if ((ret = render_waitready ()) != 0) {
        logwf ("WARN: render_waitready failed with error code %d: %s", ret,
               strerror (ret));
        pthread_exit (NULL);
    }

    logi ("render_ready = true; audio_play thread proceeding");

    struct audio_play_args_t *args = args_vp;
    strvec_t *const           sv   = args->sv;
    int                       pth_err;

    size_t i;

    const size_t ntracks = sv->siz;

    while (true) {
        // set current track number
        do {
            config_get (i, cur_track, pth_err);
        } while (pth_err != 0);

        logdf ("got i=%zu", i);

        // get path

        logvf ("preparing to play `%s'", sv->ptr[i]);

        static char fn_in[MAX_PATH_LEN], fn_out[MAX_PATH_LEN];
        path_concat (fn_in, args->prefix, sv->ptr[i]);
        path_concat (fn_out, activity->internalDataPath,
                     NCAP_AUDIO_CACHE_FILE);

        // get PCM

        logif ("converting `%s' to WAV file `%s'...", fn_in, fn_out);

        if ((args->errstat = libav_cvt_cwav (fn_in, fn_out)) != NCAP_OK) {
            logef ("ERROR: libav_cvt_wav failed with code %d. aborting...\n",
                   args->errstat);
            goto exit;
        }

        // play audio

        logi ("playing audio...");

        if ((args->errstat = audio_play (fn_out)) < NCAP_OK) {
            logef ("ERROR: libav_cvt_wav failed with code %d. aborting...\n",
                   args->errstat);
            goto exit;
        }

        // check for wclose

        if (render_closing ()) {
            logd ("wclose = true, exiting thread early...");
            break;
        }

        if (args->errstat == NCAP_INT)
            continue;

        // increment current track number

        do {
            config_set (++i, cur_track, pth_err);
        } while (pth_err != 0);

        if (i == ntracks) {
            do {
                config_set (i = 0, cur_track, pth_err);
            } while (pth_err != 0);

            uint8_t isrepeat;
            config_get (isrepeat, isrepeat, pth_err);

            if (!(pth_err == 0 && isrepeat))
                audio_pause ();
        }
    }

    if (i == ntracks) {
        do {
            config_set (0, cur_track, pth_err);
        } while (pth_err != 0);
    }

exit:
    pthread_exit (NULL);
}

int
main (void)
{
    audio_init ();
    render_init ();

    activity = GetAndroidApp ()->activity;

    static char cfgfile[MAX_PATH_LEN];
    path_concat (cfgfile, activity->internalDataPath, NCAP_CONFIG_FILE);
    logdf ("initializing config file `%s'", cfgfile);
    // remove (cfgfile);
    switch (config_init (cfgfile)) {
        case CONFIG_INIT_CREAT:
            logi ("creating config...");
            ncap_config.aaudio_optimize = 2; // power saving
            ncap_config.cur_track       = 0;
            ncap_config.isrepeat        = 0; // false
            ncap_config.isshuffle       = 0; // false
            ncap_config.volume          = 100;
            ncap_config.track_path      = NCAP_DEFAULT_TRACK_PATH;
            ncap_config.track_path_len  = strlen (ncap_config.track_path) + 1;
            logi ("writing to config...");
            config_write ();
            break;
        case CONFIG_INIT_EXISTS:
            logi ("config exists. reading config...");

            if (config_read () < 0) {
                loge ("ERROR: config_read failed. aborting...");
                return 1;
            }

            break;
        case CONFIG_ERR:
        default:
            loge ("ERROR: config init failed");
    }

    config_logdump ();

    logif ("loading tracks in configured directory `%s'...",
           ncap_config.track_path);
    strvec_t sv;
    strvec_init (&sv);

    int loadret = load_dir (&sv, ncap_config.track_path);

    pthread_t                audio_tid;
    struct audio_play_args_t audio_args = {
        .prefix = ncap_config.track_path,
        .sv     = &sv,
    };

    if (loadret >= 0) {
        pthread_create (&audio_tid, NULL, tfn_audio_play, &audio_args);
        logi ("spawned audio_play thread");
    } else {
        loge ("not playing audio, load_dir failed");
    }

    render (&sv);

    if (loadret >= 0) {
        logi ("joining threads...");
        pthread_join (audio_tid, NULL);
        logdf ("audio_play thread joined with a status code of %d...",
               audio_args.errstat);
    }

    logi ("updating config...");
    config_write ();

    logi ("deinit strvec...");
    strvec_deinit (&sv);

    logi ("deinit config...");

    if (config_deinit () != CONFIG_OK)
        logw ("WARN: config_deinit failed");

    logi ("main finished");

    return 0;
}
