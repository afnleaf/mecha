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
    uint64_t hitMask;
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
    uint64_t hitMask;
} Spin;

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
} Player;

typedef struct Bullet {
    Vector2 pos;
    Vector2 vel;
    float lifetime;
    bool active;
    bool isEnemy;
    int damage;
} Bullet;

// enemy -------------------------------------------------------------------- /
// we should use this to determine the enemy type
typedef enum EnemyType {
    TRI, // triangle
    RECT, // rectangle add this first
    PENTA, // pentagon this can come later
} EnemyType;

// have an array of enemies 
// but switch on the type enum
// might need to add more data to this struct for the rectangle
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
    Bullet bullets[MAX_BULLETS];
    // separate enemy bullets?
    Enemy enemies[MAX_ENEMIES];
    // 
    Particle particles[MAX_PARTICLES];
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
static void SpawnBullet(
    Vector2 pos, Vector2 dir, 
    float speed, int damage, float lifetime, bool isEnemy);
static void SpawnEnemy(void);
static void SpawnParticles(Vector2 pos, Color color, int count);
static void SpawnParticle(
    Vector2 pos, Vector2 vel, 
    Color color, float size, float lifetime);
static void DamageEnemy(int idx, int damage);

// refactoring artifact
static void UpdateGamePlayer(float dt);
static void UpdateGame(void);

static void DrawCube2D(
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

    p->gun.fireRate   = GUN_FIRE_RATE;
    p->sword.duration = SWORD_DURATION;
    p->sword.arc      = SWORD_ARC;
    p->sword.radius   = SWORD_RADIUS;
    p->dash.speed       = DASH_SPEED;
    p->dash.duration    = DASH_DURATION;
    p->dash.rechargeTime = DASH_COOLDOWN;
    p->dash.charges     = DASH_MAX_CHARGES;
    p->dash.maxCharges  = DASH_MAX_CHARGES;
    p->spin.duration  = SPIN_DURATION;
    p->spin.radius    = SPIN_RADIUS;
    p->spin.cooldown  = SPIN_COOLDOWN;

    g.camera.offset   = (Vector2){ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    g.camera.target   = p->pos;
    g.camera.zoom     = 1.0f;

    g.spawnInterval   = SPAWN_INTERVAL;
    g.spawnTimer      = 1.0f;
}

// ========================================================================== /
// Spawn Helpers
// ========================================================================== /
static void SpawnBullet(
    Vector2 pos, Vector2 dir, 
    float speed, int damage, float lifetime, bool isEnemy)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g.bullets[i].active) {
            g.bullets[i].active = true;
            g.bullets[i].pos = pos;
            g.bullets[i].vel = Vector2Scale(dir, speed);
            g.bullets[i].lifetime = lifetime;
            g.bullets[i].damage = damage;
            g.bullets[i].isEnemy = isEnemy;
            return;
        }
    }
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

            // Roll for RECT type after enough kills
            bool spawnRect = (g.enemiesKilled >= RECT_SPAWN_KILLS)
                && (GetRandomValue(1, 100) <= RECT_SPAWN_CHANCE);
            // spawn pentagon
            // spawn boss after a while?

            if (spawnRect) {
                e->type = RECT;
                e->size = RECT_SIZE;
                e->speed = RECT_SPEED_MIN
                    + (float)GetRandomValue(0, RECT_SPEED_VAR);
                e->hp = RECT_HP;
                e->maxHp = RECT_HP;
                e->contactDamage = RECT_CONTACT_DAMAGE;
            } else {
                e->type = TRI;
                e->size = TRI_SIZE;
                e->speed = TRI_SPEED_MIN
                    + (float)GetRandomValue(0, TRI_SPEED_VAR);
                e->hp = TRI_HP;
                e->maxHp = TRI_HP;
                e->contactDamage = TRI_CONTACT_DAMAGE;
            }

            // Spawn on random map edge, biased toward player vicinity
            int edge = GetRandomValue(0, 3);
            float margin = SPAWN_MARGIN;
            switch (edge) {
                case 0: // top
                    e->pos.x = g.player.pos.x 
                        + (float)GetRandomValue(-500, 500);
                    e->pos.y = g.player.pos.y - margin;
                    break;
                case 1: // bottom
                    e->pos.x = g.player.pos.x 
                        + (float)GetRandomValue(-500, 500);
                    e->pos.y = g.player.pos.y + margin;
                    break;
                case 2: // left
                    e->pos.x = g.player.pos.x - margin;
                    e->pos.y = g.player.pos.y 
                        + (float)GetRandomValue(-500, 500);
                    break;
                case 3: // right
                    e->pos.x = g.player.pos.x + margin;
                    e->pos.y = g.player.pos.y 
                        + (float)GetRandomValue(-500, 500);
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
        float speed = (float)GetRandomValue(50, 200);
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        float size = (float)GetRandomValue(2, 5);
        SpawnParticle(pos, vel, color, size, 0.4f);
    }
}

// to scale enemy types, each enemy has an enum type which we switch on?
// then calculate damage?
// assuming more complex damage calculation is down the line
// like in a real game, types of damage exist, armor and resist of enemy, etc
// diff weapons, diff damage! damage comes in as a raw number though?
// also needs its type? and context?
// getting ahead of ourselves here
static void DamageEnemy(int idx, int damage)
{
    Enemy *e = &g.enemies[idx];
    e->hp -= damage;
    e->hitFlash = 0.1f;
    SpawnParticles(e->pos, WHITE, 3);

    if (e->hp <= 0) {
        e->active = false;
        g.score += 100;
        g.enemiesKilled++;
        // fire/explosion
        SpawnParticles(e->pos, RED, 8);
        SpawnParticles(e->pos, ORANGE, 8);
        SpawnParticles(e->pos, YELLOW, 8);
    }
}

// ========================================================================== /
// Check if point is inside a swing arc
// ========================================================================== /
// what is C convetion for multi line function signature?
static bool PointInArc(
    Vector2 center, Vector2 point, 
    float startAngle, float arcWidth, float radius)
{
    Vector2 diff = Vector2Subtract(point, center);
    float dist = Vector2Length(diff);
    if (dist > radius) return false;

    float angle = atan2f(diff.y, diff.x);
    // Normalize angle difference to [-PI, PI]
    float da = angle - startAngle;
    while (da > PI) da -= 2.0f * PI;
    while (da < -PI) da += 2.0f * PI;

    return (da >= -arcWidth / 2.0f && da <= arcWidth / 2.0f);
}

// ========================================================================== /
// Update
// ========================================================================== /
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

// should we refactor this further?
// or do we tackle that once we start adding different loadoats?
// diff base mechas
// adding abilities, etc
static void UpdateGamePlayer(float dt)
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
        SpawnParticles(p->pos, SKYBLUE, 5);
    }

    if (p->dash.active) {
        p->dash.timer -= dt;
        p->pos = Vector2Add(
            p->pos, Vector2Scale(p->dash.dir, p->dash.speed * dt));

        float trailAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
        Vector2 trailVel = 
            { cosf(trailAngle) * 30.0f, sinf(trailAngle) * 30.0f };
        SpawnParticle(p->pos, trailVel, SKYBLUE, 3.0f, 0.2f);

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
        float spread = ((float)GetRandomValue(-30, 30)) * 0.001f;
        float bulletAngle = p->angle + spread;
        Vector2 bulletDir = { cosf(bulletAngle), sinf(bulletAngle) };
        Vector2 muzzle = 
            Vector2Add(p->pos, Vector2Scale(aimDir, p->size + 4.0f));
        SpawnBullet(muzzle, bulletDir, 
            GUN_BULLET_SPEED, GUN_BULLET_DAMAGE, GUN_BULLET_LIFETIME, false);
        // we should make the bullet, actually bullet coloured
        // like a golden brassy, u know
        SpawnParticle(muzzle, 
                      Vector2Scale(bulletDir, 100.0f), YELLOW, 3.0f, 0.08f);
    }

    // i think i see the pattern here
    // --- Sword swing (M2) ---
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) 
        && p->sword.timer <= 0 
        && p->spin.timer <= 0
    ) {
        p->sword.timer = p->sword.duration;
        p->sword.angle = p->angle;
        p->sword.dashSlash = p->dash.active;
        p->sword.hitMask = 0;

        // no probably not using ternary? if its over 80 chars?
        // nvm its fine with multiline
        float arc = 
            p->sword.dashSlash ? 
            p->sword.arc * 1.3f : 
            p->sword.arc;
        float radius = 
            p->sword.dashSlash ? 
            p->sword.radius * 1.5f : 
            p->sword.radius;
        // hmm when does this trigger?
        Color slashColor = 
            p->sword.dashSlash ? 
            SKYBLUE : 
            ORANGE;
        for (int i = 0; i < 6; i++) {
            float a = p->sword.angle - arc / 2.0f + arc * (float)i / 5.0f;
            Vector2 particlePos = Vector2Add(
                p->pos, 
                (Vector2){ cosf(a) * radius * 0.7f, sinf(a) * radius * 0.7f }
            );
            Vector2 particleVel = { cosf(a) * 80.0f, sinf(a) * 80.0f };
            SpawnParticle(particlePos, particleVel, slashColor, 3.0f, 0.2f);
        }
    }
    
    // this belongs to sword
    // Lingering sword damage — check every frame while swinging
    if (p->sword.timer > 0) {
        float radius = p->sword.dashSlash ? p->sword.radius * 1.5f : p->sword.radius;
        float arc = p->sword.dashSlash ? p->sword.arc * 1.3f : p->sword.arc;
        int dmg = p->sword.dashSlash ? SWORD_DASH_DAMAGE : SWORD_DAMAGE;

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g.enemies[i].active) continue;
            if (p->sword.hitMask & ((uint64_t)1 << i)) continue;
            if (PointInArc(p->pos, g.enemies[i].pos, p->sword.angle, arc, radius)) {
                DamageEnemy(i, dmg);
                p->sword.hitMask |= ((uint64_t)1 << i);
            }
        }
        p->sword.timer -= dt;
    }

    // special
    // --- Spin attack (Shift) ---
    p->spin.cooldownTimer -= dt;
    if (p->spin.cooldownTimer < 0) p->spin.cooldownTimer = 0;

    if (IsKeyPressed(KEY_LEFT_SHIFT) && p->spin.timer <= 0 && p->spin.cooldownTimer <= 0) {
        p->spin.timer = p->spin.duration;
        p->spin.cooldownTimer = p->spin.cooldown;
        p->spin.hitMask = 0;

        for (int i = 0; i < 16; i++) {
            float a = (float)i / 16.0f * 2.0f * PI;
            Vector2 particlePos = Vector2Add(p->pos, (Vector2){ cosf(a) * p->spin.radius * 0.5f, sinf(a) * p->spin.radius * 0.5f });
            Vector2 particleVel = { cosf(a) * 150.0f, sinf(a) * 150.0f };
            SpawnParticle(particlePos, particleVel, YELLOW, 4.0f, 0.3f);
        }
    }
    
    // stays with special? can it be refactored with sword linger?
    // Lingering spin damage — check every frame while spinning
    if (p->spin.timer > 0) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g.enemies[i].active) continue;
            if (p->spin.hitMask & ((uint64_t)1 << i)) continue;
            float dist = Vector2Distance(p->pos, g.enemies[i].pos);
            if (dist <= p->spin.radius) {
                DamageEnemy(i, SPIN_DAMAGE);
                p->spin.hitMask |= ((uint64_t)1 << i);
                Vector2 kb = Vector2Normalize(Vector2Subtract(g.enemies[i].pos, p->pos));
                g.enemies[i].vel = Vector2Scale(kb, SPIN_KNOCKBACK);
            }
        }
        p->spin.timer -= dt;
    }

    // don't let player p leave map boundary
    if (p->pos.x < p->size) p->pos.x = p->size;
    if (p->pos.y < p->size) p->pos.y = p->size;
    if (p->pos.x > MAP_SIZE - p->size) p->pos.x = MAP_SIZE - p->size;
    if (p->pos.y > MAP_SIZE - p->size) p->pos.y = MAP_SIZE - p->size;

    // ok this is an important piece of gamefeel we need to get right
    // currently, the iframes don't feel right?
    // how is it supposed to work? I take damage dashing through enemies.
    // --- iFrames ---
    if (p->iFrames > 0) p->iFrames -= dt;

    // does this belong to gun or to an all bullets thing?
    // --- Bullets ---
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &g.bullets[i];
        if (!b->active) continue;

        b->pos = Vector2Add(b->pos, Vector2Scale(b->vel, dt));
        b->lifetime -= dt;

        if (b->lifetime <= 0 || b->pos.x < 0 || b->pos.x > MAP_SIZE ||
            b->pos.y < 0 || b->pos.y > MAP_SIZE) {
            b->active = false;
            continue;
        }

        if (b->isEnemy) {
            // Enemy bullet — hit player
            float dist = Vector2Distance(b->pos, p->pos);
            if (dist < p->size + 3.0f && p->iFrames <= 0) {
                p->hp -= b->damage;
                p->iFrames = IFRAME_DURATION;
                SpawnParticles(p->pos, RED, 4);
                b->active = false;
                if (p->hp <= 0) {
                    p->hp = 0;
                    g.gameOver = true;
                }
            }
        } else {
            // Player bullet — hit enemies
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!g.enemies[j].active) continue;
                float dist = Vector2Distance(b->pos, g.enemies[j].pos);
                if (dist < g.enemies[j].size + 3.0f) {
                    DamageEnemy(j, b->damage);
                    b->active = false;
                    break;
                }
            }
        }
    }



}
// ok so game state. giant function. let's separate this and make it more modular so its not so hard to add features. or is it commonplace to have massive functions like this in the game dev world?


static void enemies(float dt) {
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

        // Chase player
        Vector2 toPlayer = Vector2Subtract(p->pos, e->pos);
        float dist = Vector2Length(toPlayer);
        if (dist > 1.0f) {
            Vector2 chaseDir = Vector2Scale(toPlayer, 1.0f / dist);
            // Blend velocity for smoother movement
            Vector2 desired = Vector2Scale(chaseDir, e->speed);
            e->vel = Vector2Lerp(e->vel, desired, 3.0f * dt);
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
                    Vector2Scale(shootDir, e->size + 4.0f));
                SpawnBullet(muzzle, shootDir, RECT_BULLET_SPEED,
                    RECT_BULLET_DAMAGE, RECT_BULLET_LIFETIME, true);
                SpawnParticle(muzzle, Vector2Scale(shootDir, 60.0f),
                    MAGENTA, 3.0f, 0.1f);
            }
        }

        // Hit flash decay
        if (e->hitFlash > 0) e->hitFlash -= dt;

        // Collide with player
        if (dist < p->size + e->size && p->iFrames <= 0) {
            p->hp -= e->contactDamage;
            p->iFrames = IFRAME_DURATION;
            SpawnParticles(p->pos, RED, 6);
            // Knockback enemy
            if (dist > 1.0f) {
                Vector2 kb = Vector2Scale(Vector2Normalize(Vector2Subtract(e->pos, p->pos)), 200.0f);
                e->vel = kb;
            }
            if (p->hp <= 0) {
                p->hp = 0;
                g.gameOver = true;
            }
        }
    }
}

static void particles(float dt) 
{
    // --- Particles ---
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &g.particles[i];
        if (!pt->active) continue;
        pt->pos = Vector2Add(pt->pos, Vector2Scale(pt->vel, dt));
        pt->vel = Vector2Scale(pt->vel, 1.0f - 3.0f * dt); // drag
        pt->lifetime -= dt;
        if (pt->lifetime <= 0) pt->active = false;
    }

}

// the 8.0f could be inside default
// what is it doing exactly?
// how much can we fiddle with this value?
static void camera(float dt) 
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
    g.camera.target = Vector2Lerp(g.camera.target, p->pos, 8.0f * dt);

    // update camera offset to current browser window size
#ifdef PLATFORM_WEB
    int sw = EM_ASM_INT({ return window.innerWidth; });
    int sh = EM_ASM_INT({ return window.innerHeight; });
    if (sw != GetScreenWidth() || sh != GetScreenHeight()) {
        SetWindowSize(sw, sh);
    }
#endif
    // native is static size for now
    // could we add dynamic resize? fullscreen? later
    g.camera.offset = 
        (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
}

static void UpdateGame(void) 
{
    // frametime clamp
    // ? why 0.05? if less delta time greater than this we make it that time?
    // 0.05 is a 20fps frametime, this is a nice minimum
    // why, is it like a max frame time/hz for each calculation we want to do?
    // if the world get's too far ahead, don't let the physics break.
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    
    // need to make sure that esc also pauses in native build
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) g.paused = !g.paused;

    if (g.gameOver) {
        if (IsKeyPressed(KEY_R)) InitGame();
        return;
    }

    // like a pause state? hmmm
    // early return on pause, make sure nothing in the game world is updating
    if (g.paused) return;

    // check keys
    // check mouse
    // update player
    // update movement
    // update bullets
    UpdateGamePlayer(dt);
    
    // update enemy
    // update movement
    // update bullets
    enemies(dt);
    
    particles(dt);
   
    // player camera
    camera(dt);
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
        (unsigned char)((r + m) * 255.0f),
        (unsigned char)((g + m) * 255.0f),
        (unsigned char)((b + m) * 255.0f),
        (unsigned char)(alpha * 255.0f)
    };
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
        hues[i] = fmodf(time * 60.0f + (float)i * 45.0f, 360.0f);
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
    DrawRectangleLinesEx((Rectangle){ 0, 0, MAP_SIZE, MAP_SIZE }, 3.0f, RED);

    // --- Particles (behind entities) ---
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &g.particles[i];
        if (!pt->active) continue;
        float alpha = pt->lifetime / pt->maxLifetime;
        Color c = Fade(pt->color, alpha);
        DrawCircleV(pt->pos, pt->size * alpha, c);
    }

    // --- Bullets ---
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &g.bullets[i];
        if (!b->active) continue;
        Color bColor = b->isEnemy ? MAGENTA : YELLOW;
        float bSize = b->isEnemy ? 4.0f : 3.0f;
        DrawCircleV(b->pos, bSize, bColor);
        Vector2 trail = Vector2Subtract(b->pos, Vector2Scale(b->vel, 0.02f));
        DrawLineV(trail, b->pos, Fade(bColor, 0.5f));
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
                        cosf(eAngle + 2.4f) * e->size, 
                        sinf(eAngle + 2.4f) * e->size 
                    }
                );
            Vector2 right = Vector2Add(
                    e->pos, 
                    (Vector2){ 
                        cosf(eAngle - 2.4f) * e->size, 
                        sinf(eAngle - 2.4f) * e->size 
                    }
                );
            DrawTriangle(tip, right, left, eColor);
            DrawTriangleLines(tip, right, left, MAROON);
        } break;
        case RECT: {
            Color rFill = (e->hitFlash > 0) ? WHITE : MAGENTA;
            float hw = e->size, hh = e->size * 0.7f;
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
        default: break;
        }

        // HP bar
        if (e->hp < e->maxHp) {
            float barW = e->size * 2.0f;
            float barH = 3.0f;
            float hpRatio = (float)e->hp / e->maxHp;
            DrawRectangle(
                (int)(e->pos.x - barW / 2), 
                (int)(e->pos.y - e->size - 8), 
                (int)barW, 
                (int)barH,
                DARKGRAY);
            DrawRectangle(
                (int)(e->pos.x - barW / 2), 
                (int)(e->pos.y - e->size - 8), 
                (int)(barW * hpRatio), 
                (int)barH, 
                RED);
        }
    }

    // --- Player — RGB color cube ---
    bool visible = true;
    if (p->iFrames > 0) {
        visible = ((int)(p->iFrames * 20) % 2 == 0);
    }

    if (visible) {
        float t = (float)GetTime();
        float rotY = t * 2.0f;
        float rotX = 0.45f;

        // Ghost trail during dash
        if (p->dash.active) {
            for (int gi = 5; gi >= 1; gi--) {
                float offset = (float)gi * 14.0f;
                Vector2 ghostPos = Vector2Subtract(p->pos,
                    Vector2Scale(p->dash.dir, offset));
                float ga = 0.5f * (1.0f - (float)gi / 6.0f);
                DrawCube2D(
                    ghostPos, 
                    p->size, 
                    rotY - (float)gi * 0.15f, 
                    rotX, 
                    ga);
            }
        }

        DrawCube2D(p->pos, p->size, rotY, rotX, 1.0f);

        // Gun barrel
        float ca = cosf(p->angle), sa = sinf(p->angle);
        Vector2 gunTip = Vector2Add(p->pos,
            (Vector2){ ca * (p->size + 12.0f), sa * (p->size + 12.0f) });
        DrawLineEx(p->pos, gunTip, 3.0f, DARKGRAY);
    }

    // --- Sword swing arc ---
    if (p->sword.timer > 0) {
        float progress = 1.0f - (p->sword.timer / p->sword.duration);
        float sweepAngle = 
            p->sword.angle - p->sword.arc / 2.0f + p->sword.arc * progress;
        int numSegments = 12;
        float arcHalf = p->sword.arc / 2.0f;
        for (int i = 0; i <= numSegments; i++) {
            float a = 
                p->sword.angle - arcHalf 
                + p->sword.arc * (float)i / numSegments;
            Vector2 pt = Vector2Add(
                p->pos, 
                (Vector2){ 
                    cosf(a) * p->sword.radius, 
                    sinf(a) * p->sword.radius 
                });
            if (i > 0) {
                float aPrev = 
                    p->sword.angle - arcHalf 
                    + p->sword.arc * (float)(i - 1) / numSegments;
                Vector2 ptPrev = Vector2Add(
                    p->pos, 
                    (Vector2){ 
                        cosf(aPrev) * p->sword.radius, 
                        sinf(aPrev) * p->sword.radius 
                    });
                DrawLineEx(ptPrev, pt, 2.0f, Fade(ORANGE, 0.7f));
            }
        }
        Vector2 sweepEnd = Vector2Add(
            p->pos, 
            (Vector2){ 
                cosf(sweepAngle) * p->sword.radius, 
                sinf(sweepAngle) * p->sword.radius 
            });
        DrawLineEx(p->pos, sweepEnd, 3.0f, WHITE);
    }

    // --- Spin attack circle ---
    if (p->spin.timer > 0) {
        float alpha = p->spin.timer / p->spin.duration;
        DrawCircleLinesV(
            p->pos, 
            p->spin.radius, 
            Fade(YELLOW, alpha));
        DrawCircleLinesV(
            p->pos, 
            p->spin.radius * 0.7f, 
            Fade(ORANGE, alpha * 0.5f));
        float spinAngle = (1.0f - alpha) * PI * 4.0f;
        for (int i = 0; i < 4; i++) {
            float a = spinAngle + (float)i * PI / 2.0f;
            Vector2 end = Vector2Add(
                p->pos, 
                (Vector2){ 
                    cosf(a) * p->spin.radius, 
                    sinf(a) * p->spin.radius 
                });
            DrawLineEx(p->pos, end, 2.0f, Fade(YELLOW, alpha * 0.6f));
        }
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
    float ui = (float)sh / 450.0f;

    // HP bar
    int hpBarW = (int)(200 * ui);
    int hpBarH = (int)(18 * ui);
    int hpBarX = (int)(10 * ui);
    int hpBarY = (int)(10 * ui);
    float hpRatio = (float)p->hp / p->maxHp;
    DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, DARKGRAY);
    Color hpColor = (hpRatio > 0.5f) ? GREEN : (hpRatio > 0.25f) ? ORANGE : RED;
    DrawRectangle(hpBarX, hpBarY, (int)(hpBarW * hpRatio), hpBarH, hpColor);
    DrawRectangleLines(hpBarX, hpBarY, hpBarW, hpBarH, WHITE);
    DrawText(
        TextFormat("HP: %d/%d", p->hp, p->maxHp), 
        hpBarX + (int)(4 * ui), hpBarY + (int)(1 * ui), (int)(14 * ui), WHITE);

    // Score
    DrawText(
        TextFormat("Score: %d", g.score), 
        (int)(10 * ui), (int)(34 * ui), (int)(20 * ui), WHITE);
    DrawText(
        TextFormat("Kills: %d", g.enemiesKilled), 
        (int)(10 * ui), (int)(58 * ui), (int)(16 * ui), LIGHTGRAY);

    // Cooldown indicators
    // this needs to be refactored
    // different from dash, diff CDs 
    // also its a UI thing that will be redesigned eventually
    // spin??
    // spin bar?
    int cdBarW = (int)(28 * ui);
    int cdBarH = (int)(10 * ui);
    int cdX = (int)(10 * ui);
    int cdY = (int)(82 * ui);
    int cdFontSize = (int)(12 * ui);
    int cdLabelX = cdX + cdBarW + (int)(6 * ui);
    
    // Dash charges
    {
        int pipW = (int)(12 * ui);
        int pipGap = (int)(4 * ui);
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

    cdY += (int)(16 * ui);
    // Spin
    if (p->spin.cooldownTimer > 0) {
        float cdRatio = p->spin.cooldownTimer / p->spin.cooldown;
        DrawRectangle(cdX, cdY, (int)(cdBarW * (1.0f - cdRatio)), cdBarH, YELLOW);
        DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
        DrawText("SPIN", cdLabelX, cdY, cdFontSize, GRAY);
    } else {
        DrawRectangle(cdX, cdY, cdBarW, cdBarH, YELLOW);
        DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
        DrawText("SPIN", cdLabelX, cdY, cdFontSize, YELLOW);
    }

    // Controls reminder (bottom-left)
    int ctrlFont = (int)(14 * ui);
    DrawText("WASD: Move  |  M1: Shoot  |  M2: Sword  |  Space: Dash  |  Shift: Spin  |  R: Restart",
             (int)(10 * ui), sh - (int)(24 * ui), ctrlFont, Fade(WHITE, 0.5f));

    // FPS (top-right)
    int fpsFont = (int)(16 * ui);
    DrawText(TextFormat("%d FPS", GetFPS()), sw - (int)(80 * ui), (int)(10 * ui), fpsFont, GREEN);

    // Crosshair cursor
    Vector2 mouse = GetMousePosition();
    // we can refactor this crosshair to default
    // eventually make it editable
    float ch = 4.0f * ui;
    float chThick = 1.0f * ui;
    float chGap = 2.0f * ui;
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

    // Pause overlay
    if (g.paused && !g.gameOver) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.5f));
        int pauseFont = (int)(60 * ui);
        const char *pauseText = "PAUSED";
        int pW = MeasureText(pauseText, pauseFont);
        DrawText(
            pauseText, 
            sw / 2 - pW / 2,
            sh / 2 - (int)(40 * ui), 
            pauseFont, 
            WHITE);
        int resumeFont = (int)(20 * ui);
        const char *resumeText = "P or Esc to resume";
        int rW = MeasureText(resumeText, resumeFont);
        DrawText(
            resumeText, 
            sw / 2 - rW / 2, 
            sh / 2 + (int)(30 * ui), 
            resumeFont, 
            LIGHTGRAY);
    }

    // Game Over overlay
    if (g.gameOver) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.7f));
        int goFont = (int)(70 * ui);
        const char *goText = "GAME OVER";
        int goW = MeasureText(goText, goFont);
        DrawText(
            goText, 
            sw / 2 - goW / 2, 
            sh / 2 - (int)(60 * ui), 
            goFont, 
            RED);

        int scFont = (int)(28 * ui);
        const char *scoreText = 
            TextFormat("Score: %d  |  Kills: %d", g.score, g.enemiesKilled);
        int sW = MeasureText(scoreText, scFont);
        DrawText(
            scoreText, 
            sw / 2 - sW / 2, 
            sh / 2 + (int)(20 * ui), 
            scFont, 
            WHITE);

        int rsFont = (int)(20 * ui);
        const char *restartText = "Press R to restart";
        int rW = MeasureText(restartText, rsFont);
        DrawText(
            restartText, 
            sw / 2 - rW / 2, 
            sh / 2 + (int)(60 * ui), 
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
    InitWindow(SCREEN_W, SCREEN_H, "mecha prototype");
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
