/*
 * Code based on FFmpeg official example at
 * https://ffmpeg.org/doxygen/trunk/decode_audio_8c-example.html
 * with license copied below
 */

/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include <libavutil/frame.h>
#include <libavutil/mem.h>

#include <libavcodec/avcodec.h>

#include "audio.h"
#include "util.h"

#define AUDIO_INBUF_SIZE    20480
#define AUDIO_REFILL_THRESH 4096

static const char *FILENAME = "libav_bind.c";

/**
 * call after initialization of ctx
 */
static void
gen_wav_header (struct wav_header_t *header, const AVCodecContext *const ctx,
                uint32_t datasiz, uint32_t samples)
{
    // RIFF chunk
    strncpy (header->riff.RIFF, "RIFF", sizeof header->riff.RIFF);
    strncpy (header->riff.WAVE, "WAVE", sizeof header->riff.WAVE);
    header->riff.file_siz = datasiz + WAV_HEADER_SIZ - 8;

    // data format chunk
    // clang-format off
    strncpy (header->format.FMT_, "fmt\0", sizeof header->format.FMT_);
    header->format.bloc_siz     = 16; // 16 is for PCM
    header->format.audio_format = (int)ctx->sample_fmt % 5; // 1 is S16; 6 is S16P
                                                            // 3 is float; 8 is float planar
    const uint32_t channels     = header->format.channels    = ctx->ch_layout.nb_channels;
    const uint32_t sample_rate  = header->format.sample_rate = ctx->sample_rate;
    const uint32_t bytes_per_sample = av_get_bytes_per_sample (ctx->sample_fmt);
    header->format.bits_per_sample  = bytes_per_sample << 3;
    header->format.byte_rate        = sample_rate * channels * bytes_per_sample;
    header->format.bloc_align       = channels * bytes_per_sample;
    // clang-format on

    // sampled data chunk
    strncpy (header->data.DATA, "data", sizeof header->data.DATA);
    header->data.data_siz = samples * channels * bytes_per_sample;
}

/**
 * @return 0 on success
 */
static int
decode (AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, FILE *fp_out)
{
    int avret = avcodec_send_packet (dec_ctx, pkt);

    if (avret < 0) {
        fprintf (stderr, "ERROR: avcodec_send_packet failed with code %d\n",
                 avret);
        return -1;
    }

    // read all the output frames (in general there may be any number of them
    while (avret >= 0) {
        avret = avcodec_receive_frame (dec_ctx, frame);

        if (avret == AVERROR (EAGAIN) || avret == AVERROR_EOF) {
            return 0;
        } else if (avret < 0) {
            fprintf (stderr, "ERROR: Decode error with code %d\n", avret);
            return -1;
        }

        int datasiz = av_get_bytes_per_sample (dec_ctx->sample_fmt);

        for (int f = 0; f < frame->nb_samples; f++) {
            for (int ch = 0; ch < dec_ctx->ch_layout.nb_channels; ch++) {
                fwrite (frame->data[ch] + datasiz * f, 1, datasiz, fp_out);
            }
        }
    }

    return 0;
}

int
libav_cvt_wav (FILE *const fp_in, FILE *const fp_out)
{
    int ret = EXIT_SUCCESS;

    AVPacket *pkt = av_packet_alloc (); // 1: deinit_pkt

    // TODO(M-Y-SUN): detect audio decoder
    const AVCodec *codec = avcodec_find_decoder (AV_CODEC_ID_MP3);

    if (codec == NULL) {
        fprintf (stderr, "ERROR: avcodec_find_decoder failed\n");
        return EXIT_FAILURE;
    }

    AVCodecParserContext *parser
        = av_parser_init (codec->id); // 2: deinit_parser

    if (parser == NULL) {
        fprintf (stderr, "ERROR: av_parser_init failed\n");
        ret = EXIT_FAILURE;
        goto deinit_pkt;
    }

    AVCodecContext *ctx = avcodec_alloc_context3 (codec); // 3: deinit_ctx

    if (ctx == NULL) {
        fprintf (stderr, "ERROR: avcodec_alloc_context3 failed\n");
        ret = EXIT_FAILURE;
        goto deinit_parser;
    }

    int avret = avcodec_open2 (ctx, codec, NULL);

    if (avret < 0) {
        fprintf (stderr, "ERROR: avcodec_open2 failed with code %d\n", avret);
        ret = EXIT_FAILURE;
        goto deinit_ctx;
    }

    static uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];

    AVFrame *frame = av_frame_alloc (); // 6: deinit_frame

    if (frame == NULL) {
        fprintf (stderr, "ERROR: av_frame_alloc failed\n");
        ret = EXIT_FAILURE;
        goto deinit_fout;
    }

    // allocate space for WAV header
    fseek (fp_out, WAV_HEADER_SIZ, SEEK_SET);

    // decode until eof

    uint32_t samples = 0;
    uint8_t *data    = inbuf;
    size_t datasiz = fread (inbuf, sizeof (uint8_t), AUDIO_INBUF_SIZE, fp_in);

    while (datasiz > 0) {
        int nbytes
            = av_parser_parse2 (parser, ctx, &pkt->data, &pkt->size, data,
                                datasiz, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        if (nbytes < 0) {
            fprintf (stderr, "ERROR: av_parser_parse2 failed with code %d\n",
                     nbytes);
            ret = EXIT_FAILURE;
            goto deinit;
        }

        data += nbytes;
        datasiz -= nbytes;

        if (pkt->size > 0) {
            decode (ctx, pkt, frame, fp_out);
            samples += frame->nb_samples;
        }

        if (datasiz < AUDIO_REFILL_THRESH) {
            memmove (inbuf, data, datasiz);
            data       = inbuf;
            size_t len = fread (data + datasiz, sizeof (uint8_t),
                                AUDIO_INBUF_SIZE - datasiz, fp_in);

            if (len > 0)
                datasiz += len;
        }
    }

    // flush the decoder
    pkt->data = NULL;
    pkt->size = 0;
    decode (ctx, pkt, frame, fp_out);
    samples += frame->nb_samples;

    // construct and write WAV header
    struct wav_header_t header;
    gen_wav_header (&header, ctx, ftell (fp_out), samples);
    fseek (fp_out, 0, SEEK_SET);
    fwrite (&header, WAV_HEADER_SIZ, 1, fp_out);

#ifndef NDEBUG
    // clang-format off
    logv ("%s: WAV header RIFF:\t%.4s",          FILENAME, header.riff.RIFF);
    logv ("%s: WAV header file size:\t%u",       FILENAME, header.riff.file_siz);
    logv ("%s: WAV header WAVE:\t%.4s",          FILENAME, header.riff.WAVE);
    logv ("%s: WAV header fmt :\t%.4s",          FILENAME, header.format.FMT_);
    logv ("%s: WAV header block size:\t%u",      FILENAME, header.format.bloc_siz);
    logv ("%s: WAV header audio format:\t%u",    FILENAME, header.format.audio_format);
    logv ("%s: WAV header channels:\t%u",        FILENAME, header.format.channels);
    logv ("%s: WAV header sample rate:\t%u",     FILENAME, header.format.sample_rate);
    logv ("%s: WAV header byte rate:\t%u",       FILENAME, header.format.byte_rate);
    logv ("%s: WAV header block alignment:\t%u", FILENAME, header.format.bloc_align);
    logv ("%s: WAV header bits per sample:\t%u", FILENAME, header.format.bits_per_sample);
    logv ("%s: WAV header data:\t%.4s",          FILENAME, header.data.DATA);
    logv ("%s: WAV header data size:\t%u",       FILENAME, header.data.data_siz);
// clang-format on
#endif // !NDEBUG

deinit: // deinit_frame:
    av_frame_free (&frame);

deinit_fout:
    fclose (fp_out);

deinit_fin:
    fclose (fp_in);

deinit_ctx:
    avcodec_free_context (&ctx);

deinit_parser:
    av_parser_close (parser);

deinit_pkt:
    av_packet_free (&pkt);

    return 0;
}
