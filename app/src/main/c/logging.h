#pragma once

#ifndef LOGGING_H
#define LOGGING_H

#include <android/log.h>
#include <libavutil/error.h>

#define loge(...) __android_log_print (ANDROID_LOG_ERROR, APPID, __VA_ARGS__)
#define logi(...) __android_log_print (ANDROID_LOG_INFO, APPID, __VA_ARGS__)
#define logd(...) __android_log_print (ANDROID_LOG_DEBUG, APPID, __VA_ARGS__)
#define logv(...) __android_log_print (ANDROID_LOG_VERBOSE, APPID, __VA_ARGS__)

#endif // !LOGGING_H
