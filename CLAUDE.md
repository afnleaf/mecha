# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

Mecha bullet hell prototype. C game built with raylib, compiled to WASM via Emscripten, packed into a single HTML file with Histos. Data-oriented design — read PHILOSOPHY.md before making changes.

## Build Commands
You never run the build command. The user will always do this.
```bash
./build.sh        # WASM build (emcc + histos -> mecha.html)
./build.sh n      # debug native build (gcc -> ./game)
./build.sh o      # optimized native build (-O2 -march=native -flto -ffast-math, stripped)
```

No automated test suite. Testing is manual gameplay: `./build.sh n && ./game`.

## Project Structure

```
mecha.c           - all game logic (monolithic)
mecha.h           - all types, structs, and declarations
default.h         - all tunable gameplay constants (defines)
rtypes.h          - Rust-style type aliases (u8, u16, u32, u64, etc.)
build.sh          - compile + pack script
config.yaml       - histos packing config
setup.js          - emscripten Module pre-config
style.css         - canvas styling + cursor hide
raylib/           - raylib source (git submodule)
lib/              - pre-built raylib libraries (libraylib.web.a)
asset_blob/       - custom asset blob encoder/loader (blobbert.c)
```

Key documentation: `claude_review.md` (persistent code review state), `NOTES.md` (todo list), `GDD.md` (game design doc), `PHILOSOPHY.md` (must follow).

## Architecture

- Single global `GameState g` struct (Emscripten requires `void(*)(void)` callback pattern)
- Player weapons: `Gun`, `Sword`, `Revolver`, `Dash`, `Spin`, `Shotgun`, `Rocket` composed as fields in `Player`
- Enemy types: `EnemyDef` data table indexed by `EnemyType` enum, table-driven spawning via `SPAWN_PRIORITY`
- Collision dispatchers: `EnemyHitSweep`, `EnemyHitPoint`, `EnemyHitCircle` — switch on enemy type, one `case` per shape
- Update pipeline: `UpdatePlayer` -> `UpdateEnemies` -> `UpdateProjectiles` -> `UpdateLightningChain` -> `UpdateParticles` -> `UpdateBeams` -> `UpdateDeployables` -> `MoveCamera`
- Draw pipeline: `DrawWorld` (camera space) -> `DrawHUD` (screen space)
- Player models: all 5 platonic solids — `DrawTetra2D`, `DrawCube2D`, `DrawOcta2D`, `DrawDodeca2D`, `DrawIcosa2D`. **Do not delete any Draw*2D functions.**
- All gameplay constants belong in `default.h`, not as magic numbers in mecha.c
- HUD scales via `ui = screenHeight / 450.0f`
- Player-relative HUD: weapon status (revolver rounds, gun heat, reload/overheat QTE) drawn as `DrawRing` arcs near the player via `GetWorldToScreen2D`. Constants prefixed `HUD_ARC_*` in default.h. Left-side HUD column is for all cooldowns.
- Pools: projectiles[1024], enemies[1024], particles[1024], explosives[8], beams[8], deployables[8]
- Deployable pool: shared by turret, mine, heal field, fire zone — type-switched via `DeployableType`
- Enemy debuffs: `slowTimer`/`slowFactor` (reduced speed), `rootTimer` (no move, can shoot), `stunTimer` (no move, no shoot)

## Code Style

- Section separators: `// ========================================================================== /`
- Subsection headers: `// name ------------------------------------------------------------------- /`
- PascalCase for functions/types, lowercase for locals
- Use type aliases from `rtypes.h` (u8, u16, u32, u64, i8-i64)
- Match existing comment and formatting patterns

## Working With This Codebase

- **Read `claude_review.md` before modifying mecha.c or mecha.h. Update it after significant changes.** It tracks resolved items, current stats, refactoring priorities, and architectural assessment.
- Follow PHILOSOPHY.md: solve for the common case, design around data not abstractions, prefer simple explicit code
- When the user asks for implementation help, give specifications and snippets, not full rewrites — unless in agent building mode
- Refactor from ends inward: design output shape first, then input, then the middle transformation
- Don't over-engineer for hypothetical edge cases

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

- **v(-2)**: Add all weapons and enemies (current milestone — requires projectile/shooting/sweep refactors first)
- **v(-1)**: Room-based progression with weapon selection between rounds, mini boss fight
- **v(0)**: Chassis selection (platonic solid characters with unique identities)

## Controls

WASD: move | Mouse: aim | M1: primary weapon | M2: alt fire | Ctrl: swap weapon | Space: dash (3 charges) | Space+M2: dash slash | P/Esc: pause | 0: exit

Primary weapons (select screen): Machine Gun, Sword, Revolver, Sniper, Rocket. Revolver uses both mouse buttons — M1 precise single shots, M2 fans all remaining rounds rapidly.

Abilities (12 slots, all keybinds shown in pause menu and weapon select):
Q: Shotgun | E: Railgun | Shift: Spin (lifesteal) | 1: Ground Slam (AoE stun) | 2: Parry (deflect window) | 3: Turret (auto-shooter) | 4: Mine (root on trigger) | Z: Heal Field | X: Shield | C: Grenade | V: Fire Zone | F: BFG
