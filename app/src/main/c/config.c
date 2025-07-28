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
#define loge(fmt)       puts (fmt)
#define logw(fmt)       puts (fmt)
#define logi(fmt)       puts (fmt)
#define logd(fmt)       puts (fmt)
#define logv(fmt)       puts (fmt)
#define logef(fmt, ...) printf (fmt, __VA_ARGS__)
#define logwf(fmt, ...) printf (fmt, __VA_ARGS__)
#define logif(fmt, ...) printf (fmt, __VA_ARGS__)
#define logdf(fmt, ...) printf (fmt, __VA_ARGS__)
#define logvf(fmt, ...) printf (fmt, __VA_ARGS__)
#endif // NCAP_ISTEST

#include "config.h"

static const char *FILENAME = "config.c";

struct config_t ncap_config;
FILE           *ncap_config_fp = NULL;

/** set to NULL when unused/freed */
static char    *pathbuf = NULL;
static uint8_t *volsbuf = NULL;

pthread_mutex_t config_mx = PTHREAD_MUTEX_INITIALIZER;

#define CONFIG_LOCK_MX                                                        \
    do {                                                                      \
        if ((pth_ret = pthread_mutex_lock (&config_mx)) != 0) {               \
            logwf ("WARN: could not lock config_mx. Error code %d: %s",       \
                   pth_ret, strerror (pth_ret));                              \
            return CONFIG_ETHRD;                                              \
        }                                                                     \
    } while (0);

#define CONFIG_UNLOCK_MX                                                      \
    do {                                                                      \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

int
config_init (const char *fn)
{
    int pth_ret;

    CONFIG_LOCK_MX;

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
    CONFIG_UNLOCK_MX;
    return pth_ret;
}

int
config_deinit (void)
{
    int pth_ret;

    CONFIG_LOCK_MX;

    free (pathbuf);
    free (volsbuf);
    pathbuf = NULL;
    volsbuf = NULL;

    int ret = CONFIG_OK;

    if (fclose (ncap_config_fp) == EOF) {
        logef ("ERROR: could not close file pointed to by ncap_config_fp: %s",
               strerror (errno));
        ret = CONFIG_ERR;
        goto exit;
    }

exit:
    CONFIG_UNLOCK_MX;
    return ret;
}

int
config_read (void)
{
    int pth_ret;

    CONFIG_LOCK_MX;

    fseek (ncap_config_fp, 0, SEEK_SET);
    fread (&ncap_config, NCAP_CONFIG_SIZ, 1, ncap_config_fp);

    // track paths

    if (pathbuf != NULL)
        free (pathbuf);

    if ((pathbuf = malloc (ncap_config.track_path_len)) == NULL)
        return CONFIG_EMEM;

    fread (pathbuf, sizeof (char), ncap_config.track_path_len, ncap_config_fp);
    ncap_config.track_path = pathbuf;

    // track volumes

    if (volsbuf != NULL)
        free (volsbuf);

    if ((volsbuf = malloc (ncap_config.ntracks)) == NULL)
        return CONFIG_EMEM;

    fread (volsbuf, sizeof (uint8_t), ncap_config.ntracks, ncap_config_fp);
    ncap_config.track_vols = volsbuf;

    CONFIG_UNLOCK_MX;
    return CONFIG_OK;
}

int
config_write (void)
{
    int pth_ret;

    CONFIG_LOCK_MX;

    fseek (ncap_config_fp, 0, SEEK_SET);
    fwrite (&ncap_config, NCAP_CONFIG_SIZ, 1, ncap_config_fp);
    fwrite (ncap_config.track_path, sizeof (char), ncap_config.track_path_len,
            ncap_config_fp);
    fwrite (ncap_config.track_vols, sizeof (uint8_t), ncap_config.ntracks,
            ncap_config_fp);

    CONFIG_UNLOCK_MX;
    return CONFIG_OK;
}

int
config_upd_vols (uint32_t ntracks, uint8_t def)
{
    int pth_ret;

    CONFIG_LOCK_MX;

    const uint32_t cfg_ntracks = ncap_config.ntracks;

    if (cfg_ntracks != ntracks) {
        uint8_t *tmp = realloc (ncap_config.track_vols, ntracks);

        if (tmp == NULL)
            return CONFIG_EMEM;

        if (cfg_ntracks < ntracks)
            memset (tmp + cfg_ntracks, 100, ntracks - cfg_ntracks);

        ncap_config.track_vols = tmp;
        ncap_config.ntracks    = ntracks;
    }

    CONFIG_UNLOCK_MX;
    return CONFIG_OK;
}

int
config_logdump (void)
{
    int pth_ret;

    CONFIG_LOCK_MX;

    logif ("isrepeat:\t%hhu", ncap_config.isrepeat);
    logif ("isshuffle:\t%hhu", ncap_config.isshuffle);
    logif ("aaudio_optimize:\t%hhu", ncap_config.aaudio_optimize);
    logif ("volume:\t%hhu", ncap_config.volume);
    logif ("cur_track:\t%u", ncap_config.cur_track);
    logif ("track_path_len:\t%u", ncap_config.track_path_len);
    logif ("track_path:\t%s", ncap_config.track_path);

    CONFIG_UNLOCK_MX;
    return CONFIG_OK;
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

#undef CONFIG_LOCK_MX
#undef CONFIG_UNLOCK_MX
