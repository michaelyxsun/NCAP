#pragma once

#ifndef AUDIO_H
#define AUDIO_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define CWAV_HEADER_SIZ 44

struct cwav_header_t {
    struct {
        char     ckID[4];
        uint32_t cksize;
        char     WAVEID[4];
    } riff;

    struct {
        char     ckID[4];
        uint32_t cksize;

        /**
         * 0 is U8, 5 is U8P
         *
         * 1 is S16, 6 is S16P (main)
         *
         * 2 is S32, 7 is S32P (NOT WAV STANDARD)
         *
         * 3 is FLT, 8 is FLTP (main)
         *
         * 4 is DBL, 9 is DBLP (NOT SUPPORTED BY AAUDIO)
         *
         * 10 is S64, 11 is S64P (NOT SUPPORTED BY AAUDIO; detects as U8 AND
         * S16 with mod 5)
         */
        uint16_t wFormatTag;
        uint16_t nChannels;
        uint32_t nSamplesPerSec;
        uint32_t nAvgBytesPerSec;
        uint16_t nBlockAlign;
        uint16_t wBitsPerSample;
    } fmt;

    struct {
        char     ckID[4];
        uint32_t cksize;
    } data;
};

#define NCAP_OK     0
#define NCAP_EGEN   1
#define NCAP_EALLOC 2
#define NCAP_EIO    3
#define NCAP_ENULL  4

// TODO(michaelyxsun): add err2str

extern int libav_cvt_cwav (const char *fn_in, const char *fn_out);

extern int audio_play (const char *fn);

extern void audio_init (void);

extern bool audio_isplaying (void);

extern int audio_resume (void);

extern int audio_pause (void);

#endif // !AUDIO_H
