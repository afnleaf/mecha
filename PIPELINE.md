# Mecha Data Transformation Pipeline

## The Data

One struct. `GameState g`. Everything the game is and does lives here.

```
GameState g
├── Player         (pos, vel, angle, hp, 17 weapon/ability structs, slots[12])
├── Projectile     [1024]
├── Enemy          [1024]
├── Deployable     [8]
├── FirePatch      [64]
├── LightningChain (single, complex)
├── VfxState vfx
│   ├── Particle   [1024]
│   ├── Beam       [8]
│   ├── Explosive  [8]
│   └── MineWebVfx [4]
├── Camera2D
└── scalars        (score, spawnTimer, spawnInterval, enemiesKilled, gameOver, paused, screen, selectIndex, selectPhase)
```

All pools are flat fixed-size arrays with an `active` bool as the allocation flag. Linear scan to find a free slot (spawn), linear scan to process active slots (update/draw). No pointers between entities — relationships are by index (`aggroIdx`, `targetIdx`, `hit[MAX_ENEMIES]`).

## The Pipeline

```
NextFrame()
├── cursor visibility (screen state → OS cursor)
├── UpdateGame()         INPUT → TRANSFORM → mutate g
└── DrawGame()           READ g → TRANSFORM → OUTPUT pixels
```

### Phase 1: Update — Input → State Mutation

```
UpdateGame()
│
├── WindowResize()                    [platform → window state]
├── SCREEN_SELECT? → UpdateSelect()   [keyboard → g.player.primary/secondary, g.screen]
│   return early
│
├── dt = clamp(GetFrameTime(), DT_MAX)
├── pause/fullscreen toggle           [keyboard → g.paused]
├── gameOver? → restart check          [keyboard → InitGame()]
│   return early
│
├── UpdatePlayer(dt)
│   ├── mouse()                        [mouse+camera → player.angle, toMouse]
│   ├── movement                       [WASD → moveDir → vel → pos]
│   │   └── speed modifiers            (minigun slow, ADS slow, overheat boost)
│   ├── dash                           [space → dash state, decoy, super window]
│   ├── iframes                        [timer tick]
│   ├── shadow                         [lerp shadowPos toward pos]
│   ├── map clamp                      [pos → clamped pos]
│   │
│   ├── PRIMARY WEAPON (switch on player.primary)
│   │   ├── WPN_GUN                    [M1 → SpawnProjectile, heat++]
│   │   │   └── minigun (M2)           [M2 → spinUp, SpawnProjectile, heat++]
│   │   │   └── overheat/vent QTE      [heat → overheated → ventCursor → result]
│   │   ├── WPN_SWORD                  [M1 → sweep, M2 → lunge]
│   │   │   └── dash slash/lunge       [dash.active + M1/M2 → enhanced attack]
│   │   │   └── hit detection          [sweep arc vs enemies → DamageEnemy]
│   │   ├── WPN_REVOLVER               [M1 → precise, M2 → fan]
│   │   │   └── reload + active reload [empty → reloadTimer → QTE timing]
│   │   │   └── dash bonus rounds      [dash during reload → bonusRounds]
│   │   ├── WPN_SNIPER                 [M1 → hip fire, M2 → ADS]
│   │   │   └── super shot             [dash timing + M2 → superShotReady]
│   │   └── WPN_ROCKET                 [M1 → SpawnRocket, M2 → detonate]
│   │       └── rocket jump             [self-proximity → knockback]
│   │
│   ├── ABILITIES (12 slots, keybind-driven)
│   │   ├── shotgun                    [key → FireShotgunBlast]
│   │   ├── railgun                    [key → FireHitscan(pierce all)]
│   │   ├── spin                       [key → sweep + deflect + lifesteal]
│   │   ├── ground slam                [key → cone AoE stun]
│   │   ├── parry                      [key → deflect window → stun on success]
│   │   ├── turret                     [key → SpawnDeployable(TURRET)]
│   │   ├── mine                       [key → SpawnDeployable(MINE)]
│   │   ├── heal field                 [key → SpawnDeployable(HEAL)]
│   │   ├── shield                     [key → arc shield, absorb projectiles]
│   │   ├── grenade                    [key → SpawnGrenade]
│   │   ├── fire zone                  [key → SpawnFirePatch spray]
│   │   └── BFG                        [key → SpawnProjectile(BFG) if charge full]
│   │
│   └── SPAWN TIMER                    [spawnTimer -= dt → SpawnEnemy()]
│       └── SpawnEnemy                 [ENEMY_DEFS table + SPAWN_PRIORITY + kills threshold + RNG → Enemy slot]
│
├── UpdateEnemies(dt)
│   ├── per enemy:
│   │   ├── debuff timers              [slow/root/stun tick down]
│   │   ├── aggro                      [decoy/turret check → aggroIdx]
│   │   ├── movement                   [chase target (lerp vel) → pos, map clamp]
│   │   ├── shooting (switch on type)  [RECT/PENTA/HEXA → SpawnProjectile at player]
│   │   └── contact damage             [overlap player → DamagePlayer + knockback]
│   └── hitFlash tick
│
├── UpdateProjectiles(dt)
│   ├── per projectile:
│   │   ├── movement                   [vel → pos, drag for grenades, height arc]
│   │   ├── lifetime                   [timer tick → deactivate]
│   │   ├── map boundary               [out of bounds → deactivate or bounce]
│   │   ├── enemy collision            [EnemyHitPoint → DamageEnemy, deactivate]
│   │   │   └── rocket/grenade         [→ RocketExplode/GrenadeExplode (AoE)]
│   │   │   └── BFG                    [→ TriggerLightningChain]
│   │   │   └── shotgun bounce         [bounces > 0 → reflect, bounces--]
│   │   ├── player collision           [isEnemy + overlap → DamagePlayer]
│   │   │   └── parry active?          [→ reflect projectile, stun nearby]
│   │   │   └── spin active?           [→ deflect, speed up, damage boost]
│   │   │   └── shield active?         [→ absorb if in arc]
│   │   └── deployable collision       [isEnemy + turret overlap → damage turret]
│
├── UpdateLightningChain(dt)           [BFG chain: wave propagation, hop to nearby enemies]
├── UpdateParticles(dt)                [vel*drag → pos, lifetime → fade → deactivate]
├── UpdateBeams(dt)                    [timer tick → deactivate]
├── UpdateDeployables(dt)
│   ├── TURRET                         [find nearest enemy → SpawnProjectile]
│   ├── MINE                           [enemy in trigger radius → root AoE]
│   └── HEAL                           [player in radius → heal tick]
├── UpdateFirePatches(dt)              [timer tick, damage enemies in radius]
├── UpdateExplosives(dt)               [timer tick → deactivate (VFX only)]
│
└── MoveCamera(dt)                     [lerp camera.target → player.pos, handle resize]
```

### Phase 2: Draw — State → Pixels

```
DrawGame()
│
├── SCREEN_SELECT? → DrawSelect()
│   ├── weapon list + cursor
│   ├── keybind columns
│   └── platonic solid preview per weapon
│   return early
│
├── BeginDrawing / ClearBackground
│
├── BeginMode2D(camera) ─── WORLD SPACE ───
│   DrawWorld()
│   ├── grid                            [MAP_SIZE, GRID_STEP → lines]
│   ├── map border                      [rectangle outline]
│   ├── fire patches                    [pos, radius, timer → flickering circles]
│   ├── mine webs                       [pos, timer → spoke+ring pattern]
│   ├── decoy                           [shadowPos → pulsing ghost solid]
│   ├── player shadow                   [shadowPos → dark projected solid]
│   ├── player                          [pos, angle, rotY → DrawPlayerSolid (HSV rainbow)]
│   │   └── DrawTetra2D / DrawCube2D / DrawOcta2D / DrawDodeca2D / DrawIcosa2D
│   │       [3D→2D projection, face sorting by Z, HSV coloring per vertex]
│   ├── weapon visuals
│   │   ├── gun/minigun barrel          [pos, angle → rotated rectangle]
│   │   ├── sword/lunge arc             [timer, angle → DrawRingSector]
│   │   ├── spin arc                    [timer → full ring trail]
│   │   ├── shield arc                  [angle, hp → colored arc]
│   │   ├── slam cone                   [vfxTimer, angle → expanding sector]
│   │   ├── parry ring                  [timer → pulsing circle]
│   │   └── sniper scope line           [aiming → thin line to mouse]
│   ├── enemies (per active)
│   │   ├── shape (switch on type)      [TRI→triangle, RECT→rotated rect, PENTA→pentagon, etc]
│   │   ├── hit flash                   [hitFlash > 0 → WHITE overlay]
│   │   └── HP bar                      [hp/maxHp → colored bar above enemy]
│   ├── projectiles (per active)
│   │   ├── bullets                     [pos, vel → circle + trail line]
│   │   ├── rockets                     [pos, vel → oriented triangle]
│   │   ├── grenades                    [pos, height → circle + shadow + arc]
│   │   ├── sniper rounds               [pos, vel → elongated diamond]
│   │   └── BFG orb                     [pos → pulsing glow circle]
│   ├── deployables
│   │   ├── turret                      [pos, aim → base + barrel]
│   │   ├── mine                        [pos → pulsing circle]
│   │   └── heal field                  [pos, radius → green ring]
│   ├── beams                           [origin→tip, timer/duration → fading line+glow]
│   ├── lightning arcs                  [from→to, jitter → segmented bolts]
│   ├── explosives VFX                  [pos, timer → expanding ring]
│   └── particles                       [pos, lifetime/maxLifetime → fading circle]
│   EndMode2D
│
├── ─── SCREEN SPACE ───
│   DrawHUD()
│   ├── HP bar                          [hp/maxHp → colored bar, text]
│   ├── score + kills                   [g.score, g.enemiesKilled → text]
│   ├── cooldown column (left side)     [per ability: cooldownTimer → labeled bar]
│   ├── dash pips                       [charges → filled/empty rectangles]
│   ├── BFG charge bar                  [charge/cost → blue bar]
│   ├── player-relative arcs (WorldToScreen2D)
│   │   ├── revolver rounds             [rounds → pip arcs]
│   │   ├── revolver reload QTE         [reloadTimer → sweeping arc + sweet spot]
│   │   ├── gun heat                    [heat → orange arc]
│   │   ├── gun overheat QTE            [ventCursor → sweeping arc + zone]
│   │   ├── minigun spin-up             [spinUp → blue arc]
│   │   ├── sniper cooldown arc         [cooldownTimer → arc]
│   │   ├── rocket cooldown arc         [cooldownTimer → arc]
│   │   └── shield HP arc              [hp/maxHp → segmented arc]
│   ├── crosshair                       [mouse pos → green cross with gap]
│   ├── title text                      [bottom of screen]
│   ├── FPS                             [GetFPS() → text]
│   ├── pause overlay                   [g.paused → dimmed bg, controls list]
│   └── game over overlay               [g.gameOver → score, restart prompt]
│
└── EndDrawing
```

## The Operation Table

Per-frame operations on each entity pool:

```
               | input  | spawn  | move   | collide | damage | debuff | timer  | draw   |
player         |   x    |        |   x    |   x     |   x    |        |   x    |   x    |
enemies[1024]  |        |   x    |   x    |   x     |   x    |   x    |   x    |   x    |
projectiles    |        |   x    |   x    |   x     |   x    |        |   x    |   x    |
particles      |        |   x    |   x    |         |        |        |   x    |   x    |
deployables[8] |        |   x    |        |   x     |   x    |        |   x    |   x    |
firePatches    |        |   x    |        |   x     |   x    |        |   x    |   x    |
beams[8]       |        |   x    |        |         |        |        |   x    |   x    |
explosives[8]  |        |   x    |        |         |        |        |   x    |   x    |
lightning      |        |   x    |        |   x     |   x    |        |   x    |   x    |
camera         |        |        |   x    |         |        |        |        |        |
```

## Cross-Cutting Data Flows

These are the key interactions where one system reaches into another:

```
player input ──→ SpawnProjectile/SpawnDeployable/FireHitscan ──→ pools
                                                                   │
enemy.shootTimer ──→ SpawnProjectile(isEnemy=true) ────────────────┘
                                                                   │
projectile collision ──→ DamageEnemy(idx) ──→ enemy.hp             │
                    ──→ DamagePlayer() ──→ player.hp               │
                    ──→ RocketExplode/GrenadeExplode ──→ AoE scan  │
                    ──→ TriggerLightningChain ──→ wave propagation │
                                                                   │
DamageEnemy ──→ bfg.charge += damage  (ult charge from all damage) │
            ──→ enemy.hp <= 0 → score++, enemiesKilled++           │
                                       │                           │
                    enemiesKilled ──→ SpawnEnemy unlock thresholds  │
                                                                   │
deploy.turret ──→ SpawnProjectile (auto-aim nearest enemy)         │
deploy.mine ──→ enemy.rootTimer (AoE root)                         │
deploy.heal ──→ player.hp++ / turret.hp++                          │
                                                                   │
parry/spin/shield ──→ intercept enemy projectiles in UpdateProjectiles
```

## Spawn → Live → Die Lifecycle

Every pooled entity follows the same pattern:

```
INACTIVE (active=false)
    │
    ▼  linear scan finds first free slot
SPAWN (set fields from constants/params, active=true)
    │
    ▼  per-frame in Update*()
LIVE  (move, collide, tick timers, interact with other pools)
    │
    ▼  lifetime <= 0 || hp <= 0 || out of bounds || consumed
DIE   (active=false, spawn death particles/VFX, update score)
    │
    ▼  slot now available for reuse
INACTIVE
```

## VfxState — The Event Buffer

`VfxState` is a sub-struct inside `GameState` that holds purely visual pools. Update writes into `g.vfx`, draw reads from `g.vfx`. This cleanly separates gameplay state from draw state while keeping everything in the same contiguous struct.

```c
typedef struct VfxState {
    Particle particles[MAX_PARTICLES];
    Beam beams[MAX_BEAMS];
    Explosive explosives[MAX_EXPLOSIVES];
    MineWebVfx mineWebs[MAX_MINE_WEBS];
} VfxState;
```

## What's Not In The Pipeline Yet

- Damage calculation: `DamageEnemy`/`DamagePlayer` pass through `DamageType` and `DamageMethod` but ignore them. The commented-out `Damage` struct in mecha.h shows the intended future event system.
- TRAP (mini boss) and CIRC (big boss) enemy types — defined in the enum, no `EnemyDef` row or behavior yet.
- Room/round progression — currently infinite survival, no state machine for rounds.
- Chassis identity — all 5 solids drawn, but no gameplay differentiation per shape.
