// draw.c
// render the game state as pixels
#include "game.h"

static const char* AbilityName(AbilityID id) {
    switch (id) {
        case ABL_SHOTGUN: return "Shotgun";
        case ABL_SPIN:    return "Spin";
        case ABL_GRENADE: return "Grenade";
        case ABL_RAILGUN: return "Railgun";
        case ABL_BFG:     return "BFG";
        case ABL_SHIELD:  return "Shield";
        case ABL_TURRET:  return "Turret";
        case ABL_MINE:    return "Mine";
        case ABL_SLAM:    return "Slam";
        case ABL_PARRY:   return "Parry";
        case ABL_HEAL:    return "Heal";
        case ABL_FIRE:    return "Flame";
        default:          return NULL;
    }
}

static const char* KeyName(int key) {
    switch (key) {
        case KEY_Q:             return "Q";
        case KEY_E:             return "E";
        case KEY_F:             return "F";
        case KEY_Z:             return "Z";
        case KEY_X:             return "X";
        case KEY_C:             return "C";
        case KEY_V:             return "V";
        case KEY_LEFT_SHIFT:    return "Shift";
        case KEY_LEFT_CONTROL:  return "Ctrl";
        case KEY_ONE:           return "1";
        case KEY_TWO:           return "2";
        case KEY_THREE:         return "3";
        case KEY_FOUR:          return "4";
        default:                return "?";
    }
}

static const char* WeaponName(WeaponType w) {
    switch (w) {
        case WPN_GUN:           return "GUN";
        case WPN_SWORD:         return "SWORD";
        case WPN_REVOLVER:      return "REVLVR";
        case WPN_SNIPER:        return "SNIPER";
        case WPN_ROCKET:        return "ROCKET";
        default:                return "???";
    }
}

// Draw - Rainbow cube (fake 3D, subdivided gradient faces)
// Manual HSV conversion to ensure no dependency issues in web build
static Color HsvToRgb(float h, float s, float v, float alpha) {
    if (h < 0.0f) h = fmodf(h, 360.0f) + 360.0f;
    else if (h >= 360.0f) h = fmodf(h, 360.0f);
    
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r = 0, g = 0, b = 0;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    
    return (Color){
        (u8)((r + m) * 255.0f),
        (u8)((g + m) * 255.0f),
        (u8)((b + m) * 255.0f),
        (u8)(alpha * 255.0f)
    };
}

// shape helpers ---------------------------------------------------------- /
static void ProjectVertices(
    const float vtx[][3], int n,
    float rotY, float rotX,
    Vector2 center, float scale,
    Vector2 *pj, float *pz)
{
    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);
    for (int i = 0; i < n; i++) {
        float x = vtx[i][0] * scale, y = vtx[i][1] * scale, z = vtx[i][2] * scale;
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ center.x + x1, center.y + y1 };
        pz[i] = z2;
    }
}

static void SubdivDrawTri(
    Vector2 p0, Vector2 p1, Vector2 p2,
    float h0, float h1, float h2,
    int N, float sat, float alpha)
{
    for (int row = 0; row < N; row++) {
        for (int col = 0; col < N - row; col++) {
            float u0 = (float)col / N;
            float v0 = (float)row / N;
            float w0 = 1.0f - u0 - v0;
            float u1 = (float)(col + 1) / N;
            float v1 = (float)row / N;
            float w1 = 1.0f - u1 - v1;
            float u2 = (float)col / N;
            float v2 = (float)(row + 1) / N;
            float w2 = 1.0f - u2 - v2;

            Vector2 q0 = { w0*p0.x + u0*p1.x + v0*p2.x, w0*p0.y + u0*p1.y + v0*p2.y };
            Vector2 q1 = { w1*p0.x + u1*p1.x + v1*p2.x, w1*p0.y + u1*p1.y + v1*p2.y };
            Vector2 q2 = { w2*p0.x + u2*p1.x + v2*p2.x, w2*p0.y + u2*p1.y + v2*p2.y };

            float hC = fmodf(w0*h0 + u0*h1 + v0*h2 + 360.0f, 360.0f);
            DrawTriangle(q0, q1, q2, HsvToRgb(hC, sat, 1.0f, alpha));

            if (col + 1 < N - row) {
                float u3 = (float)(col + 1) / N;
                float v3 = (float)(row + 1) / N;
                float w3 = 1.0f - u3 - v3;
                Vector2 q3 = { w3*p0.x + u3*p1.x + v3*p2.x, w3*p0.y + u3*p1.y + v3*p2.y };
                float hC2 = fmodf(w3*h0 + u3*h1 + v3*h2 + 360.0f, 360.0f);
                DrawTriangle(q1, q3, q2, HsvToRgb(hC2, sat, 1.0f, alpha));
            }
        }
    }
}

static void SubdivDrawQuad(
    Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3,
    float h0, float h1, float h2, float h3,
    int N, float sat, float alpha)
{
    #define BILERP(A,B,C,D,u,v) \
        ((1-(v))*((1-(u))*(A) + (u)*(B)) + (v)*((1-(u))*(D) + (u)*(C)))
    for (int gy = 0; gy < N; gy++) {
        for (int gx = 0; gx < N; gx++) {
            float u0 = (float)gx / N, u1 = (float)(gx + 1) / N;
            float v0 = (float)gy / N, v1 = (float)(gy + 1) / N;
            Vector2 q00 = { BILERP(p0.x,p1.x,p2.x,p3.x,u0,v0), BILERP(p0.y,p1.y,p2.y,p3.y,u0,v0) };
            Vector2 q10 = { BILERP(p0.x,p1.x,p2.x,p3.x,u1,v0), BILERP(p0.y,p1.y,p2.y,p3.y,u1,v0) };
            Vector2 q11 = { BILERP(p0.x,p1.x,p2.x,p3.x,u1,v1), BILERP(p0.y,p1.y,p2.y,p3.y,u1,v1) };
            Vector2 q01 = { BILERP(p0.x,p1.x,p2.x,p3.x,u0,v1), BILERP(p0.y,p1.y,p2.y,p3.y,u0,v1) };
            float uc = (u0 + u1) * 0.5f, vc = (v0 + v1) * 0.5f;
            float hC = BILERP(h0, h1, h2, h3, uc, vc);
            Color cc = HsvToRgb(hC, sat, 1.0f, alpha);
            DrawTriangle(q00, q10, q11, cc);
            DrawTriangle(q00, q11, q01, cc);
        }
    }
    #undef BILERP
}

static void DrawShapeShadow(
    const Vector2 *sp, const int faces[][5],
    int nFaces, int vpf, float shadowAlpha)
{
    Color scol = Fade(SHADOW_COLOR, shadowAlpha);
    for (int f = 0; f < nFaces; f++) {
        Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]], c = sp[faces[f][2]];
        float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
        if (cross >= 0) continue;
        if (vpf == 3) {
            DrawTriangle(a, b, c, scol);
        } else if (vpf == 4) {
            Vector2 d = sp[faces[f][3]];
            DrawTriangle(a, b, c, scol);
            DrawTriangle(a, c, d, scol);
        } else {
            for (int tri = 0; tri < 3; tri++)
                DrawTriangle(a, sp[faces[f][tri+1]], sp[faces[f][tri+2]], scol);
        }
    }
}

static void CullSortFaces(
    const Vector2 *pj, const float *pz,
    const int faces[][5], int nFaces, int vpf,
    bool *faceVis, float *faceZ, int *order)
{
    for (int f = 0; f < nFaces; f++) {
        faceZ[f] = 0;
        for (int vi = 0; vi < vpf; vi++) faceZ[f] += pz[faces[f][vi]];
        faceZ[f] /= (float)vpf;
        Vector2 a = pj[faces[f][0]], b = pj[faces[f][1]], c = pj[faces[f][2]];
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
        order[f] = f;
    }
    for (int i = 0; i < nFaces - 1; i++)
        for (int j = i + 1; j < nFaces; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }
}

// tetrahedron ------------------------------------------------------------- /
static void DrawTetra2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size / 1.73f;
    float vtx[4][3] = {
        { s,  s,  s}, { s, -s, -s}, {-s,  s, -s}, {-s, -s,  s},
    };
    int faces[4][5] = {
        {0,1,2,0,0}, {0,2,3,0,0}, {0,3,1,0,0}, {1,3,2,0,0},
    };

    if (shadowAlpha > 0) {
        Vector2 sp[4]; float spz[4];
        ProjectVertices(vtx, 4, rotY, PI/2, shadowPos, SHADOW_SCALE, sp, spz);
        DrawShapeShadow(sp, faces, 4, 3, shadowAlpha);
    }

    Vector2 pj[4]; float pz[4]; float hues[4];
    ProjectVertices(vtx, 4, rotY, rotX, pos, 1.0f, pj, pz);
    float time = (float)GetTime();
    for (int i = 0; i < 4; i++)
        hues[i] = time * SHAPE_HUE_SPEED + pz[i] * (SHAPE_HUE_DEPTH_SCALE / s);

    bool faceVis[4]; float faceZ[4]; int order[4];
    CullSortFaces(pj, pz, faces, 4, 3, faceVis, faceZ, order);

    Color edge = Fade(WHITE, 0.5f * alpha);
    for (int fi = 0; fi < 4; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;
        SubdivDrawTri(pj[faces[f][0]], pj[faces[f][1]], pj[faces[f][2]],
                      hues[faces[f][0]], hues[faces[f][1]], hues[faces[f][2]],
                      SHAPE_SUBDIV_TRI, SHAPE_SAT_DEFAULT, alpha);
        for (int e = 0; e < 3; e++)
            DrawLineV(pj[faces[f][e]], pj[faces[f][(e+1)%3]], edge);
    }
}

static void DrawCube2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size / 1.73f;
    float vtx[8][3] = {
        {-s,-s,-s}, { s,-s,-s}, { s, s,-s}, {-s, s,-s},
        {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s},
    };
    int faces[6][5] = {
        {0,3,2,1,0}, {4,5,6,7,0}, {0,4,7,3,0},
        {1,2,6,5,0}, {0,1,5,4,0}, {3,7,6,2,0},
    };

    if (shadowAlpha > 0) {
        Vector2 sp[8]; float spz[8];
        ProjectVertices(vtx, 8, rotY, PI/2, shadowPos, SHADOW_SCALE, sp, spz);
        DrawShapeShadow(sp, faces, 6, 4, shadowAlpha);
    }

    Vector2 pj[8]; float pz[8]; float hues[8];
    ProjectVertices(vtx, 8, rotY, rotX, pos, 1.0f, pj, pz);
    float time = (float)GetTime();
    for (int i = 0; i < 8; i++)
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * CUBE_HUE_VERTEX_STEP;

    bool faceVis[6]; float faceZ[6]; int order[6];
    CullSortFaces(pj, pz, faces, 6, 4, faceVis, faceZ, order);

    Color edge = Fade(WHITE, 0.5f * alpha);
    for (int fi = 0; fi < 6; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;
        SubdivDrawQuad(pj[faces[f][0]], pj[faces[f][1]], pj[faces[f][2]], pj[faces[f][3]],
                       hues[faces[f][0]], hues[faces[f][1]], hues[faces[f][2]], hues[faces[f][3]],
                       SHAPE_SUBDIV_TRI, SHAPE_SAT_CUBE, alpha);
        for (int e = 0; e < 4; e++)
            DrawLineV(pj[faces[f][e]], pj[faces[f][(e+1)%4]], edge);
    }
}

// octahedron -------------------------------------------------------------- /
static void DrawOcta2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size;
    float vtx[6][3] = {
        { s, 0, 0}, {-s, 0, 0}, { 0, s, 0}, { 0,-s, 0}, { 0, 0, s}, { 0, 0,-s},
    };
    int faces[8][5] = {
        {0,2,4,0,0}, {2,1,4,0,0}, {1,3,4,0,0}, {3,0,4,0,0},
        {2,0,5,0,0}, {1,2,5,0,0}, {3,1,5,0,0}, {0,3,5,0,0},
    };

    if (shadowAlpha > 0) {
        Vector2 sp[6]; float spz[6];
        ProjectVertices(vtx, 6, rotY, PI/2, shadowPos, SHADOW_SCALE, sp, spz);
        DrawShapeShadow(sp, faces, 8, 3, shadowAlpha);
    }

    Vector2 pj[6]; float pz[6]; float hues[6];
    ProjectVertices(vtx, 6, rotY, rotX, pos, 1.0f, pj, pz);
    float time = (float)GetTime();
    for (int i = 0; i < 6; i++)
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * OCTA_HUE_VERTEX_STEP;

    bool faceVis[8]; float faceZ[8]; int order[8];
    CullSortFaces(pj, pz, faces, 8, 3, faceVis, faceZ, order);

    Color edge = Fade(WHITE, 0.5f * alpha);
    for (int fi = 0; fi < 8; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;
        SubdivDrawTri(pj[faces[f][0]], pj[faces[f][1]], pj[faces[f][2]],
                      hues[faces[f][0]], hues[faces[f][1]], hues[faces[f][2]],
                      SHAPE_SUBDIV_TRI, SHAPE_SAT_DEFAULT, alpha);
        for (int e = 0; e < 3; e++)
            DrawLineV(pj[faces[f][e]], pj[faces[f][(e+1)%3]], edge);
    }
}

// icosahedron ------------------------------------------------------------- /
static void DrawIcosa2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    float sc = size / phi;
    float vtx[12][3] = {
        { 0,  sc,  sc*phi}, { 0,  sc, -sc*phi},
        { 0, -sc,  sc*phi}, { 0, -sc, -sc*phi},
        { sc,  sc*phi, 0}, {-sc,  sc*phi, 0},
        { sc, -sc*phi, 0}, {-sc, -sc*phi, 0},
        { sc*phi, 0,  sc}, { sc*phi, 0, -sc},
        {-sc*phi, 0,  sc}, {-sc*phi, 0, -sc},
    };
    int faces[20][5] = {
        {0,2,8,0,0},   {0,8,4,0,0},   {0,4,5,0,0},   {0,5,10,0,0},  {0,10,2,0,0},
        {2,6,8,0,0},   {8,6,9,0,0},   {8,9,4,0,0},   {4,9,1,0,0},   {4,1,5,0,0},
        {5,1,11,0,0},  {5,11,10,0,0}, {10,11,7,0,0}, {10,7,2,0,0},  {2,7,6,0,0},
        {3,9,6,0,0},   {3,1,9,0,0},   {3,11,1,0,0},  {3,7,11,0,0},  {3,6,7,0,0},
    };

    if (shadowAlpha > 0) {
        Vector2 sp[12]; float spz[12];
        ProjectVertices(vtx, 12, rotY, PI/2, shadowPos, SHADOW_SCALE, sp, spz);
        DrawShapeShadow(sp, faces, 20, 3, shadowAlpha);
    }

    Vector2 pj[12]; float pz[12]; float hues[12];
    ProjectVertices(vtx, 12, rotY, rotX, pos, 1.0f, pj, pz);
    float time = (float)GetTime();
    for (int i = 0; i < 12; i++)
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * ICOSA_HUE_VERTEX_STEP;

    bool faceVis[20]; float faceZ[20]; int order[20];
    CullSortFaces(pj, pz, faces, 20, 3, faceVis, faceZ, order);

    Color edge = Fade(WHITE, 0.5f * alpha);
    for (int fi = 0; fi < 20; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;
        SubdivDrawTri(pj[faces[f][0]], pj[faces[f][1]], pj[faces[f][2]],
                      hues[faces[f][0]], hues[faces[f][1]], hues[faces[f][2]],
                      SHAPE_SUBDIV_PENTA, SHAPE_SAT_DEFAULT, alpha);
        for (int e = 0; e < 3; e++)
            DrawLineV(pj[faces[f][e]], pj[faces[f][(e+1)%3]], edge);
    }
}

// dodecahedron ------------------------------------------------------------- /
static void DrawDodeca2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    float iphi = 1.0f / phi;
    float sc = size / 1.73f;
    float vtx[20][3] = {
        { sc, sc, sc}, { sc, sc,-sc}, { sc,-sc, sc}, { sc,-sc,-sc},
        {-sc, sc, sc}, {-sc, sc,-sc}, {-sc,-sc, sc}, {-sc,-sc,-sc},
        {0,  sc*phi,  sc*iphi}, {0,  sc*phi, -sc*iphi},
        {0, -sc*phi,  sc*iphi}, {0, -sc*phi, -sc*iphi},
        { sc*iphi, 0,  sc*phi}, {-sc*iphi, 0,  sc*phi},
        { sc*iphi, 0, -sc*phi}, {-sc*iphi, 0, -sc*phi},
        { sc*phi,  sc*iphi, 0}, { sc*phi, -sc*iphi, 0},
        {-sc*phi,  sc*iphi, 0}, {-sc*phi, -sc*iphi, 0},
    };
    int faces[12][5] = {
        { 0,12, 2,17,16}, { 0,16, 1, 9, 8}, { 0, 8, 4,13,12}, { 1,16,17, 3,14},
        { 1,14,15, 5, 9}, { 2,12,13, 6,10}, { 2,10,11, 3,17}, { 4, 8, 9, 5,18},
        { 4,18,19, 6,13}, { 7,11,10, 6,19}, { 7,19,18, 5,15}, { 7,15,14, 3,11},
    };

    if (shadowAlpha > 0) {
        Vector2 sp[20]; float spz[20];
        ProjectVertices(vtx, 20, rotY, PI/2, shadowPos, SHADOW_SCALE, sp, spz);
        DrawShapeShadow(sp, faces, 12, 5, shadowAlpha);
    }

    Vector2 pj[20]; float pz[20]; float hues[20];
    ProjectVertices(vtx, 20, rotY, rotX, pos, 1.0f, pj, pz);
    float time = (float)GetTime();
    for (int i = 0; i < 20; i++)
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * DODECA_HUE_VERTEX_STEP;

    bool faceVis[12]; float faceZ[12]; int order[12];
    CullSortFaces(pj, pz, faces, 12, 5, faceVis, faceZ, order);

    Color edge = Fade(WHITE, 0.5f * alpha);
    for (int fi = 0; fi < 12; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;
        // Fan pentagon into 3 triangles from vertex 0
        for (int tri = 0; tri < 3; tri++)
            SubdivDrawTri(pj[faces[f][0]], pj[faces[f][tri+1]], pj[faces[f][tri+2]],
                          hues[faces[f][0]], hues[faces[f][tri+1]], hues[faces[f][tri+2]],
                          SHAPE_SUBDIV_PENTA, SHAPE_SAT_DODECA, alpha);
        for (int e = 0; e < 5; e++)
            DrawLineV(pj[faces[f][e]], pj[faces[f][(e+1)%5]], edge);
    }
}

// sphere (weapon select player) ------------------------------------------- /
static void DrawSphere2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    // UV sphere: SPHERE_SLICES longitude, SPHERE_STACKS latitude
    // Vertex layout: [0] = north pole, [1] = south pole,
    //   [2..] = ring vertices (STACKS-1 rings * SLICES each)
    int nVerts = 2 + (SPHERE_STACKS - 1) * SPHERE_SLICES;
    float vtx[2 + (SPHERE_STACKS - 1) * SPHERE_SLICES][3];

    // North pole
    vtx[0][0] = 0; vtx[0][1] = size; vtx[0][2] = 0;
    // South pole
    vtx[1][0] = 0; vtx[1][1] = -size; vtx[1][2] = 0;
    // Ring vertices
    for (int stack = 1; stack < SPHERE_STACKS; stack++) {
        float phi = PI * (float)stack / (float)SPHERE_STACKS;
        float sp = sinf(phi), cp = cosf(phi);
        for (int slice = 0; slice < SPHERE_SLICES; slice++) {
            float theta = 2.0f * PI * (float)slice / (float)SPHERE_SLICES;
            int idx = 2 + (stack - 1) * SPHERE_SLICES + slice;
            vtx[idx][0] = size * sp * cosf(theta);
            vtx[idx][1] = size * cp;
            vtx[idx][2] = size * sp * sinf(theta);
        }
    }

    // Shadow: circle (sphere always projects to circle)
    if (shadowAlpha > 0) {
        DrawCircleV(shadowPos, size * SHADOW_SCALE,
            Fade(SHADOW_COLOR, shadowAlpha));
    }

    // Project all vertices to 2D
    Vector2 pj[2 + (SPHERE_STACKS - 1) * SPHERE_SLICES];
    float pz[2 + (SPHERE_STACKS - 1) * SPHERE_SLICES];
    float hues[2 + (SPHERE_STACKS - 1) * SPHERE_SLICES];
    ProjectVertices(vtx, nVerts, rotY, rotX, pos, 1.0f, pj, pz);
    float time = (float)GetTime();
    for (int i = 0; i < nVerts; i++)
        hues[i] = fmodf(time * 60.0f + (pz[i] / size) * 180.0f + 360.0f, 360.0f);

    // Draw faces with backface culling
    // Top cap: north pole (0) to first ring
    for (int s = 0; s < SPHERE_SLICES; s++) {
        int a = 0;
        int b = 2 + s;
        int c = 2 + (s + 1) % SPHERE_SLICES;
        float cross = (pj[b].x-pj[a].x)*(pj[c].y-pj[a].y) -
                       (pj[b].y-pj[a].y)*(pj[c].x-pj[a].x);
        if (cross < 0) {
            float hC = fmodf((hues[a] + hues[b] + hues[c]) / 3.0f, 360.0f);
            DrawTriangle(pj[a], pj[b], pj[c],
                HsvToRgb(hC, SHAPE_SAT_DEFAULT, 1.0f, alpha));
        }
    }

    // Middle bands
    for (int stack = 0; stack < SPHERE_STACKS - 2; stack++) {
        for (int slice = 0; slice < SPHERE_SLICES; slice++) {
            int curr = 2 + stack * SPHERE_SLICES + slice;
            int next = 2 + stack * SPHERE_SLICES + (slice + 1) % SPHERE_SLICES;
            int below = 2 + (stack + 1) * SPHERE_SLICES + slice;
            int belowNext = 2 + (stack + 1) * SPHERE_SLICES +
                (slice + 1) % SPHERE_SLICES;

            // Triangle 1: curr, below, next
            float cross1 = (pj[below].x-pj[curr].x)*(pj[next].y-pj[curr].y) -
                            (pj[below].y-pj[curr].y)*(pj[next].x-pj[curr].x);
            if (cross1 < 0) {
                float hC = fmodf((hues[curr]+hues[below]+hues[next]) / 3.0f,
                    360.0f);
                DrawTriangle(pj[curr], pj[below], pj[next],
                    HsvToRgb(hC, SHAPE_SAT_DEFAULT, 1.0f, alpha));
            }

            // Triangle 2: next, below, belowNext
            float cross2 = (pj[below].x-pj[next].x)*(pj[belowNext].y-pj[next].y) -
                            (pj[below].y-pj[next].y)*(pj[belowNext].x-pj[next].x);
            if (cross2 < 0) {
                float hC = fmodf((hues[next]+hues[below]+hues[belowNext]) / 3.0f,
                    360.0f);
                DrawTriangle(pj[next], pj[below], pj[belowNext],
                    HsvToRgb(hC, SHAPE_SAT_DEFAULT, 1.0f, alpha));
            }
        }
    }

    // Bottom cap: south pole (1) to last ring
    int lastRing = 2 + (SPHERE_STACKS - 2) * SPHERE_SLICES;
    for (int s = 0; s < SPHERE_SLICES; s++) {
        int a = 1;
        int b = lastRing + (s + 1) % SPHERE_SLICES;
        int c = lastRing + s;
        float cross = (pj[b].x-pj[a].x)*(pj[c].y-pj[a].y) -
                       (pj[b].y-pj[a].y)*(pj[c].x-pj[a].x);
        if (cross < 0) {
            float hC = fmodf((hues[a] + hues[b] + hues[c]) / 3.0f, 360.0f);
            DrawTriangle(pj[a], pj[b], pj[c],
                HsvToRgb(hC, SHAPE_SAT_DEFAULT, 1.0f, alpha));
        }
    }

    // Edge wireframe for definition
    Color edge = Fade(WHITE, 0.3f * alpha);
    for (int stack = 0; stack < SPHERE_STACKS - 1; stack++) {
        for (int slice = 0; slice < SPHERE_SLICES; slice++) {
            int curr = 2 + stack * SPHERE_SLICES + slice;
            int next = 2 + stack * SPHERE_SLICES + (slice + 1) % SPHERE_SLICES;
            DrawLineV(pj[curr], pj[next], edge);
            if (stack < SPHERE_STACKS - 2) {
                int below = 2 + (stack + 1) * SPHERE_SLICES + slice;
                DrawLineV(pj[curr], pj[below], edge);
            }
        }
    }
}

// player solid dispatcher -------------------------------------------------- /
static void DrawPlayerSolid(Vector2 pos, float size, float rotY, float rotX, float alpha,
                            Vector2 shadowPos, float shadowAlpha) {
    switch (g.player.primary) {
        case WPN_SWORD:    DrawTetra2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_REVOLVER: DrawCube2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_GUN:      DrawOcta2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_SNIPER:   DrawDodeca2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        case WPN_ROCKET:   DrawIcosa2D(pos, size, rotY, rotX, alpha, shadowPos, shadowAlpha); break;
        default: break;
    }
}

// projectile rendering ----------------------------------------------------- /
static void DrawProjectiles(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *b = &g.projectiles[i];
        if (!b->active) continue;

        // sniper .50 cal bullet — elongated pointed shape
        if (b->dmgType == DMG_PIERCE && !b->isEnemy) {
            Vector2 fwd = Vector2Normalize(b->vel);
            Vector2 perp = { -fwd.y, fwd.x };
            float len = b->size * SNIPER_BULLET_LENGTH;
            float w   = b->size * SNIPER_BULLET_WIDTH;
            float tip = b->size * SNIPER_NOSE_LENGTH;
            Vector2 noseTip = Vector2Add(b->pos, Vector2Scale(fwd, tip));
            Vector2 frontL = Vector2Add(b->pos, Vector2Scale(perp,  w));
            Vector2 frontR = Vector2Add(b->pos, Vector2Scale(perp, -w));
            Vector2 base = Vector2Subtract(b->pos, Vector2Scale(fwd, len));
            Vector2 rearL = Vector2Add(base, Vector2Scale(perp,  w));
            Vector2 rearR = Vector2Add(base, Vector2Scale(perp, -w));
            DrawTriangle(frontL, rearL, rearR, SNIPER_BODY_COLOR);
            DrawTriangle(frontL, rearR, frontR, SNIPER_BODY_COLOR);
            DrawTriangle(noseTip, frontR, frontL, SNIPER_TIP_COLOR);
            Vector2 trail = Vector2Subtract(base, Vector2Scale(fwd, len * SNIPER_TRAIL_MULT));
            DrawLineEx(base, trail, w * 0.8f, Fade(SNIPER_COLOR, 0.4f));
        } else if (b->type == PROJ_BFG) {
            float pulse = sinf(GetTime() * BFG_PULSE_SPEED) * BFG_PULSE_AMOUNT;
            float outerR = b->size + pulse;
            float innerR = b->size * 0.6f + pulse * 0.5f;
            DrawCircleV(b->pos, outerR, (Color)BFG_GLOW_COLOR);
            DrawCircleV(b->pos, innerR, (Color)BFG_COLOR);
            DrawCircleV(b->pos, innerR * 0.4f, WHITE);
        } else if (b->type == PROJ_GRENADE) {
            DrawCircleV(b->pos, b->size * 0.8f, Fade(BLACK, 0.3f));
            Vector2 drawPos = { b->pos.x, b->pos.y - b->height };
            DrawCircleV(drawPos, b->size, GRENADE_COLOR);
            DrawCircleLinesV(drawPos, b->size + 1.0f, GRENADE_GLOW_COLOR);
        } else {
            Color bColor = b->isEnemy ? MAGENTA : YELLOW;
            float bSize = b->size;
            DrawCircleV(b->pos, bSize, bColor);
            Vector2 trail = Vector2Subtract(b->pos, Vector2Scale(b->vel, BULLET_TRAIL_FACTOR));
            DrawLineV(trail, b->pos, Fade(bColor, 0.5f));
        }
    }
}

// sword arc rendering ------------------------------------------------------ /
static void DrawSwordArc(Vector2 origin, float timer, float duration,
                         float angle, float arc, float radius, Color color)
{
    float progress = 1.0f - (timer / duration);
    float arcHalf = arc / 2.0f;
    for (int i = 1; i <= SWORD_DRAW_SEGMENTS; i++) {
        float segPos = (float)i / SWORD_DRAW_SEGMENTS;
        if (segPos > progress) break;

        float t = segPos / progress;
        float alpha = t * t * 0.9f;
        float thick = 2.0f + t * 6.0f;

        float a0 = angle - arcHalf + arc * (float)(i - 1) / SWORD_DRAW_SEGMENTS;
        float a1 = angle - arcHalf + arc * (float)i / SWORD_DRAW_SEGMENTS;
        Vector2 pt0 = Vector2Add(origin,
            (Vector2){ cosf(a0) * radius, sinf(a0) * radius });
        Vector2 pt1 = Vector2Add(origin,
            (Vector2){ cosf(a1) * radius, sinf(a1) * radius });
        DrawLineEx(pt0, pt1, thick, Fade(color, alpha));
    }
    float sweepAngle = angle - arcHalf + arc * progress;
    Vector2 sweepEnd = Vector2Add(origin,
        (Vector2){ cosf(sweepAngle) * radius, sinf(sweepAngle) * radius });
    DrawLineEx(origin, sweepEnd, 3.0f, WHITE);
}

// Draw - World (camera space) ---------------------------------------------- /
static void DrawWorld(void)
{
    Player *p = &g.player;

    // --- Map grid ---
    for (float x = 0; x <= MAP_SIZE; x += GRID_STEP) {
        DrawLineV((Vector2){ x, 0 }, (Vector2){ x, MAP_SIZE }, GRID_COLOR);
    }
    for (float y = 0; y <= MAP_SIZE; y += GRID_STEP) {
        DrawLineV((Vector2){ 0, y }, (Vector2){ MAP_SIZE, y }, GRID_COLOR);
    }

    // Map boundary
    DrawRectangleLinesEx((Rectangle){ 0, 0, MAP_SIZE, MAP_SIZE },
        MAP_BORDER_THICKNESS, RED);

    // --- Particles (behind entities) ---
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &g.vfx.particles[i];
        if (!pt->active) continue;
        float alpha = pt->lifetime / pt->maxLifetime;
        Color c = Fade(pt->color, alpha);
        DrawCircleV(pt->pos, pt->size * alpha, c);
    }

    // --- Deployables (ground level) ---
    for (int i = 0; i < MAX_DEPLOYABLES; i++) {
        Deployable *d = &g.deployables[i];
        if (!d->active) continue;
        float t = (float)GetTime();

        switch (d->type) {
        case DEPLOY_TURRET: {
            // rotating triangle with line toward nearest enemy
            float rot = t * TURRET_VIS_ROT_SPEED;
            DrawCircleV(d->pos, TURRET_VIS_GLOW_R, Fade((Color)TURRET_COLOR, 0.3f));
            for (int v = 0; v < 3; v++) {
                float a0 = rot + v * (2.0f * PI / 3.0f);
                float a1 = rot + (v + 1) * (2.0f * PI / 3.0f);
                Vector2 p0 = Vector2Add(d->pos,
                    (Vector2){ cosf(a0) * TURRET_VIS_SIZE, sinf(a0) * TURRET_VIS_SIZE });
                Vector2 p1 = Vector2Add(d->pos,
                    (Vector2){ cosf(a1) * TURRET_VIS_SIZE, sinf(a1) * TURRET_VIS_SIZE });
                DrawLineEx(p0, p1, TURRET_VIS_THICKNESS, (Color)TURRET_COLOR);
            }
            // HP bar
            float hpFrac = (float)d->hp / TURRET_HP;
            float barW = TURRET_HPBAR_W, barH = TURRET_HPBAR_H;
            Vector2 barPos = { d->pos.x - barW * 0.5f, d->pos.y + TURRET_HPBAR_YOFFSET };
            DrawRectangleV(barPos, (Vector2){ barW, barH },
                Fade(DARKGRAY, 0.5f));
            Color hpColor = (hpFrac > 0.5f) ? (Color)TURRET_COLOR : ORANGE;
            if (hpFrac < 0.25f) hpColor = RED;
            DrawRectangleV(barPos, (Vector2){ barW * hpFrac, barH }, hpColor);
        } break;
        case DEPLOY_MINE: {
            float pulse = MINE_VIS_PULSE_MIN + MINE_VIS_PULSE_RANGE * sinf(t * MINE_PULSE_SPEED);
            DrawCircleV(d->pos, d->radius * pulse,
                Fade((Color)MINE_COLOR, 0.15f));
            DrawCircleLinesV(d->pos, MINE_VIS_OUTER_R, (Color)MINE_COLOR);
            DrawCircleV(d->pos, MINE_VIS_CORE_R,
                Fade((Color)MINE_COLOR, pulse));
        } break;
        case DEPLOY_HEAL: {
            float pulse = HEAL_VIS_PULSE_MIN + HEAL_VIS_PULSE_RANGE * sinf(t * HEAL_PULSE_SPEED);
            DrawCircleV(d->pos, d->radius * pulse,
                Fade((Color)HEAL_COLOR, 0.1f));
            DrawCircleLinesV(d->pos, d->radius,
                Fade((Color)HEAL_COLOR, 0.4f));
        } break;
        case DEPLOY_FIRE: {
            float life = d->timer / FLAME_PATCH_LIFETIME;
            float fade = (life > FIRE_PATCH_FADE_THRESH) ? 1.0f : life / FIRE_PATCH_FADE_THRESH;
            float r = d->radius;
            int embers = FIRE_PATCH_EMBER_COUNT;
            for (int j = 0; j < embers; j++) {
                float seed = i * 7.13f + j * 3.71f;
                float ox = sinf(seed + t * 1.5f) * r * 0.6f
                         + cosf(seed * 2.3f) * r * 0.3f;
                float oy = cosf(seed * 1.7f + t * 1.8f) * r * 0.5f
                         - (1.0f - life) * r * 0.4f;
                float eSize = r * (0.3f + 0.25f * sinf(seed * 5.1f + t * FLAME_FLICKER_SPEED));
                Vector2 ep = { d->pos.x + ox, d->pos.y + oy };
                float heat = 1.0f - (fabsf(ox) + fabsf(oy)) / (r * 1.2f);
                if (heat < 0) heat = 0;
                Color c;
                if (heat > 0.6f)
                    c = YELLOW;
                else if (heat > 0.3f)
                    c = ORANGE;
                else
                    c = (Color){ 255, 80, 20, 255 };
                DrawCircleV(ep, eSize, Fade(c, 0.22f * fade));
            }
            DrawCircleV(d->pos, r * 0.8f,
                Fade((Color){ 255, 60, 10, 255 }, 0.08f * fade));
            float coreFlicker = 0.6f + 0.4f * sinf(t * 14.0f + i * 2.0f);
            DrawCircleV(d->pos, r * 0.2f * coreFlicker,
                Fade(YELLOW, 0.35f * fade));
        } break;
        }
    }

    // --- VFX Timers (mine webs, explosion rings) ---
    for (int i = 0; i < MAX_VFX_TIMERS; i++) {
        VfxTimer *vt = &g.vfx.timers[i];
        if (!vt->active) continue;
        float t = vt->timer / vt->duration;

        switch (vt->type) {
        case VFX_MINE_WEB: {
            float alpha = t;
            float r = MINE_ROOT_RADIUS;
            for (int s = 0; s < MINE_WEB_SPOKES; s++) {
                float a = s * (2.0f * PI / MINE_WEB_SPOKES);
                Vector2 tip = { vt->pos.x + cosf(a) * r,
                                vt->pos.y + sinf(a) * r };
                DrawLineEx(vt->pos, tip, 1.0f, Fade(WHITE, alpha * 0.6f));
            }
            for (int ring = 1; ring <= MINE_WEB_RINGS; ring++) {
                float rr = r * ring / (float)MINE_WEB_RINGS;
                for (int s = 0; s < MINE_WEB_SPOKES; s++) {
                    float a0 = s * (2.0f * PI / MINE_WEB_SPOKES);
                    float a1 = (s + 1) * (2.0f * PI / MINE_WEB_SPOKES);
                    Vector2 p0 = { vt->pos.x + cosf(a0) * rr,
                                   vt->pos.y + sinf(a0) * rr };
                    Vector2 p1 = { vt->pos.x + cosf(a1) * rr,
                                   vt->pos.y + sinf(a1) * rr };
                    DrawLineEx(p0, p1, 1.0f, Fade(WHITE, alpha * 0.4f));
                }
            }
        } break;
        case VFX_EXPLOSION: {
            float alpha = t * EXPLOSION_RING_ALPHA;
            float radius = ROCKET_EXPLOSION_RADIUS * (1.0f - t * EXPLOSION_RING_DECAY);
            DrawCircleLinesV(vt->pos, radius, Fade(ORANGE, alpha));
            DrawCircleLinesV(vt->pos, radius - 2.0f, Fade(RED, alpha * 0.6f));
        } break;
        }
    }

    // --- Projectiles ---
    DrawProjectiles();

    // --- Lightning chain arcs (BFG) ---
    if (g.lightning.active) {
        LightningChain *lc = &g.lightning;
        for (int i = 0; i < lc->arcCount; i++) {
            LightningArc *a = &lc->arcs[i];
            if (!a->active) continue;
            float t = a->timer / a->duration;

            // jagged line: 3 midpoints with perpendicular jitter
            Vector2 diff = Vector2Subtract(a->to, a->from);
            Vector2 perp = { -diff.y, diff.x };
            float pLen = Vector2Length(perp);
            if (pLen > 0.1f) perp = Vector2Scale(perp, 1.0f / pLen);

            Vector2 pts[5];
            pts[0] = a->from;
            pts[4] = a->to;
            for (int m = 1; m <= 3; m++) {
                float frac = (float)m / 4.0f;
                Vector2 base = Vector2Lerp(a->from, a->to, frac);
                float jitter = (float)GetRandomValue(
                    -(int)BFG_ARC_JITTER, (int)BFG_ARC_JITTER);
                pts[m] = Vector2Add(base, Vector2Scale(perp, jitter));
            }

            // draw glow then core
            Color glow = { BFG_GLOW_COLOR.r, BFG_GLOW_COLOR.g,
                           BFG_GLOW_COLOR.b, (u8)(BFG_GLOW_COLOR.a * t) };
            Color core = Fade((Color)BFG_COLOR, t);
            for (int s = 0; s < 4; s++) {
                DrawLineEx(pts[s], pts[s + 1], BFG_ARC_GLOW_WIDTH * t, glow);
                DrawLineEx(pts[s], pts[s + 1], BFG_ARC_CORE_WIDTH, core);
            }
        }
    }

    // --- Enemies ---
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g.enemies[i];
        if (!e->active) continue;

        float eAngle = EnemyAngle(e);
        Color eColor = (e->hitFlash > 0) ? WHITE : RED;
        // Icy tint when slowed
        if (e->slowTimer > 0 && e->hitFlash <= 0) {
            eColor = SNIPER_COLOR;
        }

        switch (e->type) {
        case TRI: {
            Vector2 tip = Vector2Add(
                    e->pos, 
                    (Vector2){ 
                        cosf(eAngle) * e->size, 
                        sinf(eAngle) * e->size 
                    }
                );
            Vector2 left = Vector2Add(
                    e->pos, 
                    (Vector2){ 
                        cosf(eAngle + TRI_WING_ANGLE) * e->size,
                        sinf(eAngle + TRI_WING_ANGLE) * e->size 
                    }
                );
            Vector2 right = Vector2Add(
                    e->pos, 
                    (Vector2){ 
                        cosf(eAngle - TRI_WING_ANGLE) * e->size,
                        sinf(eAngle - TRI_WING_ANGLE) * e->size 
                    }
                );
            DrawTriangle(tip, right, left, eColor);
            DrawTriangleLines(tip, right, left, MAROON);
        } break;
        case RECT: {
            Color rFill = (e->hitFlash > 0) ? WHITE : MAGENTA;
            float hw = e->size, hh = e->size * RECT_ASPECT_RATIO;
            float ca = cosf(eAngle), sa = sinf(eAngle);
            DrawRectanglePro(
                (Rectangle){ e->pos.x, e->pos.y, hw * 2.0f, hh * 2.0f },
                (Vector2){ hw, hh },
                eAngle * RAD2DEG,
                rFill);
            // Corner outline
            Vector2 corners[4] = {
                    { 
                        e->pos.x + (-hw)*ca - (-hh)*sa, 
                        e->pos.y + (-hw)*sa + (-hh)*ca 
                    },
                    { 
                        e->pos.x + ( hw)*ca - (-hh)*sa, 
                        e->pos.y + ( hw)*sa + (-hh)*ca 
                    },
                    { 
                        e->pos.x + ( hw)*ca - ( hh)*sa, 
                        e->pos.y + ( hw)*sa + ( hh)*ca 
                    },
                    { 
                        e->pos.x + (-hw)*ca - ( hh)*sa, 
                        e->pos.y + (-hw)*sa + ( hh)*ca 
                    },
            };
            for (int ci = 0; ci < 4; ci++)
                DrawLineV(corners[ci], corners[(ci + 1) % 4], DARKPURPLE);
        } break;
        case PENTA: {
            Color pFill = (e->hitFlash > 0) ? WHITE : PENTA_COLOR;
            DrawPoly(e->pos, 5, e->size, eAngle * RAD2DEG, pFill);
            DrawPolyLines(e->pos, 5, e->size, eAngle * RAD2DEG, DARKGREEN);
        } break;
        case RHOM: {
            Color rFill = (e->hitFlash > 0) ? WHITE : RHOM_COLOR;
            // Elongated diamond pointing at player
            float s = e->size;
            Vector2 tip = Vector2Add(e->pos,
                (Vector2){ cosf(eAngle) * s * RHOM_TIP_MULT,
                           sinf(eAngle) * s * RHOM_TIP_MULT });
            Vector2 back = Vector2Add(e->pos,
                (Vector2){ cosf(eAngle + PI) * s * RHOM_BACK_MULT,
                           sinf(eAngle + PI) * s * RHOM_BACK_MULT });
            float perpAngle = eAngle + PI * 0.5f;
            Vector2 left = Vector2Add(e->pos,
                (Vector2){ cosf(perpAngle) * s * RHOM_SIDE_MULT,
                           sinf(perpAngle) * s * RHOM_SIDE_MULT });
            Vector2 right = Vector2Add(e->pos,
                (Vector2){ cosf(perpAngle) * s * -RHOM_SIDE_MULT,
                           sinf(perpAngle) * s * -RHOM_SIDE_MULT });
            // Draw as two triangles (tip-left-right, back-right-left)
            DrawTriangle(tip, right, left, rFill);
            DrawTriangle(back, left, right, rFill);
            // Outline
            Color rOutline = RHOM_OUTLINE_COLOR;
            DrawLineV(tip, left, rOutline);
            DrawLineV(left, back, rOutline);
            DrawLineV(back, right, rOutline);
            DrawLineV(right, tip, rOutline);
        } break;
        case HEXA: {
            Color hFill = (e->hitFlash > 0) ? WHITE : HEXA_COLOR;
            DrawPoly(e->pos, 6, e->size, eAngle * RAD2DEG, hFill);
            DrawPolyLines(e->pos, 6, e->size, eAngle * RAD2DEG, HEXA_OUTLINE_COLOR);
        } break;
        case OCTA: {
            Color hFill = (e->hitFlash > 0) ? WHITE : OCTA_COLOR;
            DrawPoly(e->pos, 8, e->size, eAngle * RAD2DEG, hFill);
            DrawPolyLines(e->pos, 8, e->size, eAngle * RAD2DEG, OCTA_OUTLINE_COLOR);
        } break;
        case TRAP: {
            Color tFill = (e->hitFlash > 0) ? WHITE : TRAP_COLOR;
            float s = e->size;
            float ca = cosf(eAngle), sa = sinf(eAngle);
            float fw = s * TRAP_FRONT_WIDTH;
            float bw = s * TRAP_BACK_WIDTH;
            float hl = s * TRAP_LENGTH;
            // Narrow front (toward player)
            Vector2 fl = { e->pos.x + ca*hl - sa*fw,
                           e->pos.y + sa*hl + ca*fw };
            Vector2 fr = { e->pos.x + ca*hl + sa*fw,
                           e->pos.y + sa*hl - ca*fw };
            // Wide back (away from player)
            Vector2 bl = { e->pos.x - ca*hl - sa*bw,
                           e->pos.y - sa*hl + ca*bw };
            Vector2 br = { e->pos.x - ca*hl + sa*bw,
                           e->pos.y - sa*hl - ca*bw };
            DrawTriangle(br, fl, fr, tFill);
            DrawTriangle(br, bl, fl, tFill);
            // Outline
            DrawLineV(fr, fl, TRAP_OUTLINE_COLOR);
            DrawLineV(fl, bl, TRAP_OUTLINE_COLOR);
            DrawLineV(bl, br, TRAP_OUTLINE_COLOR);
            DrawLineV(br, fr, TRAP_OUTLINE_COLOR);
        } break;
        case CIRC: {
            break;
        } break;
        default: break;
        }

        // HP bar
        if (e->hp < e->maxHp) {
            float barW = e->size * 2.0f;
            float barH = ENEMY_HPBAR_HEIGHT;
            float hpRatio = (float)e->hp / e->maxHp;
            DrawRectangle(
                (int)(e->pos.x - barW / 2),
                (int)(e->pos.y - e->size - ENEMY_HPBAR_YOFFSET),
                (int)barW,
                (int)barH,
                DARKGRAY);
            DrawRectangle(
                (int)(e->pos.x - barW / 2),
                (int)(e->pos.y - e->size - ENEMY_HPBAR_YOFFSET), 
                (int)(barW * hpRatio), 
                (int)barH, 
                RED);
        }
    }

    // --- Player — RGB color cube ---
    bool visible = true;
    if (p->iFrames > 0) {
        visible = ((int)(p->iFrames * PLAYER_BLINK_RATE) % 2 == 0);
    }

    if (visible) {
        float t = (float)GetTime();
        float rotY = t * PLAYER_ROT_SPEED;
        float rotX = PLAYER_ROT_TILT;

        // Ghost trail during dash
        if (p->dash.active) {
            for (int gi = DASH_GHOST_COUNT; gi >= 1; gi--) {
                float offset = (float)gi * DASH_GHOST_SPACING;
                Vector2 ghostPos = Vector2Subtract(p->pos,
                    Vector2Scale(p->dash.dir, offset));
                float ga = 0.5f * (1.0f - (float)gi / (DASH_GHOST_COUNT + 1.0f));
                DrawPlayerSolid(
                    ghostPos,
                    p->size,
                    rotY - (float)gi * DASH_GHOST_ROT_STEP,
                    rotX,
                    ga,
                    (Vector2){0}, 0);
            }
        }

        // Decoy ghost — pulsing translucent player at anchored pos
        if (p->dash.decoyActive) {
            float pulse = sinf((float)GetTime() * DECOY_PULSE_SPEED);
            float da = DECOY_MIN_ALPHA
                + (DECOY_MAX_ALPHA - DECOY_MIN_ALPHA)
                * (pulse * 0.5f + 0.5f);
            DrawPlayerSolid(
                p->dash.decoyPos, p->size,
                rotY, rotX, da,
                (Vector2){0}, 0);
        }

        // Shadow drawn inside DrawPlayerSolid at lagged position
        Vector2 shadowPos = {
            p->shadowPos.x + SHADOW_OFFSET_X,
            p->shadowPos.y + SHADOW_OFFSET_Y
        };
        DrawPlayerSolid(p->pos, p->size, rotY, rotX, 1.0f, shadowPos, SHADOW_ALPHA);

        // Shield arc
        if (p->shield.active && p->shield.hp > 0) {
            float shieldAlpha = p->shield.hp / p->shield.maxHp;
            float startAngle = p->shield.angle - SHIELD_ARC / 2.0f;
            for (int si = 0; si < SHIELD_SEGMENTS; si++) {
                float t0 = (float)si / SHIELD_SEGMENTS;
                float t1 = (float)(si + 1) / SHIELD_SEGMENTS;
                float a0 = startAngle + SHIELD_ARC * t0;
                float a1 = startAngle + SHIELD_ARC * t1;
                Vector2 p0 = Vector2Add(p->pos,
                    (Vector2){ cosf(a0) * SHIELD_RADIUS,
                               sinf(a0) * SHIELD_RADIUS });
                Vector2 p1 = Vector2Add(p->pos,
                    (Vector2){ cosf(a1) * SHIELD_RADIUS,
                               sinf(a1) * SHIELD_RADIUS });
                DrawLineEx(p0, p1, 3.0f,
                    Fade((Color)SHIELD_COLOR, shieldAlpha));
            }
            // Inner glow arc
            for (int si = 0; si < SHIELD_SEGMENTS; si++) {
                float t0 = (float)si / SHIELD_SEGMENTS;
                float t1 = (float)(si + 1) / SHIELD_SEGMENTS;
                float a0 = startAngle + SHIELD_ARC * t0;
                float a1 = startAngle + SHIELD_ARC * t1;
                float innerR = SHIELD_RADIUS * 0.85f;
                Vector2 p0 = Vector2Add(p->pos,
                    (Vector2){ cosf(a0) * innerR, sinf(a0) * innerR });
                Vector2 p1 = Vector2Add(p->pos,
                    (Vector2){ cosf(a1) * innerR, sinf(a1) * innerR });
                DrawLineEx(p0, p1, 1.5f,
                    Fade((Color)SHIELD_COLOR, shieldAlpha * 0.3f));
            }
        }

        // Ground Slam expanding cone
        if (p->slam.vfxTimer > 0) {
            float progress = 1.0f - (p->slam.vfxTimer / SLAM_VFX_DURATION);
            float r = SLAM_RANGE * progress;
            float alpha = 1.0f - progress;
            float halfArc = SLAM_ARC * 0.5f;
            float a = p->slam.angle;
            int segs = SLAM_VIS_SEGMENTS;
            // outer arc
            for (int i = 0; i < segs; i++) {
                float a0 = a - halfArc + (float)i / segs * SLAM_ARC;
                float a1 = a - halfArc + (float)(i + 1) / segs * SLAM_ARC;
                Vector2 p0 = Vector2Add(p->pos,
                    (Vector2){ cosf(a0) * r, sinf(a0) * r });
                Vector2 p1 = Vector2Add(p->pos,
                    (Vector2){ cosf(a1) * r, sinf(a1) * r });
                DrawLineEx(p0, p1, 2.0f, Fade((Color)SLAM_COLOR, alpha));
            }
            // side edges
            Vector2 left = Vector2Add(p->pos,
                (Vector2){ cosf(a - halfArc) * r, sinf(a - halfArc) * r });
            Vector2 right = Vector2Add(p->pos,
                (Vector2){ cosf(a + halfArc) * r, sinf(a + halfArc) * r });
            DrawLineEx(p->pos, left, 1.5f, Fade(WHITE, alpha * 0.5f));
            DrawLineEx(p->pos, right, 1.5f, Fade(WHITE, alpha * 0.5f));
        }

        // Parry active flash
        if (p->parry.active) {
            float alpha = p->parry.timer / PARRY_WINDOW;
            DrawCircleLinesV(p->pos, p->size + 20.0f,
                Fade((Color)PARRY_COLOR, alpha));
            DrawCircleLinesV(p->pos, p->size + 16.0f,
                Fade((Color)PARRY_COLOR, alpha * 0.5f));
        }

        // Gun barrel (heat-tinted)
        float ca = cosf(p->angle), sa = sinf(p->angle);
        // Barrel color lerps DARKGRAY -> RED based on heat
        Color barrelColor = DARKGRAY;
        if (p->primary == WPN_GUN && p->gun.heat > 0) {
            float h = p->gun.heat;
            barrelColor = (Color){
                (u8)(80 + (int)(175 * h)),
                (u8)(80 - (int)(80 * h)),
                (u8)(80 - (int)(80 * h)),
                255 };
        }
        if (p->primary == WPN_GUN && p->minigun.spinUp > 0) {
            Vector2 miniTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + MINIGUN_TIP_OFFSET),
                           sa * (p->size + MINIGUN_TIP_OFFSET) });
            DrawLineEx(p->pos, miniTip, MINIGUN_BARREL_THICKNESS, barrelColor);
            // spinning barrel lines
            float spin = (float)GetTime() * p->minigun.spinUp * MINIGUN_VIS_SPIN_SPEED;
            for (int b = 0; b < MINIGUN_VIS_BARREL_COUNT; b++) {
                float bAngle = p->angle + spin + b * (2.0f * PI / MINIGUN_VIS_BARREL_COUNT);
                float bca = cosf(bAngle), bsa = sinf(bAngle);
                Vector2 bOff = { -bsa * 2.0f, bca * 2.0f };
                Vector2 bStart = Vector2Add(p->pos, bOff);
                Vector2 bEnd = Vector2Add(miniTip, bOff);
                DrawLineEx(bStart, bEnd, 1.0f,
                    Fade(LIGHTGRAY, 0.4f * p->minigun.spinUp));
            }
        } else if (p->primary == WPN_ROCKET) {
            // Thicker rocket tube barrel
            Vector2 rocketTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + MINIGUN_TIP_OFFSET),
                           sa * (p->size + MINIGUN_TIP_OFFSET) });
            DrawLineEx(p->pos, rocketTip, MINIGUN_BARREL_THICKNESS + 1.0f, DARKGRAY);
        } else if (p->primary == WPN_SNIPER) {
            // Long sniper barrel — tint shows ADS/super state
            Color sniperBarrelColor = DARKGRAY;
            if (p->sniper.superShotReady) sniperBarrelColor = GOLD;
            else if (p->sniper.aiming)    sniperBarrelColor = WHITE;
            Vector2 sniperTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + MINIGUN_TIP_OFFSET),
                           sa * (p->size + MINIGUN_TIP_OFFSET) });
            DrawLineEx(p->pos, sniperTip, GUN_BARREL_THICKNESS, sniperBarrelColor);
        } else {
            Vector2 gunTip = Vector2Add(p->pos,
                (Vector2){ ca * (p->size + GUN_TIP_OFFSET),
                           sa * (p->size + GUN_TIP_OFFSET) });
            DrawLineEx(p->pos, gunTip, GUN_BARREL_THICKNESS, barrelColor);
        }

#if 0 // laser beam draw — preserved for future use
        // --- Laser beam (live while key held) ---
        if (p->laser.active) {
            Vector2 aimDir = { cosf(p->angle), sinf(p->angle) };
            Vector2 muzzle = Vector2Add(p->pos,
                Vector2Scale(aimDir, p->size + MUZZLE_OFFSET));
            DrawLineEx(muzzle, p->laser.beamTip, LASER_GLOW_WIDTH, LASER_GLOW_COLOR);
            DrawLineEx(muzzle, p->laser.beamTip, LASER_BEAM_WIDTH, LASER_COLOR);
        }
#endif
    }

    // --- Beam pool (railgun lingering flash) ---
    for (int i = 0; i < MAX_BEAMS; i++) {
        Beam *b = &g.vfx.beams[i];
        if (!b->active) continue;
        float t = b->timer / b->duration;
        Color glow = { RAILGUN_GLOW_COLOR.r, RAILGUN_GLOW_COLOR.g,
                       RAILGUN_GLOW_COLOR.b, (u8)(80.0f * t) };
        Color core = { b->color.r, b->color.g, b->color.b, (u8)(255.0f * t) };
        DrawLineEx(b->origin, b->tip, RAILGUN_GLOW_WIDTH * t, glow);
        DrawLineEx(b->origin, b->tip, b->width, core);
    }

    // --- Sword swing arc ---
    if (p->sword.timer > 0 && !p->sword.lunge) {
        Color arcColor = p->sword.dashSlash ? SKYBLUE : ORANGE;
        DrawSwordArc(p->pos, p->sword.timer, p->sword.duration,
            p->sword.angle, p->sword.arc, p->sword.radius, arcColor);
    }
    // --- Sword lunge thrust ---
    if (p->sword.timer > 0 && p->sword.lunge) {
        float range = p->sword.dashSlash ?
            LUNGE_RANGE * LUNGE_DASH_RANGE_MULT : LUNGE_RANGE;
        float progress = 1.0f - (p->sword.timer / LUNGE_DURATION);
        Vector2 dir = { cosf(p->sword.angle), sinf(p->sword.angle) };
        Vector2 tip = Vector2Add(p->pos, Vector2Scale(dir, range));
        Color lungeColor = p->sword.dashSlash ? SKYBLUE : RED;

        // Cone edges
        Vector2 leftDir = { cosf(p->sword.angle - LUNGE_CONE_HALF),
                            sinf(p->sword.angle - LUNGE_CONE_HALF) };
        Vector2 rightDir = { cosf(p->sword.angle + LUNGE_CONE_HALF),
                             sinf(p->sword.angle + LUNGE_CONE_HALF) };
        Vector2 leftTip = Vector2Add(p->pos, Vector2Scale(leftDir, range));
        Vector2 rightTip = Vector2Add(p->pos, Vector2Scale(rightDir, range));

        // Trailing cone fill
        float fadeOut = p->sword.timer / LUNGE_DURATION;
        DrawTriangle(p->pos, leftTip, rightTip, Fade(lungeColor, 0.15f * fadeOut));

        // Cone edge lines
        DrawLineEx(p->pos, leftTip, 1.5f, Fade(lungeColor, 0.5f * fadeOut));
        DrawLineEx(p->pos, rightTip, 1.5f, Fade(lungeColor, 0.5f * fadeOut));

        // Leading thrust line (thick white center)
        float thick = 3.0f + progress * 4.0f;
        DrawLineEx(p->pos, tip, thick, Fade(WHITE, 0.9f * fadeOut));

        // Tip dot
        DrawCircleV(tip, 4.0f, Fade(lungeColor, fadeOut));
    }

    // --- Spin attack trailing arc ---
    if (p->spin.timer > 0) {
        float progress = 1.0f - (p->spin.timer / p->spin.duration);
        float fadeOut = p->spin.timer / p->spin.duration;

        // 2 full rotations over the spin duration
        float totalSweep = PI * 4.0f;
        float sweepAngle = totalSweep * progress;

        int numSegments = SPIN_DRAW_SEGMENTS;
        float trailLength = SPIN_TRAIL_LENGTH;
        float actualTrail = fminf(trailLength, sweepAngle);

        // Outer trailing arc (YELLOW)
        for (int i = 0; i < numSegments; i++) {
            float t = (float)(i + 1) / numSegments;
            float segAlpha = t * t * fadeOut * 0.9f;
            float thick = 2.0f + t * 5.0f;

            float a0 = sweepAngle
                - actualTrail * (1.0f - (float)i / numSegments);
            float a1 = sweepAngle
                - actualTrail * (1.0f - (float)(i + 1) / numSegments);

            Vector2 pt0 = Vector2Add(p->pos,
                (Vector2){ cosf(a0) * p->spin.radius,
                           sinf(a0) * p->spin.radius });
            Vector2 pt1 = Vector2Add(p->pos,
                (Vector2){ cosf(a1) * p->spin.radius,
                           sinf(a1) * p->spin.radius });
            DrawLineEx(pt0, pt1, thick, Fade(YELLOW, segAlpha));
        }

        // Inner trailing arc (ORANGE, smaller radius)
        for (int i = 0; i < numSegments; i++) {
            float t = (float)(i + 1) / numSegments;
            float segAlpha = t * t * fadeOut * 0.4f;
            float thick = 1.0f + t * 3.0f;

            float a0 = sweepAngle
                - actualTrail * (1.0f - (float)i / numSegments);
            float a1 = sweepAngle
                - actualTrail * (1.0f - (float)(i + 1) / numSegments);

            Vector2 pt0 = Vector2Add(p->pos,
                (Vector2){ cosf(a0) * p->spin.radius * SPIN_INNER_RADIUS_FRAC,
                           sinf(a0) * p->spin.radius * SPIN_INNER_RADIUS_FRAC });
            Vector2 pt1 = Vector2Add(p->pos,
                (Vector2){ cosf(a1) * p->spin.radius * SPIN_INNER_RADIUS_FRAC,
                           sinf(a1) * p->spin.radius * SPIN_INNER_RADIUS_FRAC });
            DrawLineEx(pt0, pt1, thick, Fade(ORANGE, segAlpha));
        }

        // White leading edge line
        Vector2 sweepEnd = Vector2Add(p->pos,
            (Vector2){ cosf(sweepAngle) * p->spin.radius,
                       sinf(sweepAngle) * p->spin.radius });
        DrawLineEx(p->pos, sweepEnd, 3.0f, Fade(WHITE, fadeOut));
    }
}

static void DrawSelect(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float ui = (float)sh / HUD_SCALE_REF;
    Player *p = &g.player;
    float t = (float)GetTime();

    WeaponType selectWeapons[] = {
        WPN_SWORD, WPN_REVOLVER, WPN_GUN, WPN_SNIPER, WPN_ROCKET
    };
    const char *names[] = { "SWORD", "REVOLVER", "MACHINE GUN", "SNIPER", "ROCKET" };
    const char *descs[] = {
        "M1 sweep, M2 lunge, dash slash",
        "M1 single, M2 fan hammer, dash reload",
        "M1 burst, M2 minigun, overheat QTE",
        "M1 hip fire, M2 ADS, dash super shot",
        "M1 rocket, M2 detonate, rocket jump",
    };

    typedef void (*DrawSolidFn)(Vector2, float, float, float, float, Vector2, float);
    DrawSolidFn solidFns[] = {
        DrawTetra2D, DrawCube2D, DrawOcta2D, DrawDodeca2D, DrawIcosa2D,
    };

    // Pedestal positions (U curve, left to right by face count)
    Vector2 pedestals[NUM_PRIMARY_WEAPONS];
    float spacing = SELECT_ARENA_SIZE / 6.0f;
    for (int i = 0; i < NUM_PRIMARY_WEAPONS; i++) {
        float norm = (float)(i - 2) / 2.0f;
        pedestals[i] = (Vector2){
            spacing * (float)(i + 1),
            SELECT_PEDESTAL_Y + SELECT_PEDESTAL_CURVE * (1.0f - norm * norm)
        };
    }

    BeginDrawing();
    ClearBackground(SELECT_BG_COLOR);

    // --- World space ---
    BeginMode2D(g.camera);

    // Arena grid
    for (float x = 0; x <= SELECT_ARENA_SIZE; x += SELECT_GRID_STEP_SEL) {
        DrawLineV((Vector2){ x, 0 }, (Vector2){ x, SELECT_ARENA_SIZE }, SELECT_GRID_COLOR_SEL);
    }
    for (float y = 0; y <= SELECT_ARENA_SIZE; y += SELECT_GRID_STEP_SEL) {
        DrawLineV((Vector2){ 0, y }, (Vector2){ SELECT_ARENA_SIZE, y }, SELECT_GRID_COLOR_SEL);
    }
    // Arena border
    DrawRectangleLinesEx((Rectangle){ 0, 0, SELECT_ARENA_SIZE, SELECT_ARENA_SIZE },
        MAP_BORDER_THICKNESS, Fade(SELECT_HIGHLIGHT_COLOR, 0.3f));

    // Particles (behind everything)
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pt = &g.vfx.particles[i];
        if (!pt->active) continue;
        float alpha = pt->lifetime / pt->maxLifetime;
        DrawCircleV(pt->pos, pt->size * alpha, Fade(pt->color, alpha));
    }

    // Projectiles
    DrawProjectiles();

    // Sword demo arc
    if (g.selectSwordTimer > 0 && g.selectIndex == 0) {
        DrawSwordArc(pedestals[0], g.selectSwordTimer, SWORD_DURATION,
            g.selectSwordAngle, SWORD_ARC, SWORD_RADIUS, ORANGE);
    }

    // Pedestals
    for (int i = 0; i < NUM_PRIMARY_WEAPONS; i++) {
        bool locked = (g.selectPhase == 1 && selectWeapons[i] == p->primary);
        bool highlighted = (i == g.selectIndex);

        // Ground ring
        float ringR = SELECT_RING_RADIUS;
        if (highlighted) {
            float pulse = 1.0f + 0.15f * sinf(t * SELECT_RING_PULSE_SPEED);
            ringR *= pulse;
            DrawCircleLines(pedestals[i].x, pedestals[i].y, ringR,
                SELECT_HIGHLIGHT_COLOR);
            DrawCircleLines(pedestals[i].x, pedestals[i].y, ringR - 2.0f,
                Fade(SELECT_HIGHLIGHT_COLOR, 0.4f));
        } else if (locked) {
            DrawCircleLines(pedestals[i].x, pedestals[i].y, ringR,
                Fade(GREEN, 0.4f));
        } else {
            DrawCircleLines(pedestals[i].x, pedestals[i].y, ringR,
                Fade(WHITE, 0.2f));
        }

        // Rotating solid
        float solidSize = highlighted ? SELECT_SOLID_SIZE * 1.2f : SELECT_SOLID_SIZE;
        float solidAlpha = locked ? 0.35f : highlighted ? 1.0f : 0.6f;
        float shadowAlpha = locked ? SHADOW_ALPHA * 0.2f :
            highlighted ? SHADOW_ALPHA : SHADOW_ALPHA * 0.3f;
        Vector2 shadowP = { pedestals[i].x + SHADOW_OFFSET_X,
                            pedestals[i].y + SHADOW_OFFSET_Y };
        solidFns[i](pedestals[i], solidSize, t * PLAYER_ROT_SPEED, PLAYER_ROT_TILT,
            solidAlpha, shadowP, shadowAlpha);
    }

    // Player
    {
        float rotY = t * PLAYER_ROT_SPEED;
        float rotX = PLAYER_ROT_TILT;
        Vector2 shadowPos = { p->shadowPos.x + SHADOW_OFFSET_X,
                              p->shadowPos.y + SHADOW_OFFSET_Y };
        if (g.selectPhase == 0) {
            DrawSphere2D(p->pos, p->size, rotY, rotX, 1.0f,
                shadowPos, SHADOW_ALPHA);
        } else {
            DrawPlayerSolid(p->pos, p->size, rotY, rotX, 1.0f,
                shadowPos, SHADOW_ALPHA);
        }
    }

    EndMode2D();

    // --- Screen space overlay ---
    int titleFont = (int)(SELECT_TITLE_FONT * ui);
    int hintFont = (int)(SELECT_HINT_FONT * ui);
    int labelFont = (int)(SELECT_LABEL_FONT * ui);
    int descFont = (int)(SELECT_DESC_FONT * ui);
    int gap = (int)(SELECT_HINT_GAP * ui);

    // Title + hint at top
    const char *title = g.selectPhase == 0
        ? "CHOOSE PRIMARY" : "CHOOSE SECONDARY";
    int titleW = MeasureText(title, titleFont);
    int titleY = (int)(sh * SELECT_TITLE_Y);
    DrawText(title, sw / 2 - titleW / 2, titleY, titleFont, WHITE);

    const char *hint = "walk to a weapon and click to select";
    int hintW = MeasureText(hint, hintFont);
    DrawText(hint, sw / 2 - hintW / 2,
        titleY + titleFont + gap, hintFont, Fade(WHITE, 0.5f));

    // Weapon name/desc at bottom
    if (g.selectIndex >= 0) {
        int idx = g.selectIndex;
        const char *desc = descs[idx];
        int descW = MeasureText(desc, descFont);
        int descY = sh - descFont - gap;
        DrawText(desc, sw / 2 - descW / 2, descY, descFont, WHITE);

        int nameW = MeasureText(names[idx], labelFont);
        int nameY = descY - labelFont - gap;
        DrawText(names[idx], sw / 2 - nameW / 2, nameY, labelFont,
            SELECT_HIGHLIGHT_COLOR);
    }

    // Locked primary label
    if (g.selectPhase == 1) {
        for (int i = 0; i < NUM_PRIMARY_WEAPONS; i++) {
            if (selectWeapons[i] != p->primary) continue;
            Vector2 screenPos = GetWorldToScreen2D(pedestals[i], g.camera);
            int lblFont = (int)(SELECT_PROMPT_FONT * ui);
            const char *lbl = "PRIMARY";
            int lblW = MeasureText(lbl, lblFont);
            DrawText(lbl, (int)screenPos.x - lblW / 2,
                (int)screenPos.y + (int)(SELECT_RING_RADIUS * 0.8f), lblFont, Fade(GREEN, 0.6f));
        }
    }

    EndDrawing();
}

static void DrawCooldownBar(int x, int y, int w, int h,
    float ratio, Color color, const char *label, int labelX, int fontSize)
{
    if (ratio < 1.0f) {
        DrawRectangle(x, y, (int)(w * ratio), h, color);
        DrawRectangleLines(x, y, w, h, GRAY);
        DrawText(label, labelX, y, fontSize, GRAY);
    } else {
        DrawRectangle(x, y, w, h, color);
        DrawRectangleLines(x, y, w, h, WHITE);
        DrawText(label, labelX, y, fontSize, color);
    }
}

// Draw - Cooldown Column ------------------------------------------------- /
static void DrawCooldownColumn(Player *p, float ui)
{
    int cdBarW = (int)(HUD_CD_W * ui);
    int cdBarH = (int)(HUD_CD_H * ui);
    int cdX = (int)(HUD_MARGIN * ui);
    int cdY = (int)(HUD_CD_Y * ui);
    int cdFontSize = (int)(HUD_CD_FONT * ui);
    int cdLabelX = cdX + cdBarW + (int)(HUD_CD_LABEL_GAP * ui);

    // Dash charges
    {
        int pipW = (int)(HUD_PIP_W * ui);
        int pipGap = (int)(HUD_PIP_GAP * ui);
        for (int i = 0; i < p->dash.maxCharges; i++) {
            int pipX = cdX + i * (pipW + pipGap);
            if (i < p->dash.charges) {
                DrawRectangle(pipX, cdY, pipW, cdBarH, SKYBLUE);
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, WHITE);
            } else if (i == p->dash.charges
                && p->dash.charges < p->dash.maxCharges) {
                float ratio = 1.0f
                    - (p->dash.rechargeTimer / p->dash.rechargeTime);
                DrawRectangle(pipX, cdY, (int)(pipW * ratio), cdBarH,
                    Fade(SKYBLUE, 0.4f));
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            } else {
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            }
        }
        int labelX = cdX
            + p->dash.maxCharges * (pipW + pipGap) + (int)(2 * ui);
        DrawText("DASH", labelX, cdY, cdFontSize,
            p->dash.charges > 0 ? SKYBLUE : GRAY);
    }

    // Shotgun blasts (pip display like dash)
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        int pipW = (int)(HUD_PIP_W * ui);
        int pipGap = (int)(HUD_PIP_GAP * ui);
        for (int i = 0; i < SHOTGUN_BLASTS; i++) {
            int pipX = cdX + i * (pipW + pipGap);
            if (i < p->shotgun.blastsLeft) {
                DrawRectangle(pipX, cdY, pipW, cdBarH, ORANGE);
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, WHITE);
            } else if (p->shotgun.blastsLeft == 0
                && p->shotgun.cooldownTimer > 0) {
                float ratio = 1.0f
                    - (p->shotgun.cooldownTimer / SHOTGUN_COOLDOWN);
                DrawRectangle(pipX, cdY, (int)(pipW * ratio), cdBarH,
                    Fade(ORANGE, 0.4f));
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            } else {
                DrawRectangleLines(pipX, cdY, pipW, cdBarH, GRAY);
            }
        }
        int labelX = cdX
            + SHOTGUN_BLASTS * (pipW + pipGap) + (int)(2 * ui);
        DrawText("SHTGN", labelX, cdY, cdFontSize,
            p->shotgun.blastsLeft > 0 ? ORANGE : GRAY);
    }


    // rockets
    cdY += (int)(HUD_ROW_SPACING * ui);
    DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
        1.0f - p->rocket.cooldownTimer / p->rocket.cooldown,
        RED, "ROCKET", cdLabelX, cdFontSize);

    // Railgun cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
        1.0f - p->railgun.cooldownTimer / RAILGUN_COOLDOWN,
        RAILGUN_COLOR, "RAIL", cdLabelX, cdFontSize);

    // Sniper cooldown (when sniper is equipped)
    if (p->primary == WPN_SNIPER || p->secondary == WPN_SNIPER) {
        cdY += (int)(HUD_ROW_SPACING * ui);
        const char *snLabel;
        Color snColor;
        float snCooldown;
        if (p->sniper.superShotReady) {
            snLabel = "SUPER"; snColor = GOLD; snCooldown = SNIPER_AIM_COOLDOWN;
        } else if (p->sniper.aiming) {
            snLabel = "AIM"; snColor = WHITE; snCooldown = SNIPER_AIM_COOLDOWN;
        } else {
            snLabel = "HIP"; snColor = SNIPER_COLOR; snCooldown = SNIPER_HIP_COOLDOWN;
        }
        float snRatio = 1.0f - p->sniper.cooldownTimer / snCooldown;
        if (snRatio > 1.0f) snRatio = 1.0f;
        DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
            snRatio, snColor, snLabel, cdLabelX, cdFontSize);
    }

    // Grenade cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
        1.0f - p->grenade.cooldownTimer / GRENADE_COOLDOWN,
        GRENADE_COLOR, "GREN", cdLabelX, cdFontSize);

    // BFG cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color bfgColor = BFG_COLOR;
        float chargeRatio = p->bfg.charge / BFG_CHARGE_COST;
        if (chargeRatio < 1.0f || p->bfg.active) {
            DrawRectangle(cdX, cdY, (int)(cdBarW * chargeRatio), cdBarH, bfgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("BFG", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, bfgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("BFG", cdLabelX, cdY, cdFontSize, bfgColor);
        }
    }

    // Shield HP bar
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color shColor = (Color)SHIELD_COLOR;
        float hpRatio = p->shield.hp / p->shield.maxHp;
        bool broken = p->shield.regenTimer < 0;
        if (broken) {
            // Show broken cooldown
            float cd = -p->shield.regenTimer / SHIELD_BROKEN_COOLDOWN;
            DrawRectangle(cdX, cdY,
                (int)(cdBarW * (1.0f - cd)), cdBarH,
                Fade(RED, 0.6f));
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("SHLD", cdLabelX, cdY, cdFontSize, RED);
        } else if (hpRatio < 1.0f) {
            DrawRectangle(cdX, cdY,
                (int)(cdBarW * hpRatio), cdBarH, shColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("SHLD", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, shColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("SHLD", cdLabelX, cdY, cdFontSize, shColor);
        }
    }

    // Slam cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
        1.0f - p->slam.cooldownTimer / SLAM_COOLDOWN,
        SLAM_COLOR, "SLAM", cdLabelX, cdFontSize);

    // Parry cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color paColor = PARRY_COLOR;
        if (p->parry.active) {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("PARRY", cdLabelX, cdY, cdFontSize, WHITE);
        } else if (p->parry.cooldownTimer > 0) {
            float maxCd = p->parry.succeeded
                ? PARRY_SUCCESS_COOLDOWN : PARRY_COOLDOWN;
            float ratio = 1.0f - (p->parry.cooldownTimer / maxCd);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, paColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("PARRY", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, paColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("PARRY", cdLabelX, cdY, cdFontSize, paColor);
        }
    }

    // Turret cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
        1.0f - p->turretCooldown / TURRET_COOLDOWN,
        TURRET_COLOR, "TURRT", cdLabelX, cdFontSize);

    // Mine cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
        1.0f - p->mineCooldown / MINE_COOLDOWN,
        MINE_COLOR, "MINE", cdLabelX, cdFontSize);

    // Heal cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    DrawCooldownBar(cdX, cdY, cdBarW, cdBarH,
        1.0f - p->healCooldown / HEAL_COOLDOWN,
        HEAL_COLOR, "HEAL", cdLabelX, cdFontSize);

    // Fuel bar
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color fiColor = FLAME_COLOR;
        float ratio = p->flame.fuel / FLAME_FUEL_MAX;
        DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, fiColor);
        DrawRectangleLines(cdX, cdY, cdBarW, cdBarH,
            p->flame.active ? WHITE : GRAY);
        DrawText("FUEL", cdLabelX, cdY, cdFontSize,
            (p->flame.fuel > 0) ? fiColor : RED);
    }
}

// Draw - Weapon Status --------------------------------------------------- /
static void DrawWeaponStatus(Player *p, float ui)
{
    Vector2 pScreen = GetWorldToScreen2D(p->pos, g.camera);
    pScreen.y += HUD_ARC_Y_OFFSET * ui;
    float arcInner  = HUD_ARC_INNER_R * ui;
    float arcOuter  = HUD_ARC_OUTER_R * ui;
    float arcStart  = HUD_ARC_START_DEG;
    float arcEnd    = HUD_ARC_END_DEG;
    float arcSpan   = arcEnd - arcStart;
    int   arcSegs   = HUD_ARC_SEGMENTS;
    int   arcFont   = (int)(HUD_ARC_LABEL_FONT * ui);
    float arcCurPad = HUD_ARC_CURSOR_PAD * ui;

    // Revolver arc (always visible for revolver players)
    if (p->primary == WPN_REVOLVER) {
        Color revColor = (Color){ 220, 180, 80, 255 };
        Color bonusColor = GOLD;
        float segGap = HUD_ARC_SEG_GAP;
        float segAngle = (arcSpan
            - (REVOLVER_ROUNDS - 1) * segGap) / REVOLVER_ROUNDS;

        // 6 round segments
        for (int i = 0; i < REVOLVER_ROUNDS; i++) {
            float sEnd = arcEnd - i * (segAngle + segGap);
            float sStart = sEnd - segAngle;
            bool loaded = i < p->revolver.rounds;
            bool isBonus = i < p->revolver.bonusRounds;

            if (loaded) {
                // Bonus glow
                if (isBonus) {
                    float pulse = 0.3f
                        + 0.2f * sinf(GetTime() * 8.0f);
                    DrawRing(pScreen,
                        arcInner - arcCurPad,
                        arcOuter + arcCurPad,
                        sStart, sEnd, arcSegs,
                        Fade(bonusColor, pulse));
                }
                DrawRing(pScreen, arcInner, arcOuter,
                    sStart, sEnd, arcSegs,
                    isBonus ? bonusColor : revColor);
            } else {
                DrawRing(pScreen, arcInner, arcOuter,
                    sStart, sEnd, arcSegs,
                    (Color){ 40, 40, 40, 160 });
            }
        }

        // Reload overlay
        if (p->revolver.reloadTimer > 0) {
            float ratio = 1.0f
                - (p->revolver.reloadTimer
                    / REVOLVER_RELOAD_TIME);
            if (ratio > 1.0f) ratio = 1.0f;
            if (ratio < 0.0f) ratio = 0.0f;

            // Sweet spot zone
            float sweetSA = arcEnd
                - REVOLVER_RELOAD_SWEET_END * arcSpan;
            float sweetEA = arcEnd
                - REVOLVER_RELOAD_SWEET_START * arcSpan;
            Color sweetColor;
            if (p->revolver.reloadLocked) {
                bool hit = p->revolver.reloadTimer
                    <= REVOLVER_RELOAD_FAST_TIME + 0.01f;
                sweetColor = hit
                    ? (Color){ 60, 255, 60, 120 }
                    : (Color){ 255, 60, 60, 120 };
            } else {
                sweetColor = (Color){ 255, 255, 100, 100 };
            }
            DrawRing(pScreen, arcInner, arcOuter,
                sweetSA, sweetEA, arcSegs, sweetColor);

            // Progress sweep
            float progressA = arcEnd - ratio * arcSpan;
            DrawRing(pScreen, arcInner, arcOuter,
                progressA, arcEnd, arcSegs,
                Fade(SKYBLUE, 0.3f));

            // Cursor line
            float curRad = progressA * DEG2RAD;
            Vector2 c1 = {
                pScreen.x + (arcInner - arcCurPad)
                    * cosf(curRad),
                pScreen.y + (arcInner - arcCurPad)
                    * sinf(curRad) };
            Vector2 c2 = {
                pScreen.x + (arcOuter + arcCurPad)
                    * cosf(curRad),
                pScreen.y + (arcOuter + arcCurPad)
                    * sinf(curRad) };
            DrawLineEx(c1, c2, 2.0f * ui, WHITE);

            // Label
            const char *rlText = "RELOAD";
            int rlW = MeasureText(rlText, arcFont);
            DrawText(rlText,
                (int)pScreen.x - rlW / 2,
                (int)(pScreen.y + arcOuter
                    + HUD_ARC_LABEL_PAD * ui),
                arcFont, GRAY);
        }
    }

    // Gun heat arc (always visible for gun players)
    if (p->primary == WPN_GUN) {
        float h = p->gun.heat;

        // Background arc (always present)
        DrawRing(pScreen, arcInner, arcOuter,
            arcStart, arcEnd, arcSegs, (Color){ 40, 40, 40, 160 });

        // Heat fill — color gradient WHITE -> YELLOW -> RED
        if (h > 0) {
            Color heatColor;
            if (h < 0.5f) {
                float t = h * 2.0f;
                heatColor = (Color){
                    255, 255,
                    (u8)(255 - (int)(255 * t)), 255 };
            } else {
                float t = (h - 0.5f) * 2.0f;
                heatColor = (Color){
                    255, (u8)(255 - (int)(255 * t)),
                    0, 255 };
            }
            float heatA = arcEnd - h * arcSpan;
            DrawRing(pScreen, arcInner, arcOuter,
                heatA, arcEnd, arcSegs, Fade(heatColor, 0.6f));
        }

        // Overheat QTE overlay
        if (p->gun.overheated) {
            // Sweet spot zone
            float zSA = arcEnd
                - (p->gun.ventZoneStart + p->gun.ventZoneWidth)
                * arcSpan;
            float zEA = arcEnd
                - p->gun.ventZoneStart * arcSpan;
            Color zoneColor = (p->gun.ventResult == 1) ? GREEN
                : (p->gun.ventResult == -1)
                    ? (Color){ 80, 20, 20, 255 } : YELLOW;
            DrawRing(pScreen, arcInner, arcOuter,
                zSA, zEA, arcSegs, zoneColor);

            // Cursor line (only while pending)
            if (p->gun.ventResult == 0) {
                float curA = arcEnd
                    - p->gun.ventCursor * arcSpan;
                float curRad = curA * DEG2RAD;
                Vector2 c1 = {
                    pScreen.x + (arcInner - arcCurPad)
                        * cosf(curRad),
                    pScreen.y + (arcInner - arcCurPad)
                        * sinf(curRad) };
                Vector2 c2 = {
                    pScreen.x + (arcOuter + arcCurPad)
                        * cosf(curRad),
                    pScreen.y + (arcOuter + arcCurPad)
                        * sinf(curRad) };
                DrawLineEx(c1, c2, 2.0f * ui, WHITE);
            }

            // QTE label
            const char *ovLabel =
                (p->gun.ventResult == 1) ? "VENT"
                : (p->gun.ventResult == -1) ? "OVHT" : "[R]";
            Color ovColor = (p->gun.ventResult == 1) ? GREEN
                : (p->gun.ventResult == -1) ? RED : YELLOW;
            int ovW = MeasureText(ovLabel, arcFont);
            DrawText(ovLabel,
                (int)pScreen.x - ovW / 2,
                (int)(pScreen.y + arcOuter
                    + HUD_ARC_LABEL_PAD * ui),
                arcFont, ovColor);
        }
    }
}

// Draw - Pause Menu ------------------------------------------------------ /
static void DrawPauseMenu(int sw, int sh, float ui)
{
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.5f));
    int pauseFont = (int)(HUD_PAUSE_FONT * ui);
    const char *pauseText = "PAUSED";
    int pW = MeasureText(pauseText, pauseFont);
    DrawText(
        pauseText,
        sw / 2 - pW / 2,
        sh / 2 - (int)(HUD_PAUSE_Y * ui),
        pauseFont,
        WHITE);
    int resumeFont = (int)(HUD_RESUME_FONT * ui);
    const char *resumeText = "P or Esc to resume";
    int rW = MeasureText(resumeText, resumeFont);
    DrawText(
        resumeText,
        sw / 2 - rW / 2,
        sh / 2 + (int)(HUD_RESUME_Y * ui),
        resumeFont,
        LIGHTGRAY);

    // Keybinds — three columns
    int pkFont = (int)(HUD_PAUSE_KEYS_FONT * ui);
    int pkSpacing = (int)(HUD_PAUSE_KEYS_SPACING * ui);
    int pkY = sh / 2 + (int)(HUD_PAUSE_KEYS_Y * ui);
    int pkTabW = (int)(62 * ui);

    // Left column: core controls
    const char *lKeys[] = {
        "WASD", "Space", "M1", "M2", "Ctrl", "P/Esc", "0"
    };
    const char *lDescs[] = {
        "Move", "Dash", "Primary", "Alt Fire",
        "Swap", "Pause", "Exit"
    };
    int lCount = 7;

    int totalW = (int)(420 * ui);
    int colW = totalW / 3;
    int lColX = sw / 2 - totalW / 2;
    int mColX = lColX + colW;
    int rColX = mColX + colW;

    for (int i = 0; i < lCount; i++) {
        int ky = pkY + i * pkSpacing;
        DrawText(lKeys[i], lColX, ky, pkFont, SELECT_HIGHLIGHT_COLOR);
        DrawText(lDescs[i], lColX + pkTabW, ky, pkFont, Fade(WHITE, 0.6f));
    }

    // Middle + Right columns: abilities from slots (split in half)
    Player *p = &g.player;
    int ablCount = 0;
    int ablSlots[ABILITY_SLOTS];
    for (int i = 0; i < ABILITY_SLOTS; i++) {
        if (AbilityName(p->slots[i].ability))
            ablSlots[ablCount++] = i;
    }
    int half = (ablCount + 1) / 2;
    for (int a = 0; a < ablCount; a++) {
        int si = ablSlots[a];
        int colX = (a < half) ? mColX : rColX;
        int row = (a < half) ? a : a - half;
        int ky = pkY + row * pkSpacing;
        DrawText(KeyName(p->slots[si].key), colX, ky, pkFont, SELECT_HIGHLIGHT_COLOR);
        DrawText(AbilityName(p->slots[si].ability),
            colX + pkTabW, ky, pkFont, Fade(WHITE, 0.6f));
    }

    // Fullscreen toggle
    int maxRows = lCount > half ? lCount : half;
    int fsY = pkY + (maxRows + 1) * pkSpacing;
    const char *fsLabel = IsWindowFullscreen() ? "[F] Fullscreen: ON" : "[F] Fullscreen: OFF";
    int fsW = MeasureText(fsLabel, pkFont);
    DrawText(fsLabel, sw / 2 - fsW / 2, fsY, pkFont, Fade(WHITE, 0.6f));
}

// Draw - Game Over ------------------------------------------------------- /
static void DrawGameOver(int sw, int sh, float ui)
{
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.7f));
    int goFont = (int)(HUD_GO_FONT * ui);
    const char *goText = "GAME OVER";
    int goW = MeasureText(goText, goFont);
    DrawText(
        goText,
        sw / 2 - goW / 2,
        sh / 2 - (int)(HUD_GO_Y * ui),
        goFont,
        RED);

    int scFont = (int)(HUD_GO_SCORE_FONT * ui);
    const char *scoreText =
        TextFormat("Score: %d  |  Kills: %d", g.score, g.enemiesKilled);
    int sW = MeasureText(scoreText, scFont);
    DrawText(
        scoreText,
        sw / 2 - sW / 2,
        sh / 2 + (int)(HUD_GO_SCORE_Y * ui),
        scFont,
        WHITE);

    int rsFont = (int)(HUD_GO_RESTART_FONT * ui);
    const char *restartText = "Press ENTER to restart";
    int rW = MeasureText(restartText, rsFont);
    DrawText(
        restartText,
        sw / 2 - rW / 2,
        sh / 2 + (int)(HUD_GO_RESTART_Y * ui),
        rsFont,
        LIGHTGRAY);
}

// Draw - HUD (screen space)
static void DrawHUD(void)
{
    Player *p = &g.player;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float ui = (float)sh / HUD_SCALE_REF;

    // HP bar
    int hpBarW = (int)(HUD_HP_W * ui);
    int hpBarH = (int)(HUD_HP_H * ui);
    int hpBarX = (int)(HUD_MARGIN * ui);
    int hpBarY = (int)(HUD_MARGIN * ui);
    float hpRatio = (float)p->hp / p->maxHp;
    DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, DARKGRAY);
    Color hpColor = (hpRatio > 0.5f) ? GREEN : (hpRatio > 0.25f) ? ORANGE : RED;
    DrawRectangle(hpBarX, hpBarY, (int)(hpBarW * hpRatio), hpBarH, hpColor);
    DrawRectangleLines(hpBarX, hpBarY, hpBarW, hpBarH, WHITE);
    DrawText(
        TextFormat("HP: %d/%d", (int)p->hp, (int)p->maxHp),
        hpBarX + (int)(HUD_HP_TEXT_PAD_X * ui), hpBarY + (int)(HUD_HP_TEXT_PAD_Y * ui), (int)(HUD_HP_FONT * ui), WHITE);

    // Score
    DrawText(
        TextFormat("Score: %d", g.score), 
        (int)(HUD_MARGIN * ui), (int)(HUD_SCORE_Y * ui), (int)(HUD_SCORE_FONT * ui), WHITE);
    DrawText(
        TextFormat("Kills: %d", g.enemiesKilled),
        (int)(HUD_MARGIN * ui), (int)(HUD_KILLS_Y * ui), (int)(HUD_KILLS_FONT * ui), LIGHTGRAY);
    DrawText(
        TextFormat("Level: %d", g.level),
        (int)(HUD_MARGIN * ui), (int)(HUD_LEVEL_Y * ui), (int)(HUD_KILLS_FONT * ui), LIGHTGRAY);

    // Weapon swap indicator
    {
        int wFont = (int)(HUD_CD_FONT * ui);
        int wX = (int)(HUD_MARGIN * ui);
        int wY = (int)(HUD_CD_Y * ui) - (int)(20 * ui);
        const char *pri = WeaponName(p->primary);
        const char *sec = WeaponName(p->secondary);
        DrawText(pri, wX, wY, wFont, SELECT_HIGHLIGHT_COLOR);
        int secX = wX + MeasureText(pri, wFont) + (int)(6 * ui);
        DrawText(TextFormat("/ %s", sec), secX, wY, wFont,
            Fade(WHITE, 0.35f));
        int ctrlX = secX + MeasureText(TextFormat("/ %s", sec), wFont)
            + (int)(6 * ui);
        DrawText("[Ctrl]", ctrlX, wY, wFont, Fade(WHITE, 0.2f));
    }

    DrawCooldownColumn(p, ui);

    DrawWeaponStatus(p, ui);

    // FPS (top-right)
    int fpsFont = (int)(HUD_FPS_FONT * ui);
    DrawText(TextFormat("%d FPS",
        GetFPS()), sw - (int)(HUD_FPS_X * ui), (int)(HUD_MARGIN * ui), fpsFont, GREEN);

    // Crosshair cursor
    Vector2 mouse = GetMousePosition();
    // we can refactor this crosshair to default
    // eventually make it editable
    float ch = HUD_CROSSHAIR_SIZE * ui;
    float chThick = HUD_CROSSHAIR_THICKNESS * ui;
    float chGap = HUD_CROSSHAIR_GAP * ui;
    //Color chColor = Fade(WHITE, 0.8f);
    Color chColor = Fade(GREEN, 1.0f);
    // we're going to create a gap between in the middle of the cross hair
    // so we need 4 lines
    // middle left
    //DrawLineEx(
    //    (Vector2){ mouse.x - ch, mouse.y }, 
    //    (Vector2){ mouse.x + ch, mouse.y }, 
    //    chThick, chColor);
    DrawLineEx(
        (Vector2){ mouse.x - chGap, mouse.y },
        (Vector2){ mouse.x - ch - chGap, mouse.y },
        chThick, chColor
    );
    // middle right
    DrawLineEx(
        (Vector2){ mouse.x + chGap, mouse.y },
        (Vector2){ mouse.x + ch + chGap, mouse.y },
        chThick, chColor
    );
    // top
    //DrawLineEx(
    //    (Vector2){ mouse.x, mouse.y - ch }, 
    //    (Vector2){ mouse.x, mouse.y + ch }, 
    //    chThick, chColor);
    DrawLineEx(
        (Vector2){ mouse.x, mouse.y - chGap},
        (Vector2){ mouse.x, mouse.y - ch - chGap},
        chThick, chColor
    );
    // bottom
    DrawLineEx(
        (Vector2){ mouse.x, mouse.y + ch + chGap},
        (Vector2){ mouse.x, mouse.y + chGap},
        chThick, chColor
    );
    //DrawCircleLines((int)mouse.x, (int)mouse.y, 6.0f * ui, chColor);
    
    // bottom right, game title text
    int titleFont = (int)(HUD_TITLE_FONT * ui);
    DrawText("Untitled Mecha Game - Version 0.0.1",
             sw - (int)(HUD_TITLE_X * ui),
             sh - (int)(HUD_BOTTOM_Y * ui),
             titleFont,
             Fade(WHITE, 0.8f));

    if (g.paused && !g.gameOver) DrawPauseMenu(sw, sh, ui);

    if (g.gameOver) DrawGameOver(sw, sh, ui);
}

// Draw - orchestrator
static void DrawGame(void)
{
    if (g.screen == SCREEN_SELECT) {
        DrawSelect();
        return;
    }

    BeginDrawing();
    ClearBackground(BG_COLOR);

    BeginMode2D(g.camera);
    DrawWorld();
    EndMode2D();

    DrawHUD();

    EndDrawing();
}

// Main loop callback
void NextFrame(void)
{
    if (g.screen == SCREEN_PLAYING && !IsCursorHidden()) HideCursor();
    if (g.screen != SCREEN_PLAYING &&  IsCursorHidden()) ShowCursor();
    UpdateGame();
    DrawGame();
}
