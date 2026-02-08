# Mecha Bullet Hell Prototype

## What This Is
A playable mecha prototype: player square with machine gun, sword, dash, and spin attack against chaser triangle enemies on a bounded map. Built with raylib, compiled to WASM via emscripten, packed into a single HTML file with Histos.

## Project Structure
```
GDD.md          - the game design document, still barebones
PHILOSOPHY.     - the software philosophy to follow
game.c          - all game logic (single file)
default.h       - all tunable gameplay values (defines)
config.yaml     - histos packing config
setup.js        - emscripten Module pre-config (canvas binding)
style.css       - canvas styling + cursor hide
build.sh        - compile + pack in one command
index.html      - output (single self-contained HTML)
raylib/         - raylib source (git clone, built for PLATFORM_WEB)
```

## Build
```bash
./build.sh
```
This runs emcc then histos. Output: `index.html`.

The full compile command (inside build.sh):
```bash
emcc game.c -o game.js -Os -I ./raylib/src -L ./raylib/src -l:libraylib.web.a -s USE_GLFW=3 -s SINGLE_FILE=1 -DPLATFORM_WEB
histos config.yaml
```

## Native Build
game.c supports native compilation via `#ifdef PLATFORM_WEB` guards. The emscripten-specific code (EM_ASM_INT, emscripten_set_main_loop) is guarded; the else branch uses a standard raylib game loop.

## Architecture
- Single global `GameState g` struct — required because emscripten_set_main_loop takes void(*)(void) with no userdata
- Player abilities are separate structs: `Gun`, `Sword`, `Dash`, `Spin` — composed into `Player`
- All gameplay constants live in `default.h`
- HUD auto-scales based on screen height (`ui = screenHeight / 450.0f`)
- Camera smooth-follows player with Vector2Lerp
- OS cursor hidden via CSS, replaced with in-game crosshair
- Histos runtime DISABLED — emscripten SINGLE_FILE embeds WASM as base64 in JS
- No canvas.html needed — emscripten spawns its own canvas

## Controls
- WASD: move
- Mouse: aim
- M1 (left click): machine gun
- M2 (right click): sword swing (arc toward cursor)
- Space: dash (movement dir or aim dir)
- Space + M2: dash slash (wider arc, more damage)
- Shift: spin attack (360 AoE, cooldown)
- P / Esc: pause
- R: restart (on game over)

## Prerequisites
```bash
cd /home/ironhands/Code/raylib_test
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_WEB
```
