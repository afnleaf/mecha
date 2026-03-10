// main.c
#include "game.h"

int main(void) 
{
    //SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W, SCREEN_H, "mecha prototype");
#ifndef PLATFORM_WEB
    // 0 is primary, 1 is secondary, etc
    // hmm it should first be primary, but if primary is h>w then choose secondary
    // how to get monitor screenwidth stats?
    int monitor = 0;
    if (monitor < GetMonitorCount()) {
        SetWindowMonitor(monitor);
        // center on that monitor
        int mx = GetMonitorWidth(monitor);
        int my = GetMonitorHeight(monitor);
        Vector2 pos = GetMonitorPosition(monitor);
        SetWindowPosition(pos.x + (float)(mx - SCREEN_W) / 2,
                          pos.y + (float)(my - SCREEN_H) / 2);
    }
#endif
    // this zero key could be set better?
    SetExitKey(KEY_ZERO);
    InitGame();
#ifdef PLATFORM_WEB
    emscripten_set_main_loop(NextFrame, 0, 1);
#else
    // target should not be 60 but unlimited or max screen refresh rate
    // minimum 60 even on 30fps screens though? or do we want to do the whole
    // internal game logic fps vs draw fps? internal 120? 240? light game...
    //SetTargetFPS(60);
    SetTargetFPS(240);
    // also in firefox on linux the game doesn't go above 60 fps
    while (!WindowShouldClose()) {
        NextFrame();
    }
#endif
    CloseWindow();
    return 0;
}
