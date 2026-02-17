/******************************************************************************
* Mecha Bullet Hell - Mini Prototype
*
* Version 0.0.1
******************************************************************************/
#include "raylib.h"
#include "raymath.h"
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "rtypes.h"
#include "default.h"

// ========================================================================== /
// Data Structures
// ========================================================================== /

// player ------------------------------------------------------------------- /
// declare abilities above player
typedef struct Gun {
    float cooldown;
    float fireRate;
} Gun;

typedef struct Sword {
    float timer;
    float angle;
    float duration;
    float arc;
    float radius;
    bool dashSlash;
    float lastHitAngle[MAX_ENEMIES];
} Sword;

typedef struct Dash {
    bool active;
    float timer;
    float speed;
    float duration;
    float rechargeTime;
    float rechargeTimer;
    int charges;
    int maxCharges;
    Vector2 dir;
} Dash;

typedef struct Spin {
    float timer;
    float duration;
    float radius;
    float cooldown;
    float cooldownTimer;
    float lastHitAngle[MAX_ENEMIES];
} Spin;

typedef struct Shotgun {
    int blastsLeft;
    float cooldownTimer;
} Shotgun;

typedef struct Rocket {
    int rocketsLeft;
    float cooldown;
    float cooldownTimer;
} Rocket;

typedef struct Player {
    Vector2 pos;
    Vector2 vel;
    float angle;
    float speed;
    float size;
    int hp, maxHp;
    float iFrames;
    Gun gun;
    Sword sword;
    Dash dash;
    Spin spin;
    Shotgun shotgun;
    Rocket rocket;
} Player;

// damage method and type ------------------------------------------------------ /
typedef enum DamageMethod {
    HIT_PROJ,       // travel time
    HIT_SCAN,       // instant
    HIT_MELEE,      // sweep arc
    HIT_AOE,        // radius from center point
    // maybe aoe + melee are similar? just an arc amount
} DamageMethod;

typedef enum DamageType {
    DMG_BALLISTIC,
    DMG_EXPLOSIVE,
    DMG_SLASH,      // there are a few types of melee damage
    DMG_PIERCE,
    DMG_BLUNT,
    DMG_ABILITY,    // spirit/special
    DMG_PURE,       //
} DamageType;

// Damage struct — preserved for future event system redesign
//typedef struct Damage {
//    DamageMethod hit;
//    DamageType   dmg;
//    int          baseDamage;
//    bool         isEnemy;
//    bool         active;
//    Vector2      pos;
//    Vector2      vel;
//    float        lifetime;
//    float        size;
//} Damage;

typedef enum ProjectileType {
    PROJ_BULLET,
    PROJ_ROCKET,
} ProjectileType;

typedef struct Projectile {
    Vector2 pos;
    Vector2 vel;
    Vector2 target;       // rocket target position
    float lifetime;
    float size;
    ProjectileType type;
    DamageType dmgType;
    int damage;
    bool active;
    bool isEnemy;
    bool knockback;
} Projectile;

#define MAX_EXPLOSIVES 8
typedef struct Explosive {
    Vector2 pos;
    float timer;
    float duration;
    bool active;
} Explosive;

// enemy -------------------------------------------------------------------- /
// we should use this to determine the enemy type
typedef enum EnemyType {
    TRI, // triangle
    RECT, // rectangle add this first
    PENTA, // pentagon this can come later (rename to pent)
    RHOM, // rhombus faster version of triangle
    HEXA, // harder version of penta
    OCTA, // stop sign
    TRAP, // trapezoid, mini boss
    CIRC, // big circle boss?
    // one more enemy?
} EnemyType;

// static data table — one row per enemy type
typedef struct EnemyDef {
    float size;
    int   hp;
    float speedMin;
    int   speedVar;
    int   contactDamage;
    int   spawnKills;
    int   spawnChance;
    int   score;
} EnemyDef;

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
typedef struct Enemy {
    Vector2 pos;
    Vector2 vel;
    float size;
    float speed;
    int hp, maxHp;
    bool active;
    float hitFlash;
    float shootTimer;
    int contactDamage;
    int score;
    EnemyType type;
} Enemy;

// other -------------------------------------------------------------------- /
typedef struct Particle {
    Vector2 pos;
    Vector2 vel;
    Color color;
    float lifetime;
    float maxLifetime;
    float size;
    bool active;
} Particle;


// state -------------------------------------------------------------------- /
// this is THE piece of data that we are operating on
// it sits inside of our pipeline
typedef struct GameState {
    // the player entity
    Player player;
    Projectile projectiles[MAX_PROJECTILES];
    // separate enemy bullets?
    Enemy enemies[MAX_ENEMIES];
    //
    Particle particles[MAX_PARTICLES];
    Explosive explosives[MAX_EXPLOSIVES];
    //
    Camera2D camera;
    int score;
    // maybe I can combine some of this into a u64? u32? bit pack
    // especially the bools
    float spawnTimer;
    float spawnInterval;
    int enemiesKilled;
    bool gameOver;
    bool paused;
} GameState;

// the static struct and the emscripten void requirement are important
// futher study needed
static GameState g;

// ========================================================================== /
// Forward Declarations
// ========================================================================== /
// when do start to consider using a header file?
// right now, we already have a default config header
void NextFrame(void);
static void InitGame(void);
static Projectile* SpawnProjectile(
    Vector2 pos, Vector2 dir,
    float speed, int damage, float lifetime, float size,
    bool isEnemy, bool knockback,
    ProjectileType type, DamageType dmgType);
static void FireShotgunBlast(Player *p, Vector2 toMouse);
static void SpawnEnemy(void);
static void SpawnParticles(Vector2 pos, Color color, int count);
static void SpawnParticle(
    Vector2 pos, Vector2 vel,
    Color color, float size, float lifetime);
static void DamageEnemy(int idx, int damage, DamageType dmgType, DamageMethod method);
static void DamagePlayer(int damage, DamageType dmgType, DamageMethod method);

// refactoring artifact
static void UpdatePlayer(float dt);
static void UpdateGame(void);

static void DrawShape2D(
    Vector2 pos, 
    float size, float rotY, float rotX, float alpha);
static void DrawWorld(void);
static void DrawHUD(void);

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

    g.camera.offset   = (Vector2){ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    g.camera.target   = p->pos;
    g.camera.zoom     = 1.0f;

    g.spawnInterval   = SPAWN_INTERVAL;
    g.spawnTimer      = SPAWN_INITIAL_DELAY;
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
        SpawnProjectile(muzzle, dir,
            SHOTGUN_BULLET_SPEED, SHOTGUN_DAMAGE,
            SHOTGUN_BULLET_LIFETIME, SHOTGUN_PROJECTILE_SIZE, false, true,
            PROJ_BULLET, DMG_BALLISTIC);
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
    if (r) r->target = worldTarget;
    // muzzle flash
    SpawnParticles(muzzle, RED, ROCKET_MUZZLE_PARTICLES);
    SpawnParticle(muzzle,
        Vector2Scale(aimDir, ROCKET_MUZZLE_SPEED), ORANGE,
        ROCKET_MUZZLE_SIZE, ROCKET_MUZZLE_LIFETIME);
    SpawnParticles(muzzle, YELLOW, HIT_PARTICLES);
    // backblast
    SpawnParticles(p->pos, GRAY, ROCKET_BACKBLAST_PARTICLES);
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
// Get the enemy facing angle (faces player)
static float EnemyAngle(Enemy *e) {
    Vector2 toPlayer = Vector2Subtract(g.player.pos, e->pos);
    return atan2f(toPlayer.y, toPlayer.x);
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
static bool EnemyHitSweep(Enemy *e, Vector2 a, Vector2 b) {
    switch (e->type) {
    case RECT:
        return LineSegOBB(a, b, e->pos,
            e->size, e->size * RECT_ASPECT_RATIO, EnemyAngle(e));
    default:
        return LineSegCircle(a, b, e->pos, e->size);
    }
}

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
        p->pos = Vector2Add(p->pos, Vector2Scale(moveDir, p->speed * dt));
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

    if (IsKeyPressed(KEY_SPACE)
        && !p->dash.active
        && p->dash.charges > 0
    ) {
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

        // Retroactive dash slash: if sword is already swinging,
        // upgrade it so press order (M2 before Space) doesn't matter
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

        if (p->dash.timer <= 0) {
            p->dash.active = false;
        }
    }

    // or is it an ability?
    // --- Gun (M1) ---
    p->gun.cooldown -= dt;
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
        // we should make the bullet, actually bullet coloured
        // like a golden brassy, u know
        SpawnParticle(muzzle,
                      Vector2Scale(bulletDir, GUN_MUZZLE_SPEED), WHITE,
                      GUN_MUZZLE_SIZE, GUN_MUZZLE_LIFETIME);
    }

    // --- Sword swing (M2) ---
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) 
        && p->sword.timer <= 0 
        && p->spin.timer <= 0
    ) {
        p->sword.timer = p->sword.duration;
        p->sword.angle = p->angle;
        p->sword.dashSlash = p->dash.active;
        for (int i = 0; i < MAX_ENEMIES; i++)
            p->sword.lastHitAngle[i] = -1000.0f;

        // no probably not using ternary? if its over 80 chars?
        // nvm its fine with multiline
        // dash slash makes the sword attack bigger
        float arc =
            p->sword.dashSlash ?
            p->sword.arc * DASH_SLASH_ARC_MULT :
            p->sword.arc;
        float radius =
            p->sword.dashSlash ?
            p->sword.radius * DASH_SLASH_RADIUS_MULT :
            p->sword.radius;
        Color slashColor = p->sword.dashSlash ? SKYBLUE : ORANGE;
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
            SpawnParticle(particlePos, particleVel, slashColor,
                SWORD_SPARK_SIZE, SWORD_SPARK_LIFETIME);
        }
    }
    
    // Sword damage — sweep line hits enemies as it passes over them
    if (p->sword.timer > 0) {
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

    // special
    // --- Spin attack (Shift) ---
    // activation of ability
    p->spin.cooldownTimer -= dt;
    if (p->spin.cooldownTimer < 0) p->spin.cooldownTimer = 0;

    if (IsKeyPressed(KEY_LEFT_SHIFT)
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
                p->hp = p->hp + (int)heal;
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

    if (IsKeyPressed(KEY_E) && p->shotgun.blastsLeft > 0) {
        FireShotgunBlast(p, toMouse);
        p->shotgun.blastsLeft--;
        if (p->shotgun.blastsLeft == 0)
            p->shotgun.cooldownTimer = SHOTGUN_COOLDOWN;
    }

    // rocket launcher — single shot, cooldown
    if (p->rocket.cooldownTimer > 0)
        p->rocket.cooldownTimer -= dt;

    if (IsKeyPressed(KEY_Q) && p->rocket.cooldownTimer <= 0) {
        SpawnRocket(p, toMouse);
        p->rocket.cooldownTimer = ROCKET_COOLDOWN;
    }

    // don't let player p leave map boundary
    if (p->pos.x < p->size) p->pos.x = p->size;
    if (p->pos.y < p->size) p->pos.y = p->size;
    if (p->pos.x > MAP_SIZE - p->size) p->pos.x = MAP_SIZE - p->size;
    if (p->pos.y > MAP_SIZE - p->size) p->pos.y = MAP_SIZE - p->size;

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

        // Chase player (HEXA strafes instead)
        Vector2 toPlayer = Vector2Subtract(p->pos, e->pos);
        float dist = Vector2Length(toPlayer);
        if (dist > 1.0f) {
            Vector2 chaseDir = Vector2Scale(toPlayer, 1.0f / dist);
            Vector2 desired;
            if (e->type == HEXA) {
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
            e->vel = Vector2Lerp(e->vel, desired, ENEMY_VEL_LERP_RATE * dt);
        }
        e->pos = Vector2Add(e->pos, Vector2Scale(e->vel, dt));

        // Clamp to map
        if (e->pos.x < 0) e->pos.x = 0;
        if (e->pos.x > MAP_SIZE) e->pos.x = MAP_SIZE;
        if (e->pos.y < 0) e->pos.y = 0;
        if (e->pos.y > MAP_SIZE) e->pos.y = MAP_SIZE;

        // RECT shooting AI
        if (e->type == RECT) {
            e->shootTimer -= dt;
            if (e->shootTimer <= 0 && dist > 1.0f) {
                e->shootTimer = RECT_SHOOT_INTERVAL;
                Vector2 shootDir = Vector2Scale(toPlayer, 1.0f / dist);
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
        if (e->type == PENTA) {
            e->shootTimer -= dt;
            if (e->shootTimer <= 0 && dist > 1.0f) {
                e->shootTimer = PENTA_SHOOT_INTERVAL;
                Vector2 shootDir = Vector2Scale(toPlayer, 1.0f / dist);
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
        if (e->type == HEXA) {
            e->shootTimer -= dt;
            if (e->shootTimer <= 0 && dist > 1.0f) {
                e->shootTimer = HEXA_SHOOT_INTERVAL;
                Vector2 shootDir = Vector2Scale(toPlayer, 1.0f / dist);
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
            DamagePlayer(e->contactDamage, DMG_BLUNT, HIT_MELEE);
            // Knockback enemy
            if (dist > 1.0f) {
                Vector2 kb = Vector2Scale(Vector2Normalize(Vector2Subtract(e->pos, p->pos)), ENEMY_CONTACT_KNOCKBACK);
                e->vel = kb;
            }
        }
    }
}

static void RocketExplode(Vector2 pos) {
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

static void UpdateProjectiles(float dt) {
    Player *p = &g.player;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *b = &g.projectiles[i];
        if (!b->active) continue;

        b->pos = Vector2Add(b->pos, Vector2Scale(b->vel, dt));
        b->lifetime -= dt;

        if (b->lifetime <= 0 || b->pos.x < 0 || b->pos.x > MAP_SIZE ||
            b->pos.y < 0 || b->pos.y > MAP_SIZE) {
            if (b->type == PROJ_ROCKET) RocketExplode(b->pos);
            b->active = false;
            continue;
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
                    if (b->type == PROJ_ROCKET) {
                        RocketExplode(b->pos);
                    } else if (b->knockback) {
                        Vector2 kb = Vector2Normalize(b->vel);
                        ej->vel = Vector2Scale(kb, SHOTGUN_KNOCKBACK);
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
    // frametime clamp
    // ? why 0.05? if less delta time greater than this we make it that time?
    // 0.05 is a 20fps frametime, this is a nice minimum
    // why, is it like a max frame time/hz for each calculation we want to do?
    // if the world get's too far ahead, don't let the physics break.
    float dt = GetFrameTime();
    if (dt > DT_MAX) dt = DT_MAX;

    WindowResize();
    
    // need to make sure that esc also pauses in native build
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) g.paused = !g.paused;

    if (g.gameOver) {
        if (IsKeyPressed(KEY_R)) InitGame();
        return;
    }

    // like a pause state? hmmm
    // early return on pause, make sure nothing in the game world is updating
    if (g.paused) return;

    // check mouse
    // check keys
    // update player
    // update movement
    UpdatePlayer(dt);
    
    // update enemy
    // update movement
    UpdateEnemies(dt);

    // update projectiles
    UpdateProjectiles(dt);
    
    UpdateParticles(dt);

    for (int i = 0; i < MAX_EXPLOSIVES; i++) {
        if (!g.explosives[i].active) continue;
        g.explosives[i].timer -= dt;
        if (g.explosives[i].timer <= 0)
            g.explosives[i].active = false;
    }

    // player camera
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

static void DrawShape2D(
    Vector2 pos, 
    float size, float rotY, float rotX, float alpha)
{
    float s = size;

    // 4 tetrahedron vertices in local 3D space (regular tetrahedron)
    float vtx[4][3] = {
        { s,  s,  s},
        { s, -s, -s},
        {-s,  s, -s},
        {-s, -s,  s},
    };

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

    // 4 triangular faces of tetrahedron
    int faces[4][3] = {
        {0, 1, 2},
        {0, 2, 3},
        {0, 3, 1},
        {1, 3, 2},
    };

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
    float size, float rotY, float rotX, float alpha)
{
    float s = size;

    // 8 cube vertices in local 3D space
    float vtx[8][3] = {
        {-s,-s,-s}, { s,-s,-s}, { s, s,-s}, {-s, s,-s},
        {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s},
    };

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

    // 6 faces: {0,3,2,1} is Front, {4,5,6,7} is Back
    int faces[6][4] = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3},
        {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2},
    };

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

    // --- Projectiles ---
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *b = &g.projectiles[i];
        if (!b->active) continue;
        Color bColor = b->isEnemy ? MAGENTA : YELLOW;
        float bSize = b->size;
        DrawCircleV(b->pos, bSize, bColor);
        Vector2 trail = Vector2Subtract(b->pos, Vector2Scale(b->vel, BULLET_TRAIL_FACTOR));
        DrawLineV(trail, b->pos, Fade(bColor, 0.5f));
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

    // --- Enemies ---
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g.enemies[i];
        if (!e->active) continue;

        Vector2 toPlayer = Vector2Subtract(p->pos, e->pos);
        float eAngle = atan2f(toPlayer.y, toPlayer.x);
        Color eColor = (e->hitFlash > 0) ? WHITE : RED;

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
                DrawShape2D(
                    ghostPos,
                    p->size,
                    rotY - (float)gi * DASH_GHOST_ROT_STEP,
                    rotX,
                    ga);
            }
        }

        DrawShape2D(p->pos, p->size, rotY, rotX, 1.0f);

        // Gun barrel
        float ca = cosf(p->angle), sa = sinf(p->angle);
        Vector2 gunTip = Vector2Add(p->pos,
            (Vector2){ ca * (p->size + GUN_TIP_OFFSET),
                       sa * (p->size + GUN_TIP_OFFSET) });
        DrawLineEx(p->pos, gunTip, GUN_BARREL_THICKNESS, DARKGRAY);
    }

    // --- Sword swing arc ---
    if (p->sword.timer > 0) {
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
        TextFormat("HP: %d/%d", p->hp, p->maxHp), 
        hpBarX + (int)(HUD_HP_TEXT_PAD_X * ui), hpBarY + (int)(HUD_HP_TEXT_PAD_Y * ui), (int)(HUD_HP_FONT * ui), WHITE);

    // Score
    DrawText(
        TextFormat("Score: %d", g.score), 
        (int)(HUD_MARGIN * ui), (int)(HUD_SCORE_Y * ui), (int)(HUD_SCORE_FONT * ui), WHITE);
    DrawText(
        TextFormat("Kills: %d", g.enemiesKilled),
        (int)(HUD_MARGIN * ui), (int)(HUD_KILLS_Y * ui), (int)(HUD_KILLS_FONT * ui), LIGHTGRAY);

    // Cooldown indicators
    // this needs to be refactored
    // different from dash, diff CDs 
    // also its a UI thing that will be redesigned eventually
    // spin??
    // spin bar?
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

    // Spin
    cdY += (int)(HUD_ROW_SPACING * ui); 
    {
        if (p->spin.cooldownTimer > 0) {
            float cdRatio = p->spin.cooldownTimer / p->spin.cooldown;
            DrawRectangle(cdX, cdY, 
                (int)(cdBarW * (1.0f - cdRatio)), cdBarH, YELLOW);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("SPIN", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, YELLOW);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("SPIN", cdLabelX, cdY, cdFontSize, YELLOW);
        }

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
    
    // Controls reminder (bottom-left)
    int ctrlFont = (int)(HUD_CTRL_FONT * ui);
    DrawText("WASD: Move | Space: Dash | M1: Shoot | M2: Sword | E: Shotgun | Q: Rocket | Shift: Spin | R: Restart | 0: Quit",
        (int)(HUD_MARGIN * ui), sh - (int)(HUD_BOTTOM_Y * ui), ctrlFont, Fade(WHITE, 0.5f));

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
        const char *restartText = "Press R to restart";
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
    UpdateGame();
    DrawGame();
}

// ========================================================================== /
// main
// ========================================================================== /
// I am confident about this.
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W, SCREEN_H, "mecha prototype");
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
}
