#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>

#include "render.h"
#include "util.h"

#define MAX_TOUCH_POINTS 4

static const char *FILENAME = "render.c";

void
render (void)
{
    InitWindow (0, 0, "com.msun.ncap");

    const int FPS = 60;
    SetTargetFPS (FPS);

    logd ("%s: Set target FPS to %d", FILENAME, FPS);

    const int SCW = GetScreenWidth ();
    const int SCH = GetScreenHeight ();

    const int FONTSIZ = 40;

    char str[32];
    snprintf (str, sizeof str, "hello from raylib in %d x %d", SCW, SCH);

    logd ("%s: Window dimensions: %d x %d", FILENAME, SCW, SCH);

    const int POSX = (SCW >> 1) - (MeasureText (str, FONTSIZ) >> 1);
    const int POSY = (SCH >> 1) - (FONTSIZ >> 1);

    static Vector2 tpos[MAX_TOUCH_POINTS];

    while (!WindowShouldClose ()) {
        if (IsKeyPressed (KEY_Q))
            break;

        int tcnt = GetTouchPointCount ();

        if (tcnt > MAX_TOUCH_POINTS)
            tcnt = MAX_TOUCH_POINTS;

        for (int i = 0; i < tcnt; ++i)
            tpos[i] = GetTouchPosition (i);

        BeginDrawing ();
        {
            ClearBackground (WHITE);

            for (int i = 0; i < tcnt; ++i) {
                if (tpos[i].x == 0 || tpos[i].y == 0)
                    continue;

                DrawCircleV (tpos[i], 30, ORANGE);
                DrawText (TextFormat ("%d", i), tpos[i].x - 10, tpos[i].y - 70,
                          FONTSIZ, BLACK);
            }

            DrawText (str, POSX, POSY, FONTSIZ, BLACK);
        }
        EndDrawing ();
    }

    logi ("%s: Closing window...", FILENAME);
    CloseWindow ();
}

#undef MAX_TOUCH_POINTS
