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
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
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
gen_wav_header (struct cwav_header_t *header, const AVCodecContext *const ctx,
                uint32_t datasiz, uint32_t samples)
{
    // RIFF chunk
    strncpy (header->riff.ckID, "RIFF", 4);
    strncpy (header->riff.WAVEID, "WAVE", 4);
    header->riff.cksize = datasiz + CWAV_HEADER_SIZ - 8;

    // data fmt chunk
    // clang-format off
    strncpy (header->fmt.ckID, "fmt\0", 4);
    header->fmt.cksize     = 16; // 16 is for PCM
    header->fmt.wFormatTag = (int)ctx->sample_fmt % 5; // 1 is S16; 6 is S16P
                                                       // 3 is float; 8 is float planar
                                                       // 0xfffe or 65534 is extended
    const uint32_t channels         = header->fmt.nChannels = ctx->ch_layout.nb_channels;
    const uint32_t sample_rate      = header->fmt.nSamplesPerSec = ctx->sample_rate;
    const uint32_t bytes_per_sample = av_get_bytes_per_sample (ctx->sample_fmt);
    header->fmt.wBitsPerSample  = bytes_per_sample << 3;
    header->fmt.nAvgBytesPerSec = sample_rate * channels * bytes_per_sample;
    header->fmt.nBlockAlign     = channels * bytes_per_sample;
    // clang-format on

    // sampled data chunk
    strncpy (header->data.ckID, "data", sizeof header->data.ckID);
    header->data.cksize = samples * channels * bytes_per_sample;
}

/**
 * @return 0 on success
 */
static int
decode (AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame, FILE *fp_out)
{
    int avret = avcodec_send_packet (ctx, pkt);

    if (avret < 0) {
        loge ("%s: %s: ERROR: avcodec_send_packet failed with code %d: %s\n",
              FILENAME, __func__, avret, av_err2str (avret));
        return avret;
    }

    // read all the output frames (in general there may be any number of them
    while (avret >= 0) {
        avret = avcodec_receive_frame (ctx, frame);

        if (avret == AVERROR (EAGAIN) || avret == AVERROR_EOF) {
            return 0;
        } else if (avret < 0) {
            loge ("%s: %s: ERROR: Decode error with code %d: %s\n", FILENAME,
                  __func__, avret, av_err2str (avret));
            return avret;
        }

        const int datasiz  = av_get_bytes_per_sample (ctx->sample_fmt);
        const int channels = ctx->ch_layout.nb_channels;

        assert (frame->nb_samples * datasiz * channels == frame->linesize[0]);

        if (av_sample_fmt_is_planar (ctx->sample_fmt)) {
            for (int f = 0; f < frame->nb_samples; ++f) {
                for (int ch = 0; ch < channels; ++ch)
                    fwrite (frame->data[ch] + datasiz * f, datasiz, 1, fp_out);

                for (int ch = 0; ch < frame->nb_extended_buf; ++ch)
                    fwrite (frame->extended_data[ch] + datasiz * f, datasiz, 1,
                            fp_out);
            }
        } else {
            fwrite (frame->data[0], 1, frame->linesize[0], fp_out);
        }
    }

    return 0;
}

static const AVCodec *
init_codec (const char *fn, AVFormatContext **fctx)
{
    int avret;

    if ((avret = avformat_open_input (fctx, fn, NULL, NULL)) != 0) {
        loge ("%s: %s: ERROR: avformat_open_input failed with error code %d: "
              "%s\n",
              FILENAME, __func__, avret, av_err2str (avret));
        return NULL;
    }

    if ((avret = avformat_find_stream_info (*fctx, NULL)) < 0) {
        loge (

            "%s: %s: ERROR: avformat_find_stream_info failed with error code "
            "%d\n",
            FILENAME, __func__, avret);
        return NULL;
    }

    logd ("%s: %s: AVFormat format:\t%s\n", FILENAME, __func__,
          (*fctx)->iformat->name);
    logd ("%s: %s: AVFormat duration:\t%" PRId64 "\n", FILENAME, __func__,
          (*fctx)->duration);
    logd ("%s: %s: AVFormat bit rate:\t%" PRId64 "\n", FILENAME, __func__,
          (*fctx)->bit_rate);

    if ((*fctx)->nb_streams != 1) {
        loge ("%s: %s: expected 1 audio input stream, found %d\n", FILENAME,
              __func__, (*fctx)->nb_streams);
        return NULL;
    }

    const AVCodecParameters *const params = (*fctx)->streams[0]->codecpar;

    if (params->codec_type != AVMEDIA_TYPE_AUDIO) {
        loge ("%s: %s: not an input stream, found %d\n", FILENAME, __func__,
              params->codec_type);
        return NULL;
    }

    const AVCodec *codec = avcodec_find_decoder (params->codec_id);

    logd ("%s: %s: codec_id:\t%d\n", FILENAME, __func__, codec->id);
    logd ("%s: %s: codec name:\t%s\n", FILENAME, __func__, codec->name);
    logd ("%s: %s: codec long name:\t%s\n", FILENAME, __func__,
          codec->long_name);

    logd ("%s: %s: channels:\t%d\n", FILENAME, __func__,
          params->ch_layout.nb_channels);
    logd ("%s: %s: block align:\t%d\n", FILENAME, __func__,
          params->block_align);

    return codec;
}

static int
init_codec_context (const AVCodec *const codec, const AVStream *const stream,
                    AVCodecContext **cctx)
{
    int avret;

    if ((avret = avcodec_parameters_to_context (*cctx, stream->codecpar))
        < 0) {
        loge ("%s: %s: ERROR: avcodec_parameters_to_context failed with error "
              "code %d: %s\n",
              FILENAME, __func__, avret, av_err2str (avret));
        return avret;
    }

    if ((avret = avcodec_open2 (*cctx, codec, NULL)) < 0) {
        loge ("%s: %s: avcodec_open2 failed with error code %d: %s\n",
              FILENAME, __func__, avret, av_err2str (avret));
        return avret;
    }

    return 0;
}

int
libav_cvt_cwav (const char *fn_in, const char *fn_out)
{
    logd ("%s: %s: testing fopen `%s' for rb", FILENAME, __func__, fn_in);
    FILE *fp_in = fopen (fn_in, "rb");

    if (fp_in == NULL) {
        loge ("%s: %s: ERROR fopen `%s' for rb failed: errno %d: %s", FILENAME,
              __func__, fn_in, errno, strerror (errno));
        return NCAP_EIO;
    }

    fclose (fp_in);
    logd ("%s: %s: fopen `%s' test succeeded", FILENAME, __func__, fn_in);

    int ret = NCAP_OK;

    logd ("%s: %s: opening file `%s' for wb...", FILENAME, __func__, fn_out);

    FILE *fp_out = fopen (fn_out, "wb"); // 2: deinit_fp_out

    if (fp_out == NULL) {
        loge ("%s: %s: ERROR: fopen `%s' failed for wb: errno %d: %s",
              FILENAME, __func__, fn_out, errno, strerror (errno));
        return NCAP_EIO;
    }

    // init decoder

    logd ("%s: %s: initializing avformat context...", FILENAME, __func__);

    AVFormatContext *fctx = avformat_alloc_context (); // deinit_fctx

    if (fctx == NULL) {
        loge ("%s: %s: ERROR: avformat_alloc_context failed\n", FILENAME,
              __func__);
        ret = NCAP_EALLOC;
        goto deinit_fp_out;
    }

    logd ("%s: %s: initializing codec with init_codec...", FILENAME, __func__);

    const AVCodec *codec = init_codec (fn_in, &fctx);

    if (codec == NULL) {
        loge ("%s: %s: ERROR: avcodec_find_decoder failed\n", FILENAME,
              __func__);
        ret = NCAP_EALLOC;
        goto deinit_fctx;
    }

    logd ("%s: %s: initializing allocating avcodec context...", FILENAME,
          __func__);

    AVCodecContext *cctx = avcodec_alloc_context3 (codec); // deinit_cctx

    if (cctx == NULL) {
        loge ("%s: %s: ERROR: avcodec_alloc_context3 failed\n", FILENAME,
              __func__);
        ret = NCAP_EALLOC;
        goto deinit_fctx;
    }

    logd ("%s: %s: initializing initializing codec context with "
          "init_codec_context...",
          FILENAME, __func__);

    int avret = init_codec_context (codec, fctx->streams[0], &cctx);

    if (avret < 0) {
        loge ("%s: %s: ERROR: init_codec_context failed\n", FILENAME,
              __func__);
        ret = NCAP_EALLOC;
        goto deinit_fctx;
    }

    logd ("%s: %s: initializing initializing parser...", FILENAME, __func__);

    AVCodecParserContext *parser = av_parser_init (codec->id); // deinit_parser

    if (parser == NULL) {
        loge ("%s: %s: ERROR: av_parser_init failed\n", FILENAME, __func__);
        ret = NCAP_EALLOC;
        goto deinit_cctx;
    }

    logd ("%s: %s: allocating frame...", FILENAME, __func__);

    AVFrame *frame = av_frame_alloc (); // deinit_frame

    if (frame == NULL) {
        loge ("%s: %s: ERROR: av_frame_alloc failed\n", FILENAME, __func__);
        ret = NCAP_EALLOC;
        goto deinit_parser;
    }

    logd ("%s: %s: allocating packet...", FILENAME, __func__);

    AVPacket *pkt = av_packet_alloc (); // deinit_pkt

    if (pkt == NULL) {
        loge ("%s: %s: ERROR: av_packet_alloc failed\n", FILENAME, __func__);
        ret = NCAP_EALLOC;
        goto deinit_frame;
    }

    logd ("%s: %s: reserving bytes for WAV header...", FILENAME, __func__);

    // allocate space for WAV header
    fseek (fp_out, CWAV_HEADER_SIZ, SEEK_SET);

    logd ("%s: %s: reading frames...", FILENAME, __func__);

    // decode until eof

    uint32_t samples = 0;

    while (av_read_frame (fctx, pkt) >= 0) {
        if (fctx->streams[pkt->stream_index]->codecpar->codec_type
            != AVMEDIA_TYPE_AUDIO) {
            loge ("%s: %s: ERROR: packet read was not from audio stream. "
                  "stopping...\n",
                  FILENAME, __func__);
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

    logd ("%s: %s: generating header...", FILENAME, __func__);

    // construct and write WAV header
    struct cwav_header_t header;
    gen_wav_header (&header, cctx, ftell (fp_out), samples);
    fseek (fp_out, 0, SEEK_SET);
    fwrite (&header, CWAV_HEADER_SIZ, 1, fp_out);

#ifndef NDEBUG
    // clang-format off
    logv ("%s: %s: WAV header RIFF:\t%.4s",          FILENAME, __func__, header.riff.ckID);
    logv ("%s: %s: WAV header file size:\t%u",       FILENAME, __func__, header.riff.cksize);
    logv ("%s: %s: WAV header WAVE:\t%.4s",          FILENAME, __func__, header.riff.WAVEID);
    logv ("%s: %s: WAV header fmt :\t%.4s",          FILENAME, __func__, header.fmt.ckID);
    logv ("%s: %s: WAV header block size:\t%u",      FILENAME, __func__, header.fmt.cksize);
    logv ("%s: %s: WAV header audio fmt:\t%u",       FILENAME, __func__, header.fmt.wFormatTag);
    logv ("%s: %s: WAV header channels:\t%u",        FILENAME, __func__, header.fmt.nChannels);
    logv ("%s: %s: WAV header sample rate:\t%u",     FILENAME, __func__, header.fmt.nSamplesPerSec);
    logv ("%s: %s: WAV header byte rate:\t%u",       FILENAME, __func__, header.fmt.nAvgBytesPerSec);
    logv ("%s: %s: WAV header block alignment:\t%u", FILENAME, __func__, header.fmt.nBlockAlign);
    logv ("%s: %s: WAV header bits per sample:\t%u", FILENAME, __func__, header.fmt.wBitsPerSample);
    logv ("%s: %s: WAV header data:\t%.4s",          FILENAME, __func__, header.data.ckID);
    logv ("%s: %s: WAV header data size:\t%u",       FILENAME, __func__, header.data.cksize);
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
