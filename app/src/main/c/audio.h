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

extern int libav_cvt_wav (FILE *const fp_in, FILE *const fp_out);

extern int audio_play (const char *fn);

#endif // !AUDIO_H
