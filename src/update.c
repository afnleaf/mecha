// update.c
// mutate the game state with various data transformations
// handle input
#include "game.h"

// forward declarations for functions used before defined
static void RocketExplode(Vector2 pos);
static void SpawnDeployable(DeployableType type, Vector2 pos);
static void SpawnFirePatch(Vector2 pos);
static void UpdateParticles(float dt);
static void UpdateProjectiles(float dt);


// Hitscan ------------------------------------------------------------------ /
// Returns beam tip: closest enemy pos (laser) or rayEnd (railgun).
// maxPierces==1 -> stop at first hit; >1 -> hit all enemies along ray.
static Vector2 FireHitscan(Vector2 origin, Vector2 dir,
    float range, int damage, DamageType dmgType, int maxPierces)
{
    Vector2 rayEnd = Vector2Add(origin, Vector2Scale(dir, range));

    if (maxPierces == 1) {
        // Laser: find closest hit, terminate beam there
        int   firstIdx  = -1;
        float firstDist = 1e30f;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &g.enemies[i];
            if (!e->active) continue;
            if (EnemyHitSweep(e, origin, rayEnd)) {
                float d = Vector2Distance(origin, e->pos);
                if (d < firstDist) {
                    firstIdx  = i;
                    firstDist = d;
                }
            }
        }
        if (firstIdx >= 0) {
            Enemy *e = &g.enemies[firstIdx];
            // project enemy center onto ray, back up by radius -> beam tip
            // stays on the aim line so there's no visual snapping to e->pos
            float proj = Vector2DotProduct(
                Vector2Subtract(e->pos, origin), dir);
            float tEntry = proj - e->size;
            if (tEntry < 0.0f) tEntry = 0.0f;
            Vector2 surfacePt = Vector2Add(origin, Vector2Scale(dir, tEntry));
            if (damage > 0) {
                DamageEnemy(firstIdx, damage, dmgType, HIT_SCAN);
                SpawnParticles(surfacePt, WHITE, HITSCAN_HIT_PARTICLES);
            }
            return surfacePt;
        }
        return rayEnd;
    } else {
        // Railgun: pierce every enemy along ray
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &g.enemies[i];
            if (!e->active) continue;
            if (EnemyHitSweep(e, origin, rayEnd)) {
                if (damage > 0) {
                    Vector2 hitPos = e->pos;
                    DamageEnemy(i, damage, dmgType, HIT_SCAN);
                    SpawnParticles(hitPos, WHITE, RAILGUN_HIT_PARTICLES);
                }
            }
        }
        return rayEnd;
    }
}

static bool IsAbilityPressed(Player *p, AbilityID ability) {
    for (int i = 0; i < ABILITY_SLOTS; i++) {
        if (p->slots[i].ability == ability
            && IsKeyPressed(p->slots[i].key))
            return true;
    }
    return false;
}

static bool IsAbilityDown(Player *p, AbilityID ability) {
    for (int i = 0; i < ABILITY_SLOTS; i++) {
        if (p->slots[i].ability == ability
            && IsKeyDown(p->slots[i].key))
            return true;
    }
    return false;
}

static void UpdateRailgun(Player *p, Vector2 toMouse, float dt)
{
    p->railgun.cooldownTimer -= dt;
    if (p->railgun.cooldownTimer < 0) p->railgun.cooldownTimer = 0;

    if (IsAbilityPressed(p, ABL_RAILGUN) && p->railgun.cooldownTimer <= 0) {
        p->railgun.cooldownTimer = RAILGUN_COOLDOWN;
        Vector2 aimDir = Vector2Normalize(toMouse);
        Vector2 muzzle = Vector2Add(p->pos,
            Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));

        Vector2 tip = FireHitscan(muzzle, aimDir,
            RAILGUN_RANGE, RAILGUN_DAMAGE, DMG_BALLISTIC, MAX_ENEMIES);
        SpawnBeam(muzzle, tip, RAILGUN_BEAM_DURATION,
            RAILGUN_COLOR, RAILGUN_BEAM_WIDTH);
        SpawnParticles(muzzle, WHITE, RAILGUN_MUZZLE_PARTICLES);
    }
}

//
static Vector2 UpdateMouseAim() {
    Player *p = &g.player;
    Vector2 screenMouse = GetMousePosition();
    Vector2 worldMouse = GetScreenToWorld2D(screenMouse, g.camera);
    Vector2 toMouse = Vector2Subtract(worldMouse, p->pos);
    p->angle = atan2f(toMouse.y, toMouse.x);
    return toMouse;
}

static int CountActiveDeployables(DeployableType type) {
    int count = 0;
    for (int i = 0; i < MAX_DEPLOYABLES; i++)
        if (g.deployables[i].active && g.deployables[i].type == type) count++;
    return count;
}

// Weapon Select Screen ----------------------------------------------------- /
static void UpdateSelect(void)
{
    float dt = GetFrameTime();
    if (dt > DT_MAX) dt = DT_MAX;
    Player *p = &g.player;

    WeaponType selectWeapons[] = {
        WPN_SWORD, WPN_REVOLVER, WPN_GUN, WPN_SNIPER, WPN_ROCKET
    };

    // WASD movement
    Vector2 moveDir = { 0, 0 };
    if (IsKeyDown(KEY_W)) moveDir.y -= 1;
    if (IsKeyDown(KEY_S)) moveDir.y += 1;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1;
    if (IsKeyDown(KEY_D)) moveDir.x += 1;
    float moveLen = Vector2Length(moveDir);
    if (moveLen > 0) moveDir = Vector2Scale(moveDir, 1.0f / moveLen);

    if (!p->dash.active)
        p->pos = Vector2Add(p->pos, Vector2Scale(moveDir, SELECT_PLAYER_SPEED * dt));

    // Dash
    if (p->dash.charges < p->dash.maxCharges) {
        p->dash.rechargeTimer -= dt;
        if (p->dash.rechargeTimer <= 0) {
            p->dash.charges++;
            if (p->dash.charges < p->dash.maxCharges)
                p->dash.rechargeTimer = p->dash.rechargeTime;
        }
    }
    if (IsKeyPressed(KEY_SPACE) && !p->dash.active && p->dash.charges > 0) {
        p->dash.active = true;
        p->dash.timer = p->dash.duration;
        bool wasFull = (p->dash.charges == p->dash.maxCharges);
        p->dash.charges--;
        if (wasFull) p->dash.rechargeTimer = p->dash.rechargeTime;
        if (moveLen > 0) {
            p->dash.dir = moveDir;
        } else {
            p->dash.dir = (Vector2){ cosf(p->angle), sinf(p->angle) };
        }
        SpawnParticles(p->pos, SKYBLUE, DASH_BURST_PARTICLES);
    }
    if (p->dash.active) {
        p->dash.timer -= dt;
        p->pos = Vector2Add(p->pos,
            Vector2Scale(p->dash.dir, p->dash.speed * dt));
        if (p->dash.timer <= 0) p->dash.active = false;
    }

    // Clamp to arena bounds
    float margin = p->size;
    if (p->pos.x < margin) p->pos.x = margin;
    if (p->pos.y < margin) p->pos.y = margin;
    if (p->pos.x > SELECT_ARENA_SIZE - margin) p->pos.x = SELECT_ARENA_SIZE - margin;
    if (p->pos.y > SELECT_ARENA_SIZE - margin) p->pos.y = SELECT_ARENA_SIZE - margin;

    // Mouse aim
    Vector2 screenMouse = GetMousePosition();
    Vector2 worldMouse = GetScreenToWorld2D(screenMouse, g.camera);
    Vector2 toMouse = Vector2Subtract(worldMouse, p->pos);
    p->angle = atan2f(toMouse.y, toMouse.x);

    // Pedestal positions (U curve, left to right by face count)
    Vector2 pedestals[NUM_PRIMARY_WEAPONS];
    float spacing = SELECT_ARENA_SIZE / 6.0f;
    for (int i = 0; i < NUM_PRIMARY_WEAPONS; i++) {
        float norm = (float)(i - 2) / 2.0f;
        pedestals[i] = (Vector2){
            spacing * (float)(i + 1),
            SELECT_PEDESTAL_Y + SELECT_PEDESTAL_CURVE * (1.0f - norm * norm)
        };
    }

    // Find nearest pedestal within interact radius
    g.selectIndex = -1;
    float bestDist = SELECT_INTERACT_RADIUS;
    for (int i = 0; i < NUM_PRIMARY_WEAPONS; i++) {
        // Phase 1: skip already-chosen primary
        if (g.selectPhase == 1 && selectWeapons[i] == p->primary) continue;
        float d = Vector2Distance(p->pos, pedestals[i]);
        if (d < bestDist) {
            bestDist = d;
            g.selectIndex = i;
        }
    }

    // M1 selects weapon
    if (g.selectIndex >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g.selectPhase == 0) {
            p->primary = selectWeapons[g.selectIndex];
            g.selectPhase = 1;
        } else {
            p->secondary = selectWeapons[g.selectIndex];
            // Transition to gameplay
            p->pos = (Vector2){ MAP_SIZE / 2.0f, MAP_SIZE / 2.0f };
            p->shadowPos = p->pos;
            // Clear particles and projectiles
            for (int i = 0; i < MAX_PARTICLES; i++)
                g.vfx.particles[i].active = false;
            for (int i = 0; i < MAX_PROJECTILES; i++)
                g.projectiles[i].active = false;
            g.screen = SCREEN_PLAYING;
            g.level = 1;
        }
    }

    // Weapon demo: fire same M1 attacks from highlighted pedestal
    if (g.selectIndex < 0) {
        g.selectDemoTimer = 0;
    }
    g.selectDemoTimer -= dt;
    if (g.selectDemoTimer <= 0 && g.selectIndex >= 0) {
        float intervals[] = {
            SWORD_DURATION + 0.3f,          // sword: swing + pause
            REVOLVER_COOLDOWN,              // revolver: actual cooldown
            1.0f / GUN_FIRE_RATE,           // gun: actual fire rate
            SNIPER_AIM_COOLDOWN,            // sniper: aimed cooldown
            ROCKET_COOLDOWN,                // rocket: actual cooldown
        };
        g.selectDemoTimer += intervals[g.selectIndex];
        int i = g.selectIndex;
        Vector2 base = pedestals[i];
        float demoAngle = (float)GetTime() * 1.5f;
        Vector2 aimDir = { cosf(demoAngle), sinf(demoAngle) };
        Vector2 muzzle = Vector2Add(base,
            Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
        switch (i) {
            case 0: { // SWORD — same as M1 sweep
                g.selectSwordTimer = SWORD_DURATION;
                g.selectSwordAngle = demoAngle;
                float arc = SWORD_ARC;
                float radius = SWORD_RADIUS;
                for (int j = 0; j < SWORD_SPARK_COUNT; j++) {
                    float a = demoAngle - arc / 2.0f
                        + arc * (float)j / (SWORD_SPARK_COUNT - 1);
                    Vector2 particlePos = Vector2Add(base,
                        (Vector2){ cosf(a) * radius * SWORD_SPARK_RADIUS_FRAC,
                                   sinf(a) * radius * SWORD_SPARK_RADIUS_FRAC });
                    Vector2 particleVel = { cosf(a) * SWORD_SPARK_SPEED,
                                            sinf(a) * SWORD_SPARK_SPEED };
                    SpawnParticle(particlePos, particleVel, ORANGE,
                        SWORD_SPARK_SIZE, SWORD_SPARK_LIFETIME);
                }
            } break;
            case 1: { // REVOLVER — same as M1 precise shot
                float spread = ((float)GetRandomValue(
                    -REVOLVER_PRECISE_SPREAD, REVOLVER_PRECISE_SPREAD))
                    * 0.001f;
                float bulletAngle = demoAngle + spread;
                Vector2 bulletDir = { cosf(bulletAngle), sinf(bulletAngle) };
                SpawnProjectile(muzzle, bulletDir,
                    REVOLVER_BULLET_SPEED, REVOLVER_DAMAGE,
                    REVOLVER_BULLET_LIFETIME, REVOLVER_PROJECTILE_SIZE,
                    false, false, PROJ_BULLET, DMG_BALLISTIC);
                SpawnParticle(muzzle,
                    Vector2Scale(bulletDir, GUN_MUZZLE_SPEED), WHITE,
                    GUN_MUZZLE_SIZE, GUN_MUZZLE_LIFETIME);
            } break;
            case 2: { // GUN — same as M1 machine gun
                float spread = ((float)GetRandomValue(
                    -GUN_SPREAD, GUN_SPREAD)) * 0.001f;
                float bulletAngle = demoAngle + spread;
                Vector2 bulletDir = { cosf(bulletAngle), sinf(bulletAngle) };
                SpawnProjectile(muzzle, bulletDir,
                    GUN_BULLET_SPEED, GUN_BULLET_DAMAGE,
                    GUN_BULLET_LIFETIME, GUN_PROJECTILE_SIZE,
                    false, false, PROJ_BULLET, DMG_BALLISTIC);
                SpawnParticle(muzzle,
                    Vector2Scale(bulletDir, GUN_MUZZLE_SPEED), WHITE,
                    GUN_MUZZLE_SIZE, GUN_MUZZLE_LIFETIME);
            } break;
            case 3: { // SNIPER — same as M1 hip fire
                float spread = (float)(GetRandomValue(
                    -SNIPER_HIP_SPREAD, SNIPER_HIP_SPREAD)) / 1000.0f;
                Vector2 dir = Vector2Rotate(aimDir, spread);
                Projectile *sn = SpawnProjectile(muzzle, dir,
                    SNIPER_HIP_BULLET_SPEED, SNIPER_HIP_DAMAGE,
                    SNIPER_BULLET_LIFETIME, SNIPER_PROJECTILE_SIZE,
                    false, false, PROJ_BULLET, DMG_PIERCE);
                if (sn) sn->appliesSlow = true;
                SpawnParticles(muzzle, (Color)SNIPER_COLOR,
                    SNIPER_MUZZLE_PARTICLES);
                SpawnParticle(muzzle,
                    Vector2Scale(aimDir, SNIPER_MUZZLE_SPEED), WHITE,
                    SNIPER_MUZZLE_SIZE, SNIPER_MUZZLE_LIFETIME);
            } break;
            case 4: { // ROCKET — same as SpawnRocket
                Projectile *r = SpawnProjectile(muzzle, aimDir,
                    ROCKET_SPEED, ROCKET_DIRECT_DAMAGE,
                    ROCKET_LIFETIME, ROCKET_PROJECTILE_SIZE,
                    false, true, PROJ_ROCKET, DMG_EXPLOSIVE);
                if (r) r->target = Vector2Add(base,
                    Vector2Scale(aimDir, 300.0f));
                SpawnParticles(muzzle, RED, ROCKET_MUZZLE_PARTICLES);
                SpawnParticle(muzzle,
                    Vector2Scale(aimDir, ROCKET_MUZZLE_SPEED), ORANGE,
                    ROCKET_MUZZLE_SIZE, ROCKET_MUZZLE_LIFETIME);
            } break;
        }
    }

    // Tick sword demo arc
    if (g.selectSwordTimer > 0) g.selectSwordTimer -= dt;

    // Update projectiles, particles, shadow lag, camera
    UpdateProjectiles(dt);
    UpdateParticles(dt);
    p->shadowPos = Vector2Lerp(p->shadowPos, p->pos, SHADOW_LAG * dt);
    g.camera.target = Vector2Lerp(g.camera.target, p->pos, CAMERA_LERP_RATE * dt);
    g.camera.offset = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}

// SweepDamage — shared by sword and spin
// Returns hit count; hitIndices[] filled with enemy indices for post-hit work
static int SweepDamage(
    Vector2 origin, Vector2 sweepEnd, float sweepAngle,
    int damage, DamageType dmgType,
    float *lastHitAngles, int *hitIndices, int maxHits)
{
    int hits = 0;
    for (int i = 0; i < MAX_ENEMIES && hits < maxHits; i++) {
        Enemy *ei = &g.enemies[i];
        if (!ei->active) continue;
        if (sweepAngle - lastHitAngles[i] < PI) continue;
        if (!EnemyHitSweep(ei, origin, sweepEnd)) continue;
        DamageEnemy(i, damage, dmgType, HIT_MELEE);
        lastHitAngles[i] = sweepAngle;
        hitIndices[hits++] = i;
    }
    return hits;
}

// player inputs and mechanics
// should we refactor this further?
// or do we tackle that once we start adding different loadoats?
// diff base mechas
// adding abilities, etc
static void UpdatePlayer(float dt)
{
    Player *p = &g.player;
    
    Vector2 toMouse = UpdateMouseAim();

    // --- Movement ---
    // a movement function?
    Vector2 moveDir = { 0, 0 };
    if (IsKeyDown(KEY_W)) moveDir.y -= 1;
    if (IsKeyDown(KEY_S)) moveDir.y += 1;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1;
    if (IsKeyDown(KEY_D)) moveDir.x += 1;

    float moveLen = Vector2Length(moveDir);
    if (moveLen > 0) moveDir = Vector2Scale(moveDir, 1.0f / moveLen);

    if (!p->dash.active) {
        float moveSpeed = p->speed;
        if (p->primary == WPN_GUN && p->minigun.slowTimer > 0)
            moveSpeed = p->speed * MINIGUN_SLOW_FACTOR;
        if (p->primary == WPN_GUN && p->gun.overheatBoostTimer > 0) {
            moveSpeed *= GUN_OVERHEAT_DASH_BOOST;
            // Speed trail particles while boosted
            if (moveLen > 0) {
                Vector2 trail = { -moveDir.x * GUN_OVERHEAT_TRAIL_SPEED,
                                  -moveDir.y * GUN_OVERHEAT_TRAIL_SPEED };
                SpawnParticle(p->pos, trail,
                    GUN_OVERHEAT_TRAIL_COLOR, GUN_OVERHEAT_TRAIL_SIZE,
                    GUN_OVERHEAT_TRAIL_LIFE);
            }
        }
        if (p->primary == WPN_SNIPER && p->sniper.aiming)
            moveSpeed *= SNIPER_AIM_SLOW;
        if (p->shield.active)
            moveSpeed *= SHIELD_SLOW_FACTOR;
        if (p->flame.active)
            moveSpeed *= FLAME_SLOW_FACTOR;
        p->pos = Vector2Add(p->pos, Vector2Scale(moveDir, moveSpeed * dt));
    }

    // knockback velocity (rocket jump)
    if (Vector2Length(p->vel) > 1.0f) {
        p->pos = Vector2Add(p->pos, Vector2Scale(p->vel, dt));
        float decay = ROCKET_JUMP_FRICTION * dt;
        if (decay > 1.0f) decay = 1.0f;
        p->vel = Vector2Scale(p->vel, 1.0f - decay);
    } else {
        p->vel = (Vector2){ 0, 0 };
    }

    // --- Dash (charge system) ---
    if (p->dash.charges < p->dash.maxCharges) {
        p->dash.rechargeTimer -= dt;
        if (p->dash.rechargeTimer <= 0) {
            p->dash.charges++;
            if (p->dash.charges < p->dash.maxCharges)
                p->dash.rechargeTimer = p->dash.rechargeTime;
        }
    }

    bool spacePressed = IsKeyPressed(KEY_SPACE);

    // Pressed space during active dash = mistimed, no super dash
    if (spacePressed && p->dash.active) {
        p->dash.superMissed = true;
    }

    // Tick super dash window
    if (p->dash.superWindow > 0) p->dash.superWindow -= dt;

    // Tick decoy
    if (p->dash.decoyActive) {
        p->dash.decoyTimer -= dt;
        if (p->dash.decoyTimer <= 0) {
            p->dash.decoyActive = false;
            SpawnParticles(p->dash.decoyPos, SKYBLUE,
                DECOY_EXPIRE_PARTICLES);
        }
    }

    if (spacePressed
        && !p->dash.active
        && p->dash.charges > 0
    ) {
        // Check for super dash timing
        bool isSuperDash = (p->dash.superWindow > 0
            && !p->dash.superMissed);
        if (isSuperDash) {
            p->dash.decoyActive = true;
            p->dash.decoyPos = p->pos;
            p->dash.decoyTimer = DECOY_DURATION;
            SpawnParticles(p->pos, WHITE, DECOY_EXPIRE_PARTICLES);
        }
        p->dash.superWindow = 0;
        p->dash.superMissed = false;

        // Normal dash mechanics
        if (p->primary == WPN_SNIPER) p->sniper.adsDuringDash = false;
        p->dash.active = true;
        p->dash.timer = p->dash.duration;
        bool wasFull = (p->dash.charges == p->dash.maxCharges);
        p->dash.charges--;
        if (wasFull)
            p->dash.rechargeTimer = p->dash.rechargeTime;
        p->iFrames = p->dash.duration;
        if (moveLen > 0) {
            p->dash.dir = moveDir;
        } else {
            p->dash.dir = (Vector2){ cosf(p->angle), sinf(p->angle) };
        }
        SpawnParticles(p->pos, SKYBLUE, DASH_BURST_PARTICLES);
        SpawnParticles(p->pos, WHITE, DASH_BURST_PARTICLES);

        // Retroactive dash slash: upgrade mid-swing sword
        // so press order (M1 before Space) doesn't matter
        if (p->sword.timer > 0 && !p->sword.dashSlash) {
            p->sword.dashSlash = true;
            for (int i = 0; i < MAX_ENEMIES; i++)
                p->sword.lastHitAngle[i] = -1000.0f;
        }
    }

    if (p->dash.active) {
        p->dash.timer -= dt;
        p->pos = Vector2Add(
            p->pos, Vector2Scale(p->dash.dir, p->dash.speed * dt));

        float trailAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
        Vector2 trailVel = { cosf(trailAngle) * DASH_TRAIL_SPEED,
                             sinf(trailAngle) * DASH_TRAIL_SPEED };
        SpawnParticle(p->pos, trailVel, SKYBLUE,
            DASH_TRAIL_SIZE, DASH_TRAIL_LIFETIME);

        // Sniper: track M2 pressed (not held) during dash
        if (p->primary == WPN_SNIPER && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            p->sniper.adsDuringDash = true;

        if (p->dash.timer <= 0) {
            p->dash.active = false;
            // Open super dash window
            p->dash.superWindow = DASH_SUPER_WINDOW;
            p->dash.superMissed = false;
            // Sniper: only if M2 was pressed during the dash
            if (p->primary == WPN_SNIPER && p->sniper.adsDuringDash) {
                p->sniper.superShotReady = true;
                p->sniper.adsDuringDash = false;
            }
        }
    }

    // --- Weapon swap (Ctrl) ---
    if (IsKeyPressed(WEAPON_SWAP_KEY)) {
        WeaponType tmp = p->primary;
        p->primary = p->secondary;
        p->secondary = tmp;
        // Reset active states on swap-out
        p->sniper.aiming = false;
        p->minigun.slowTimer = 0;
        p->revolver.fanning = false;
    }

    // --- Holstered weapon passive timers ---
    if (p->primary != WPN_SNIPER && p->sniper.cooldownTimer > 0)
        p->sniper.cooldownTimer -= dt;
    if (p->primary != WPN_REVOLVER) {
        if (p->revolver.reloadTimer > 0) {
            p->revolver.reloadTimer -= dt;
            if (p->revolver.reloadTimer <= 0) {
                p->revolver.rounds = REVOLVER_ROUNDS;
                p->revolver.reloadTimer = 0;
                p->revolver.reloadLocked = false;
            }
        }
    }
    if (p->primary != WPN_GUN) {
        // Passive heat decay while holstered
        if (p->gun.heat > 0) {
            p->gun.heat -= GUN_HEAT_DECAY * dt;
            if (p->gun.heat <= 0) {
                p->gun.heat = 0;
                p->gun.overheated = false;
            }
        }
        p->minigun.spinUp -= dt / MINIGUN_SPIN_DOWN_TIME;
        if (p->minigun.spinUp < 0) p->minigun.spinUp = 0;
    }

    // --- M1 weapon dispatch ---
    p->gun.cooldown -= dt;
    switch (p->primary) {
    case WPN_GUN: {
        // --- Heat decay ---
        bool firing = false;
        if (p->gun.overheated) {
            // QTE vent: one chance — press R or Dash in the zone
            if (p->gun.ventResult == 0) {
                p->gun.ventCursor += GUN_VENT_CURSOR_SPEED * dt;
                if (p->gun.ventCursor >= 1.0f) {
                    p->gun.ventResult = -1; // missed — no press
                }
                bool m1Pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
                bool dashPressed = IsKeyPressed(KEY_SPACE);
                if (m1Pressed || dashPressed) {
                    float c = p->gun.ventCursor;
                    float zs = p->gun.ventZoneStart;
                    float ze = zs + p->gun.ventZoneWidth;
                    if (c >= zs && c <= ze) {
                        p->gun.ventResult = 1;  // hit
                        if (dashPressed) {
                            p->gun.overheatBoostTimer = GUN_OVERHEAT_BOOST_DUR;
                            // Burst of particles on dash-vent
                            for (int i = 0; i < GUN_VENT_BURST_PARTICLES; i++) {
                                float a = ((float)i / GUN_VENT_BURST_PARTICLES) * PI * 2.0f;
                                Vector2 v = { cosf(a) * GUN_VENT_BURST_SPEED,
                                              sinf(a) * GUN_VENT_BURST_SPEED };
                                SpawnParticle(p->pos, v,
                                    GUN_VENT_BURST_COLOR, GUN_VENT_BURST_SIZE,
                                    GUN_VENT_BURST_LIFETIME);
                            }
                        }
                    } else {
                        p->gun.ventResult = -1; // miss
                    }
                }
            }
            // Decay: fast on hit, normal otherwise
            float decay = GUN_OVERHEAT_DECAY;
            if (p->gun.ventResult == 1)
                decay = GUN_VENT_HIT_DECAY;
            p->gun.heat -= decay * dt;
            // Vent particles on hit
            if (p->gun.ventResult == 1) {
                for (int i = 0; i < 2; i++) {
                    float vAngle = -PI/2.0f + ((float)GetRandomValue(-30, 30) * 0.01f);
                    Vector2 vVel = { cosf(vAngle) * GUN_VENT_STEAM_SPEED,
                                     sinf(vAngle) * GUN_VENT_STEAM_SPEED };
                    SpawnParticle(p->pos, vVel,
                        GUN_VENT_STEAM_COLOR, GUN_VENT_STEAM_SIZE,
                        GUN_VENT_STEAM_LIFETIME);
                }
            }
            if (p->gun.heat <= GUN_OVERHEAT_CLEAR) {
                p->gun.heat = 0;
                p->gun.overheated = false;
            }
            // Force minigun spin down while overheated
            p->minigun.spinUp -= dt / MINIGUN_SPIN_DOWN_TIME;
            if (p->minigun.spinUp < 0) p->minigun.spinUp = 0;
        }

        // --- M2: Minigun mode — spin-up + high volume fire, slows movement ---
        p->minigun.cooldown -= dt;
        if (!p->gun.overheated && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            p->minigun.slowTimer = MINIGUN_SLOW_LINGER;
            p->minigun.spinUp += dt / MINIGUN_SPIN_UP_TIME;
            if (p->minigun.spinUp > 1.0f) p->minigun.spinUp = 1.0f;

            float rate = MINIGUN_MIN_FIRE_RATE +
                (MINIGUN_MAX_FIRE_RATE - MINIGUN_MIN_FIRE_RATE) * p->minigun.spinUp;

            if (p->minigun.cooldown <= 0 && p->minigun.spinUp > 0.1f) {
                p->minigun.cooldown = 1.0f / rate;
                int spread = MINIGUN_SPREAD_MIN +
                    (int)((MINIGUN_SPREAD_MAX - MINIGUN_SPREAD_MIN) * p->minigun.spinUp);
                float s = ((float)GetRandomValue(-spread, spread)) * 0.001f;
                float bulletAngle = p->angle + s;
                Vector2 bulletDir = { cosf(bulletAngle), sinf(bulletAngle) };
                Vector2 aimDir = Vector2Normalize(toMouse);
                Vector2 muzzle =
                    Vector2Add(p->pos, Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
                SpawnProjectile(muzzle, bulletDir,
                    MINIGUN_BULLET_SPEED, MINIGUN_BULLET_DAMAGE,
                    MINIGUN_BULLET_LIFETIME, MINIGUN_PROJECTILE_SIZE, false, false,
                    PROJ_BULLET, DMG_BALLISTIC);
                SpawnParticle(muzzle,
                    Vector2Scale(bulletDir, MINIGUN_MUZZLE_SPEED), WHITE,
                    MINIGUN_MUZZLE_SIZE, MINIGUN_MUZZLE_LIFETIME);
                // Heat accumulation (M2)
                p->gun.heat += MINIGUN_HEAT_PER_SHOT;
                p->gun.heatDecayWait = 0;
                firing = true;
            }
        } else if (!p->gun.overheated) {
            p->minigun.slowTimer -= dt;
            if (p->minigun.slowTimer < 0) p->minigun.slowTimer = 0;
            p->minigun.spinUp -= dt / MINIGUN_SPIN_DOWN_TIME;
            if (p->minigun.spinUp < 0) p->minigun.spinUp = 0;

            // M1: Normal machine gun (only when not spinning minigun)
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)
                && p->gun.cooldown <= 0
                && p->sword.timer <= 0
            ) {
                p->gun.cooldown = 1.0f / p->gun.fireRate;
                Vector2 aimDir = Vector2Normalize(toMouse);
                float spread = ((float)GetRandomValue(-GUN_SPREAD, GUN_SPREAD)) * 0.001f;
                float bulletAngle = p->angle + spread;
                Vector2 bulletDir = { cosf(bulletAngle), sinf(bulletAngle) };
                Vector2 muzzle =
                    Vector2Add(p->pos, Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
                SpawnProjectile(muzzle, bulletDir,
                    GUN_BULLET_SPEED, GUN_BULLET_DAMAGE,
                    GUN_BULLET_LIFETIME, GUN_PROJECTILE_SIZE, false, false,
                    PROJ_BULLET, DMG_BALLISTIC);
                SpawnParticle(muzzle,
                              Vector2Scale(bulletDir, GUN_MUZZLE_SPEED), WHITE,
                              GUN_MUZZLE_SIZE, GUN_MUZZLE_LIFETIME);
                // Heat accumulation (M1)
                p->gun.heat += GUN_HEAT_PER_SHOT;
                p->gun.heatDecayWait = 0;
                firing = true;
            }
        }

        // Check overheat trigger — start QTE
        if (p->gun.heat >= GUN_OVERHEAT_THRESHOLD) {
            p->gun.heat = 1.0f;
            p->gun.overheated = true;
            p->gun.ventCursor = 0;
            p->gun.ventResult = 0;
            float range = GUN_VENT_ZONE_MAX - GUN_VENT_ZONE_MIN;
            p->gun.ventZoneStart = GUN_VENT_ZONE_MIN +
                ((float)GetRandomValue(0, 1000) * 0.001f) * range;
            p->gun.ventZoneWidth = GUN_VENT_ZONE_WIDTH;
        }

        // Passive heat decay: only after not firing for GUN_HEAT_DECAY_DELAY
        if (!p->gun.overheated && !firing) {
            p->gun.heatDecayWait += dt;
            if (p->gun.heatDecayWait >= GUN_HEAT_DECAY_DELAY) {
                p->gun.heat -= GUN_HEAT_DECAY * dt;
                if (p->gun.heat < 0) p->gun.heat = 0;
            }
        }

        // Overheat boost timer countdown (persists after overheat clears)
        if (p->gun.overheatBoostTimer > 0)
            p->gun.overheatBoostTimer -= dt;
    } break;
#if 0 // laser primary — preserved for future use
    case WPN_LASER:
        p->laser.active = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        if (p->laser.active) {
            Vector2 aimDir = Vector2Normalize(toMouse);
            Vector2 muzzle = Vector2Add(p->pos,
                Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
            p->laser.damageAccum += LASER_DPS * dt;
            int dmg = (int)p->laser.damageAccum;
            if (dmg > 0) p->laser.damageAccum -= (float)dmg;
            p->laser.beamTip = FireHitscan(muzzle, aimDir,
                LASER_RANGE, dmg, DMG_BALLISTIC, 1);
        } else {
            p->laser.damageAccum = 0;
        }
        break;
#endif
    case WPN_SNIPER: {
        if (p->sniper.cooldownTimer > 0)
            p->sniper.cooldownTimer -= dt;

        // ADS state from M2
        p->sniper.aiming = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

        // M1 fires — behavior depends on aiming state
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && p->sniper.cooldownTimer <= 0) {
            Vector2 aimDir = Vector2Normalize(toMouse);
            int spreadVal;
            float speed, cooldown;
            int damage;

            if (p->sniper.aiming) {
                damage  = p->sniper.superShotReady ? SNIPER_SUPER_DAMAGE : SNIPER_AIM_DAMAGE;
                spreadVal = SNIPER_AIM_SPREAD;
                speed   = SNIPER_AIM_BULLET_SPEED;
                cooldown = SNIPER_AIM_COOLDOWN;
            } else {
                damage  = SNIPER_HIP_DAMAGE;
                spreadVal = SNIPER_HIP_SPREAD;
                speed   = SNIPER_HIP_BULLET_SPEED;
                cooldown = SNIPER_HIP_COOLDOWN;
            }

            float spread = (float)(GetRandomValue(-spreadVal, spreadVal)) / 1000.0f;
            Vector2 dir = Vector2Rotate(aimDir, spread);
            Vector2 muzzle = Vector2Add(p->pos,
                Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));

            Projectile *sn = SpawnProjectile(muzzle, dir,
                speed, damage,
                SNIPER_BULLET_LIFETIME, SNIPER_PROJECTILE_SIZE, false, false,
                PROJ_BULLET, DMG_PIERCE);
            if (sn) sn->appliesSlow = true;
            p->sniper.cooldownTimer = cooldown;

            // Muzzle flash — gold burst for super shot
            if (p->sniper.superShotReady) {
                SpawnParticles(muzzle, GOLD, SNIPER_MUZZLE_PARTICLES * 2);
                SpawnParticle(muzzle,
                    Vector2Scale(aimDir, SNIPER_MUZZLE_SPEED), GOLD,
                    SNIPER_MUZZLE_SIZE * 1.5f, SNIPER_MUZZLE_LIFETIME);
                p->sniper.superShotReady = false;
            } else {
                SpawnParticles(muzzle, (Color)SNIPER_COLOR, SNIPER_MUZZLE_PARTICLES);
                SpawnParticle(muzzle,
                    Vector2Scale(aimDir, SNIPER_MUZZLE_SPEED), WHITE,
                    SNIPER_MUZZLE_SIZE, SNIPER_MUZZLE_LIFETIME);
            }
        }

        // Clear super shot if not aiming
        if (!p->sniper.aiming) p->sniper.superShotReady = false;
        break;
    }
    case WPN_SWORD:
        // M1: Sweep
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
            && p->sword.timer <= 0
            && p->spin.timer <= 0
        ) {
            p->sword.timer = p->sword.duration;
            p->sword.angle = p->angle;
            p->sword.lunge = false;
            p->sword.dashSlash = p->dash.active;
            for (int i = 0; i < MAX_ENEMIES; i++)
                p->sword.lastHitAngle[i] = -1000.0f;

            float arc = p->sword.arc;
            float radius = p->sword.radius;
            for (int i = 0; i < SWORD_SPARK_COUNT; i++) {
                float a = p->sword.angle - arc / 2.0f
                    + arc * (float)i / (SWORD_SPARK_COUNT - 1);
                Vector2 particlePos = Vector2Add(
                    p->pos,
                    (Vector2){ cosf(a) * radius * SWORD_SPARK_RADIUS_FRAC,
                               sinf(a) * radius * SWORD_SPARK_RADIUS_FRAC }
                );
                Vector2 particleVel = { cosf(a) * SWORD_SPARK_SPEED,
                                        sinf(a) * SWORD_SPARK_SPEED };
                SpawnParticle(particlePos, particleVel, ORANGE,
                    SWORD_SPARK_SIZE, SWORD_SPARK_LIFETIME);
            }
        }
        // M2: Lunge
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
            && p->sword.timer <= 0
            && p->spin.timer <= 0
        ) {
            p->sword.timer = LUNGE_DURATION;
            p->sword.angle = p->angle;
            p->sword.lunge = true;
            p->sword.dashSlash = p->dash.active;
            for (int i = 0; i < MAX_ENEMIES; i++)
                p->sword.lastHitAngle[i] = -1000.0f;

            // Spark burst along thrust line
            float range = p->dash.active ?
                LUNGE_RANGE * LUNGE_DASH_RANGE_MULT : LUNGE_RANGE;
            for (int i = 0; i < SWORD_SPARK_COUNT; i++) {
                float t = (float)(i + 1) / SWORD_SPARK_COUNT;
                Vector2 particlePos = Vector2Add(p->pos,
                    (Vector2){ cosf(p->angle) * range * t * SWORD_SPARK_RADIUS_FRAC,
                               sinf(p->angle) * range * t * SWORD_SPARK_RADIUS_FRAC });
                Vector2 particleVel = { cosf(p->angle) * SWORD_SPARK_SPEED,
                                        sinf(p->angle) * SWORD_SPARK_SPEED };
                SpawnParticle(particlePos, particleVel, RED,
                    SWORD_SPARK_SIZE, SWORD_SPARK_LIFETIME);
            }
        }
        break;
    case WPN_REVOLVER:
        // Reload
        if (p->revolver.reloadTimer > 0) {
            float progress = 1.0f - (p->revolver.reloadTimer / REVOLVER_RELOAD_TIME);
            // Active reload: M1 or Space during reload
            bool reloadInput = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE);
            if (reloadInput && !p->revolver.reloadLocked) {
                bool inSweet = progress >= REVOLVER_RELOAD_SWEET_START
                    && progress <= REVOLVER_RELOAD_SWEET_END;
                if (inSweet) {
                    p->revolver.reloadTimer = REVOLVER_RELOAD_FAST_TIME;
                    // Dash reload — next cylinder does double damage
                    if (IsKeyPressed(KEY_SPACE))
                        p->revolver.bonusRounds = REVOLVER_ROUNDS;
                } else {
                    p->revolver.reloadTimer += REVOLVER_RELOAD_FAIL_PENALTY;
                }
                p->revolver.reloadLocked = true;
            }
            p->revolver.reloadTimer -= dt;
            if (p->revolver.reloadTimer <= 0) {
                p->revolver.rounds = REVOLVER_ROUNDS;
                p->revolver.reloadTimer = 0;
                p->revolver.reloadLocked = false;
            }
            break;
        }
        // M2 fan the hammer — commits to emptying the cylinder
        if (p->revolver.fanning) {
            p->revolver.cooldownTimer -= dt;
            if (p->revolver.cooldownTimer <= 0 && p->revolver.rounds > 0) {
                p->revolver.cooldownTimer = REVOLVER_FAN_COOLDOWN;
                int dmg = REVOLVER_DAMAGE;
                bool bonus = p->revolver.bonusRounds > 0;
                if (bonus) { dmg *= 2; p->revolver.bonusRounds--; }
                Vector2 aimDir = Vector2Normalize(toMouse);
                float spread = ((float)GetRandomValue(-REVOLVER_FAN_SPREAD, REVOLVER_FAN_SPREAD)) * 0.001f;
                float bulletAngle = p->angle + spread;
                Vector2 bulletDir = { cosf(bulletAngle), sinf(bulletAngle) };
                Vector2 muzzle = Vector2Add(p->pos,
                    Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
                SpawnProjectile(muzzle, bulletDir,
                    REVOLVER_BULLET_SPEED, dmg,
                    REVOLVER_BULLET_LIFETIME, REVOLVER_PROJECTILE_SIZE, false, false,
                    PROJ_BULLET, DMG_BALLISTIC);
                SpawnParticle(muzzle,
                    Vector2Scale(bulletDir, GUN_MUZZLE_SPEED),
                    bonus ? GOLD : ORANGE,
                    GUN_MUZZLE_SIZE, GUN_MUZZLE_LIFETIME);
                p->revolver.rounds--;
            }
            if (p->revolver.rounds <= 0) p->revolver.fanning = false;
            break;
        }
        // M1 precise shot — fires as fast as you click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
            && p->revolver.rounds > 0
        ) {
            int dmg = REVOLVER_DAMAGE;
            bool bonus = p->revolver.bonusRounds > 0;
            if (bonus) { dmg *= 2; p->revolver.bonusRounds--; }
            Vector2 aimDir = Vector2Normalize(toMouse);
            float spread = ((float)GetRandomValue(-REVOLVER_PRECISE_SPREAD, REVOLVER_PRECISE_SPREAD)) * 0.001f;
            float bulletAngle = p->angle + spread;
            Vector2 bulletDir = { cosf(bulletAngle), sinf(bulletAngle) };
            Vector2 muzzle = Vector2Add(p->pos,
                Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
            SpawnProjectile(muzzle, bulletDir,
                REVOLVER_BULLET_SPEED, dmg,
                REVOLVER_BULLET_LIFETIME, REVOLVER_PROJECTILE_SIZE, false, false,
                PROJ_BULLET, DMG_BALLISTIC);
            SpawnParticle(muzzle,
                Vector2Scale(bulletDir, GUN_MUZZLE_SPEED),
                bonus ? GOLD : WHITE,
                GUN_MUZZLE_SIZE, GUN_MUZZLE_LIFETIME);
            p->revolver.rounds--;
        }
        // M2 press starts fanning
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
            && p->revolver.rounds > 0
        ) {
            p->revolver.fanning = true;
            p->revolver.cooldownTimer = 0;
        }
        // Auto-reload when empty
        if (p->revolver.reloadTimer <= 0
            && p->revolver.rounds <= 0) {
            p->revolver.reloadTimer = REVOLVER_RELOAD_TIME;
            p->revolver.reloadLocked = false;
        }
        break;
    case WPN_ROCKET:
        // M1: Rocket (one at a time)
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
            && p->rocket.cooldownTimer <= 0
            && !p->rocket.inFlight) {
            SpawnRocket(p, toMouse);
            p->rocket.cooldownTimer = ROCKET_COOLDOWN;
        }
        // M2: Detonate in-flight rocket
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
            && p->rocket.inFlight) {
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                Projectile *b = &g.projectiles[i];
                if (b->active && b->type == PROJ_ROCKET
                    && !b->isEnemy) {
                    RocketExplode(b->pos);
                    b->active = false;
                    break;
                }
            }
        }
        break;
    default: break;
    }

    // Sword damage — sweep or lunge
    if (p->sword.timer > 0 && !p->sword.lunge) {
        // Sweep: line hits enemies as it passes over them
        float radius =
            p->sword.dashSlash ?
            p->sword.radius * DASH_SLASH_RADIUS_MULT :
            p->sword.radius;
        float arc = p->sword.dashSlash ?
            p->sword.arc * DASH_SLASH_ARC_MULT : p->sword.arc;
        int dmg = p->sword.dashSlash ? SWORD_DASH_DAMAGE : SWORD_DAMAGE;
        float progress = 1.0f - (p->sword.timer / p->sword.duration);
        float sweepAngle =
            p->sword.angle - arc / 2.0f + arc * progress;
        Vector2 sweepEnd = Vector2Add(p->pos,
            (Vector2){ cosf(sweepAngle) * radius,
                       sinf(sweepAngle) * radius });

        int hits[MAX_ENEMIES];
        SweepDamage(p->pos, sweepEnd, sweepAngle,
            dmg, DMG_SLASH, p->sword.lastHitAngle, hits, MAX_ENEMIES);
        p->sword.timer -= dt;
    }
    if (p->sword.timer > 0 && p->sword.lunge) {
        // Lunge: forward thrust, pierce all enemies in cone
        float range = p->sword.dashSlash ?
            LUNGE_RANGE * LUNGE_DASH_RANGE_MULT : LUNGE_RANGE;
        float lungeSpeed = p->sword.dashSlash ?
            LUNGE_SPEED * LUNGE_DASH_SPEED_MULT : LUNGE_SPEED;
        int dmg = p->sword.dashSlash ? LUNGE_DASH_DAMAGE : LUNGE_DAMAGE;

        // Move player forward along committed direction
        Vector2 dir = { cosf(p->sword.angle), sinf(p->sword.angle) };
        p->pos = Vector2Add(p->pos, Vector2Scale(dir, lungeSpeed * dt));

        // Cone damage — pierce all enemies in range
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *ei = &g.enemies[i];
            if (!ei->active) continue;
            if (p->sword.lastHitAngle[i] > -500.0f) continue; // already hit

            Vector2 toEnemy = Vector2Subtract(ei->pos, p->pos);
            float dist = Vector2Length(toEnemy);
            if (dist > range + ei->size) continue;
            float enemyAngle = atan2f(toEnemy.y, toEnemy.x);
            float angleDiff = fabsf(fmodf(enemyAngle - p->sword.angle + PI, 2*PI) - PI);
            if (angleDiff > LUNGE_CONE_HALF) continue;

            DamageEnemy(i, dmg, DMG_PIERCE, HIT_MELEE);
            p->sword.lastHitAngle[i] = 0.0f; // mark as hit
        }
        p->sword.timer -= dt;
    }

    // special
    // --- Spin attack (Shift) ---
    // activation of ability
    p->spin.cooldownTimer -= dt;
    if (p->spin.cooldownTimer < 0) p->spin.cooldownTimer = 0;

    if (IsAbilityPressed(p, ABL_SPIN)
        && p->spin.timer <= 0
        && p->spin.cooldownTimer <= 0
    ) {
        p->spin.timer = p->spin.duration;
        p->spin.cooldownTimer = p->spin.cooldown;
        for (int i = 0; i < MAX_ENEMIES; i++)
            p->spin.lastHitAngle[i] = -1000.0f;

        // animation of the ability at start
        for (int i = 0; i < SPIN_BURST_COUNT; i++) {
            float a = (float)i / (float)SPIN_BURST_COUNT * 2.0f * PI;
            Vector2 particlePos = Vector2Add(
                p->pos,
                (Vector2){
                    cosf(a) * p->spin.radius * SPIN_BURST_INNER_FRAC,
                    sinf(a) * p->spin.radius * SPIN_BURST_INNER_FRAC
                });
            Vector2 particleVel = { cosf(a) * SPIN_BURST_SPEED,
                                    sinf(a) * SPIN_BURST_SPEED };
            SpawnParticle(particlePos, particleVel, YELLOW,
                SPIN_BURST_SIZE, SPIN_BURST_LIFETIME);
        }
    }
    
    // duration of the ability
    // Spin damage — sweep line hits enemies as it passes over them
    if (p->spin.timer > 0) {
        float progress = 1.0f - (p->spin.timer / p->spin.duration);
        float sweepAngle = PI * 4.0f * progress;
        Vector2 sweepEnd = Vector2Add(p->pos,
            (Vector2){ cosf(sweepAngle) * p->spin.radius,
                       sinf(sweepAngle) * p->spin.radius });

        int hits[MAX_ENEMIES];
        int nhits = SweepDamage(p->pos, sweepEnd, sweepAngle,
            SPIN_DAMAGE, DMG_SLASH, p->spin.lastHitAngle, hits, MAX_ENEMIES);
        for (int h = 0; h < nhits; h++) {
            Enemy *ei = &g.enemies[hits[h]];
            float heal = SPIN_DAMAGE * SPIN_LIFESTEAL;
            p->hp = p->hp + heal;
            if (p->hp > p->maxHp) p->hp = p->maxHp;
            SpawnParticles(ei->pos, GREEN,
                (int)(heal * SPIN_HEAL_PARTICLE_MULT));
            Vector2 kb = Vector2Normalize(
                Vector2Subtract(ei->pos, p->pos));
            ei->vel = Vector2Scale(kb, SPIN_KNOCKBACK);
        }

        // deflect enemy bullets inside spin radius
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            Projectile *b = &g.projectiles[i];
            if (!b->active || !b->isEnemy) continue;
            float dist = Vector2Distance(b->pos, p->pos);
            if (dist < p->spin.radius) {
                b->isEnemy = false;
                b->damage *= SPIN_DEFLECT_DAMAGE_MULT;
                Vector2 away = Vector2Normalize(
                    Vector2Subtract(b->pos, p->pos));
                float speed = Vector2Length(b->vel) * SPIN_DEFLECT_SPEED_MULT;
                b->vel = Vector2Scale(away, speed);
                SpawnParticles(b->pos, YELLOW, 4);
            }
        }

        p->spin.timer -= dt;
    }

    // shotgun — two manual blasts, cooldown after both spent
    if (p->shotgun.blastsLeft == 0 && p->shotgun.cooldownTimer > 0) {
        p->shotgun.cooldownTimer -= dt;
        if (p->shotgun.cooldownTimer <= 0) {
            p->shotgun.cooldownTimer = 0;
            p->shotgun.blastsLeft = SHOTGUN_BLASTS;
        }
    }

    if (IsAbilityPressed(p, ABL_SHOTGUN) && p->shotgun.blastsLeft > 0) {
        FireShotgunBlast(p, toMouse);
        p->shotgun.blastsLeft--;
        if (p->shotgun.blastsLeft == 0)
            p->shotgun.cooldownTimer = SHOTGUN_COOLDOWN;
    }

    // rocket launcher cooldown (fired via M1 when WPN_ROCKET primary)
    if (p->rocket.cooldownTimer > 0)
        p->rocket.cooldownTimer -= dt;

    // grenade launcher — arcing explosive, cooldown
    if (p->grenade.cooldownTimer > 0)
        p->grenade.cooldownTimer -= dt;

    if (IsAbilityPressed(p, ABL_GRENADE) && p->grenade.cooldownTimer <= 0) {
        SpawnGrenade(p, toMouse);
        p->grenade.cooldownTimer = GRENADE_COOLDOWN;
    }

    UpdateRailgun(p, toMouse, dt);

    // sniper — primary weapon only, no secondary binding

    // bfg10k — charges from damage dealt, fires when full
    if (IsAbilityPressed(p, ABL_BFG) && p->bfg.charge >= BFG_CHARGE_COST && !p->bfg.active) {
        Vector2 aimDir = Vector2Normalize(toMouse);
        Vector2 muzzle = Vector2Add(p->pos,
            Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
        SpawnProjectile(muzzle, aimDir,
            BFG_SPEED, BFG_DIRECT_DAMAGE,
            BFG_LIFETIME, BFG_PROJECTILE_SIZE, false, false,
            PROJ_BFG, DMG_ABILITY);
        p->bfg.charge = 0;
        p->bfg.active = true;
        // muzzle flash
        SpawnParticles(muzzle, (Color)BFG_COLOR, BFG_MUZZLE_PARTICLES);
        SpawnParticle(muzzle,
            Vector2Scale(aimDir, BFG_MUZZLE_SPEED), WHITE,
            BFG_MUZZLE_SIZE, BFG_MUZZLE_LIFETIME);
    }

    // --- Shield (hold to maintain) ---
    {
        bool wantShield = IsAbilityDown(p, ABL_SHIELD)
            && p->shield.hp > 0
            && p->shield.regenTimer >= 0; // not broken (negative = broken cooldown)
        if (wantShield) {
            p->shield.active = true;
            p->shield.angle = p->angle;
            p->shield.regenTimer = 0;
        } else {
            if (p->shield.active) {
                // just lowered — start regen delay
                p->shield.regenTimer = SHIELD_REGEN_DELAY;
            }
            p->shield.active = false;
            // Regen timer + HP regen
            if (p->shield.regenTimer > 0) {
                p->shield.regenTimer -= dt;
                if (p->shield.regenTimer < 0)
                    p->shield.regenTimer = 0;
            } else if (p->shield.regenTimer >= 0
                && p->shield.hp < p->shield.maxHp) {
                p->shield.hp += SHIELD_REGEN_RATE * dt;
                if (p->shield.hp > p->shield.maxHp)
                    p->shield.hp = p->shield.maxHp;
            }
            // Broken cooldown recovery
            if (p->shield.regenTimer < 0) {
                p->shield.regenTimer += dt;
                if (p->shield.regenTimer >= 0) {
                    p->shield.regenTimer = 0;
                    p->shield.hp = 1.0f; // start regen from 1hp
                }
            }
        }
    }

    // --- Ground Slam (instant AoE + stun) ---
    if (p->slam.cooldownTimer > 0) p->slam.cooldownTimer -= dt;
    if (p->slam.vfxTimer > 0) p->slam.vfxTimer -= dt;

    if (IsAbilityPressed(p, ABL_SLAM) && p->slam.cooldownTimer <= 0) {
        p->slam.cooldownTimer = SLAM_COOLDOWN;
        p->slam.vfxTimer = SLAM_VFX_DURATION;
        p->slam.angle = p->angle;
        float halfArc = SLAM_ARC * 0.5f;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g.enemies[i].active) continue;
            Enemy *ei = &g.enemies[i];
            float dist = Vector2Distance(p->pos, ei->pos);
            if (dist > SLAM_RANGE + ei->size) continue;
            // cone check
            Vector2 toEnemy = Vector2Subtract(ei->pos, p->pos);
            float enemyAngle = atan2f(toEnemy.y, toEnemy.x);
            float diff = fmodf(enemyAngle - p->slam.angle + 3*PI, 2*PI) - PI;
            if (fabsf(diff) > halfArc) continue;

            DamageEnemy(i, SLAM_DAMAGE, DMG_BLUNT, HIT_AOE);
            float t = dist / SLAM_RANGE;
            ei->stunTimer = SLAM_STUN_MAX + (SLAM_STUN_MIN - SLAM_STUN_MAX) * t;
            if (dist > 1.0f)
                ei->vel = Vector2Scale(
                    Vector2Normalize(toEnemy), SLAM_KNOCKBACK);
        }
        // cone particles
        for (int i = 0; i < EXPLOSION_RING_COUNT; i++) {
            float a = p->slam.angle - halfArc
                + (float)i / (float)EXPLOSION_RING_COUNT * SLAM_ARC;
            float speed = (float)GetRandomValue(SLAM_PARTICLE_SPEED_MIN,
                SLAM_PARTICLE_SPEED_MAX);
            Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
            Color c = (i % 2 == 0) ? (Color)SLAM_COLOR : WHITE;
            SpawnParticle(p->pos, vel, c, SLAM_PARTICLE_SIZE,
                SLAM_PARTICLE_LIFETIME);
        }
    }

    // --- Parry (brief deflect window) ---
    if (p->parry.cooldownTimer > 0) p->parry.cooldownTimer -= dt;

    if (IsAbilityPressed(p, ABL_PARRY) && p->parry.cooldownTimer <= 0
        && !p->parry.active) {
        p->parry.active = true;
        p->parry.timer = PARRY_WINDOW;
        p->parry.succeeded = false;
    }

    if (p->parry.active) {
        p->parry.timer -= dt;
        if (p->parry.timer <= 0) {
            p->parry.active = false;
            p->parry.cooldownTimer = p->parry.succeeded
                ? PARRY_SUCCESS_COOLDOWN : PARRY_COOLDOWN;
        }
    }

    // --- Turret deploy ---
    if (p->turretCooldown > 0) p->turretCooldown -= dt;
    if (IsAbilityPressed(p, ABL_TURRET) && p->turretCooldown <= 0) {
        if (CountActiveDeployables(DEPLOY_TURRET) < TURRET_MAX_ACTIVE) {
            float mouseDist = Vector2Length(toMouse);
            float placeDist = (mouseDist < TURRET_PLACEMENT_DIST) ? mouseDist : TURRET_PLACEMENT_DIST;
            Vector2 placePos = (mouseDist > 1.0f)
                ? Vector2Add(p->pos, Vector2Scale(toMouse, placeDist / mouseDist))
                : p->pos;
            SpawnDeployable(DEPLOY_TURRET, placePos);
            p->turretCooldown = TURRET_COOLDOWN;
        }
    }

    // --- Mine deploy ---
    if (p->mineCooldown > 0) p->mineCooldown -= dt;
    if (IsAbilityPressed(p, ABL_MINE) && p->mineCooldown <= 0) {
        if (CountActiveDeployables(DEPLOY_MINE) < MINE_MAX_ACTIVE) {
            SpawnDeployable(DEPLOY_MINE, p->pos);
            p->mineCooldown = MINE_COOLDOWN;
        }
    }

    // --- Healing Field deploy ---
    if (p->healCooldown > 0) p->healCooldown -= dt;
    if (IsAbilityPressed(p, ABL_HEAL) && p->healCooldown <= 0) {
        if (CountActiveDeployables(DEPLOY_HEAL) < HEAL_MAX_ACTIVE) {
            SpawnDeployable(DEPLOY_HEAL, p->pos);
            p->healCooldown = HEAL_COOLDOWN;
        }
    }

    // --- Flamethrower ---
    bool wantFlame = IsAbilityDown(p, ABL_FIRE) && p->flame.fuel > 0;
    if (wantFlame) {
        p->flame.active = true;
        p->flame.fuel -= FLAME_DRAIN_RATE * dt;
        if (p->flame.fuel < 0) p->flame.fuel = 0;
        p->flame.regenDelay = FLAME_REGEN_DELAY;

        // spray patches at interval
        p->flame.sprayTimer -= dt;
        if (p->flame.sprayTimer <= 0) {
            p->flame.sprayTimer = FLAME_SPRAY_INTERVAL;
            float baseAngle = p->angle;
            float spread = ((float)GetRandomValue(-1000, 1000) / 1000.0f) * FLAME_SPREAD;
            float dist = FLAME_RANGE_MIN + (float)GetRandomValue(0, 1000) / 1000.0f
                         * (FLAME_RANGE - FLAME_RANGE_MIN);
            float a = baseAngle + spread;
            Vector2 patchPos = Vector2Add(p->pos,
                (Vector2){ cosf(a) * dist, sinf(a) * dist });
            SpawnFirePatch(patchPos);

            // spray particles along the path
            for (int i = 0; i < 3; i++) {
                float pa = baseAngle + ((float)GetRandomValue(-1000, 1000) / 1000.0f) * FLAME_SPREAD;
                float pd = (float)GetRandomValue(200, 600) / 10.0f;
                Vector2 ppos = Vector2Add(p->pos,
                    (Vector2){ cosf(pa) * pd, sinf(pa) * pd });
                Vector2 pvel = { cosf(pa) * FLAME_PARTICLE_SPEED,
                                 sinf(pa) * FLAME_PARTICLE_SPEED };
                Color c = (GetRandomValue(0, 1) == 0) ? ORANGE : YELLOW;
                SpawnParticle(ppos, pvel, c, FLAME_PARTICLE_SIZE,
                    FLAME_PARTICLE_LIFETIME);
            }
        }
    } else {
        p->flame.active = false;
        // regen fuel after delay
        if (p->flame.regenDelay > 0) {
            p->flame.regenDelay -= dt;
        } else if (p->flame.fuel < FLAME_FUEL_MAX) {
            p->flame.fuel += FLAME_REGEN_RATE * dt;
            if (p->flame.fuel > FLAME_FUEL_MAX)
                p->flame.fuel = FLAME_FUEL_MAX;
        }
    }

    // don't let player p leave map boundary
    if (p->pos.x < p->size) p->pos.x = p->size;
    if (p->pos.y < p->size) p->pos.y = p->size;
    if (p->pos.x > MAP_SIZE - p->size) p->pos.x = MAP_SIZE - p->size;
    if (p->pos.y > MAP_SIZE - p->size) p->pos.y = MAP_SIZE - p->size;

    // Shadow lag — anchored during decoy, lerp otherwise
    if (p->dash.decoyActive) {
        p->shadowPos = p->dash.decoyPos;
    } else {
        p->shadowPos = Vector2Lerp(
            p->shadowPos, p->pos, SHADOW_LAG * dt);
    }

    // ok this is an important piece of gamefeel we need to get right
    // this means iframes after taking damage
    // --- iFrames ---
    if (p->iFrames > 0) p->iFrames -= dt;

}

// enemy shoot functions --------------------------------------------------- /
void ShootRect(Enemy *e, Vector2 toTarget, float dist, float dt) {
    e->shootTimer -= dt;
    if (e->shootTimer <= 0 && dist > 1.0f) {
        e->shootTimer = RECT_SHOOT_INTERVAL;
        Vector2 shootDir = Vector2Scale(toTarget, 1.0f / dist);
        Vector2 muzzle = Vector2Add(e->pos,
            Vector2Scale(shootDir, e->size + MUZZLE_OFFSET));
        SpawnProjectile(muzzle, shootDir, RECT_BULLET_SPEED,
            RECT_BULLET_DAMAGE, RECT_BULLET_LIFETIME,
            RECT_PROJECTILE_SIZE, true, false,
            PROJ_BULLET, DMG_BALLISTIC);
        SpawnParticle(muzzle,
            Vector2Scale(shootDir, ENEMY_MUZZLE_SPEED),
            MAGENTA, ENEMY_MUZZLE_SIZE, ENEMY_MUZZLE_LIFETIME);
    }
}

void ShootPenta(Enemy *e, Vector2 toTarget, float dist, float dt) {
    e->shootTimer -= dt;
    if (e->shootTimer <= 0 && dist > 1.0f) {
        e->shootTimer = PENTA_SHOOT_INTERVAL;
        Vector2 shootDir = Vector2Scale(toTarget, 1.0f / dist);
        Vector2 perp = { -shootDir.y, shootDir.x };

        for (int r = -1; r <= 1; r += 2) {
            Vector2 rowOff = Vector2Scale(perp,
                PENTA_ROW_OFFSET * r);
            for (int b = 0; b < PENTA_BULLETS_PER_ROW; b++) {
                Vector2 bpos = Vector2Add(e->pos, rowOff);
                bpos = Vector2Add(bpos,
                    Vector2Scale(shootDir,
                        e->size + MUZZLE_OFFSET
                        + b * PENTA_BULLET_SPACING));
                SpawnProjectile(bpos, shootDir,
                    PENTA_BULLET_SPEED, PENTA_BULLET_DAMAGE,
                    PENTA_BULLET_LIFETIME, PENTA_PROJECTILE_SIZE,
                    true, false, PROJ_BULLET, DMG_BALLISTIC);
            }
        }
        Vector2 muzzle = Vector2Add(e->pos,
            Vector2Scale(shootDir, e->size + MUZZLE_OFFSET));
        SpawnParticle(muzzle,
            Vector2Scale(shootDir, ENEMY_MUZZLE_SPEED),
            PENTA_COLOR, PENTA_MUZZLE_SIZE, PENTA_MUZZLE_LIFETIME);
        SpawnParticle(muzzle,
            Vector2Scale(perp, PENTA_SIDE_SPEED),
            PENTA_COLOR, ENEMY_MUZZLE_SIZE, ENEMY_MUZZLE_LIFETIME);
        SpawnParticle(muzzle,
            Vector2Scale(perp, -PENTA_SIDE_SPEED),
            PENTA_COLOR, ENEMY_MUZZLE_SIZE, ENEMY_MUZZLE_LIFETIME);
    }
}

void ShootHexa(Enemy *e, Vector2 toTarget, float dist, float dt) {
    e->shootTimer -= dt;
    if (e->shootTimer <= 0 && dist > 1.0f) {
        e->shootTimer = HEXA_SHOOT_INTERVAL;
        Vector2 shootDir = Vector2Scale(toTarget, 1.0f / dist);
        float baseAngle = atan2f(shootDir.y, shootDir.x);
        float halfSpread = HEXA_FAN_SPREAD / 2.0f;
        float step = HEXA_FAN_SPREAD / (HEXA_FAN_COUNT - 1);

        for (int b = 0; b < HEXA_FAN_COUNT; b++) {
            float angle = baseAngle - halfSpread + step * b;
            Vector2 dir = { cosf(angle), sinf(angle) };
            Vector2 muzzle = Vector2Add(e->pos,
                Vector2Scale(dir, e->size + MUZZLE_OFFSET));
            SpawnProjectile(muzzle, dir, HEXA_BULLET_SPEED,
                HEXA_BULLET_DAMAGE, HEXA_BULLET_LIFETIME,
                HEXA_PROJECTILE_SIZE, true, false,
                PROJ_BULLET, DMG_BALLISTIC);
        }
        Vector2 muzzle = Vector2Add(e->pos,
            Vector2Scale(shootDir, e->size + MUZZLE_OFFSET));
        SpawnParticle(muzzle,
            Vector2Scale(shootDir, ENEMY_MUZZLE_SPEED),
            HEXA_COLOR, HEXA_MUZZLE_SIZE, HEXA_MUZZLE_LIFETIME);
    }
}

void ShootTrap(Enemy *e, Vector2 toTarget, float dist, float dt) {
    if (e->chargeTimer > 0) return;
    e->shootTimer -= dt;
    if (e->shootTimer <= 0 && dist > 1.0f) {
        int pattern = e->attackPhase % 3;
        switch (pattern) {
        case 0: { // Aimed burst — cone at player
            Vector2 shootDir = Vector2Scale(
                toTarget, 1.0f / dist);
            float baseAngle = atan2f(shootDir.y, shootDir.x);
            float halfSpread = TRAP_BURST_SPREAD / 2.0f;
            float step = TRAP_BURST_SPREAD
                / (TRAP_BURST_COUNT - 1);
            for (int b = 0; b < TRAP_BURST_COUNT; b++) {
                float a = baseAngle - halfSpread + step * b;
                Vector2 dir = { cosf(a), sinf(a) };
                Vector2 muzzle = Vector2Add(e->pos,
                    Vector2Scale(dir, e->size + MUZZLE_OFFSET));
                SpawnProjectile(muzzle, dir,
                    TRAP_BULLET_SPEED, TRAP_BULLET_DAMAGE,
                    TRAP_BULLET_LIFETIME, TRAP_PROJECTILE_SIZE,
                    true, false, PROJ_BULLET, DMG_BALLISTIC);
            }
            Vector2 muzzle = Vector2Add(e->pos,
                Vector2Scale(shootDir,
                    e->size + MUZZLE_OFFSET));
            SpawnParticle(muzzle,
                Vector2Scale(shootDir, ENEMY_MUZZLE_SPEED),
                TRAP_COLOR, PENTA_MUZZLE_SIZE,
                PENTA_MUZZLE_LIFETIME);
        } break;
        case 1: { // Ring shot — bullets in all directions
            float step = 2.0f * PI / TRAP_RING_COUNT;
            for (int b = 0; b < TRAP_RING_COUNT; b++) {
                float a = step * b;
                Vector2 dir = { cosf(a), sinf(a) };
                Vector2 muzzle = Vector2Add(e->pos,
                    Vector2Scale(dir, e->size + MUZZLE_OFFSET));
                SpawnProjectile(muzzle, dir,
                    TRAP_RING_SPEED, TRAP_RING_DAMAGE,
                    TRAP_BULLET_LIFETIME, TRAP_PROJECTILE_SIZE,
                    true, false, PROJ_BULLET, DMG_BALLISTIC);
            }
            SpawnParticles(e->pos, TRAP_COLOR, 12);
        } break;
        case 2: { // Charge at player
            Vector2 shootDir = Vector2Scale(
                toTarget, 1.0f / dist);
            e->chargeDir = shootDir;
            e->chargeTimer = TRAP_CHARGE_DURATION;
        } break;
        }
        e->attackPhase++;
        e->shootTimer = TRAP_ATTACK_INTERVAL;
    }
}

// enemy AI
static void UpdateEnemies(float dt) {
    Player *p = &g.player;
    
    // --- Boss phase state machine ---
    if (g.phase == 0
        && g.enemiesKilled >= TRAP_SPAWN_KILLS * g.level) {
        g.phase = 1;
    }
    if (g.phase == 1) {
        bool anyAlive = false;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (g.enemies[i].active) { anyAlive = true; break; }
        }
        if (!anyAlive) {
            // Spawn TRAP at map edge
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!g.enemies[i].active) {
                    Enemy *boss = &g.enemies[i];
                    boss->active = true;
                    boss->type = TRAP;
                    boss->size = TRAP_SIZE;
                    boss->speed = TRAP_SPEED_MIN;
                    boss->hp = TRAP_HP;
                    boss->maxHp = TRAP_HP;
                    boss->contactDamage = TRAP_CONTACT_DAMAGE;
                    boss->score = TRAP_SCORE;
                    boss->hitFlash = 0;
                    boss->shootTimer = TRAP_ATTACK_INTERVAL;
                    boss->slowTimer = 0;
                    boss->slowFactor = 1.0f;
                    boss->rootTimer = 0;
                    boss->stunTimer = 0;
                    boss->aggroIdx = -1;
                    boss->attackPhase = 0;
                    boss->chargeTimer = 0;
                    boss->chargeDir = (Vector2){ 0, 0 };
                    boss->vel = (Vector2){ 0, 0 };
                    int edge = GetRandomValue(0, 3);
                    switch (edge) {
                    case 0: boss->pos = (Vector2){
                        (float)GetRandomValue(0, MAP_SIZE),
                        p->pos.y - SPAWN_MARGIN }; break;
                    case 1: boss->pos = (Vector2){
                        (float)GetRandomValue(0, MAP_SIZE),
                        p->pos.y + SPAWN_MARGIN }; break;
                    case 2: boss->pos = (Vector2){
                        p->pos.x - SPAWN_MARGIN,
                        (float)GetRandomValue(0, MAP_SIZE) }; break;
                    case 3: boss->pos = (Vector2){
                        p->pos.x + SPAWN_MARGIN,
                        (float)GetRandomValue(0, MAP_SIZE) }; break;
                    }
                    if (boss->pos.x < 0) boss->pos.x = 0;
                    if (boss->pos.x > MAP_SIZE) boss->pos.x = MAP_SIZE;
                    if (boss->pos.y < 0) boss->pos.y = 0;
                    if (boss->pos.y > MAP_SIZE) boss->pos.y = MAP_SIZE;
                    break;
                }
            }
            g.phase = 2;
        }
    }

    // --- Normal enemy spawning (phase 0 and 2) ---
    if (g.phase != 1) {
        g.spawnTimer -= dt;
        if (g.spawnTimer <= 0) {
            SpawnEnemy();
            g.spawnTimer = g.spawnInterval;
            if (g.spawnInterval > SPAWN_MIN_INTERVAL)
                g.spawnInterval *= SPAWN_RAMP;
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g.enemies[i];
        if (!e->active) continue;

        // tick debuffs
        if (e->slowTimer > 0) e->slowTimer -= dt;
        if (e->rootTimer > 0) e->rootTimer -= dt;
        if (e->stunTimer > 0) e->stunTimer -= dt;

        bool rooted = e->rootTimer > 0 || e->stunTimer > 0;

        // Clear aggro if target deployable is gone
        if (e->aggroIdx >= 0) {
            Deployable *ad = &g.deployables[e->aggroIdx];
            if (!ad->active || ad->type != DEPLOY_TURRET)
                e->aggroIdx = -1;
        }

        // Chase target: turret if aggroed, otherwise player shadow
        Vector2 target = (e->aggroIdx >= 0)
            ? g.deployables[e->aggroIdx].pos : p->shadowPos;
        Vector2 toTarget = Vector2Subtract(target, e->pos);
        float dist = Vector2Length(toTarget);
        // toPlayer still needed for shooting AI
        Vector2 toPlayer = Vector2Subtract(p->shadowPos, e->pos);
        float distToPlayer = Vector2Length(toPlayer);
        // TRAP charge movement — committed direction, no lerp
        if (e->type == TRAP && e->chargeTimer > 0) {
            e->chargeTimer -= dt;
            e->vel = Vector2Scale(e->chargeDir, TRAP_CHARGE_SPEED);
            if (e->chargeTimer <= 0) {
                // Slam AoE at end of charge
                float playerDist = Vector2Distance(e->pos, p->pos);
                if (playerDist <= TRAP_SLAM_RADIUS) {
                    DamagePlayer(TRAP_SLAM_DAMAGE, DMG_BLUNT, HIT_AOE);
                }
                SpawnParticles(e->pos, TRAP_COLOR, 16);
                for (int v = 0; v < MAX_EXPLOSIVES; v++) {
                    if (!g.vfx.explosives[v].active) {
                        g.vfx.explosives[v].active = true;
                        g.vfx.explosives[v].pos = e->pos;
                        g.vfx.explosives[v].timer = EXPLOSION_VFX_DURATION;
                        g.vfx.explosives[v].duration = EXPLOSION_VFX_DURATION;
                        break;
                    }
                }
            }
        } else if (!rooted && dist > 1.0f) {
            Vector2 chaseDir = Vector2Scale(toTarget, 1.0f / dist);
            Vector2 desired;
            if (e->type == HEXA && e->aggroIdx < 0) {
                float strafeDir = (i % 2 == 0) ? 1.0f : -1.0f;
                Vector2 perp = { -chaseDir.y * strafeDir,
                                  chaseDir.x * strafeDir };
                float radial = (dist - HEXA_ORBIT_DIST) / HEXA_ORBIT_DIST;
                if (radial > 1.0f) radial = 1.0f;
                if (radial < -1.0f) radial = -1.0f;
                desired = Vector2Add(
                    Vector2Scale(chaseDir, e->speed * radial),
                    Vector2Scale(perp, e->speed));
                float dLen = Vector2Length(desired);
                if (dLen > 0.01f)
                    desired = Vector2Scale(desired, e->speed / dLen);
            } else {
                desired = Vector2Scale(chaseDir, e->speed);
            }
            // apply slow debuff
            if (e->slowTimer > 0)
                desired = Vector2Scale(desired, e->slowFactor);
            e->vel = Vector2Lerp(e->vel, desired, ENEMY_VEL_LERP_RATE * dt);
        } else if (rooted) {
            e->vel = (Vector2){ 0, 0 };
        }
        e->pos = Vector2Add(e->pos, Vector2Scale(e->vel, dt));

        // Clamp to map
        if (e->pos.x < 0) e->pos.x = 0;
        if (e->pos.x > MAP_SIZE) e->pos.x = MAP_SIZE;
        if (e->pos.y < 0) e->pos.y = 0;
        if (e->pos.y > MAP_SIZE) e->pos.y = MAP_SIZE;

        bool stunned = e->stunTimer > 0;

        // Shoot target: turret if aggroed, otherwise player
        Vector2 toShoot = (e->aggroIdx >= 0) ? toTarget : toPlayer;
        float distToShoot = (e->aggroIdx >= 0) ? dist : distToPlayer;

        // Shoot dispatch
        if (!stunned && ENEMY_DEFS[e->type].shoot)
            ENEMY_DEFS[e->type].shoot(e, toShoot, distToShoot, dt);

        // Hit flash decay
        if (e->hitFlash > 0) e->hitFlash -= dt;

        // Collide with player
        bool contact = EnemyHitCircle(e, p->pos, p->size);
        if (contact && p->iFrames <= 0) {
            // Parry stuns enemy and prevents damage
            if (p->parry.active) {
                e->stunTimer = PARRY_STUN_DURATION;
                p->parry.succeeded = true;
                SpawnParticles(e->pos, WHITE, 8);
                if (distToPlayer > 1.0f) {
                    Vector2 kb = Vector2Scale(
                        Vector2Normalize(Vector2Subtract(e->pos, p->pos)),
                        PARRY_KNOCKBACK);
                    e->vel = kb;
                }
            } else {
                DamagePlayer(e->contactDamage, DMG_BLUNT, HIT_MELEE);
                // Knockback enemy
                if (distToPlayer > 1.0f) {
                    Vector2 kb = Vector2Scale(Vector2Normalize(Vector2Subtract(e->pos, p->pos)), ENEMY_CONTACT_KNOCKBACK);
                    e->vel = kb;
                }
            }
        }

        // Collide with aggroed turret
        if (e->aggroIdx >= 0) {
            Deployable *ad = &g.deployables[e->aggroIdx];
            float turretDist = Vector2Distance(e->pos, ad->pos);
            if (turretDist < e->size + 10.0f) {
                ad->hp -= e->contactDamage;
                if (ad->hp <= 0) {
                    ad->active = false;
                    SpawnParticles(ad->pos, (Color)TURRET_COLOR, 12);
                    e->aggroIdx = -1;
                }
                // Knockback enemy off turret
                if (turretDist > 1.0f) {
                    Vector2 kb = Vector2Scale(
                        Vector2Normalize(Vector2Subtract(e->pos, ad->pos)),
                        ENEMY_CONTACT_KNOCKBACK);
                    e->vel = kb;
                }
            }
        }
    }
}

static void SpawnExplosionVfx(Vector2 pos, Color c1, Color c2, Color c3, Color fireColor) {
    // explosion ring — fast outward burst
    for (int i = 0; i < EXPLOSION_RING_COUNT; i++) {
        float a = (float)i / (float)EXPLOSION_RING_COUNT * 2.0f * PI;
        float speed = (float)GetRandomValue(
            EXPLOSION_RING_SPEED_MIN, EXPLOSION_RING_SPEED_MAX);
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        Color c = (i % 3 == 0) ? c1 : (i % 3 == 1) ? c2 : c3;
        SpawnParticle(pos, vel, c, EXPLOSION_RING_SIZE,
            EXPLOSION_RING_LIFETIME);
    }
    // inner fireball — slower, bigger
    for (int i = 0; i < EXPLOSION_FIRE_COUNT; i++) {
        float a = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(
            EXPLOSION_FIRE_SPEED_MIN, EXPLOSION_FIRE_SPEED_MAX);
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        SpawnParticle(pos, vel, fireColor, EXPLOSION_FIRE_SIZE,
            EXPLOSION_FIRE_LIFETIME);
    }
    // smoke — slow drift outward
    for (int i = 0; i < EXPLOSION_SMOKE_COUNT; i++) {
        float a = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(
            EXPLOSION_SMOKE_SPEED_MIN, EXPLOSION_SMOKE_SPEED_MAX);
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        SpawnParticle(pos, vel, GRAY, EXPLOSION_SMOKE_SIZE,
            EXPLOSION_SMOKE_LIFETIME);
    }
    // spawn radius ring
    for (int i = 0; i < MAX_EXPLOSIVES; i++) {
        if (!g.vfx.explosives[i].active) {
            g.vfx.explosives[i].active = true;
            g.vfx.explosives[i].pos = pos;
            g.vfx.explosives[i].timer = EXPLOSION_VFX_DURATION;
            g.vfx.explosives[i].duration = EXPLOSION_VFX_DURATION;
            break;
        }
    }
}

static void RocketExplode(Vector2 pos) {
    Player *p = &g.player;
    p->rocket.inFlight = false;

    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (!g.enemies[j].active) continue;
        Enemy *ej = &g.enemies[j];
        float dist = Vector2Distance(pos, ej->pos);
        if (dist < ROCKET_EXPLOSION_RADIUS + ej->size) {
            DamageEnemy(j, ROCKET_EXPLOSION_DAMAGE, DMG_EXPLOSIVE, HIT_AOE);
            Vector2 away = Vector2Subtract(ej->pos, pos);
            if (Vector2Length(away) > 1.0f)
                ej->vel = Vector2Scale(
                    Vector2Normalize(away), ROCKET_KNOCKBACK);
        }
    }

    // rocket jump — push player away from explosion
    float playerDist = Vector2Distance(pos, p->pos);
    if (playerDist < ROCKET_EXPLOSION_RADIUS) {
        float scale = 1.0f - (playerDist / ROCKET_EXPLOSION_RADIUS);
        Vector2 away = Vector2Subtract(p->pos, pos);
        if (Vector2Length(away) > 1.0f) {
            p->vel = Vector2Scale(
                Vector2Normalize(away), ROCKET_JUMP_FORCE * scale);
        }
        p->hp -= ROCKET_JUMP_DAMAGE;
        SpawnParticles(p->pos, RED, CONTACT_HIT_PARTICLES);
        if (p->hp <= 0) { p->hp = 0; g.gameOver = true; }
    }
    SpawnExplosionVfx(pos, RED, ORANGE, YELLOW, ORANGE);
}

static void GrenadeExplode(Vector2 pos) {
    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (!g.enemies[j].active) continue;
        Enemy *ej = &g.enemies[j];
        float dist = Vector2Distance(pos, ej->pos);
        if (dist < GRENADE_EXPLOSION_RADIUS + ej->size) {
            DamageEnemy(j, GRENADE_EXPLOSION_DAMAGE, DMG_EXPLOSIVE, HIT_AOE);
            Vector2 away = Vector2Subtract(ej->pos, pos);
            if (Vector2Length(away) > 1.0f)
                ej->vel = Vector2Scale(
                    Vector2Normalize(away), GRENADE_KNOCKBACK);
        }
    }
    SpawnExplosionVfx(pos, GREEN, YELLOW, ORANGE, YELLOW);
}

// deployables ------------------------------------------------------------- /
static void SpawnDeployable(DeployableType type, Vector2 pos) {
    for (int i = 0; i < MAX_DEPLOYABLES; i++) {
        if (!g.deployables[i].active) {
            Deployable *d = &g.deployables[i];
            d->active = true;
            d->pos = pos;
            d->type = type;
            d->actionTimer = 0;
            d->hp = 0;
            switch (type) {
                case DEPLOY_TURRET:
                    d->timer = TURRET_LIFETIME;
                    d->radius = TURRET_RANGE;
                    d->hp = TURRET_HP;
                    break;
                case DEPLOY_MINE:
                    d->timer = MINE_LIFETIME;
                    d->radius = MINE_TRIGGER_RADIUS;
                    break;
                case DEPLOY_HEAL:
                    d->timer = HEAL_LIFETIME;
                    d->radius = HEAL_RADIUS;
                    break;
            }
            break;
        }
    }
}

static void UpdateDeployables(float dt) {
    Player *p = &g.player;
    for (int i = 0; i < MAX_DEPLOYABLES; i++) {
        Deployable *d = &g.deployables[i];
        if (!d->active) continue;

        d->timer -= dt;
        if (d->timer <= 0) {
            d->active = false;
            continue;
        }

        switch (d->type) {
        case DEPLOY_TURRET: {
            d->actionTimer -= dt;
            if (d->actionTimer <= 0) {
                // find nearest enemy in range
                int best = -1;
                float bestDist = d->radius;
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (!g.enemies[j].active) continue;
                    float dist = Vector2Distance(d->pos, g.enemies[j].pos);
                    if (dist < bestDist) {
                        bestDist = dist;
                        best = j;
                    }
                }
                if (best >= 0) {
                    Vector2 dir = Vector2Normalize(
                        Vector2Subtract(g.enemies[best].pos, d->pos));
                    Vector2 muzzle = Vector2Add(d->pos,
                        Vector2Scale(dir, TURRET_MUZZLE_OFFSET));
                    SpawnProjectile(muzzle, dir,
                        TURRET_BULLET_SPEED, TURRET_DAMAGE,
                        TURRET_BULLET_LIFETIME, TURRET_BULLET_SIZE,
                        false, false, PROJ_BULLET, DMG_BALLISTIC);
                    SpawnParticle(muzzle,
                        Vector2Scale(dir, TURRET_MUZZLE_SPEED),
                        (Color)TURRET_COLOR, TURRET_MUZZLE_SIZE,
                        TURRET_MUZZLE_LIFETIME);
                    g.enemies[best].aggroIdx = (i8)i;
                    d->actionTimer = TURRET_FIRE_RATE;
                }
            }
        } break;

        case DEPLOY_MINE: {
            // mine trigger check: proximity to any enemy
            bool triggered = false;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                Enemy *e = &g.enemies[j];
                if (!e->active) continue;
                if (Vector2Distance(d->pos, e->pos) < d->radius + e->size) {
                    triggered = true;
                    break;
                }
            }
            if (triggered) {
                // root all enemies in AOE (no move, can still shoot)
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    Enemy *e = &g.enemies[j];
                    if (!e->active) continue;
                    if (Vector2Distance(d->pos, e->pos) < MINE_ROOT_RADIUS)
                        e->rootTimer = MINE_ROOT_DURATION;
                }
                d->active = false;
                SpawnParticles(d->pos, (Color)MINE_COLOR, 10);
                // spawn web VFX
                for (int j = 0; j < MAX_MINE_WEBS; j++) {
                    if (!g.vfx.mineWebs[j].active) {
                        g.vfx.mineWebs[j] = (MineWebVfx){
                            .pos = d->pos,
                            .timer = MINE_WEB_DURATION,
                            .active = true,
                        };
                        break;
                    }
                }
            }
        } break;

        case DEPLOY_HEAL: {
            float dist = Vector2Distance(d->pos, p->pos);
            if (dist < d->radius && p->hp < p->maxHp) {
                p->hp += HEAL_PER_SEC * dt;
                if (p->hp > p->maxHp) p->hp = p->maxHp;
            }
            // heal turrets in range
            for (int j = 0; j < MAX_DEPLOYABLES; j++) {
                Deployable *t = &g.deployables[j];
                if (!t->active || t->type != DEPLOY_TURRET) continue;
                if (Vector2Distance(d->pos, t->pos) < d->radius && t->hp < TURRET_HP) {
                    t->hp += HEAL_PER_SEC * dt;
                    if (t->hp > TURRET_HP) t->hp = TURRET_HP;
                }
            }
        } break;

        }
    }

    // tick mine web VFX
    for (int i = 0; i < MAX_MINE_WEBS; i++) {
        if (!g.vfx.mineWebs[i].active) continue;
        g.vfx.mineWebs[i].timer -= dt;
        if (g.vfx.mineWebs[i].timer <= 0) g.vfx.mineWebs[i].active = false;
    }
}

// fire patches ------------------------------------------------------------ /
static void SpawnFirePatch(Vector2 pos) {
    for (int i = 0; i < MAX_FIRE_PATCHES; i++) {
        if (!g.firePatches[i].active) {
            g.firePatches[i] = (FirePatch){
                .pos = pos,
                .timer = FLAME_PATCH_LIFETIME,
                .actionTimer = 0,
                .radius = FLAME_PATCH_RADIUS,
                .active = true,
            };
            return;
        }
    }
}

static void UpdateFirePatches(float dt) {
    for (int i = 0; i < MAX_FIRE_PATCHES; i++) {
        FirePatch *fp = &g.firePatches[i];
        if (!fp->active) continue;
        fp->timer -= dt;
        if (fp->timer <= 0) { fp->active = false; continue; }
        fp->actionTimer -= dt;
        if (fp->actionTimer <= 0) {
            fp->actionTimer = FLAME_PATCH_TICK;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                Enemy *e = &g.enemies[j];
                if (!e->active) continue;
                if (Vector2Distance(fp->pos, e->pos) < fp->radius + e->size) {
                    DamageEnemy(j,
                        (int)(FLAME_PATCH_DPS * FLAME_PATCH_TICK),
                        DMG_ABILITY, HIT_AOE);
                }
            }
        }
    }
}

// bfg10k lightning chain -------------------------------------------------- /
static void TriggerLightningChain(Vector2 origin, int firstEnemyIdx) {
    LightningChain *lc = &g.lightning;
    memset(lc, 0, sizeof(*lc));
    lc->active = true;
    lc->propagating = true;
    lc->hopTimer = BFG_HOP_DELAY;
    lc->currentWave = 0;

    // first enemy is wave 0 — already damaged by direct hit
    lc->hit[firstEnemyIdx] = true;
    lc->sources[0] = origin;
    lc->sourceCount = 1;
}

static void UpdateLightningChain(float dt) {
    LightningChain *lc = &g.lightning;
    if (!lc->active) return;

    // fade existing arcs
    bool anyArcAlive = false;
    for (int i = 0; i < lc->arcCount; i++) {
        LightningArc *a = &lc->arcs[i];
        if (!a->active) continue;
        a->timer -= dt;
        if (a->timer <= 0) {
            a->active = false;
        } else {
            anyArcAlive = true;
        }
    }

    // propagation
    if (lc->propagating) {
        lc->hopTimer -= dt;
        if (lc->hopTimer <= 0) {
            lc->hopTimer = BFG_HOP_DELAY;
            lc->nextSourceCount = 0;

            // for each source in current wave, find nearby unhit enemies
            for (int s = 0; s < lc->sourceCount; s++) {
                Vector2 src = lc->sources[s];
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (!g.enemies[j].active) continue;
                    if (lc->hit[j]) continue;

                    float dist = Vector2Distance(src, g.enemies[j].pos);
                    if (dist <= BFG_CHAIN_RADIUS + g.enemies[j].size) {
                        lc->hit[j] = true;

                        // damage enemy
                        DamageEnemy(j, BFG_CHAIN_DAMAGE, DMG_ABILITY, HIT_AOE);

                        // spark particles on hit
                        SpawnParticles(g.enemies[j].pos, (Color)BFG_COLOR, BFG_HIT_PARTICLES);

                        // create visual arc
                        if (lc->arcCount < BFG_MAX_ARCS) {
                            LightningArc *a = &lc->arcs[lc->arcCount++];
                            a->from = src;
                            a->to = g.enemies[j].pos;
                            a->timer = BFG_ARC_DURATION;
                            a->duration = BFG_ARC_DURATION;
                            a->active = true;
                            a->damageApplied = true;
                            a->targetIdx = j;
                        }

                        // add as source for next wave
                        if (lc->nextSourceCount < BFG_MAX_CHAIN_TARGETS) {
                            lc->nextSources[lc->nextSourceCount++] = g.enemies[j].pos;
                        }
                    }
                }
            }

            // move to next wave
            lc->currentWave++;
            if (lc->nextSourceCount == 0 || lc->currentWave >= BFG_MAX_HOPS) {
                lc->propagating = false;
            } else {
                memcpy(lc->sources, lc->nextSources,
                    sizeof(Vector2) * lc->nextSourceCount);
                lc->sourceCount = lc->nextSourceCount;
            }
        }
    }

    // chain fully done when no arcs remain and propagation stopped
    if (!lc->propagating && !anyArcAlive) {
        lc->active = false;
        g.player.bfg.active = false;
    }
}

static void UpdateProjectiles(float dt) {
    Player *p = &g.player;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *b = &g.projectiles[i];
        if (!b->active) continue;

        // grenade: drag + visual bounce arc
        if (b->type == PROJ_GRENADE) {
            b->vel = Vector2Scale(b->vel, 1.0f - GRENADE_DRAG * dt);
            // visual height simulation
            b->heightVel -= GRENADE_ARC_GRAVITY * dt;
            b->height += b->heightVel * dt;
            if (b->height <= 0.0f) {
                b->height = 0.0f;
                b->heightVel = -b->heightVel * GRENADE_ARC_BOUNCE_DAMPING;
                if (b->heightVel < GRENADE_ARC_MIN_VEL) b->heightVel = 0.0f;
            }
        }

        // bfg: spawn trail particles
        if (b->type == PROJ_BFG && !b->isEnemy) {
            Vector2 tvel = { (float)GetRandomValue(-40, 40),
                             (float)GetRandomValue(-40, 40) };
            Color tc = GetRandomValue(0, 1) ? (Color)BFG_COLOR : WHITE;
            SpawnParticle(b->pos, tvel, tc, BFG_TRAIL_SIZE, BFG_TRAIL_LIFETIME);
        }

        b->pos = Vector2Add(b->pos, Vector2Scale(b->vel, dt));
        b->lifetime -= dt;

        // fuse timer expired
        if (b->lifetime <= 0) {
            if (b->type == PROJ_ROCKET) RocketExplode(b->pos);
            if (b->type == PROJ_GRENADE) GrenadeExplode(b->pos);
            if (b->type == PROJ_BFG) {
                // fizzle — sad sparks, no chain
                SpawnParticles(b->pos, (Color)BFG_COLOR, 4);
                g.player.bfg.active = false;
            }
            b->active = false;
            continue;
        }

        // map boundary
        if (b->pos.x < 0 || b->pos.x > MAP_SIZE ||
            b->pos.y < 0 || b->pos.y > MAP_SIZE) {
            if (b->type == PROJ_GRENADE && b->bounces > 0) {
                // bounce off map edges
                if (b->pos.x < 0)        { b->pos.x = 0;        b->vel.x = -b->vel.x; }
                if (b->pos.x > MAP_SIZE)  { b->pos.x = MAP_SIZE; b->vel.x = -b->vel.x; }
                if (b->pos.y < 0)        { b->pos.y = 0;        b->vel.y = -b->vel.y; }
                if (b->pos.y > MAP_SIZE)  { b->pos.y = MAP_SIZE; b->vel.y = -b->vel.y; }
                b->vel = Vector2Scale(b->vel, GRENADE_BOUNCE_DAMPING);
                b->bounces--;
            } else {
                if (b->type == PROJ_ROCKET) RocketExplode(b->pos);
                if (b->type == PROJ_GRENADE) GrenadeExplode(b->pos);
                if (b->type == PROJ_BFG) {
                    SpawnParticles(b->pos, (Color)BFG_COLOR, 4);
                    g.player.bfg.active = false;
                }
                b->active = false;
                continue;
            }
        }

        // rocket reached target — explode
        if (b->type == PROJ_ROCKET) {
            float toTarget = Vector2Distance(b->pos, b->target);
            if (toTarget < ROCKET_SPEED * dt) {
                RocketExplode(b->target);
                b->active = false;
                continue;
            }
        }

        if (b->isEnemy) {
            // Shield absorbs enemy projectiles
            if (p->shield.active && p->shield.hp > 0) {
                float dist = Vector2Distance(b->pos, p->pos);
                if (dist < SHIELD_RADIUS + b->size) {
                    // Check if projectile is within shield arc
                    Vector2 toProj = Vector2Subtract(b->pos, p->pos);
                    float projAngle = atan2f(toProj.y, toProj.x);
                    float diff = fmodf(projAngle - p->shield.angle + 3*PI, 2*PI) - PI;
                    if (fabsf(diff) <= SHIELD_ARC / 2.0f) {
                        p->shield.hp -= (float)b->damage;
                        // Shield break
                        if (p->shield.hp <= 0) {
                            p->shield.hp = 0;
                            p->shield.active = false;
                            p->shield.regenTimer = -SHIELD_BROKEN_COOLDOWN;
                            SpawnParticles(p->pos, (Color)SHIELD_COLOR, 12);
                        } else {
                            // Absorb spark
                            SpawnParticle(b->pos, (Vector2){ 0, 0 },
                                (Color)SHIELD_COLOR, 3.0f, 0.15f);
                        }
                        b->active = false;
                        continue;
                    }
                }
            }
            // Enemy projectile — hit turrets
            bool hitTurret = false;
            for (int j = 0; j < MAX_DEPLOYABLES; j++) {
                Deployable *d = &g.deployables[j];
                if (!d->active || d->type != DEPLOY_TURRET) continue;
                float td = Vector2Distance(b->pos, d->pos);
                if (td < 10.0f + b->size) {
                    d->hp -= b->damage;
                    SpawnParticle(b->pos, (Vector2){ 0, 0 },
                        (Color)TURRET_COLOR, 2.0f, 0.1f);
                    if (d->hp <= 0) {
                        d->active = false;
                        SpawnParticles(d->pos, (Color)TURRET_COLOR, 12);
                    }
                    b->active = false;
                    hitTurret = true;
                    break;
                }
            }
            if (hitTurret) continue;
            // Enemy projectile — hit player
            float dist = Vector2Distance(b->pos, p->pos);
            if (dist < p->size + b->size && p->iFrames <= 0) {
                DamagePlayer(b->damage, b->dmgType, HIT_PROJ);
                b->active = false;
            }
        } else {
            // Player projectile — hit enemies
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!g.enemies[j].active) continue;
                Enemy *ej = &g.enemies[j];
                bool hit = EnemyHitPoint(ej, b->pos, b->size);
                if (hit) {
                    DamageEnemy(j, b->damage, b->dmgType, HIT_PROJ);
                    // sniper slow debuff (super shot gets enhanced slow)
                    if (b->appliesSlow) {
                        if (b->damage >= SNIPER_SUPER_DAMAGE) {
                            ej->slowTimer = SNIPER_SUPER_SLOW_DUR;
                            ej->slowFactor = SNIPER_SUPER_SLOW_FACTOR;
                        } else {
                            ej->slowTimer = SNIPER_SLOW_DURATION;
                            ej->slowFactor = SNIPER_SLOW_FACTOR;
                        }
                    }
                    if (b->type == PROJ_ROCKET) {
                        RocketExplode(b->pos);
                    } else if (b->type == PROJ_GRENADE) {
                        GrenadeExplode(b->pos);
                    } else if (b->type == PROJ_BFG) {
                        TriggerLightningChain(ej->pos, j);
                        // detonation burst
                        SpawnParticles(ej->pos, (Color)BFG_COLOR, BFG_DETONATION_PARTICLES);
                        SpawnParticles(ej->pos, WHITE, BFG_DETONATION_PARTICLES / 2);
                    } else if (b->knockback) {
                        Vector2 kb = Vector2Normalize(b->vel);
                        ej->vel = Vector2Scale(kb, SHOTGUN_KNOCKBACK);
                        // ricochet to nearest enemy
                        if (b->bounces > 0) {
                            b->bounces--;
                            b->lifetime = SHOTGUN_BULLET_LIFETIME;
                            float bestDist = 1e9f;
                            int bestIdx = -1;
                            for (int k = 0; k < MAX_ENEMIES; k++) {
                                if (k == j || !g.enemies[k].active) continue;
                                float d = Vector2Distance(b->pos, g.enemies[k].pos);
                                if (d < bestDist) {
                                    bestDist = d;
                                    bestIdx = k;
                                }
                            }
                            float speed = Vector2Length(b->vel) * SHOTGUN_BOUNCE_SPEED;
                            if (bestIdx >= 0) {
                                Vector2 dir = Vector2Normalize(
                                    Vector2Subtract(g.enemies[bestIdx].pos, b->pos));
                                b->vel = Vector2Scale(dir, speed);
                            } else {
                                // no target — reflect velocity
                                b->vel = Vector2Scale(Vector2Normalize(b->vel), -speed);
                            }
                            break;
                        }
                    }
                    b->active = false;
                    break;
                }
            }
        }
    }
}

// one pool of particles belonging, spawned wherever
static void UpdateParticles(float dt) 
{
    // --- Particles ---
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &g.vfx.particles[i];
        if (!pt->active) continue;
        pt->pos = Vector2Add(pt->pos, Vector2Scale(pt->vel, dt));
        pt->vel = Vector2Scale(pt->vel, 1.0f - PARTICLE_DRAG * dt);
        pt->lifetime -= dt;
        if (pt->lifetime <= 0) pt->active = false;
    }

}

static void UpdateBeams(float dt)
{
    for (int i = 0; i < MAX_BEAMS; i++) {
        Beam *b = &g.vfx.beams[i];
        if (!b->active) continue;
        b->timer -= dt;
        if (b->timer <= 0) b->active = false;
    }
}

// update camera offset to current browser window size
// how to make it resize while game is paused?
// separate window resize and camera
// native is static size for now
// could we add dynamic resize? fullscreen? later
static void WindowResize(void) 
{
#ifdef PLATFORM_WEB
    int sw = EM_ASM_INT({ return window.innerWidth; });
    int sh = EM_ASM_INT({ return window.innerHeight; });
    if (sw != GetScreenWidth() || sh != GetScreenHeight()) {
        SetWindowSize(sw, sh);
    }
#endif
// add native resize
}

// the 8.0f could be inside default
// what is it doing exactly?
// how much can we fiddle with this value?
static void MoveCamera(float dt) 
{
    // camera should be it's own thing to
    // --- Camera smooth follow ---
    Player *p = &g.player;
    // let's search up Vector2Lerp in raylib.h
    // ok I understand what linear interpolation is, but what is this amount?
    // ok so amount, t if taken as a value between 0 and 1 is where you are 
    // looking to interpolate, 0.5, would be right in the middle of both points
    // why 8 * delta time? lags 8 times the ms of the frame?
    // so its like how much the camera lags behind the player?
    // should be normed between 0 and 1 or capped?
    // game feel thing for sure
    g.camera.target = Vector2Lerp(g.camera.target, p->pos, CAMERA_LERP_RATE * dt);

    g.camera.offset = 
        (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}


// ok so game state. giant function.
// let's separate this and make it more modular
// so its not so hard to add features. 
// or is it commonplace to have massive functions in the game dev world?
void UpdateGame(void)
{
    WindowResize();

    if (g.screen == SCREEN_SELECT) {
        UpdateSelect();
        return;
    }

    // frametime clamp
    float dt = GetFrameTime();
    if (dt > DT_MAX) dt = DT_MAX;

    // need to make sure that esc also pauses in native build
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) g.paused = !g.paused;

    if (g.paused && IsKeyPressed(KEY_F)) {
        int mon = GetCurrentMonitor();
        ToggleFullscreen();
        if (!IsWindowFullscreen()) {
            int mx = GetMonitorWidth(mon);
            int my = GetMonitorHeight(mon);
            Vector2 pos = GetMonitorPosition(mon);
            SetWindowSize(SCREEN_W, SCREEN_H);
            SetWindowPosition(pos.x + (float)(mx - SCREEN_W) / 2,
                              pos.y + (float)(my - SCREEN_H) / 2);
        }
    }

    if (g.gameOver) {
        if (IsKeyPressed(KEY_ENTER)) InitGame();
        return;
    }

    if (g.paused) return;

    UpdatePlayer(dt);
    UpdateEnemies(dt);
    UpdateProjectiles(dt);
    UpdateLightningChain(dt);
    UpdateParticles(dt);
    UpdateBeams(dt);
    UpdateDeployables(dt);
    UpdateFirePatches(dt);

    for (int i = 0; i < MAX_EXPLOSIVES; i++) {
        if (!g.vfx.explosives[i].active) continue;
        g.vfx.explosives[i].timer -= dt;
        if (g.vfx.explosives[i].timer <= 0)
            g.vfx.explosives[i].active = false;
    }

    MoveCamera(dt);
}
