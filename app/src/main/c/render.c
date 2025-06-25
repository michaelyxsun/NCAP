#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "pthread.h"
#include "render.h"
#include "strvec.h"

static const char *FILENAME = "render.c";

pthread_mutex_t render_ready_mx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  render_ready_cv = PTHREAD_COND_INITIALIZER;
bool            render_ready    = false;

pthread_mutex_t render_atrid_mx = PTHREAD_MUTEX_INITIALIZER;
int             render_atrid    = -1;

static const int FPS_ACTIVE = 30;
static const int FPS_STATIC = 5;
static const int FONTSIZ    = 40;
static int       fps        = FPS_STATIC;

static bool wclose = false;

static void
act_wclose (void)
{
    logi ("window close signaled");
    wclose = true;
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
    static char           buf_[OBJ_BUF_SIZ];
    char                 *buf = buf_;
    int                   pos = 0;
    int                   x, y, w, h, pad, add;
    struct rl_text_arg_t *textarg;
    struct rl_rect_arg_t *rectarg;

    // rectangle background

    rectarg = objs[0].params = &rectbg;
    objs[0].typ              = RL_RECT;
    objs[0].dyn              = false;

    w = rectarg->siz.x = SCW * 0.8;
    h = rectarg->siz.y = SCH * 0.5;
    rectarg->pos.x     = (SCW - w) >> 1;
    rectarg->pos.y     = (SCH - h) >> 1;
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

    rectarg->siz.x = w + 20;
    rectarg->siz.y = h + 20;
    rectarg->pos.x = x - 10;
    rectarg->pos.y = y - 10;
    rectarg->color = RED;

    // // prompt text
    //
    // static struct rl_text_arg_t objs3;
    // textarg = objs[3].params = &objs3;
    // objs[3].typ              = RL_TEXT;
    // objs[3].dyn              = false;
    //
    // pos += (add = snprintf (buf, sizeof buf_ - pos,
    //                         "hello from raylib in %d x %d", SCW, SCH)
    //               + 1);
    //
    // textarg->str   = buf;
    // textarg->fsiz  = FONTSIZ;
    // w              = MeasureText (buf, FONTSIZ);
    // textarg->x     = (SCW - w) >> 1;
    // textarg->y     = (SCH - FONTSIZ) >> 1;
    // textarg->color = BLACK;
    //
    // buf += add + 1;

    objs_len = 3;
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

static bool
touches (Vector2 p, const struct obj_t *const obj)
{
    float   a, b, dx, dy;
    Vector2 u, v;

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
        case RL_TRI: // TODO(M-Y-Sun): implement
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

    pthread_mutex_lock (&render_atrid_mx);

    for (size_t i = 0; i < len; ++i) {
        DrawRectangleV (rectpos, par->rectsiz,
                        i == render_atrid ? YELLOW : WHITE);
        DrawText (tracks[i], rectpos.x + par->txtpad, rectpos.y + par->txtpad,
                  par->fontsiz, BLACK);
        rectpos.y += par->rectsiz.y + par->pad; // par->pad is spacing
    }

    pthread_mutex_unlock (&render_atrid_mx);
#undef two_txtpad
#undef two_pad
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

    pthread_mutex_lock (&render_ready_mx);
    render_ready = true;

    // NOTE: change to broadcast when needed
    pthread_cond_signal (&render_ready_cv);
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

                if (touches (ptpos, &objs[i]))
                    objs[i].act ();
            }
        }

        if (wclose)
            break;

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

    logi ("Closing window...");
    CloseWindow ();
    wclose = false;

    logd ("freeing tracks_trunc...");

    for (size_t i = 0; i < sv->siz; ++i)
        free (tracks_trunc[i]);

    free (tracks_trunc);
}
