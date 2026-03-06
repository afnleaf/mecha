#include "mecha.h"

// so we can statically create the enemy definitions
static const EnemyDef ENEMY_DEFS[] = {
//              size         hp        spdMin            spdVar
//              contactDmg   spnKills  spnChance         score
    [TRI]   = { TRI_SIZE,    TRI_HP,   TRI_SPEED_MIN,   TRI_SPEED_VAR,
                TRI_CONTACT_DAMAGE,    0,               0,              
                TRI_SCORE   },
    [RECT]  = { RECT_SIZE,   RECT_HP,  RECT_SPEED_MIN,  RECT_SPEED_VAR,
                RECT_CONTACT_DAMAGE,   RECT_SPAWN_KILLS, RECT_SPAWN_CHANCE, 
                RECT_SCORE },
    [PENTA] = { PENTA_SIZE,  PENTA_HP, PENTA_SPEED_MIN, PENTA_SPEED_VAR,
                PENTA_CONTACT_DAMAGE,  PENTA_SPAWN_KILLS,PENTA_SPAWN_CHANCE,
                PENTA_SCORE},
    [RHOM]  = { RHOM_SIZE,   RHOM_HP,  RHOM_SPEED_MIN,  RHOM_SPEED_VAR,
                RHOM_CONTACT_DAMAGE,   RHOM_SPAWN_KILLS, RHOM_SPAWN_CHANCE,
                RHOM_SCORE },
    [HEXA]  = { HEXA_SIZE,   HEXA_HP,  HEXA_SPEED_MIN,  HEXA_SPEED_VAR,
                HEXA_CONTACT_DAMAGE,   HEXA_SPAWN_KILLS, HEXA_SPAWN_CHANCE,
                HEXA_SCORE },
    [OCTA]  = { OCTA_SIZE,   OCTA_HP,  OCTA_SPEED_MIN,  OCTA_SPEED_VAR,
                OCTA_CONTACT_DAMAGE,   OCTA_SPAWN_KILLS, OCTA_SPAWN_CHANCE, 
                OCTA_SCORE },
};

// checked in order, first to pass wins. TRI is fallback.
static const EnemyType SPAWN_PRIORITY[] = { PENTA, OCTA, HEXA, RHOM, RECT };
#define SPAWN_PRIORITY_COUNT 5

// the static struct and the emscripten void requirement are important
// futher study needed
static GameState g;

static void RocketExplode(Vector2 pos);

// ========================================================================== /
// Init
// ========================================================================== /
// this is quite important.
static void InitGame(void)
{
    memset(&g, 0, sizeof(g));

    Player *p  = &g.player;
    p->pos     = (Vector2){ MAP_SIZE / 2.0f, MAP_SIZE / 2.0f };
    p->speed   = PLAYER_SPEED;
    p->size    = PLAYER_SIZE;
    p->hp      = PLAYER_HP;
    p->maxHp   = PLAYER_HP;

    p->gun.fireRate         = GUN_FIRE_RATE;
    p->sword.duration       = SWORD_DURATION;
    p->sword.arc            = SWORD_ARC;
    p->sword.radius         = SWORD_RADIUS;
    p->dash.speed           = DASH_SPEED;
    p->dash.duration        = DASH_DURATION;
    p->dash.rechargeTime    = DASH_COOLDOWN;
    p->dash.charges         = DASH_MAX_CHARGES;
    p->dash.maxCharges      = DASH_MAX_CHARGES;
    p->spin.duration        = SPIN_DURATION;
    p->spin.radius          = SPIN_RADIUS;
    p->spin.cooldown        = SPIN_COOLDOWN;
    p->rocket.cooldown      = ROCKET_COOLDOWN;
    p->shotgun.blastsLeft   = SHOTGUN_BLASTS;
    p->revolver.rounds      = REVOLVER_ROUNDS;
    p->shield.hp            = SHIELD_MAX_HP;
    p->shield.maxHp         = SHIELD_MAX_HP;
    p->flame.fuel           = FLAME_FUEL_MAX;

    // ability slots — default layout
    p->slots[0]  = (AbilitySlot){ ABL_SHOTGUN,  KEY_Q };
    p->slots[1]  = (AbilitySlot){ ABL_RAILGUN,  KEY_E };
    p->slots[2]  = (AbilitySlot){ ABL_SPIN,     KEY_LEFT_SHIFT };
    p->slots[3]  = (AbilitySlot){ ABL_SLAM,     KEY_ONE };
    p->slots[4]  = (AbilitySlot){ ABL_PARRY,    KEY_TWO };
    p->slots[5]  = (AbilitySlot){ ABL_TURRET,   KEY_THREE };
    p->slots[6]  = (AbilitySlot){ ABL_MINE,     KEY_FOUR };
    p->slots[7]  = (AbilitySlot){ ABL_HEAL,     KEY_Z };
    p->slots[8]  = (AbilitySlot){ ABL_SHIELD,   KEY_X };
    p->slots[9]  = (AbilitySlot){ ABL_GRENADE,  KEY_C };
    p->slots[10] = (AbilitySlot){ ABL_FIRE,     KEY_V };
    p->slots[11] = (AbilitySlot){ ABL_BFG,      KEY_F };

    p->shadowPos            = p->pos;

    g.camera.offset   = (Vector2){ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    g.camera.target   = p->pos;
    g.camera.zoom     = 1.0f;

    g.spawnInterval   = SPAWN_INTERVAL;
    g.spawnTimer      = SPAWN_INITIAL_DELAY;

    g.screen          = SCREEN_SELECT;
    g.selectIndex     = 0;
    g.selectPhase     = 0;
}

// ========================================================================== /
// Spawn Helpers
// ========================================================================== /
static Projectile* SpawnProjectile(
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
            b->type = type;
            b->dmgType = dmgType;
            return b;
        }
    }
    return NULL;
}

static void FireShotgunBlast(Player *p, Vector2 toMouse) {
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

static void SpawnRocket(Player *p, Vector2 toMouse) {
    Vector2 aimDir = Vector2Normalize(toMouse);
    Vector2 muzzle = Vector2Add(p->pos,
        Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
    Vector2 worldTarget = Vector2Add(p->pos, toMouse);
    Projectile *r = SpawnProjectile(muzzle, aimDir,
        ROCKET_SPEED, ROCKET_DIRECT_DAMAGE,
        ROCKET_LIFETIME, ROCKET_PROJECTILE_SIZE, false, true,
        PROJ_ROCKET, DMG_EXPLOSIVE);
    if (r) {
        r->target = worldTarget;
        p->rocket.inFlight = true;
    }
    // muzzle flash
    SpawnParticles(muzzle, RED, ROCKET_MUZZLE_PARTICLES);
    SpawnParticle(muzzle,
        Vector2Scale(aimDir, ROCKET_MUZZLE_SPEED), ORANGE,
        ROCKET_MUZZLE_SIZE, ROCKET_MUZZLE_LIFETIME);
    SpawnParticles(muzzle, YELLOW, HIT_PARTICLES);
    // backblast
    SpawnParticles(p->pos, GRAY, ROCKET_BACKBLAST_PARTICLES);
}

static void SpawnGrenade(Player *p, Vector2 toMouse) {
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

// this of course will need a lot of work
// serves well for the mechanics practice / design phase
// maybe we should scale up the number of enemies that spawn as time goes on?
// or just make it slightly more likely to spawn?
// the hardcoded positions for spawning are bad form like 500 around?
// should be dependent on the game map? or does it matter rn?
static void SpawnEnemy(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g.enemies[i].active) {
            Enemy *e = &g.enemies[i];
            e->active = true;
            e->hitFlash = 0;
            e->shootTimer = 0;
            e->slowTimer = 0;
            e->slowFactor = 1.0f;
            e->rootTimer = 0;
            e->stunTimer = 0;
            e->aggroIdx = -1;

            // Roll for enemy type — priority table, TRI fallback
            EnemyType type = TRI;
            for (int t = 0; t < SPAWN_PRIORITY_COUNT; t++) {
                const EnemyDef *d = &ENEMY_DEFS[SPAWN_PRIORITY[t]];
                if (g.enemiesKilled >= d->spawnKills
                    && GetRandomValue(1, 100) <= d->spawnChance) {
                    type = SPAWN_PRIORITY[t];
                    break;
                }
            }
            const EnemyDef *d = &ENEMY_DEFS[type];
            e->type          = type;
            e->size          = d->size;
            e->speed         = d->speedMin
                + (float)GetRandomValue(0, d->speedVar);
            e->hp            = d->hp;
            e->maxHp         = d->hp;
            e->contactDamage = d->contactDamage;
            e->score         = d->score;

            // Spawn on random map edge, biased toward player vicinity
            // clamp to map edges 0 -> 1600
            int edge = GetRandomValue(0, 3);
            float margin = SPAWN_MARGIN;
            switch (edge) {
                case 0: // top
                    //e->pos.x = g.player.pos.x 
                    //    + (float)GetRandomValue(-500, 500);
                    e->pos.x = (float)GetRandomValue(0, MAP_SIZE);
                    e->pos.y = g.player.pos.y - margin;
                    break;
                case 1: // bottom
                    //e->pos.x = g.player.pos.x 
                    //    + (float)GetRandomValue(-500, 500);
                    e->pos.x = (float)GetRandomValue(0, MAP_SIZE);
                    e->pos.y = g.player.pos.y + margin;
                    break;
                case 2: // left
                    e->pos.x = g.player.pos.x - margin;
                    e->pos.y = (float)GetRandomValue(0, MAP_SIZE);
                    //e->pos.y = g.player.pos.y 
                    //    + (float)GetRandomValue(-500, 500);
                    break;
                case 3: // right
                    e->pos.x = g.player.pos.x + margin;
                    e->pos.y = (float)GetRandomValue(0, MAP_SIZE);
                    //e->pos.y = g.player.pos.y 
                    //    + (float)GetRandomValue(-500, 500);
                    break;
            }
            // Clamp to map
            if (e->pos.x < 0) e->pos.x = 0;
            if (e->pos.x > MAP_SIZE) e->pos.x = MAP_SIZE;
            if (e->pos.y < 0) e->pos.y = 0;
            if (e->pos.y > MAP_SIZE) e->pos.y = MAP_SIZE;

            e->vel = (Vector2){ 0, 0 };
            return;
        }
    }
}

// particles will have diff properties of size, angle and speed eventually
static void SpawnParticle(
    Vector2 pos, Vector2 vel, 
    Color color, float size, float lifetime)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g.particles[i].active) {
            g.particles[i].active = true;
            g.particles[i].pos = pos;
            g.particles[i].vel = vel;
            g.particles[i].color = color;
            g.particles[i].size = size;
            g.particles[i].lifetime = lifetime;
            g.particles[i].maxLifetime = lifetime;
            return;
        }
    }
}

static void SpawnParticles(Vector2 pos, Color color, int count)
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

static void SpawnBeam(Vector2 origin, Vector2 tip,
    float duration, Color color, float width)
{
    for (int i = 0; i < MAX_BEAMS; i++) {
        Beam *b = &g.beams[i];
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
static void DamageEnemy(int idx, int damage, DamageType dmgType, DamageMethod method)
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
        g.score += e->score;
        g.enemiesKilled++;
        // fire/explosion
        SpawnParticles(e->pos, RED, DEATH_PARTICLES);
        SpawnParticles(e->pos, ORANGE, DEATH_PARTICLES);
        SpawnParticles(e->pos, YELLOW, DEATH_PARTICLES);
    }
}

static void DamagePlayer(int damage, DamageType dmgType, DamageMethod method)
{
    (void)dmgType; (void)method; // threaded through for future resistances
    Player *p = &g.player;
    if (p->iFrames > 0) return;
    p->hp -= damage;
    p->iFrames = IFRAME_DURATION;
    SpawnParticles(p->pos, RED, CONTACT_HIT_PARTICLES);
    if (p->hp <= 0) {
        p->hp = 0;
        g.gameOver = true;
    }
}

// ========================================================================== /
// Check if point is inside a swing arc
// ========================================================================== /
// what is C convetion for multi line function signature?
// Line segment vs circle: does segment AB touch circle at C with radius r?
static bool LineSegCircle(
    Vector2 a, Vector2 b, Vector2 c, float r)
{
    Vector2 ab = Vector2Subtract(b, a);
    Vector2 ac = Vector2Subtract(c, a);
    float ab2 = Vector2DotProduct(ab, ab);
    if (ab2 < 1e-8f) return Vector2DistanceSqr(a, c) <= r * r;
    float t = Vector2DotProduct(ac, ab) / ab2;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    Vector2 closest = Vector2Add(a, Vector2Scale(ab, t));
    return Vector2DistanceSqr(closest, c) <= r * r;
}

// Line segment vs OBB: transform to local space, then slab test
static bool LineSegOBB(
    Vector2 la, Vector2 lb,
    Vector2 center, float hw, float hh, float angle)
{
    float ca = cosf(angle), sa = sinf(angle);
    Vector2 da = Vector2Subtract(la, center);
    Vector2 db = Vector2Subtract(lb, center);
    // rotate into OBB local space
    Vector2 a = {  da.x * ca + da.y * sa, -da.x * sa + da.y * ca };
    Vector2 b = {  db.x * ca + db.y * sa, -db.x * sa + db.y * ca };
    // either endpoint inside box
    if (fabsf(a.x) <= hw && fabsf(a.y) <= hh) return true;
    if (fabsf(b.x) <= hw && fabsf(b.y) <= hh) return true;
    // parametric slab clipping
    Vector2 d = Vector2Subtract(b, a);
    float tmin = 0.0f, tmax = 1.0f;
    if (fabsf(d.x) > 1e-8f) {
        float t1 = (-hw - a.x) / d.x;
        float t2 = ( hw - a.x) / d.x;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        tmin = fmaxf(tmin, t1);
        tmax = fminf(tmax, t2);
        if (tmin > tmax) return false;
    } else if (fabsf(a.x) > hw) return false;
    if (fabsf(d.y) > 1e-8f) {
        float t1 = (-hh - a.y) / d.y;
        float t2 = ( hh - a.y) / d.y;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        tmin = fmaxf(tmin, t1);
        tmax = fminf(tmax, t2);
        if (tmin > tmax) return false;
    } else if (fabsf(a.y) > hh) return false;
    return true;
}

// ========================================================================== /
// OBB collision helpers (for RECT enemy hitbox)
// ========================================================================== /
// Get the enemy facing angle (faces aggro target or player shadow)
static float EnemyAngle(Enemy *e) {
    Vector2 target = (e->aggroIdx >= 0)
        ? g.deployables[e->aggroIdx].pos : g.player.shadowPos;
    Vector2 toTarget = Vector2Subtract(target, e->pos);
    return atan2f(toTarget.y, toTarget.x);
}

// Check if a point is inside an oriented bounding box
static bool PointInOBB(
    Vector2 point, Vector2 center,
    float hw, float hh, float angle)
{
    float ca = cosf(angle), sa = sinf(angle);
    Vector2 d = Vector2Subtract(point, center);
    float localX =  d.x * ca + d.y * sa;
    float localY = -d.x * sa + d.y * ca;
    return fabsf(localX) <= hw && fabsf(localY) <= hh;
}

// Check if a circle overlaps an oriented bounding box
// refactor this signature to each same type on same line
static bool CircleOBBOverlap(
    Vector2 circlePos, float radius,
    Vector2 center, float hw, float hh, float angle)
{
    float ca = cosf(angle), sa = sinf(angle);
    Vector2 d = Vector2Subtract(circlePos, center);
    float localX =  d.x * ca + d.y * sa;
    float localY = -d.x * sa + d.y * ca;
    // Closest point on the box to the circle center
    float cx = fmaxf(-hw, fminf(localX, hw));
    float cy = fmaxf(-hh, fminf(localY, hh));
    float dx = localX - cx, dy = localY - cy;
    return (dx * dx + dy * dy) <= radius * radius;
}

// ========================================================================== /
// Collision Dispatchers — switch on enemy type, one case per shape
// ========================================================================== /
// Sweep line (sword, spin): does segment AB intersect enemy hitbox?
// this does all the hitscan damage detection too because it makes contact
static bool EnemyHitSweep(Enemy *e, Vector2 a, Vector2 b) {
    switch (e->type) {
    case RECT:
        return LineSegOBB(a, b, e->pos,
            e->size, e->size * RECT_ASPECT_RATIO, EnemyAngle(e));
    default:
        return LineSegCircle(a, b, e->pos, e->size);
    }
}

// we should make it clear in the comments above these functions exactly what 
// weapons they do damage detection for
// Point test with padding (bullets)
static bool EnemyHitPoint(Enemy *e, Vector2 point, float pad) {
    switch (e->type) {
    case RECT:
        return PointInOBB(point, e->pos,
            e->size + pad, e->size * RECT_ASPECT_RATIO + pad,
            EnemyAngle(e));
    default: {
        float dist = Vector2Distance(point, e->pos);
        return dist < e->size + pad;
    }
    }
}

// Circle overlap (contact damage)
static bool EnemyHitCircle(Enemy *e, Vector2 center, float radius) {
    switch (e->type) {
    case RECT:
        return CircleOBBOverlap(center, radius, e->pos,
            e->size, e->size * RECT_ASPECT_RATIO, EnemyAngle(e));
    default: {
        float dist = Vector2Distance(center, e->pos);
        return dist < radius + e->size;
    }
    }
}

// ========================================================================== /
// Update
// ========================================================================== /

// ========================================================================== /
// Hitscan
// ========================================================================== /
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
                SpawnParticles(surfacePt, WHITE, 4);
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

static const char* AbilityName(AbilityID id) {
    switch (id) {
        case ABL_SHOTGUN: return "Shotgun";
        case ABL_SPIN:    return "Spin";
        case ABL_GRENADE: return "Grenade";
        case ABL_RAILGUN: return "Railgun";
        case ABL_BFG:     return "BFG";
        case ABL_SHIELD:  return "Shield";
        case ABL_TURRET:  return "Turret";
        case ABL_MINE:    return "Mine";
        case ABL_SLAM:    return "Slam";
        case ABL_PARRY:   return "Parry";
        case ABL_HEAL:    return "Heal";
        case ABL_FIRE:    return "Flame";
        default:          return NULL;
    }
}

static const char* KeyName(int key) {
    switch (key) {
        case KEY_Q:           return "Q";
        case KEY_E:           return "E";
        case KEY_F:           return "F";
        case KEY_Z:           return "Z";
        case KEY_X:           return "X";
        case KEY_C:           return "C";
        case KEY_V:           return "V";
        case KEY_LEFT_SHIFT:  return "Shift";
        case KEY_LEFT_CONTROL: return "Ctrl";
        case KEY_ONE:         return "1";
        case KEY_TWO:         return "2";
        case KEY_THREE:       return "3";
        case KEY_FOUR:        return "4";
        default:              return "?";
    }
}

static const char* WeaponName(WeaponType w) {
    switch (w) {
        case WPN_GUN:      return "GUN";
        case WPN_SWORD:    return "SWORD";
        case WPN_REVOLVER: return "REVLVR";
        case WPN_SNIPER:   return "SNIPER";
        case WPN_ROCKET:   return "ROCKET";
        default:           return "???";
    }
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
static Vector2 mouse() {
    Player *p = &g.player;
    // ok I see now, this could be out? 
    // we need toMouse as output? so a mouse function?
    // --- Mouse aim ---
    // this takes the position of the mouse and the camera
    // trace a ray between them, figure out where mouse is
    // then you have the player position in the world
    // we should draw this out on paper to understand better
    Vector2 screenMouse = GetMousePosition();
    Vector2 worldMouse = GetScreenToWorld2D(screenMouse, g.camera);
    Vector2 toMouse = Vector2Subtract(worldMouse, p->pos);
    p->angle = atan2f(toMouse.y, toMouse.x);
    return toMouse;
}

// ========================================================================== /
// Weapon Select Screen
// ========================================================================== /
static void UpdateSelect(void)
{
    // Display order: SWORD, REVOLVER, GUN, SNIPER, ROCKET (face count ascending)
    WeaponType selectWeapons[] = {
        WPN_SWORD, WPN_REVOLVER, WPN_GUN, WPN_SNIPER, WPN_ROCKET
    };

    // Navigation — skip already-chosen primary in phase 1
    int dir = 0;
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))   dir = -1;
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  dir = 1;
    if (dir) {
        int next = g.selectIndex;
        for (int tries = 0; tries < 5; tries++) {
            next = (next + dir + 5) % 5;
            if (g.selectPhase == 1
                && selectWeapons[next] == g.player.primary)
                continue;
            break;
        }
        g.selectIndex = next;
    }

    if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (g.selectPhase == 0) {
            g.player.primary = selectWeapons[g.selectIndex];
            g.selectPhase = 1;
            // Move cursor to next available slot
            int next = (g.selectIndex + 1) % 5;
            if (selectWeapons[next] == g.player.primary)
                next = (next + 1) % 5;
            g.selectIndex = next;
        } else {
            g.player.secondary = selectWeapons[g.selectIndex];
            g.screen = SCREEN_PLAYING;
        }
    }
}

static void DrawSelect(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float ui = (float)sh / HUD_SCALE_REF;

    BeginDrawing();
    ClearBackground(SELECT_BG_COLOR);

    // Title — changes per phase
    int titleFont = (int)(SELECT_TITLE_FONT * ui);
    const char *title = g.selectPhase == 0
        ? "CHOOSE PRIMARY" : "CHOOSE SECONDARY";
    int titleW = MeasureText(title, titleFont);
    int titleY = (int)(sh * SELECT_TITLE_Y);
    DrawText(title, sw / 2 - titleW / 2, titleY, titleFont, WHITE);

    // Hint (below title)
    int hintFont = (int)(SELECT_HINT_FONT * ui);
    const char *hint = "W/S to navigate - Enter or M1 to confirm";
    int hintW = MeasureText(hint, hintFont);
    DrawText(hint, sw / 2 - hintW / 2,
        titleY + titleFont + (int)(SELECT_HINT_GAP * ui), hintFont, Fade(WHITE, 0.5f));

    // Keybinds — two columns on left side
    const char *coreKeys[]  = { "WASD", "Space", "M1", "M2", "Ctrl", "P/Esc", "0" };
    const char *coreDescs[] = { "Move", "Dash", "Primary", "Alt Fire", "Swap Weapon", "Pause", "Exit" };
    int coreCount = 7;
    int keyFont = (int)(SELECT_KEYS_FONT * ui);
    int keySpacing = (int)(SELECT_KEYS_SPACING * ui);
    int keyX = (int)(sw * SELECT_KEYS_X);
    int keyBaseY = (int)(sh * SELECT_KEYS_Y);
    int keyTabW = (int)(60 * ui);

    // Column 1: core controls
    for (int i = 0; i < coreCount; i++) {
        int ky = keyBaseY + i * keySpacing;
        DrawText(coreKeys[i], keyX, ky, keyFont, SELECT_HIGHLIGHT_COLOR);
        DrawText(coreDescs[i], keyX + keyTabW, ky, keyFont, Fade(WHITE, 0.6f));
    }

    // Column 2: abilities (starts to the right of column 1)
    int ablX = keyX + (int)(160 * ui);
    Player *sp = &g.player;
    int ablRow = 0;
    for (int i = 0; i < ABILITY_SLOTS; i++) {
        const char *name = AbilityName(sp->slots[i].ability);
        if (!name) continue;
        int ky = keyBaseY + ablRow * keySpacing;
        DrawText(KeyName(sp->slots[i].key), ablX, ky, keyFont, SELECT_HIGHLIGHT_COLOR);
        DrawText(name, ablX + keyTabW, ky, keyFont, Fade(WHITE, 0.6f));
        ablRow++;
    }

    // Weapons (right column) — ordered by solid face count (4→6→8→12→20)
    const char *names[] = { "SWORD", "REVOLVER", "MACHINE GUN", "SNIPER", "ROCKET" };
    const char *descs[] = {
        "M1 sweep, M2 lunge, dash slash",
        "M1 single, M2 fan hammer, dash reload",
        "M1 burst, M2 minigun, overheat QTE",
        "M1 hip fire, M2 ADS, dash super shot",
        "M1 rocket, M2 detonate, rocket jump",
    };
    // Map display index → WeaponType
    WeaponType selectWeapons[] = {
        WPN_SWORD, WPN_REVOLVER, WPN_GUN, WPN_SNIPER, WPN_ROCKET
    };
    int optFont = (int)(SELECT_OPTION_FONT * ui);
    int descFont = (int)(SELECT_DESC_FONT * ui);
    int spacing = (int)(SELECT_OPTION_SPACING * ui);
    int baseY = (int)(sh * SELECT_OPTIONS_Y);
    int weaponRightX = (int)(sw * SELECT_WEAPONS_X);

    // Draw function per display slot (face count order)
    typedef void (*DrawSolidFn)(Vector2, float, float, float, float, Vector2, float);
    DrawSolidFn solidFns[] = {
        DrawTetra2D,    // Tetrahedron  (4)  — SWORD
        DrawCube2D,     // Cube         (6)  — REVOLVER
        DrawOcta2D,     // Octahedron   (8)  — GUN
        DrawDodeca2D,   // Dodecahedron (12) — SNIPER
        DrawIcosa2D,    // Icosahedron  (20) — ROCKET
    };

    for (int i = 0; i < 5; i++) {
        int y = baseY + i * spacing;
        // Phase 1: gray out already-chosen primary
        bool locked = (g.selectPhase == 1
            && selectWeapons[i] == g.player.primary);
        Color nameColor, descColor;
        if (locked) {
            nameColor = Fade(GREEN, 0.4f);
            descColor = Fade(GREEN, 0.25f);
        } else if (i == g.selectIndex) {
            nameColor = SELECT_HIGHLIGHT_COLOR;
            descColor = WHITE;
        } else {
            nameColor = GRAY;
            descColor = DARKGRAY;
        }

        int nameW = MeasureText(names[i], optFont);
        int nameX = weaponRightX - nameW;
        DrawText(names[i], nameX, y, optFont, nameColor);

        // Selection highlight box
        if (i == g.selectIndex && !locked) {
            int pad = (int)(SELECT_CURSOR_PAD * ui);
            DrawRectangleLinesEx(
                (Rectangle){ nameX - pad, y - pad,
                    nameW + pad * 2, optFont + pad * 2 },
                SELECT_CURSOR_THICK * ui, SELECT_HIGHLIGHT_COLOR);
        }
        // Locked indicator for chosen primary
        if (locked) {
            int pad = (int)(SELECT_CURSOR_PAD * ui);
            DrawRectangleLinesEx(
                (Rectangle){ nameX - pad, y - pad,
                    nameW + pad * 2, optFont + pad * 2 },
                SELECT_CURSOR_THICK * ui, Fade(GREEN, 0.4f));
        }

        // Solid preview next to weapon name
        float solidSize = optFont * 0.35f;
        float previewAlpha = locked ? 0.4f :
            (i == g.selectIndex) ? 1.0f : 0.3f;
        float shadowAlpha = locked ? SHADOW_ALPHA * 0.2f :
            (i == g.selectIndex) ? SHADOW_ALPHA : SHADOW_ALPHA * 0.3f;
        float t = (float)GetTime();
        Vector2 solidPos = { nameX - optFont * 1.0f, y + optFont * 0.5f };
        Vector2 shadowP = { solidPos.x + SHADOW_OFFSET_X, solidPos.y + SHADOW_OFFSET_Y };
        solidFns[i](solidPos, solidSize, t * PLAYER_ROT_SPEED, PLAYER_ROT_TILT, previewAlpha, shadowP, shadowAlpha);

        int descW = MeasureText(descs[i], descFont);
        DrawText(descs[i], weaponRightX - descW,
            y + optFont + (int)(SELECT_DESC_GAP * ui), descFont, descColor);
    }

    EndDrawing();
}

// player inputs and mechanics
// should we refactor this further?
// or do we tackle that once we start adding different loadoats?
// diff base mechas
// adding abilities, etc
static void UpdatePlayer(float dt)
{
    Player *p = &g.player;
    
    Vector2 toMouse = mouse();

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
                Vector2 trail = { -moveDir.x * 20.0f, -moveDir.y * 20.0f };
                SpawnParticle(p->pos, trail,
                    (Color){ 100, 200, 255, 150 }, 2.0f, 0.2f);
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
                            for (int i = 0; i < 12; i++) {
                                float a = ((float)i / 12.0f) * PI * 2.0f;
                                Vector2 v = { cosf(a) * 80.0f, sinf(a) * 80.0f };
                                SpawnParticle(p->pos, v,
                                    (Color){ 100, 200, 255, 200 }, 3.0f, 0.4f);
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
                    Vector2 vVel = { cosf(vAngle) * 40.0f, sinf(vAngle) * 40.0f };
                    SpawnParticle(p->pos, vVel,
                        (Color){ 200, 200, 200, 150 }, 2.0f, 0.3f);
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

            SpawnProjectile(muzzle, dir,
                speed, damage,
                SNIPER_BULLET_LIFETIME, SNIPER_PROJECTILE_SIZE, false, false,
                PROJ_BULLET, DMG_PIERCE);
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

        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *ei = &g.enemies[i];
            if (!ei->active) continue;
            if (sweepAngle - p->sword.lastHitAngle[i] < PI) continue;
            bool swordHit = EnemyHitSweep(ei, p->pos, sweepEnd);
            if (swordHit) {
                DamageEnemy(i, dmg, DMG_SLASH, HIT_MELEE);
                p->sword.lastHitAngle[i] = sweepAngle;
            }
        }
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

        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *ei = &g.enemies[i];
            if (!ei->active) continue;
            if (sweepAngle - p->spin.lastHitAngle[i] < PI) continue;
            bool spinHit = EnemyHitSweep(ei, p->pos, sweepEnd);
            if (spinHit) {
                DamageEnemy(i, SPIN_DAMAGE, DMG_SLASH, HIT_MELEE);
                p->spin.lastHitAngle[i] = sweepAngle;
                // player heals on spin attack
                // 5% lifesteal
                float heal = SPIN_DAMAGE * SPIN_LIFESTEAL;
                p->hp = p->hp + heal;
                if (p->hp > p->maxHp) p->hp = p->maxHp;
                // animation of ability throughout
                //SpawnParticles(p->pos, GREEN, 64);
                SpawnParticles(ei->pos, GREEN,
                    (int)(heal * SPIN_HEAL_PARTICLE_MULT));
                Vector2 kb = Vector2Normalize(
                    Vector2Subtract(ei->pos, p->pos));
                ei->vel = Vector2Scale(kb, SPIN_KNOCKBACK);
            }
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
            float speed = (float)GetRandomValue(200, 400);
            Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
            Color c = (i % 2 == 0) ? (Color)SLAM_COLOR : WHITE;
            SpawnParticle(p->pos, vel, c, 4.0f, 0.3f);
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
        int count = 0;
        for (int i = 0; i < MAX_DEPLOYABLES; i++)
            if (g.deployables[i].active
                && g.deployables[i].type == DEPLOY_TURRET) count++;
        if (count < TURRET_MAX_ACTIVE) {
            float mouseDist = Vector2Length(toMouse);
            float placeDist = (mouseDist < 100.0f) ? mouseDist : 100.0f;
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
        int count = 0;
        for (int i = 0; i < MAX_DEPLOYABLES; i++)
            if (g.deployables[i].active
                && g.deployables[i].type == DEPLOY_MINE) count++;
        if (count < MINE_MAX_ACTIVE) {
            SpawnDeployable(DEPLOY_MINE, p->pos);
            p->mineCooldown = MINE_COOLDOWN;
        }
    }

    // --- Healing Field deploy ---
    if (p->healCooldown > 0) p->healCooldown -= dt;
    if (IsAbilityPressed(p, ABL_HEAL) && p->healCooldown <= 0) {
        int count = 0;
        for (int i = 0; i < MAX_DEPLOYABLES; i++)
            if (g.deployables[i].active
                && g.deployables[i].type == DEPLOY_HEAL) count++;
        if (count < HEAL_MAX_ACTIVE) {
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
                Vector2 pvel = { cosf(pa) * 30.0f, sinf(pa) * 30.0f };
                Color c = (GetRandomValue(0, 1) == 0) ? ORANGE : YELLOW;
                SpawnParticle(ppos, pvel, c, 2.0f, 0.3f);
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

// enemy AI
static void UpdateEnemies(float dt) {
    Player *p = &g.player;
    
    // --- Enemies ---
    g.spawnTimer -= dt;
    if (g.spawnTimer <= 0) {
        SpawnEnemy();
        g.spawnTimer = g.spawnInterval;
        // Ramp up spawn rate
        if (g.spawnInterval > SPAWN_MIN_INTERVAL) g.spawnInterval *= SPAWN_RAMP;
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
        if (!rooted && dist > 1.0f) {
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

        // RECT shooting AI
        if (e->type == RECT && !stunned) {
            e->shootTimer -= dt;
            if (e->shootTimer <= 0 && distToShoot > 1.0f) {
                e->shootTimer = RECT_SHOOT_INTERVAL;
                Vector2 shootDir = Vector2Scale(toShoot, 1.0f / distToShoot);
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

        // PENTA shooting AI — two parallel rows of 5 bullets
        if (e->type == PENTA && !stunned) {
            e->shootTimer -= dt;
            if (e->shootTimer <= 0 && distToShoot > 1.0f) {
                e->shootTimer = PENTA_SHOOT_INTERVAL;
                Vector2 shootDir = Vector2Scale(toShoot, 1.0f / distToShoot);
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

        // HEXA shooting AI — fan of 5 bullets
        if (e->type == HEXA && !stunned) {
            e->shootTimer -= dt;
            if (e->shootTimer <= 0 && distToShoot > 1.0f) {
                e->shootTimer = HEXA_SHOOT_INTERVAL;
                Vector2 shootDir = Vector2Scale(toShoot, 1.0f / distToShoot);
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
    // explosion ring — fast outward burst
    for (int i = 0; i < EXPLOSION_RING_COUNT; i++) {
        float a = (float)i / (float)EXPLOSION_RING_COUNT * 2.0f * PI;
        float speed = (float)GetRandomValue(
            EXPLOSION_RING_SPEED_MIN, EXPLOSION_RING_SPEED_MAX);
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        Color c = (i % 3 == 0) ? RED : (i % 3 == 1) ? ORANGE : YELLOW;
        SpawnParticle(pos, vel, c, EXPLOSION_RING_SIZE,
            EXPLOSION_RING_LIFETIME);
    }
    // inner fireball — slower, bigger
    for (int i = 0; i < EXPLOSION_FIRE_COUNT; i++) {
        float a = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(
            EXPLOSION_FIRE_SPEED_MIN, EXPLOSION_FIRE_SPEED_MAX);
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        SpawnParticle(pos, vel, ORANGE, EXPLOSION_FIRE_SIZE,
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
        if (!g.explosives[i].active) {
            g.explosives[i].active = true;
            g.explosives[i].pos = pos;
            g.explosives[i].timer = EXPLOSION_VFX_DURATION;
            g.explosives[i].duration = EXPLOSION_VFX_DURATION;
            break;
        }
    }
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
    // explosion ring — green/yellow/orange theme
    for (int i = 0; i < EXPLOSION_RING_COUNT; i++) {
        float a = (float)i / (float)EXPLOSION_RING_COUNT * 2.0f * PI;
        float speed = (float)GetRandomValue(
            EXPLOSION_RING_SPEED_MIN, EXPLOSION_RING_SPEED_MAX);
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        Color c = (i % 3 == 0) ? GREEN : (i % 3 == 1) ? YELLOW : ORANGE;
        SpawnParticle(pos, vel, c, EXPLOSION_RING_SIZE,
            EXPLOSION_RING_LIFETIME);
    }
    // inner fireball
    for (int i = 0; i < EXPLOSION_FIRE_COUNT; i++) {
        float a = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(
            EXPLOSION_FIRE_SPEED_MIN, EXPLOSION_FIRE_SPEED_MAX);
        Vector2 vel = { cosf(a) * speed, sinf(a) * speed };
        SpawnParticle(pos, vel, YELLOW, EXPLOSION_FIRE_SIZE,
            EXPLOSION_FIRE_LIFETIME);
    }
    // smoke
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
        if (!g.explosives[i].active) {
            g.explosives[i].active = true;
            g.explosives[i].pos = pos;
            g.explosives[i].timer = EXPLOSION_VFX_DURATION;
            g.explosives[i].duration = EXPLOSION_VFX_DURATION;
            break;
        }
    }
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
                        Vector2Scale(dir, 10.0f));
                    SpawnProjectile(muzzle, dir,
                        TURRET_BULLET_SPEED, TURRET_DAMAGE,
                        TURRET_BULLET_LIFETIME, TURRET_BULLET_SIZE,
                        false, false, PROJ_BULLET, DMG_BALLISTIC);
                    SpawnParticle(muzzle,
                        Vector2Scale(dir, 80.0f),
                        (Color)TURRET_COLOR, 2.0f, 0.08f);
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
                    if (!g.mineWebs[j].active) {
                        g.mineWebs[j] = (MineWebVfx){
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
        if (!g.mineWebs[i].active) continue;
        g.mineWebs[i].timer -= dt;
        if (g.mineWebs[i].timer <= 0) g.mineWebs[i].active = false;
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
                    if (b->dmgType == DMG_PIERCE) {
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
        Particle *pt = &g.particles[i];
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
        Beam *b = &g.beams[i];
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
static void UpdateGame(void)
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
        if (!g.explosives[i].active) continue;
        g.explosives[i].timer -= dt;
        if (g.explosives[i].timer <= 0)
            g.explosives[i].active = false;
    }

    MoveCamera(dt);
}


// ========================================================================== /
// Draw — Rainbow cube (fake 3D, subdivided gradient faces)
// ========================================================================== /
// Manual HSV conversion to ensure no dependency issues in web build
static Color HsvToRgb(float h, float s, float v, float alpha) {
    if (h < 0.0f) h = fmodf(h, 360.0f) + 360.0f;
    else if (h >= 360.0f) h = fmodf(h, 360.0f);
    
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r = 0, g = 0, b = 0;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    
    return (Color){
        (u8)((r + m) * 255.0f),
        (u8)((g + m) * 255.0f),
        (u8)((b + m) * 255.0f),
        (u8)(alpha * 255.0f)
    };
}

static void DrawTetra2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size / 1.73f; // normalize: vertices at (±s,±s,±s) → distance = s*√3

    // 4 tetrahedron vertices in local 3D space (regular tetrahedron)
    float vtx[4][3] = {
        { s,  s,  s},
        { s, -s, -s},
        {-s,  s, -s},
        {-s, -s,  s},
    };

    // 4 triangular faces of tetrahedron
    int faces[4][3] = {
        {0, 1, 2},
        {0, 2, 3},
        {0, 3, 1},
        {1, 3, 2},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[4];
        for (int i = 0; i < 4; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 4; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]], c = sp[faces[f][2]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) DrawTriangle(a, b, c, scol);
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[4];
    float pz[4];
    float hues[4];
    float time = (float)GetTime();

    for (int i = 0; i < 4; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        // Hue based on Z depth — smooth across rotation, no jumps on face swap
        hues[i] = time * SHAPE_HUE_SPEED + pz[i] * (SHAPE_HUE_DEPTH_SCALE / s);
    }

    float faceZ[4];
    bool faceVis[4];
    for (int f = 0; f < 4; f++) {
        faceZ[f] = (pz[faces[f][0]] + pz[faces[f][1]] + pz[faces[f][2]]) / 3.0f;
        
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        // 2D Cross Product for backface culling
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    // Sort back-to-front (Painter's algorithm)
    int order[4] = {0, 1, 2, 3};
    for (int i = 0; i < 3; i++)
        for (int j = i + 1; j < 4; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = 6; // Grid subdivision for smooth gradient
    
    for (int fi = 0; fi < 4; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]];
        Vector2 p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]];
        
        float h0 = hues[faces[f][0]];
        float h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]];

        // Subdivide triangle using barycentric coordinates
        for (int row = 0; row < N; row++) {
            for (int col = 0; col < N - row; col++) {
                float u0 = (float)col / N;
                float v0 = (float)row / N;
                float w0 = 1.0f - u0 - v0;
                
                float u1 = (float)(col + 1) / N;
                float v1 = (float)row / N;
                float w1 = 1.0f - u1 - v1;
                
                float u2 = (float)col / N;
                float v2 = (float)(row + 1) / N;
                float w2 = 1.0f - u2 - v2;
                
                // Triangle vertices using barycentric interpolation
                Vector2 q0 = {
                    w0 * p0.x + u0 * p1.x + v0 * p2.x,
                    w0 * p0.y + u0 * p1.y + v0 * p2.y
                };
                Vector2 q1 = {
                    w1 * p0.x + u1 * p1.x + v1 * p2.x,
                    w1 * p0.y + u1 * p1.y + v1 * p2.y
                };
                Vector2 q2 = {
                    w2 * p0.x + u2 * p1.x + v2 * p2.x,
                    w2 * p0.y + u2 * p1.y + v2 * p2.y
                };
                
                // Interpolate hue
                float hC = fmodf(w0 * h0 + u0 * h1 + v0 * h2 + 360.0f, 360.0f);
                Color cc = HsvToRgb(hC, 1.0f, 1.0f, alpha);

                DrawTriangle(q0, q1, q2, cc);

                // Draw second triangle in quad if not at edge
                if (col + 1 < N - row) {
                    float u3 = (float)(col + 1) / N;
                    float v3 = (float)(row + 1) / N;
                    float w3 = 1.0f - u3 - v3;

                    Vector2 q3 = {
                        w3 * p0.x + u3 * p1.x + v3 * p2.x,
                        w3 * p0.y + u3 * p1.y + v3 * p2.y
                    };

                    float hC2 = fmodf(w3 * h0 + u3 * h1 + v3 * h2 + 360.0f, 360.0f);
                    Color cc2 = HsvToRgb(hC2, 1.0f, 1.0f, alpha);
                    
                    DrawTriangle(q1, q3, q2, cc2);
                }
            }
        }
        
        // Draw edges
        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p0, edge);
    }


}






static void DrawCube2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size / 1.73f; // normalize: vertices at (±s,±s,±s) → distance = s*√3

    // 8 cube vertices in local 3D space
    float vtx[8][3] = {
        {-s,-s,-s}, { s,-s,-s}, { s, s,-s}, {-s, s,-s},
        {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s},
    };

    // 6 faces: {0,3,2,1} is Front, {4,5,6,7} is Back
    int faces[6][4] = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3},
        {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[8];
        for (int i = 0; i < 8; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 6; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]];
            Vector2 c = sp[faces[f][2]], d = sp[faces[f][3]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) {
                DrawTriangle(a, b, c, scol);
                DrawTriangle(a, c, d, scol);
            }
        }
    }

    // 4 tetrahedron vertices in local 3D space
    // around origin
    //float vtx[4][3] = {
    //    {0, 0, s},
    //    {s, -s, -s}, {-s, -s, -s}, {0, s, -s},
    //};

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[8];
    float pz[8];
    float hues[8];
    float time = (float)GetTime();

    for (int i = 0; i < 8; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        // Generate hue offset for "rainbow" look
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * CUBE_HUE_VERTEX_STEP;
    }

    float faceZ[6];
    bool faceVis[6];
    for (int f = 0; f < 6; f++) {
        faceZ[f] = 0;
        for (int vi = 0; vi < 4; vi++) faceZ[f] += pz[faces[f][vi]];
        faceZ[f] *= 0.25f;
        
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        // 2D Cross Product: (b-a)x(c-a)
        // Note: Screen Y is down. Standard CCW winding produces Negative cross product.
        // We want to show faces facing the camera (Front).
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    // Sort back-to-front (Painter's algorithm)
    // Smallest Z is "Far" in this projection? Actually standard 3D: Z+ is near?
    // Let's sort simply by Z value.
    int order[6] = {0, 1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
        for (int j = i + 1; j < 6; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) { // Swap if i is 'closer' than j?
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = 6; // Grid subdivision
    
    for (int fi = 0; fi < 6; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]], p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]], p3 = pj[faces[f][3]];
        float h0 = hues[faces[f][0]], h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]], h3 = hues[faces[f][3]];

        for (int gy = 0; gy < N; gy++) {
            for (int gx = 0; gx < N; gx++) {
                float u0 = (float)gx / N, u1 = (float)(gx + 1) / N;
                float v0 = (float)gy / N, v1 = (float)(gy + 1) / N;

                #define BILERP(A,B,C,D,u,v) \
                    ((1-(v))*((1-(u))*(A) + (u)*(B)) + (v)*((1-(u))*(D) + (u)*(C)))

                Vector2 q00 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u0,v0), 
                    BILERP(p0.y,p1.y,p2.y,p3.y,u0,v0) 
                };
                Vector2 q10 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u1,v0), 
                    BILERP(p0.y,p1.y,p2.y,p3.y,u1,v0) 
                };
                Vector2 q11 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u1,v1),
                    BILERP(p0.y,p1.y,p2.y,p3.y,u1,v1)
                };
                Vector2 q01 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u0,v1), 
                    BILERP(p0.y,p1.y,p2.y,p3.y,u0,v1)
                };

                float uc = (u0 + u1) * 0.5f, vc = (v0 + v1) * 0.5f;
                float hC = BILERP(h0, h1, h2, h3, uc, vc);
                
                Color cc = HsvToRgb(hC, 0.85f, 1.0f, alpha);
                DrawTriangle(q00, q10, q11, cc);
                DrawTriangle(q00, q11, q01, cc);

                #undef BILERP
            }
        }
        
        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p3, edge);
        DrawLineV(p3, p0, edge);
    }
}

// octahedron -------------------------------------------------------------- /
static void DrawOcta2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size;

    // 6 octahedron vertices: axis-aligned
    float vtx[6][3] = {
        { s, 0, 0}, {-s, 0, 0},
        { 0, s, 0}, { 0,-s, 0},
        { 0, 0, s}, { 0, 0,-s},
    };

    // 8 triangular faces
    int faces[8][3] = {
        {0, 2, 4}, {2, 1, 4}, {1, 3, 4}, {3, 0, 4},
        {2, 0, 5}, {1, 2, 5}, {3, 1, 5}, {0, 3, 5},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[6];
        for (int i = 0; i < 6; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 8; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]], c = sp[faces[f][2]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) DrawTriangle(a, b, c, scol);
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[6];
    float pz[6];
    float hues[6];
    float time = (float)GetTime();

    for (int i = 0; i < 6; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * OCTA_HUE_VERTEX_STEP;
    }

    float faceZ[8];
    bool faceVis[8];
    for (int f = 0; f < 8; f++) {
        faceZ[f] = (pz[faces[f][0]] + pz[faces[f][1]] + pz[faces[f][2]]) / 3.0f;
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    int order[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int i = 0; i < 7; i++)
        for (int j = i + 1; j < 8; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = 6;

    for (int fi = 0; fi < 8; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]];
        Vector2 p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]];
        float h0 = hues[faces[f][0]];
        float h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]];

        for (int row = 0; row < N; row++) {
            for (int col = 0; col < N - row; col++) {
                float u0 = (float)col / N;
                float v0 = (float)row / N;
                float w0 = 1.0f - u0 - v0;

                float u1 = (float)(col + 1) / N;
                float v1 = (float)row / N;
                float w1 = 1.0f - u1 - v1;

                float u2 = (float)col / N;
                float v2 = (float)(row + 1) / N;
                float w2 = 1.0f - u2 - v2;

                Vector2 q0 = {
                    w0 * p0.x + u0 * p1.x + v0 * p2.x,
                    w0 * p0.y + u0 * p1.y + v0 * p2.y
                };
                Vector2 q1 = {
                    w1 * p0.x + u1 * p1.x + v1 * p2.x,
                    w1 * p0.y + u1 * p1.y + v1 * p2.y
                };
                Vector2 q2 = {
                    w2 * p0.x + u2 * p1.x + v2 * p2.x,
                    w2 * p0.y + u2 * p1.y + v2 * p2.y
                };

                float hC = fmodf(w0 * h0 + u0 * h1 + v0 * h2 + 360.0f, 360.0f);
                Color cc = HsvToRgb(hC, 1.0f, 1.0f, alpha);
                DrawTriangle(q0, q1, q2, cc);

                if (col + 1 < N - row) {
                    float u3 = (float)(col + 1) / N;
                    float v3 = (float)(row + 1) / N;
                    float w3 = 1.0f - u3 - v3;
                    Vector2 q3 = {
                        w3 * p0.x + u3 * p1.x + v3 * p2.x,
                        w3 * p0.y + u3 * p1.y + v3 * p2.y
                    };
                    float hC2 = fmodf(w3 * h0 + u3 * h1 + v3 * h2 + 360.0f, 360.0f);
                    Color cc2 = HsvToRgb(hC2, 1.0f, 1.0f, alpha);
                    DrawTriangle(q1, q3, q2, cc2);
                }
            }
        }

        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p0, edge);
    }
}

// icosahedron ------------------------------------------------------------- /
static void DrawIcosa2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    float sc = size / phi; // scale so max extent ≈ size

    // 12 icosahedron vertices: (0, ±1, ±φ) and permutations
    float vtx[12][3] = {
        { 0,  sc,  sc*phi}, { 0,  sc, -sc*phi},
        { 0, -sc,  sc*phi}, { 0, -sc, -sc*phi},
        { sc,  sc*phi, 0}, {-sc,  sc*phi, 0},
        { sc, -sc*phi, 0}, {-sc, -sc*phi, 0},
        { sc*phi, 0,  sc}, { sc*phi, 0, -sc},
        {-sc*phi, 0,  sc}, {-sc*phi, 0, -sc},
    };

    // 20 triangular faces
    int faces[20][3] = {
        {0, 2, 8},  {0, 8, 4},  {0, 4, 5},  {0, 5, 10}, {0, 10, 2},
        {2, 6, 8},  {8, 6, 9},  {8, 9, 4},  {4, 9, 1},  {4, 1, 5},
        {5, 1, 11}, {5, 11, 10},{10, 11, 7}, {10, 7, 2}, {2, 7, 6},
        {3, 9, 6},  {3, 1, 9},  {3, 11, 1}, {3, 7, 11}, {3, 6, 7},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[12];
        for (int i = 0; i < 12; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 20; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]], c = sp[faces[f][2]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) DrawTriangle(a, b, c, scol);
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[12];
    float pz[12];
    float hues[12];
    float time = (float)GetTime();

    for (int i = 0; i < 12; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * ICOSA_HUE_VERTEX_STEP;
    }

    float faceZ[20];
    bool faceVis[20];
    for (int f = 0; f < 20; f++) {
        faceZ[f] = (pz[faces[f][0]] + pz[faces[f][1]] + pz[faces[f][2]]) / 3.0f;
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    int order[20];
    for (int i = 0; i < 20; i++) order[i] = i;
    for (int i = 0; i < 19; i++)
        for (int j = i + 1; j < 20; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = 4;

    for (int fi = 0; fi < 20; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]];
        Vector2 p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]];
        float h0 = hues[faces[f][0]];
        float h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]];

        for (int row = 0; row < N; row++) {
            for (int col = 0; col < N - row; col++) {
                float u0 = (float)col / N;
                float v0 = (float)row / N;
                float w0 = 1.0f - u0 - v0;

                float u1 = (float)(col + 1) / N;
                float v1 = (float)row / N;
                float w1 = 1.0f - u1 - v1;

                float u2 = (float)col / N;
                float v2 = (float)(row + 1) / N;
                float w2 = 1.0f - u2 - v2;

                Vector2 q0 = {
                    w0 * p0.x + u0 * p1.x + v0 * p2.x,
                    w0 * p0.y + u0 * p1.y + v0 * p2.y
                };
                Vector2 q1 = {
                    w1 * p0.x + u1 * p1.x + v1 * p2.x,
                    w1 * p0.y + u1 * p1.y + v1 * p2.y
                };
                Vector2 q2 = {
                    w2 * p0.x + u2 * p1.x + v2 * p2.x,
                    w2 * p0.y + u2 * p1.y + v2 * p2.y
                };

                float hC = fmodf(w0 * h0 + u0 * h1 + v0 * h2 + 360.0f, 360.0f);
                Color cc = HsvToRgb(hC, 1.0f, 1.0f, alpha);
                DrawTriangle(q0, q1, q2, cc);

                if (col + 1 < N - row) {
                    float u3 = (float)(col + 1) / N;
                    float v3 = (float)(row + 1) / N;
                    float w3 = 1.0f - u3 - v3;
                    Vector2 q3 = {
                        w3 * p0.x + u3 * p1.x + v3 * p2.x,
                        w3 * p0.y + u3 * p1.y + v3 * p2.y
                    };
                    float hC2 = fmodf(w3 * h0 + u3 * h1 + v3 * h2 + 360.0f, 360.0f);
                    Color cc2 = HsvToRgb(hC2, 1.0f, 1.0f, alpha);
                    DrawTriangle(q1, q3, q2, cc2);
                }
            }
        }

        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p0, edge);
    }
}

// dodecahedron ------------------------------------------------------------ /
static void DrawDodeca2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    float iphi = 1.0f / phi;
    // scale so max extent ≈ size (cube verts are at ±1 → √3 ≈ 1.73)
    float sc = size / 1.73f;

    // 20 dodecahedron vertices:
    // 8 cube vertices (±1,±1,±1)
    // 4 vertices (0, ±φ, ±1/φ)
    // 4 vertices (±1/φ, 0, ±φ)
    // 4 vertices (±φ, ±1/φ, 0)
    float vtx[20][3] = {
        // cube vertices
        { sc, sc, sc}, { sc, sc,-sc}, { sc,-sc, sc}, { sc,-sc,-sc},
        {-sc, sc, sc}, {-sc, sc,-sc}, {-sc,-sc, sc}, {-sc,-sc,-sc},
        // (0, ±φ, ±1/φ)
        {0,  sc*phi,  sc*iphi}, {0,  sc*phi, -sc*iphi},
        {0, -sc*phi,  sc*iphi}, {0, -sc*phi, -sc*iphi},
        // (±1/φ, 0, ±φ)
        { sc*iphi, 0,  sc*phi}, {-sc*iphi, 0,  sc*phi},
        { sc*iphi, 0, -sc*phi}, {-sc*iphi, 0, -sc*phi},
        // (±φ, ±1/φ, 0)
        { sc*phi,  sc*iphi, 0}, { sc*phi, -sc*iphi, 0},
        {-sc*phi,  sc*iphi, 0}, {-sc*phi, -sc*iphi, 0},
    };

    // 12 pentagonal faces
    int faces[12][5] = {
        { 0, 12,  2, 17, 16}, { 0, 16,  1,  9,  8},
        { 0,  8,  4, 13, 12}, { 1, 16, 17,  3, 14},
        { 1, 14, 15,  5,  9}, { 2, 12, 13,  6, 10},
        { 2, 10, 11,  3, 17}, { 4,  8,  9,  5, 18},
        { 4, 18, 19,  6, 13}, { 7, 11, 10,  6, 19},
        { 7, 19, 18,  5, 15}, { 7, 15, 14,  3, 11},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[20];
        for (int i = 0; i < 20; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 12; f++) {
            Vector2 pv[5];
            for (int vi = 0; vi < 5; vi++) pv[vi] = sp[faces[f][vi]];
            float cross = (pv[1].x-pv[0].x)*(pv[2].y-pv[0].y) - (pv[1].y-pv[0].y)*(pv[2].x-pv[0].x);
            if (cross < 0) {
                for (int tri = 0; tri < 3; tri++)
                    DrawTriangle(pv[0], pv[tri+1], pv[tri+2], scol);
            }
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[20];
    float pz[20];
    float hues[20];
    float time = (float)GetTime();

    for (int i = 0; i < 20; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * DODECA_HUE_VERTEX_STEP;
    }

    float faceZ[12];
    bool faceVis[12];
    for (int f = 0; f < 12; f++) {
        faceZ[f] = 0;
        for (int vi = 0; vi < 5; vi++) faceZ[f] += pz[faces[f][vi]];
        faceZ[f] /= 5.0f;

        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    int order[12];
    for (int i = 0; i < 12; i++) order[i] = i;
    for (int i = 0; i < 11; i++)
        for (int j = i + 1; j < 12; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    for (int fi = 0; fi < 12; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        // Fan pentagon into 3 triangles from vertex 0
        Vector2 pv[5];
        float hv[5];
        for (int vi = 0; vi < 5; vi++) {
            pv[vi] = pj[faces[f][vi]];
            hv[vi] = hues[faces[f][vi]];
        }

        for (int tri = 0; tri < 3; tri++) {
            Vector2 p0 = pv[0];
            Vector2 p1 = pv[tri + 1];
            Vector2 p2 = pv[tri + 2];
            float h0 = hv[0];
            float h1 = hv[tri + 1];
            float h2 = hv[tri + 2];

            int N = 4;
            for (int row = 0; row < N; row++) {
                for (int col = 0; col < N - row; col++) {
                    float u0b = (float)col / N;
                    float v0b = (float)row / N;
                    float w0b = 1.0f - u0b - v0b;

                    float u1b = (float)(col + 1) / N;
                    float v1b = (float)row / N;
                    float w1b = 1.0f - u1b - v1b;

                    float u2b = (float)col / N;
                    float v2b = (float)(row + 1) / N;
                    float w2b = 1.0f - u2b - v2b;

                    Vector2 q0 = {
                        w0b * p0.x + u0b * p1.x + v0b * p2.x,
                        w0b * p0.y + u0b * p1.y + v0b * p2.y
                    };
                    Vector2 q1 = {
                        w1b * p0.x + u1b * p1.x + v1b * p2.x,
                        w1b * p0.y + u1b * p1.y + v1b * p2.y
                    };
                    Vector2 q2 = {
                        w2b * p0.x + u2b * p1.x + v2b * p2.x,
                        w2b * p0.y + u2b * p1.y + v2b * p2.y
                    };

                    float hC = fmodf(w0b * h0 + u0b * h1 + v0b * h2 + 360.0f, 360.0f);
                    Color cc = HsvToRgb(hC, 0.85f, 1.0f, alpha);
                    DrawTriangle(q0, q1, q2, cc);

                    if (col + 1 < N - row) {
                        float u3b = (float)(col + 1) / N;
                        float v3b = (float)(row + 1) / N;
                        float w3b = 1.0f - u3b - v3b;
                        Vector2 q3 = {
                            w3b * p0.x + u3b * p1.x + v3b * p2.x,
                            w3b * p0.y + u3b * p1.y + v3b * p2.y
                        };
                        float hC2 = fmodf(w3b * h0 + u3b * h1 + v3b * h2 + 360.0f, 360.0f);
                        Color cc2 = HsvToRgb(hC2, 0.85f, 1.0f, alpha);
                        DrawTriangle(q1, q3, q2, cc2);
                    }
                }
            }
        }

        // Draw pentagon edges
        Color edge = Fade(WHITE, 0.5f * alpha);
        for (int vi = 0; vi < 5; vi++)
            DrawLineV(pv[vi], pv[(vi + 1) % 5], edge);
    }
}

// player solid dispatcher ------------------------------------------------- /
static void DrawPlayerSolid(Vector2 pos, float size, float rotY, float rotX, float alpha,
                            Vector2 shadowPos, float shadowAlpha) {
    switch (g.player.primary) {
        case WPN_SWORD:    DrawTetra2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_REVOLVER: DrawCube2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_GUN:      DrawOcta2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_SNIPER:   DrawDodeca2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_ROCKET:   DrawIcosa2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        default: break;
    }
}

// ========================================================================== /
// Draw — World (camera space)
// ========================================================================== /
static void DrawWorld(void)
{
    Player *p = &g.player;

    // --- Map grid ---
    for (float x = 0; x <= MAP_SIZE; x += GRID_STEP) {
        DrawLineV((Vector2){ x, 0 }, (Vector2){ x, MAP_SIZE }, GRID_COLOR);
    }
    for (float y = 0; y <= MAP_SIZE; y += GRID_STEP) {
        DrawLineV((Vector2){ 0, y }, (Vector2){ MAP_SIZE, y }, GRID_COLOR);
    }

    // Map boundary
    DrawRectangleLinesEx((Rectangle){ 0, 0, MAP_SIZE, MAP_SIZE },
        MAP_BORDER_THICKNESS, RED);

    // --- Particles (behind entities) ---
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &g.particles[i];
        if (!pt->active) continue;
        float alpha = pt->lifetime / pt->maxLifetime;
        Color c = Fade(pt->color, alpha);
        DrawCircleV(pt->pos, pt->size * alpha, c);
    }

    // --- Fire patches ---
    for (int i = 0; i < MAX_FIRE_PATCHES; i++) {
        FirePatch *fp = &g.firePatches[i];
        if (!fp->active) continue;
        float t = (float)GetTime();
        float life = fp->timer / FLAME_PATCH_LIFETIME;
        float fade = (life > 0.3f) ? 1.0f : life / 0.3f;
        float r = fp->radius;
        // scatter embers — deterministic offsets from patch index
        int embers = 6;
        for (int j = 0; j < embers; j++) {
            float seed = i * 7.13f + j * 3.71f;
            // each ember drifts in a small loop
            float ox = sinf(seed + t * 1.5f) * r * 0.6f
                     + cosf(seed * 2.3f) * r * 0.3f;
            float oy = cosf(seed * 1.7f + t * 1.8f) * r * 0.5f
                     - (1.0f - life) * r * 0.4f; // drift upward as patch ages
            float eSize = r * (0.3f + 0.25f * sinf(seed * 5.1f + t * FLAME_FLICKER_SPEED));
            Vector2 ep = { fp->pos.x + ox, fp->pos.y + oy };
            // color: outer embers red/orange, inner ones yellow
            float heat = 1.0f - (fabsf(ox) + fabsf(oy)) / (r * 1.2f);
            if (heat < 0) heat = 0;
            Color c;
            if (heat > 0.6f)
                c = YELLOW;
            else if (heat > 0.3f)
                c = ORANGE;
            else
                c = (Color){ 255, 80, 20, 255 };
            DrawCircleV(ep, eSize, Fade(c, 0.22f * fade));
        }
        // soft glow underneath
        DrawCircleV(fp->pos, r * 0.8f,
            Fade((Color){ 255, 60, 10, 255 }, 0.08f * fade));
        // bright core flicker
        float coreFlicker = 0.6f + 0.4f * sinf(t * 14.0f + i * 2.0f);
        DrawCircleV(fp->pos, r * 0.2f * coreFlicker,
            Fade(YELLOW, 0.35f * fade));
    }

    // --- Deployables (ground level) ---
    for (int i = 0; i < MAX_DEPLOYABLES; i++) {
        Deployable *d = &g.deployables[i];
        if (!d->active) continue;
        float t = (float)GetTime();

        switch (d->type) {
        case DEPLOY_TURRET: {
            // rotating triangle with line toward nearest enemy
            float rot = t * 3.0f;
            DrawCircleV(d->pos, 8.0f, Fade((Color)TURRET_COLOR, 0.3f));
            for (int v = 0; v < 3; v++) {
                float a0 = rot + v * (2.0f * PI / 3.0f);
                float a1 = rot + (v + 1) * (2.0f * PI / 3.0f);
                Vector2 p0 = Vector2Add(d->pos,
                    (Vector2){ cosf(a0) * 10.0f, sinf(a0) * 10.0f });
                Vector2 p1 = Vector2Add(d->pos,
                    (Vector2){ cosf(a1) * 10.0f, sinf(a1) * 10.0f });
                DrawLineEx(p0, p1, 2.0f, (Color)TURRET_COLOR);
            }
            // HP bar
            float hpFrac = (float)d->hp / TURRET_HP;
            float barW = 16.0f, barH = 2.0f;
            Vector2 barPos = { d->pos.x - barW * 0.5f, d->pos.y + 13.0f };
            DrawRectangleV(barPos, (Vector2){ barW, barH },
                Fade(DARKGRAY, 0.5f));
            Color hpColor = (hpFrac > 0.5f) ? (Color)TURRET_COLOR : ORANGE;
            if (hpFrac < 0.25f) hpColor = RED;
            DrawRectangleV(barPos, (Vector2){ barW * hpFrac, barH }, hpColor);
        } break;
        case DEPLOY_MINE: {
            float pulse = 0.6f + 0.4f * sinf(t * MINE_PULSE_SPEED);
            DrawCircleV(d->pos, d->radius * pulse,
                Fade((Color)MINE_COLOR, 0.15f));
            DrawCircleLinesV(d->pos, 6.0f, (Color)MINE_COLOR);
            DrawCircleV(d->pos, 3.0f,
                Fade((Color)MINE_COLOR, pulse));
        } break;
        case DEPLOY_HEAL: {
            float pulse = 0.7f + 0.3f * sinf(t * HEAL_PULSE_SPEED);
            DrawCircleV(d->pos, d->radius * pulse,
                Fade((Color)HEAL_COLOR, 0.1f));
            DrawCircleLinesV(d->pos, d->radius,
                Fade((Color)HEAL_COLOR, 0.4f));
        } break;
        }
    }

    // --- Mine Web VFX ---
    for (int i = 0; i < MAX_MINE_WEBS; i++) {
        MineWebVfx *w = &g.mineWebs[i];
        if (!w->active) continue;
        float progress = 1.0f - (w->timer / MINE_WEB_DURATION);
        float alpha = 1.0f - progress;
        float r = MINE_ROOT_RADIUS;
        // spokes
        for (int s = 0; s < MINE_WEB_SPOKES; s++) {
            float a = s * (2.0f * PI / MINE_WEB_SPOKES);
            Vector2 tip = { w->pos.x + cosf(a) * r,
                            w->pos.y + sinf(a) * r };
            DrawLineEx(w->pos, tip, 1.0f, Fade(WHITE, alpha * 0.6f));
        }
        // concentric rings connecting the spokes
        for (int ring = 1; ring <= MINE_WEB_RINGS; ring++) {
            float rr = r * ring / (float)MINE_WEB_RINGS;
            for (int s = 0; s < MINE_WEB_SPOKES; s++) {
                float a0 = s * (2.0f * PI / MINE_WEB_SPOKES);
                float a1 = (s + 1) * (2.0f * PI / MINE_WEB_SPOKES);
                Vector2 p0 = { w->pos.x + cosf(a0) * rr,
                               w->pos.y + sinf(a0) * rr };
                Vector2 p1 = { w->pos.x + cosf(a1) * rr,
                               w->pos.y + sinf(a1) * rr };
                DrawLineEx(p0, p1, 1.0f, Fade(WHITE, alpha * 0.4f));
            }
        }
    }

    // --- Projectiles ---
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *b = &g.projectiles[i];
        if (!b->active) continue;

        // sniper .50 cal bullet — elongated pointed shape
        if (b->dmgType == DMG_PIERCE && !b->isEnemy) {
            Vector2 fwd = Vector2Normalize(b->vel);
            Vector2 perp = { -fwd.y, fwd.x };
            float len = b->size * SNIPER_BULLET_LENGTH;
            float w   = b->size * SNIPER_BULLET_WIDTH;
            float tip = b->size * SNIPER_NOSE_LENGTH;
            // nose cone tip
            Vector2 noseTip = Vector2Add(b->pos, Vector2Scale(fwd, tip));
            // body front (where nose meets body)
            Vector2 frontL = Vector2Add(b->pos, Vector2Scale(perp,  w));
            Vector2 frontR = Vector2Add(b->pos, Vector2Scale(perp, -w));
            // body rear
            Vector2 base = Vector2Subtract(b->pos, Vector2Scale(fwd, len));
            Vector2 rearL = Vector2Add(base, Vector2Scale(perp,  w));
            Vector2 rearR = Vector2Add(base, Vector2Scale(perp, -w));
            // draw body (two triangles for the rect)
            DrawTriangle(frontL, rearL, rearR, SNIPER_BODY_COLOR);
            DrawTriangle(frontL, rearR, frontR, SNIPER_BODY_COLOR);
            // draw nose cone
            DrawTriangle(noseTip, frontR, frontL, SNIPER_TIP_COLOR);
            // tracer trail
            Vector2 trail = Vector2Subtract(base, Vector2Scale(fwd, len * SNIPER_TRAIL_MULT));
            DrawLineEx(base, trail, w * 0.8f, Fade(SNIPER_COLOR, 0.4f));
        } else if (b->type == PROJ_BFG) {
            // pulsing lightning ball
            float pulse = sinf(GetTime() * BFG_PULSE_SPEED) * BFG_PULSE_AMOUNT;
            float outerR = b->size + pulse;
            float innerR = b->size * 0.6f + pulse * 0.5f;
            DrawCircleV(b->pos, outerR, (Color)BFG_GLOW_COLOR);
            DrawCircleV(b->pos, innerR, (Color)BFG_COLOR);
            DrawCircleV(b->pos, innerR * 0.4f, WHITE);
        } else if (b->type == PROJ_GRENADE) {
            // shadow on ground
            DrawCircleV(b->pos, b->size * 0.8f, Fade(BLACK, 0.3f));
            // grenade drawn at visual height offset (negative Y = up on screen)
            Vector2 drawPos = { b->pos.x, b->pos.y - b->height };
            DrawCircleV(drawPos, b->size, GRENADE_COLOR);
            DrawCircleLinesV(drawPos, b->size + 1.0f, GRENADE_GLOW_COLOR);
        } else {
            Color bColor = b->isEnemy ? MAGENTA : YELLOW;
            float bSize = b->size;
            DrawCircleV(b->pos, bSize, bColor);
            Vector2 trail = Vector2Subtract(b->pos, Vector2Scale(b->vel, BULLET_TRAIL_FACTOR));
            DrawLineV(trail, b->pos, Fade(bColor, 0.5f));
        }
    }

    // --- Explosion rings ---
    for (int i = 0; i < MAX_EXPLOSIVES; i++) {
        Explosive *ex = &g.explosives[i];
        if (!ex->active) continue;
        float t = ex->timer / ex->duration;
        float alpha = t * 0.7f;
        float radius = ROCKET_EXPLOSION_RADIUS * (1.0f - t * 0.3f);
        DrawCircleLinesV(ex->pos, radius, Fade(ORANGE, alpha));
        DrawCircleLinesV(ex->pos, radius - 2.0f, Fade(RED, alpha * 0.6f));
    }

    // --- Lightning chain arcs (BFG) ---
    if (g.lightning.active) {
        LightningChain *lc = &g.lightning;
        for (int i = 0; i < lc->arcCount; i++) {
            LightningArc *a = &lc->arcs[i];
            if (!a->active) continue;
            float t = a->timer / a->duration;

            // jagged line: 3 midpoints with perpendicular jitter
            Vector2 diff = Vector2Subtract(a->to, a->from);
            Vector2 perp = { -diff.y, diff.x };
            float pLen = Vector2Length(perp);
            if (pLen > 0.1f) perp = Vector2Scale(perp, 1.0f / pLen);

            Vector2 pts[5];
            pts[0] = a->from;
            pts[4] = a->to;
            for (int m = 1; m <= 3; m++) {
                float frac = (float)m / 4.0f;
                Vector2 base = Vector2Lerp(a->from, a->to, frac);
                float jitter = (float)GetRandomValue(
                    -(int)BFG_ARC_JITTER, (int)BFG_ARC_JITTER);
                pts[m] = Vector2Add(base, Vector2Scale(perp, jitter));
            }

            // draw glow then core
            Color glow = { BFG_GLOW_COLOR.r, BFG_GLOW_COLOR.g,
                           BFG_GLOW_COLOR.b, (u8)(BFG_GLOW_COLOR.a * t) };
            Color core = Fade((Color)BFG_COLOR, t);
            for (int s = 0; s < 4; s++) {
                DrawLineEx(pts[s], pts[s + 1], BFG_ARC_GLOW_WIDTH * t, glow);
                DrawLineEx(pts[s], pts[s + 1], BFG_ARC_CORE_WIDTH, core);
            }
        }
    }

    // --- Enemies ---
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g.enemies[i];
        if (!e->active) continue;

        float eAngle = EnemyAngle(e);
        Color eColor = (e->hitFlash > 0) ? WHITE : RED;
        // Icy tint when slowed
        if (e->slowTimer > 0 && e->hitFlash <= 0) {
            eColor = SNIPER_COLOR;
        }

        switch (e->type) {
        case TRI: {
            Vector2 tip = Vector2Add(
                    e->pos, 
                    (Vector2){ 
                        cosf(eAngle) * e->size, 
                        sinf(eAngle) * e->size 
                    }
                );
            Vector2 left = Vector2Add(
                    e->pos, 
                    (Vector2){ 
                        cosf(eAngle + TRI_WING_ANGLE) * e->size,
                        sinf(eAngle + TRI_WING_ANGLE) * e->size 
                    }
                );
            Vector2 right = Vector2Add(
                    e->pos, 
                    (Vector2){ 
                        cosf(eAngle - TRI_WING_ANGLE) * e->size,
                        sinf(eAngle - TRI_WING_ANGLE) * e->size 
                    }
                );
            DrawTriangle(tip, right, left, eColor);
            DrawTriangleLines(tip, right, left, MAROON);
        } break;
        case RECT: {
            Color rFill = (e->hitFlash > 0) ? WHITE : MAGENTA;
            float hw = e->size, hh = e->size * RECT_ASPECT_RATIO;
            float ca = cosf(eAngle), sa = sinf(eAngle);
            DrawRectanglePro(
                (Rectangle){ e->pos.x, e->pos.y, hw * 2.0f, hh * 2.0f },
                (Vector2){ hw, hh },
                eAngle * RAD2DEG,
                rFill);
            // Corner outline
            Vector2 corners[4] = {
                    { 
                        e->pos.x + (-hw)*ca - (-hh)*sa, 
                        e->pos.y + (-hw)*sa + (-hh)*ca 
                    },
                    { 
                        e->pos.x + ( hw)*ca - (-hh)*sa, 
                        e->pos.y + ( hw)*sa + (-hh)*ca 
                    },
                    { 
                        e->pos.x + ( hw)*ca - ( hh)*sa, 
                        e->pos.y + ( hw)*sa + ( hh)*ca 
                    },
                    { 
                        e->pos.x + (-hw)*ca - ( hh)*sa, 
                        e->pos.y + (-hw)*sa + ( hh)*ca 
                    },
            };
            for (int ci = 0; ci < 4; ci++)
                DrawLineV(corners[ci], corners[(ci + 1) % 4], DARKPURPLE);
        } break;
        case PENTA: {
            Color pFill = (e->hitFlash > 0) ? WHITE : PENTA_COLOR;
            DrawPoly(e->pos, 5, e->size, eAngle * RAD2DEG, pFill);
            DrawPolyLines(e->pos, 5, e->size, eAngle * RAD2DEG, DARKGREEN);
        } break;
        case RHOM: {
            Color rFill = (e->hitFlash > 0) ? WHITE : RHOM_COLOR;
            // Elongated diamond pointing at player
            float s = e->size;
            Vector2 tip = Vector2Add(e->pos,
                (Vector2){ cosf(eAngle) * s * RHOM_TIP_MULT,
                           sinf(eAngle) * s * RHOM_TIP_MULT });
            Vector2 back = Vector2Add(e->pos,
                (Vector2){ cosf(eAngle + PI) * s * RHOM_BACK_MULT,
                           sinf(eAngle + PI) * s * RHOM_BACK_MULT });
            float perpAngle = eAngle + PI * 0.5f;
            Vector2 left = Vector2Add(e->pos,
                (Vector2){ cosf(perpAngle) * s * RHOM_SIDE_MULT,
                           sinf(perpAngle) * s * RHOM_SIDE_MULT });
            Vector2 right = Vector2Add(e->pos,
                (Vector2){ cosf(perpAngle) * s * -RHOM_SIDE_MULT,
                           sinf(perpAngle) * s * -RHOM_SIDE_MULT });
            // Draw as two triangles (tip-left-right, back-right-left)
            DrawTriangle(tip, right, left, rFill);
            DrawTriangle(back, left, right, rFill);
            // Outline
            Color rOutline = RHOM_OUTLINE_COLOR;
            DrawLineV(tip, left, rOutline);
            DrawLineV(left, back, rOutline);
            DrawLineV(back, right, rOutline);
            DrawLineV(right, tip, rOutline);
        } break;
        case HEXA: {
            Color hFill = (e->hitFlash > 0) ? WHITE : HEXA_COLOR;
            DrawPoly(e->pos, 6, e->size, eAngle * RAD2DEG, hFill);
            DrawPolyLines(e->pos, 6, e->size, eAngle * RAD2DEG, HEXA_OUTLINE_COLOR);
        } break;
        case OCTA: {
            Color hFill = (e->hitFlash > 0) ? WHITE : OCTA_COLOR;
            DrawPoly(e->pos, 8, e->size, eAngle * RAD2DEG, hFill);
            DrawPolyLines(e->pos, 8, e->size, eAngle * RAD2DEG, OCTA_OUTLINE_COLOR);
        } break;
        case TRAP: {
            break;
        } break;
        case CIRC: {
            break;
        } break;
        default: break;
        }

        // HP bar
        if (e->hp < e->maxHp) {
            float barW = e->size * 2.0f;
            float barH = ENEMY_HPBAR_HEIGHT;
            float hpRatio = (float)e->hp / e->maxHp;
            DrawRectangle(
                (int)(e->pos.x - barW / 2),
                (int)(e->pos.y - e->size - ENEMY_HPBAR_YOFFSET),
                (int)barW,
                (int)barH,
                DARKGRAY);
            DrawRectangle(
                (int)(e->pos.x - barW / 2),
                (int)(e->pos.y - e->size - ENEMY_HPBAR_YOFFSET), 
                (int)(barW * hpRatio), 
                (int)barH, 
                RED);
        }
    }

    // --- Player — RGB color cube ---
    bool visible = true;
    if (p->iFrames > 0) {
        visible = ((int)(p->iFrames * PLAYER_BLINK_RATE) % 2 == 0);
    }

    if (visible) {
        float t = (float)GetTime();
        float rotY = t * PLAYER_ROT_SPEED;
        float rotX = PLAYER_ROT_TILT;

        // Ghost trail during dash
        if (p->dash.active) {
            for (int gi = DASH_GHOST_COUNT; gi >= 1; gi--) {
                float offset = (float)gi * DASH_GHOST_SPACING;
                Vector2 ghostPos = Vector2Subtract(p->pos,
                    Vector2Scale(p->dash.dir, offset));
                float ga = 0.5f * (1.0f - (float)gi / (DASH_GHOST_COUNT + 1.0f));
                DrawPlayerSolid(
                    ghostPos,
                    p->size,
                    rotY - (float)gi * DASH_GHOST_ROT_STEP,
                    rotX,
                    ga,
                    (Vector2){0}, 0);
            }
        }

        // Decoy ghost — pulsing translucent player at anchored pos
        if (p->dash.decoyActive) {
            float pulse = sinf((float)GetTime() * DECOY_PULSE_SPEED);
            float da = DECOY_MIN_ALPHA
                + (DECOY_MAX_ALPHA - DECOY_MIN_ALPHA)
                * (pulse * 0.5f + 0.5f);
            DrawPlayerSolid(
                p->dash.decoyPos, p->size,
                rotY, rotX, da,
                (Vector2){0}, 0);
        }

        // Shadow drawn inside DrawPlayerSolid at lagged position
        Vector2 shadowPos = {
            p->shadowPos.x + SHADOW_OFFSET_X,
            p->shadowPos.y + SHADOW_OFFSET_Y
        };
        DrawPlayerSolid(p->pos, p->size, rotY, rotX, 1.0f, shadowPos, SHADOW_ALPHA);

        // Shield arc
        if (p->shield.active && p->shield.hp > 0) {
            float shieldAlpha = p->shield.hp / p->shield.maxHp;
            float startAngle = p->shield.angle - SHIELD_ARC / 2.0f;
            for (int si = 0; si < SHIELD_SEGMENTS; si++) {
                float t0 = (float)si / SHIELD_SEGMENTS;
                float t1 = (float)(si + 1) / SHIELD_SEGMENTS;
                float a0 = startAngle + SHIELD_ARC * t0;
                float a1 = startAngle + SHIELD_ARC * t1;
                Vector2 p0 = Vector2Add(p->pos,
                    (Vector2){ cosf(a0) * SHIELD_RADIUS,
                               sinf(a0) * SHIELD_RADIUS });
                Vector2 p1 = Vector2Add(p->pos,
                    (Vector2){ cosf(a1) * SHIELD_RADIUS,
                               sinf(a1) * SHIELD_RADIUS });
                DrawLineEx(p0, p1, 3.0f,
                    Fade((Color)SHIELD_COLOR, shieldAlpha));
            }
            // Inner glow arc
            for (int si = 0; si < SHIELD_SEGMENTS; si++) {
                float t0 = (float)si / SHIELD_SEGMENTS;
                float t1 = (float)(si + 1) / SHIELD_SEGMENTS;
                float a0 = startAngle + SHIELD_ARC * t0;
                float a1 = startAngle + SHIELD_ARC * t1;
                float innerR = SHIELD_RADIUS * 0.85f;
                Vector2 p0 = Vector2Add(p->pos,
                    (Vector2){ cosf(a0) * innerR, sinf(a0) * innerR });
                Vector2 p1 = Vector2Add(p->pos,
                    (Vector2){ cosf(a1) * innerR, sinf(a1) * innerR });
                DrawLineEx(p0, p1, 1.5f,
                    Fade((Color)SHIELD_COLOR, shieldAlpha * 0.3f));
            }
        }

        // Ground Slam expanding cone
        if (p->slam.vfxTimer > 0) {
            float progress = 1.0f - (p->slam.vfxTimer / SLAM_VFX_DURATION);
            float r = SLAM_RANGE * progress;
            float alpha = 1.0f - progress;
            float halfArc = SLAM_ARC * 0.5f;
            float a = p->slam.angle;
            int segs = 12;
            // outer arc
            for (int i = 0; i < segs; i++) {
                float a0 = a - halfArc + (float)i / segs * SLAM_ARC;
                float a1 = a - halfArc + (float)(i + 1) / segs * SLAM_ARC;
                Vector2 p0 = Vector2Add(p->pos,
                    (Vector2){ cosf(a0) * r, sinf(a0) * r });
                Vector2 p1 = Vector2Add(p->pos,
                    (Vector2){ cosf(a1) * r, sinf(a1) * r });
                DrawLineEx(p0, p1, 2.0f, Fade((Color)SLAM_COLOR, alpha));
            }
            // side edges
            Vector2 left = Vector2Add(p->pos,
                (Vector2){ cosf(a - halfArc) * r, sinf(a - halfArc) * r });
            Vector2 right = Vector2Add(p->pos,
                (Vector2){ cosf(a + halfArc) * r, sinf(a + halfArc) * r });
            DrawLineEx(p->pos, left, 1.5f, Fade(WHITE, alpha * 0.5f));
            DrawLineEx(p->pos, right, 1.5f, Fade(WHITE, alpha * 0.5f));
        }

        // Parry active flash
        if (p->parry.active) {
            float alpha = p->parry.timer / PARRY_WINDOW;
            DrawCircleLinesV(p->pos, p->size + 20.0f,
                Fade((Color)PARRY_COLOR, alpha));
            DrawCircleLinesV(p->pos, p->size + 16.0f,
                Fade((Color)PARRY_COLOR, alpha * 0.5f));
        }

        // Gun barrel (heat-tinted)
        float ca = cosf(p->angle), sa = sinf(p->angle);
        // Barrel color lerps DARKGRAY -> RED based on heat
        Color barrelColor = DARKGRAY;
        if (p->primary == WPN_GUN && p->gun.heat > 0) {
            float h = p->gun.heat;
            barrelColor = (Color){
                (u8)(80 + (int)(175 * h)),
                (u8)(80 - (int)(80 * h)),
                (u8)(80 - (int)(80 * h)),
                255 };
        }
        if (p->primary == WPN_GUN && p->minigun.spinUp > 0) {
            Vector2 miniTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + MINIGUN_TIP_OFFSET),
                           sa * (p->size + MINIGUN_TIP_OFFSET) });
            DrawLineEx(p->pos, miniTip, MINIGUN_BARREL_THICKNESS, barrelColor);
            // spinning barrel lines
            float spin = (float)GetTime() * p->minigun.spinUp * 30.0f;
            for (int b = 0; b < 3; b++) {
                float bAngle = p->angle + spin + b * (2.0f * PI / 3.0f);
                float bca = cosf(bAngle), bsa = sinf(bAngle);
                Vector2 bOff = { -bsa * 2.0f, bca * 2.0f };
                Vector2 bStart = Vector2Add(p->pos, bOff);
                Vector2 bEnd = Vector2Add(miniTip, bOff);
                DrawLineEx(bStart, bEnd, 1.0f,
                    Fade(LIGHTGRAY, 0.4f * p->minigun.spinUp));
            }
        } else if (p->primary == WPN_ROCKET) {
            // Thicker rocket tube barrel
            Vector2 rocketTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + MINIGUN_TIP_OFFSET),
                           sa * (p->size + MINIGUN_TIP_OFFSET) });
            DrawLineEx(p->pos, rocketTip, MINIGUN_BARREL_THICKNESS + 1.0f, DARKGRAY);
        } else if (p->primary == WPN_SNIPER) {
            // Long sniper barrel — tint shows ADS/super state
            Color sniperBarrelColor = DARKGRAY;
            if (p->sniper.superShotReady) sniperBarrelColor = GOLD;
            else if (p->sniper.aiming)    sniperBarrelColor = WHITE;
            Vector2 sniperTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + MINIGUN_TIP_OFFSET),
                           sa * (p->size + MINIGUN_TIP_OFFSET) });
            DrawLineEx(p->pos, sniperTip, GUN_BARREL_THICKNESS, sniperBarrelColor);
        } else {
            Vector2 gunTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + GUN_TIP_OFFSET),
                           sa * (p->size + GUN_TIP_OFFSET) });
            DrawLineEx(p->pos, gunTip, GUN_BARREL_THICKNESS, barrelColor);
        }

#if 0 // laser beam draw — preserved for future use
        // --- Laser beam (live while key held) ---
        if (p->laser.active) {
            Vector2 aimDir = { cosf(p->angle), sinf(p->angle) };
            Vector2 muzzle = Vector2Add(p->pos,
                Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
            DrawLineEx(muzzle, p->laser.beamTip, LASER_GLOW_WIDTH, LASER_GLOW_COLOR);
            DrawLineEx(muzzle, p->laser.beamTip, LASER_BEAM_WIDTH, LASER_COLOR);
        }
#endif
    }

    // --- Beam pool (railgun lingering flash) ---
    for (int i = 0; i < MAX_BEAMS; i++) {
        Beam *b = &g.beams[i];
        if (!b->active) continue;
        float t = b->timer / b->duration;
        Color glow = { RAILGUN_GLOW_COLOR.r, RAILGUN_GLOW_COLOR.g,
                       RAILGUN_GLOW_COLOR.b, (u8)(80.0f * t) };
        Color core = { b->color.r, b->color.g, b->color.b, (u8)(255.0f * t) };
        DrawLineEx(b->origin, b->tip, RAILGUN_GLOW_WIDTH * t, glow);
        DrawLineEx(b->origin, b->tip, b->width, core);
    }

    // --- Sword swing arc ---
    if (p->sword.timer > 0 && !p->sword.lunge) {
        float progress = 1.0f - (p->sword.timer / p->sword.duration);
        float sweepAngle =
            p->sword.angle - p->sword.arc / 2.0f + p->sword.arc * progress;
        int numSegments = SWORD_DRAW_SEGMENTS;
        float arcHalf = p->sword.arc / 2.0f;
        Color arcColor = p->sword.dashSlash ? SKYBLUE : ORANGE;
        for (int i = 1; i <= numSegments; i++) {
            float segPos = (float)i / numSegments;
            if (segPos > progress) break;

            float t = segPos / progress;
            float alpha = t * t * 0.9f;
            float thick = 2.0f + t * 6.0f;

            float a0 = p->sword.angle - arcHalf
                + p->sword.arc * (float)(i - 1) / numSegments;
            float a1 = p->sword.angle - arcHalf
                + p->sword.arc * (float)i / numSegments;
            Vector2 pt0 = Vector2Add(p->pos,
                (Vector2){ cosf(a0) * p->sword.radius,
                           sinf(a0) * p->sword.radius });
            Vector2 pt1 = Vector2Add(p->pos,
                (Vector2){ cosf(a1) * p->sword.radius,
                           sinf(a1) * p->sword.radius });
            DrawLineEx(pt0, pt1, thick, Fade(arcColor, alpha));
        }
        Vector2 sweepEnd = Vector2Add(
            p->pos,
            (Vector2){
                cosf(sweepAngle) * p->sword.radius,
                sinf(sweepAngle) * p->sword.radius
            });
        DrawLineEx(p->pos, sweepEnd, 3.0f, WHITE);
    }
    // --- Sword lunge thrust ---
    if (p->sword.timer > 0 && p->sword.lunge) {
        float range = p->sword.dashSlash ?
            LUNGE_RANGE * LUNGE_DASH_RANGE_MULT : LUNGE_RANGE;
        float progress = 1.0f - (p->sword.timer / LUNGE_DURATION);
        Vector2 dir = { cosf(p->sword.angle), sinf(p->sword.angle) };
        Vector2 tip = Vector2Add(p->pos, Vector2Scale(dir, range));
        Color lungeColor = p->sword.dashSlash ? SKYBLUE : RED;

        // Cone edges
        Vector2 leftDir = { cosf(p->sword.angle - LUNGE_CONE_HALF),
                            sinf(p->sword.angle - LUNGE_CONE_HALF) };
        Vector2 rightDir = { cosf(p->sword.angle + LUNGE_CONE_HALF),
                             sinf(p->sword.angle + LUNGE_CONE_HALF) };
        Vector2 leftTip = Vector2Add(p->pos, Vector2Scale(leftDir, range));
        Vector2 rightTip = Vector2Add(p->pos, Vector2Scale(rightDir, range));

        // Trailing cone fill
        float fadeOut = p->sword.timer / LUNGE_DURATION;
        DrawTriangle(p->pos, leftTip, rightTip, Fade(lungeColor, 0.15f * fadeOut));

        // Cone edge lines
        DrawLineEx(p->pos, leftTip, 1.5f, Fade(lungeColor, 0.5f * fadeOut));
        DrawLineEx(p->pos, rightTip, 1.5f, Fade(lungeColor, 0.5f * fadeOut));

        // Leading thrust line (thick white center)
        float thick = 3.0f + progress * 4.0f;
        DrawLineEx(p->pos, tip, thick, Fade(WHITE, 0.9f * fadeOut));

        // Tip dot
        DrawCircleV(tip, 4.0f, Fade(lungeColor, fadeOut));
    }

    // --- Spin attack trailing arc ---
    if (p->spin.timer > 0) {
        float progress = 1.0f - (p->spin.timer / p->spin.duration);
        float fadeOut = p->spin.timer / p->spin.duration;

        // 2 full rotations over the spin duration
        float totalSweep = PI * 4.0f;
        float sweepAngle = totalSweep * progress;

        int numSegments = SPIN_DRAW_SEGMENTS;
        float trailLength = SPIN_TRAIL_LENGTH;
        float actualTrail = fminf(trailLength, sweepAngle);

        // Outer trailing arc (YELLOW)
        for (int i = 0; i < numSegments; i++) {
            float t = (float)(i + 1) / numSegments;
            float segAlpha = t * t * fadeOut * 0.9f;
            float thick = 2.0f + t * 5.0f;

            float a0 = sweepAngle
                - actualTrail * (1.0f - (float)i / numSegments);
            float a1 = sweepAngle
                - actualTrail * (1.0f - (float)(i + 1) / numSegments);

            Vector2 pt0 = Vector2Add(p->pos,
                (Vector2){ cosf(a0) * p->spin.radius,
                           sinf(a0) * p->spin.radius });
            Vector2 pt1 = Vector2Add(p->pos,
                (Vector2){ cosf(a1) * p->spin.radius,
                           sinf(a1) * p->spin.radius });
            DrawLineEx(pt0, pt1, thick, Fade(YELLOW, segAlpha));
        }

        // Inner trailing arc (ORANGE, smaller radius)
        for (int i = 0; i < numSegments; i++) {
            float t = (float)(i + 1) / numSegments;
            float segAlpha = t * t * fadeOut * 0.4f;
            float thick = 1.0f + t * 3.0f;

            float a0 = sweepAngle
                - actualTrail * (1.0f - (float)i / numSegments);
            float a1 = sweepAngle
                - actualTrail * (1.0f - (float)(i + 1) / numSegments);

            Vector2 pt0 = Vector2Add(p->pos,
                (Vector2){ cosf(a0) * p->spin.radius * SPIN_INNER_RADIUS_FRAC,
                           sinf(a0) * p->spin.radius * SPIN_INNER_RADIUS_FRAC });
            Vector2 pt1 = Vector2Add(p->pos,
                (Vector2){ cosf(a1) * p->spin.radius * SPIN_INNER_RADIUS_FRAC,
                           sinf(a1) * p->spin.radius * SPIN_INNER_RADIUS_FRAC });
            DrawLineEx(pt0, pt1, thick, Fade(ORANGE, segAlpha));
        }

        // White leading edge line
        Vector2 sweepEnd = Vector2Add(p->pos,
            (Vector2){ cosf(sweepAngle) * p->spin.radius,
                       sinf(sweepAngle) * p->spin.radius });
        DrawLineEx(p->pos, sweepEnd, 3.0f, Fade(WHITE, fadeOut));
    }
}

// ========================================================================== /
// Draw — HUD (screen space)
// ========================================================================== /
static void DrawHUD(void)
{
    Player *p = &g.player;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float ui = (float)sh / HUD_SCALE_REF;

    // HP bar
    int hpBarW = (int)(HUD_HP_W * ui);
    int hpBarH = (int)(HUD_HP_H * ui);
    int hpBarX = (int)(HUD_MARGIN * ui);
    int hpBarY = (int)(HUD_MARGIN * ui);
    float hpRatio = (float)p->hp / p->maxHp;
    DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, DARKGRAY);
    Color hpColor = (hpRatio > 0.5f) ? GREEN : (hpRatio > 0.25f) ? ORANGE : RED;
    DrawRectangle(hpBarX, hpBarY, (int)(hpBarW * hpRatio), hpBarH, hpColor);
    DrawRectangleLines(hpBarX, hpBarY, hpBarW, hpBarH, WHITE);
    DrawText(
        TextFormat("HP: %d/%d", (int)p->hp, (int)p->maxHp),
        hpBarX + (int)(HUD_HP_TEXT_PAD_X * ui), hpBarY + (int)(HUD_HP_TEXT_PAD_Y * ui), (int)(HUD_HP_FONT * ui), WHITE);

    // Score
    DrawText(
        TextFormat("Score: %d", g.score), 
        (int)(HUD_MARGIN * ui), (int)(HUD_SCORE_Y * ui), (int)(HUD_SCORE_FONT * ui), WHITE);
    DrawText(
        TextFormat("Kills: %d", g.enemiesKilled),
        (int)(HUD_MARGIN * ui), (int)(HUD_KILLS_Y * ui), (int)(HUD_KILLS_FONT * ui), LIGHTGRAY);

    // Weapon swap indicator
    {
        int wFont = (int)(HUD_CD_FONT * ui);
        int wX = (int)(HUD_MARGIN * ui);
        int wY = (int)(HUD_CD_Y * ui) - (int)(20 * ui);
        const char *pri = WeaponName(p->primary);
        const char *sec = WeaponName(p->secondary);
        DrawText(pri, wX, wY, wFont, SELECT_HIGHLIGHT_COLOR);
        int secX = wX + MeasureText(pri, wFont) + (int)(6 * ui);
        DrawText(TextFormat("/ %s", sec), secX, wY, wFont,
            Fade(WHITE, 0.35f));
        int ctrlX = secX + MeasureText(TextFormat("/ %s", sec), wFont)
            + (int)(6 * ui);
        DrawText("[Ctrl]", ctrlX, wY, wFont, Fade(WHITE, 0.2f));
    }

    // Cooldown indicators
    int cdBarW = (int)(HUD_CD_W * ui);
    int cdBarH = (int)(HUD_CD_H * ui);
    int cdX = (int)(HUD_MARGIN * ui);
    int cdY = (int)(HUD_CD_Y * ui);
    int cdFontSize = (int)(HUD_CD_FONT * ui);
    int cdLabelX = cdX + cdBarW + (int)(HUD_CD_LABEL_GAP * ui);
    
    // Dash charges
    {
        int pipW = (int)(HUD_PIP_W * ui);
        int pipGap = (int)(HUD_PIP_GAP * ui);
        for (int i = 0; i < p->dash.maxCharges; i++) {
            int pipX = cdX + i * (pipW + pipGap);
            if (i < p->dash.charges) {
                DrawRectangle(pipX, cdY, pipW, cdBarH, SKYBLUE);
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, WHITE);
            } else if (i == p->dash.charges
                && p->dash.charges < p->dash.maxCharges) {
                float ratio = 1.0f
                    - (p->dash.rechargeTimer / p->dash.rechargeTime);
                DrawRectangle(pipX, cdY, (int)(pipW * ratio), cdBarH,
                    Fade(SKYBLUE, 0.4f));
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            } else {
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            }
        }
        int labelX = cdX
            + p->dash.maxCharges * (pipW + pipGap) + (int)(2 * ui);
        DrawText("DASH", labelX, cdY, cdFontSize,
            p->dash.charges > 0 ? SKYBLUE : GRAY);
    }

    // Shotgun blasts (pip display like dash)
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        int pipW = (int)(HUD_PIP_W * ui);
        int pipGap = (int)(HUD_PIP_GAP * ui);
        for (int i = 0; i < SHOTGUN_BLASTS; i++) {
            int pipX = cdX + i * (pipW + pipGap);
            if (i < p->shotgun.blastsLeft) {
                DrawRectangle(pipX, cdY, pipW, cdBarH, ORANGE);
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, WHITE);
            } else if (p->shotgun.blastsLeft == 0
                && p->shotgun.cooldownTimer > 0) {
                float ratio = 1.0f
                    - (p->shotgun.cooldownTimer / SHOTGUN_COOLDOWN);
                DrawRectangle(pipX, cdY, (int)(pipW * ratio), cdBarH,
                    Fade(ORANGE, 0.4f));
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            } else {
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            }
        }
        int labelX = cdX
            + SHOTGUN_BLASTS * (pipW + pipGap) + (int)(2 * ui);
        DrawText("SHTGN", labelX, cdY, cdFontSize,
            p->shotgun.blastsLeft > 0 ? ORANGE : GRAY);
    }


    // rockets
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        if (p->rocket.cooldownTimer > 0) {
            float cdRatio = p->rocket.cooldownTimer / p->rocket.cooldown;
            DrawRectangle(cdX, cdY,
                (int)(cdBarW * (1.0f - cdRatio)), cdBarH, RED);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("ROCKET", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, RED);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("ROCKET", cdLabelX, cdY, cdFontSize, RED);
        }
    }

    // Railgun cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color rgColor = RAILGUN_COLOR;
        if (p->railgun.cooldownTimer > 0) {
            float ratio = 1.0f - (p->railgun.cooldownTimer / RAILGUN_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, rgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("RAIL", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, rgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("RAIL", cdLabelX, cdY, cdFontSize, rgColor);
        }
    }

    // Sniper cooldown (when sniper is equipped)
    if (p->primary == WPN_SNIPER || p->secondary == WPN_SNIPER) {
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        // Label and color change with mode
        const char *snLabel;
        Color snColor;
        float snCooldown;
        if (p->sniper.superShotReady) {
            snLabel = "SUPER";
            snColor = GOLD;
            snCooldown = SNIPER_AIM_COOLDOWN;
        } else if (p->sniper.aiming) {
            snLabel = "AIM";
            snColor = WHITE;
            snCooldown = SNIPER_AIM_COOLDOWN;
        } else {
            snLabel = "HIP";
            snColor = SNIPER_COLOR;
            snCooldown = SNIPER_HIP_COOLDOWN;
        }
        if (p->sniper.cooldownTimer > 0) {
            float ratio = 1.0f - (p->sniper.cooldownTimer / snCooldown);
            if (ratio > 1.0f) ratio = 1.0f;
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, snColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText(snLabel, cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, snColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText(snLabel, cdLabelX, cdY, cdFontSize, snColor);
        }
    }
    }

    // Grenade cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color grColor = GRENADE_COLOR;
        if (p->grenade.cooldownTimer > 0) {
            float ratio = 1.0f - (p->grenade.cooldownTimer / GRENADE_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, grColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("GREN", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, grColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("GREN", cdLabelX, cdY, cdFontSize, grColor);
        }
    }

    // BFG cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color bfgColor = BFG_COLOR;
        float chargeRatio = p->bfg.charge / BFG_CHARGE_COST;
        if (chargeRatio < 1.0f || p->bfg.active) {
            DrawRectangle(cdX, cdY, (int)(cdBarW * chargeRatio), cdBarH, bfgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("BFG", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, bfgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("BFG", cdLabelX, cdY, cdFontSize, bfgColor);
        }
    }

    // Shield HP bar
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color shColor = (Color)SHIELD_COLOR;
        float hpRatio = p->shield.hp / p->shield.maxHp;
        bool broken = p->shield.regenTimer < 0;
        if (broken) {
            // Show broken cooldown
            float cd = -p->shield.regenTimer / SHIELD_BROKEN_COOLDOWN;
            DrawRectangle(cdX, cdY,
                (int)(cdBarW * (1.0f - cd)), cdBarH,
                Fade(RED, 0.6f));
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("SHLD", cdLabelX, cdY, cdFontSize, RED);
        } else if (hpRatio < 1.0f) {
            DrawRectangle(cdX, cdY,
                (int)(cdBarW * hpRatio), cdBarH, shColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("SHLD", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, shColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("SHLD", cdLabelX, cdY, cdFontSize, shColor);
        }
    }

    // Slam cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color slColor = SLAM_COLOR;
        if (p->slam.cooldownTimer > 0) {
            float ratio = 1.0f - (p->slam.cooldownTimer / SLAM_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, slColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("SLAM", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, slColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("SLAM", cdLabelX, cdY, cdFontSize, slColor);
        }
    }

    // Parry cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color paColor = PARRY_COLOR;
        if (p->parry.active) {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("PARRY", cdLabelX, cdY, cdFontSize, WHITE);
        } else if (p->parry.cooldownTimer > 0) {
            float maxCd = p->parry.succeeded
                ? PARRY_SUCCESS_COOLDOWN : PARRY_COOLDOWN;
            float ratio = 1.0f - (p->parry.cooldownTimer / maxCd);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, paColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("PARRY", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, paColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("PARRY", cdLabelX, cdY, cdFontSize, paColor);
        }
    }

    // Turret cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color tuColor = TURRET_COLOR;
        if (p->turretCooldown > 0) {
            float ratio = 1.0f - (p->turretCooldown / TURRET_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, tuColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("TURRT", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, tuColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("TURRT", cdLabelX, cdY, cdFontSize, tuColor);
        }
    }

    // Mine cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color miColor = MINE_COLOR;
        if (p->mineCooldown > 0) {
            float ratio = 1.0f - (p->mineCooldown / MINE_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, miColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("MINE", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, miColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("MINE", cdLabelX, cdY, cdFontSize, miColor);
        }
    }

    // Heal cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color heColor = HEAL_COLOR;
        if (p->healCooldown > 0) {
            float ratio = 1.0f - (p->healCooldown / HEAL_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, heColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("HEAL", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, heColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("HEAL", cdLabelX, cdY, cdFontSize, heColor);
        }
    }

    // Fuel bar
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color fiColor = FLAME_COLOR;
        float ratio = p->flame.fuel / FLAME_FUEL_MAX;
        DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, fiColor);
        DrawRectangleLines(cdX, cdY, cdBarW, cdBarH,
            p->flame.active ? WHITE : GRAY);
        DrawText("FUEL", cdLabelX, cdY, cdFontSize,
            (p->flame.fuel > 0) ? fiColor : RED);
    }

    // Active reload/overheat arc (near player) ------------------------------ /
    Vector2 pScreen = GetWorldToScreen2D(p->pos, g.camera);
    pScreen.y += HUD_ARC_Y_OFFSET * ui;
    float arcInner  = HUD_ARC_INNER_R * ui;
    float arcOuter  = HUD_ARC_OUTER_R * ui;
    float arcStart  = HUD_ARC_START_DEG;
    float arcEnd    = HUD_ARC_END_DEG;
    float arcSpan   = arcEnd - arcStart;
    int   arcSegs   = HUD_ARC_SEGMENTS;
    int   arcFont   = (int)(HUD_ARC_LABEL_FONT * ui);
    float arcCurPad = HUD_ARC_CURSOR_PAD * ui;

    // Revolver arc (always visible for revolver players)
    if (p->primary == WPN_REVOLVER) {
        Color revColor = (Color){ 220, 180, 80, 255 };
        Color bonusColor = GOLD;
        float segGap = HUD_ARC_SEG_GAP;
        float segAngle = (arcSpan
            - (REVOLVER_ROUNDS - 1) * segGap) / REVOLVER_ROUNDS;

        // 6 round segments
        for (int i = 0; i < REVOLVER_ROUNDS; i++) {
            float sEnd = arcEnd - i * (segAngle + segGap);
            float sStart = sEnd - segAngle;
            bool loaded = i < p->revolver.rounds;
            bool isBonus = i < p->revolver.bonusRounds;

            if (loaded) {
                // Bonus glow
                if (isBonus) {
                    float pulse = 0.3f
                        + 0.2f * sinf(GetTime() * 8.0f);
                    DrawRing(pScreen,
                        arcInner - arcCurPad,
                        arcOuter + arcCurPad,
                        sStart, sEnd, arcSegs,
                        Fade(bonusColor, pulse));
                }
                DrawRing(pScreen, arcInner, arcOuter,
                    sStart, sEnd, arcSegs,
                    isBonus ? bonusColor : revColor);
            } else {
                DrawRing(pScreen, arcInner, arcOuter,
                    sStart, sEnd, arcSegs,
                    (Color){ 40, 40, 40, 160 });
            }
        }

        // Reload overlay
        if (p->revolver.reloadTimer > 0) {
            float ratio = 1.0f
                - (p->revolver.reloadTimer
                    / REVOLVER_RELOAD_TIME);
            if (ratio > 1.0f) ratio = 1.0f;
            if (ratio < 0.0f) ratio = 0.0f;

            // Sweet spot zone
            float sweetSA = arcEnd
                - REVOLVER_RELOAD_SWEET_END * arcSpan;
            float sweetEA = arcEnd
                - REVOLVER_RELOAD_SWEET_START * arcSpan;
            Color sweetColor;
            if (p->revolver.reloadLocked) {
                bool hit = p->revolver.reloadTimer
                    <= REVOLVER_RELOAD_FAST_TIME + 0.01f;
                sweetColor = hit
                    ? (Color){ 60, 255, 60, 120 }
                    : (Color){ 255, 60, 60, 120 };
            } else {
                sweetColor = (Color){ 255, 255, 100, 100 };
            }
            DrawRing(pScreen, arcInner, arcOuter,
                sweetSA, sweetEA, arcSegs, sweetColor);

            // Progress sweep
            float progressA = arcEnd - ratio * arcSpan;
            DrawRing(pScreen, arcInner, arcOuter,
                progressA, arcEnd, arcSegs,
                Fade(SKYBLUE, 0.3f));

            // Cursor line
            float curRad = progressA * DEG2RAD;
            Vector2 c1 = {
                pScreen.x + (arcInner - arcCurPad)
                    * cosf(curRad),
                pScreen.y + (arcInner - arcCurPad)
                    * sinf(curRad) };
            Vector2 c2 = {
                pScreen.x + (arcOuter + arcCurPad)
                    * cosf(curRad),
                pScreen.y + (arcOuter + arcCurPad)
                    * sinf(curRad) };
            DrawLineEx(c1, c2, 2.0f * ui, WHITE);

            // Label
            const char *rlText = "RELOAD";
            int rlW = MeasureText(rlText, arcFont);
            DrawText(rlText,
                (int)pScreen.x - rlW / 2,
                (int)(pScreen.y + arcOuter
                    + HUD_ARC_LABEL_PAD * ui),
                arcFont, GRAY);
        }
    }

    // Gun heat arc (always visible for gun players)
    if (p->primary == WPN_GUN) {
        float h = p->gun.heat;

        // Background arc (always present)
        DrawRing(pScreen, arcInner, arcOuter,
            arcStart, arcEnd, arcSegs, (Color){ 40, 40, 40, 160 });

        // Heat fill — color gradient WHITE -> YELLOW -> RED
        if (h > 0) {
            Color heatColor;
            if (h < 0.5f) {
                float t = h * 2.0f;
                heatColor = (Color){
                    255, 255,
                    (u8)(255 - (int)(255 * t)), 255 };
            } else {
                float t = (h - 0.5f) * 2.0f;
                heatColor = (Color){
                    255, (u8)(255 - (int)(255 * t)),
                    0, 255 };
            }
            float heatA = arcEnd - h * arcSpan;
            DrawRing(pScreen, arcInner, arcOuter,
                heatA, arcEnd, arcSegs, Fade(heatColor, 0.6f));
        }

        // Overheat QTE overlay
        if (p->gun.overheated) {
            // Sweet spot zone
            float zSA = arcEnd
                - (p->gun.ventZoneStart + p->gun.ventZoneWidth)
                * arcSpan;
            float zEA = arcEnd
                - p->gun.ventZoneStart * arcSpan;
            Color zoneColor = (p->gun.ventResult == 1) ? GREEN
                : (p->gun.ventResult == -1)
                    ? (Color){ 80, 20, 20, 255 } : YELLOW;
            DrawRing(pScreen, arcInner, arcOuter,
                zSA, zEA, arcSegs, zoneColor);

            // Cursor line (only while pending)
            if (p->gun.ventResult == 0) {
                float curA = arcEnd
                    - p->gun.ventCursor * arcSpan;
                float curRad = curA * DEG2RAD;
                Vector2 c1 = {
                    pScreen.x + (arcInner - arcCurPad)
                        * cosf(curRad),
                    pScreen.y + (arcInner - arcCurPad)
                        * sinf(curRad) };
                Vector2 c2 = {
                    pScreen.x + (arcOuter + arcCurPad)
                        * cosf(curRad),
                    pScreen.y + (arcOuter + arcCurPad)
                        * sinf(curRad) };
                DrawLineEx(c1, c2, 2.0f * ui, WHITE);
            }

            // QTE label
            const char *ovLabel =
                (p->gun.ventResult == 1) ? "VENT"
                : (p->gun.ventResult == -1) ? "OVHT" : "[R]";
            Color ovColor = (p->gun.ventResult == 1) ? GREEN
                : (p->gun.ventResult == -1) ? RED : YELLOW;
            int ovW = MeasureText(ovLabel, arcFont);
            DrawText(ovLabel,
                (int)pScreen.x - ovW / 2,
                (int)(pScreen.y + arcOuter
                    + HUD_ARC_LABEL_PAD * ui),
                arcFont, ovColor);
        }
    }

    // FPS (top-right)
    int fpsFont = (int)(HUD_FPS_FONT * ui);
    DrawText(TextFormat("%d FPS",
        GetFPS()), sw - (int)(HUD_FPS_X * ui), (int)(HUD_MARGIN * ui), fpsFont, GREEN);

    // Crosshair cursor
    Vector2 mouse = GetMousePosition();
    // we can refactor this crosshair to default
    // eventually make it editable
    float ch = HUD_CROSSHAIR_SIZE * ui;
    float chThick = HUD_CROSSHAIR_THICKNESS * ui;
    float chGap = HUD_CROSSHAIR_GAP * ui;
    //Color chColor = Fade(WHITE, 0.8f);
    Color chColor = Fade(GREEN, 1.0f);
    // we're going to create a gap between in the middle of the cross hair
    // so we need 4 lines
    // middle left
    //DrawLineEx(
    //    (Vector2){ mouse.x - ch, mouse.y }, 
    //    (Vector2){ mouse.x + ch, mouse.y }, 
    //    chThick, chColor);
    DrawLineEx(
        (Vector2){ mouse.x - chGap, mouse.y },
        (Vector2){ mouse.x - ch - chGap, mouse.y },
        chThick, chColor
    );
    // middle right
    DrawLineEx(
        (Vector2){ mouse.x + chGap, mouse.y },
        (Vector2){ mouse.x + ch + chGap, mouse.y },
        chThick, chColor
    );
    // top
    //DrawLineEx(
    //    (Vector2){ mouse.x, mouse.y - ch }, 
    //    (Vector2){ mouse.x, mouse.y + ch }, 
    //    chThick, chColor);
    DrawLineEx(
        (Vector2){ mouse.x, mouse.y - chGap},
        (Vector2){ mouse.x, mouse.y - ch - chGap},
        chThick, chColor
    );
    // bottom
    DrawLineEx(
        (Vector2){ mouse.x, mouse.y + ch + chGap},
        (Vector2){ mouse.x, mouse.y + chGap},
        chThick, chColor
    );
    //DrawCircleLines((int)mouse.x, (int)mouse.y, 6.0f * ui, chColor);
    
    // bottom right, game title text
    int titleFont = (int)(HUD_TITLE_FONT * ui);
    DrawText("Untitled Mecha Game - Version 0.0.1",
             sw - (int)(HUD_TITLE_X * ui),
             sh - (int)(HUD_BOTTOM_Y * ui),
             titleFont,
             Fade(WHITE, 0.8f));

    // Pause overlay
    if (g.paused && !g.gameOver) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.5f));
        int pauseFont = (int)(HUD_PAUSE_FONT * ui);
        const char *pauseText = "PAUSED";
        int pW = MeasureText(pauseText, pauseFont);
        DrawText(
            pauseText,
            sw / 2 - pW / 2,
            sh / 2 - (int)(HUD_PAUSE_Y * ui),
            pauseFont,
            WHITE);
        int resumeFont = (int)(HUD_RESUME_FONT * ui);
        const char *resumeText = "P or Esc to resume";
        int rW = MeasureText(resumeText, resumeFont);
        DrawText(
            resumeText,
            sw / 2 - rW / 2,
            sh / 2 + (int)(HUD_RESUME_Y * ui),
            resumeFont,
            LIGHTGRAY);

        // Keybinds — three columns
        int pkFont = (int)(HUD_PAUSE_KEYS_FONT * ui);
        int pkSpacing = (int)(HUD_PAUSE_KEYS_SPACING * ui);
        int pkY = sh / 2 + (int)(HUD_PAUSE_KEYS_Y * ui);
        int pkTabW = (int)(62 * ui);

        // Left column: core controls
        const char *lKeys[] = {
            "WASD", "Space", "M1", "M2", "Ctrl", "P/Esc", "0"
        };
        const char *lDescs[] = {
            "Move", "Dash", "Primary", "Alt Fire",
            "Swap", "Pause", "Exit"
        };
        int lCount = 7;

        int totalW = (int)(420 * ui);
        int colW = totalW / 3;
        int lColX = sw / 2 - totalW / 2;
        int mColX = lColX + colW;
        int rColX = mColX + colW;

        for (int i = 0; i < lCount; i++) {
            int ky = pkY + i * pkSpacing;
            DrawText(lKeys[i], lColX, ky, pkFont, SELECT_HIGHLIGHT_COLOR);
            DrawText(lDescs[i], lColX + pkTabW, ky, pkFont, Fade(WHITE, 0.6f));
        }

        // Middle + Right columns: abilities from slots (split in half)
        Player *pp = &g.player;
        int ablCount = 0;
        int ablSlots[ABILITY_SLOTS];
        for (int i = 0; i < ABILITY_SLOTS; i++) {
            if (AbilityName(pp->slots[i].ability))
                ablSlots[ablCount++] = i;
        }
        int half = (ablCount + 1) / 2;
        for (int a = 0; a < ablCount; a++) {
            int si = ablSlots[a];
            int colX = (a < half) ? mColX : rColX;
            int row = (a < half) ? a : a - half;
            int ky = pkY + row * pkSpacing;
            DrawText(KeyName(pp->slots[si].key), colX, ky, pkFont, SELECT_HIGHLIGHT_COLOR);
            DrawText(AbilityName(pp->slots[si].ability),
                colX + pkTabW, ky, pkFont, Fade(WHITE, 0.6f));
        }

        // Fullscreen toggle
        int maxRows = lCount > half ? lCount : half;
        int fsY = pkY + (maxRows + 1) * pkSpacing;
        const char *fsLabel = IsWindowFullscreen() ? "[F] Fullscreen: ON" : "[F] Fullscreen: OFF";
        int fsW = MeasureText(fsLabel, pkFont);
        DrawText(fsLabel, sw / 2 - fsW / 2, fsY, pkFont, Fade(WHITE, 0.6f));
    }

    // Game Over overlay
    if (g.gameOver) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.7f));
        int goFont = (int)(HUD_GO_FONT * ui);
        const char *goText = "GAME OVER";
        int goW = MeasureText(goText, goFont);
        DrawText(
            goText,
            sw / 2 - goW / 2,
            sh / 2 - (int)(HUD_GO_Y * ui),
            goFont,
            RED);

        int scFont = (int)(HUD_GO_SCORE_FONT * ui);
        const char *scoreText =
            TextFormat("Score: %d  |  Kills: %d", g.score, g.enemiesKilled);
        int sW = MeasureText(scoreText, scFont);
        DrawText(
            scoreText,
            sw / 2 - sW / 2,
            sh / 2 + (int)(HUD_GO_SCORE_Y * ui),
            scFont,
            WHITE);

        int rsFont = (int)(HUD_GO_RESTART_FONT * ui);
        const char *restartText = "Press ENTER to restart";
        int rW = MeasureText(restartText, rsFont);
        DrawText(
            restartText,
            sw / 2 - rW / 2,
            sh / 2 + (int)(HUD_GO_RESTART_Y * ui),
            rsFont,
            LIGHTGRAY);
    }
}

// ========================================================================== /
// Draw — orchestrator
// ========================================================================== /
static void DrawGame(void)
{
    if (g.screen == SCREEN_SELECT) {
        DrawSelect();
        return;
    }

    BeginDrawing();
    ClearBackground(BG_COLOR);

    BeginMode2D(g.camera);
    DrawWorld();
    EndMode2D();

    DrawHUD();

    EndDrawing();
}

// ========================================================================== /
// Main loop callback
// ========================================================================== /
void NextFrame(void)
{
    if (g.screen == SCREEN_PLAYING && !IsCursorHidden()) HideCursor();
    if (g.screen != SCREEN_PLAYING &&  IsCursorHidden()) ShowCursor();
    UpdateGame();
    DrawGame();
}

// ========================================================================== /
// main
// ========================================================================== /
// I am confident about this.
int main(void)
{
    //SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W, SCREEN_H, "mecha prototype");
#ifndef PLATFORM_WEB
    // 0 is primary, 1 is secondary, etc
    // hmm it should first be primary, but if primary is h>w then choose secondary
    // how to get monitor screenwidth stats?
    int monitor = 0;
    if (monitor < GetMonitorCount()) {
        SetWindowMonitor(monitor);
        // center on that monitor
        int mx = GetMonitorWidth(monitor);
        int my = GetMonitorHeight(monitor);
        Vector2 pos = GetMonitorPosition(monitor);
        SetWindowPosition(pos.x + (float)(mx - SCREEN_W) / 2,
                          pos.y + (float)(my - SCREEN_H) / 2);
    }
#endif
    SetExitKey(KEY_ZERO);
    InitGame();
#ifdef PLATFORM_WEB
    emscripten_set_main_loop(NextFrame, 0, 1);
#else
    // target should not be 60 but unlimited or max screen refresh rate 
    // minimum 60 even on 30fps screens though? or do we want to do the whole
    // internal game logic fps vs draw fps? internal 120? 240? light game...
    //SetTargetFPS(60);
    SetTargetFPS(240);
    while (!WindowShouldClose()) {
        NextFrame();
    }
#endif
    CloseWindow();
    return 0;

