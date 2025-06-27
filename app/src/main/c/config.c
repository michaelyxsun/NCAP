#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef NCAP_ISTEST
#include <aaudio/AAudio.h>
#include "logging.h"
#else // NCAP_ISTEST
#define loge(fmt)       puts
#define logw(fmt)       puts
#define logi(fmt)       puts
#define logd(fmt)       puts
#define logv(fmt)       puts
#define logef(fmt, ...) printf
#define logwf(fmt, ...) printf
#define logif(fmt, ...) printf
#define logdf(fmt, ...) printf
#define logvf(fmt, ...) printf
#endif // NCAP_ISTEST

#include "config.h"

static const char *FILENAME = "config.c";

struct config_t ncap_config;
FILE           *ncap_config_fp = NULL;

/** set to NULL when unused/freed */
static char *pathbuf = NULL;

pthread_mutex_t config_mx = PTHREAD_MUTEX_INITIALIZER;

int
config_init (const char *fn)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&config_mx)) != 0) {
        logwf ("WARN: could not lock config_mx. Error code %d: %s", pth_ret,
               strerror (pth_ret));
        return CONFIG_ETHRD;
    }

    if (access (fn, F_OK) == 0) {
        if ((ncap_config_fp = fopen (fn, "rb+")) == NULL) {
            logef ("ERROR: could not open config file `%s' for rb+: %s", fn,
                   strerror (errno));
            pth_ret = CONFIG_ERR;
            goto exit;
        }

        pth_ret = CONFIG_INIT_EXISTS;
    } else {
        if ((ncap_config_fp = fopen (fn, "wb+")) == NULL) {
            logef ("ERROR: could not open config file `%s' for wb+: %s", fn,
                   strerror (errno));
            pth_ret = CONFIG_ERR;
            goto exit;
        }

        pth_ret = CONFIG_INIT_CREAT;
    }

exit:
    pthread_mutex_unlock (&config_mx);
    return pth_ret;
}

int
config_deinit (void)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&config_mx)) != 0) {
        logwf ("WARN: could not lock config_mx. Error code %d: %s", pth_ret,
               strerror (pth_ret));
        return CONFIG_ETHRD;
    }

    free (pathbuf);
    pathbuf = NULL;

    int ret = CONFIG_OK;

    if (fclose (ncap_config_fp) == EOF) {
        logef ("ERROR: could not close file pointed to by ncap_config_fp: %s",
               strerror (errno));
        ret = CONFIG_ERR;
        goto exit;
    }

exit:
    pthread_mutex_unlock (&config_mx);
    return ret;
}

int
config_read (void)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&config_mx)) != 0) {
        logwf ("WARN: could not lock config_mx. Error code %d: %s", pth_ret,
               strerror (pth_ret));
        return CONFIG_ETHRD;
    }

    fseek (ncap_config_fp, 0, SEEK_SET);
    fread (&ncap_config, sizeof (struct config_t) - sizeof (char *), 1,
           ncap_config_fp);

    if (pathbuf != NULL)
        free (pathbuf);

    if ((pathbuf = malloc (ncap_config.track_path_len)) == NULL)
        return CONFIG_EMEM;

    fread (pathbuf, sizeof (char), ncap_config.track_path_len, ncap_config_fp);
    ncap_config.track_path = pathbuf;

    pthread_mutex_unlock (&config_mx);
    return CONFIG_OK;
}

void
config_write (void)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&config_mx)) != 0) {
        logwf ("WARN: could not lock config_mx. Error code %d: %s", pth_ret,
               strerror (pth_ret));
        return;
    }

    fseek (ncap_config_fp, 0, SEEK_SET);
    fwrite (&ncap_config, sizeof (struct config_t) - sizeof (char *), 1,
            ncap_config_fp);
    fputs (ncap_config.track_path, ncap_config_fp);
    fputc ('\0', ncap_config_fp);

    pthread_mutex_unlock (&config_mx);
}

void
config_logdump ()
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&config_mx)) != 0) {
        logwf ("WARN: could not lock config_mx. Error code %d: %s", pth_ret,
               strerror (pth_ret));
        return;
    }

    logif ("isrepeat:\t%hhu", ncap_config.isrepeat);
    logif ("isshuffle:\t%hhu", ncap_config.isshuffle);
    logif ("aaudio_optimize:\t%hhu", ncap_config.aaudio_optimize);
    logif ("volume:\t%hhu", ncap_config.volume);
    logif ("cur_track:\t%u", ncap_config.cur_track);
    logif ("track_path_len:\t%u", ncap_config.track_path_len);
    logif ("track_path:\t%s", ncap_config.track_path);

    pthread_mutex_unlock (&config_mx);
}

int
to_aaudio_pm (uint8_t cfg_code)
{
#ifndef NCAP_ISTEST
    switch (cfg_code) {
        case 1:
            return AAUDIO_PERFORMANCE_MODE_LOW_LATENCY;
        case 2:
            return AAUDIO_PERFORMANCE_MODE_POWER_SAVING;
        case 0:
        default:
            return AAUDIO_PERFORMANCE_MODE_NONE;
    }
#else  // NCAP_ISTEST
    puts ("to_aaudio_pm doesn't work when NCAP_ISTEST is defined");
    return 0;
#endif // !NCAP_ISTEST
}
