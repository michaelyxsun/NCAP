#pragma once

#ifndef RENDER_H
#define RENDER_H

#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>

#include "strvec.h"

#include <android_native_app_glue.h>

extern struct android_app *GetAndroidApp (void);

#define MAX_OBJS    32
#define OBJ_BUF_SIZ 1024

enum obj_typ_e {
    RL_LINE,
    RL_CIRC,
    RL_RECT,
    RL_TRI,
    RL_TEXT,
};

struct rl_line_arg_t {
    Vector2 v1;
    Vector2 v2;
    Color   color;
};

struct rl_circ_arg_t {
    Vector2 c;
    float   r;
    Color   color;
};

struct rl_rect_arg_t {
    Vector2 pos;
    Vector2 siz;
    Color   color;
};

struct rl_tri_arg_t {
    Vector2 v1;
    Vector2 v2;
    Vector2 v3;
    Color   color;
};

struct rl_text_arg_t {
    char *str;
    int   x;
    int   y;
    int   fsiz;
    Color color;
};

struct obj_t {
    void          *params;
    enum obj_typ_e typ;
    bool           dyn;
    void (*act) (void);
};

extern pthread_mutex_t render_ready_mx;
extern pthread_cond_t  render_ready_cv;
extern bool            render_ready;

extern pthread_mutex_t render_atrid_mx;
extern int             render_atrid;

extern void render (const strvec_t *sv);

#endif // !RENDER_H
