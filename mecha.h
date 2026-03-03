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

// weapon / screen types ---------------------------------------------------- /
typedef enum WeaponType {
    WPN_GUN,
    WPN_LASER,
    WPN_SWORD,
    WPN_REVOLVER,
    WPN_ROCKET,
    WPN_SNIPER,
} WeaponType;

typedef enum GameScreen {
    SCREEN_SELECT,
    SCREEN_PLAYING,
} GameScreen;

// ability slots ------------------------------------------------------------- /
#define ABILITY_SLOTS     12

typedef enum AbilityID {
    ABL_NONE,
    ABL_SHOTGUN,
    ABL_SPIN,
    ABL_GRENADE,
    ABL_RAILGUN,
    ABL_BFG,
    ABL_COUNT,
} AbilityID;

typedef struct AbilitySlot {
    AbilityID ability;
    int key;            // raylib KeyboardKey
} AbilitySlot;

// player ------------------------------------------------------------------- /
// declare abilities above player
typedef struct Gun {
    float cooldown;
    float fireRate;
    float heat;              // 0.0 to 1.0 (normalized)
    float heatDecayWait;     // seconds since last shot, decay starts after threshold
    bool  overheated;        // locked out when true
    float overheatBoostTimer; // movespeed buff remaining after dash-during-overheat
    // QTE vent
    float ventCursor;        // 0..1 sweeping indicator position
    float ventZoneStart;     // 0..1 sweet spot left edge
    float ventZoneWidth;     // width of sweet spot (normalized)
    i8    ventResult;        // 0=pending, 1=hit, -1=miss
} Gun;

typedef struct Sword {
    float timer;
    float angle;
    float duration;
    float arc;
    float radius;
    bool dashSlash;
    bool lunge;
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
    // Super dash / decoy
    float superWindow;
    bool superMissed;
    bool decoyActive;
    Vector2 decoyPos;
    float decoyTimer;
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
    float cooldown;
    float cooldownTimer;
    bool inFlight;
} Rocket;

typedef struct Grenade {
    float cooldownTimer;
} Grenade;

typedef struct Revolver {
    int rounds;
    float cooldownTimer;
    float reloadTimer;
    bool fanning;
    bool reloadLocked;  // true = already attempted active reload this cycle
    int bonusRounds;    // rounds remaining with double damage (dash reload)
} Revolver;

typedef struct Minigun {
    float spinUp;       // current spin-up progress (0.0 = idle, 1.0 = max speed)
    float cooldown;     // per-shot cooldown timer
} Minigun;

typedef struct Laser {
    float   damageAccum;
    bool    active;
    Vector2 beamTip;
} Laser;

typedef struct Railgun {
    // but Railgun does not?
    float cooldownTimer;
} Railgun;

typedef struct Sniper {
    float cooldownTimer;
    bool  aiming;           // true while M2 held (ADS mode)
    bool  superShotReady;   // dash timing grants next aimed shot bonus
    bool  adsDuringDash;    // M2 was pressed (not held) during active dash
} Sniper;

typedef struct Bfg {
    float charge;       // accumulated damage, fires at BFG_CHARGE_COST
    bool  active;       // true while projectile or lightning chain is active
} Bfg;

typedef struct Beam {
    Vector2 origin;
    Vector2 tip;
    float   timer;
    float   duration;
    Color   color;
    float   width;
    bool    active;
} Beam;

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
    Grenade grenade;
    Revolver revolver;
    Minigun minigun;
    Laser   laser;
    Railgun railgun;
    Sniper  sniper;
    Bfg     bfg;
    WeaponType primary;
    AbilitySlot slots[ABILITY_SLOTS];
    Vector2 shadowPos;
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
    PROJ_GRENADE,
    PROJ_BFG,
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
    u8 bounces;
    float height;       // visual-only: simulated height above ground
    float heightVel;    // visual-only: vertical velocity for bounce arc
} Projectile;

#define MAX_EXPLOSIVES 8
typedef struct Explosive {
    Vector2 pos;
    float timer;
    float duration;
    bool active;
} Explosive;

typedef struct LightningArc {
    Vector2 from;
    Vector2 to;
    float timer;
    float duration;
    bool active;
    bool damageApplied;
    int targetIdx;
} LightningArc;

typedef struct LightningChain {
    bool active;
    float hopTimer;
    int currentWave;
    Vector2 sources[BFG_MAX_CHAIN_TARGETS];
    int sourceCount;
    Vector2 nextSources[BFG_MAX_CHAIN_TARGETS];
    int nextSourceCount;
    bool hit[MAX_ENEMIES];
    LightningArc arcs[BFG_MAX_ARCS];
    int arcCount;
    bool propagating;
} LightningChain;

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
    float slowTimer;
    float slowFactor;
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
    Beam beams[MAX_BEAMS];
    LightningChain lightning;
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
    GameScreen screen;
    int selectIndex;
} GameState;

// ========================================================================== /
// Forward Declarations
// ========================================================================== /
// we should put this in the header file
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
static void SpawnBeam(Vector2 origin, Vector2 tip,
    float duration, Color color, float width);
static void UpdateBeams(float dt);
static Vector2 FireHitscan(Vector2 origin, Vector2 dir,
    float range, int damage, DamageType dmgType, int maxPierces);
static void UpdateRailgun(Player *p, Vector2 toMouse, float dt);
static void SpawnGrenade(Player *p, Vector2 toMouse);
static void GrenadeExplode(Vector2 pos);
static void TriggerLightningChain(Vector2 origin, int firstEnemyIdx);
static void UpdateLightningChain(float dt);

// refactoring artifact
static void UpdatePlayer(float dt);
static void UpdateSelect(void);
static void DrawSelect(void);
static void UpdateGame(void);

static void DrawTetra2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha);
static void DrawCube2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha);
static void DrawOcta2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha);
static void DrawIcosa2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha);
static void DrawDodeca2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha);
static void DrawPlayerSolid(
    Vector2 pos, float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha);
static void DrawWorld(void);
static void DrawHUD(void);

#endif // MECHA_H
