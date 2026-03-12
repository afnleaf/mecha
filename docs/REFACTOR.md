v(-2) refactor

looking at the GameState g

There are some visual effects in there that shouldn't be there. Or there might be a better way to do it.

Update -> Draw is the correct approach.

but right now we draw in update in some places. that has to go.

update the game state

read game state to know what pixels to draw

key thing about the game state is that it does not require player input to update.

we should think about using events to trigger draws

events of the same type should be executed in parallel

---

# Refactor Roadmap

## TIER 1: Before CIRC Boss (unblocks current milestone) — DONE

### 1A. Enemy Shooting Dispatch Table — DONE
`EnemyShootFn shoot` pointer added to `EnemyDef`. Four shoot functions (`ShootRect`, `ShootPenta`, `ShootHexa`, `ShootTrap`) registered in `ENEMY_DEFS` table in `spawn.c`. `UpdateEnemies` dispatches via `ENEMY_DEFS[e->type].shoot()`. Adding CIRC just needs `ShootCirc()` + one table entry.

### 1B. Sweep Damage Deduplication — DONE
Shared `static int SweepDamage(origin, sweepEnd, sweepAngle, damage, dmgType, lastHitAngles, hitIndices, maxHits)` in `update.c`. Sword calls it directly; Spin calls it then applies lifesteal on returned hit indices.

### 1C. Explosion VFX Deduplication — DONE
Shared `static void SpawnExplosionVfx(pos, c1, c2, c3, fireColor)` in `update.c`. `RocketExplode` calls with `(RED, ORANGE, YELLOW, ORANGE)`, `GrenadeExplode` with `(GREEN, YELLOW, ORANGE, YELLOW)`. Both keep their own damage/knockback logic.

---

## TIER 2: Quick Wins (5-15 min each) — DONE

### 2A. HUD Cooldown Helpers — DONE
`static void DrawCooldownBar(x, y, w, h, ratio, color, label, labelX, fontSize)` in `draw.c`. Takes a 0→1 fill ratio; draws ready state (WHITE outline, colored label) when ratio >= 1, cooling state (GRAY) otherwise. Replaced 8 identical blocks (rocket, railgun, sniper, grenade, slam, turret, mine, heal). BFG, shield, parry kept as custom (multi-state logic). Dash/shotgun pip bars left as-is (only 2 instances, each unique).

### 2B. `mouse()` → `UpdateMouseAim()` — DONE
Renamed definition and single call site in `update.c`.

### 2C. `selectWeapons[]` + Hardcoded `5` — DONE
Added `#define NUM_PRIMARY_WEAPONS 5` in `default.h`. Replaced 5 hardcoded `5` loop bounds in `update.c` and `draw.c`. Duplicate `selectWeapons[]` arrays kept local (used in separate static functions).

### 2D. Deployable Count Helper — DONE
`static int CountActiveDeployables(DeployableType type)` in `update.c`. Replaced 3 identical loops (turret, mine, heal).

### 2E. Sniper Slow Decoupling from DMG_PIERCE — DONE
Added `bool appliesSlow` to `Projectile` in `mecha.h`. Initialized `false` in `SpawnProjectile` (`spawn.c`). Set `true` only at sniper projectile spawn sites (gameplay + select demo). Collision check changed from `b->dmgType == DMG_PIERCE` to `b->appliesSlow`.

---

## TIER 3: Structural (medium effort, high payoff for v(-1))

### 3A. Split UpdatePlayer — DONE
**Files**: `update.c` only

Extracted 4 static functions: `UpdateMovement(p, moveDir, moveLen, dt)` (~33 lines), `UpdateDash(p, moveDir, moveLen, dt)` (~99 lines), `UpdateWeapon(p, toMouse, dt)` (~487 lines, includes weapon swap + holstered timers + sword damage), `UpdateAbilities(p, toMouse, dt)` (~298 lines). `moveDir`/`moveLen` hoisted to orchestrator, shared by movement and dash. `UpdatePlayer` is now a ~30-line orchestrator.

### 3B. Split DrawHUD — DONE
**Files**: `draw.c` only

Extracted 4 static functions: `DrawCooldownColumn(p, ui)` (~200 lines, all cooldown bars), `DrawWeaponStatus(p, ui)` (~180 lines, revolver arc + gun heat arc), `DrawPauseMenu(sw, sh, ui)` (~76 lines, pause overlay + keybinds), `DrawGameOver(sw, sh, ui)` (~34 lines). `DrawHUD` is now a ~113-line orchestrator (HP bar, score, weapon swap, calls, FPS, crosshair, title, overlay dispatch).

### 3C. Boss Spawn Path — DONE
**Files**: `spawn.c`, `update.c`, `game.h`
**Scope**: ~30 lines | **Risk**: Low

Extracted `InitEnemy()` + `SpawnAtEdge()` static helpers in `spawn.c`. Added `SpawnBoss(EnemyType type)` reusing `ENEMY_DEFS` table. `SpawnEnemy` refactored to use same helpers. 40-line inline boss spawn in `UpdateEnemies` replaced with `SpawnBoss(TRAP)`.

### 3E. Draw Shape Deduplication — DONE
**Files**: `draw.c` only

Extracted 5 shared static helpers: `ProjectVertices` (rotation + 3D→2D projection), `SubdivDrawTri` (barycentric subdivision), `SubdivDrawQuad` (bilinear subdivision, cube only), `DrawShapeShadow` (shadow polygons, handles tri/quad/penta via vpf), `CullSortFaces` (backface cull + painter's sort). All face arrays unified to `int faces[N][5]`. Each `Draw*2D` is now: define vtx+faces → project → hues → cull+sort → subdiv draw → edges. Sphere uses `ProjectVertices` only, keeps its own procedural verts and circle shadow. ~911 lines → ~466 lines.

### 3D. lastHitAngle Memory Reduction — DONE
**Files**: `mecha.h`, `update.c`

Replaced two `float[1024]` arrays (8KB) in `Sword` and `Spin` with `u8 hitBits[128]` bitfields + `float lastResetAngle` (264 bytes, 97% reduction). `SweepDamage` checks/sets bits instead of storing angles. PI-boundary batch reset via `memset` preserves multi-hit behavior for Spin (4PI rotation). Lunge uses the same bitfield directly.

---

## TIER 4: Polish & v(-1) Prep

### 4B. Pool Consolidation — DONE
**Files**: `mecha.h`, `default.h`, `update.c`, `draw.c`

Two merges:

**1. FirePatch → Deployable** — `FirePatch` struct deleted, fire zones now use `DEPLOY_FIRE` in the deployable pool. `SpawnFirePatch` replaced by `SpawnDeployable(DEPLOY_FIRE, pos)`. Update logic is a `case DEPLOY_FIRE:` in `UpdateDeployables`. Draw logic is a `case DEPLOY_FIRE:` in the deployable draw switch.

**2. Explosive + MineWebVfx → VfxTimer** — Both structs deleted. Replaced with `VfxTimer` (pos, timer, duration, type) and `VfxTimerType` enum (`VFX_EXPLOSION`, `VFX_MINE_WEB`). Pool: `VfxState.timers[MAX_VFX_TIMERS]` (72). `SpawnVfxTimer(pos, duration, type)` helper deduplicates all 3 spawn sites. Single tick loop in game loop, single draw loop with type switch.

Pools reduced from 8 → 6: projectiles, enemies, deployables, particles, beams, vfxTimers.

### 4A. VFX Pool Helpers — DONE (absorbed by 4B)
Inline pool-scan loops replaced by `SpawnVfxTimer(pos, duration, type)` helper as part of 4B pool consolidation.

---

## TIER 5: Structural Cleanup

### 5A. Move Enemy Shoot Functions to spawn.c (= old 4C)
**Files**: `update.c`, `spawn.c`, `game.h`
**Scope**: ~136 lines moved | **Risk**: Low

Move `ShootRect` (update.c:1335), `ShootPenta` (update.c:1352), `ShootHexa` (update.c:1388), `ShootTrap` (update.c:1415) from `update.c` to `spawn.c`. Make them `static`. Remove 4 declarations from `game.h`. All enemy definition (data table + shooting behavior) lives in one file. `UpdateEnemies` dispatches via `ENEMY_DEFS[e->type].shoot()` pointer — no cross-file calls needed. Unblocks adding `ShootCirc` for the CIRC boss cleanly.

### 5B. selectWeapons[] Deduplication
**Files**: `update.c`, `draw.c`, `mecha.h` or `default.h`
**Scope**: ~6 lines | **Risk**: None

Identical `selectWeapons[]` arrays declared locally in both `UpdateSelect()` (update.c:135) and `DrawSelect()` (draw.c:1282). Extract to a single `static const WeaponType SELECT_WEAPONS[]` in `mecha.h` (or `default.h` alongside `NUM_PRIMARY_WEAPONS`). Replace both local declarations. The associated `names[]`, `descs[]`, `solidFns[]` arrays stay local — they're draw-only or update-only.

### 5C. Split DrawWorld
**Files**: `draw.c` only
**Scope**: ~632 lines → 5-6 functions | **Risk**: Low

`DrawWorld()` (draw.c:641-1272) is the largest function in the codebase. Extract subsections into static helpers following existing pattern (like DrawHUD split in 3B):

1. `DrawDeployableWorld()` (~76 lines) — turret/mine/heal/fire switch, currently lines 666-742
2. `DrawVfxWorld()` (~36 lines) — explosion rings + mine webs, lines 744-780
3. `DrawLightningWorld()` (~44 lines) — BFG chain arcs, lines 785-829
4. `DrawEnemies()` (~100 lines) — enemy shapes, HP bars, hit flash
5. `DrawPlayerWorld()` (~210 lines) — solid body + weapon barrels + sword/lunge arcs + ability visuals

`DrawWorld()` becomes a ~40-line orchestrator: grid, border, particles, then calls. Same pattern as `UpdatePlayer` (3A) and `DrawHUD` (3B).

### 5D. Inline Color Constants to default.h
**Files**: `draw.c`, `default.h`
**Scope**: ~10 constants | **Risk**: None

Scattered magic colors in draw.c that should be named constants in default.h:
- Revolver HUD arc color `(Color){ 220, 180, 80, 255 }` (draw.c:1675)
- Crosshair color `(Color){ 0, 255, 0, 180 }` (draw.c ~2017)
- Various enemy draw colors used inline rather than from constants
- Any other `(Color){ ... }` literals in draw.c not already defined in default.h

Audit draw.c for all inline `(Color)` literals, name them `*_COLOR` in the appropriate default.h section.

---

## TIER 6: v(-1) Architecture

### 6A. Game Phase State Machine (= old 4D)
**Files**: `mecha.h`, `update.c`, `draw.c`
**Scope**: Design first | **Risk**: Medium

Currently `g.screen` (`SELECT`/`PLAYING`) + `g.phase` (`0`/`1`/`2`) + `g.level` overlap in meaning. For v(-1) room system, unify into single state enum: `PHASE_SELECT → PHASE_ROUND → PHASE_CLEARING → PHASE_ITEM_PICK → PHASE_BOSS → PHASE_END`. Each phase has its own update function and draw function. `g.screen` and `g.phase` collapse into `g.phase`. Design the data shape (what state does each phase need?) before implementing.

### 6B. Damage System Skeleton
**Files**: `mecha.h`, `spawn.c`
**Scope**: Design first | **Risk**: Medium

`DamageEnemy`/`DamagePlayer` pass through `DamageType` and `DamageMethod` but ignore them. The commented-out `Damage` struct in mecha.h shows the intended future event system. Design the damage pipeline:
- What are the input parameters? (base damage, type, method, source)
- What are the transforms? (armor, resist, vulnerability, crit, damage amp debuffs)
- What is the output? (final damage number, side effects like slow/stun)

Don't implement — design the data transformation. The `EnemyDef` table may need `armor`/`resist` fields. This unblocks v(-1) enemy variety (boss phases that change vulnerability) and v(0) chassis differentiation (damage type specialization per solid).

---

## Execution Order

```
Tier 1 (DONE):                         1A ✓, 1B ✓, 1C ✓
Tier 2 (DONE):                         2A ✓, 2B ✓, 2C ✓, 2D ✓, 2E ✓
Tier 3 (DONE):                         3C ✓, 3A ✓, 3B ✓, 3D ✓, 3E ✓
Tier 4 (polish + v(-1) prep):          4B ✓, 4A ✓ (absorbed by 4B)
Tier 5 (structural cleanup):           5A, 5B, 5C, 5D
Tier 6 (v(-1) architecture):           6A, 6B
```
