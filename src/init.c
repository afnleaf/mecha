#include "game.h"

// the static struct and the emscripten void requirement are important
// futher study needed
GameState g;

// ========================================================================== /
// Init
// ========================================================================== /
// this is quite important.
void InitGame(void)
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
    p->revolver.rounds      = REVOLVER_ROUNDS;
    p->shield.hp            = SHIELD_MAX_HP;
    p->shield.maxHp         = SHIELD_MAX_HP;
    p->flame.fuel           = FLAME_FUEL_MAX;

    // ability slots — default layout
    p->slots[0]  = (AbilitySlot){ ABL_SHOTGUN,  KEY_Q };
    p->slots[1]  = (AbilitySlot){ ABL_RAILGUN,  KEY_E };
    p->slots[2]  = (AbilitySlot){ ABL_SPIN,     KEY_LEFT_SHIFT };
    p->slots[3]  = (AbilitySlot){ ABL_SLAM,     KEY_ONE };
    p->slots[4]  = (AbilitySlot){ ABL_PARRY,    KEY_TWO };
    p->slots[5]  = (AbilitySlot){ ABL_TURRET,   KEY_THREE };
    p->slots[6]  = (AbilitySlot){ ABL_MINE,     KEY_FOUR };
    p->slots[7]  = (AbilitySlot){ ABL_HEAL,     KEY_Z };
    p->slots[8]  = (AbilitySlot){ ABL_SHIELD,   KEY_X };
    p->slots[9]  = (AbilitySlot){ ABL_GRENADE,  KEY_C };
    p->slots[10] = (AbilitySlot){ ABL_FIRE,     KEY_V };
    p->slots[11] = (AbilitySlot){ ABL_BFG,      KEY_F };

    p->shadowPos            = p->pos;

    g.camera.offset   = (Vector2){ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    g.camera.target   = p->pos;
    g.camera.zoom     = 1.0f;

    g.spawnInterval   = SPAWN_INTERVAL;
    g.spawnTimer      = SPAWN_INITIAL_DELAY;

    g.screen          = SCREEN_SELECT;
    g.selectIndex     = 0;
    g.selectPhase     = 0;
}
