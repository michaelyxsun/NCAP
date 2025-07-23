#pragma once

#ifndef PROPERTIES_H
#define PROPERTIES_H

#define APPID "com.msun.ncap"

#define NCAP_DEFAULT_TRACK_PATH "/sdcard/Music/NCAP-share"

#define NCAP_AUDIO_CACHE_FILE     "audio.wav.custom"
#define NCAP_AUDIO_CACHE_FILE_LEN 16

#define NCAP_CONFIG_FILE "ncaprc"

#include "config.h"

extern struct config_t ncap_config;

// compile options

/** turns off debug features */
#define NCAP_NDEBUG

/** for audio debugging: plays each track for max 5 seconds */
// #define DEBUG_TIMED

#endif // !PROPERTIES_H
