#pragma once

#ifndef RENDER_H
#define RENDER_H

#include <pthread.h>
#include <stdbool.h>

#include <android_native_app_glue.h>

extern struct android_app *GetAndroidApp (void);

extern bool            render_ready;
extern pthread_mutex_t render_mx;
extern pthread_cond_t  render_cv;

extern void render (void);

#endif // !RENDER_H
