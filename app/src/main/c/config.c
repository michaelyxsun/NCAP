#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <aaudio/AAudio.h>

#include "config.h"
#include "logging.h"

static const char *FILENAME = "config.c";

struct config_t ncap_config;
FILE           *ncap_config_fp = NULL;

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

#ifdef config_write
#undef config_write
#else
#error macro function config_write not defined in  header file
#endif
#define config_write(config)                                                  \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fwrite (&(config), sizeof (struct config_t), 1, ncap_config_fp);      \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

#ifdef config_upd
#undef config_upd
#else
#error macro function config_upd not defined in  header file
#endif
#define config_upd(val, field)                                                \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fseek (fp, offsetof (struct config_t, field), SEEK_SET);              \
        fwrite (&(val), sizeof (val), 1, ncap_config_fp);                     \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

#ifdef config_read_val
#undef config_read_val
#else
#error macro function config_read_val not defined in  header file
#endif
#define config_read_val(val, field)                                           \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fseek (fp, offsetof (struct config_t, field), SEEK_SET);              \
        fread (&(val), sizeof (val), 1, ncap_config_fp);                      \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

#ifdef config_read
#undef config_read
#else
#error macro function config_read_val not defined in  header file
#endif
#define config_read(config)                                                   \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fread (&(config), sizeof (struct config_t), 1, ncap_config_fp);       \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

void
config_logdump ()
{
    logif ("isrepeat:\t%hhu", ncap_config.isrepeat);
    logif ("isshuffle:\t%hhu", ncap_config.isshuffle);
    logif ("aaudio_optimize:\t%hhu", ncap_config.aaudio_optimize);
    logif ("volume:\t%hhu", ncap_config.volume);
    logif ("cur_track:\t%u", ncap_config.cur_track);
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
