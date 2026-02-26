# Claude Thoughts — Codebase Analysis

Last analyzed: 2026-02-26
mecha.c: 2482 lines | default.h: 434 lines | mecha.h: 327 lines

---

## What Exists

### Player Systems (11 weapon/ability systems)
| System | Input | Delivery | Status |
|--------|-------|----------|--------|
| Machine Gun | M1 hold | Projectile | Working |
| Laser | M1 hold | Hitscan (1 pierce) | Working |
| Sword | M1 click | Sweep arc | Working, dash slash may be buggy |
| Revolver | M1 click / M2 fan | Projectile | Working |
| Minigun | M1 hold | Projectile (spin-up) | Working |
| Shotgun | E | Projectile (spread) | Working |
| Rocket | Q | Projectile -> AOE | Working |
| Railgun | Z | Hitscan (full pierce) | Working |
| Sniper | X | Projectile + slow debuff | Working |
| Dash | Space | Movement + iframes | Working (2 charges) |
| Spin | Shift | Sweep arc + lifesteal | Working |

### Enemy Types (6 active, 2 stubbed)
| Type | Shape | Behavior | Spawns After |
|------|-------|----------|-------------|
| TRI | Triangle | Chase | Always |
| RECT | Rectangle | Chase + shoot single | 5 kills |
| RHOM | Rhombus | Fast chase | 100 kills |
| PENTA | Pentagon | Slow + shoot 2x5 rows | 15 kills |
| HEXA | Hexagon | Strafe orbit + fan shot | 25 kills |
| OCTA | Octagon | Very fast chase | 10 kills |
| TRAP | Trapezoid | Stubbed (mini boss) | — |
| CIRC | Circle | Stubbed (big boss) | — |

### Infrastructure
- **Pools**: projectiles[1024], enemies[1024], particles[1024], explosives[8], beams[8]
- **Collision**: 3 dispatchers (EnemyHitSweep, EnemyHitPoint, EnemyHitCircle) switching on enemy type
- **Rendering**: DrawShape2D (tetrahedron), DrawCube2D (cube) — HSV gradient with barycentric subdivision, backface culling, painter's sort
- **Pipeline**: UpdateGame -> UpdatePlayer -> UpdateEnemies -> UpdateProjectiles -> UpdateParticles -> UpdateBeams -> MoveCamera -> DrawGame -> DrawWorld -> DrawHUD
- **Build**: gcc native (debug/optimized), emcc WASM -> histos pack -> single HTML

---

## What Changed Since claude_review.md Was Written

The review file is now outdated. It says 2229 lines and tracks the Bullet->Projectile migration as the #1 HIGH priority — that migration is **done**. The codebase has grown ~250 lines and added significant weapon systems since the review was written.

**Completed since review:**
1. Bullet -> Projectile migration (was HIGH #1) — fully done, SpawnProjectile is the unified API
2. Revolver with M1 precise / M2 fan-the-hammer
3. Minigun with spin-up mechanic and movement slow
4. Laser as hitscan weapon (continuous DPS, stops at first hit)
5. Railgun as hitscan weapon (pierces all, beam pool linger)
6. Sniper with slow debuff system
7. Weapon select screen (5 primary choices)
8. Beam pool for lingering visual effects
9. Magic numbers to default.h — massive pass done (434 lines of defines now)

**Still outstanding from review:**
- Enemy shooting dispatch via function pointers (HIGH #2) — still inline if-blocks in UpdateEnemies
- Sweep damage deduplication (HIGH #3) — sword and spin still copy-paste
- HUD cooldown helpers (MEDIUM #5) — still repetitive, now even worse with more weapons
- Explosive update inline in UpdateGame (MEDIUM #8)
- mouse() naming/side-effect (MEDIUM #9)
- lastHitAngle[MAX_ENEMIES] memory (MEDIUM #6)

---

## Architectural Observations

### What's Working Well

**The single GameState struct.** `static GameState g` with `memset(&g, 0, sizeof(g))` in InitGame is clean. All game state is contiguous, cache-friendly for the common case. The Emscripten `void(*)(void)` callback pattern requires this anyway, but it's also just the right design.

**EnemyDef data table.** The ENEMY_DEFS[] array with designated initializers and the SPAWN_PRIORITY[] table is the right pattern. Adding a new enemy type is: add enum value, add defines to default.h, add a row to the table, add drawing in DrawWorld switch, add shooting AI if needed, add collision case if non-circular.

**Collision dispatcher pattern.** Three functions switching on enemy type means every new hitbox shape is exactly 3 cases. RECT is the only non-circle so far, but the pattern scales cleanly to TRAP (trapezoid) or CIRC (circle with radius).

**Hitscan reuse.** `FireHitscan` with `maxPierces` parameter elegantly handles both laser (maxPierces=1, stops at first hit) and railgun (maxPierces=MAX_ENEMIES, hits everything). The same `EnemyHitSweep` collision dispatchers handle sweep melee and hitscan ray tests. That's good — the line-segment-vs-shape test is the same math regardless of whether it's a sword or a laser.

**HsvToRgb + barycentric subdivision.** The platonic solid rendering is genuinely interesting. Projecting 3D vertices to 2D, backface culling via cross product sign, painter's sort, then subdividing each face into N^2 subtriangles with interpolated hue values. This is a mini software rasterizer. The approach generalizes cleanly to all 5 platonic solids.

### What's Not Working

**The sniper slow debuff is type-coupled.** In UpdateProjectiles (line 1278): `if (b->dmgType == DMG_PIERCE)` triggers the slow. This means any future weapon with DMG_PIERCE damage type will also apply sniper slow. The debuff should be keyed on weapon identity, not damage type. Quick fix: add a `bool appliesSlow` or similar to Projectile, or check `b->dmgType == DMG_PIERCE && !b->isEnemy`.

**Revolver M2 fan can be triggered during reload.** Looking at the code flow: reloadTimer check is at line 776 and breaks early, so M2 press check at line 827 only fires when not reloading. Actually this is fine — the `break` at 782 prevents reaching the M2 block. Good.

**The weapon select screen hardcodes 5 options** with magic number `5` in the modulo wrapping (lines 560-562) and the for-loop (line 599). This should be `NUM_PRIMARY_WEAPONS` or derived from the enum.

**Explosive update is orphaned.** Lines 1394-1399 in UpdateGame are the only entity update not extracted into its own function. Every other pool has `UpdateX(dt)`.

### Structural Patterns

The codebase has three distinct layers of weapon complexity:

1. **Primary weapons** (mutually exclusive via select screen): Gun, Laser, Sword, Revolver, Minigun — these dispatch on `p->primary` in the M1 handler switch
2. **Secondary weapons** (always available): Shotgun, Rocket, Railgun, Sniper — independent cooldowns, separate keybinds
3. **Abilities** (always available): Dash, Spin — movement/utility with unique mechanics

This three-tier structure isn't explicitly codified anywhere but it's the de facto design. Understanding this is important for the v(-1) milestone (weapon selection between rounds) — the user will want to add/swap weapons at each tier.

### Projectile Ownership

All projectiles share one pool with `isEnemy` bool for ownership. Enemy projectiles only do player-hit checks, player projectiles only do enemy-hit checks. This works now but means:
- No friendly fire or reflect mechanics
- No projectile-on-projectile collision (parry, counterspell)
- Pool contention between player and enemy projectiles during intense fights

For v(-2) this is fine. Splitting the pool would be premature.

---

## Memory Layout Thinking

The GameState struct is approximately:
- Player: ~300 bytes (includes all weapon substruct fields + lastHitAngle arrays = 2x 4096 bytes)
- Projectiles: 1024 * ~60 bytes = ~60KB
- Enemies: 1024 * ~72 bytes = ~72KB
- Particles: 1024 * ~36 bytes = ~36KB
- Explosives: 8 * ~20 bytes = ~160 bytes
- Beams: 8 * ~40 bytes = ~320 bytes
- Camera + scalars: ~50 bytes

Total: **~177KB** — fits comfortably in L2 cache on any modern CPU. The hot loop (UpdateProjectiles iterating enemies for collision) touches ~132KB across two pools. Not a performance concern at these pool sizes.

The `lastHitAngle[MAX_ENEMIES]` arrays (8KB in Player) are the one outlier. They're reset with a fill-to-negative-1000 pattern every swing activation. A bitfield (128 bytes) or hit-frame-counter per enemy would be better, but at these sizes it doesn't matter for performance — it's a code smell issue.

---

## What Needs To Happen Next

In priority order, aligned with v(-2) goals:

### 1. Update claude_review.md
The review file is the persistent state document. It needs to reflect current reality: line counts, weapon counts, which HIGH items are resolved, new refactoring priorities.

### 2. Enemy shooting dispatch (function pointers)
Three if-blocks in UpdateEnemies (RECT, PENTA, HEXA) need to become a dispatch table before adding TRAP or CIRC boss shooting patterns. The pattern:
```c
typedef void (*EnemyShootFn)(Enemy*, Vector2 toPlayer, float dist, float dt);
static const EnemyShootFn ENEMY_SHOOT[] = {
    [TRI] = NULL, [RECT] = ShootRect, [PENTA] = ShootPenta,
    [HEXA] = ShootHexa, [OCTA] = NULL, ...
};
```
Call site: `if (ENEMY_SHOOT[e->type]) ENEMY_SHOOT[e->type](e, toPlayer, dist, dt);`

### 3. Sweep damage deduplication
Sword (lines 873-898) and Spin (lines 934-963) are structurally identical loops. Extract:
```c
static void SweepDamage(Vector2 origin, float sweepAngle, float radius,
    int damage, DamageType dmgType, float *lastHitAngles);
```

### 4. HUD cooldown helpers
Now 7 separate cooldown/pip displays (dash, spin, shotgun, rocket, railgun, sniper, revolver) with copy-paste rendering. Two helper functions would collapse this.

### 5. Boss enemies (TRAP, CIRC)
With the shooting dispatch table in place, these become: define stats in ENEMY_DEFS, write ShootTrap/ShootCirc, add DrawWorld case, add collision case if non-circular hitbox.

### 6. Missing weapons (grenade launcher, BFG)
The projectile system is ready. Grenade = PROJ_GRENADE type with delayed fuse (timer before RocketExplode). BFG = big slow projectile with massive AOE.

---

## Nitpicks and Cleanup

- `mouse()` lowercase function mutates player angle as side effect — rename or make explicit
- `MAX_ENTITIES` define on line 16 of default.h is empty/unused
- TRAP and CIRC draw cases are empty switch blocks with redundant `break` after `break`
- Commented-out crosshair code (4 blocks) in DrawHUD
- Commented-out spawn position code in SpawnEnemy (4 blocks)
- build.sh is actually build2.sh (the original build.sh is in old/)
- The `Rocket` struct has comments questioning its own fields ("this isn't getting used?" / "why does Rocket have a cooldown")
- Spin lifesteal truncation: 25 * 0.1 = 2.5 -> (int)2 = 8% effective, not 10%
