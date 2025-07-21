/**
 * included by audio.h
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <aaudio/AAudio.h>

#include "audio.h"
#include "config.h"
#include "logging.h"
#include "render.h"

static const char *FILENAME = "aaudio_bind.c";

static int
init_aaudio_fmt (int wav_fmt_code, int *fmt, size_t *siz)
{
    switch (wav_fmt_code) {
        case 0:                           // U8
        case 4:                           // DBL
            *fmt = AAUDIO_FORMAT_INVALID; // -1
            *siz = 0;
            return NCAP_EGEN;
        case 1:                           // S16
            *fmt = AAUDIO_FORMAT_PCM_I16; // 1
            *siz = 2;
            return NCAP_OK;
        case 2:                           // S32
            *fmt = AAUDIO_FORMAT_PCM_I32; // 4
            *siz = 4;
            return NCAP_OK;
        case 3:                             // FLT
            *fmt = AAUDIO_FORMAT_PCM_FLOAT; // 2
            *siz = 4;
            return NCAP_OK;
        default:
            *fmt = AAUDIO_FORMAT_UNSPECIFIED; // 0
            *siz = 2;
            return NCAP_EGEN;
    }
}

static void
sclbuf (void *buf, const aaudio_format_t fmt, const size_t width, size_t len)
{
    int     pth_ret;
    uint8_t vol;
    config_get (vol, volume, pth_ret);

    float scl;

    if (pth_ret == 0) {
        scl = vol / 100.0f;
    } else {
        logwf ("WARN: config_get failed with error code %d: %s. Dropping "
               "frame...",
               pth_ret, strerror (pth_ret));
        scl = 0;
    }

    for (; len--; buf += width) {
        switch (fmt) {
            case AAUDIO_FORMAT_PCM_I16:
                *(int16_t *)buf *= scl;
                break;
            case AAUDIO_FORMAT_PCM_I32:
                *(int32_t *)buf *= scl;
                break;
            case AAUDIO_FORMAT_PCM_FLOAT:
                *(float *)buf *= scl;
                break;
            case AAUDIO_FORMAT_INVALID:
            default:
                return;
        }
    }
}

pthread_mutex_t audio_mx     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  audio_cv     = PTHREAD_COND_INITIALIZER;
bool            audio_isplay = false;

pthread_mutex_t audio_int_mx = PTHREAD_MUTEX_INITIALIZER;
bool            audio_int    = false;

int
audio_play (const char *fn)
{
    FILE *fp = fopen (fn, "rb");

    if (fp == NULL) {
        logef ("Failed to open file `%s': error: %s", fn, strerror (errno));
        return NCAP_EIO;
    }

    logif ("Opened file `%s'", fn);

    struct cwav_header_t header;
    fread (&header, CWAV_HEADER_SIZ, 1, fp);

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

    // stream builder

    const uint64_t nstimeout   = 1000000000;
    const uint32_t channels    = header.fmt.nChannels;
    const uint32_t sample_rate = header.fmt.nSamplesPerSec;

    AAudioStreamBuilder *builder;
    aaudio_result_t      res = AAudio_createStreamBuilder (&builder);

    // init aaudio setup data
    int    AAUDIO_FMT;
    size_t PCM_DATA_WIDTH;
    int    stat = init_aaudio_fmt (header.fmt.wFormatTag, &AAUDIO_FMT,
                                   &PCM_DATA_WIDTH);

    if (stat < 0) {
        logef ("ERROR: init_aaudio_fmt failed with code %d\n", stat);
        return NCAP_EGEN;
    }

    logif ("Using AAudio format with code %d", AAUDIO_FMT);
    logif ("Using PCM data width of %zu", PCM_DATA_WIDTH);

    if (AAUDIO_FMT == AAUDIO_FORMAT_UNSPECIFIED)
        logw ("WARN: using AAUDIO_FORMAT_UNSPECIFIED");

    AAudioStreamBuilder_setFormat (builder, AAUDIO_FMT);
    AAudioStreamBuilder_setChannelCount (builder, channels);
    AAudioStreamBuilder_setSampleRate (builder, sample_rate);
    AAudioStreamBuilder_setPerformanceMode (
        builder, to_aaudio_pm (ncap_config.aaudio_optimize));

    // stream

    AAudioStream *stream;
    res = AAudioStreamBuilder_openStream (builder, &stream);
    AAudioStreamBuilder_delete (builder);

    if (res != AAUDIO_OK) {
        loge ("AAudio openStream failed");
        return NCAP_EGEN;
    }

    const int32_t frames_per_burst = AAudioStream_getFramesPerBurst (stream);
    const int32_t buf_cap = AAudioStream_getBufferCapacityInFrames (stream);
    int32_t       buf_siz = AAudioStream_getBufferSizeInFrames (stream);

#ifndef NDEBUG
    // clang-format off
    logvf ("device id: %d",        AAudioStream_getDeviceId (stream));
    logvf ("direction: %d",        AAudioStream_getDirection (stream));
    logvf ("sharing mode: %d",     AAudioStream_getSharingMode (stream));
    logvf ("stream channels: %d",  channels);
    logvf ("frames_per_burst: %d", frames_per_burst);
    logvf ("sample_rate: %d",      sample_rate);
    logvf ("buf_cap: %d",          buf_cap);
    logvf ("buf_siz: %d",          buf_siz);
    // clang-format on
#endif // !NDEBUG

    AAudioStream_requestStart (stream);
    aaudio_stream_state_t state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    res                         = AAudioStream_waitForStateChange (
        stream, AAUDIO_STREAM_STATE_STARTING, &state, nstimeout);

    const size_t buflen      = frames_per_burst * channels;
    void        *buf         = malloc (buflen * PCM_DATA_WIDTH);
    int32_t      prev_ur_cnt = 0;

    const time_t timer_start = time (NULL);
    const time_t dur         = 5;

    logi ("Stream started. Playing audio...");

    int pth_err;
    int ret = NCAP_OK;

    while (res >= AAUDIO_OK && !feof (fp) && time (NULL) - timer_start < dur) {
        // check for interrupt

        if ((pth_err = pthread_mutex_lock (&audio_int_mx)) != 0) {
            logef ("ERROR: pthread_mutex_lock on audio_mx failed with error "
                   "code %d: %s. stopping playback...",
                   pth_err, strerror (pth_err));
            break;
        }

        if (audio_int) {
            audio_int = false;
            ret       = NCAP_INT;
            pthread_mutex_unlock (&audio_int_mx);
            break;
        }

        pthread_mutex_unlock (&audio_int_mx);

        // check for pause (playback control)

        if ((pth_err = pthread_mutex_lock (&audio_mx)) != 0) {
            logef ("ERROR: pthread_mutex_lock on audio_mx failed with error "
                   "code %d: %s. stopping playback...",
                   pth_err, strerror (pth_err));
            break;
        }

        if (!audio_isplay)
            logi ("audio_isplay = false. waiting for audio_cv...");

        while (!audio_isplay)
            pthread_cond_wait (&audio_cv, &audio_mx);

        pthread_mutex_unlock (&audio_mx);

        // check for window close

        if (render_closing_nb ()) {
            logi ("stopping playback...; wclose = true");
            break;
        }

        // play

        if (fread (buf, PCM_DATA_WIDTH, buflen, fp) <= 0)
            logw ("WARN: fread returned with code <= 0");

        sclbuf (buf, AAUDIO_FMT, PCM_DATA_WIDTH, buflen);
        res = AAudioStream_write (stream, buf, frames_per_burst, nstimeout);

        if (buf_siz < buf_cap) {
            int32_t ur_cnt = AAudioStream_getXRunCount (stream);

            logdf ("Underruns: %d", ur_cnt);

            if (ur_cnt > prev_ur_cnt) {
                prev_ur_cnt = ur_cnt;
                buf_siz     = AAudioStream_setBufferSizeInFrames (
                    stream, buf_siz + frames_per_burst);
            }
        }
    }

    if (res < AAUDIO_OK)
        logef ("Write loop stopped due to AAudio error with code %d.", res);

    // deinit

    free (buf);
    fclose (fp);

    logif ("Audio play ended after %u secs. Stopping stream...",
           (uint32_t)dur);

    AAudioStream_requestStop (stream);
    state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    res   = AAudioStream_waitForStateChange (
        stream, AAUDIO_STREAM_STATE_STOPPING, &state, nstimeout);

    if (res != AAUDIO_OK)
        loge ("AAudio failed to stop. Closing anyway...");
    else
        logi ("AAudio stream stopped.");

    res = AAudioStream_close (stream);

    if (res != AAUDIO_OK) {
        loge ("AAudio failed to close");
        return NCAP_EGEN;
    }

    logi ("AAudio stream closed.");

    return ret;
}
