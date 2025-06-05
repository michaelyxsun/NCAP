#include <raylib.h>
#include <stdbool.h>

int
main (void)
{

    InitWindow (0, 0, "RLCAP");
    SetTargetFPS (30);

    const int SCW = GetScreenWidth ();
    const int SCH = GetScreenHeight ();

    const int FONTSIZ = 20;

    const char *str
        = TextFormat ("hello from raylib\ndimens: %d x %d", SCW, SCH);

    const int POSX = (SCW >> 1) - MeasureText (str, FONTSIZ);
    const int POSY = (SCH >> 1) - FONTSIZ;

    while (!WindowShouldClose ()) {
        if (IsKeyPressed (KEY_Q))
            break;

        BeginDrawing ();
        {
            ClearBackground (WHITE);
            DrawText (str, POSX, POSY, 20, BLACK);
        }
        EndDrawing ();
    }

    CloseWindow ();

    return 0;
}
