#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>

#include "audio.h"
#include "util.h"

#define MAX_TOUCH_POINTS 4

static const char *FILENAME = "native-lib.c";

static void *
tfn_audio_play (void *errstat)
{
    *(int *)errstat = audio_play ();
    pthread_exit (NULL);
}

int
main (void)
{
    pthread_t audio_tid;
    int       stat;
    logi ("%s: Spawning audio_play thread...", FILENAME);
    pthread_create (&audio_tid, NULL, tfn_audio_play, &stat);

    logi ("%s: Initializing window...", FILENAME);

    InitWindow (0, 0, "RLCAP");
    SetTargetFPS (60);

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

    logi ("%s: Joining thread...", FILENAME);
    pthread_join (audio_tid, NULL);

    logi ("%s: audio_play returned a status code of %d", FILENAME, stat);

    return 0;
}
