#pragma once

// ========================================================================== /
// Game Config — tweak these to tune gameplay
// ========================================================================== /

// Window / Pools
#define SCREEN_W                1544
#define SCREEN_H                1080
#define MAX_BULLETS             1024
#define MAX_ENEMIES             1024
#define MAX_PARTICLES           1024

// Map
#define MAP_SIZE                2000.0f
#define BG_COLOR                (Color){ 20, 20, 30, 255 }
#define GRID_COLOR              (Color){ 40, 40, 55, 255 }
#define GRID_STEP               100.0f

// Mecha ------------------------------------------------------------------- /
// Player
#define PLAYER_SPEED            300.0f
#define PLAYER_SIZE             9.0f
#define PLAYER_HP               100
#define IFRAME_DURATION         0.9f

// Gun
#define GUN_FIRE_RATE           12.0f
#define GUN_BULLET_SPEED        1200.0f
#define GUN_BULLET_LIFETIME     2.0f
#define GUN_BULLET_DAMAGE       10
#define GUN_PROJECTILE_SIZE     3.0f

// Sword
#define SWORD_DURATION          0.16f
#define SWORD_ARC               (0.9f * 3.14159265f)
#define SWORD_RADIUS            120.0f
#define SWORD_DAMAGE            25
#define SWORD_DASH_DAMAGE       40

// Dash
#define DASH_SPEED              1600.0f
#define DASH_DURATION           0.2f
#define DASH_COOLDOWN           1.0f
#define DASH_MAX_CHARGES        2

// Spin
#define SPIN_DURATION           0.32f
#define SPIN_RADIUS             120.0f
#define SPIN_COOLDOWN           2.0f
#define SPIN_DAMAGE             25
#define SPIN_KNOCKBACK          250.0f
#define SPIN_LIFESTEAL          0.1f

// Shotgun
#define SHOTGUN_PELLETS         10
#define SHOTGUN_SPREAD          0.72f
#define SHOTGUN_BULLET_SPEED    1400.0f
#define SHOTGUN_BULLET_LIFETIME 0.20f
#define SHOTGUN_DAMAGE          10
#define SHOTGUN_PROJECTILE_SIZE 3.0f
#define SHOTGUN_KNOCKBACK       350.0f
#define SHOTGUN_BLASTS          2
#define SHOTGUN_COOLDOWN        1.5f

// Rocket Launcher
#define ROCKET_SPEED            800.0f
#define ROCKET_LIFETIME         3.0f
#define ROCKET_DIRECT_DAMAGE    40
#define ROCKET_EXPLOSION_DAMAGE 60
#define ROCKET_EXPLOSION_RADIUS 160.0f
#define ROCKET_KNOCKBACK        250.0f
#define ROCKET_COOLDOWN         2.0f
#define ROCKET_SIZE             6.0f
#define ROCKET_PROJECTILE_SIZE  6.0f

// Enemies ------------------------------------------------------------------ /
// Spawning
#define SPAWN_INTERVAL          2.0f
#define SPAWN_RAMP              0.98f
#define SPAWN_MIN_INTERVAL      0.4f
#define SPAWN_MARGIN            400.0f

// Enemy — Triangle (chaser)
#define TRI_SIZE                14.0f
#define TRI_HP                  20
#define TRI_SPEED_MIN           100.0f
#define TRI_SPEED_VAR           80
#define TRI_CONTACT_DAMAGE      15
#define TRI_SCORE               100

// Enemy — Rectangle (ranged)
#define RECT_SIZE               22.0f
#define RECT_HP                 40
#define RECT_SPEED_MIN          55.0f
#define RECT_SPEED_VAR          30
#define RECT_CONTACT_DAMAGE     18
#define RECT_SHOOT_INTERVAL     0.9f
#define RECT_BULLET_SPEED       300.0f
#define RECT_BULLET_DAMAGE      12
#define RECT_PROJECTILE_SIZE    4.0f
#define RECT_BULLET_LIFETIME    10.0f
#define RECT_SPAWN_KILLS        5
#define RECT_SPAWN_CHANCE       20
#define RECT_ASPECT_RATIO       0.7f
#define RECT_SCORE              200

// Enemy — Rhombus (fast chaser)
#define RHOM_SIZE               15.0f
#define RHOM_HP                 40
#define RHOM_SPEED_MIN          180.0f
#define RHOM_SPEED_VAR          100
#define RHOM_CONTACT_DAMAGE     22
#define RHOM_SPAWN_KILLS        100
#define RHOM_SPAWN_CHANCE       20
#define RHOM_SCORE              300
#define RHOM_COLOR              (Color){ 160, 32, 240, 255 }

// Enemy — Pentagon (elite ranged)
#define PENTA_SIZE              28.0f
#define PENTA_HP                100
#define PENTA_SPEED_MIN         40.0f
#define PENTA_SPEED_VAR         20
#define PENTA_CONTACT_DAMAGE    24
#define PENTA_SHOOT_INTERVAL    2.5f
#define PENTA_BULLET_SPEED      350.0f
#define PENTA_BULLET_DAMAGE     8
#define PENTA_PROJECTILE_SIZE   5.0f
#define PENTA_BULLET_LIFETIME   10.0f
#define PENTA_SPAWN_KILLS       15
#define PENTA_SPAWN_CHANCE      5
#define PENTA_ROW_OFFSET        14.0f
#define PENTA_BULLET_SPACING    18.0f
#define PENTA_SCORE             600
#define PENTA_COLOR             (Color){ 57, 255, 20, 255 }

// Enemy — Hexagon (strafing shooter, fan pattern)
#define HEXA_SIZE               20.0f
#define HEXA_HP                 61
#define HEXA_SPEED_MIN          90.0f
#define HEXA_SPEED_VAR          30
#define HEXA_CONTACT_DAMAGE     18
#define HEXA_SHOOT_INTERVAL     2.0f
#define HEXA_BULLET_SPEED       300.0f
#define HEXA_BULLET_DAMAGE      7
#define HEXA_PROJECTILE_SIZE    4.0f
#define HEXA_BULLET_LIFETIME    8.0f
#define HEXA_SPAWN_KILLS        25
#define HEXA_SPAWN_CHANCE       10
#define HEXA_FAN_COUNT          5
#define HEXA_FAN_SPREAD         0.7854f
#define HEXA_ORBIT_DIST         280.0f
#define HEXA_SCORE              600
#define HEXA_COLOR              (Color){ 255, 160, 30, 255 }

// Enemy — Octagon (the stopper/chaser)
#define OCTA_SIZE               18.0f
#define OCTA_HP                 40
#define OCTA_SPEED_MIN          300.0f
#define OCTA_SPEED_VAR          100
#define OCTA_CONTACT_DAMAGE     33
#define OCTA_SPAWN_KILLS        10
#define OCTA_SPAWN_CHANCE       10
#define OCTA_SCORE              300
#define OCTA_COLOR              (Color){ 255, 0, 0, 255 }
