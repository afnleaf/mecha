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

### 3A. Split UpdatePlayer (~983 lines)
**Files**: `update.c` only
**Scope**: ~100 lines restructured | **Risk**: Medium

Extract into static functions:
- `UpdateMovement()` — WASD, knockback (~70 lines)
- `UpdateDash()` — charges, super dash, decoy (~120 lines)
- `UpdatePrimaryWeapon()` — the weapon switch (~400 lines)
- `UpdateAbilities()` — all 12 abilities (~300 lines)

`UpdatePlayer` becomes a ~30-line orchestrator. Do after 1B/1C (reduces code before splitting).

### 3B. Split DrawHUD (~684 lines)
**Files**: `draw.c` only
**Scope**: ~80 lines restructured | **Risk**: Low

Extract: `DrawCooldownColumn()`, `DrawWeaponStatus()`, `DrawPauseMenu()`, `DrawGameOver()`. Unblocked by 2A.

### 3C. Boss Spawn Path
**Files**: `spawn.c`, `update.c`, `game.h`
**Scope**: ~30 lines | **Risk**: Low

TRAP boss spawn in `UpdateEnemies` duplicates most of `SpawnEnemy` with inline field init. Extract `SpawnBoss(EnemyType type)` in `spawn.c` reusing `ENEMY_DEFS` table. Prevents another copy-paste for CIRC.

### 3E. Draw Shape Deduplication (~890 lines → ~300)
**Files**: `draw.c` only
**Scope**: ~590 lines reduced | **Risk**: Low

6 `Draw*2D` functions (Tetra, Cube, Octa, Icosa, Dodeca, Sphere) repeat identical logic for shadow pass, vertex projection, backface cull, painter's sort, subdivided triangle rendering, and edge drawing. Only differences per shape: vertex/face data, vertex count, face count, verts-per-face (3/4/5), hue step, saturation.

Extract shared helpers:
- `ProjectVertices(vtx, n, rotY, rotX, pos, pj, pz)` — rotation + 3D→2D projection
- `DrawShapeShadow(vtx, n, faces, nFaces, vpf, rotY, shadowPos, shadowAlpha)` — shadow pass
- `SubdivDrawTri(p0, p1, p2, h0, h1, h2, N, sat, alpha)` — barycentric subdivision + HSV draw (shared by tetra, octa, icosa, dodeca fan)
- `SubdivDrawQuad(p0-p3, h0-h3, N, sat, alpha)` — bilinear subdivision (cube only)

Each `Draw*2D` becomes: define static vtx+faces → call helpers → draw edges. Sphere stays slightly special (procedural verts, circle shadow). No functional change.

### 3D. lastHitAngle Memory Reduction
**Files**: `mecha.h`, `update.c`
**Scope**: ~30 lines | **Risk**: Medium

Two `float[1024]` arrays (8KB) in Player for sweep hit tracking. Replace with per-enemy `u16 lastHitFrame` + global `u16 sweepFrame` counter, or use `u8 hitBits[128]` bitfield (128 bytes). Do after 1B (sweep dedup means changing logic in one place).

---

## TIER 4: Long-term (v(-1) and beyond)

### 4A. Update/Draw Separation
See notes at top of this file. Revisit when adding room transitions or if determinism matters.

### 4B. Code Cleanup
Remove unused `MAX_ENTITIES` define. Remove commented spawn positions in `spawn.c`. Keep `#if 0` laser block (intentional).

---

## Execution Order

```
Tier 1 (DONE):                         1A ✓, 1B ✓, 1C ✓
Tier 2 (DONE):                         2A ✓, 2B ✓, 2C ✓, 2D ✓, 2E ✓
Tier 3 (structural, has deps):         3C, 3A (1B+1C done), 3B (after 2A), 3D (1B done), 3E (no deps)
Tier 4 (future):                        4B, 4A
```

