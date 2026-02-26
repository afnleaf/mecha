#ifndef MECHA_H
#define MECHA_H

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

#endif // MECHA_H
