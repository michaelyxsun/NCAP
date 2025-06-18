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
        logef ("ERROR: avcodec_send_packet failed with code %d: %s\n", avret,
               av_err2str (avret));
        return avret;
    }

    // read all the output frames (in general there may be any number of them
    while (avret >= 0) {
        avret = avcodec_receive_frame (ctx, frame);

        if (avret == AVERROR (EAGAIN) || avret == AVERROR_EOF) {
            return 0;
        } else if (avret < 0) {
            logef ("ERROR: Decode error with code %d: %s\n", avret,
                   av_err2str (avret));
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
        logef ("ERROR: avformat_open_input failed with error code %d: "
               "%s\n",
               avret, av_err2str (avret));
        return NULL;
    }

    if ((avret = avformat_find_stream_info (*fctx, NULL)) < 0) {
        logef (

            "ERROR: avformat_find_stream_info failed with error code "
            "%d\n",
            avret);
        return NULL;
    }

    logdf ("AVFormat format:\t%s\n", (*fctx)->iformat->name);
    logdf ("AVFormat duration:\t%" PRId64 "\n", (*fctx)->duration);
    logdf ("AVFormat bit rate:\t%" PRId64 "\n", (*fctx)->bit_rate);

    if ((*fctx)->nb_streams != 1) {
        logef ("expected 1 audio input stream, found %d\n",
               (*fctx)->nb_streams);
        return NULL;
    }

    const AVCodecParameters *const params = (*fctx)->streams[0]->codecpar;

    if (params->codec_type != AVMEDIA_TYPE_AUDIO) {
        logef ("not an input stream, found %d\n", params->codec_type);
        return NULL;
    }

    const AVCodec *codec = avcodec_find_decoder (params->codec_id);

    logdf ("codec_id:\t%d\n", codec->id);
    logdf ("codec name:\t%s\n", codec->name);
    logdf ("codec long name:\t%s\n", codec->long_name);

    logdf ("channels:\t%d\n", params->ch_layout.nb_channels);
    logdf ("block align:\t%d\n", params->block_align);

    return codec;
}

static int
init_codec_context (const AVCodec *const codec, const AVStream *const stream,
                    AVCodecContext **cctx)
{
    int avret;

    if ((avret = avcodec_parameters_to_context (*cctx, stream->codecpar))
        < 0) {
        logef ("ERROR: avcodec_parameters_to_context failed with error "
               "code %d: %s\n",
               avret, av_err2str (avret));
        return avret;
    }

    if ((avret = avcodec_open2 (*cctx, codec, NULL)) < 0) {
        logef ("avcodec_open2 failed with error code %d: %s\n", avret,
               av_err2str (avret));
        return avret;
    }

    return 0;
}

int
libav_cvt_cwav (const char *fn_in, const char *fn_out)
{
    logdf ("testing fopen `%s' for rb", fn_in);
    FILE *fp_in = fopen (fn_in, "rb");

    if (fp_in == NULL) {
        logef ("ERROR fopen `%s' for rb failed: errno %d: %s", fn_in, errno,
               strerror (errno));
        return NCAP_EIO;
    }

    fclose (fp_in);
    logdf ("fopen `%s' test succeeded", fn_in);

    int ret = NCAP_OK;

    logdf ("opening file `%s' for wb...", fn_out);

    FILE *fp_out = fopen (fn_out, "wb"); // 2: deinit_fp_out

    if (fp_out == NULL) {
        logef ("ERROR: fopen `%s' failed for wb: errno %d: %s", fn_out, errno,
               strerror (errno));
        return NCAP_EIO;
    }

    // init decoder

    logd ("initializing avformat context...");

    AVFormatContext *fctx = avformat_alloc_context (); // deinit_fctx

    if (fctx == NULL) {
        loge ("ERROR: avformat_alloc_context failed\n");
        ret = NCAP_EALLOC;
        goto deinit_fp_out;
    }

    logd ("initializing codec with init_codec...");

    const AVCodec *codec = init_codec (fn_in, &fctx);

    if (codec == NULL) {
        loge ("ERROR: avcodec_find_decoder failed\n");
        ret = NCAP_EALLOC;
        goto deinit_fctx;
    }

    logd ("initializing allocating avcodec context...");

    AVCodecContext *cctx = avcodec_alloc_context3 (codec); // deinit_cctx

    if (cctx == NULL) {
        loge ("ERROR: avcodec_alloc_context3 failed\n");
        ret = NCAP_EALLOC;
        goto deinit_fctx;
    }

    logd ("initializing initializing codec context with "
          "init_codec_context...");

    int avret = init_codec_context (codec, fctx->streams[0], &cctx);

    if (avret < 0) {
        loge ("ERROR: init_codec_context failed\n");
        ret = NCAP_EALLOC;
        goto deinit_fctx;
    }

    logd ("initializing initializing parser...");

    AVCodecParserContext *parser = av_parser_init (codec->id); // deinit_parser

    if (parser == NULL) {
        loge ("ERROR: av_parser_init failed\n");
        ret = NCAP_EALLOC;
        goto deinit_cctx;
    }

    logd ("allocating frame...");

    AVFrame *frame = av_frame_alloc (); // deinit_frame

    if (frame == NULL) {
        loge ("ERROR: av_frame_alloc failed\n");
        ret = NCAP_EALLOC;
        goto deinit_parser;
    }

    logd ("allocating packet...");

    AVPacket *pkt = av_packet_alloc (); // deinit_pkt

    if (pkt == NULL) {
        loge ("ERROR: av_packet_alloc failed\n");
        ret = NCAP_EALLOC;
        goto deinit_frame;
    }

    logd ("reserving bytes for WAV header...");

    // allocate space for WAV header
    fseek (fp_out, CWAV_HEADER_SIZ, SEEK_SET);

    logd ("reading frames...");

    // decode until eof

    uint32_t samples = 0;

    while (av_read_frame (fctx, pkt) >= 0) {
        if (fctx->streams[pkt->stream_index]->codecpar->codec_type
            != AVMEDIA_TYPE_AUDIO) {
            loge ("ERROR: packet read was not from audio stream. "
                  "stopping...\n");
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

    logd ("generating header...");

    // construct and write WAV header
    struct cwav_header_t header;
    gen_wav_header (&header, cctx, ftell (fp_out), samples);
    fseek (fp_out, 0, SEEK_SET);
    fwrite (&header, CWAV_HEADER_SIZ, 1, fp_out);

#ifndef NDEBUG
    // clang-format off
    logvf ("WAV header RIFF:\t%.4s",          header.riff.ckID);
    logvf ("WAV header file size:\t%u",       header.riff.cksize);
    logvf ("WAV header WAVE:\t%.4s",          header.riff.WAVEID);
    logvf ("WAV header fmt :\t%.4s",          header.fmt.ckID);
    logvf ("WAV header block size:\t%u",      header.fmt.cksize);
    logvf ("WAV header audio fmt:\t%u",       header.fmt.wFormatTag);
    logvf ("WAV header channels:\t%u",        header.fmt.nChannels);
    logvf ("WAV header sample rate:\t%u",     header.fmt.nSamplesPerSec);
    logvf ("WAV header byte rate:\t%u",       header.fmt.nAvgBytesPerSec);
    logvf ("WAV header block alignment:\t%u", header.fmt.nBlockAlign);
    logvf ("WAV header bits per sample:\t%u", header.fmt.wBitsPerSample);
    logvf ("WAV header data:\t%.4s",          header.data.ckID);
    logvf ("WAV header data size:\t%u",       header.data.cksize);
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
