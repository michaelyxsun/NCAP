#pragma once

#ifndef CONFIG_H
#define CONFIG_H

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

extern pthread_mutex_t config_mx;

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
    uint32_t track_path_len;
    char    *track_path; // path to media
} ncap_config;

extern FILE *ncap_config_fp;

#define CONFIG_ERR         -1
#define CONFIG_OK          0
#define CONFIG_INIT_CREAT  1
#define CONFIG_INIT_EXISTS 2

/** synced with config_mx */
extern int config_init (const char *fn);

/** synced with config_mx */
extern int config_deinit (void);

/** synced with config_mx */
extern void config_read (void);

/** synced with config_mx */
extern void config_write (void);

/** synced with config_mx */
#define config_upd(val, field)                                                \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fseek (fp, offsetof (struct config_t, field), SEEK_SET);              \
        fwrite (&(val), sizeof (val), 1, ncap_config_fp);                     \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

/** synced with config_mx */
#define config_read_val(val, field)                                           \
    do {                                                                      \
        pthread_mutex_lock (&config_mx);                                      \
        fseek (fp, offsetof (struct config_t, field), SEEK_SET);              \
        fread (&(val), sizeof (val), 1, ncap_config_fp);                      \
        pthread_mutex_unlock (&config_mx);                                    \
    } while (0);

extern void config_logdump (void);

/**
 * @return type aaudio_performance_mode_t cast to int
 */
extern int to_aaudio_pm (uint8_t cfg_code);

#endif // !CONFIG_H
