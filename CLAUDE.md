# Mecha Bullet Hell Prototype

Single-file C game built with raylib, compiled to WASM via emscripten, packed into a single HTML file with Histos. Data-oriented design — read PHILOSOPHY.md.

## Project Structure
```
game.c            - all game logic (single file, ~2200 lines)
default.h         - all tunable gameplay values (defines)
rtypes.h          - Rust-style type aliases (u8, u16, u32, u64, etc.)
claude_review.md  - persistent code review state (read before modifying game.c)
NOTES.md          - todo list and design thinking space
GDD.md            - game design document
PHILOSOPHY.md     - software philosophy (must follow)
build.sh          - compile + pack
config.yaml       - histos packing config
setup.js          - emscripten Module pre-config
style.css         - canvas styling + cursor hide
index.html        - output (single self-contained HTML)
raylib/           - raylib source (git clone, built for PLATFORM_WEB)
```

## Build
```bash
./build.sh        # WASM build (emcc + histos -> index.html)
./build.sh n      # debug native build
./build.sh o      # optimized native build (-O2 -march=native -flto -ffast-math, stripped)
```

## Architecture
- Single global `GameState g` struct (emscripten requires void(*)(void) callback)
- Player weapons: `Gun`, `Sword`, `Dash`, `Spin`, `Shotgun`, `Rocket` composed into `Player`
- Enemy types: `EnemyDef` data table indexed by `EnemyType` enum, table-driven spawning
- Collision: `EnemyHitSweep`, `EnemyHitPoint`, `EnemyHitCircle` dispatchers (switch on type)
- Update: `UpdatePlayer` -> `UpdateEnemies` -> `UpdateBullets` -> `UpdateParticles` -> `MoveCamera`
- Draw: `DrawWorld` (camera space) -> `DrawHUD` (screen space)
- Player models: `DrawShape2D` (tetrahedron), `DrawCube2D` (cube) — plan is all 5 platonic solids as selectable characters. Do not delete any Draw*2D functions.
- All gameplay constants in `default.h`. HUD scales via `ui = screenHeight / 450.0f`

## Controls
WASD: move | Mouse: aim | M1: gun | M2: sword | E: shotgun | Q: rocket | Space: dash (2 charges) | Space+M2: dash slash | Shift: spin (lifesteal) | P/Esc: pause | R: restart

## Code Review
**Read `claude_review.md` before modifying game.c. Update it after significant changes.** It tracks resolved items, current stats, refactoring priorities, and architectural assessment.
