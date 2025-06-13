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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>

#include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>

#include "audio.h"
#include "logging.h"

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
        loge ("%s: ERROR: avcodec_send_packet failed with code %d: %s\n",
              FILENAME, avret, av_err2str (avret));
        return avret;
    }

    // read all the output frames (in general there may be any number of them
    while (avret >= 0) {
        avret = avcodec_receive_frame (dec_ctx, frame);

        if (avret == AVERROR (EAGAIN) || avret == AVERROR_EOF) {
            return 0;
        } else if (avret < 0) {
            loge ("%s: ERROR: Decode error with code %d: %s\n", FILENAME,
                  avret, av_err2str (avret));
            return avret;
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

static const AVCodec *
init_codec (const char *fn, AVFormatContext **fctx)
{
    int avret;

    if ((avret = avformat_open_input (fctx, fn, NULL, NULL)) != 0) {
        fprintf (stderr,
                 "ERROR: avformat_open_input failed with error code %d: %s\n",
                 avret, av_err2str (avret));
        return NULL;
    }

    if ((avret = avformat_find_stream_info (*fctx, NULL)) < 0) {
        fprintf (
            stderr,
            "ERROR: avformat_find_stream_info failed with error code %d\n",
            avret);
        return NULL;
    }

    logd ("%s: AVFormat format:\t%s\n", FILENAME, (*fctx)->iformat->name);
    logd ("%s: AVFormat duration:\t%" PRId64 "\n", FILENAME,
          (*fctx)->duration);
    logd ("%s: AVFormat bit rate:\t%" PRId64 "\n", FILENAME,
          (*fctx)->bit_rate);

    if ((*fctx)->nb_streams != 1) {
        loge ("%s: expected 1 audio input stream, found %d\n", FILENAME,
              (*fctx)->nb_streams);
        return NULL;
    }

    const AVCodecParameters *const params = (*fctx)->streams[0]->codecpar;

    if (params->codec_type != AVMEDIA_TYPE_AUDIO) {
        loge ("%s: not an input stream, found %d\n", FILENAME,
              params->codec_type);
        return NULL;
    }

    const AVCodec *codec = avcodec_find_decoder (params->codec_id);

    logd ("%s: codec_id:\t%d\n", FILENAME, codec->id);
    logd ("%s: codec name:\t%s\n", FILENAME, codec->name);
    logd ("%s: codec long name:\t%s\n", FILENAME, codec->long_name);

    logd ("%s: channels:\t%d\n", FILENAME, params->ch_layout.nb_channels);
    logd ("%s: block align:\t%d\n", FILENAME, params->block_align);

    return codec;
}

static const AVCodec *
init_codec_context (const AVCodec *const codec, const AVStream *const stream,
                    AVCodecContext **cctx)
{
    int avret;

    if ((avret = avcodec_parameters_to_context (*cctx, stream->codecpar))
        < 0) {
        loge ("%s: ERROR: avcodec_parameters_to_context failed with error "
              "code %d: %s\n",
              FILENAME, avret, av_err2str (avret));
        return NULL;
    }

    if ((avret = avcodec_open2 (*cctx, codec, NULL)) < 0) {
        loge ("%s: avcodec_open2 failed with error code %d: %s\n", FILENAME,
              avret, av_err2str (avret));
        return NULL;
    }

    return codec;
}

int
libav_cvt_wav (const char *fn_in, const char *fn_out)
{
    logd ("%s: testing fopen `%s' for rb", FILENAME, fn_in);
    FILE *fp_in = fopen (fn_in, "rb");

    if (fn_in == NULL) {
        loge ("%s: ERROR fopen `%s' for rb failed", FILENAME, fn_in);
        return EXIT_FAILURE;
    }

    fclose (fp_in);
    logd ("%s: fopen `%s' test succeeded", FILENAME, fn_in);

    int ret = EXIT_SUCCESS;

    FILE *fp_out = fopen (fn_out, "wb"); // 2: deinit_fp_out

    if (fn_out == NULL) {
        loge ("%s: ERROR: fopen `%s' failed for wb\n", FILENAME, fn_out);
        return EXIT_FAILURE;
    }

    // init decoder

    AVFormatContext *fctx = avformat_alloc_context (); // deinit_fctx

    if (fctx == NULL) {
        fprintf (stderr, "ERROR: avformat_alloc_context failed\n");
        ret = EXIT_FAILURE;
        goto deinit_fp_out;
    }

    const AVCodec *codec = init_codec (fn_in, &fctx);

    if (codec == NULL) {
        fprintf (stderr, "ERROR: avcodec_find_decoder failed\n");
        ret = EXIT_FAILURE;
        goto deinit_fctx;
    }

    AVCodecContext *cctx = avcodec_alloc_context3 (codec); // deinit_cctx

    if (cctx == NULL) {
        fprintf (stderr, "ERROR: avcodec_alloc_context3 failed\n");
        ret = EXIT_FAILURE;
        goto deinit_fctx;
    }

    init_codec_context (codec, fctx->streams[0], &cctx);

    AVCodecParserContext *parser = av_parser_init (codec->id); // deinit_parser

    if (parser == NULL) {
        fprintf (stderr, "ERROR: av_parser_init failed\n");
        ret = EXIT_FAILURE;
        goto deinit_cctx;
    }

    AVFrame *frame = av_frame_alloc (); // deinit_frame

    if (frame == NULL) {
        fprintf (stderr, "ERROR: av_frame_alloc failed\n");
        ret = EXIT_FAILURE;
        goto deinit_parser;
    }

    AVPacket *pkt = av_packet_alloc (); // deinit_pkt

    if (pkt == NULL) {
        fprintf (stderr, "av_packet_alloc failed\n");
        ret = EXIT_FAILURE;
        goto deinit_frame;
    }

    // allocate space for WAV header
    fseek (fp_out, WAV_HEADER_SIZ, SEEK_SET);

    // decode until eof

    uint32_t samples = 0;

    while (av_read_frame (fctx, pkt) >= 0) {
        if (fctx->streams[pkt->stream_index]->codecpar->codec_type
            != AVMEDIA_TYPE_AUDIO) {
            loge ("%s: ERROR: packet read was not from audio stream. "
                  "stopping...\n",
                  FILENAME);
            break;
        }

        if (pkt->size <= 0)
            continue;

        decode (cctx, pkt, frame, fp_out);
        samples += frame->nb_samples;
    }

    // flush the decoder
    pkt->data = NULL;
    pkt->size = 0;
    decode (cctx, pkt, frame, fp_out);

    // construct and write WAV header
    struct wav_header_t header;
    gen_wav_header (&header, cctx, ftell (fp_out), samples);
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

deinit_pkt:
    av_packet_free (&pkt);

deinit_frame: // deinit_frame:
    av_frame_free (&frame);

deinit_parser:
    av_parser_close (parser);

deinit_fctx:
    avformat_close_input (&fctx);

deinit_cctx:
    avcodec_free_context (&cctx);

deinit_fp_out:
    fclose (fp_out);

    return ret;
}
