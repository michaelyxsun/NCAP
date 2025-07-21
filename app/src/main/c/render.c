#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "logging.h"
#include "render.h"
#include "strvec.h"
#include "time.h"

static const char *FILENAME = "render.c";

pthread_mutex_t render_wclose_mx = PTHREAD_MUTEX_INITIALIZER;
bool            wclose           = false;

pthread_mutex_t render_ready_mx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  render_ready_cv = PTHREAD_COND_INITIALIZER;
bool            render_ready    = false;

pthread_mutex_t render_atrid_mx = PTHREAD_MUTEX_INITIALIZER;
int             render_atrid    = -1;

static const int FPS_ACTIVE = 30;
static const int FPS_STATIC = 10;
static const int FONTSIZ    = 60;
static int       fps        = FPS_STATIC;

static const struct timespec retry_ts
    = { .tv_sec = 0, .tv_nsec = 250000000 }; // 250 ms

void
render_init (void)
{
    wclose = false;
}

static void
act_wclose (struct obj_t *this)
{
    int ret;

    if ((ret = render_close ()) != 0) {
        logwf (
            "WARN: render_close failed with error code %d: %s. continuing...",
            ret, strerror (ret));
    }

    // signal to interrupt aaudio hang

    while ((ret = audio_resume ()) != 0) {
        logwf ("WARN: audio_resume failed with error code %d: %s. Retrying...",
               ret, strerror (ret));
        nanosleep (&retry_ts, NULL);
    }
}

static void
act_toggleplay (struct obj_t *this)
{
    logi ("act_toggleplay signaled");

    struct rl_rect_arg_t *const par     = this->params;
    struct rl_text_arg_t *const linkpar = this->link->params;

    int aret;

    if (audio_isplaying ()) {
        while ((aret = audio_pause ()) != 0) {
            logwf (
                "WARN: audio_pause failed with error code %d: %s. Retrying...",
                aret, strerror (aret));
            nanosleep (&retry_ts, NULL);
        }

        memcpy (linkpar->str, " play", 6);
        par->color = DARKGREEN;
    } else {
        while ((aret = audio_resume ()) != 0) {
            logwf ("WARN: audio_resume failed with error code %d: %s. "
                   "Retrying...",
                   aret, strerror (aret));
            nanosleep (&retry_ts, NULL);
        }

        memcpy (linkpar->str, "pause", 6);
        par->color = MAROON;
    }
}

static void
act_incvol (struct obj_t *this)
{
    logi ("act_incvol called");

    struct rl_text_arg_t *const linkpar = this->link->params;
    int                         pth_ret;

    uint8_t vol;
    config_get (vol, volume, pth_ret);

    if (pth_ret != 0) {
        logwf ("WARN: config_get failed with exit code %d: %s. aborting...",
               pth_ret, strerror (pth_ret));
        return;
    }

    if (vol <= 90) {
        config_set (vol += 10, volume, pth_ret);

        if (pth_ret != 0) {
            logwf (
                "WARN: config_set failed with exit code %d: %s. aborting...",
                pth_ret, strerror (pth_ret));
            return;
        }

        snprintf (linkpar->str, 5, "%hhu%%", vol);
    }
}

static void
act_decvol (struct obj_t *this)
{
    logi ("act_decvol called");

    struct rl_text_arg_t *const linkpar = this->link->params;
    int                         pth_ret;

    uint8_t vol;
    config_get (vol, volume, pth_ret);

    if (pth_ret != 0) {
        logwf ("WARN: config_get failed with exit code %d: %s. aborting...",
               pth_ret, strerror (pth_ret));
        return;
    }

    if (vol >= 10) {
        config_set (vol -= 10, volume, pth_ret);

        if (pth_ret != 0) {
            logwf (
                "WARN: config_set failed with exit code %d: %s. aborting...",
                pth_ret, strerror (pth_ret));
            return;
        }

        snprintf (linkpar->str, 5, "%hhu%%", vol);
    }
}

/**
 * 0: close text background
 * 1: close text
 * 2: prompt text
 */
static struct obj_t objs[MAX_OBJS];
static size_t       objs_len;

static struct rl_rect_arg_t rectbg;

static void
init_objs (const int SCW, const int SCH)
{
    int                   x, y, w, h, pad, add;
    struct rl_text_arg_t *textarg;
    struct rl_rect_arg_t *rectarg;
    struct rl_tri_arg_t  *triarg;
    int                   pth_ret;

    // rectangle background

    rectarg = objs[0].params = &rectbg;
    objs[0].typ              = RL_RECT;
    objs[0].dyn              = false;

    w = rectarg->siz.x = SCW * 0.8;
    h = rectarg->siz.y = SCH * 0.4;
    rectarg->pos.x     = (SCW - w) >> 1;
    rectarg->pos.y     = ((SCH - h) >> 1) - 200;
    rectarg->color     = DARKGRAY;

    // close text

    static struct rl_text_arg_t objs2;
    textarg = objs[2].params = &objs2;
    objs[2].typ              = RL_TEXT;
    objs[2].dyn              = false;

    textarg->str  = "close";
    textarg->fsiz = FONTSIZ + 20;
    w             = MeasureText (textarg->str, textarg->fsiz);
    h             = textarg->fsiz;
    x = textarg->x = SCW - w - 90;
    y = textarg->y = 90;
    textarg->color = WHITE;

    // close text background (tappable)

    static struct rl_rect_arg_t objs1;
    rectarg = objs[1].params = &objs1;
    objs[1].typ              = RL_RECT;
    objs[1].dyn              = true;
    objs[1].act              = act_wclose;
    objs[1].link             = &objs[2];

    rectarg->siz.x = w + 64;
    rectarg->siz.y = h + 32;
    rectarg->pos.x = x - 32;
    rectarg->pos.y = y - 16;
    rectarg->color = RED;

    static struct rl_text_arg_t objs4;
    textarg = objs[4].params = &objs4;
    objs[4].typ              = RL_TEXT;
    objs[4].dyn              = false;

    static char playctl_str[6] = " play";
    textarg->str               = playctl_str;
    textarg->fsiz              = FONTSIZ + 10;
    w                          = MeasureText ("pause", textarg->fsiz);
    h                          = textarg->fsiz;
    x = textarg->x = (SCW - w) >> 1;
    y = textarg->y = rectbg.pos.y - h - 30;
    textarg->color = WHITE;

    // play/pause button background (tappable)

    static struct rl_rect_arg_t objs3;
    rectarg = objs[3].params = &objs3;
    objs[3].typ              = RL_RECT;
    objs[3].dyn              = true;
    objs[3].act              = act_toggleplay;
    objs[3].link             = &objs[4];

    rectarg->siz.x = w + 64;
    rectarg->siz.y = h + 32;
    rectarg->pos.x = x - 32;
    rectarg->pos.y = y - 16;
    rectarg->color = DARKGREEN;

    // volume up (tappable)

    static struct rl_tri_arg_t objs5;
    triarg = objs[5].params = &objs5;
    objs[5].typ             = RL_TRI;
    objs[5].dyn             = true;
    objs[5].act             = act_incvol;
    objs[5].link            = &objs[7];

    w = 160;
    x = rectbg.pos.x;
    y = rectbg.pos.y + rectbg.siz.y + 16;

    triarg->v1.x = x;
    triarg->v2.x = x + w;
    triarg->v1.y = triarg->v2.y = y + w;
    triarg->v3.x                = x + (w >> 1);
    triarg->v3.y                = y;
    triarg->color               = MAROON;

    // volume down (tappable)

    static struct rl_tri_arg_t objs6;
    triarg = objs[6].params = &objs6;
    objs[6].typ             = RL_TRI;
    objs[6].dyn             = true;
    objs[6].act             = act_decvol;
    objs[6].link            = &objs[7];

    x = rectbg.pos.x;
    y += w + 16;

    triarg->v1.x = x + (w >> 1);
    triarg->v1.y = y + w;
    triarg->v2.x = x + w;
    triarg->v3.x = x;
    triarg->v2.y = triarg->v3.y = y;
    triarg->color               = MAROON;

    y += w;

    static struct rl_text_arg_t objs7;
    textarg = objs[7].params = &objs7;
    objs[7].typ              = RL_TEXT;
    objs[7].dyn              = false;

    static char vol_str[5];

    uint8_t vol;

    do {
        config_get (vol, volume, pth_ret);
        logdf ("config_get returned code %d: %s", pth_ret, strerror (pth_ret));
    } while (pth_ret != 0);

    snprintf (vol_str, sizeof vol_str, "%hhu%%", vol);

    textarg->str   = vol_str;
    textarg->fsiz  = FONTSIZ + 10;
    w              = MeasureText ("100%", textarg->fsiz);
    h              = textarg->fsiz;
    textarg->x     = x;
    textarg->y     = y + 32;
    textarg->color = BLACK;

    objs_len = 8;
}

static void
draw (const struct obj_t *const obj)
{
    switch (obj->typ) {
        case RL_LINE:
            DrawLineV (((struct rl_line_arg_t *)obj->params)->v1,
                       ((struct rl_line_arg_t *)obj->params)->v2,
                       ((struct rl_line_arg_t *)obj->params)->color);
            break;
        case RL_CIRC:
            DrawCircleV (((struct rl_circ_arg_t *)obj->params)->c,
                         ((struct rl_circ_arg_t *)obj->params)->r,
                         ((struct rl_circ_arg_t *)obj->params)->color);

            break;
        case RL_RECT:
            DrawRectangleV (((struct rl_rect_arg_t *)obj->params)->pos,
                            ((struct rl_rect_arg_t *)obj->params)->siz,
                            ((struct rl_rect_arg_t *)obj->params)->color);

            break;
        case RL_TRI:
            DrawTriangle (((struct rl_tri_arg_t *)obj->params)->v1,
                          ((struct rl_tri_arg_t *)obj->params)->v2,
                          ((struct rl_tri_arg_t *)obj->params)->v3,
                          ((struct rl_tri_arg_t *)obj->params)->color);

            break;
        case RL_TEXT:
            DrawText (((struct rl_text_arg_t *)obj->params)->str,
                      ((struct rl_text_arg_t *)obj->params)->x,
                      ((struct rl_text_arg_t *)obj->params)->y,
                      ((struct rl_text_arg_t *)obj->params)->fsiz,
                      ((struct rl_text_arg_t *)obj->params)->color);

            break;
        default:
            break;
    }
}

#define vsub(v1, v2)                                                          \
    do {                                                                      \
        v1.x -= v2.x;                                                         \
        v1.y -= v2.y;                                                         \
    } while (0);
/**
 * Cone apex at a.
 * points a, b, and c must be ordered COUNTERCLOCKWISE
 */
static bool
incone (Vector2 x, Vector2 a, Vector2 b, Vector2 c)
{
    vsub (b, a);
    vsub (c, a);
    vsub (x, a);
    a.x = 0;
    a.y = 0;
    x.y *= -1;
    b.y *= -1;
    c.y *= -1;

    // logdf ("x = (%f, %f)", x.x, x.y);
    // logdf ("a = (%f, %f)", a.x, a.y);
    // logdf ("b = (%f, %f)", b.x, b.y);
    // logdf ("c = (%f, %f)", c.x, c.y);
    // logdf ("<=0: %f\n", b.y * x.x - b.x * x.y);
    // logdf (">=0: %f\n", c.y * x.x - c.x * x.y);
    return (b.y * x.x - b.x * x.y <= 0) && (c.y * x.x - c.x * x.y >= 0);
}
#undef vsub

static bool
touches (Vector2 p, const struct obj_t *const obj)
{
    float   a, b, dx, dy;
    Vector2 u, v, x;

    switch (obj->typ) {
        case RL_LINE:
        case RL_TEXT:
            return false;
        case RL_CIRC:
            u  = ((struct rl_circ_arg_t *)obj->params)->c;
            a  = ((struct rl_circ_arg_t *)obj->params)->r;
            dx = p.x - u.x;
            dy = p.y - u.y;
            return dx * dx + dy * dy <= a * a;
        case RL_RECT:
            u = ((struct rl_rect_arg_t *)obj->params)->pos;
            v = ((struct rl_rect_arg_t *)obj->params)->siz;
            a = p.x - u.x;
            b = p.y - u.y;
            return a >= 0 && b >= 0 && a <= v.x && b <= v.y;
        case RL_TRI:
            x = ((struct rl_tri_arg_t *)obj->params)->v1;
            u = ((struct rl_tri_arg_t *)obj->params)->v2;
            v = ((struct rl_tri_arg_t *)obj->params)->v3;
            return incone (p, x, u, v) && incone (p, u, v, x);
            return false;
    }
}

/**
 * @param buf should be unmodified
 */
static size_t
truncpos (char *buf, size_t siz, const int fontsiz, const int width)
{
    size_t lo = 0;
    size_t hi = siz;

    if (MeasureText (buf, fontsiz) <= width)
        return siz;

    while (lo < hi) {
        const size_t mid = lo + ((hi - lo + 1) >> 1);

        const char c = buf[mid];
        buf[mid]     = '\0';

        if (MeasureText (buf, fontsiz) <= width)
            lo = mid;
        else
            hi = mid - 1;

        buf[mid] = c;
    }

    return lo;
}

struct draw_tracks_params_t {
    const int     pad;
    const int     txtpad;
    const int     fontsiz;
    const Vector2 rectpos;
    const Vector2 rectsiz;
};

static struct draw_tracks_params_t
init_draw_tracks_params (int pad, int fontsiz)
{
    const struct draw_tracks_params_t params = {
        .pad     = pad,
        .txtpad  = fontsiz >> 1,
        .fontsiz = fontsiz,
        .rectpos = { .x = rectbg.pos.x + pad, .y = rectbg.pos.y + pad },
        .rectsiz = { .x = rectbg.siz.x - (pad << 1), .y = fontsiz + fontsiz },
    };

    return params;
}

static void
draw_tracks (const char *const *tracks, const size_t len,
             const struct draw_tracks_params_t *par)
{
    Vector2 rectpos = par->rectpos;
    int     pth_ret;

    for (size_t i = 0; i < len; ++i) {
        DrawRectangleV (rectpos, par->rectsiz,
                        i == render_get_active_track_id () ? YELLOW : WHITE);
        DrawText (tracks[i], rectpos.x + par->txtpad, rectpos.y + par->txtpad,
                  par->fontsiz, BLACK);
        rectpos.y += par->rectsiz.y + par->pad; // par->pad is spacing
    }
}

void
render (const strvec_t *sv)
{
    InitWindow (0, 0, "com.msun.ncap");
    SetTargetFPS (fps);

    logdf ("Set target FPS to %d", fps);

    const int SCW = GetScreenWidth ();
    const int SCH = GetScreenHeight ();

    char str[64];
    snprintf (str, sizeof str, "hello from raylib in %d x %d", SCW, SCH);

    logdf ("Window dimensions: %d x %d", SCW, SCH);
    logi ("initialization finished, locking and signaling...");

    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&render_ready_mx)) != 0) {
        logwf ("WARN: could not lock render_ready_mx. Error code %d: %s. "
               "Retrying...",
               pth_ret, strerror (pth_ret));
        nanosleep (&retry_ts, NULL);
    }

    render_ready = true;

    // NOTE: change to broadcast when needed
    while ((pth_ret = pthread_cond_signal (&render_ready_cv)) != 0) {
        logwf ("WARN: could not signal render_ready_cv. Error code %d: %s. "
               "Retrying...",
               pth_ret, strerror (pth_ret));
        nanosleep (&retry_ts, NULL);
    }

    pthread_mutex_unlock (&render_ready_mx);

    init_objs (SCW, SCH);

    Vector2 tpos;
    Vector2 ptpos = { 0, 0 };
    int     touched;
    int     ptouched = 0;

    const struct draw_tracks_params_t draw_tracks_par
        = init_draw_tracks_params (10, FONTSIZ);

    char **tracks_trunc = malloc (sv->siz * sizeof (char *));

    for (size_t i = 0; i < sv->siz; ++i) {
        const size_t pos = truncpos (
            sv->ptr[i], strlen (sv->ptr[i]) + 1, draw_tracks_par.fontsiz,
            draw_tracks_par.rectsiz.x - (draw_tracks_par.txtpad << 1));

        tracks_trunc[i] = malloc (pos + 1);
        memcpy (tracks_trunc[i], sv->ptr[i], pos);
        tracks_trunc[i][pos] = '\0';

        logvf ("truncated track `%s' to `%s'", sv->ptr[i], tracks_trunc[i]);
    }

    for (; !WindowShouldClose (); ptouched = touched, ptpos = tpos) {
        touched = GetTouchPointCount ();

        if (!touched && !ptouched) {
            if (fps != FPS_STATIC) {
                SetTargetFPS (fps = FPS_STATIC);
                logif ("set FPS to %d", fps);
            }

            BeginDrawing ();
            {
                ClearBackground (WHITE);

                for (size_t i = 0; i < objs_len; ++i)
                    draw (&objs[i]);

                draw_tracks (tracks_trunc, sv->siz, &draw_tracks_par);
            }
            EndDrawing ();
            continue;
        }

        if (fps != FPS_ACTIVE) {
            SetTargetFPS (fps = FPS_ACTIVE);
            logif ("set FPS to %d", fps);
        }

        // check touch on touch release
        if (touched) {
            tpos = GetTouchPosition (0);
        } else {
            tpos.x = -1;
            tpos.y = -1;

            for (size_t i = 0; i < objs_len; ++i) {
                if (objs[i].dyn == false)
                    continue;

                if (touches (ptpos, &objs[i])) {
                    objs[i].act (&objs[i]);
                    logdf ("act called for object %zu", i);
                }
            }
        }

        // test window close

        if ((pth_ret = pthread_mutex_trylock (&render_wclose_mx)) != 0) {
            logwf ("WARN: could not lock render_wclose_mx. Error "
                   "code %d: %s",
                   pth_ret, strerror (pth_ret));
        } else {
            if (wclose) {
                pthread_mutex_unlock (&render_wclose_mx);
                break;
            }

            pthread_mutex_unlock (&render_wclose_mx);
        }

        BeginDrawing ();
        {
            ClearBackground (WHITE);

            if (touched) {
                if (tpos.x == 0 || tpos.y == 0)
                    continue;

                DrawCircleV (tpos, 30, ORANGE);
                DrawText ("0", tpos.x - 10, tpos.y - 70, FONTSIZ, BLACK);
            }

            for (size_t i = 0; i < objs_len; ++i)
                draw (&objs[i]);

            draw_tracks (tracks_trunc, sv->siz, &draw_tracks_par);
        }
        EndDrawing ();
    }

    logi ("Closing raylib window...");
    CloseWindow ();

    logd ("freeing tracks_trunc...");
    for (size_t i = 0; i < sv->siz; ++i)
        free (tracks_trunc[i]);
    free (tracks_trunc);
}

int
render_close (void)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&render_wclose_mx)) != 0)
        return pth_ret;

    wclose = true;

    pthread_mutex_unlock (&render_wclose_mx);

    // reset play/pause text

    memcpy (((struct rl_text_arg_t *)objs[4].params)->str, " play", 6);

    return 0;
}

bool
render_closing (void)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&render_wclose_mx)) != 0)
        return false;

    if (wclose) {
        pthread_mutex_unlock (&render_wclose_mx);
        return true;
    }

    pthread_mutex_unlock (&render_wclose_mx);
    return false;
}

bool
render_closing_nb (void)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_trylock (&render_wclose_mx)) == 0) {
        if (wclose) {
            pthread_mutex_unlock (&render_wclose_mx);
            return true;
        }

        pthread_mutex_unlock (&render_wclose_mx);
    } else {
        logwf ("WARN: could not acquire render_wclose_mx. Error "
               "code %d: %s. ignoring...",
               pth_ret, strerror (pth_ret));
    }

    return false;
}

int
render_waitready (void)
{
    int pth_ret;

    if ((pth_ret = pthread_mutex_lock (&render_ready_mx)) != 0)
        return pth_ret;

    while (!render_ready)
        pthread_cond_wait (&render_ready_cv, &render_ready_mx);

    pthread_mutex_unlock (&render_ready_mx);

    return 0;
}

int
render_set_active_track_id (int id)
{
    int pth_err;

    if ((pth_err = pthread_mutex_lock (&render_atrid_mx)) != 0)
        return pth_err;

    render_atrid = id;

    pthread_mutex_unlock (&render_atrid_mx);

    return 0;
}

int
render_get_active_track_id (void)
{
    int ret;

    if ((ret = pthread_mutex_lock (&render_atrid_mx)) != 0) {
        logwf ("WARN: could not lock render_atrid_mx. Error code %d: %s", ret,
               strerror (ret));
        return -1;
    }

    ret = render_atrid;
    pthread_mutex_unlock (&render_atrid_mx);
    return ret;
}
