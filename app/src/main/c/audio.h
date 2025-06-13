#pragma once

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdio.h>

#define WAV_HEADER_SIZ 44

struct wav_header_t {
    struct {
        char     RIFF[4];
        uint32_t file_siz;
        char     WAVE[4];
    } riff;

    struct {
        char     FMT_[4];
        uint32_t bloc_siz;
        uint16_t audio_format;
        uint16_t channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t bloc_align;
        uint16_t bits_per_sample;
    } format;

    struct {
        char     DATA[4];
        uint32_t data_siz;
    } data;
};

#define NCAP_OK     0
#define NCAP_EGEN   1
#define NCAP_EALLOC 2
#define NCAP_EIO    3
#define NCAP_ENULL  4

// TODO(M-Y-Sun): add err2str

extern int libav_cvt_wav (const char *fn_in, const char *fn_out);

extern int audio_play (const char *fn);

#endif // !AUDIO_H
