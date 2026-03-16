// mecha.h
// the data structures
#ifndef MECHA_H
#define MECHA_H

// brought to you by raylib
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

// weapon / screen types ---------------------------------------------------- /
typedef enum WeaponType {
    WPN_GUN,
    WPN_LASER,
    WPN_SWORD,
    WPN_REVOLVER,
    WPN_ROCKET,
    WPN_SNIPER,
} WeaponType;

static const WeaponType SELECT_WEAPONS[] = {
    WPN_SWORD, WPN_REVOLVER, WPN_GUN, WPN_SNIPER, WPN_ROCKET
};

typedef enum GameScreen {
    SCREEN_SELECT,
    SCREEN_PLAYING,
} GameScreen;

// ability slots ------------------------------------------------------------- /
typedef enum AbilityID {
    ABL_NONE,
    ABL_SHOTGUN,
    ABL_SPIN,
    ABL_GRENADE,
    ABL_RAILGUN,
    ABL_BFG,
    ABL_SHIELD,
    ABL_TURRET,
    ABL_MINE,
    ABL_SLAM,
    ABL_PARRY,
    ABL_HEAL,
    ABL_FIRE,
    ABL_BLINK,
    ABL_COUNT,
} AbilityID;

typedef struct AbilitySlot {
    AbilityID ability;
    int key;            // raylib KeyboardKey
} AbilitySlot;

// gamepad ability bindings ------------------------------------------------- /
typedef struct PadAbilityBind {
    AbilityID ability;
    int button;         // GAMEPAD_BUTTON_*
    bool needsLB;       // true = LB must be held
} PadAbilityBind;

static const PadAbilityBind PAD_ABILITY_MAP[] = {
    // direct (no modifier)
    { ABL_SHOTGUN,  GAMEPAD_BUTTON_RIGHT_FACE_LEFT,   false }, // X
    { ABL_RAILGUN,  GAMEPAD_BUTTON_RIGHT_FACE_UP,     false }, // Y
    { ABL_GRENADE,  GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,  false }, // B
    { ABL_BLINK,    GAMEPAD_BUTTON_RIGHT_FACE_DOWN,   false }, // A
    { ABL_SLAM,     GAMEPAD_BUTTON_RIGHT_THUMB,        false }, // RS click
    // LB + button
    { ABL_SPIN,     GAMEPAD_BUTTON_RIGHT_FACE_LEFT,   true },  // LB+X
    { ABL_PARRY,    GAMEPAD_BUTTON_RIGHT_FACE_DOWN,   true },  // LB+A
    { ABL_SHIELD,   GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,  true },  // LB+B
    { ABL_BFG,      GAMEPAD_BUTTON_RIGHT_FACE_UP,     true },  // LB+Y
    { ABL_HEAL,     GAMEPAD_BUTTON_LEFT_FACE_UP,      false }, // DUp
    { ABL_FIRE,     GAMEPAD_BUTTON_LEFT_FACE_DOWN,    false }, // DDown
    { ABL_TURRET,   GAMEPAD_BUTTON_LEFT_FACE_LEFT,    false }, // DLeft
    { ABL_MINE,     GAMEPAD_BUTTON_LEFT_FACE_RIGHT,   false }, // DRight
};
#define PAD_ABILITY_COUNT (sizeof(PAD_ABILITY_MAP) / sizeof(PAD_ABILITY_MAP[0]))

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
    u8 hitBits[MAX_ENEMIES / 8];
    float lastResetAngle;
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
    float orbAngle;     // visual orbit angle, ticks continuously
} Dash;

typedef struct Spin {
    float timer;
    float duration;
    float radius;
    float cooldown;
    float cooldownTimer;
    u8 hitBits[MAX_ENEMIES / 8];
    float lastResetAngle;
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
    float slowTimer;    // lingers after releasing M2
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

typedef struct Shield {
    float hp;           // current shield health
    float maxHp;        // max shield health
    bool  active;       // true while held up
    float regenTimer;   // time since shield was lowered (regen starts after delay)
    float angle;        // facing direction (follows mouse)
} Shield;

typedef struct Flamethrower {
    float fuel;          // current fuel (0 to FLAME_FUEL_MAX)
    float regenDelay;    // countdown before regen kicks in
    float sprayTimer;    // time until next patch spawn
    bool  active;        // currently spraying
} Flamethrower;

typedef struct BlinkDagger {
    float cooldown;
    bool  damageActive;
    float damageTimer;
    Vector2 slashOrigin;
    Vector2 slashTip;
} BlinkDagger;

// deployables -------------------------------------------------------------- /
typedef enum DeployableType {
    DEPLOY_TURRET,
    DEPLOY_MINE,
    DEPLOY_HEAL,
    DEPLOY_FIRE,
} DeployableType;


typedef struct Deployable {
    Vector2 pos;
    float timer;        // lifetime remaining
    float actionTimer;  // shoot cd / damage tick / heal tick
    float radius;       // effect radius
    float hp;           // turret health (0 = not damageable)
    bool active;
    DeployableType type;
} Deployable;

typedef struct GroundSlam {
    float cooldownTimer;
    float vfxTimer;     // expanding cone visual
    float angle;        // direction of slam
} GroundSlam;

typedef struct Parry {
    float timer;        // active window remaining
    float cooldownTimer;
    bool active;
    bool succeeded;     // landed a parry this window
} Parry;

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
    float hp, maxHp;
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
    Shield  shield;
    Flamethrower flame;
    GroundSlam slam;
    Parry   parry;
    BlinkDagger blink;
    float turretCooldown;
    float mineCooldown;
    float healCooldown;
    WeaponType primary;
    WeaponType secondary;
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
    bool appliesSlow;
    u8 bounces;
    float height;       // visual-only: simulated height above ground
    float heightVel;    // visual-only: vertical velocity for bounce arc
} Projectile;

typedef enum VfxTimerType {
    VFX_EXPLOSION,
    VFX_MINE_WEB,
} VfxTimerType;

typedef struct VfxTimer {
    Vector2 pos;
    float timer;
    float duration;
    bool active;
    VfxTimerType type;
} VfxTimer;

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
    float rootTimer;    // can't move, CAN shoot
    float stunTimer;    // can't move, CAN'T shoot
    i8 aggroIdx;        // deployable index to chase (-1 = player)
    u8 attackPhase;     // boss attack cycle counter
    float chargeTimer;  // boss charge duration remaining
    Vector2 chargeDir;  // committed charge direction
    float blinkMark;    // blink dagger slash mark timer (visual)
    bool  blinkMarked;  // queued for blink damage
} Enemy;

// shoot function pointer — NULL means no shooting AI
typedef void (*EnemyShootFn)(
    Enemy *e, Vector2 toTarget, float dist, float dt);

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
    EnemyShootFn shoot;
} EnemyDef;

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



// vfx ---------------------------------------------------------------------- /
typedef struct VfxState {
    Particle particles[MAX_PARTICLES];
    Beam beams[MAX_BEAMS];
    VfxTimer timers[MAX_VFX_TIMERS];
} VfxState;

// State -------------------------------------------------------------------- /
// this is THE piece of data that we are operating on
// it sits inside of our pipeline
typedef struct GameState {
    // the player entity
    Player player;
    Projectile projectiles[MAX_PROJECTILES];
    Enemy enemies[MAX_ENEMIES];
    Deployable deployables[MAX_DEPLOYABLES];
    LightningChain lightning;
    // vfx event buffer — update writes, draw reads
    VfxState vfx;
    //
    Camera2D camera;
    int score;
    float spawnTimer;
    float spawnInterval;
    int enemiesKilled;
    int phase;      // 0=normal, 1=clearing, 2=boss fight
    int level;
    bool gameOver;
    bool paused;
    GameScreen screen;
    bool gamepadActive;     // true = last input from gamepad
    int selectIndex;
    int selectPhase;    // 0 = picking primary, 1 = picking secondary
    float selectDemoTimer;
    float selectSwordTimer;  // sword arc demo in select screen
    float selectSwordAngle;
    Vector2 selectPedestals[NUM_PRIMARY_WEAPONS];
    float transitionTimer;   // >0 = fade transition active
} GameState;

#endif // MECHA_H
