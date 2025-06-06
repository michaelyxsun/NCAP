#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <aaudio/AAudio.h>
#include <string.h>

#include "audio.h"
#include "util.h"

#define TWO_PI (M_PI * 2)

#define HEADER_MAX_SIZ 64

static const char *FILENAME = "audio.c";

static float
gentone (int32_t sample_rate)
{
    static float phase = 0;

    const float amp  = 0.5;
    const float freq = 440;

    phase += freq * TWO_PI / sample_rate;

    if (phase > TWO_PI)
        phase -= TWO_PI;

    return amp * sinf (phase);
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

    // TODO(M-Y-Sun): adjust for different file types
    uint8_t      header[HEADER_MAX_SIZ];
    const size_t headersiz = 44;
    fread (header, sizeof (uint8_t), headersiz, fp);

    fseek (fp, 0, SEEK_END);
    const size_t datasiz = ftell (fp) - headersiz;

    fseek (fp, headersiz, SEEK_SET);

    // stream builder

    const int64_t nstimeout = 1000000000;
    const int32_t channels  = 1;

    AAudioStreamBuilder *builder;
    aaudio_result_t      res = AAudio_createStreamBuilder (&builder);

    AAudioStreamBuilder_setChannelCount (builder, channels);
    AAudioStreamBuilder_setFormat (builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode (
        builder, AAUDIO_PERFORMANCE_MODE_POWER_SAVING);

    AAudioStream *stream;
    res = AAudioStreamBuilder_openStream (builder, &stream);
    AAudioStreamBuilder_delete (builder);

    if (res != AAUDIO_OK) {
        loge ("%s: AAudio openStream failed", FILENAME);
        return 1;
    }

    logv ("%s: device id: %d", FILENAME, AAudioStream_getDeviceId (stream));
    logv ("%s: direction: %d", FILENAME, AAudioStream_getDirection (stream));
    logv ("%s: sharing mode: %d", FILENAME,
          AAudioStream_getSharingMode (stream));

    // stream

    const int32_t frames_per_burst = AAudioStream_getFramesPerBurst (stream);
    const int32_t sample_rate      = AAudioStream_getSampleRate (stream);
    const int32_t buf_cap = AAudioStream_getBufferCapacityInFrames (stream);
    int32_t       buf_siz = AAudioStream_getBufferSizeInFrames (stream);

    logv ("%s: frames_per_burst: %d", FILENAME, frames_per_burst);
    logv ("%s: sample_rate: %d", FILENAME, sample_rate);
    logv ("%s: buf_cap: %d", FILENAME, buf_cap);
    logv ("%s: buf_siz: %d", FILENAME, buf_siz);

    AAudioStream_requestStart (stream);
    aaudio_stream_state_t state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    res                         = AAudioStream_waitForStateChange (
        stream, AAUDIO_STREAM_STATE_STARTING, &state, nstimeout);

    const size_t buflen = frames_per_burst * channels;
    float       *buf    = malloc (buflen * sizeof (float));

    int32_t        prev_ur_cnt = 0;
    const uint64_t MAX_ITER    = 100;
    uint64_t       i           = 0;

    logi ("%s: Stream started. Playing audio...", FILENAME);

    while (i++ < MAX_ITER && res >= AAUDIO_OK) {
        fread (buf, sizeof (float), buflen, fp);
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

    logi ("%s: Audio play ended. Stopping stream...", FILENAME);

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
