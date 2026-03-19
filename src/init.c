// init.c
// sets the initial game state before any transformations are done
#include "game.h"

// the static struct and the emscripten void requirement are important
// futher study needed
GameState g;

// this is quite important.
void InitGame(void)
{
    memset(&g, 0, sizeof(g));
    
    // must be called after memory is set
    InitPlayer();

    g.camera.offset   = (Vector2){ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    g.camera.target   = (Vector2){ BASE_CENTER_X, BASE_CENTER_Y };  // base room center
    g.camera.zoom     = 1.0f;

    g.spawnInterval   = SPAWN_INTERVAL;
    g.spawnTimer      = SPAWN_INITIAL_DELAY;
    g.podValue        = POD_VALUE_INITIAL;

    g.phase           = PHASE_SELECT;
    g.selectIndex     = -1;
    g.selectPhase     = 0;

    // start player in base room, above the pedestal U curve
    g.player.pos      = (Vector2){ PLAYER_SPAWN_X, PLAYER_SPAWN_Y };
    g.player.shadowPos = g.player.pos;

    // shop pedestal positions — U along base room walls
    // left wall (4), bottom wall (5), right wall (4)
    g.shopIndex = -1;
    {
        float inset = SHOP_WALL_INSET;
        float top = BASE_TOP + inset;
        float bot = BASE_BOTTOM - inset;
        float h = bot - top;
        int idx = 0;
        // left wall — bottom to top
        for (int i = 0; i < 4; i++)
            g.shopPedestals[idx++] = (Vector2){
                inset, bot - h * (float)i / 3.0f };
        // bottom wall — left to right
        for (int i = 0; i < 5; i++)
            g.shopPedestals[idx++] = (Vector2){
                inset + (BASE_W - 2*inset) * (float)(i + 1) / 6.0f, bot };
        // right wall — bottom to top
        for (int i = 0; i < 4; i++)
            g.shopPedestals[idx++] = (Vector2){
                BASE_W - inset, bot - h * (float)i / 3.0f };
    }
}

// seperate for ease of maintainability
void InitPlayer(void) 
{
    Player *p  = &g.player;
    p->pos     = (Vector2){ MAP_LEFT + MAP_SIZE / 2.0f, MAP_SIZE / 2.0f };
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

    // ability slots — default layout (all unowned, purchased at shop)
    p->slots[0]  = (AbilitySlot){ ABL_BFG,      KEY_R,          false };
    p->slots[1]  = (AbilitySlot){ ABL_SHOTGUN,  KEY_Q,          false };
    p->slots[2]  = (AbilitySlot){ ABL_RAILGUN,  KEY_E,          false };
    p->slots[3]  = (AbilitySlot){ ABL_SPIN,     KEY_LEFT_SHIFT, false };
    p->slots[4]  = (AbilitySlot){ ABL_PARRY,    KEY_F,          false };
    p->slots[5]  = (AbilitySlot){ ABL_HEAL,     KEY_Z,          false };
    p->slots[6]  = (AbilitySlot){ ABL_SHIELD,   KEY_X,          false };
    p->slots[7]  = (AbilitySlot){ ABL_GRENADE,  KEY_C,          false };
    p->slots[8]  = (AbilitySlot){ ABL_FIRE,     KEY_V,          false };
    p->slots[9]  = (AbilitySlot){ ABL_SLAM,     KEY_ONE,        false };
    p->slots[10] = (AbilitySlot){ ABL_BLINK,    KEY_TWO,        false };
    p->slots[11] = (AbilitySlot){ ABL_TURRET,   KEY_THREE,      false };
    p->slots[12] = (AbilitySlot){ ABL_MINE,     KEY_FOUR,       false };

    p->shadowPos            = p->pos;
}

// helpers ------------------------------------------------------------------ /

// resets all the entity pools, useful for scene transitions
// setting .active = false is fine because the arrays are already allocated
void ClearPools(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
        g.vfx.particles[i].active = false;
    for (int i = 0; i < MAX_PROJECTILES; i++)
        g.projectiles[i].active = false;
    for (int i = 0; i < MAX_BEAMS; i++)
        g.vfx.beams[i].active = false;
    for (int i = 0; i < MAX_VFX_TIMERS; i++)
        g.vfx.timers[i].active = false;
    for (int i = 0; i < MAX_DEPLOYABLES; i++)
        g.deployables[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++)
        g.enemies[i].active = false;
    g.lightning.active = false;
}
