#pragma once

#ifndef CONFIG_H
#define CONFIG_H

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

extern pthread_mutex_t config_mx;

/**
 * struct config_t should be packed
 */
extern struct config_t {
    uint8_t isrepeat;  // bool
    uint8_t isshuffle; // bool
    /**
     * 0: none (AAUDIO_PERFORMANCE_MODE_NONE)
     *
     * 1: low latench (AAUDIO_PERFORMANCE_MODE_LOW_LATENCY)
     *
     * 2: low latench (AAUDIO_PERFORMANCE_MODE_POWER_SAVING)
     */
    uint8_t  aaudio_optimize;
    uint8_t  volume; // 0 to 100
    uint32_t cur_track;
    uint32_t track_path_len; // includes the null byte
    uint32_t ntracks;
    char    *track_path; // path to media
    uint8_t *track_vols; // volume for each track
                         // NOTE: memsets will not work if this is not 1 byte
} ncap_config;

#define NCAP_CONFIG_SIZ                                                       \
    (sizeof (struct config_t) - (sizeof (char *) + sizeof (uint8_t *)))

extern FILE *ncap_config_fp;

#define CONFIG_ETHRD       -3
#define CONFIG_EMEM        -2
#define CONFIG_ERR         -1
#define CONFIG_OK          0
#define CONFIG_INIT_CREAT  1
#define CONFIG_INIT_EXISTS 2

/** not thread safe */
extern int config_init (const char *fn);

/** not thread safe */
extern int config_deinit (void);

extern int config_read (void);
extern int config_write (void);

#define config_get(val, field, pth_ret)                                       \
    do {                                                                      \
        (pth_ret) = pthread_mutex_lock (&config_mx);                          \
        if ((pth_ret) == 0) {                                                 \
            (val) = ncap_config.field;                                        \
            pthread_mutex_unlock (&config_mx);                                \
        }                                                                     \
    } while (0);

#define config_set(val, field, pth_ret)                                       \
    do {                                                                      \
        (pth_ret) = pthread_mutex_lock (&config_mx);                          \
        if ((pth_ret) == 0) {                                                 \
            ncap_config.field = (val);                                        \
            pthread_mutex_unlock (&config_mx);                                \
        }                                                                     \
    } while (0);

/**
 * `ncap_config.track_vols` should be `NULL` or allocated with `malloc`.
 */
extern int config_upd_vols (uint32_t newsiz, uint8_t def);

extern int config_logdump (void);

extern size_t *config_tord;

/** not thread safe */
extern int config_tord_init (size_t ntracks, unsigned int seed);

/** not thread safe */
extern int config_tord_deinit (void);

extern int config_tord_reshuffle (size_t ntracks);

extern size_t config_tord_at (size_t i, int *_Nullable stat);

/**
 * @return type `aaudio_performance_mode_t` cast to `int`
 */
extern int to_aaudio_pm (uint8_t cfg_code);

#endif // !CONFIG_H
