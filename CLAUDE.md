# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

Mecha bullet hell prototype. C game built with raylib, compiled to WASM via Emscripten, packed into a single HTML file with Histos. Data-oriented design — read docs/PHILOSOPHY.md before making changes.

## Build Commands
You never run the build command. The user will always do this.
```bash
./build2.sh        # WASM build (emcc -> web_pkg/mecha.js, histos -> mecha.html)
./build2.sh n      # debug native build (gcc -> ./mecha)
./build2.sh o      # optimized native build (-O2 -march=native -flto -ffast-math, stripped)
```

No automated test suite. Testing is manual gameplay: `./build2.sh n && ./mecha`.

## Project Structure

```
src/              - all game source code
  main.c          - entry point, window init, game loop
  init.c          - GameState g definition, InitGame
  spawn.c         - enemy data tables, spawn/damage functions
  collision.c     - geometry helpers, collision dispatchers
  update.c        - all update logic (player, enemies, projectiles, deployables)
  draw.c          - all draw logic (solids, world, HUD, select screen, NextFrame)
  game.h          - master header (includes mecha.h, extern g, cross-file prototypes)
  mecha.h         - all types, structs, enums
  default.h       - all tunable gameplay constants (defines)
  rtypes.h        - Rust-style type aliases (u8, u16, u32, u64, etc.)
build2.sh         - compile + pack script (runs from repo root)
config.yaml       - histos packing config
web_pkg/          - web build assets and output
  setup.js        - emscripten Module pre-config
  style.css       - canvas styling + cursor hide
  mecha.js        - emcc output (generated)
  mecha.html      - histos packed output (generated, output to repo root)
docs/             - project documentation
  PHILOSOPHY.md   - design philosophy (must follow)
  GDD.md          - game design document
  NOTES.md        - todo list and notes
  ABILITIES.md    - ability documentation
  PIPELINE.md     - pipeline documentation
  REFACTOR.md     - refactoring notes
  STATE.md        - state documentation
raylib/           - raylib source (git submodule)
lib/              - pre-built raylib libraries (libraylib.web.a)
asset_blob/       - custom asset blob encoder/loader (blobbert.c)
```

Key documentation: `docs/NOTES.md` (todo list), `docs/GDD.md` (game design doc), `docs/PHILOSOPHY.md` (must follow).

## Architecture

- Single global `GameState g` struct defined in `init.c`, extern'd via `game.h` (Emscripten requires `void(*)(void)` callback pattern)
- Multi-file build: each `.c` includes `game.h` which includes `mecha.h`. Cross-file functions are non-static and declared in `game.h`. File-internal functions stay `static`.
- Player weapons/abilities: `Gun`, `Minigun`, `Sword`, `Revolver`, `Sniper`, `Rocket`, `Shotgun`, `Railgun`, `Bfg`, `Grenade`, `Shield`, `Flamethrower`, `GroundSlam`, `Parry`, `Dash`, `Spin`, `Laser` composed as fields in `Player`
- Enemy types: `EnemyDef` data table indexed by `EnemyType` enum, table-driven spawning via `SPAWN_PRIORITY` (both in `spawn.c`)
- Collision dispatchers: `EnemyHitSweep`, `EnemyHitPoint`, `EnemyHitCircle` in `collision.c` — switch on enemy type, one `case` per shape
- Update pipeline in `update.c`: `UpdatePlayer` -> `UpdateEnemies` -> `UpdateProjectiles` -> `UpdateLightningChain` -> `UpdateParticles` -> `UpdateBeams` -> `UpdateDeployables` -> `MoveCamera`
- Draw pipeline in `draw.c`: `DrawWorld` (camera space) -> `DrawHUD` (screen space)
- Player models: all 5 platonic solids — `DrawTetra2D`, `DrawCube2D`, `DrawOcta2D`, `DrawDodeca2D`, `DrawIcosa2D`. **Do not delete any Draw*2D functions.**
- All gameplay constants belong in `default.h`, not as magic numbers in source files
- HUD scales via `ui = screenHeight / 450.0f`
- Player-relative HUD: weapon status (revolver rounds, gun heat, reload/overheat QTE) drawn as `DrawRing` arcs near the player via `GetWorldToScreen2D`. Constants prefixed `HUD_ARC_*` in default.h. Left-side HUD column is for all cooldowns.
- Pools: projectiles[1024], enemies[1024], deployables[1024], particles[1024], beams[8], vfxTimers[72]
- Deployable pool: shared by turret, mine, heal field, fire zone — type-switched via `DeployableType`
- VfxTimer pool: shared by explosion rings and mine webs — type-switched via `VfxTimerType`
- Enemy debuffs: `slowTimer`/`slowFactor` (reduced speed), `rootTimer` (no move, can shoot), `stunTimer` (no move, no shoot)

## Code Style

- Section separators: `// ========================================================================== /`
- Subsection headers: `// name ------------------------------------------------------------------- /`
- PascalCase for functions/types, lowercase for locals
- Use type aliases from `rtypes.h` (u8, u16, u32, u64, i8-i64)
- Match existing comment and formatting patterns

## Working With This Codebase

- Follow docs/PHILOSOPHY.md: solve for the common case, design around data not abstractions, prefer simple explicit code
- When the user asks for implementation help, give specifications and snippets, not full rewrites — unless in agent building mode
- Refactor from ends inward: design output shape first, then input, then the middle transformation
- Don't over-engineer for hypothetical edge cases
- **Read the existing code before reimplementing behavior.** When new code needs to do what existing code already does, read the existing implementation and replicate it faithfully. Don't guess or take shortcuts. Example: CIRC boss weapon attacks were supposed to mirror the player's weapons, but were all implemented as instant spread shots instead of reading how each player weapon actually fires (sequential fan, sustained burst, melee sweep, etc).
- **Trust the user's visual observations.** The user playtests the game — you cannot see it. If they say something isn't working, investigate rather than arguing from code. Example: user reported boss attacks weren't firing, spent multiple rounds being told they were, when actually a god-mode bug was silently consuming all enemy projectiles before rendering.
- **Think through side effects across the pipeline.** When modifying one system, trace the consequences through other systems that interact with it. Example: adding invincibility made `DamagePlayer` early-return, but enemy bullets were still deactivated on player hit (`b->active = false` ran unconditionally after the damage call), so all enemy projectiles touching the player were silently eaten.

### Commit Messages
Use `git commit -m "short sentence"` — always a single `-m` with a one-line string, no heredocs or multi-line. No co-author line — the user is the sole author.
Prefixes:
- Updated:
- Added:
- Fixed:
- Refactored:
- Removed:
- In progress:
etc

## Version Roadmap

- **v(-2)**: Add all weapons and enemies (current milestone — weapons/abilities done, remaining: mini boss TRAP, big boss CIRC)
- **v(-1)**: Room-based progression with weapon selection between rounds, mini boss fight
- **v(0)**: Chassis selection (platonic solid characters with unique identities)

## Controls

WASD: move | Mouse: aim | M1: primary weapon | M2: alt fire | Ctrl: swap weapon | Space: dash (3 charges) | Space+M2: dash slash | P/Esc: pause | 0: exit

Primary weapons (select screen): Machine Gun, Sword, Revolver, Sniper, Rocket. Revolver uses both mouse buttons — M1 precise single shots, M2 fans all remaining rounds rapidly.

Abilities (12 slots, all keybinds shown in pause menu and weapon select):
Q: Shotgun | E: Railgun | Shift: Spin (lifesteal) | 1: Ground Slam (AoE stun) | 2: Parry (deflect window) | 3: Turret (auto-shooter) | 4: Mine (root on trigger) | Z: Heal Field | X: Shield | C: Grenade | V: Fire Zone | F: BFG
