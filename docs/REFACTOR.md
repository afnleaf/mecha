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

## TIER 4: Long-term (v(-1) and beyond)

### 4A. Update/Draw Separation
See notes at top of this file. Revisit when adding room transitions or if determinism matters.

### 4B. Code Cleanup
Remove unused `MAX_ENTITIES` define. Remove commented spawn positions in `spawn.c`. Keep `#if 0` laser block (intentional).

---

## 

make weapon select menu accept enter and m1 for picking a chassis

## Execution Order

```
Tier 1 (DONE):                         1A ✓, 1B ✓, 1C ✓
Tier 2 (DONE):                         2A ✓, 2B ✓, 2C ✓, 2D ✓, 2E ✓
Tier 3 (structural, has deps):         3C ✓, 3A ✓, 3B ✓, 3D ✓, 3E ✓
Tier 4 (future):                        4B, 4A
```

