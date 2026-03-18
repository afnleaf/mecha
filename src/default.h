// default.h
// game configuration constants, tweak these to tune gameplay
#pragma once

// Window
#define SCREEN_W                1443
#define SCREEN_H                1000

// Keyboard-only controls (right hand)
#define AIM_UP_KEY              KEY_O
#define AIM_DOWN_KEY            KEY_L
#define AIM_LEFT_KEY            KEY_K
#define AIM_RIGHT_KEY           KEY_SEMICOLON
#define KB_M1_KEY               KEY_RIGHT_SHIFT
#define KB_M2_KEY               KEY_ENTER
#define ARROW_AIM_DIST          150.0f
#define AIM_KEY_GRACE           0.08f

// Gamepad
#define GAMEPAD_INDEX           0
#define GAMEPAD_STICK_DEADZONE  0.20f
#define GAMEPAD_AIM_DIST        150.0f
#define GAMEPAD_AIM_SENS        0.4f    // stick input scalar 0 low 1 high
#define GAMEPAD_TRIGGER_THRESH  0.5f

// Kit
#define ABILITY_SLOTS           13
#define NUM_PRIMARY_WEAPONS     5

// Entity Pools
#define MAX_PROJECTILES         1024
#define MAX_ENEMIES             1024
#define MAX_PARTICLES           1024
#define MAX_BEAMS               8
#define MAX_DEPLOYABLES         1024
#define MAX_VFX_TIMERS          72      // this could be more?

// Map
#define MAP_SIZE                2000.0f
#define BG_COLOR                (Color){ 20, 20, 30, 255 }
//#define BG_COLOR                (Color){ 128, 128, 128, 255 }
#define GRID_COLOR              (Color){ 40, 40, 55, 255 }
#define GRID_STEP               100.0f
#define MAP_BORDER_THICKNESS    3.0f

// Combat zone offset (indented left wall to make room for base)
#define MAP_LEFT                300.0f
#define MAP_RIGHT               (MAP_LEFT + MAP_SIZE)
// Base (separate room hanging off bottom-left of combat zone)
#define STEP_Y                  (MAP_SIZE - 300.0f)
#define BASE_W                  600.0f
#define BASE_H                  325.0f
#define BASE_TOP                MAP_SIZE
#define BASE_BOTTOM             (BASE_TOP + BASE_H)
#define BASE_CENTER_X           (BASE_W / 2.0f)
#define BASE_CENTER_Y           (BASE_TOP + BASE_H / 2.0f)
#define BASE_WALL_THICKNESS     4.0f
#define BASE_WALL_COLOR         (Color){ 80, 80, 120, 255 }
#define BASE_LABEL_FONT         60
#define BASE_LABEL_COLOR        (Color){ 50, 50, 70, 255 }
#define COMBAT_LABEL_FONT       80
#define COMBAT_LABEL_COLOR      (Color){ 50, 50, 70, 255 }

// Physics ------------------------------------------------------------------ /
#define DT_MAX                  0.05f
#define CAMERA_LERP_RATE        8.0f
#define PARTICLE_DRAG           3.0f
#define ENEMY_VEL_LERP_RATE     3.0f
#define ENEMY_CONTACT_KNOCKBACK 200.0f
#define SPAWN_INITIAL_DELAY     1.0f

// Weapon Swap
#define WEAPON_SWAP_KEY         KEY_LEFT_CONTROL

// Mecha -------------------------------------------------------------------- /
// Player
#define PLAYER_SPEED            300.0f
#define PLAYER_SIZE             16.0f
#define PLAYER_HP               100
#define IFRAME_DURATION         0.9f
#define PLAYER_ROT_SPEED        1.0f
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

// Overheat (shared M1/M2 heat resource)
#define GUN_HEAT_PER_SHOT        0.02f    // M1
#define MINIGUN_HEAT_PER_SHOT    0.004f   // M2
#define GUN_HEAT_DECAY           0.3f     // passive heat loss per second
#define GUN_HEAT_DECAY_DELAY     0.4f     // seconds after last shot before heat decays
#define GUN_OVERHEAT_DECAY       0.5f     // heat loss/sec while overheated (normal/miss)
#define GUN_OVERHEAT_THRESHOLD   1.0f     // heat level that triggers overheat
#define GUN_OVERHEAT_CLEAR       0.0f     // heat must reach this to unlock
// QTE vent
#define GUN_VENT_CURSOR_SPEED    1.0f     // cursor sweeps 0->1 in 1/speed seconds
#define GUN_VENT_ZONE_WIDTH      0.12f    // sweet spot width (fraction of bar)
#define GUN_VENT_ZONE_MIN        0.6f     // earliest sweet spot left edge
#define GUN_VENT_ZONE_MAX        0.78f    // latest sweet spot left edge (max + width <= 1)
#define GUN_VENT_HIT_DECAY       5.0f     // heat loss/sec on perfect vent
#define GUN_OVERHEAT_DASH_BOOST  1.4f     // movespeed multiplier after dash during overheat
#define GUN_OVERHEAT_BOOST_DUR   3.0f     // seconds the dash-vent movespeed buff lasts
#define GUN_OVERHEAT_TRAIL_SPEED 20.0f
#define GUN_OVERHEAT_TRAIL_COLOR (Color){ 100, 200, 255, 150 }
#define GUN_OVERHEAT_TRAIL_SIZE  2.0f
#define GUN_OVERHEAT_TRAIL_LIFE  0.2f
#define GUN_VENT_BURST_PARTICLES 12
#define GUN_VENT_BURST_SPEED     80.0f
#define GUN_VENT_BURST_COLOR     (Color){ 100, 200, 255, 200 }
#define GUN_VENT_BURST_SIZE      3.0f
#define GUN_VENT_BURST_LIFETIME  0.4f
#define GUN_VENT_STEAM_SPEED     40.0f
#define GUN_VENT_STEAM_COLOR     (Color){ 200, 200, 200, 150 }
#define GUN_VENT_STEAM_SIZE      2.0f
#define GUN_VENT_STEAM_LIFETIME  0.3f
#define GUN_VENT_FAIL_COLOR      (Color){ 80, 20, 20, 255 }

// Minigun
#define MINIGUN_MAX_FIRE_RATE    40.0f
#define MINIGUN_MIN_FIRE_RATE    4.0f
#define MINIGUN_SPIN_UP_TIME     0.8f
#define MINIGUN_SPIN_DOWN_TIME   1.6f
#define MINIGUN_BULLET_SPEED     1000.0f
#define MINIGUN_BULLET_LIFETIME  1.5f
#define MINIGUN_BULLET_DAMAGE    6
#define MINIGUN_PROJECTILE_SIZE  2.5f
#define MINIGUN_SPREAD_MIN       20
#define MINIGUN_SPREAD_MAX       80
#define MINIGUN_SLOW_FACTOR      0.25f
#define MINIGUN_SLOW_LINGER      0.25f
#define MINIGUN_TIP_OFFSET       14.0f
#define MINIGUN_BARREL_THICKNESS 4.0f
#define MINIGUN_MUZZLE_SPEED     100.0f
#define MINIGUN_MUZZLE_SIZE      2.0f
#define MINIGUN_MUZZLE_LIFETIME  0.06f

// Sword
#define SWORD_DURATION          0.16f
#define SWORD_ARC               (0.9f * 3.14159265f)
#define SWORD_RADIUS            120.0f
#define SWORD_DAMAGE            25
#define SWORD_DASH_DAMAGE       40
#define DASH_SLASH_ARC_MULT     1.3f
#define DASH_SLASH_RADIUS_MULT  1.5f
#define SWORD_DRAW_SEGMENTS     12

// Lunge (M2 for sword)
#define LUNGE_DURATION          0.18f
#define LUNGE_RANGE             150.0f
#define LUNGE_CONE_HALF         0.30f
#define LUNGE_DAMAGE            30
#define LUNGE_DASH_DAMAGE       50
#define LUNGE_SPEED             600.0f
#define LUNGE_DASH_RANGE_MULT   1.5f
#define LUNGE_DASH_SPEED_MULT   1.6f
#define LUNGE_DRAW_SEGMENTS     8

// Dash
#define DASH_SPEED              1600.0f
#define DASH_DURATION           0.2f
#define DASH_COOLDOWN           2.0f
#define DASH_MAX_CHARGES        3
#define DASH_GHOST_COUNT        5
#define DASH_GHOST_SPACING      14.0f
#define DASH_GHOST_ROT_STEP     0.15f
#define DASH_BURST_PARTICLES    5
#define DASH_SUPER_WINDOW       0.12f
#define DECOY_DURATION          1.6f
#define DECOY_PULSE_SPEED       8.0f
#define DECOY_MIN_ALPHA         0.15f
#define DECOY_MAX_ALPHA         0.45f
#define DECOY_EXPIRE_PARTICLES  10
// Dash orbs (diegetic charge display)
#define DASH_ORB_SPEED          2.0f    // rad/s orbit speed
#define DASH_ORB_RADIUS         40.0f   // orbit distance from player center
#define DASH_ORB_SIZE           4.0f    // orb draw radius

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
#define SHOTGUN_BOUNCES         2
#define SHOTGUN_BOUNCE_SPEED    0.8f
#define SHOTGUN_BLASTS          2
#define SHOTGUN_COOLDOWN        1.5f

// Revolver
#define REVOLVER_ROUNDS         6
#define REVOLVER_DAMAGE         22
#define REVOLVER_COOLDOWN       0.35f
#define REVOLVER_FAN_COOLDOWN   0.10f
#define REVOLVER_FAN_SPREAD     120
#define REVOLVER_RELOAD_TIME    1.2f
#define REVOLVER_BULLET_SPEED   1800.0f
#define REVOLVER_BULLET_LIFETIME 2.0f
#define REVOLVER_PROJECTILE_SIZE 4.0f
#define REVOLVER_PRECISE_SPREAD  8
// Active reload
#define REVOLVER_RELOAD_SWEET_START 0.60f
#define REVOLVER_RELOAD_SWEET_END   0.75f
#define REVOLVER_RELOAD_FAST_TIME   0.08f
#define REVOLVER_RELOAD_FAIL_PENALTY 0.5f
#define REVOLVER_ARC_COLOR      (Color){ 220, 180, 80, 255 }
#define RELOAD_HIT_COLOR        (Color){ 60, 255, 60, 120 }
#define RELOAD_MISS_COLOR       (Color){ 255, 60, 60, 120 }
#define RELOAD_SWEET_COLOR      (Color){ 255, 255, 100, 100 }

// Rocket Launcher
#define ROCKET_SPEED            800.0f
#define ROCKET_LIFETIME         3.0f
#define ROCKET_DIRECT_DAMAGE    40
#define ROCKET_EXPLOSION_DAMAGE 60
#define ROCKET_EXPLOSION_RADIUS 160.0f
#define ROCKET_KNOCKBACK        250.0f
#define ROCKET_COOLDOWN         0.915f
#define ROCKET_SIZE             6.0f
#define ROCKET_PROJECTILE_SIZE  6.0f
#define ROCKET_JUMP_DAMAGE      4
#define ROCKET_JUMP_FORCE       800.0f
#define ROCKET_JUMP_FRICTION    5.0f

// Grenade Launcher
#define GRENADE_SPEED               500.0f
#define GRENADE_DRAG                1.5f
#define GRENADE_LIFETIME            2.5f
#define GRENADE_DIRECT_DAMAGE       35
#define GRENADE_EXPLOSION_DAMAGE    60
#define GRENADE_EXPLOSION_RADIUS    140.0f
#define GRENADE_KNOCKBACK           200.0f
#define GRENADE_COOLDOWN            1.5f
#define GRENADE_PROJECTILE_SIZE     5.0f
#define GRENADE_MAX_BOUNCES         2
#define GRENADE_BOUNCE_DAMPING      0.6f
#define GRENADE_LAUNCH_HEIGHT_VEL   200.0f
#define GRENADE_ARC_GRAVITY         500.0f
#define GRENADE_ARC_BOUNCE_DAMPING  0.5f
#define GRENADE_ARC_MIN_VEL         20.0f
#define GRENADE_COLOR               (Color){ 214, 144, 36, 255 }
#define GRENADE_GLOW_COLOR          (Color){ 255, 180, 50, 255 }
#define GRENADE_MUZZLE_PARTICLES    6
#define GRENADE_MUZZLE_SPEED        100.0f
#define GRENADE_MUZZLE_SIZE         4.0f
#define GRENADE_MUZZLE_LIFETIME     0.1f

// BFG10k ------------------------------------------------------------------- /
#define BFG_SPEED                   600.0f
#define BFG_LIFETIME                3.0f
#define BFG_PROJECTILE_SIZE         14.0f
#define BFG_DIRECT_DAMAGE           80
#define BFG_CHARGE_COST             1000.0f
#define BFG_COLOR                   (Color){ 120, 220, 255, 255 }
#define BFG_GLOW_COLOR              (Color){ 80, 160, 255, 100 }
#define BFG_CHAIN_DAMAGE            100
#define BFG_CHAIN_RADIUS            400.0f
#define BFG_HOP_DELAY               0.1f
#define BFG_ARC_DURATION            0.4f
#define BFG_MAX_HOPS                256
#define BFG_MAX_CHAIN_TARGETS       256
#define BFG_MAX_ARCS                256
#define BFG_TRAIL_SPEED             50.0f
#define BFG_TRAIL_SIZE              3.0f
#define BFG_TRAIL_LIFETIME          0.15f
#define BFG_MUZZLE_PARTICLES        8
#define BFG_MUZZLE_SPEED            120.0f
#define BFG_MUZZLE_SIZE             5.0f
#define BFG_MUZZLE_LIFETIME         0.12f
#define BFG_HIT_PARTICLES           6
#define BFG_DETONATION_PARTICLES    12
#define BFG_ARC_JITTER              12.0f
#define BFG_ARC_CORE_WIDTH          2.0f
#define BFG_ARC_GLOW_WIDTH          5.0f
#define BFG_PULSE_SPEED             15.0f
#define BFG_PULSE_AMOUNT            2.0f

// Shield ------------------------------------------------------------------- /
#define SHIELD_MAX_HP           200.0f
#define SHIELD_REGEN_DELAY      1.5f    // seconds after lowering before regen starts
#define SHIELD_REGEN_RATE       50.0f   // hp/sec regeneration
#define SHIELD_ARC              PI      // half circle (180 degrees)
#define SHIELD_RADIUS           60.0f   // distance from player center
#define SHIELD_SLOW_FACTOR      0.5f    // movement multiplier while shielding
#define SHIELD_SEGMENTS         16      // draw segments for the arc
#define SHIELD_COLOR            (Color){ 80, 160, 255, 180 }
#define SHIELD_BROKEN_COOLDOWN  2.0f    // lockout when shield breaks

// Ground Slam -------------------------------------------------------------- /
#define SLAM_DAMAGE             15
#define SLAM_RANGE              300.0f
#define SLAM_ARC                (PI * 0.417f)   // 75 degree cone
#define SLAM_KNOCKBACK          250.0f
#define SLAM_STUN_MIN           0.5f
#define SLAM_STUN_MAX           2.0f
#define SLAM_COOLDOWN           6.0f
#define SLAM_VFX_DURATION       0.2f
#define SLAM_PARTICLE_SPEED_MIN 200
#define SLAM_PARTICLE_SPEED_MAX 400
#define SLAM_PARTICLE_SIZE      4.0f
#define SLAM_PARTICLE_LIFETIME  0.3f
#define SLAM_VIS_SEGMENTS       12
#define SLAM_COLOR              (Color){ 255, 200, 60, 255 }

// Parry -------------------------------------------------------------------- /
#define PARRY_WINDOW            0.75f   // active deflect window
#define PARRY_COOLDOWN          4.0f    // normal cooldown
#define PARRY_SUCCESS_COOLDOWN  2.0f    // reduced cooldown on success
#define PARRY_IFRAMES           0.5f    // iframes granted on success
#define PARRY_STUN_DURATION     2.75f   // stun applied to enemies on parry
#define PARRY_KNOCKBACK         400.0f  // knockback on parried contact
#define PARRY_COLOR             (Color){ 220, 180, 50, 220 }

// Turret ------------------------------------------------------------------- /
#define TURRET_LIFETIME         1024.0f
#define TURRET_FIRE_RATE        0.5f
#define TURRET_RANGE            600.0f
#define TURRET_DAMAGE           12
#define TURRET_HP               160
#define TURRET_BULLET_SPEED     800.0f
#define TURRET_BULLET_LIFETIME  3.0f
#define TURRET_BULLET_SIZE      3.0f
#define TURRET_COOLDOWN         8.0f
#define TURRET_MAX_ACTIVE       64
#define TURRET_PLACEMENT_DIST   100.0f
#define TURRET_MUZZLE_OFFSET    10.0f
#define TURRET_MUZZLE_SPEED     80.0f
#define TURRET_MUZZLE_SIZE      2.0f
#define TURRET_MUZZLE_LIFETIME  0.08f
#define TURRET_COLOR            (Color){ 100, 200, 255, 255 }

// Root Mine ---------------------------------------------------------------- /
#define MINE_LIFETIME           1024.0f
#define MINE_TRIGGER_RADIUS     40.0f
#define MINE_ROOT_RADIUS        100.0f
#define MINE_ROOT_DURATION      3.0f
#define MINE_COOLDOWN           5.0f
#define MINE_MAX_ACTIVE         3
#define MINE_COLOR              (Color){ 255, 100, 100, 255 }
#define MINE_PULSE_SPEED        4.0f
#define MINE_WEB_DURATION       MINE_ROOT_DURATION
#define MINE_WEB_SPOKES         8
#define MINE_WEB_RINGS          3

// Healing Field ------------------------------------------------------------ /
#define HEAL_LIFETIME           8.0f
#define HEAL_RADIUS             160.0f
#define HEAL_PER_SEC            15.0f
#define HEAL_COOLDOWN           15.0f
#define HEAL_MAX_ACTIVE         1
#define HEAL_COLOR              (Color){ 80, 255, 80, 120 }
#define HEAL_PULSE_SPEED        3.0f

// Flamethrower ------------------------------------------------------------- /
#define FLAME_FUEL_MAX          100.0f
#define FLAME_DRAIN_RATE        40.0f    // fuel/sec while firing
#define FLAME_REGEN_RATE        12.0f    // fuel/sec regen
#define FLAME_REGEN_DELAY       1.0f     // seconds after release before regen starts
#define FLAME_SPRAY_INTERVAL    0.12f    // seconds between patch spawns
#define FLAME_RANGE             400.0f   // max distance patches land from player
#define FLAME_SPREAD            0.35f    // radians of cone spread (particles only)
#define FLAME_JITTER            20       // patch placement jitter in pixels
#define FLAME_SLOW_FACTOR       0.7f     // move speed while spraying
#define FLAME_PATCH_LIFETIME    5.0f     // seconds each ground patch lasts
#define FLAME_PATCH_RADIUS      30.0f    // damage radius per patch
#define FLAME_PATCH_DPS         30.0f    // damage per second per patch
#define FLAME_PATCH_TICK        0.2f     // damage tick interval
#define FLAME_COLOR             (Color){ 255, 120, 30, 150 }
#define FIRE_EMBER_COLOR        (Color){ 255, 80, 20, 255 }
#define FIRE_GLOW_COLOR         (Color){ 255, 60, 10, 255 }
#define FLAME_FLICKER_SPEED     10.0f
#define FLAME_PARTICLE_SPEED    30.0f
#define FLAME_PARTICLE_SIZE     2.0f
#define FLAME_PARTICLE_LIFETIME 0.3f

// Blink Dagger ------------------------------------------------------------- /
#define BLINK_DISTANCE          480.0f      // 1.5x dash distance (320)
#define BLINK_COOLDOWN          10.0f       // base cd when undamaged
#define BLINK_COOLDOWN_HIT      3.0f        // cd after taking damage
#define BLINK_DAMAGE            40
#define BLINK_DAMAGE_DELAY      1.6f        // seconds before slash damage
#define BLINK_BEAM_DURATION     0.25f       // trail linger time
#define BLINK_BEAM_WIDTH        32.0f
#define BLINK_BEAM_OFFSET       12.0f       // perpendicular spread between lines
#define BLINK_COLOR             (Color){ 80, 140, 255, 40 }
#define BLINK_GLOW_COLOR        (Color){ 80, 140, 255, 40 }
#define BLINK_GLOW_WIDTH        22.0f

// Hitscan weapons ---------------------------------------------------------- /
#if 0 // laser constants — preserved for future use
// Laser (hold F, continuous DPS, terminates at first hit)
#define LASER_DPS                   80
#define LASER_RANGE                 1000.0f
#define LASER_BEAM_WIDTH            2.0f
#define LASER_GLOW_WIDTH            4.0f
//#define LASER_COLOR                 (Color){ 255, 60, 60, 255 }
//#define LASER_GLOW_COLOR            (Color){ 255, 60, 60, 80 }
#define LASER_COLOR                 (Color){ 100, 180, 255, 255 }
#define LASER_GLOW_COLOR            (Color){ 100, 180, 255, 80 }
#define LASER_HIT_PARTICLES         4
#endif

// Railgun (press Z, pierce all, long cooldown)
#define RAILGUN_DAMAGE              200
#define RAILGUN_RANGE               10000.0f
#define RAILGUN_COOLDOWN            2.0f
#define RAILGUN_BEAM_DURATION       0.2f
#define RAILGUN_BEAM_RADIUS         10.0f   // hurtbox
#define RAILGUN_BEAM_WIDTH          20.0f   // cosmetic
#define RAILGUN_GLOW_WIDTH          25.0f    // cosmetic
#define RAILGUN_COLOR               (Color){ 0, 255, 0, 255 }
#define RAILGUN_GLOW_COLOR          (Color){ 0, 255, 0, 80 }
#define RAILGUN_HIT_PARTICLES       6
#define RAILGUN_MUZZLE_PARTICLES    8

// Sniper (press X, single fast projectile, slows target, long cooldown)
#define SNIPER_DAMAGE           120
#define SNIPER_BULLET_SPEED     2400.0f
#define SNIPER_BULLET_LIFETIME  1.5f
#define SNIPER_PROJECTILE_SIZE  4.0f
#define SNIPER_COOLDOWN         0.915f
#define SNIPER_SLOW_FACTOR      0.4f
#define SNIPER_SLOW_DURATION    2.0f
#define SNIPER_SPREAD           4
#define SNIPER_MUZZLE_PARTICLES 6
#define SNIPER_MUZZLE_SPEED     120.0f
#define SNIPER_MUZZLE_SIZE      4.0f
#define SNIPER_MUZZLE_LIFETIME  0.1f
// Sniper — M1 hip fire
#define SNIPER_HIP_SPREAD        160       // wide spread (80/1000 rad)
#define SNIPER_HIP_COOLDOWN      0.915f    // fast semi-auto
#define SNIPER_HIP_BULLET_SPEED  2000.0f  // slightly slower than aimed

// Sniper — M2 aimed shot (hold M2 + click M1)
#define SNIPER_AIM_SPREAD        0        // zero
#define SNIPER_AIM_COOLDOWN      0.915f   // same as hip
#define SNIPER_AIM_BULLET_SPEED  2800.0f  // faster travel
#define SNIPER_AIM_SLOW          0.35f    // movement multiplier while ADS

// Sniper — super shot (dash timing + M2)
#define SNIPER_SUPER_DAMAGE      300      // massive payoff
#define SNIPER_SUPER_SLOW_DUR    4.0f     // longer debuff on target
#define SNIPER_SUPER_SLOW_FACTOR 0.2f     // stronger slow (vs normal 0.4)
#define SNIPER_COLOR            (Color){ 180, 220, 255, 255 }
#define SNIPER_BULLET_LENGTH    5.0f
#define SNIPER_BULLET_WIDTH     1.1f
#define SNIPER_NOSE_LENGTH      8.0f
#define SNIPER_TRAIL_MULT       0.6f
#define SNIPER_BODY_COLOR       (Color){ 160, 100, 60, 255 }
#define SNIPER_TIP_COLOR        (Color){ 200, 130, 80, 255 }

// Enemies ------------------------------------------------------------------ /
// Spawning
#define SPAWN_INTERVAL          2.0f
#define SPAWN_RAMP              0.98f
#define SPAWN_MIN_INTERVAL      0.4f
#define SPAWN_MARGIN            400.0f
#define SPAWN_CHANCE_MAX        100

// Enemy pod values
#define TRI_VALUE               1
#define RECT_VALUE              2
#define RHOM_VALUE              2
#define OCTA_VALUE              3
#define PENTA_VALUE             4
#define HEXA_VALUE              5
#define TRAP_VALUE              10
#define POD_VALUE_INITIAL       3

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

// Enemy — Trapezoid (mini boss)
#define TRAP_SIZE               36.0f
#define TRAP_HP                 800
#define TRAP_SPEED_MIN          70.0f
#define TRAP_SPEED_VAR          0
#define TRAP_CONTACT_DAMAGE     30
#define TRAP_SPAWN_KILLS        50
#define TRAP_SPAWN_CHANCE       0       // not random-spawned
#define TRAP_SCORE              5000
#define TRAP_COLOR              (Color){ 255, 200, 50, 255 }
#define TRAP_OUTLINE_COLOR      (Color){ 180, 120, 20, 255 }
// Shape proportions
#define TRAP_FRONT_WIDTH        0.5f    // narrow front half-width (× size)
#define TRAP_BACK_WIDTH         1.0f    // wide back half-width (× size)
#define TRAP_LENGTH             1.2f    // half-length front to back (× size)
// Attack timing
#define TRAP_ATTACK_INTERVAL    2.5f
// Aimed burst
#define TRAP_BURST_COUNT        5
#define TRAP_BURST_SPREAD       0.5f
#define TRAP_BULLET_SPEED       400.0f
#define TRAP_BULLET_DAMAGE      15
#define TRAP_PROJECTILE_SIZE    5.0f
#define TRAP_BULLET_LIFETIME    8.0f
// Ring shot
#define TRAP_RING_COUNT         12
#define TRAP_RING_SPEED         300.0f
#define TRAP_RING_DAMAGE        12
// Charge + slam
#define TRAP_CHARGE_SPEED       600.0f
#define TRAP_CHARGE_DURATION    0.6f
#define TRAP_SLAM_RADIUS        120.0f
#define TRAP_SLAM_DAMAGE        25

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
#define HITSCAN_HIT_PARTICLES       4

// VFX ---------------------------------------------------------------------- /
// Gun muzzle
#define GUN_MUZZLE_SPEED        100.0f
#define GUN_MUZZLE_SIZE         3.0f
#define GUN_MUZZLE_LIFETIME     0.08f

// Shotgun muzzle
#define SHOTGUN_MUZZLE_SPEED    150.0f
#define SHOTGUN_MUZZLE_SIZE     5.0f
#define SHOTGUN_MUZZLE_LIFETIME 0.2f

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
#define SPIN_DEFLECT_SPEED_MULT 1.5f
#define SPIN_DEFLECT_DAMAGE_MULT 2

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
#define TRAP_MUZZLE_SIZE        4.0f
#define TRAP_MUZZLE_LIFETIME    0.15f

// Drawing ------------------------------------------------------------------ /
// Player solid rendering
#define SHAPE_SUBDIV_TRI        6       // tetra/cube/octa subdivision
#define SHAPE_SUBDIV_PENTA      4       // icosa/dodeca subdivision
#define SHAPE_SAT_DEFAULT       1.0f
#define SHAPE_SAT_CUBE          0.85f
#define SHAPE_SAT_DODECA        0.85f
// Sphere (weapon select unselected player)
#define SPHERE_SLICES           12
#define SPHERE_STACKS           8

// Turret visuals
#define TURRET_VIS_ROT_SPEED    3.0f
#define TURRET_VIS_GLOW_R       8.0f
#define TURRET_VIS_SIZE         10.0f
#define TURRET_VIS_THICKNESS    2.0f
#define TURRET_HPBAR_W          16.0f
#define TURRET_HPBAR_H          2.0f
#define TURRET_HPBAR_YOFFSET    13.0f

// Mine visuals
#define MINE_VIS_PULSE_MIN      0.6f
#define MINE_VIS_PULSE_RANGE    0.4f
#define MINE_VIS_OUTER_R        6.0f
#define MINE_VIS_CORE_R         3.0f

// Heal visuals
#define HEAL_VIS_PULSE_MIN      0.7f
#define HEAL_VIS_PULSE_RANGE    0.3f

// Fire patch visuals
#define FIRE_PATCH_EMBER_COUNT  6
#define FIRE_PATCH_FADE_THRESH  0.3f

// Explosion ring visuals
#define EXPLOSION_RING_DECAY    0.3f
#define EXPLOSION_RING_ALPHA    0.7f

// Minigun visuals
#define MINIGUN_VIS_BARREL_COUNT 3
#define MINIGUN_VIS_SPIN_SPEED  30.0f

// Shadows
#define SHADOW_OFFSET_X         4.0f
#define SHADOW_OFFSET_Y         8.0f
#define SHADOW_ALPHA            0.3f
#define SHADOW_COLOR            (Color){ 240, 240, 240 }
#define SHADOW_SCALE            1.0f
#define SHADOW_LAG              20.0f

#define BULLET_TRAIL_FACTOR     0.02f
#define ENEMY_HPBAR_HEIGHT      3.0f
#define ENEMY_HPBAR_YOFFSET     8
#define SHAPE_HUE_SPEED         60.0f
#define SHAPE_HUE_DEPTH_SCALE   90.0f
#define CUBE_HUE_VERTEX_STEP    45.0f
#define OCTA_HUE_VERTEX_STEP    60.0f
#define ICOSA_HUE_VERTEX_STEP   30.0f
#define DODECA_HUE_VERTEX_STEP  18.0f

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
#define HUD_LEVEL_Y             78
// Cooldown bottom bar
#define HUD_CD_W                22
#define HUD_CD_H                22
#define HUD_CD_FONT             13
#define HUD_CD_ALPHA            0.6f    // bar fill transparency
#define HUD_CD_LABEL_GAP        2       // gap between label and bar
#define HUD_CD_COL_GAP          12      // horizontal gap between slots
#define HUD_CD_BOTTOM_Y         30      // distance from screen bottom
#define HUD_PIP_W               12
#define HUD_PIP_GAP             4
// Weapon swap (top-left, below Level)
#define HUD_SWAP_Y              98
// FPS
#define HUD_FPS_FONT            16
#define HUD_FPS_X               80
// Crosshair
#define HUD_CROSSHAIR_SIZE      4.0f
#define HUD_CROSSHAIR_THICKNESS 1.0f
#define HUD_CROSSHAIR_GAP       2.0f
// Active reload/overheat arc (near player)
#define HUD_ARC_INNER_R         16.0f
#define HUD_ARC_OUTER_R         20.0f
#define HUD_ARC_Y_OFFSET        4.0f
#define HUD_ARC_START_DEG       30.0f
#define HUD_ARC_END_DEG         150.0f
#define HUD_ARC_SEGMENTS        24
#define HUD_ARC_CURSOR_PAD      2.0f
#define HUD_ARC_SEG_GAP         2.0f
#define HUD_ARC_LABEL_FONT      8
#define HUD_ARC_LABEL_PAD       4.0f
#define HUD_ARC_BG_COLOR        (Color){ 40, 40, 40, 160 }
// Info text
#define HUD_BOTTOM_Y            20
#define HUD_TITLE_FONT          12
#define HUD_TITLE_X             210
// Pause
#define HUD_PAUSE_FONT          60
#define HUD_PAUSE_Y             130
#define HUD_RESUME_FONT         20
#define HUD_RESUME_Y            -60
#define HUD_PAUSE_KEYS_Y        -30
#define HUD_PAUSE_KEYS_FONT     16
#define HUD_PAUSE_KEYS_SPACING  24
#define HUD_PAUSE_KEYS_COL_W    200
// Game Over
#define HUD_GO_FONT             70
#define HUD_GO_Y                60
#define HUD_GO_SCORE_FONT       28
#define HUD_GO_SCORE_Y          20
#define HUD_GO_RESTART_FONT     20
#define HUD_GO_RESTART_Y        60

// Weapon Select Screen ----------------------------------------------------- /
#define SELECT_TITLE_FONT       40
#define SELECT_TITLE_Y          0.08f
#define SELECT_DESC_FONT        12
#define SELECT_DESC_GAP         6
#define SELECT_HINT_FONT        14
#define SELECT_HINT_GAP         10
#define SELECT_HIGHLIGHT_COLOR  (Color){ 100, 200, 255, 255 }
// Pedestals sit inside the base room (below combat zone)
#define SELECT_PEDESTAL_Y       (BASE_TOP + BASE_H * 0.65f)
#define SELECT_PEDESTAL_X       75.0f
#define SELECT_PEDESTAL_SPACING 8.0f
#define SELECT_PEDESTAL_CURVE   50.0f
#define SELECT_SOLID_SIZE       25.0f
#define SELECT_INTERACT_RADIUS  50.0f
#define SELECT_PLAYER_SPEED     250.0f
#define SELECT_RING_PULSE_SPEED 4.0f
#define SELECT_RING_RADIUS      35.0f
#define SELECT_LABEL_FONT       20
#define SELECT_PROMPT_FONT      14
#define TRANSITION_DURATION     0.5f

