#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <aaudio/AAudio.h>

#include "audio.h"
#include "logging.h"

static const char *FILENAME = "audio.c";

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
            assert (*siz == 2);
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

int
audio_play (const char *fn)
{
    FILE *fp = fopen (fn, "rb");

    if (fp == NULL) {
        loge ("%s: %s: Failed to open file `%s': error: %s", FILENAME,
              __func__, fn, strerror (errno));
        return NCAP_EIO;
    }

    logi ("%s: %s: Opened file `%s'", FILENAME, __func__, fn);

    struct cwav_header_t header;
    fread (&header, CWAV_HEADER_SIZ, 1, fp);

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
        loge ("%s: %s: ERROR: init_aaudio_fmt failed with code %d\n", FILENAME,
              __func__, stat);
        return NCAP_EGEN;
    }

    logi ("%s: %s: Using AAudio format with code %d", FILENAME, __func__,
          AAUDIO_FMT);
    logi ("%s: %s: Using PCM data width of %zu", FILENAME, __func__,
          PCM_DATA_WIDTH);

    if (AAUDIO_FMT == AAUDIO_FORMAT_UNSPECIFIED)
        logw ("%s: %s: WARN: using AAUDIO_FORMAT_UNSPECIFIED", FILENAME,
              __func__);

    AAudioStreamBuilder_setFormat (builder, AAUDIO_FMT);
    AAudioStreamBuilder_setChannelCount (builder, channels);
    AAudioStreamBuilder_setSampleRate (builder, sample_rate);
    AAudioStreamBuilder_setPerformanceMode (
        builder, AAUDIO_PERFORMANCE_MODE_POWER_SAVING);

    // stream

    AAudioStream *stream;
    res = AAudioStreamBuilder_openStream (builder, &stream);
    AAudioStreamBuilder_delete (builder);

    if (res != AAUDIO_OK) {
        loge ("%s: %s: AAudio openStream failed", FILENAME, __func__);
        return NCAP_EGEN;
    }

    const int32_t frames_per_burst = AAudioStream_getFramesPerBurst (stream);
    const int32_t buf_cap = AAudioStream_getBufferCapacityInFrames (stream);
    int32_t       buf_siz = AAudioStream_getBufferSizeInFrames (stream);

#ifndef NDEBUG
    // clang-format off
    logv ("%s: %s: device id: %d",        FILENAME, __func__, AAudioStream_getDeviceId (stream));
    logv ("%s: %s: direction: %d",        FILENAME, __func__, AAudioStream_getDirection (stream));
    logv ("%s: %s: sharing mode: %d",     FILENAME, __func__, AAudioStream_getSharingMode (stream));
    logv ("%s: %s: stream channels: %d",  FILENAME, __func__, channels);
    logv ("%s: %s: frames_per_burst: %d", FILENAME, __func__, frames_per_burst);
    logv ("%s: %s: sample_rate: %d",      FILENAME, __func__, sample_rate);
    logv ("%s: %s: buf_cap: %d",          FILENAME, __func__, buf_cap);
    logv ("%s: %s: buf_siz: %d",          FILENAME, __func__, buf_siz);
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

    logi ("%s: %s: Stream started. Playing audio...", FILENAME, __func__);

    while (time (NULL) - timer_start < dur && res >= AAUDIO_OK) {
        fread (buf, PCM_DATA_WIDTH, buflen, fp);
        res = AAudioStream_write (stream, buf, frames_per_burst, nstimeout);

        if (buf_siz < buf_cap) {
            int32_t ur_cnt = AAudioStream_getXRunCount (stream);

            logd ("%s: %s: Underruns: %d", FILENAME, __func__, ur_cnt);

            if (ur_cnt > prev_ur_cnt) {
                prev_ur_cnt = ur_cnt;
                buf_siz     = AAudioStream_setBufferSizeInFrames (
                    stream, buf_siz + frames_per_burst);
            }
        }
    }

    if (res < AAUDIO_OK)
        loge ("%s: %s: Write loop stopped due to AAudio error with code %d.",
              FILENAME, __func__, res);

    // deinit

    free (buf);
    fclose (fp);

    logi ("%s: %s: Audio play ended after %u secs. Stopping stream...",
          FILENAME, __func__, (uint32_t)dur);

    AAudioStream_requestStop (stream);
    state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    res   = AAudioStream_waitForStateChange (
        stream, AAUDIO_STREAM_STATE_STOPPING, &state, nstimeout);

    if (res != AAUDIO_OK)
        loge ("%s: %s: AAudio failed to stop. Closing anyway...", FILENAME,
              __func__);
    else
        logi ("%s: %s: AAudio stream stopped.", FILENAME, __func__);

    res = AAudioStream_close (stream);

    if (res != AAUDIO_OK) {
        loge ("%s: %s: AAudio failed to close", FILENAME, __func__);
        return NCAP_EGEN;
    }

    loge ("%s: %s: AAudio stream closed.", FILENAME, __func__);

    return NCAP_OK;
}
