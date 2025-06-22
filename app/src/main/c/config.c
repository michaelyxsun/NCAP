#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <aaudio/AAudio.h>

#include "config.h"
#include "logging.h"

static const char *FILENAME = "config.c";

struct config_t ncap_config;
FILE           *ncap_config_fp = NULL;
static char    *pathbuf        = NULL;

pthread_mutex_t config_mx = PTHREAD_MUTEX_INITIALIZER;

int
config_init (const char *fn)
{
    pthread_mutex_lock (&config_mx);

    int ret;

    if (access (fn, F_OK) == 0) {
        if ((ncap_config_fp = fopen (fn, "rb+")) == NULL) {
            logef ("ERROR: could not open config file `%s' for rb+: %s", fn,
                   strerror (errno));
            ret = CONFIG_ERR;
            goto exit;
        }

        ret = CONFIG_INIT_EXISTS;
    } else {
        if ((ncap_config_fp = fopen (fn, "wb+")) == NULL) {
            logef ("ERROR: could not open config file `%s' for wb+: %s", fn,
                   strerror (errno));
            ret = CONFIG_ERR;
            goto exit;
        }

        ret = CONFIG_INIT_CREAT;
    }

exit:
    pthread_mutex_unlock (&config_mx);
    return ret;
}

int
config_deinit (void)
{
    pthread_mutex_lock (&config_mx);

    free (pathbuf);

    int ret;

    if (fclose (ncap_config_fp) == EOF) {
        logef ("ERROR: could not close file pointed to bye ncap_config_fp: %s",
               strerror (errno));
        ret = CONFIG_ERR;
        goto exit;
    }

exit:
    pthread_mutex_unlock (&config_mx);
    return ret;
}

void
config_read (void)
{
    pthread_mutex_lock (&config_mx);
    fread (&ncap_config, sizeof (struct config_t) - sizeof (char *), 1,
           ncap_config_fp);
    pathbuf = realloc (pathbuf, ncap_config.track_path_len);
    fgets (pathbuf, ncap_config.track_path_len, ncap_config_fp);
    ncap_config.track_path = pathbuf;
    pthread_mutex_unlock (&config_mx);
}

void
config_write (void)
{
    pthread_mutex_lock (&config_mx);
    fwrite (&ncap_config, sizeof (struct config_t) - sizeof (char *), 1,
            ncap_config_fp);
    fputs (ncap_config.track_path, ncap_config_fp);
    pthread_mutex_unlock (&config_mx);
}

void
config_logdump ()
{
    logif ("isrepeat:\t%hhu", ncap_config.isrepeat);
    logif ("isshuffle:\t%hhu", ncap_config.isshuffle);
    logif ("aaudio_optimize:\t%hhu", ncap_config.aaudio_optimize);
    logif ("volume:\t%hhu", ncap_config.volume);
    logif ("cur_track:\t%u", ncap_config.cur_track);
    logif ("cur_track:\t%u", ncap_config.cur_track);
    logif ("track_path_len:\t%u", ncap_config.track_path_len);
    logif ("track_path:\t%s", ncap_config.track_path);
}

int
to_aaudio_pm (uint8_t cfg_code)
{
    switch (cfg_code) {
        case 1:
            return AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
        case 2:
            return AAUDIO_PERFORMANCE_MODE_POWER_SAVING;
        case 0:
        default:
            return AAUDIO_PERFORMANCE_MODE_NONE;
    }
}
