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

static void
init_aaudio_fmt (int wav_fmt_code, int *fmt, size_t *siz)
{
    switch (wav_fmt_code) {
        case 1:
            *fmt = AAUDIO_FORMAT_PCM_I16; // 1
            assert (*siz == 2);
            break;
        case 2:
            *fmt = AAUDIO_FORMAT_PCM_I32; // 4
            *siz = 4;
            break;
        case 3:
            *fmt = AAUDIO_FORMAT_PCM_FLOAT; // 2
            *siz = 4;
            break;
        default:
            *fmt = AAUDIO_FORMAT_UNSPECIFIED;
            *siz = 2;
    }
}

int
audio_play (const char *fn)
{
    FILE *fp = fopen (fn, "rb");

    if (fp == NULL) {
        loge ("%s: Failed to open file `%s': error: %s", FILENAME, fn,
              strerror (errno));
        return NCAP_EIO;
    }

    logi ("%s: Opened file `%s'", FILENAME, fn);

    struct wav_header_t header;
    fread (&header, WAV_HEADER_SIZ, 1, fp);

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

    // stream builder

    const uint64_t nstimeout   = 1000000000;
    const uint32_t channels    = header.format.channels;
    const uint32_t sample_rate = header.format.sample_rate;

    AAudioStreamBuilder *builder;
    aaudio_result_t      res = AAudio_createStreamBuilder (&builder);

    // init aaudio setup data
    int    AAUDIO_FMT;
    size_t PCM_DATA_WIDTH;
    init_aaudio_fmt (header.format.audio_format, &AAUDIO_FMT, &PCM_DATA_WIDTH);

    logi ("%s: Using AAudio format with code %d", FILENAME, AAUDIO_FMT);
    logi ("%s: Using PCM data width of %zu", FILENAME, PCM_DATA_WIDTH);

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
        loge ("%s: AAudio openStream failed", FILENAME);
        return NCAP_EGEN;
    }

    const int32_t frames_per_burst = AAudioStream_getFramesPerBurst (stream);
    const int32_t buf_cap = AAudioStream_getBufferCapacityInFrames (stream);
    int32_t       buf_siz = AAudioStream_getBufferSizeInFrames (stream);

#ifndef NDEBUG
    // clang-format off
    logv ("%s: device id: %d",        FILENAME, AAudioStream_getDeviceId (stream));
    logv ("%s: direction: %d",        FILENAME, AAudioStream_getDirection (stream));
    logv ("%s: sharing mode: %d",     FILENAME, AAudioStream_getSharingMode (stream));
    logv ("%s: stream channels: %d",  FILENAME, channels);
    logv ("%s: frames_per_burst: %d", FILENAME, frames_per_burst);
    logv ("%s: sample_rate: %d",      FILENAME, sample_rate);
    logv ("%s: buf_cap: %d",          FILENAME, buf_cap);
    logv ("%s: buf_siz: %d",          FILENAME, buf_siz);
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

    logi ("%s: Stream started. Playing audio...", FILENAME);

    while (time (NULL) - timer_start < dur && res >= AAUDIO_OK) {
        fread (buf, PCM_DATA_WIDTH, buflen, fp);
        res = AAudioStream_write (stream, buf, frames_per_burst, nstimeout);

        if (buf_siz < buf_cap) {
            int32_t ur_cnt = AAudioStream_getXRunCount (stream);

            logd ("%s: Underruns: %d", FILENAME, ur_cnt);

            if (ur_cnt > prev_ur_cnt) {
                prev_ur_cnt = ur_cnt;
                buf_siz     = AAudioStream_setBufferSizeInFrames (
                    stream, buf_siz + frames_per_burst);
            }
        }
    }

    if (res < AAUDIO_OK)
        loge ("%s: Write loop stopped due to AAudio error with code %d.",
              FILENAME, res);

    // deinit

    free (buf);
    fclose (fp);

    logi ("%s: Audio play ended after %u secs. Stopping stream...", FILENAME,
          (uint32_t)dur);

    AAudioStream_requestStop (stream);
    state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    res   = AAudioStream_waitForStateChange (
        stream, AAUDIO_STREAM_STATE_STOPPING, &state, nstimeout);

    if (res != AAUDIO_OK)
        loge ("%s: AAudio failed to stop. Closing anyway...", FILENAME);
    else
        logi ("%s: AAudio stream stopped.", FILENAME);

    res = AAudioStream_close (stream);

    if (res != AAUDIO_OK) {
        loge ("%s: AAudio failed to close", FILENAME);
        return NCAP_EGEN;
    }

    loge ("%s: AAudio stream closed.", FILENAME);

    return NCAP_OK;
}
