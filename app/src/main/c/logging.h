#pragma once

#ifndef LOGGING_H
#define LOGGING_H

#include <android/log.h>
#include <libavutil/error.h>

#include "properties.h"

#define loge(fmt) __android_log_print (ANDROID_LOG_ERROR, APPID, fmt)
#define logw(fmt) __android_log_print (ANDROID_LOG_WARN, APPID, fmt)
#define logi(fmt) __android_log_print (ANDROID_LOG_INFO, APPID, fmt)
#define logd(fmt) __android_log_print (ANDROID_LOG_DEBUG, APPID, fmt)
#define logv(fmt) __android_log_print (ANDROID_LOG_VERBOSE, APPID, fmt)

// clang-format off
#define logef(fmt, ...) __android_log_print (ANDROID_LOG_ERROR, APPID, "%s: %s: " fmt, FILENAME, __func__, __VA_ARGS__)
#define logwf(fmt, ...) __android_log_print (ANDROID_LOG_WARN, APPID, "%s: %s: " fmt, FILENAME, __func__, __VA_ARGS__)
#define logif(fmt, ...) __android_log_print (ANDROID_LOG_INFO, APPID, "%s: %s: " fmt, FILENAME, __func__, __VA_ARGS__)
#define logdf(fmt, ...) __android_log_print (ANDROID_LOG_DEBUG, APPID, "%s: %s: " fmt, FILENAME, __func__, __VA_ARGS__)
#define logvf(fmt, ...) __android_log_print (ANDROID_LOG_VERBOSE, APPID, "%s: %s: " fmt, FILENAME, __func__, __VA_ARGS__)
// clang-format on

#endif // !LOGGING_H
