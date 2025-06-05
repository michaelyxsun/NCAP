#include <jni.h>
#include <raylib.h>

jstring
Java_com_msun_rlcap_MainActivity_stringFromJNI (JNIEnv *env, jobject this)
{
    return (*env)->NewStringUTF (env, "hello from c");
}

int
main (void)
{
    const int SCW = GetScreenWidth ();
    const int SCH = GetScreenHeight ();

    InitWindow (SCW, SCH, "RLCAP");
    SetTargetFPS (30);

    while (!WindowShouldClose ()) {
        BeginDrawing ();
        {
            ClearBackground (WHITE);
            DrawText ("hello from raylib", SCW >> 1, SCW >> 1, 20, BLACK);
        }
        EndDrawing ();
    }

    CloseWindow ();

    return 0;
}
