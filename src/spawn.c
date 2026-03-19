// spawn.c
// where we call the spawns of entities
#include "game.h"

// enemy shoot functions --------------------------------------------------- /
static void ShootRect(Enemy *e, Vector2 toTarget, float dist, float dt) {
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

static void ShootPenta(Enemy *e, Vector2 toTarget, float dist, float dt) {
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

static void ShootHexa(Enemy *e, Vector2 toTarget, float dist, float dt) {
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

static void ShootTrap(Enemy *e, Vector2 toTarget, float dist, float dt) {
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
                TRAP_COLOR, TRAP_MUZZLE_SIZE,
                TRAP_MUZZLE_LIFETIME);
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

static void CircFireWeapon(
    Enemy *e, WeaponType wpn,
    Vector2 shootDir, float baseAngle, Vector2 muzzle)
{
    switch (wpn) {
    case WPN_GUN: {
        e->burstTimer = CIRC_GUN_BURST_DURATION;
        e->burstCooldown = 0;
    } break;
    case WPN_SWORD: {
        e->chargeDir = shootDir;
        e->chargeTimer = CIRC_SWORD_DASH_DURATION;
        e->sweepTimer = -1;  // flag: start sweep when charge ends
    } break;
    case WPN_REVOLVER: {
        e->fanRounds = CIRC_REV_COUNT;
        e->fanTimer = 0;
        e->fanAngle = baseAngle;
    } break;
    case WPN_SNIPER: {
        SpawnProjectile(muzzle, shootDir,
            CIRC_SNIPER_SPEED, CIRC_SNIPER_DAMAGE,
            CIRC_SNIPER_LIFETIME, CIRC_SNIPER_SIZE,
            true, false, PROJ_BULLET, DMG_BALLISTIC);
    } break;
    case WPN_ROCKET: {
        SpawnProjectile(muzzle, shootDir,
            CIRC_ROCKET_SPEED, CIRC_ROCKET_DAMAGE,
            CIRC_ROCKET_LIFETIME, CIRC_ROCKET_SIZE,
            true, false, PROJ_ROCKET, DMG_EXPLOSIVE);
    } break;
    default: break;
    }
    if (wpn != WPN_SWORD) {
        SpawnParticle(muzzle,
            Vector2Scale(shootDir, ENEMY_MUZZLE_SPEED),
            CIRC_COLOR, CIRC_MUZZLE_SIZE,
            CIRC_MUZZLE_LIFETIME);
    }
}

static void ShootCirc(
    Enemy *e, Vector2 toTarget, float dist, float dt)
{
    if (e->chargeTimer > 0) return;
    // Sword sweep — damage player if swept edge reaches them
    if (e->sweepTimer > 0) {
        float progress = 1.0f - (e->sweepTimer / CIRC_SWORD_DURATION);
        e->sweepTimer -= dt;
        float newProgress = 1.0f - (e->sweepTimer / CIRC_SWORD_DURATION);
        if (newProgress > 1.0f) newProgress = 1.0f;
        float dist2 = Vector2Distance(e->pos, g.player.pos);
        if (dist2 < CIRC_SWORD_RADIUS + g.player.size) {
            Vector2 toP = Vector2Subtract(g.player.pos, e->pos);
            float pAngle = atan2f(toP.y, toP.x);
            float diff = fmodf(pAngle - e->sweepAngle + 3*PI,
                2*PI) - PI;
            // Sweep goes from -arc/2 to +arc/2 over duration
            float sweepStart = -CIRC_SWORD_ARC / 2.0f
                + CIRC_SWORD_ARC * progress;
            float sweepEnd = -CIRC_SWORD_ARC / 2.0f
                + CIRC_SWORD_ARC * newProgress;
            if (diff >= sweepStart && diff <= sweepEnd)
                DamagePlayer(CIRC_SWORD_DAMAGE, DMG_SLASH,
                    HIT_MELEE);
        }
        if (e->sweepTimer <= 0) SpawnParticles(e->pos, WHITE, 8);
        return;
    }
    // Fan the hammer burst
    if (e->fanRounds > 0) {
        e->fanTimer -= dt;
        if (e->fanTimer <= 0) {
            float spread = ((float)GetRandomValue(
                -CIRC_REV_SPREAD, CIRC_REV_SPREAD)) * 0.001f;
            float a = e->fanAngle + spread;
            Vector2 dir = { cosf(a), sinf(a) };
            Vector2 m = Vector2Add(e->pos,
                Vector2Scale(dir, e->size + MUZZLE_OFFSET));
            SpawnProjectile(m, dir, CIRC_REV_SPEED,
                CIRC_REV_DAMAGE, CIRC_REV_LIFETIME,
                CIRC_REV_SIZE, true, false,
                PROJ_BULLET, DMG_BALLISTIC);
            SpawnParticle(m, Vector2Scale(dir, ENEMY_MUZZLE_SPEED),
                CIRC_COLOR, CIRC_MUZZLE_SIZE, CIRC_MUZZLE_LIFETIME);
            e->fanRounds--;
            e->fanTimer = CIRC_REV_FAN_COOLDOWN;
        }
        return;
    }
    // Gun burst — sustained fire aimed at player
    if (e->burstTimer > 0) {
        e->burstTimer -= dt;
        e->burstCooldown -= dt;
        if (e->burstCooldown <= 0) {
            Vector2 toP = Vector2Subtract(g.player.pos, e->pos);
            float aimAngle = atan2f(toP.y, toP.x);
            float spread = ((float)GetRandomValue(
                -CIRC_GUN_SPREAD, CIRC_GUN_SPREAD)) * 0.001f;
            float a = aimAngle + spread;
            Vector2 dir = { cosf(a), sinf(a) };
            Vector2 m = Vector2Add(e->pos,
                Vector2Scale(dir, e->size + MUZZLE_OFFSET));
            SpawnProjectile(m, dir, CIRC_GUN_SPEED,
                CIRC_GUN_DAMAGE, CIRC_GUN_LIFETIME,
                CIRC_GUN_SIZE, true, false,
                PROJ_BULLET, DMG_BALLISTIC);
            SpawnParticle(m, Vector2Scale(dir, ENEMY_MUZZLE_SPEED),
                CIRC_COLOR, CIRC_MUZZLE_SIZE, CIRC_MUZZLE_LIFETIME);
            e->burstCooldown = CIRC_GUN_FIRE_RATE;
        }
        return;
    }
    e->shootTimer -= dt;
    if (e->shootTimer <= 0 && dist > 1.0f) {
        // Build unchosen weapons list
        WeaponType weapons[3];
        int wcount = 0;
        for (int w = 0; w < NUM_PRIMARY_WEAPONS; w++) {
            WeaponType wt = SELECT_WEAPONS[w];
            if (wt != g.player.primary
                && wt != g.player.secondary)
                weapons[wcount++] = wt;
        }

        Vector2 shootDir = Vector2Scale(toTarget, 1.0f / dist);
        float baseAngle = atan2f(shootDir.y, shootDir.x);
        Vector2 muzzle = Vector2Add(e->pos,
            Vector2Scale(shootDir, e->size + MUZZLE_OFFSET));

        int pattern = GetRandomValue(0, 7);
        switch (pattern) {
        case 0: case 2: case 4: {
            // Unchosen weapon attack
            int idx = GetRandomValue(0, wcount - 1);
            CircFireWeapon(e, weapons[idx],
                shootDir, baseAngle, muzzle);
        } break;
        case 1: {
            // Ring wave
            float step = 2.0f * PI / CIRC_RING_COUNT;
            for (int b = 0; b < CIRC_RING_COUNT; b++) {
                float a = step * b;
                Vector2 dir = { cosf(a), sinf(a) };
                Vector2 m = Vector2Add(e->pos,
                    Vector2Scale(dir,
                        e->size + MUZZLE_OFFSET));
                SpawnProjectile(m, dir,
                    CIRC_RING_SPEED, CIRC_RING_DAMAGE,
                    CIRC_RING_LIFETIME, CIRC_RING_SIZE,
                    true, false, PROJ_BULLET,
                    DMG_BALLISTIC);
            }
            SpawnParticles(e->pos, CIRC_COLOR, 12);
        } break;
        case 3: {
            // Dash toward player
            e->chargeDir = shootDir;
            e->chargeTimer = CIRC_CHARGE_DURATION;
        } break;
        case 5: {
            // Spiral burst
            float time = (float)GetTime();
            for (int arm = 0; arm < CIRC_SPIRAL_ARMS;
                arm++) {
                float armAngle = 2.0f * PI * arm
                    / CIRC_SPIRAL_ARMS + time;
                for (int b = 0; b < CIRC_SPIRAL_PER_ARM;
                    b++) {
                    float a = armAngle + b * 0.3f;
                    Vector2 dir = { cosf(a), sinf(a) };
                    Vector2 m = Vector2Add(e->pos,
                        Vector2Scale(dir,
                            e->size + MUZZLE_OFFSET));
                    SpawnProjectile(m, dir,
                        CIRC_SPIRAL_SPEED,
                        CIRC_SPIRAL_DAMAGE,
                        CIRC_SPIRAL_LIFETIME,
                        CIRC_SPIRAL_SIZE,
                        true, false, PROJ_BULLET,
                        DMG_BALLISTIC);
                }
            }
            SpawnParticles(e->pos, CIRC_COLOR, 8);
        } break;
        case 6: {
            // Shotgun blast at player
            float halfSpread =
                CIRC_SHOTGUN_SPREAD / 2.0f;
            float step = CIRC_SHOTGUN_SPREAD
                / (CIRC_SHOTGUN_PELLETS - 1);
            for (int b = 0; b < CIRC_SHOTGUN_PELLETS;
                b++) {
                float a = baseAngle - halfSpread
                    + step * b;
                Vector2 dir = { cosf(a), sinf(a) };
                Vector2 m = Vector2Add(e->pos,
                    Vector2Scale(dir,
                        e->size + MUZZLE_OFFSET));
                SpawnProjectile(m, dir,
                    CIRC_SHOTGUN_SPEED,
                    CIRC_SHOTGUN_DAMAGE,
                    CIRC_SHOTGUN_LIFETIME,
                    CIRC_SHOTGUN_SIZE,
                    true, true, PROJ_BULLET,
                    DMG_BALLISTIC);
            }
            SpawnParticle(muzzle,
                Vector2Scale(shootDir,
                    ENEMY_MUZZLE_SPEED),
                ORANGE, CIRC_MUZZLE_SIZE,
                CIRC_MUZZLE_LIFETIME);
        } break;
        case 7: {
            // Ground slam AoE (damages player nearby)
            float playerDist = Vector2Distance(
                e->pos, g.player.pos);
            if (playerDist <= SLAM_RANGE) {
                DamagePlayer(CIRC_SLAM_DAMAGE,
                    DMG_BLUNT, HIT_AOE);
            }
            // Ring of bullets outward after slam
            float step = 2.0f * PI / CIRC_RING_COUNT;
            for (int b = 0; b < CIRC_RING_COUNT; b++) {
                float a = step * b;
                Vector2 dir = { cosf(a), sinf(a) };
                Vector2 m = Vector2Add(e->pos,
                    Vector2Scale(dir,
                        e->size + MUZZLE_OFFSET));
                SpawnProjectile(m, dir,
                    CIRC_RING_SPEED, CIRC_RING_DAMAGE,
                    CIRC_RING_LIFETIME, CIRC_RING_SIZE,
                    true, false, PROJ_BULLET,
                    DMG_BALLISTIC);
            }
            SpawnParticles(e->pos, CIRC_COLOR, 16);
        } break;
        }
        e->shootTimer = CIRC_ATTACK_INTERVAL;
    }
}

// enemy definitions — one row per type
const EnemyDef ENEMY_DEFS[] = {
//              size         hp        spdMin            spdVar
//              contactDmg   spnKills  gold
//              value        shoot
    [TRI]   = { TRI_SIZE,    TRI_HP,   TRI_SPEED_MIN,   TRI_SPEED_VAR,
                TRI_CONTACT_DAMAGE,    0,
                TRI_GOLD,   TRI_VALUE,   NULL },
    [RECT]  = { RECT_SIZE,   RECT_HP,  RECT_SPEED_MIN,  RECT_SPEED_VAR,
                RECT_CONTACT_DAMAGE,   RECT_SPAWN_KILLS,
                RECT_GOLD,  RECT_VALUE,  ShootRect },
    [PENTA] = { PENTA_SIZE,  PENTA_HP, PENTA_SPEED_MIN, PENTA_SPEED_VAR,
                PENTA_CONTACT_DAMAGE,  PENTA_SPAWN_KILLS,
                PENTA_GOLD, PENTA_VALUE, ShootPenta },
    [RHOM]  = { RHOM_SIZE,   RHOM_HP,  RHOM_SPEED_MIN,  RHOM_SPEED_VAR,
                RHOM_CONTACT_DAMAGE,   RHOM_SPAWN_KILLS,
                RHOM_GOLD,  RHOM_VALUE,  NULL },
    [HEXA]  = { HEXA_SIZE,   HEXA_HP,  HEXA_SPEED_MIN,  HEXA_SPEED_VAR,
                HEXA_CONTACT_DAMAGE,   HEXA_SPAWN_KILLS,
                HEXA_GOLD,  HEXA_VALUE,  ShootHexa },
    [OCTA]  = { OCTA_SIZE,   OCTA_HP,  OCTA_SPEED_MIN,  OCTA_SPEED_VAR,
                OCTA_CONTACT_DAMAGE,   OCTA_SPAWN_KILLS,
                OCTA_GOLD,  OCTA_VALUE,  NULL },
    [TRAP]  = { TRAP_SIZE,   TRAP_HP,  TRAP_SPEED_MIN,  TRAP_SPEED_VAR,
                TRAP_CONTACT_DAMAGE,   TRAP_SPAWN_KILLS,
                TRAP_GOLD,  TRAP_VALUE,  ShootTrap },
    [CIRC]  = { CIRC_SIZE,   CIRC_HP,  CIRC_SPEED_MIN,  CIRC_SPEED_VAR,
                CIRC_CONTACT_DAMAGE,   0,
                CIRC_GOLD,  CIRC_VALUE,  ShootCirc },
};

// Spawn Helpers ------------------------------------------------------------ /
Projectile* SpawnProjectile(
    Vector2 pos, Vector2 dir,
    float speed, int damage, float lifetime, float size,
    bool isEnemy, bool knockback,
    ProjectileType type, DamageType dmgType)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!g.projectiles[i].active) {
            Projectile *b = &g.projectiles[i];
            b->active = true;
            b->pos = pos;
            b->vel = Vector2Scale(dir, speed);
            b->lifetime = lifetime;
            b->size = size;
            b->damage = damage;
            b->isEnemy = isEnemy;
            b->knockback = knockback;
            b->appliesSlow = false;
            b->type = type;
            b->dmgType = dmgType;
            return b;
        }
    }
    return NULL;
}

void FireShotgunBlast(Player *p, Vector2 toMouse) {
    Vector2 aimDir = Vector2Normalize(toMouse);
    float baseAngle = atan2f(aimDir.y, aimDir.x);
    float halfSpread = SHOTGUN_SPREAD / 2.0f;
    float step = SHOTGUN_SPREAD / (SHOTGUN_PELLETS - 1);

    for (int i = 0; i < SHOTGUN_PELLETS; i++) {
        float angle = baseAngle - halfSpread + step * i;
        Vector2 dir = { cosf(angle), sinf(angle) };
        Vector2 muzzle = Vector2Add(p->pos,
            Vector2Scale(dir, p->size + MUZZLE_OFFSET));
        Projectile *pellet = SpawnProjectile(muzzle, dir,
            SHOTGUN_BULLET_SPEED, SHOTGUN_DAMAGE,
            SHOTGUN_BULLET_LIFETIME, SHOTGUN_PROJECTILE_SIZE, false, true,
            PROJ_BULLET, DMG_BALLISTIC);
        if (pellet) pellet->bounces = SHOTGUN_BOUNCES;
    }

    Vector2 muzzle = Vector2Add(p->pos,
        Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
    SpawnParticle(muzzle,
        Vector2Scale(aimDir, SHOTGUN_MUZZLE_SPEED), ORANGE,
        SHOTGUN_MUZZLE_SIZE, SHOTGUN_MUZZLE_LIFETIME);
    SpawnParticles(muzzle, YELLOW, HIT_PARTICLES);
}

void SpawnRocket(Player *p, Vector2 toMouse) {
    Vector2 aimDir = Vector2Normalize(toMouse);
    Vector2 muzzle = Vector2Add(p->pos,
        Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
    Projectile *r = SpawnProjectile(muzzle, aimDir,
        ROCKET_SPEED, ROCKET_DIRECT_DAMAGE,
        ROCKET_LIFETIME, ROCKET_PROJECTILE_SIZE, false, true,
        PROJ_ROCKET, DMG_EXPLOSIVE);
    (void)r;
    // muzzle flash
    SpawnParticles(muzzle, RED, ROCKET_MUZZLE_PARTICLES);
    SpawnParticle(muzzle,
        Vector2Scale(aimDir, ROCKET_MUZZLE_SPEED), ORANGE,
        ROCKET_MUZZLE_SIZE, ROCKET_MUZZLE_LIFETIME);
    SpawnParticles(muzzle, YELLOW, HIT_PARTICLES);
    // backblast
    SpawnParticles(p->pos, GRAY, ROCKET_BACKBLAST_PARTICLES);
}

void SpawnGrenade(Player *p, Vector2 toMouse) {
    Vector2 aimDir = Vector2Normalize(toMouse);
    Vector2 muzzle = Vector2Add(p->pos,
        Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
    Projectile *gr = SpawnProjectile(muzzle, aimDir,
        GRENADE_SPEED, GRENADE_DIRECT_DAMAGE,
        GRENADE_LIFETIME, GRENADE_PROJECTILE_SIZE, false, true,
        PROJ_GRENADE, DMG_EXPLOSIVE);
    if (gr) {
        gr->bounces = GRENADE_MAX_BOUNCES;
        gr->height = 0.0f;
        gr->heightVel = GRENADE_LAUNCH_HEIGHT_VEL;
    }
    // muzzle flash
    SpawnParticles(muzzle, GRENADE_GLOW_COLOR, GRENADE_MUZZLE_PARTICLES);
    SpawnParticle(muzzle,
        Vector2Scale(aimDir, GRENADE_MUZZLE_SPEED), GRENADE_GLOW_COLOR,
        GRENADE_MUZZLE_SIZE, GRENADE_MUZZLE_LIFETIME);
}

// spawn helpers ------------------------------------------------------------- /
static void InitEnemy(Enemy *e) {
    e->active = true;
    e->hitFlash = 0;
    e->shootTimer = 0;
    e->slowTimer = 0;
    e->slowFactor = 1.0f;
    e->rootTimer = 0;
    e->stunTimer = 0;
    e->aggroIdx = -1;
    e->attackPhase = 0;
    e->chargeTimer = 0;
    e->chargeDir = (Vector2){ 0, 0 };
    e->vel = (Vector2){ 0, 0 };
}

static void SpawnAtEdge(Enemy *e) {
    int edge = GetRandomValue(0, 3);
    switch (edge) {
        case 0: e->pos.x = (float)GetRandomValue((int)MAP_LEFT, (int)MAP_RIGHT);
                e->pos.y = g.player.pos.y - SPAWN_MARGIN; break;
        case 1: e->pos.x = (float)GetRandomValue((int)MAP_LEFT, (int)MAP_RIGHT);
                e->pos.y = g.player.pos.y + SPAWN_MARGIN; break;
        case 2: e->pos.x = g.player.pos.x - SPAWN_MARGIN;
                e->pos.y = (float)GetRandomValue(0, (int)MAP_SIZE); break;
        case 3: e->pos.x = g.player.pos.x + SPAWN_MARGIN;
                e->pos.y = (float)GetRandomValue(0, (int)MAP_SIZE); break;
    }
    e->pos = Vector2Clamp(e->pos,
        (Vector2){MAP_LEFT, 0}, (Vector2){MAP_RIGHT, MAP_SIZE});
}

static void FillFromDef(Enemy *e, EnemyType type) {
    const EnemyDef *d = &ENEMY_DEFS[type];
    e->type          = type;
    e->size          = d->size;
    e->hp            = d->hp;
    e->maxHp         = d->hp;
    e->contactDamage = d->contactDamage;
    e->gold          = d->gold;
    e->value         = d->value;
}

static const EnemyType SPAWNABLE[] = {
    TRI, RECT, RHOM, OCTA, PENTA, HEXA, TRAP
};
#define SPAWNABLE_COUNT 7

static EnemyType PickEnemyType(void) {
    EnemyType eligible[SPAWNABLE_COUNT];
    int count = 0;
    for (int t = 0; t < SPAWNABLE_COUNT; t++) {
        if (g.enemiesKilled >= ENEMY_DEFS[SPAWNABLE[t]].spawnKills)
            eligible[count++] = SPAWNABLE[t];
    }
    if (count == 0) return TRI;
    return eligible[GetRandomValue(0, count - 1)];
}

void SpawnPod(int podValue)
{
    int remaining = podValue;
    while (remaining > 0) {
        // find a free slot
        int slot = -1;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g.enemies[i].active) { slot = i; break; }
        }
        if (slot < 0) return; // pool full

        EnemyType type = PickEnemyType();
        Enemy *e = &g.enemies[slot];
        InitEnemy(e);
        FillFromDef(e, type);
        e->speed = ENEMY_DEFS[type].speedMin
            + (float)GetRandomValue(0, ENEMY_DEFS[type].speedVar);
        SpawnAtEdge(e);
        remaining -= e->value;
    }
}

void SpawnBoss(EnemyType type)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g.enemies[i].active) {
            Enemy *e = &g.enemies[i];
            InitEnemy(e);
            FillFromDef(e, type);
            e->speed = ENEMY_DEFS[type].speedMin;
            SpawnAtEdge(e);
            return;
        }
    }
}

// particles will have diff properties of size, angle and speed eventually
void SpawnParticle(
    Vector2 pos, Vector2 vel,
    Color color, float size, float lifetime)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g.vfx.particles[i].active) {
            g.vfx.particles[i].active = true;
            g.vfx.particles[i].pos = pos;
            g.vfx.particles[i].vel = vel;
            g.vfx.particles[i].color = color;
            g.vfx.particles[i].size = size;
            g.vfx.particles[i].lifetime = lifetime;
            g.vfx.particles[i].maxLifetime = lifetime;
            return;
        }
    }
}

void SpawnParticles(Vector2 pos, Color color, int count)
{
    for (int i = 0; i < count; i++) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(
            PARTICLE_BURST_SPEED_MIN, PARTICLE_BURST_SPEED_MAX);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        float size = (float)GetRandomValue(
            PARTICLE_BURST_SIZE_MIN, PARTICLE_BURST_SIZE_MAX);
        SpawnParticle(pos, vel, color, size, PARTICLE_BURST_LIFETIME);
    }
}

void SpawnBeam(Vector2 origin, Vector2 tip,
    float duration, Color color, float width)
{
    for (int i = 0; i < MAX_BEAMS; i++) {
        Beam *b = &g.vfx.beams[i];
        if (b->active) continue;
        b->origin   = origin;
        b->tip      = tip;
        b->timer    = duration;
        b->duration = duration;
        b->color    = color;
        b->width    = width;
        b->active   = true;
        return;
    }
}

// to scale enemy types, each enemy has an enum type which we switch on?
// then calculate damage?
// assuming more complex damage calculation is down the line
// like in a real game, types of damage exist, armor and resist of enemy, etc
// diff weapons, diff damage! damage comes in as a raw number though?
// also needs its type? and context?
// getting ahead of ourselves here
void DamageEnemy(int idx, int damage, DamageType dmgType, DamageMethod method)
{
    (void)dmgType; (void)method; // threaded through for future resistances
    Enemy *e = &g.enemies[idx];
    e->hp -= damage;
    e->hitFlash = HIT_FLASH_DURATION;
    SpawnParticles(e->pos, WHITE, HIT_PARTICLES);

    // charge BFG from damage dealt (only when not active)
    if (!g.player.bfg.active) {
        g.player.bfg.charge += damage;
        if (g.player.bfg.charge > BFG_CHARGE_COST)
            g.player.bfg.charge = BFG_CHARGE_COST;
    }

    if (e->hp <= 0) {
        e->active = false;
        g.gold += e->gold;
        g.enemiesKilled++;
        // Boss kill — advance level
        if (e->type == CIRC) {
            g.level++;
            g.phase = PHASE_COMBAT;
            // big white explosion
            SpawnParticles(e->pos, WHITE, DEATH_PARTICLES * 4);
        }
        // fire/explosion
        SpawnParticles(e->pos, RED, DEATH_PARTICLES);
        SpawnParticles(e->pos, ORANGE, DEATH_PARTICLES);
        SpawnParticles(e->pos, YELLOW, DEATH_PARTICLES);
    }
}

void DamagePlayer(int damage, DamageType dmgType, DamageMethod method)
{
    (void)dmgType; (void)method; // threaded through for future resistances
    Player *p = &g.player;
    if (g.invincible || p->iFrames > 0) return;
    p->hp -= damage;
    p->iFrames = IFRAME_DURATION;
    if (p->blink.cooldown <= 0)
        p->blink.cooldown = BLINK_COOLDOWN_HIT;
    SpawnParticles(p->pos, RED, CONTACT_HIT_PARTICLES);
    if (p->hp <= 0) {
        p->hp = 0;
        g.gameOver = true;
    }
}
