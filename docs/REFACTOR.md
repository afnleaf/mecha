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

## TIER 1: Before CIRC Boss (unblocks current milestone)

### 1A. Enemy Shooting Dispatch Table
**Files**: `mecha.h`, `spawn.c`, `update.c`
**Scope**: ~60 lines changed | **Risk**: Low

Inline if-blocks in `UpdateEnemies` for RECT, PENTA, HEXA, TRAP shooting patterns → extract 4 static functions, add `EnemyShootFn shoot` pointer to `EnemyDef`, wire in table. Loop body becomes:
```c
if (def->shoot && !stunned) def->shoot(e, toShoot, distToShoot, dt);
```
Adding CIRC boss then just means writing one more shoot function and one table entry.

### 1B. Sweep Damage Deduplication
**Files**: `update.c` only
**Scope**: ~40 lines reduced | **Risk**: Low

Sword sweep and Spin sweep are structurally identical loops. Extract `static int SweepDamage(origin, sweepAngle, radius, damage, dmgType, lastHitAngles, hitIndices, maxHits)`. Sword calls it directly; Spin calls it then does lifesteal on returned hits.

### 1C. Explosion VFX Deduplication
**Files**: `update.c` (or `spawn.c`)
**Scope**: ~50 lines reduced | **Risk**: Low

`RocketExplode` (68 lines) and `GrenadeExplode` (51 lines) share identical particle ring/fireball/smoke code. Extract `SpawnExplosionVfx(pos, c1, c2, c3)`. Both keep their damage loops but call shared VFX. CIRC boss will likely need explosions too.

---

## TIER 2: Quick Wins (5-15 min each)

### 2A. HUD Cooldown Helpers
**Files**: `draw.c` only
**Scope**: ~150 lines → ~30 | **Risk**: Low

9+ copy-paste cooldown bar blocks in `DrawHUD`. Extract two static helpers:
- `DrawCooldownBar(x, y, w, h, timer, maxCd, color, label, ...)` — for most abilities
- `DrawPipBar(x, y, w, h, current, max, rechargeTimer, ...)` — for dash charges, shotgun blasts

### 2B. `mouse()` → `UpdateMouseAim()`
**Files**: `update.c` only
**Scope**: ~10 lines | **Risk**: Low

Rename to PascalCase. Communicates that it mutates `p->angle` as a side effect.

### 2C. `selectWeapons[]` + Hardcoded `5`
**Files**: `default.h`, `update.c`, `draw.c`
**Scope**: ~10 lines | **Risk**: Low

Add `#define NUM_PRIMARY_WEAPONS 5`. Replace hardcoded `5` in 5 loop sites. Consolidate duplicate array declaration.

### 2D. Deployable Count Helper
**Files**: `update.c` only
**Scope**: ~15 lines | **Risk**: Low

Three identical "count active deployables of type X" loops → `static int CountActiveDeployables(DeployableType type)`.

### 2E. Sniper Slow Decoupling from DMG_PIERCE
**Files**: `mecha.h`, `update.c`
**Scope**: ~10 lines | **Risk**: Low

`DMG_PIERCE` triggers sniper slow in projectile collision — any future pierce weapon inherits it. Add `bool appliesSlow` to Projectile, set true only for sniper shots.

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

Extract: `DrawCooldownColumn()`, `DrawWeaponStatus()`, `DrawPauseMenu()`, `DrawGameOver()`. Do after 2A (cooldown helpers make the column very short).

### 3C. Boss Spawn Path
**Files**: `spawn.c`, `update.c`, `game.h`
**Scope**: ~30 lines | **Risk**: Low

TRAP boss spawn in `UpdateEnemies` duplicates most of `SpawnEnemy` with inline field init. Extract `SpawnBoss(EnemyType type)` in `spawn.c` reusing `ENEMY_DEFS` table. Prevents another copy-paste for CIRC.

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
Tier 1 (do first, unblocks CIRC):     1A → 1B → 1C
Tier 2 (quick wins, any order):        2C, 2D, 2B, 2E, 2A
Tier 3 (structural, has deps):         3C, 3A (after 1B+1C), 3B (after 2A), 3D (after 1B)
Tier 4 (future):                        4B, 4A
```

