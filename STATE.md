# Mecha — Project State

Consolidated from GDD, PHILOSOPHY, NOTES, CLAUDE_THOUGHTS, and claude_review.
Last updated: 2026-02-27

---

## Source Stats

| File | Lines | Role |
|------|------:|------|
| mecha.c | 3655 | All game logic |
| mecha.h | 402 | Types, structs, forward declarations |
| default.h | 525 | All tunable constants (defines) |
| rtypes.h | — | Rust-style type aliases (u8, u16, etc.) |

Total compiled source: ~4582 lines.

---

## Architecture

Single `static GameState g` struct, memset to zero on init. Emscripten requires `void(*)(void)` callback — `NextFrame()` calls `UpdateGame()` then `DrawGame()`.

### Pipeline

```
UpdateGame
  WindowResize
  UpdateSelect (if SCREEN_SELECT)
  UpdatePlayer(dt)
    mouse() — aim + angle side-effect
    Movement, dash, weapon firing, sword sweep, spin sweep
  UpdateEnemies(dt) — spawn, AI, shooting, contact damage
  UpdateProjectiles(dt) — movement, collision, explosions, ricochet
  UpdateLightningChain(dt) — BFG chain propagation
  UpdateParticles(dt) — drag, lifetime
  UpdateBeams(dt) — linger decay
  Explosive decay (inline, 5 lines)
  MoveCamera(dt)

DrawGame
  DrawSelect (if SCREEN_SELECT)
  BeginMode2D -> DrawWorld -> EndMode2D
  DrawHUD
```

### Pools

| Pool | Size | Struct | Notes |
|------|-----:|--------|-------|
| projectiles | 1024 | Projectile | Shared player/enemy via `isEnemy` bool |
| enemies | 1024 | Enemy | Type-driven via EnemyDef table |
| particles | 1024 | Particle | Universal VFX |
| explosives | 8 | Explosive | Visual ring only |
| beams | 8 | Beam | Hitscan linger trails |
| lightning | 1 | LightningChain | BFG chain arcs (256 max targets/arcs) |

### Memory

~177KB total GameState. Fits L2 cache. Hot loop (UpdateProjectiles * enemies) touches ~132KB. The `lastHitAngle[MAX_ENEMIES]` arrays (8KB inside Player) are the only outlier — functional but wasteful.

---

## What Works

### Player Weapons (14 systems)

| System | Input | Delivery | Type | Status |
|--------|-------|----------|------|--------|
| Machine Gun | M1 hold | Projectile | Primary | Working |
| Sword | M1 click | Sweep arc | Primary | Working |
| Revolver | M1 click / M2 fan | Projectile | Primary | Working, active reload |
| Sniper | M1 (as primary) or X | Projectile + slow | Primary or Secondary | Working |
| Minigun | M1 hold | Projectile (spin-up) | Primary | Working |
| Shotgun | E | Projectile (spread, ricochet) | Secondary | Working |
| Rocket | Q | Projectile -> AOE | Secondary | Working |
| Railgun | Z | Hitscan (full pierce) | Secondary | Working |
| Grenade | C | Projectile (arc, bounce, fuse) | Secondary | Working |
| BFG10k | 4 | Projectile -> Lightning chain | Ultimate (charge) | Working |
| Dash | Space | Movement + iframes | Ability | Working (3 charges) |
| Super Dash | Space timing | Bonus on 2nd dash | Ability | Working |
| Decoy | Auto on super dash | Shadow aggro decoy | Ability | Working |
| Spin | Shift | Sweep arc + lifesteal + deflect | Ability | Working |

**Three-tier weapon structure** (not explicitly codified but de facto):
1. **Primary** (mutually exclusive, select screen): Gun, Sword, Revolver, Sniper, Minigun
2. **Secondary** (always available): Shotgun, Rocket, Railgun, Grenade, BFG
3. **Abilities** (always available): Dash, Spin

### Enemy Types (6 active, 2 stubbed)

| Type | Shape | Behavior | Spawns After | HP |
|------|-------|----------|-------------|---:|
| TRI | Triangle | Chase | Always | 20 |
| RECT | Rectangle | Chase + single shot | 5 kills | 40 |
| OCTA | Octagon | Very fast chase | 10 kills | 40 |
| PENTA | Pentagon | Slow + 2x5 bullet rows | 15 kills | 100 |
| HEXA | Hexagon | Strafe orbit + fan shot | 25 kills | 61 |
| RHOM | Rhombus | Fast chase | 100 kills | 40 |
| TRAP | Trapezoid | Stubbed (mini boss) | — | — |
| CIRC | Circle | Stubbed (big boss) | — | — |

### Rendering

All 5 platonic solids implemented as player chassis:
- DrawTetra2D (tetrahedron, 4 faces) — SWORD
- DrawCube2D (cube, 6 faces) — REVOLVER
- DrawOcta2D (octahedron, 8 faces) — GUN
- DrawDodeca2D (dodecahedron, 12 faces) — SNIPER
- DrawIcosa2D (icosahedron, 20 faces) — MINIGUN

All use: 3D vertices -> rotate -> project to 2D -> backface cull -> painter's sort -> subdivide into N^2 subtriangles -> HSV gradient. Player shadow system (lagged 2D projection) works.

### Infrastructure

- **EnemyDef data table** with SPAWN_PRIORITY[] — adding an enemy is: enum + defines + table row + draw case + collision case + shoot function if ranged
- **Collision dispatchers**: EnemyHitSweep, EnemyHitPoint, EnemyHitCircle — switch on enemy type per shape (RECT uses OBB, rest are circles)
- **FireHitscan** with `maxPierces` — laser (1 pierce) and railgun (MAX_ENEMIES pierce) share the same code
- **SpawnProjectile** unified API — all projectile weapons use it
- **Weapon select screen** — 5 primaries with platonic solid previews, keybind display

---

## What's Resolved (since claude_review.md was written)

That file says 2229 lines and 6 weapons. Now 3655 lines and 14 weapon systems. Major items completed:

1. Bullet -> Projectile migration (was HIGH #1) — **done**, SpawnProjectile is the API
2. Magic numbers to default.h — **done**, 525 lines of defines
3. MAX_EXPLOSIVES moved to mecha.h (was MEDIUM #7) — **done** (line 226 of mecha.h)
4. Revolver with active reload timing and dash bonus rounds
5. Minigun with spin-up and movement slow
6. Sniper with slow debuff
7. Grenade launcher with arc physics, bouncing, delayed fuse
8. BFG10k with charge-up ultimate and lightning chain propagation
9. All 5 platonic solid renderers (was planned, now done)
10. Weapon select screen with chassis previews
11. Dash reworked: 3 charges, super dash timing window, decoy
12. Spin deflects enemy projectiles
13. Shotgun ricochet to nearest enemy
14. Shadow system for 3D player models
15. Sniper as primary weapon option

---

## Open Refactoring

### HIGH — should do before adding more systems

**1. Enemy shooting dispatch (function pointers).** Three inline if-blocks in UpdateEnemies (RECT line 1290, PENTA line 1308, HEXA line 1345) need to become a dispatch table before TRAP or CIRC boss AI gets added. Pattern:
```c
typedef void (*EnemyShootFn)(Enemy*, Vector2 toPlayer, float dist, float dt);
static const EnemyShootFn ENEMY_SHOOT[] = {
    [TRI] = NULL, [RECT] = ShootRect, [PENTA] = ShootPenta,
    [HEXA] = ShootHexa, [OCTA] = NULL, ...
};
// call: if (ENEMY_SHOOT[e->type]) ENEMY_SHOOT[e->type](e, toPlayer, dist, dt);
```

**2. Sweep damage deduplication.** Sword (lines 1046-1072) and Spin (lines 1108-1155) are structurally identical sweep loops. Extract:
```c
static void SweepDamage(Vector2 origin, float sweepAngle, float radius,
    int damage, DamageType dmgType, float *lastHitAngles);
```
Spin has extra behavior (lifesteal, knockback, deflect) — those stay in the caller.

**3. HUD cooldown helpers.** Now 9 separate cooldown/pip displays (dash, spin, shotgun, rocket, railgun, sniper, grenade, BFG, revolver) with massive copy-paste. Two helpers would collapse this:
```c
static void DrawCooldownBar(int x, int y, int w, int h, float ratio, Color color, const char *label, int fontSize);
static void DrawChargePips(int x, int y, int pipW, int pipH, int gap, int current, int max, float rechargeRatio, Color color, const char *label, int fontSize);
```

### MEDIUM — code quality

**4. `mouse()` side-effect.** Lowercase name breaks PascalCase convention. More importantly, it mutates `p->angle` while claiming to return `toMouse`. Should rename to `UpdateAim()` or split the mutation.

**5. Explosive update inline.** Lines 1843-1848 in UpdateGame are the only entity update not in their own function. Should be `UpdateExplosives(dt)`.

**6. Sniper slow is type-coupled.** Line 1681: `if (b->dmgType == DMG_PIERCE)` triggers sniper slow. Any future DMG_PIERCE weapon will also apply slow. Should check weapon identity, not damage type. Fix: add `bool appliesSlow` to Projectile, or check `dmgType == DMG_PIERCE && !b->isEnemy`.

**7. Weapon select hardcodes `5`.** Lines 589, 591, 676 use magic number `5` for weapon count. Should be `NUM_PRIMARY_WEAPONS` derived from an array or enum.

**8. `lastHitAngle[MAX_ENEMIES]` is 8KB.** Two `float[1024]` arrays inside Player for "don't double-hit" logic. A bitfield (128 bytes each) or frame-counter would work. Not a perf issue — it's a code smell.

**9. `selectWeapons[]` is declared twice.** Once in UpdateSelect (line 593) and again in DrawSelect (line 656). Should be file-scope `static const`.

### LOW — cleanup

**10. Commented-out code.** Old crosshair blocks, old spawn positions, dead comments, `#if 0` laser constants (preserved intentionally — fine to keep).

**11. Rocket struct has self-questioning comments.** "this isn't getting used?" / "why does Rocket have a cooldown" — resolve or remove.

**12. Spin lifesteal truncation.** `25 * 0.1 = 2.5 -> (int)2` = 8% effective, not 10%. Minor but misleading.

**13. Enemy-enemy separation.** Enemies stack when chasing. O(n^2) pairwise nudge would fix. Game feel issue.

**14. `MAX_ENTITIES` define on line 16 of default.h is empty/unused.**

---

## Version Roadmap

### v(-2) — Current milestone: all weapons and enemies

**Weapons done:** Machine Gun, Sword, Revolver, Sniper, Minigun, Shotgun, Rocket, Railgun, Grenade, BFG, Dash, Super Dash, Decoy, Spin.

**Weapons remaining (from NOTES):**
- [ ] Laser — on backburner, needs a redefined identity that feels distinct from railgun/hitscan. Constants exist in `#if 0` block.
- [ ] Overheat mechanic for continuous-fire guns
- [ ] Gun modifier button (ctrl+M1/M2) — design decision pending
- [ ] Parry ability (deflect projectiles in radius, parry melee contact) — spin already deflects bullets, so parry needs a distinct identity
- [ ] Summons (NPC allies that take aggro)
- [ ] Morph between two platonic solid forms (press 1 to switch primary/secondary)

**Enemies remaining:**
- [ ] TRAP (trapezoid mini boss) — multiple attack patterns
- [ ] CIRC (circle big boss) — final boss of v(-1) run

**Open design questions:**
- What are the final 5 primary weapons? Current: Sword, Revolver, Gun, Sniper, Minigun
- Is M2 always weapon-specific (fan for revolver, dash slash for sword) or universal?
- Each chassis signature move? (NOTES suggests each primary has a quirk)
- Should shift/spin be sword-only, freeing shift for other chassis abilities?

### v(-1) — Room-based progression

- Room with first enemies, defeat them
- Pause, pick an extra weapon from options
- Harder wave, more enemies
- Pick another weapon
- Mini boss fight (TRAP)
- Game end

### v(0) — Chassis selection

- Pick a platonic solid chassis at start (already have the select screen for this)
- Each solid has a unique identity beyond just the primary weapon
- Morph between two forms?

---

## Design Philosophy (from PHILOSOPHY.md)

- **Data transformation, not code.** Understand the data and you understand the problem.
- **Solve for the common case**, not the generic case.
- **Refactor from ends inward**: output shape first, then input, then the middle.
- **After "make it work", do "make it right"**. No exceptions.
- **When helping**: specifications and snippets, not full rewrites — unless in agent building mode.
- **LLMs** are for exploring conceptual space and showing syntax, not reliable programmers.
- **The user wants to understand the system.** Typing code builds the mental model.

---

## Design Vision (from GDD)

The game is a **single-player moba-like bullet hell** with mecha customization. You pilot a customizable mecha with a primary identity (chassis/weapon), secondary weapons, and abilities.

**Core feel:** Deterministic shop-style progression (not random roguelike drops). Your mech should feel like yours. Slight randomness for bonus loot/cosmetics only.

**Narrative layer:** The world is a simulation run by an ASI. Diegetic, never stated explicitly. Meta-narrative, fourth wall. True ending from beating the game with all classes.

**Genre identity per chassis:**
- Gunner = Shoot 'em up feel
- Sword = Rhythm / parry / dodge / hit feel
- Mastermind = Strategy / tactics feel

**Music:** Enemies and music go together. Rooms as songs. Addition and removal of elements like techno composition. Programmatic music generation tied to gameplay.

---

## Build System

```bash
./build.sh        # WASM (emcc + histos -> mecha.html, ~300KB)
./build.sh n      # debug native (gcc -> ./game, ~1.1MB)
./build.sh o      # optimized native (-O2 -march=native -flto -ffast-math, stripped)
```

WASM binary is smaller because emcc -Os strips unused raylib functions aggressively. Native links the full libraylib.a.

**Targets:** Linux, macOS, Windows (via Docker/0QUAY).

---

## Open Research (from NOTES)

- [ ] Damage system math: types (explosive, ballistic, hitscan, melee, ability, pure), armor/resist, scaling
- [ ] Asset blob format design (blobbert.c exists, header+ToC+payload structure planned)
- [ ] GLSL shaders for WebGL
- [ ] Deterministic physics for multiplayer future (FPS target)
- [ ] Camera2D target/offset interaction, smooth follow
- [ ] SetTargetFPS: internal logic FPS vs draw FPS
- [ ] Row major vs column major cache implications
- [ ] Custom render engine (ray tracing, voxels, webgpu) — via rlgl
- [ ] Touch/gesture support for mobile
