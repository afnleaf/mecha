#pragma once

// ========================================================================== /
// Game Config — tweak these to tune gameplay
// ========================================================================== /

// Window / Pools
#define SCREEN_W                1443
#define SCREEN_H                1000
#define MAX_BULLETS             256
#define MAX_ENEMIES             64
#define MAX_PARTICLES           256

// Player
#define PLAYER_SPEED            300.0f
#define PLAYER_SIZE             16.0f
#define PLAYER_HP               100

// Gun
#define GUN_FIRE_RATE           10.0f

// Sword
#define SWORD_DURATION          0.25f
#define SWORD_ARC               (0.75f * 3.14159265f)
#define SWORD_RADIUS            90.0f
#define SWORD_DAMAGE            25
#define SWORD_DASH_DAMAGE       40

// Dash
#define DASH_SPEED              900.0f
#define DASH_DURATION           0.22f
#define DASH_COOLDOWN           0.6f

// Spin
#define SPIN_DURATION           0.4f
#define SPIN_RADIUS             100.0f
#define SPIN_COOLDOWN           2.0f
#define SPIN_DAMAGE             35
#define SPIN_KNOCKBACK          300.0f

// Enemies
#define ENEMY_SIZE              14.0f
#define ENEMY_HP                30
#define ENEMY_SPEED_MIN         100.0f
#define ENEMY_SPEED_VAR         80
#define ENEMY_CONTACT_DAMAGE    15
#define SPAWN_INTERVAL          1.6f
#define SPAWN_RAMP              0.98f
#define SPAWN_MIN_INTERVAL      0.4f
#define SPAWN_MARGIN            400.0f

// Bullets
#define BULLET_SPEED            700.0f
#define BULLET_LIFETIME         1.5f
#define BULLET_DAMAGE           10

// Map
#define MAP_SIZE                1600.0f
#define BG_COLOR                (Color){ 20, 20, 30, 255 }
#define GRID_COLOR              (Color){ 40, 40, 55, 255 }
#define GRID_STEP               100.0f

// Player iFrames
#define IFRAME_DURATION         0.5f

