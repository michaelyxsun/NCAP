#include <math.h>
#include <stddef.h>
#include <stdint.h>
// #include <stdio.h>
#include <stdlib.h>

#include <aaudio/AAudio.h>

#include "audio.h"
#include "util.h"

#define TWO_PI (M_PI * 2)

#define HEADER_MAX_SIZ 64

static const char *FILENAME = "audio.c";

static float
gentone (int32_t sample_rate)
{
    static float phase = 0;

    const float amp  = 1;
    const float freq = 440;

    phase += freq * TWO_PI / sample_rate;

    if (phase > TWO_PI)
        phase -= TWO_PI;

    return amp * sinf (phase);
}

int
audio_play (void)
{
    /*
    const char *fn = "/Music/playlist/audio.m4a";
    FILE       *fp = fopen (fn, "rb");

    if (fp == NULL) {
        __android_log_print (ANDROID_LOG_ERROR, APPID,
                             "Failed to open file `%s'", fn);
        return 1;
    } else {
        __android_log_print (ANDROID_LOG_INFO, APPID, "Opened file `%s'", fn);
    }

    // TODO(M-Y-Sun): adjust for different file types
    uint8_t      header[HEADER_MAX_SIZ];
    const size_t headersiz = 2;

    fread (header, sizeof header, headersiz, fp);

    const size_t datasiz = ftell (fp) - headersiz;
    */

    const uint64_t nstimeout = 100000000;
    const int      channels  = 1;

    AAudioStreamBuilder *builder;
    aaudio_result_t      res = AAudio_createStreamBuilder (&builder);

    AAudioStreamBuilder_setChannelCount (builder, channels);
    AAudioStreamBuilder_setFormat (builder, AAUDIO_FORMAT_PCM_FLOAT);
    // AAudioStreamBuilder_setPerformanceMode (
    //     builder, AAUDIO_PERFORMANCE_MODE_POWER_SAVING);

    AAudioStream *stream;
    res = AAudioStreamBuilder_openStream (builder, &stream);
    AAudioStreamBuilder_delete (builder);

    if (res != AAUDIO_OK) {
        loge ("%s: AAudio openStream failed", FILENAME);
        return 1;
    }

    const int32_t frames_per_burst = AAudioStream_getFramesPerBurst (stream);
    const int32_t sample_rate      = AAudioStream_getSampleRate (stream);

    float *buf = malloc (frames_per_burst * channels * sizeof (float));

    logd ("%s: Using frames per burst %u", FILENAME, frames_per_burst);

    AAudioStream_requestStart (stream);
    aaudio_stream_state_t state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    res                         = AAudioStream_waitForStateChange (
        stream, AAUDIO_STREAM_STATE_STARTING, &state, nstimeout);

    logi ("%s: Stream started. Playing audio...", FILENAME);

    while (res >= AAUDIO_OK) {
        float *data = buf;

        for (int32_t f = 0; f < frames_per_burst; ++f) {
            for (int32_t c = 0; c < channels; ++c) {
                *data++ = gentone (sample_rate);
            }
        }

        res = AAudioStream_write (stream, buf, frames_per_burst, nstimeout);
    }

    logi ("%s: Audio play ended. Stopping stream...", FILENAME);

    free (buf);

    AAudioStream_requestStop (stream);
    state = AAUDIO_STREAM_STATE_UNINITIALIZED;
    res   = AAudioStream_waitForStateChange (
        stream, AAUDIO_STREAM_STATE_STOPPING, &state, nstimeout);

    if (res != AAUDIO_OK)
        loge ("%s: AAudio failed to stop. Closing anyway...", FILENAME);

    res = AAudioStream_close (stream);

    if (res != AAUDIO_OK) {
        loge ("%s: AAudio failed to close", FILENAME);
        return 1;
    }

    return 0;
}

#undef HEADER_MAX_SIZ
#undef TWO_PI
