#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <aaudio/AAudio.h>
#include <string.h>

#include "audio.h"
#include "util.h"

typedef int16_t pcm_data_t;

static const char *FILENAME = "audio.c";

void
parse_header_wav (struct wav_header_t *header, FILE *const fp)
{
    static_assert (sizeof (struct wav_header_t) == WAV_HEADER_SIZ,
                   "Macro constant WAV_HEADER_SIZ does not match computed "
                   "size of `struct wav_header_t' using `sizeof'");

    fread (header, WAV_HEADER_SIZ, 1, fp);
}

int
audio_play (const char *fn)
{
    FILE *fp = fopen (fn, "rb");

    if (fp == NULL) {
        loge ("%s: Failed to open file `%s': error: %s", FILENAME, fn,
              strerror (errno));
        return 1;
    }

    logi ("%s: Opened file `%s'", FILENAME, fn);

    struct wav_header_t header;
    parse_header_wav (&header, fp);

#ifndef NDEBUG
    // clang-format off
    logv ("%s: WAV file header file size: %u\t",        FILENAME, header.riff.file_siz);
    logv ("%s: WAV file header block size: %u\t",       FILENAME, header.format.bloc_siz);
    logv ("%s: WAV file header audio format: %u\t",     FILENAME, header.format.audio_format);
    logv ("%s: WAV file header channels: %u\t",         FILENAME, header.format.channels);
    logv ("%s: WAV file header sample rate: %u\t",        FILENAME, header.format.sample_rate);
    logv ("%s: WAV file header bytes per second: %u\t", FILENAME, header.format.bytes_per_sec);
    logv ("%s: WAV file header bytes per block: %u\t",  FILENAME, header.format.bytes_per_bloc);
    logv ("%s: WAV file header bits per sample: %u\t",  FILENAME, header.format.bits_per_sample);
    logv ("%s: WAV file header data size: %u\t",        FILENAME, header.data.data_siz);
// clang-format on
#endif // !NDEBUG

    // stream builder

    const int64_t nstimeout   = 1000000000;
    const int32_t channels    = header.format.channels;
    const int32_t sample_rate = header.format.sample_rate;

    AAudioStreamBuilder *builder;
    aaudio_result_t      res = AAudio_createStreamBuilder (&builder);

    AAudioStreamBuilder_setFormat (builder, AAUDIO_FORMAT_PCM_I16);
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
        return 1;
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
    pcm_data_t  *buf         = malloc (buflen * sizeof (pcm_data_t));
    int32_t      prev_ur_cnt = 0;

    const time_t timer_start = time (NULL);
    const time_t dur         = 5;

    logi ("%s: Stream started. Playing audio...", FILENAME);

    while (time (NULL) - timer_start < dur && res >= AAUDIO_OK) {
        fread (buf, sizeof (pcm_data_t), buflen, fp);
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

    logi ("%s: Audio play ended after %ld secs. Stopping stream...", FILENAME,
          dur);

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
        return 1;
    }

    loge ("%s: AAudio stream closed.", FILENAME);

    return 0;
}

#undef HEADER_MAX_SIZ
#undef TWO_PI
