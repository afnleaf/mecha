// collision.c
// all the collision detection math goes here
// hitboxes, hurtboxes, etc
#include "game.h"

// Check if point is inside a swing arc
// what is C convetion for multi line function signature?
// Line segment vs circle: does segment AB touch circle at C with radius r?
static bool LineSegCircle(
    Vector2 a, Vector2 b, Vector2 c, float r)
{
    Vector2 ab = Vector2Subtract(b, a);
    Vector2 ac = Vector2Subtract(c, a);
    float ab2 = Vector2DotProduct(ab, ab);
    if (ab2 < 1e-8f) return Vector2DistanceSqr(a, c) <= r * r;
    float t = Vector2DotProduct(ac, ab) / ab2;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    Vector2 closest = Vector2Add(a, Vector2Scale(ab, t));
    return Vector2DistanceSqr(closest, c) <= r * r;
}

// Line segment vs OBB: transform to local space, then slab test
static bool LineSegOBB(
    Vector2 la, Vector2 lb,
    Vector2 center, float hw, float hh, float angle)
{
    float ca = cosf(angle), sa = sinf(angle);
    Vector2 da = Vector2Subtract(la, center);
    Vector2 db = Vector2Subtract(lb, center);
    // rotate into OBB local space
    Vector2 a = {  da.x * ca + da.y * sa, -da.x * sa + da.y * ca };
    Vector2 b = {  db.x * ca + db.y * sa, -db.x * sa + db.y * ca };
    // either endpoint inside box
    if (fabsf(a.x) <= hw && fabsf(a.y) <= hh) return true;
    if (fabsf(b.x) <= hw && fabsf(b.y) <= hh) return true;
    // parametric slab clipping
    Vector2 d = Vector2Subtract(b, a);
    float tmin = 0.0f, tmax = 1.0f;
    if (fabsf(d.x) > 1e-8f) {
        float t1 = (-hw - a.x) / d.x;
        float t2 = ( hw - a.x) / d.x;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        tmin = fmaxf(tmin, t1);
        tmax = fminf(tmax, t2);
        if (tmin > tmax) return false;
    } else if (fabsf(a.x) > hw) return false;
    if (fabsf(d.y) > 1e-8f) {
        float t1 = (-hh - a.y) / d.y;
        float t2 = ( hh - a.y) / d.y;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        tmin = fmaxf(tmin, t1);
        tmax = fminf(tmax, t2);
        if (tmin > tmax) return false;
    } else if (fabsf(a.y) > hh) return false;
    return true;
}

// OBB collision helpers (for RECT enemy hitbox)
// Get the enemy facing angle (faces aggro target or player shadow)
float EnemyAngle(Enemy *e) {
    Vector2 target = (e->aggroIdx >= 0)
        ? g.deployables[e->aggroIdx].pos : g.player.shadowPos;
    Vector2 toTarget = Vector2Subtract(target, e->pos);
    return atan2f(toTarget.y, toTarget.x);
}

// Check if a point is inside an oriented bounding box
static bool PointInOBB(
    Vector2 point, Vector2 center,
    float hw, float hh, float angle)
{
    float ca = cosf(angle), sa = sinf(angle);
    Vector2 d = Vector2Subtract(point, center);
    float localX =  d.x * ca + d.y * sa;
    float localY = -d.x * sa + d.y * ca;
    return fabsf(localX) <= hw && fabsf(localY) <= hh;
}

// Check if a circle overlaps an oriented bounding box
// refactor this signature to each same type on same line
static bool CircleOBBOverlap(
    Vector2 circlePos, float radius,
    Vector2 center, float hw, float hh, float angle)
{
    float ca = cosf(angle), sa = sinf(angle);
    Vector2 d = Vector2Subtract(circlePos, center);
    float localX =  d.x * ca + d.y * sa;
    float localY = -d.x * sa + d.y * ca;
    // Closest point on the box to the circle center
    float cx = fmaxf(-hw, fminf(localX, hw));
    float cy = fmaxf(-hh, fminf(localY, hh));
    float dx = localX - cx, dy = localY - cy;
    return (dx * dx + dy * dy) <= radius * radius;
}

// Collision Dispatchers — switch on enemy type, one case per shaPe
// Sweep line (sword, spin): does segment AB intersect enemy hitbox?
// this does all the hitscan damage detection too because it makes contact
bool EnemyHitSweep(Enemy *e, Vector2 a, Vector2 b) {
    switch (e->type) {
    case RECT:
        return LineSegOBB(a, b, e->pos,
            e->size, e->size * RECT_ASPECT_RATIO, EnemyAngle(e));
    default:
        return LineSegCircle(a, b, e->pos, e->size);
    }
}

// Point-in-shape (projectile collision): is point within enemy + padding?
bool EnemyHitPoint(Enemy *e, Vector2 point, float pad) {
    switch (e->type) {
    case RECT:
        return PointInOBB(point, e->pos,
            e->size + pad, e->size * RECT_ASPECT_RATIO + pad,
            EnemyAngle(e));
    default:
        return Vector2Distance(point, e->pos) <= e->size + pad;
    }
}

// Circle overlap (AoE, contact): does circle overlap enemy hitbox?
bool EnemyHitCircle(Enemy *e, Vector2 center, float radius) {
    switch (e->type) {
    case RECT:
        return CircleOBBOverlap(center, radius, e->pos,
            e->size, e->size * RECT_ASPECT_RATIO, EnemyAngle(e));
    default:
        return Vector2Distance(center, e->pos) <= radius + e->size;
    }
}
