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
    pthread_mutex_lock (&render_ready_mx);
    while (!render_ready)
        pthread_cond_wait (&render_ready_cv, &render_ready_mx);
    pthread_mutex_unlock (&render_ready_mx);

    logi ("render_ready = true; audio_play thread proceeding");

    struct audio_play_args_t *args = args_vp;
    strvec_t *const           sv   = args->sv;

    for (size_t i = 0; i < sv->siz; ++i) {
        logvf ("preparing to play `%s'", sv->ptr[i]);

        static char fn_in[MAX_PATH_LEN], fn_out[MAX_PATH_LEN];
        path_concat (fn_in, args->prefix, sv->ptr[i]);
        path_concat (fn_out, activity->internalDataPath,
                     NCAP_AUDIO_CACHE_FILE);

        logif ("converting `%s' to WAV file `%s'...", fn_in, fn_out);

        if ((args->errstat = libav_cvt_cwav (fn_in, fn_out)) != NCAP_OK) {
            logef ("ERROR: libav_cvt_wav failed with code %d. aborting...\n",
                   args->errstat);
            goto exit;
        }

        logd ("locking active track id mutex to update render...");
        pthread_mutex_lock (&render_atrid_mx);
        render_atrid = i;
        pthread_mutex_unlock (&render_atrid_mx);

        logi ("playing audio...");

        if ((args->errstat = audio_play (fn_out)) != NCAP_OK) {
            logef ("ERROR: libav_cvt_wav failed with code %d. aborting...\n",
                   args->errstat);
            goto exit;
        }
    }

exit:
    pthread_mutex_lock (&render_atrid_mx);
    render_atrid = -1;
    pthread_mutex_unlock (&render_atrid_mx);
    pthread_exit (NULL);
}

int
main (void)
{
    activity = GetAndroidApp ()->activity;

    static char cfgfile[MAX_PATH_LEN];
    path_concat (cfgfile, activity->internalDataPath, NCAP_CONFIG_FILE);
    logdf ("initializing config file `%s'", cfgfile);
    remove (cfgfile);
    switch (config_init (cfgfile)) {
        case CONFIG_INIT_CREAT:
            logi ("creating config...");
            ncap_config.aaudio_optimize = 2; // power saving
            ncap_config.cur_track       = 0;
            ncap_config.isrepeat        = 0; // false
            ncap_config.isshuffle       = 0; // false
            ncap_config.volume          = 100;
            ncap_config.track_path      = "/sdcard/Music/NCAP-share";
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
    load_dir (&sv, ncap_config.track_path);

    pthread_t                audio_tid;
    struct audio_play_args_t audio_args = {
        .prefix = ncap_config.track_path,
        .sv     = &sv,
    };

    pthread_create (&audio_tid, NULL, tfn_audio_play, &audio_args);
    logi ("spawned audio_play thread");

    render (&sv);

    logi ("joining threads...");
    pthread_join (audio_tid, NULL);
    logdf ("audio_play thread joined with a status code of %d...",
           audio_args.errstat);

    strvec_deinit (&sv);

    logi ("deinit config...");

    if (config_deinit () != CONFIG_OK)
        logw ("WARN: config_deinit failed");

    logi ("main finished");

    return 0;
}
