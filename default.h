#pragma once

// ========================================================================== /
// Game Config — tweak these to tune gameplay
// ========================================================================== /

// Window / Pools
#define SCREEN_W                1544
#define SCREEN_H                1080

// Entities
#define MAX_BULLETS             1024
#define MAX_ENEMIES             1024
#define MAX_PARTICLES           1024
#define MAX_ENTITIES // of a type, just seeing that this is all the same

// Map
#define MAP_SIZE                2000.0f
#define BG_COLOR                (Color){ 20, 20, 30, 255 }
#define GRID_COLOR              (Color){ 40, 40, 55, 255 }
#define GRID_STEP               100.0f
#define MAP_BORDER_THICKNESS    3.0f

// Physics ------------------------------------------------------------------ /
#define DT_MAX                  0.05f
#define CAMERA_LERP_RATE        8.0f
#define PARTICLE_DRAG           3.0f
#define ENEMY_VEL_LERP_RATE     3.0f
#define ENEMY_CONTACT_KNOCKBACK 200.0f
#define SPAWN_INITIAL_DELAY     1.0f

// Mecha ------------------------------------------------------------------- /
// Player
#define PLAYER_SPEED            300.0f
#define PLAYER_SIZE             9.0f
#define PLAYER_HP               100
#define IFRAME_DURATION         0.9f
#define PLAYER_ROT_SPEED        2.0f
#define PLAYER_ROT_TILT         0.45f
#define PLAYER_BLINK_RATE       20
#define MUZZLE_OFFSET           4.0f

// Gun
#define GUN_FIRE_RATE           12.0f
#define GUN_BULLET_SPEED        1200.0f
#define GUN_BULLET_LIFETIME     2.0f
#define GUN_BULLET_DAMAGE       10
#define GUN_PROJECTILE_SIZE     3.0f
#define GUN_SPREAD              30
#define GUN_TIP_OFFSET          12.0f
#define GUN_BARREL_THICKNESS    3.0f

// Sword
#define SWORD_DURATION          0.16f
#define SWORD_ARC               (0.9f * 3.14159265f)
#define SWORD_RADIUS            120.0f
#define SWORD_DAMAGE            25
#define SWORD_DASH_DAMAGE       40
#define DASH_SLASH_ARC_MULT     1.3f
#define DASH_SLASH_RADIUS_MULT  1.5f
#define SWORD_DRAW_SEGMENTS     12

// Dash
#define DASH_SPEED              1600.0f
#define DASH_DURATION           0.2f
#define DASH_COOLDOWN           1.0f
#define DASH_MAX_CHARGES        2
#define DASH_GHOST_COUNT        5
#define DASH_GHOST_SPACING      14.0f
#define DASH_GHOST_ROT_STEP     0.15f
#define DASH_BURST_PARTICLES    5

// Spin
#define SPIN_DURATION           0.32f
#define SPIN_RADIUS             120.0f
#define SPIN_COOLDOWN           2.0f
#define SPIN_DAMAGE             25
#define SPIN_KNOCKBACK          250.0f
#define SPIN_LIFESTEAL          0.1f
#define SPIN_DRAW_SEGMENTS      24
#define SPIN_TRAIL_LENGTH       (PI * 1.5f)
#define SPIN_INNER_RADIUS_FRAC  0.65f

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
#define TRI_WING_ANGLE          2.4f

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
#define RHOM_TIP_MULT           1.2f
#define RHOM_BACK_MULT          0.8f
#define RHOM_SIDE_MULT          0.5f
#define RHOM_OUTLINE_COLOR      (Color){ 80, 16, 120, 255 }

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
#define PENTA_BULLETS_PER_ROW   5

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
#define HEXA_OUTLINE_COLOR      (Color){ 180, 100, 10, 255 }

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
#define OCTA_OUTLINE_COLOR      (Color){ 180, 100, 10, 255 }

// Particles ---------------------------------------------------------------- /
// Default burst (SpawnParticles)
#define PARTICLE_BURST_SPEED_MIN    50
#define PARTICLE_BURST_SPEED_MAX    200
#define PARTICLE_BURST_SIZE_MIN     2
#define PARTICLE_BURST_SIZE_MAX     5
#define PARTICLE_BURST_LIFETIME     0.4f

// Hit / Death
#define HIT_FLASH_DURATION          0.1f
#define HIT_PARTICLES               3
#define DEATH_PARTICLES             8
#define CONTACT_HIT_PARTICLES       6
#define BULLET_HIT_PARTICLES        4

// VFX ---------------------------------------------------------------------- /
// Gun muzzle
#define GUN_MUZZLE_SPEED        100.0f
#define GUN_MUZZLE_SIZE         3.0f
#define GUN_MUZZLE_LIFETIME     0.08f

// Shotgun muzzle
#define SHOTGUN_MUZZLE_SPEED    150.0f
#define SHOTGUN_MUZZLE_SIZE     5.0f
#define SHOTGUN_MUZZLE_LIFETIME 0.12f

// Rocket muzzle
#define ROCKET_MUZZLE_PARTICLES 10
#define ROCKET_MUZZLE_SPEED     150.0f
#define ROCKET_MUZZLE_SIZE      5.0f
#define ROCKET_MUZZLE_LIFETIME  0.12f
#define ROCKET_BACKBLAST_PARTICLES 10

// Sword sparks
#define SWORD_SPARK_COUNT       6
#define SWORD_SPARK_RADIUS_FRAC 0.7f
#define SWORD_SPARK_SPEED       80.0f
#define SWORD_SPARK_SIZE        3.0f
#define SWORD_SPARK_LIFETIME    0.2f

// Spin burst
#define SPIN_BURST_COUNT        16
#define SPIN_BURST_INNER_FRAC   0.5f
#define SPIN_BURST_SPEED        150.0f
#define SPIN_BURST_SIZE         4.0f
#define SPIN_BURST_LIFETIME     0.3f
#define SPIN_HEAL_PARTICLE_MULT 6.0f

// Dash trail
#define DASH_TRAIL_SPEED        30.0f
#define DASH_TRAIL_SIZE         3.0f
#define DASH_TRAIL_LIFETIME     0.2f

// Explosion
#define EXPLOSION_RING_COUNT        24
#define EXPLOSION_RING_SPEED_MIN    300
#define EXPLOSION_RING_SPEED_MAX    500
#define EXPLOSION_RING_SIZE         6.0f
#define EXPLOSION_RING_LIFETIME     0.35f
#define EXPLOSION_FIRE_COUNT        12
#define EXPLOSION_FIRE_SPEED_MIN    40
#define EXPLOSION_FIRE_SPEED_MAX    120
#define EXPLOSION_FIRE_SIZE         8.0f
#define EXPLOSION_FIRE_LIFETIME     0.5f
#define EXPLOSION_SMOKE_COUNT       8
#define EXPLOSION_SMOKE_SPEED_MIN   20
#define EXPLOSION_SMOKE_SPEED_MAX   60
#define EXPLOSION_SMOKE_SIZE        5.0f
#define EXPLOSION_SMOKE_LIFETIME    0.7f
#define EXPLOSION_VFX_DURATION      0.4f

// Enemy muzzle flash
#define ENEMY_MUZZLE_SPEED      60.0f
#define ENEMY_MUZZLE_SIZE       3.0f
#define ENEMY_MUZZLE_LIFETIME   0.1f
#define PENTA_MUZZLE_SIZE       4.0f
#define PENTA_MUZZLE_LIFETIME   0.15f
#define PENTA_SIDE_SPEED        40.0f
#define HEXA_MUZZLE_SIZE        4.0f
#define HEXA_MUZZLE_LIFETIME    0.15f

// Drawing ------------------------------------------------------------------ /
#define BULLET_TRAIL_FACTOR     0.02f
#define ENEMY_HPBAR_HEIGHT      3.0f
#define ENEMY_HPBAR_YOFFSET     8
#define SHAPE_HUE_SPEED         60.0f
#define SHAPE_HUE_DEPTH_SCALE   90.0f
#define CUBE_HUE_VERTEX_STEP    45.0f

// HUD ---------------------------------------------------------------------- /
#define HUD_SCALE_REF           450.0f
// HP bar
#define HUD_HP_W                200
#define HUD_HP_H                18
#define HUD_MARGIN              10
#define HUD_HP_TEXT_PAD_X       4
#define HUD_HP_TEXT_PAD_Y       1
#define HUD_HP_FONT             14
// Score
#define HUD_SCORE_Y             34
#define HUD_SCORE_FONT          20
#define HUD_KILLS_Y             58
#define HUD_KILLS_FONT          16
// Cooldown bars
#define HUD_CD_W                28
#define HUD_CD_H                10
#define HUD_CD_Y                82
#define HUD_CD_FONT             12
#define HUD_CD_LABEL_GAP        6
#define HUD_PIP_W               12
#define HUD_PIP_GAP             4
#define HUD_ROW_SPACING         16
// FPS
#define HUD_FPS_FONT            16
#define HUD_FPS_X               80
// Crosshair
#define HUD_CROSSHAIR_SIZE      4.0f
#define HUD_CROSSHAIR_THICKNESS 1.0f
#define HUD_CROSSHAIR_GAP       2.0f
// Info text
#define HUD_CTRL_FONT           12
#define HUD_BOTTOM_Y            20
#define HUD_TITLE_FONT          12
#define HUD_TITLE_X             210
// Pause
#define HUD_PAUSE_FONT          60
#define HUD_PAUSE_Y             40
#define HUD_RESUME_FONT         20
#define HUD_RESUME_Y            30
// Game Over
#define HUD_GO_FONT             70
#define HUD_GO_Y                60
#define HUD_GO_SCORE_FONT       28
#define HUD_GO_SCORE_Y          20
#define HUD_GO_RESTART_FONT     20
#define HUD_GO_RESTART_Y        60
