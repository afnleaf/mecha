# Mecha — Project State

Consolidated from GDD, PHILOSOPHY, NOTES, and codebase review.
Last updated: 2026-03-12

---

## Source Stats

| File | Lines | Role |
|------|------:|------|
| draw.c | ~2110 | All rendering (solids, world, HUD, select screen) |
| update.c | ~2066 | All update logic (player, enemies, projectiles, deployables) |
| default.h | ~700 | All tunable gameplay constants (defines) |
| mecha.h | ~457 | Types, structs, enums |
| spawn.c | ~304 | Enemy data tables, spawn/damage functions |
| collision.c | ~134 | Geometry helpers, collision dispatchers |
| init.c | ~67 | GameState g definition, InitGame |
| main.c | ~43 | Entry point, window init, game loop |
| game.h | ~43 | Master header (extern g, cross-file prototypes) |
| rtypes.h | 13 | Rust-style type aliases (u8, u16, etc.) |

Total compiled source: ~5960 lines.

---

## Project Structure

```
src/              - all game source code
  main.c          - entry point, window init, game loop
  init.c          - GameState g definition, InitGame
  spawn.c         - enemy data tables, spawn/damage functions
  collision.c     - geometry helpers, collision dispatchers
  update.c        - all update logic
  draw.c          - all draw logic
  game.h          - master header
  mecha.h         - all types, structs, enums
  default.h       - all tunable gameplay constants
  rtypes.h        - type aliases
docs/             - project documentation
  PHILOSOPHY.md   - design philosophy (must follow)
  GDD.md          - game design document
  NOTES.md        - todo list and notes
  ABILITIES.md    - ability documentation
  PIPELINE.md     - pipeline documentation
  REFACTOR.md     - refactoring notes
  STATE.md        - this file
web_pkg/          - web build assets
  setup.js        - emscripten Module pre-config
  style.css       - canvas styling + cursor hide
  mecha.js        - emcc output (generated)
build2.sh         - compile + pack script
config.yaml       - histos packing config
raylib/           - raylib source (git submodule)
lib/              - pre-built raylib libraries
asset_blob/       - custom asset blob encoder/loader
```

---

## Architecture

Multi-file build. Single global `GameState g` struct defined in `init.c`, extern'd via `game.h`. Each `.c` includes `game.h` which includes `mecha.h`. Cross-file functions are non-static and declared in `game.h`. File-internal functions stay `static`.

Emscripten requires `void(*)(void)` callback — `NextFrame()` in draw.c calls `UpdateGame()` then `DrawGame()`.

### Pipeline

```
UpdateGame (update.c)
  UpdateSelect (if SCREEN_SELECT)
  UpdatePlayer(dt)
    Movement, dash, weapon firing, abilities
  UpdateEnemies(dt) — AI, shooting, contact damage, debuffs
  UpdateProjectiles(dt) — movement, collision, explosions, ricochet
  UpdateLightningChain(dt) — BFG chain propagation
  UpdateParticles(dt) — drag, lifetime
  UpdateBeams(dt) — linger decay
  UpdateDeployables(dt) — turret, mine, heal field, fire zone
  MoveCamera(dt)

NextFrame (draw.c)
  UpdateGame()
  DrawGame()
    DrawSelect (if SCREEN_SELECT)
    BeginMode2D -> DrawWorld -> EndMode2D
      DrawDeployables, DrawVfxTimers, DrawProjectiles,
      DrawLightning, DrawEnemies, DrawPlayer
    DrawHUD
```

### Pools

| Pool | Size | Struct | Notes |
|------|-----:|--------|-------|
| projectiles | 1024 | Projectile | Shared player/enemy via `isEnemy` bool |
| enemies | 1024 | Enemy | Type-driven via EnemyDef table |
| deployables | 1024 | Deployable | Turret, mine, heal, fire — type-switched via DeployableType |
| particles | 1024 | Particle | Universal VFX (all drawn as fading circles) |
| beams | 8 | Beam | Hitscan linger trails |
| vfxTimers | 72 | VfxTimer | Explosion rings + mine webs — type-switched via VfxTimerType |
| lightning | 1 | LightningChain | BFG chain arcs (256 max targets/arcs) |

---

## What Works

### Player Weapons & Abilities

**Primary weapons** (mutually exclusive, chosen at select screen):

| System | Input | Delivery | Status |
|--------|-------|----------|--------|
| Machine Gun | M1 hold | Projectile, overheat mechanic | Working |
| Minigun | M2 hold | Projectile, spin-up/down, movement slow | Working |
| Sword | M1 click / M2 lunge | Sweep arc, dash slash | Working |
| Revolver | M1 precise / M2 fan | Projectile, active reload QTE | Working |
| Sniper | M1 hip / M2 ADS | Projectile + slow, super shot (dash timing) | Working |
| Rocket | M1 | Projectile -> AOE, rocket jump | Working |

**Abilities** (12 keybindable slots, always available):

| System | Key | Delivery | Status |
|--------|-----|----------|--------|
| Shotgun | Q | Spread projectiles, ricochet | Working |
| Railgun | E | Hitscan, full pierce | Working |
| Spin | Shift | Sweep arc, lifesteal, deflect | Working |
| Ground Slam | 1 | AOE cone stun, knockback | Working |
| Parry | 2 | Deflect window, stun on success | Working |
| Turret | 3 | Auto-shooter deployable (max 2) | Working |
| Mine | 4 | Root on trigger deployable (max 3) | Working |
| Heal Field | Z | Healing area deployable (max 1) | Working |
| Shield | X | Half-circle shield, HP pool, regen | Working |
| Grenade | C | Arc physics, bounce, fuse -> AOE | Working |
| Fire Zone | V | Flamethrower ground patches, DPS ticks | Working |
| BFG | F | Charge-up ultimate, chain lightning | Working |

**Movement:**

| System | Input | Status |
|--------|-------|--------|
| Dash | Space | 3 charges, recharge cooldown |
| Super Dash | Space timing | Bonus on 2nd dash, spawns decoy |
| Decoy | Auto | Shadow aggro decoy on super dash |
| Weapon swap | Ctrl | Toggle between primary pair |

### Overheat / Reload QTE System

- Gun overheats on sustained fire, locks out shooting
- QTE vent: cursor sweeps arc, hit sweet spot to fast-vent
- Dash during vent = movespeed boost (GUN_OVERHEAT_DASH_BOOST)
- Revolver active reload: cursor sweep, sweet spot = fast reload + damage bonus

### Enemy Types (6 active, 2 stubbed)

| Type | Shape | Behavior | Spawns After | HP |
|------|-------|----------|-------------|---:|
| TRI | Triangle | Chase | Always | 20 |
| RECT | Rectangle | Chase + single shot (OBB collision) | 5 kills | 40 |
| OCTA | Octagon | Very fast chase | 10 kills | 40 |
| PENTA | Pentagon | Slow + 2x5 bullet rows | 15 kills | 100 |
| HEXA | Hexagon | Strafe orbit + fan shot | 25 kills | 61 |
| RHOM | Rhombus | Fast chase | 100 kills | 40 |
| TRAP | Trapezoid | Stubbed (mini boss) | — | — |
| CIRC | Circle | Stubbed (big boss) | — | — |

**Enemy debuffs:** `slowTimer`/`slowFactor` (reduced speed), `rootTimer` (no move, can shoot), `stunTimer` (no move, no shoot).

### Rendering

All 5 platonic solids implemented as player chassis:
- DrawTetra2D (tetrahedron, 4 faces) — SWORD
- DrawCube2D (cube, 6 faces) — REVOLVER
- DrawOcta2D (octahedron, 8 faces) — GUN
- DrawDodeca2D (dodecahedron, 12 faces) — SNIPER
- DrawIcosa2D (icosahedron, 20 faces) — ROCKET

All use shared static helpers in `draw.c`: `ProjectVertices` (rotation + 3D→2D), `SubdivDrawTri` (barycentric subdivision), `SubdivDrawQuad` (bilinear, cube only), `DrawShapeShadow` (shadow polygons, handles tri/quad/penta via vpf param), `CullSortFaces` (backface cull + painter's sort). Face arrays unified to `int faces[N][5]`. DrawSphere2D (weapon select) uses `ProjectVertices` only, keeps its own procedural verts and circle shadow.

### Infrastructure

- **EnemyDef data table** with SPAWN_PRIORITY[] — adding an enemy is: enum + defines + table row + draw case + collision case + shoot function if ranged
- **Collision dispatchers**: EnemyHitSweep, EnemyHitPoint, EnemyHitCircle — switch on enemy type per shape (RECT uses OBB, rest are circles)
- **FireHitscan** with `maxPierces` — railgun (MAX_ENEMIES pierce) uses this
- **SpawnProjectile** unified API — all projectile weapons use it
- **SpawnParticle/SpawnParticles** — universal VFX spawning in spawn.c, drawn in draw.c
- **Weapon select screen** — 5 primaries with platonic solid previews, keybind display
- **All gameplay constants in default.h** — no magic numbers in source files

---

## Open Refactoring

### DONE (Tiers 1-4)

- Enemy shooting dispatch table (1A), sweep damage dedup (1B), explosion VFX dedup (1C)
- HUD cooldown helper (2A), mouse rename (2B), NUM_PRIMARY_WEAPONS (2C), deployable count helper (2D), sniper slow decoupling (2E)
- UpdatePlayer split (3A), DrawHUD split (3B), boss spawn path (3C), draw shape dedup (3E), lastHitAngle bitfield (3D)
- Pool consolidation (4B/4A), enemy shoot fns to spawn.c (5A), selectWeapons dedup (5B), DrawWorld split (5C), inline colors to default.h (5D)

### REMAINING

### LOW — cleanup

**9. Commented-out code.** Old crosshair blocks, old spawn positions, `#if 0` laser constants (preserved intentionally).

**10. `MAX_ENTITIES` define in default.h is empty/unused.**

---

## Version Roadmap

### v(-2) — Current milestone: all weapons and enemies

**Weapons/abilities: DONE.**

**Enemies remaining:**
- [ ] TRAP (trapezoid mini boss) — multiple attack patterns
- [ ] CIRC (circle big boss) — final boss of v(-1) run

**Open design questions:**
- Each chassis signature move?
- Should shift/spin be sword-only, freeing shift for other chassis abilities?

### v(-1) — Room-based progression

- Room with enemies, defeat them
- Pick a new item between rounds
- Harder waves
- Mini boss fight (TRAP)

### v(0) — Chassis selection

- Pick a platonic solid chassis at start
- Each solid has unique identity beyond primary weapon

---

## Design Philosophy (from docs/PHILOSOPHY.md)

- **Data transformation, not code.** Understand the data and you understand the problem.
- **Solve for the common case**, not the generic case.
- **Refactor from ends inward**: output shape first, then input, then the middle.
- **After "make it work", do "make it right"**. No exceptions.
- **When helping**: specifications and snippets, not full rewrites — unless in agent building mode.
- **The user wants to understand the system.** Typing code builds the mental model.

---

## Build System

```bash
./build2.sh        # WASM (emcc -> web_pkg/mecha.js, histos -> mecha.html)
./build2.sh n      # debug native (gcc -> ./mecha)
./build2.sh o      # optimized native (-O2 -march=native -flto -ffast-math, stripped)
```

WASM binary is smaller because emcc -Os strips unused raylib functions aggressively. Native links the full libraylib.a.

---

## Open Research (from docs/NOTES.md)

- [ ] Damage system math: types, armor/resist, scaling
- [ ] Asset blob format design (blobbert.c exists)
- [ ] GLSL shaders for WebGL
- [ ] Deterministic physics for multiplayer future
- [ ] Procedurally generate textures
- [ ] Summons (NPC drones who take aggro)
