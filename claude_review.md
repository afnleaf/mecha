# Claude Code Review — Persistent State

Read this before modifying game.c. Update after significant changes.

### What's been resolved
- **EnemyDef data table done.** SpawnEnemy collapsed from 80+ lines of cascading bools to a clean priority table + struct lookup.
- **OBB collision centralized.** Three dispatchers (`EnemyHitSweep`, `EnemyHitPoint`, `EnemyHitCircle`) switch on enemy type. Adding a new hitbox shape means adding one `case` per dispatcher.
- **Per-enemy scores wired up.** `DamageEnemy` uses `e->score`, each type sets its own from `_SCORE` defines.
- **Linger effect refactored.**
- **Player model is a tetrahedron.** `DrawShape2D` renders a regular tetrahedron with rainbow HSV gradient, backface culling, painter's sort.
- **Rocket launcher added.** Rockets fly to cursor target, explode on arrival or enemy hit. Area damage, knockback, visual ring (Explosive pool). Reuses bullet pool with `isRocket` flag.
- **Octagon (OCTA) enemy added.** Fast chaser (300+ speed), red stop sign, 33 contact damage. Pure melee.
- **rtypes.h added.** Rust-style type aliases. Currently only used in `HsvToRgb`.

### Current stats
- **game.c**: 2229 lines
- **default.h**: 166 lines, 6 enemy type configs + 6 weapon configs
- **Enemy types**: 6 implemented (TRI, RECT, PENTA, RHOM, HEXA, OCTA), 2 stubbed (TRAP, CIRC)
- **Player weapons**: 6 (gun, sword, dash, spin, shotgun, rocket)
- **Pools**: bullets[1024], enemies[1024], particles[1024], explosives[8]

### Refactoring assessment

#### HIGH — required before v(-2) (add all weapons and enemies)

**1. Bullet -> Projectile migration.** The `Projectile` struct and `ProjectileType` enum already exist (lines 87-104) but nothing uses them. The actual code runs on `Bullet` with three competing type indicators: `isRocket` bool, `isEnemy` bool, and an unused `int type` field. When laser (hitscan — no position/velocity at all) and grenade (delayed fuse) arrive, more bools won't work. The migration path: rename Bullet pool to Projectile pool, replace bool flags with the existing enum, add type-specific fields as needed.

**2. Enemy shooting AI extraction.** `UpdateEnemies` has three back-to-back if-blocks for RECT (979-992), PENTA (995-1029), HEXA (1032-1056). All follow "decrement timer, check distance, fire pattern." The patterns are different enough that a pure data table can't capture them, but function pointers per `EnemyType` would let new shooters be added without touching UpdateEnemies. Something like `static void (*EnemyShoot[])(Enemy*, Vector2, float) = { [RECT] = ShootRect, [PENTA] = ShootPenta, ... };`

**3. Sweep damage deduplication.** Sword damage (806-831) and spin damage (861-889) are structurally identical: calculate sweep angle from progress, iterate enemies, check `lastHitAngle`, call `EnemyHitSweep`, call `DamageEnemy`. A shared `SweepDamage(origin, angle, radius, damage, lastHitArray)` function would collapse both and make it trivial to add new melee abilities.

#### MEDIUM — code quality and maintainability

**4. Hardcoded magic numbers.** These should move to default.h:
- `4.0f` muzzle offset (lines 337, 352, 760, 984, 1009, 1045) -> `MUZZLE_OFFSET`
- `200.0f` contact knockback (1069) -> `CONTACT_KNOCKBACK`
- `8.0f` camera lerp (1240) -> `CAMERA_LERP_SPEED`
- `0.05f` dt clamp (1259) -> `MAX_FRAME_TIME`
- `3.0f` enemy steering lerp (968) -> `ENEMY_STEERING_RATE`
- `0.1f` hit flash duration (499) -> `HIT_FLASH_DURATION`
- `14.0f` ghost trail spacing (1804) -> `DASH_GHOST_SPACING`
- Dash-slash multipliers `1.3f` arc and `1.5f` radius appear in 3 places (784-791, 807-811, DrawWorld) -> `SWORD_DASH_ARC_MULT`, `SWORD_DASH_RADIUS_MULT`

**5. HUD cooldown drawing is repetitive.** Lines 1957-2054: dash, spin, shotgun, rocket each have near-identical bar/pip drawing. A `DrawCooldownBar(x, y, ratio, color, label)` and `DrawChargePips(x, y, current, max, ...)` helper would cut ~80 lines to ~20.

**6. `lastHitAngle[MAX_ENEMIES]` is 4KB per array.** Both `Sword` and `Spin` carry `float[1024]` — 8KB total inside Player for a "don't double-hit" check. A bitfield (`uint8_t hitMask[MAX_ENEMIES/8]` = 128 bytes) or frame-counter comparison would be far lighter. Not a perf problem now, but it's wasteful and the memset-to-negative-1000 pattern is fragile.

**7. `MAX_EXPLOSIVES` defined in game.c, not default.h.** Line 122. Every other pool size lives in default.h. This one snuck through.

**8. Explosive update is inline in UpdateGame.** Lines 1290-1295. Every other entity type has its own `UpdateX(dt)` function. Explosives are the odd one out — should be `UpdateExplosives(dt)` for consistency.

**9. `mouse()` function has a side effect.** Line 652: lowercase name breaks the `PascalCase` convention. More importantly, it **mutates `p->angle`** while claiming to just return `toMouse`. Should either be renamed to `UpdateAim()` or have the angle mutation made explicit at the call site.

**10. SpawnBullet and SpawnRocket are near-duplicates.** Both scan for a free slot, fill the same fields. SpawnRocket just adds `isRocket = true` and `target`. Should unify once Projectile migration happens.

#### LOW — cleanup

**11. Commented-out code to remove.** Old crosshair (2078-2081, 2094-2097), old spawn positions in SpawnEnemy (422-423, etc.), dead `SpawnParticles` call (881), old bSize line (1633).

**12. Enemy-enemy separation.** Enemies stack into one pixel when chasing. O(n^2) pairwise nudge in UpdateEnemies would fix it. Game feel issue, not correctness.

**13. Spin lifesteal truncation.** `SPIN_DAMAGE * SPIN_LIFESTEAL` = 25 * 0.1 = 2.5, cast to int = 2hp. The truncation means lifesteal is effectively 8% not 10%. Not critical but worth noting if damage numbers change.

**14. DrawCube2D is currently unused** (lines 1474-1599). `DrawShape2D` (tetrahedron) is what renders the player. **Do not delete** — plan is to implement all five platonic solids (tetrahedron, cube, octahedron, dodecahedron, icosahedron) as selectable player characters. DrawCube2D is the second of five. Both share the same structure (vertices -> rotate -> project -> backface cull -> painter's sort -> subdivide -> HSV gradient) and should eventually share a common rendering pipeline parameterized by vertex/face data.

### Architecture assessment

The code is well organized for where it is. The Update/Draw separation is clean. The collision dispatchers are the right abstraction. The EnemyDef data table was the right call — same pattern applies to projectiles and shooting AI next.

The three pressure points for v(-2):
1. **Projectile types** — Bullet must become Projectile before laser/grenade/railgun
2. **Enemy shooting dispatch** — function pointers before adding more shooter types
3. **Sweep damage abstraction** — shared function before adding more melee abilities

### What's next (priority order)
1. **Projectile type refactor** — Bullet -> Projectile with existing enum, before laser/grenade/railgun
2. **Enemy shoot dispatch** — function pointer table, decouple AI from UpdateEnemies
3. **Sweep damage function** — shared melee hit logic for sword/spin/future abilities
4. **Magic numbers to default.h** — full pass, makes game feel tunable
5. **HUD cooldown helpers** — reduce repetition before adding more weapon UI
6. **Platonic solid rendering pipeline** — generalize DrawShape2D/DrawCube2D into a shared vertex/face data-driven renderer
7. **Enemy-enemy separation** — nudge pass for game feel
8. Then: add remaining weapons (laser, grenade, railgun, BFG) and enemies (TRAP mini boss, CIRC big boss) on clean foundations
