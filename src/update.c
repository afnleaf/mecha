// update.c
// mutate the game state with various data transformations
// handle input
#include "game.h"

// M1/M2 wrappers — mouse buttons OR keyboard OR gamepad triggers
static inline bool M1Down(void) {
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KB_M1_KEY)
        || IsGamepadButtonDown(GAMEPAD_INDEX, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
}
static inline bool M1Pressed(void) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KB_M1_KEY)
        || IsGamepadButtonPressed(GAMEPAD_INDEX, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
}
static inline bool M2Down(void) {
    return IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsKeyDown(KB_M2_KEY)
        || IsGamepadButtonDown(GAMEPAD_INDEX, GAMEPAD_BUTTON_LEFT_TRIGGER_2);
}
static inline bool M2Pressed(void) {
    return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KB_M2_KEY)
        || IsGamepadButtonPressed(GAMEPAD_INDEX, GAMEPAD_BUTTON_LEFT_TRIGGER_2);
}

// Dash input — shared by gameplay dash, QTE timing, select screen
static inline bool DashPressed(void) {
    return IsKeyPressed(KEY_SPACE)
        || IsGamepadButtonPressed(GAMEPAD_INDEX, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
}

// Input mode detection — sets g.gamepadActive for HUD display
static void DetectInputMode(void) {
    if (IsGamepadAvailable(GAMEPAD_INDEX)) {
        for (int b = 1; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++) {
            if (IsGamepadButtonPressed(GAMEPAD_INDEX, b)) {
                g.gamepadActive = true;
                return;
            }
        }
        float lx = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_LEFT_X);
        float ly = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_LEFT_Y);
        float rx = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_RIGHT_X);
        float ry = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_RIGHT_Y);
        if (fabsf(lx) > GAMEPAD_STICK_DEADZONE || fabsf(ly) > GAMEPAD_STICK_DEADZONE ||
            fabsf(rx) > GAMEPAD_STICK_DEADZONE || fabsf(ry) > GAMEPAD_STICK_DEADZONE) {
            g.gamepadActive = true;
            return;
        }
    }
    Vector2 md = GetMouseDelta();
    if (md.x != 0 || md.y != 0) { g.gamepadActive = false; return; }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        g.gamepadActive = false; return;
    }
    if (GetKeyPressed() != 0) { g.gamepadActive = false; }
}

// forward declarations for functions used before defined
static void RocketExplode(Vector2 pos);
static void SpawnDeployable(DeployableType type, Vector2 pos);
static void SpawnVfxTimer(Vector2 pos, float duration, VfxTimerType type);
static void UpdateParticles(float dt);
static void UpdateProjectiles(float dt);
static void UpdateMovement(Player *p, Vector2 moveDir, float moveLen, float dt);
static void UpdateDash(Player *p, Vector2 moveDir, float moveLen, float dt);
static void UpdateWeapon(Player *p, Vector2 toMouse, float dt);
static void UpdateAbilities(Player *p, Vector2 toMouse, float dt);


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
            if (EnemyHitSweep(e, origin, rayEnd, 0)) {
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
            if (EnemyHitSweep(e, origin, rayEnd, RAILGUN_BEAM_RADIUS)) {
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
    if (IsGamepadAvailable(GAMEPAD_INDEX)) {
        bool lbHeld = IsGamepadButtonDown(GAMEPAD_INDEX, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        for (int i = 0; i < (int)PAD_ABILITY_COUNT; i++) {
            if (PAD_ABILITY_MAP[i].ability == ability
                && PAD_ABILITY_MAP[i].needsLB == lbHeld
                && IsGamepadButtonPressed(GAMEPAD_INDEX, PAD_ABILITY_MAP[i].button))
                return true;
        }
    }
    return false;
}

static bool IsAbilityDown(Player *p, AbilityID ability) {
    for (int i = 0; i < ABILITY_SLOTS; i++) {
        if (p->slots[i].ability == ability
            && IsKeyDown(p->slots[i].key))
            return true;
    }
    if (IsGamepadAvailable(GAMEPAD_INDEX)) {
        bool lbHeld = IsGamepadButtonDown(GAMEPAD_INDEX, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        for (int i = 0; i < (int)PAD_ABILITY_COUNT; i++) {
            if (PAD_ABILITY_MAP[i].ability == ability
                && PAD_ABILITY_MAP[i].needsLB == lbHeld
                && IsGamepadButtonDown(GAMEPAD_INDEX, PAD_ABILITY_MAP[i].button))
                return true;
        }
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
static Vector2 lastKbAim = { 1, 0 };
static bool kbAimActive = false;
static float aimGrace[4] = { 0 }; // right, left, down, up

static Vector2 UpdateMouseAim() {
    Player *p = &g.player;
    float dt = GetFrameTime();

    // Right stick aim (gamepad)
    if (IsGamepadAvailable(GAMEPAD_INDEX)) {
        float rx = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_RIGHT_X) * GAMEPAD_AIM_SENS;
        float ry = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_RIGHT_Y) * GAMEPAD_AIM_SENS;
        if (fabsf(rx) > GAMEPAD_STICK_DEADZONE || fabsf(ry) > GAMEPAD_STICK_DEADZONE) {
            lastKbAim = Vector2Normalize((Vector2){ rx, ry });
            kbAimActive = true;
        }
    }

    // Keyboard aim with grace period for diagonals
    bool held[4] = {
        IsKeyDown(KEY_RIGHT) || IsKeyDown(AIM_RIGHT_KEY),
        IsKeyDown(KEY_LEFT)  || IsKeyDown(AIM_LEFT_KEY),
        IsKeyDown(KEY_DOWN)  || IsKeyDown(AIM_DOWN_KEY),
        IsKeyDown(KEY_UP)    || IsKeyDown(AIM_UP_KEY),
    };
    for (int i = 0; i < 4; i++) {
        if (held[i]) aimGrace[i] = AIM_KEY_GRACE;
        else         aimGrace[i] -= dt;
    }
    bool anyHeld = held[0] || held[1] || held[2] || held[3];
    Vector2 arrowDir = { 0, 0 };
    if (anyHeld) {
        if (held[0] || aimGrace[0] > 0) arrowDir.x += 1;
        if (held[1] || aimGrace[1] > 0) arrowDir.x -= 1;
        if (held[2] || aimGrace[2] > 0) arrowDir.y += 1;
        if (held[3] || aimGrace[3] > 0) arrowDir.y -= 1;
    }
    float arrowLen = Vector2Length(arrowDir);
    if (arrowLen > 0) {
        arrowDir = Vector2Scale(arrowDir, 1.0f / arrowLen);
        lastKbAim = arrowDir;
        kbAimActive = true;
    }
    // Mouse movement disengages keyboard/stick aim
    Vector2 mouseDelta = GetMouseDelta();
    if (mouseDelta.x != 0 || mouseDelta.y != 0) kbAimActive = false;
    if (kbAimActive) {
        Vector2 toAim = Vector2Scale(lastKbAim, GAMEPAD_AIM_DIST);
        p->angle = atan2f(toAim.y, toAim.x);
        return toAim;
    }
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

static void SpawnDeployable(DeployableType type, Vector2 pos);
static void TrySpawnDeployable(Player *p, AbilityID ability, DeployableType type,
    float *cooldown, float cooldownTime, int maxActive, Vector2 pos, float dt)
{
    if (*cooldown > 0) *cooldown -= dt;
    if (IsAbilityPressed(p, ability) && *cooldown <= 0) {
        if (CountActiveDeployables(type) < maxActive) {
            SpawnDeployable(type, pos);
            *cooldown = cooldownTime;
        }
    }
}

static void SpawnSwordSparks(Vector2 origin, float angle, float arc, float radius) {
    for (int i = 0; i < SWORD_SPARK_COUNT; i++) {
        float a = angle - arc / 2.0f
            + arc * (float)i / (SWORD_SPARK_COUNT - 1);
        Vector2 particlePos = Vector2Add(origin,
            (Vector2){ cosf(a) * radius * SWORD_SPARK_RADIUS_FRAC,
                       sinf(a) * radius * SWORD_SPARK_RADIUS_FRAC });
        Vector2 particleVel = { cosf(a) * SWORD_SPARK_SPEED,
                                sinf(a) * SWORD_SPARK_SPEED };
        SpawnParticle(particlePos, particleVel, ORANGE,
            SWORD_SPARK_SIZE, SWORD_SPARK_LIFETIME);
    }
}

// Play area boundary clamp ------------------------------------------------- /
// Play area boundary:
//   Combat zone: (MAP_LEFT, 0) to (MAP_RIGHT, MAP_SIZE)
//   Corridor:    (0, STEP_Y) to (MAP_LEFT, MAP_SIZE)  — left of combat zone
//   Base room:   (0, MAP_SIZE) to (BASE_W, BASE_BOTTOM) — below corridor
static void ClampToPlayArea(Player *p)
{
    float r = p->size;

    // Top wall
    if (p->pos.y < r) p->pos.y = r;

    if (p->pos.y <= STEP_Y) {
        // Upper combat zone — full left wall at MAP_LEFT
        if (p->pos.x < MAP_LEFT + r) p->pos.x = MAP_LEFT + r;
        if (p->pos.x > MAP_RIGHT - r) p->pos.x = MAP_RIGHT - r;
    } else if (p->pos.y <= MAP_SIZE) {
        // Lower combat zone + corridor zone
        // Left wall: x=0 (corridor extends left of MAP_LEFT)
        if (p->pos.x < r) p->pos.x = r;
        // Right wall: MAP_RIGHT
        if (p->pos.x > MAP_RIGHT - r) p->pos.x = MAP_RIGHT - r;
        // Step wall: blocks going above STEP_Y when in corridor (x < MAP_LEFT)
        if (p->pos.x < MAP_LEFT && p->pos.y < STEP_Y + r)
            p->pos.y = STEP_Y + r;
        // Inner corner at (MAP_LEFT, STEP_Y) — round collision
        {
            float dx = p->pos.x - MAP_LEFT;
            float dy = p->pos.y - STEP_Y;
            if (dx > 0 && dx < r && dy > 0 && dy < r) {
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < r && dist > 0) {
                    float s = r / dist;
                    p->pos.x = MAP_LEFT + dx * s;
                    p->pos.y = STEP_Y + dy * s;
                }
            }
        }
        // Bottom wall — gap for base entrance (x < BASE_W)
        if (p->pos.y > MAP_SIZE - r && p->pos.x >= BASE_W)
            p->pos.y = MAP_SIZE - r;
    } else {
        // Base room (below MAP_SIZE)
        if (p->pos.x < r) p->pos.x = r;
        if (p->pos.x > BASE_W - r) p->pos.x = BASE_W - r;
        if (p->pos.y > BASE_BOTTOM - r) p->pos.y = BASE_BOTTOM - r;
    }


}

// Weapon Select Screen ----------------------------------------------------- /
static void UpdateSelect(float dt)
{
    Player *p = &g.player;

    // WASD + left stick movement
    Vector2 moveDir = { 0, 0 };
    if (IsKeyDown(KEY_W)) moveDir.y -= 1;
    if (IsKeyDown(KEY_S)) moveDir.y += 1;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1;
    if (IsKeyDown(KEY_D)) moveDir.x += 1;
    if (IsGamepadAvailable(GAMEPAD_INDEX)) {
        float lx = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_LEFT_X);
        float ly = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_LEFT_Y);
        if (fabsf(lx) > GAMEPAD_STICK_DEADZONE) moveDir.x += lx;
        if (fabsf(ly) > GAMEPAD_STICK_DEADZONE) moveDir.y += ly;
    }
    float moveLen = Vector2Length(moveDir);
    if (moveLen > 1.0f) moveDir = Vector2Scale(moveDir, 1.0f / moveLen);

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
    if (DashPressed() && !p->dash.active && p->dash.charges > 0) {
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

    // Clamp to play area (base + corridor + combat zone)
    ClampToPlayArea(p);

    // Aim (mouse / right stick)
    UpdateMouseAim();

    // Pedestal positions (U curve inside base room)
    float spacing = BASE_W / SELECT_PEDESTAL_SPACING;
    for (int i = 0; i < NUM_PRIMARY_WEAPONS; i++) {
        float norm = (float)(i - 2) / 2.0f;
        g.selectPedestals[i] = (Vector2){
            SELECT_PEDESTAL_X + spacing * (float)(i + 1),
            SELECT_PEDESTAL_Y + SELECT_PEDESTAL_CURVE * (1.0f - norm * norm)
        };
    }
    Vector2 *pedestals = g.selectPedestals;

    // Find nearest pedestal within interact radius
    g.selectIndex = -1;
    float bestDist = SELECT_INTERACT_RADIUS;
    for (int i = 0; i < NUM_PRIMARY_WEAPONS; i++) {
        // Phase 1: skip already-chosen primary
        if (g.selectPhase == 1 && SELECT_WEAPONS[i] == p->primary) continue;
        float d = Vector2Distance(p->pos, pedestals[i]);
        if (d < bestDist) {
            bestDist = d;
            g.selectIndex = i;
        }
    }

    // M1 or Enter or A selects weapon
    if (g.selectIndex >= 0 &&
        (M1Pressed() || IsKeyPressed(KEY_ENTER)
         || IsGamepadButtonPressed(GAMEPAD_INDEX, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
        if (g.selectPhase == 0) {
            p->primary = SELECT_WEAPONS[g.selectIndex];
            g.selectPhase = 1;
        } else {
            p->secondary = SELECT_WEAPONS[g.selectIndex];
            ClearPools();
            g.phase = PHASE_COMBAT;
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
                SpawnSwordSparks(base, demoAngle, SWORD_ARC, SWORD_RADIUS);
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
                    SNIPER_HIP_BULLET_SPEED, SNIPER_DAMAGE,
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
                (void)r;
                SpawnParticles(muzzle, RED, ROCKET_MUZZLE_PARTICLES);
                SpawnParticle(muzzle,
                    Vector2Scale(aimDir, ROCKET_MUZZLE_SPEED), ORANGE,
                    ROCKET_MUZZLE_SIZE, ROCKET_MUZZLE_LIFETIME);
            } break;
        }
    }

    // Tick sword demo arc
    if (g.selectSwordTimer > 0) g.selectSwordTimer -= dt;

    // Shadow lag (player model exists in select too)
    p->shadowPos = Vector2Lerp(p->shadowPos, p->pos, SHADOW_LAG * dt);
}

// SweepDamage — shared by sword and spin
// Returns hit count; hitIndices[] filled with enemy indices for post-hit work
static int SweepDamage(
    Vector2 origin, Vector2 sweepEnd, float sweepAngle,
    int damage, DamageType dmgType,
    u8 *hitBits, float *lastResetAngle, int *hitIndices, int maxHits)
{
    while (sweepAngle - *lastResetAngle >= PI) {
        memset(hitBits, 0, MAX_ENEMIES / 8);
        *lastResetAngle += PI;
    }
    int hits = 0;
    for (int i = 0; i < MAX_ENEMIES && hits < maxHits; i++) {
        Enemy *ei = &g.enemies[i];
        if (!ei->active) continue;
        if (hitBits[i >> 3] & (1 << (i & 7))) continue;
        if (!EnemyHitSweep(ei, origin, sweepEnd, 0)) continue;
        DamageEnemy(i, damage, dmgType, HIT_MELEE);
        hitBits[i >> 3] |= (1 << (i & 7));
        hitIndices[hits++] = i;
    }
    return hits;
}

// player ------------------------------------------------------------------- /

static void UpdateMovement(Player *p, Vector2 moveDir, float moveLen, float dt)
{
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
}

static void UpdateDash(Player *p, Vector2 moveDir, float moveLen, float dt)
{
    p->dash.orbAngle += DASH_ORB_SPEED * dt;

    // --- Dash (charge system) ---
    if (p->dash.charges < p->dash.maxCharges) {
        p->dash.rechargeTimer -= dt;
        if (p->dash.rechargeTimer <= 0) {
            p->dash.charges++;
            if (p->dash.charges < p->dash.maxCharges)
                p->dash.rechargeTimer = p->dash.rechargeTime;
        }
    }

    bool spacePressed = DashPressed();

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
            memset(p->sword.hitBits, 0, sizeof(p->sword.hitBits));
            p->sword.lastResetAngle = p->sword.angle
                - (p->sword.arc * DASH_SLASH_ARC_MULT) / 2.0f;
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
        if (p->primary == WPN_SNIPER && M2Pressed())
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
}

static void UpdateWeapon(Player *p, Vector2 toMouse, float dt)
{
    // --- Weapon swap (Ctrl) ---
    if (IsKeyPressed(WEAPON_SWAP_KEY)
        || IsGamepadButtonPressed(GAMEPAD_INDEX, GAMEPAD_BUTTON_LEFT_THUMB)) {
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
                bool m1Pressed = M1Pressed();
                bool dashPressed = DashPressed();
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
        if (!p->gun.overheated && M2Down()) {
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
            if (M1Down()
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
        p->laser.active = M1Down();
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
        p->sniper.aiming = M2Down();

        // M1 fires — behavior depends on aiming state
        if (M1Pressed() && p->sniper.cooldownTimer <= 0) {
            Vector2 aimDir = Vector2Normalize(toMouse);
            int spreadVal;
            float speed, cooldown;
            int damage;

            if (p->sniper.aiming) {
                damage  = p->sniper.superShotReady ? SNIPER_SUPER_DAMAGE : SNIPER_DAMAGE;
                spreadVal = SNIPER_AIM_SPREAD;
                speed   = SNIPER_AIM_BULLET_SPEED;
                cooldown = SNIPER_AIM_COOLDOWN;
            } else {
                damage  = SNIPER_DAMAGE;
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
        if (M1Pressed()
            && p->sword.timer <= 0
            && p->spin.timer <= 0
        ) {
            p->sword.timer = p->sword.duration;
            p->sword.angle = p->angle;
            p->sword.lunge = false;
            p->sword.dashSlash = p->dash.active;
            memset(p->sword.hitBits, 0, sizeof(p->sword.hitBits));
            p->sword.lastResetAngle = p->sword.angle - p->sword.arc / 2.0f;

            SpawnSwordSparks(p->pos, p->sword.angle, p->sword.arc, p->sword.radius);
        }
        // M2: Lunge
        if (M2Pressed()
            && p->sword.timer <= 0
            && p->spin.timer <= 0
        ) {
            p->sword.timer = LUNGE_DURATION;
            p->sword.angle = p->angle;
            p->sword.lunge = true;
            p->sword.dashSlash = p->dash.active;
            memset(p->sword.hitBits, 0, sizeof(p->sword.hitBits));

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
            bool reloadInput = M1Pressed() || DashPressed();
            if (reloadInput && !p->revolver.reloadLocked) {
                bool inSweet = progress >= REVOLVER_RELOAD_SWEET_START
                    && progress <= REVOLVER_RELOAD_SWEET_END;
                if (inSweet) {
                    p->revolver.reloadTimer = REVOLVER_RELOAD_FAST_TIME;
                    // Dash reload — next cylinder does double damage
                    if (DashPressed())
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
        if (M1Pressed()
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
        if (M2Pressed()
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
        if (M1Pressed()
            && p->rocket.cooldownTimer <= 0
            && !p->rocket.inFlight) {
            SpawnRocket(p, toMouse);
            p->rocket.cooldownTimer = ROCKET_COOLDOWN;
        }
        // M2: Detonate in-flight rocket
        if (M2Pressed()
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
            dmg, DMG_SLASH, p->sword.hitBits, &p->sword.lastResetAngle,
            hits, MAX_ENEMIES);
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
            if (p->sword.hitBits[i >> 3] & (1 << (i & 7))) continue; // already hit

            Vector2 toEnemy = Vector2Subtract(ei->pos, p->pos);
            float dist = Vector2Length(toEnemy);
            if (dist > range + ei->size) continue;
            float enemyAngle = atan2f(toEnemy.y, toEnemy.x);
            float angleDiff = fabsf(fmodf(enemyAngle - p->sword.angle + PI, 2*PI) - PI);
            if (angleDiff > LUNGE_CONE_HALF) continue;

            DamageEnemy(i, dmg, DMG_PIERCE, HIT_MELEE);
            p->sword.hitBits[i >> 3] |= (1 << (i & 7)); // mark as hit
        }
        p->sword.timer -= dt;
    }
}

static void UpdateBlinkDagger(Player *p, Vector2 toMouse, float dt)
{
    if (p->blink.cooldown > 0) p->blink.cooldown -= dt;

    // Delayed damage tick
    if (p->blink.damageActive) {
        p->blink.damageTimer -= dt;
        if (p->blink.damageTimer <= 0) {
            p->blink.damageActive = false;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                Enemy *e = &g.enemies[i];
                if (!e->active || !e->blinkMarked) continue;
                DamageEnemy(i, BLINK_DAMAGE,
                    DMG_SLASH, HIT_MELEE);
                e->blinkMarked = false;
            }
        }
    }

    if (!IsAbilityPressed(p, ABL_BLINK)) return;
    if (p->blink.cooldown > 0) return;

    Vector2 dir = Vector2Normalize(toMouse);
    Vector2 origin = p->pos;
    Vector2 dest = Vector2Add(origin,
        Vector2Scale(dir, BLINK_DISTANCE));

    // Clamp to map bounds
    dest = Vector2Clamp(dest,
        (Vector2){ p->size, p->size },
        (Vector2){ MAP_RIGHT - p->size, BASE_BOTTOM - p->size });

    // Teleport
    p->pos = dest;

    // Always 10s — damage cuts it to 3s via DamagePlayer
    p->blink.cooldown = BLINK_COOLDOWN;

    // Queue delayed damage + mark enemies in the path
    p->blink.damageActive = true;
    p->blink.damageTimer  = BLINK_DAMAGE_DELAY;
    p->blink.slashOrigin  = origin;
    p->blink.slashTip     = dest;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g.enemies[i];
        if (!e->active) continue;
        if (EnemyHitSweep(e, origin, dest, BLINK_BEAM_WIDTH)) {
            e->blinkMark = BLINK_DAMAGE_DELAY;
            e->blinkMarked = true;
        }
    }

    // Spawn 3 beam trails (center + 2 offset)
    Vector2 perp = { -dir.y, dir.x };
    for (int i = -1; i <= 1; i++) {
        Vector2 off = Vector2Scale(perp,
            (float)i * BLINK_BEAM_OFFSET);
        SpawnBeam(
            Vector2Add(origin, off),
            Vector2Add(dest, off),
            BLINK_BEAM_DURATION,
            BLINK_COLOR,
            BLINK_BEAM_WIDTH
        );
    }
}

static void UpdateAbilities(Player *p, Vector2 toMouse, float dt)
{
    // --- Spin attack (Shift) ---
    p->spin.cooldownTimer -= dt;
    if (p->spin.cooldownTimer < 0) p->spin.cooldownTimer = 0;

    if (IsAbilityPressed(p, ABL_SPIN)
        && p->spin.timer <= 0
        && p->spin.cooldownTimer <= 0
    ) {
        p->spin.timer = p->spin.duration;
        p->spin.cooldownTimer = p->spin.cooldown;
        memset(p->spin.hitBits, 0, sizeof(p->spin.hitBits));
        p->spin.lastResetAngle = 0.0f;

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

    // Spin damage — sweep line hits enemies as it passes over them
    if (p->spin.timer > 0) {
        float progress = 1.0f - (p->spin.timer / p->spin.duration);
        float sweepAngle = PI * 4.0f * progress;
        Vector2 sweepEnd = Vector2Add(p->pos,
            (Vector2){ cosf(sweepAngle) * p->spin.radius,
                       sinf(sweepAngle) * p->spin.radius });

        int hits[MAX_ENEMIES];
        int nhits = SweepDamage(p->pos, sweepEnd, sweepAngle,
            SPIN_DAMAGE, DMG_SLASH, p->spin.hitBits, &p->spin.lastResetAngle,
            hits, MAX_ENEMIES);
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

    // --- Deployable spawns ---
    // Turret — placed at mouse, up to placement distance
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
    // Mine
    TrySpawnDeployable(p, ABL_MINE, DEPLOY_MINE,
        &p->mineCooldown, MINE_COOLDOWN, MINE_MAX_ACTIVE, p->pos, dt);
    // Heal
    TrySpawnDeployable(p, ABL_HEAL, DEPLOY_HEAL,
        &p->healCooldown, HEAL_COOLDOWN, HEAL_MAX_ACTIVE, p->pos, dt);

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
            float mouseDist = Vector2Length(toMouse);
            float dist = mouseDist < FLAME_RANGE ? mouseDist : FLAME_RANGE;
            Vector2 target = Vector2Add(p->pos,
                (Vector2){ cosf(baseAngle) * dist, sinf(baseAngle) * dist });
            Vector2 jitter = { (float)GetRandomValue(-FLAME_JITTER, FLAME_JITTER),
                               (float)GetRandomValue(-FLAME_JITTER, FLAME_JITTER) };
            Vector2 patchPos = Vector2Add(target, jitter);
            SpawnDeployable(DEPLOY_FIRE, patchPos);

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

    // --- Blink Dagger (2) ---
    UpdateBlinkDagger(p, toMouse, dt);
}

static void UpdatePlayer(float dt)
{
    Player *p = &g.player;
    Vector2 toMouse = UpdateMouseAim();

    // Compute move direction (shared by movement + dash)
    Vector2 moveDir = { 0, 0 };
    if (IsKeyDown(KEY_W)) moveDir.y -= 1;
    if (IsKeyDown(KEY_S)) moveDir.y += 1;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1;
    if (IsKeyDown(KEY_D)) moveDir.x += 1;
    if (IsGamepadAvailable(GAMEPAD_INDEX)) {
        float lx = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_LEFT_X);
        float ly = GetGamepadAxisMovement(GAMEPAD_INDEX, GAMEPAD_AXIS_LEFT_Y);
        if (fabsf(lx) > GAMEPAD_STICK_DEADZONE) moveDir.x += lx;
        if (fabsf(ly) > GAMEPAD_STICK_DEADZONE) moveDir.y += ly;
    }
    float moveLen = Vector2Length(moveDir);
    if (moveLen > 1.0f) moveDir = Vector2Scale(moveDir, 1.0f / moveLen);

    UpdateMovement(p, moveDir, moveLen, dt);
    UpdateDash(p, moveDir, moveLen, dt);
    UpdateWeapon(p, toMouse, dt);
    UpdateAbilities(p, toMouse, dt);

    // Boundary clamp (base + corridor + combat zone)
    ClampToPlayArea(p);

    // Shadow lag
    if (p->dash.decoyActive) {
        p->shadowPos = p->dash.decoyPos;
    } else {
        p->shadowPos = Vector2Lerp(p->shadowPos, p->pos, SHADOW_LAG * dt);
    }

    if (p->iFrames > 0) p->iFrames -= dt;
}

// enemy AI
static void UpdateEnemies(float dt) {
    Player *p = &g.player;
    
    // --- Pod spawning via phases ---
    bool inBase = (p->pos.x < MAP_LEFT && p->pos.y > STEP_Y) || (p->pos.y > MAP_SIZE);
    if (g.phase == PHASE_COMBAT && !inBase) {
        bool anyAlive = false;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (g.enemies[i].active) { anyAlive = true; break; }
        }
        if (!anyAlive) {
            // Boss check
            if (g.enemiesKilled >= TRAP_SPAWN_KILLS * g.level) {
                g.phase = PHASE_BOSS;
                SpawnBoss(TRAP);
            } else {
                SpawnPod(g.podValue);
                g.podValue++;
            }
        }
    }
    if (g.phase == PHASE_BOSS) {
        bool bossAlive = false;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (g.enemies[i].active) { bossAlive = true; break; }
        }
        if (!bossAlive) g.phase = PHASE_COMBAT;
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
                SpawnVfxTimer(e->pos, EXPLOSION_VFX_DURATION, VFX_EXPLOSION);
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

        // Clamp to combat zone
        e->pos = Vector2Clamp(e->pos,
            (Vector2){MAP_LEFT, 0}, (Vector2){MAP_RIGHT, MAP_SIZE});

        bool stunned = e->stunTimer > 0;

        // Shoot target: turret if aggroed, otherwise player
        Vector2 toShoot = (e->aggroIdx >= 0) ? toTarget : toPlayer;
        float distToShoot = (e->aggroIdx >= 0) ? dist : distToPlayer;

        // Shoot dispatch
        if (!stunned && ENEMY_DEFS[e->type].shoot)
            ENEMY_DEFS[e->type].shoot(e, toShoot, distToShoot, dt);

        // Hit flash decay
        if (e->hitFlash > 0) e->hitFlash -= dt;
        if (e->blinkMark > 0) e->blinkMark -= dt;

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

static void BfgFizzle(Vector2 pos) {
    SpawnParticles(pos, (Color)BFG_COLOR, 4);
    g.player.bfg.active = false;
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
    SpawnVfxTimer(pos, EXPLOSION_VFX_DURATION, VFX_EXPLOSION);
}

static void AoeDamage(Vector2 pos, float radius, int damage,
    float knockback, DamageType dmgType)
{
    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (!g.enemies[j].active) continue;
        Enemy *ej = &g.enemies[j];
        float dist = Vector2Distance(pos, ej->pos);
        if (dist < radius + ej->size) {
            DamageEnemy(j, damage, dmgType, HIT_AOE);
            Vector2 away = Vector2Subtract(ej->pos, pos);
            if (Vector2Length(away) > 1.0f)
                ej->vel = Vector2Scale(Vector2Normalize(away), knockback);
        }
    }
}

static void RocketExplode(Vector2 pos) {
    Player *p = &g.player;
    p->rocket.inFlight = false;

    AoeDamage(pos, ROCKET_EXPLOSION_RADIUS, ROCKET_EXPLOSION_DAMAGE,
        ROCKET_KNOCKBACK, DMG_EXPLOSIVE);

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
    AoeDamage(pos, GRENADE_EXPLOSION_RADIUS, GRENADE_EXPLOSION_DAMAGE,
        GRENADE_KNOCKBACK, DMG_EXPLOSIVE);
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
                case DEPLOY_FIRE:
                    d->timer = FLAME_PATCH_LIFETIME;
                    d->radius = FLAME_PATCH_RADIUS;
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
                SpawnVfxTimer(d->pos, MINE_WEB_DURATION, VFX_MINE_WEB);
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

        case DEPLOY_FIRE: {
            d->actionTimer -= dt;
            if (d->actionTimer <= 0) {
                d->actionTimer = FLAME_PATCH_TICK;
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    Enemy *e = &g.enemies[j];
                    if (!e->active) continue;
                    if (Vector2Distance(d->pos, e->pos) < d->radius + e->size) {
                        DamageEnemy(j,
                            (int)(FLAME_PATCH_DPS * FLAME_PATCH_TICK),
                            DMG_ABILITY, HIT_AOE);
                    }
                }
            }
        } break;

        }
    }

}

// vfx timers -------------------------------------------------------------- /
static void SpawnVfxTimer(Vector2 pos, float duration, VfxTimerType type) {
    for (int i = 0; i < MAX_VFX_TIMERS; i++) {
        if (!g.vfx.timers[i].active) {
            g.vfx.timers[i] = (VfxTimer){
                .pos = pos,
                .timer = duration,
                .duration = duration,
                .active = true,
                .type = type,
            };
            return;
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
            if (b->type == PROJ_BFG) BfgFizzle(b->pos);
            b->active = false;
            continue;
        }

        // map boundary
        if (b->pos.x < 0 || b->pos.x > MAP_RIGHT ||
            b->pos.y < 0 || b->pos.y > BASE_BOTTOM) {
            if (b->type == PROJ_GRENADE && b->bounces > 0) {
                // bounce off map edges
                if (b->pos.x < 0)          { b->pos.x = 0;          b->vel.x = -b->vel.x; }
                if (b->pos.x > MAP_RIGHT)  { b->pos.x = MAP_RIGHT;  b->vel.x = -b->vel.x; }
                if (b->pos.y < 0)          { b->pos.y = 0;          b->vel.y = -b->vel.y; }
                if (b->pos.y > BASE_BOTTOM){ b->pos.y = BASE_BOTTOM; b->vel.y = -b->vel.y; }
                b->vel = Vector2Scale(b->vel, GRENADE_BOUNCE_DAMPING);
                b->bounces--;
            } else {
                if (b->type == PROJ_ROCKET) RocketExplode(b->pos);
                if (b->type == PROJ_GRENADE) GrenadeExplode(b->pos);
                if (b->type == PROJ_BFG) BfgFizzle(b->pos);
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
    DetectInputMode();

    // frametime clamp
    float dt = GetFrameTime();
    if (dt > DT_MAX) dt = DT_MAX;

    if (g.phase == PHASE_SELECT) {
        UpdateSelect(dt);
        // Shared systems still run during select (projectiles from weapon demos, etc.)
        UpdateProjectiles(dt);
        UpdateParticles(dt);
        for (int i = 0; i < MAX_VFX_TIMERS; i++) {
            if (!g.vfx.timers[i].active) continue;
            g.vfx.timers[i].timer -= dt;
            if (g.vfx.timers[i].timer <= 0)
                g.vfx.timers[i].active = false;
        }
        MoveCamera(dt);
        return;
    }

    // need to make sure that esc also pauses in native build
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)
        || IsGamepadButtonPressed(GAMEPAD_INDEX, GAMEPAD_BUTTON_MIDDLE_RIGHT))
        g.paused = !g.paused;

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
        if (IsKeyPressed(KEY_ENTER)
            || IsGamepadButtonPressed(GAMEPAD_INDEX, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
            InitGame();
        return;
    }

    if (g.paused) return;

    // Tick transition fade-in (second half, after screen switch)
    if (g.transitionTimer > 0) {
        g.transitionTimer -= dt;
        if (g.transitionTimer <= 0) g.transitionTimer = 0;
    }

    UpdatePlayer(dt);
    UpdateEnemies(dt);
    UpdateProjectiles(dt);
    UpdateLightningChain(dt);
    UpdateParticles(dt);
    UpdateBeams(dt);
    UpdateDeployables(dt);

    for (int i = 0; i < MAX_VFX_TIMERS; i++) {
        if (!g.vfx.timers[i].active) continue;
        g.vfx.timers[i].timer -= dt;
        if (g.vfx.timers[i].timer <= 0)
            g.vfx.timers[i].active = false;
    }

    MoveCamera(dt);
}
