#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <aaudio/AAudio.h>

#include "config.h"
#include "logging.h"

static const char *FILENAME = "config.c";

FILE *ncap_config_fp = NULL;

pthread_mutex_t config_mx = PTHREAD_MUTEX_INITIALIZER;

int
config_init (const char *fn)
{
    pthread_mutex_lock (&config_mx);

    int ret;

    if ((ncap_config_fp = fopen (fn, "wb+")) == NULL) {
        logef ("ERROR: could not open config file `%s' for wb+: %s", fn,
               strerror (errno));
        ret = CONFIG_ERR;
        goto exit;
    }

    ret = ungetc (fgetc (ncap_config_fp), ncap_config_fp) == EOF
              ? CONFIG_INIT_CREAT
              : CONFIG_INIT_EXISTS;

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

#ifdef set_config
#undef set_config
#endif
#define set_config(config, fp)                                                \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fwrite (&(config), sizeof (struct config_t), 1, fp);                  \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

#ifdef upd_config
#undef upd_config
#endif
#define upd_config(val, field, fp)                                            \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fseek (fp, offsetof (struct config_t, field), SEEK_SET);              \
        fwrite (&(val), sizeof (val), 1, fp);                                 \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

#ifdef read_config
#undef read_config
#endif
#define read_config(val, field, fp)                                           \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fseek (fp, offsetof (struct config_t, field), SEEK_SET);              \
        fread (&(val), sizeof (val), 1, fp);                                  \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

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
