// game.h
// the function declarations header
#ifndef GAME_H
#define GAME_H

#include "mecha.h"

// Shared State
extern GameState g;

// init.c
void InitGame(void);
void InitPlayer(void);

// spawn.c
extern const EnemyDef ENEMY_DEFS[];
Projectile* SpawnProjectile(
    Vector2 pos, Vector2 dir,
    float speed, int damage, float lifetime, float size,
    bool isEnemy, bool knockback,
    ProjectileType type, DamageType dmgType);
void FireShotgunBlast(Player *p, Vector2 toMouse);
void SpawnRocket(Player *p, Vector2 toMouse);
void SpawnGrenade(Player *p, Vector2 toMouse);
void SpawnEnemy(void);
void SpawnParticle(
    Vector2 pos, Vector2 vel,
    Color color, float size, float lifetime);
void SpawnParticles(Vector2 pos, Color color, int count);
void SpawnBeam(Vector2 origin, Vector2 tip,
    float duration, Color color, float width);
void DamageEnemy(int idx, int damage, DamageType dmgType, DamageMethod method);
void DamagePlayer(int damage, DamageType dmgType, DamageMethod method);

// collision.c
float EnemyAngle(Enemy *e);
bool EnemyHitSweep(Enemy *e, Vector2 a, Vector2 b);
bool EnemyHitPoint(Enemy *e, Vector2 point, float pad);
bool EnemyHitCircle(Enemy *e, Vector2 center, float radius);

// update.c
void UpdateGame(void);
void ShootRect(Enemy *e, Vector2 toTarget, float dist, float dt);
void ShootPenta(Enemy *e, Vector2 toTarget, float dist, float dt);
void ShootHexa(Enemy *e, Vector2 toTarget, float dist, float dt);
void ShootTrap(Enemy *e, Vector2 toTarget, float dist, float dt);

// draw.c
void NextFrame(void);

#endif // GAME_H
