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

static void DrawTetra2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size / 1.73f; // normalize: vertices at (±s,±s,±s) → distance = s*√3

    // 4 tetrahedron vertices in local 3D space (regular tetrahedron)
    float vtx[4][3] = {
        { s,  s,  s},
        { s, -s, -s},
        {-s,  s, -s},
        {-s, -s,  s},
    };

    // 4 triangular faces of tetrahedron
    int faces[4][3] = {
        {0, 1, 2},
        {0, 2, 3},
        {0, 3, 1},
        {1, 3, 2},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[4];
        for (int i = 0; i < 4; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 4; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]], c = sp[faces[f][2]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) DrawTriangle(a, b, c, scol);
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[4];
    float pz[4];
    float hues[4];
    float time = (float)GetTime();

    for (int i = 0; i < 4; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        // Hue based on Z depth — smooth across rotation, no jumps on face swap
        hues[i] = time * SHAPE_HUE_SPEED + pz[i] * (SHAPE_HUE_DEPTH_SCALE / s);
    }

    float faceZ[4];
    bool faceVis[4];
    for (int f = 0; f < 4; f++) {
        faceZ[f] = (pz[faces[f][0]] + pz[faces[f][1]] + pz[faces[f][2]]) / 3.0f;
        
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        // 2D Cross Product for backface culling
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    // Sort back-to-front (Painter's algorithm)
    int order[4] = {0, 1, 2, 3};
    for (int i = 0; i < 3; i++)
        for (int j = i + 1; j < 4; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = SHAPE_SUBDIV_TRI;

    for (int fi = 0; fi < 4; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]];
        Vector2 p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]];
        
        float h0 = hues[faces[f][0]];
        float h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]];

        // Subdivide triangle using barycentric coordinates
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
                
                // Triangle vertices using barycentric interpolation
                Vector2 q0 = {
                    w0 * p0.x + u0 * p1.x + v0 * p2.x,
                    w0 * p0.y + u0 * p1.y + v0 * p2.y
                };
                Vector2 q1 = {
                    w1 * p0.x + u1 * p1.x + v1 * p2.x,
                    w1 * p0.y + u1 * p1.y + v1 * p2.y
                };
                Vector2 q2 = {
                    w2 * p0.x + u2 * p1.x + v2 * p2.x,
                    w2 * p0.y + u2 * p1.y + v2 * p2.y
                };
                
                // Interpolate hue
                float hC = fmodf(w0 * h0 + u0 * h1 + v0 * h2 + 360.0f, 360.0f);
                Color cc = HsvToRgb(hC, 1.0f, 1.0f, alpha);

                DrawTriangle(q0, q1, q2, cc);

                // Draw second triangle in quad if not at edge
                if (col + 1 < N - row) {
                    float u3 = (float)(col + 1) / N;
                    float v3 = (float)(row + 1) / N;
                    float w3 = 1.0f - u3 - v3;

                    Vector2 q3 = {
                        w3 * p0.x + u3 * p1.x + v3 * p2.x,
                        w3 * p0.y + u3 * p1.y + v3 * p2.y
                    };

                    float hC2 = fmodf(w3 * h0 + u3 * h1 + v3 * h2 + 360.0f, 360.0f);
                    Color cc2 = HsvToRgb(hC2, 1.0f, 1.0f, alpha);
                    
                    DrawTriangle(q1, q3, q2, cc2);
                }
            }
        }
        
        // Draw edges
        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p0, edge);
    }


}

static void DrawCube2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size / 1.73f; // normalize: vertices at (±s,±s,±s) → distance = s*√3

    // 8 cube vertices in local 3D space
    float vtx[8][3] = {
        {-s,-s,-s}, { s,-s,-s}, { s, s,-s}, {-s, s,-s},
        {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s},
    };

    // 6 faces: {0,3,2,1} is Front, {4,5,6,7} is Back
    int faces[6][4] = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3},
        {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[8];
        for (int i = 0; i < 8; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 6; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]];
            Vector2 c = sp[faces[f][2]], d = sp[faces[f][3]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) {
                DrawTriangle(a, b, c, scol);
                DrawTriangle(a, c, d, scol);
            }
        }
    }

    // 4 tetrahedron vertices in local 3D space
    // around origin
    //float vtx[4][3] = {
    //    {0, 0, s},
    //    {s, -s, -s}, {-s, -s, -s}, {0, s, -s},
    //};

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[8];
    float pz[8];
    float hues[8];
    float time = (float)GetTime();

    for (int i = 0; i < 8; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        // Generate hue offset for "rainbow" look
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * CUBE_HUE_VERTEX_STEP;
    }

    float faceZ[6];
    bool faceVis[6];
    for (int f = 0; f < 6; f++) {
        faceZ[f] = 0;
        for (int vi = 0; vi < 4; vi++) faceZ[f] += pz[faces[f][vi]];
        faceZ[f] *= 0.25f;
        
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        // 2D Cross Product: (b-a)x(c-a)
        // Note: Screen Y is down. Standard CCW winding produces Negative cross product.
        // We want to show faces facing the camera (Front).
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    // Sort back-to-front (Painter's algorithm)
    // Smallest Z is "Far" in this projection? Actually standard 3D: Z+ is near?
    // Let's sort simply by Z value.
    int order[6] = {0, 1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
        for (int j = i + 1; j < 6; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) { // Swap if i is 'closer' than j?
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = SHAPE_SUBDIV_TRI;

    for (int fi = 0; fi < 6; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]], p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]], p3 = pj[faces[f][3]];
        float h0 = hues[faces[f][0]], h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]], h3 = hues[faces[f][3]];

        for (int gy = 0; gy < N; gy++) {
            for (int gx = 0; gx < N; gx++) {
                float u0 = (float)gx / N, u1 = (float)(gx + 1) / N;
                float v0 = (float)gy / N, v1 = (float)(gy + 1) / N;

                #define BILERP(A,B,C,D,u,v) \
                    ((1-(v))*((1-(u))*(A) + (u)*(B)) + (v)*((1-(u))*(D) + (u)*(C)))

                Vector2 q00 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u0,v0), 
                    BILERP(p0.y,p1.y,p2.y,p3.y,u0,v0) 
                };
                Vector2 q10 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u1,v0), 
                    BILERP(p0.y,p1.y,p2.y,p3.y,u1,v0) 
                };
                Vector2 q11 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u1,v1),
                    BILERP(p0.y,p1.y,p2.y,p3.y,u1,v1)
                };
                Vector2 q01 = {
                    BILERP(p0.x,p1.x,p2.x,p3.x,u0,v1), 
                    BILERP(p0.y,p1.y,p2.y,p3.y,u0,v1)
                };

                float uc = (u0 + u1) * 0.5f, vc = (v0 + v1) * 0.5f;
                float hC = BILERP(h0, h1, h2, h3, uc, vc);
                
                Color cc = HsvToRgb(hC, SHAPE_SAT_CUBE, 1.0f, alpha);
                DrawTriangle(q00, q10, q11, cc);
                DrawTriangle(q00, q11, q01, cc);

                #undef BILERP
            }
        }
        
        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p3, edge);
        DrawLineV(p3, p0, edge);
    }
}

// octahedron -------------------------------------------------------------- /
static void DrawOcta2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float s = size;

    // 6 octahedron vertices: axis-aligned
    float vtx[6][3] = {
        { s, 0, 0}, {-s, 0, 0},
        { 0, s, 0}, { 0,-s, 0},
        { 0, 0, s}, { 0, 0,-s},
    };

    // 8 triangular faces
    int faces[8][3] = {
        {0, 2, 4}, {2, 1, 4}, {1, 3, 4}, {3, 0, 4},
        {2, 0, 5}, {1, 2, 5}, {3, 1, 5}, {0, 3, 5},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[6];
        for (int i = 0; i < 6; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 8; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]], c = sp[faces[f][2]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) DrawTriangle(a, b, c, scol);
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[6];
    float pz[6];
    float hues[6];
    float time = (float)GetTime();

    for (int i = 0; i < 6; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * OCTA_HUE_VERTEX_STEP;
    }

    float faceZ[8];
    bool faceVis[8];
    for (int f = 0; f < 8; f++) {
        faceZ[f] = (pz[faces[f][0]] + pz[faces[f][1]] + pz[faces[f][2]]) / 3.0f;
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    int order[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int i = 0; i < 7; i++)
        for (int j = i + 1; j < 8; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = SHAPE_SUBDIV_TRI;

    for (int fi = 0; fi < 8; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]];
        Vector2 p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]];
        float h0 = hues[faces[f][0]];
        float h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]];

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

                Vector2 q0 = {
                    w0 * p0.x + u0 * p1.x + v0 * p2.x,
                    w0 * p0.y + u0 * p1.y + v0 * p2.y
                };
                Vector2 q1 = {
                    w1 * p0.x + u1 * p1.x + v1 * p2.x,
                    w1 * p0.y + u1 * p1.y + v1 * p2.y
                };
                Vector2 q2 = {
                    w2 * p0.x + u2 * p1.x + v2 * p2.x,
                    w2 * p0.y + u2 * p1.y + v2 * p2.y
                };

                float hC = fmodf(w0 * h0 + u0 * h1 + v0 * h2 + 360.0f, 360.0f);
                Color cc = HsvToRgb(hC, 1.0f, 1.0f, alpha);
                DrawTriangle(q0, q1, q2, cc);

                if (col + 1 < N - row) {
                    float u3 = (float)(col + 1) / N;
                    float v3 = (float)(row + 1) / N;
                    float w3 = 1.0f - u3 - v3;
                    Vector2 q3 = {
                        w3 * p0.x + u3 * p1.x + v3 * p2.x,
                        w3 * p0.y + u3 * p1.y + v3 * p2.y
                    };
                    float hC2 = fmodf(w3 * h0 + u3 * h1 + v3 * h2 + 360.0f, 360.0f);
                    Color cc2 = HsvToRgb(hC2, 1.0f, 1.0f, alpha);
                    DrawTriangle(q1, q3, q2, cc2);
                }
            }
        }

        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p0, edge);
    }
}

// icosahedron ------------------------------------------------------------- /
static void DrawIcosa2D(
    Vector2 pos,
    float size, float rotY, float rotX, float alpha,
    Vector2 shadowPos, float shadowAlpha)
{
    float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    float sc = size / phi; // scale so max extent ≈ size

    // 12 icosahedron vertices: (0, ±1, ±φ) and permutations
    float vtx[12][3] = {
        { 0,  sc,  sc*phi}, { 0,  sc, -sc*phi},
        { 0, -sc,  sc*phi}, { 0, -sc, -sc*phi},
        { sc,  sc*phi, 0}, {-sc,  sc*phi, 0},
        { sc, -sc*phi, 0}, {-sc, -sc*phi, 0},
        { sc*phi, 0,  sc}, { sc*phi, 0, -sc},
        {-sc*phi, 0,  sc}, {-sc*phi, 0, -sc},
    };

    // 20 triangular faces
    int faces[20][3] = {
        {0, 2, 8},  {0, 8, 4},  {0, 4, 5},  {0, 5, 10}, {0, 10, 2},
        {2, 6, 8},  {8, 6, 9},  {8, 9, 4},  {4, 9, 1},  {4, 1, 5},
        {5, 1, 11}, {5, 11, 10},{10, 11, 7}, {10, 7, 2}, {2, 7, 6},
        {3, 9, 6},  {3, 1, 9},  {3, 11, 1}, {3, 7, 11}, {3, 6, 7},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[12];
        for (int i = 0; i < 12; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 20; f++) {
            Vector2 a = sp[faces[f][0]], b = sp[faces[f][1]], c = sp[faces[f][2]];
            float cross = (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
            if (cross < 0) DrawTriangle(a, b, c, scol);
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[12];
    float pz[12];
    float hues[12];
    float time = (float)GetTime();

    for (int i = 0; i < 12; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * ICOSA_HUE_VERTEX_STEP;
    }

    float faceZ[20];
    bool faceVis[20];
    for (int f = 0; f < 20; f++) {
        faceZ[f] = (pz[faces[f][0]] + pz[faces[f][1]] + pz[faces[f][2]]) / 3.0f;
        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    int order[20];
    for (int i = 0; i < 20; i++) order[i] = i;
    for (int i = 0; i < 19; i++)
        for (int j = i + 1; j < 20; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    int N = SHAPE_SUBDIV_PENTA;

    for (int fi = 0; fi < 20; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        Vector2 p0 = pj[faces[f][0]];
        Vector2 p1 = pj[faces[f][1]];
        Vector2 p2 = pj[faces[f][2]];
        float h0 = hues[faces[f][0]];
        float h1 = hues[faces[f][1]];
        float h2 = hues[faces[f][2]];

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

                Vector2 q0 = {
                    w0 * p0.x + u0 * p1.x + v0 * p2.x,
                    w0 * p0.y + u0 * p1.y + v0 * p2.y
                };
                Vector2 q1 = {
                    w1 * p0.x + u1 * p1.x + v1 * p2.x,
                    w1 * p0.y + u1 * p1.y + v1 * p2.y
                };
                Vector2 q2 = {
                    w2 * p0.x + u2 * p1.x + v2 * p2.x,
                    w2 * p0.y + u2 * p1.y + v2 * p2.y
                };

                float hC = fmodf(w0 * h0 + u0 * h1 + v0 * h2 + 360.0f, 360.0f);
                Color cc = HsvToRgb(hC, 1.0f, 1.0f, alpha);
                DrawTriangle(q0, q1, q2, cc);

                if (col + 1 < N - row) {
                    float u3 = (float)(col + 1) / N;
                    float v3 = (float)(row + 1) / N;
                    float w3 = 1.0f - u3 - v3;
                    Vector2 q3 = {
                        w3 * p0.x + u3 * p1.x + v3 * p2.x,
                        w3 * p0.y + u3 * p1.y + v3 * p2.y
                    };
                    float hC2 = fmodf(w3 * h0 + u3 * h1 + v3 * h2 + 360.0f, 360.0f);
                    Color cc2 = HsvToRgb(hC2, 1.0f, 1.0f, alpha);
                    DrawTriangle(q1, q3, q2, cc2);
                }
            }
        }

        Color edge = Fade(WHITE, 0.5f * alpha);
        DrawLineV(p0, p1, edge);
        DrawLineV(p1, p2, edge);
        DrawLineV(p2, p0, edge);
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
    // scale so max extent ≈ size (cube verts are at ±1 → √3 ≈ 1.73)
    float sc = size / 1.73f;

    // 20 dodecahedron vertices:
    // 8 cube vertices (±1,±1,±1)
    // 4 vertices (0, ±φ, ±1/φ)
    // 4 vertices (±1/φ, 0, ±φ)
    // 4 vertices (±φ, ±1/φ, 0)
    float vtx[20][3] = {
        // cube vertices
        { sc, sc, sc}, { sc, sc,-sc}, { sc,-sc, sc}, { sc,-sc,-sc},
        {-sc, sc, sc}, {-sc, sc,-sc}, {-sc,-sc, sc}, {-sc,-sc,-sc},
        // (0, ±φ, ±1/φ)
        {0,  sc*phi,  sc*iphi}, {0,  sc*phi, -sc*iphi},
        {0, -sc*phi,  sc*iphi}, {0, -sc*phi, -sc*iphi},
        // (±1/φ, 0, ±φ)
        { sc*iphi, 0,  sc*phi}, {-sc*iphi, 0,  sc*phi},
        { sc*iphi, 0, -sc*phi}, {-sc*iphi, 0, -sc*phi},
        // (±φ, ±1/φ, 0)
        { sc*phi,  sc*iphi, 0}, { sc*phi, -sc*iphi, 0},
        {-sc*phi,  sc*iphi, 0}, {-sc*phi, -sc*iphi, 0},
    };

    // 12 pentagonal faces
    int faces[12][5] = {
        { 0, 12,  2, 17, 16}, { 0, 16,  1,  9,  8},
        { 0,  8,  4, 13, 12}, { 1, 16, 17,  3, 14},
        { 1, 14, 15,  5,  9}, { 2, 12, 13,  6, 10},
        { 2, 10, 11,  3, 17}, { 4,  8,  9,  5, 18},
        { 4, 18, 19,  6, 13}, { 7, 11, 10,  6, 19},
        { 7, 19, 18,  5, 15}, { 7, 15, 14,  3, 11},
    };

    // Shadow pass — flat black, top-down projection
    if (shadowAlpha > 0) {
        float scy = cosf(rotY), ssy = sinf(rotY);
        float scx = cosf(PI/2), ssx = sinf(PI/2);
        Vector2 sp[20];
        for (int i = 0; i < 20; i++) {
            float x = vtx[i][0]*SHADOW_SCALE, y = vtx[i][1]*SHADOW_SCALE, z = vtx[i][2]*SHADOW_SCALE;
            float x1 = x * scy - z * ssy;
            float z1 = x * ssy + z * scy;
            float y1 = y * scx - z1 * ssx;
            sp[i] = (Vector2){ shadowPos.x + x1, shadowPos.y + y1 };
        }
        Color scol = Fade(SHADOW_COLOR, shadowAlpha);
        for (int f = 0; f < 12; f++) {
            Vector2 pv[5];
            for (int vi = 0; vi < 5; vi++) pv[vi] = sp[faces[f][vi]];
            float cross = (pv[1].x-pv[0].x)*(pv[2].y-pv[0].y) - (pv[1].y-pv[0].y)*(pv[2].x-pv[0].x);
            if (cross < 0) {
                for (int tri = 0; tri < 3; tri++)
                    DrawTriangle(pv[0], pv[tri+1], pv[tri+2], scol);
            }
        }
    }

    float cy = cosf(rotY), sy = sinf(rotY);
    float cx = cosf(rotX), sx = sinf(rotX);

    Vector2 pj[20];
    float pz[20];
    float hues[20];
    float time = (float)GetTime();

    for (int i = 0; i < 20; i++) {
        float x = vtx[i][0], y = vtx[i][1], z = vtx[i][2];
        float x1 = x * cy - z * sy;
        float z1 = x * sy + z * cy;
        float y1 = y * cx - z1 * sx;
        float z2 = y * sx + z1 * cx;
        pj[i] = (Vector2){ pos.x + x1, pos.y + y1 };
        pz[i] = z2;
        hues[i] = time * SHAPE_HUE_SPEED + (float)i * DODECA_HUE_VERTEX_STEP;
    }

    float faceZ[12];
    bool faceVis[12];
    for (int f = 0; f < 12; f++) {
        faceZ[f] = 0;
        for (int vi = 0; vi < 5; vi++) faceZ[f] += pz[faces[f][vi]];
        faceZ[f] /= 5.0f;

        Vector2 a = pj[faces[f][0]];
        Vector2 b = pj[faces[f][1]];
        Vector2 c = pj[faces[f][2]];
        float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        faceVis[f] = (cross < 0);
    }

    int order[12];
    for (int i = 0; i < 12; i++) order[i] = i;
    for (int i = 0; i < 11; i++)
        for (int j = i + 1; j < 12; j++)
            if (faceZ[order[i]] > faceZ[order[j]]) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    for (int fi = 0; fi < 12; fi++) {
        int f = order[fi];
        if (!faceVis[f]) continue;

        // Fan pentagon into 3 triangles from vertex 0
        Vector2 pv[5];
        float hv[5];
        for (int vi = 0; vi < 5; vi++) {
            pv[vi] = pj[faces[f][vi]];
            hv[vi] = hues[faces[f][vi]];
        }

        for (int tri = 0; tri < 3; tri++) {
            Vector2 p0 = pv[0];
            Vector2 p1 = pv[tri + 1];
            Vector2 p2 = pv[tri + 2];
            float h0 = hv[0];
            float h1 = hv[tri + 1];
            float h2 = hv[tri + 2];

            int N = SHAPE_SUBDIV_PENTA;
            for (int row = 0; row < N; row++) {
                for (int col = 0; col < N - row; col++) {
                    float u0b = (float)col / N;
                    float v0b = (float)row / N;
                    float w0b = 1.0f - u0b - v0b;

                    float u1b = (float)(col + 1) / N;
                    float v1b = (float)row / N;
                    float w1b = 1.0f - u1b - v1b;

                    float u2b = (float)col / N;
                    float v2b = (float)(row + 1) / N;
                    float w2b = 1.0f - u2b - v2b;

                    Vector2 q0 = {
                        w0b * p0.x + u0b * p1.x + v0b * p2.x,
                        w0b * p0.y + u0b * p1.y + v0b * p2.y
                    };
                    Vector2 q1 = {
                        w1b * p0.x + u1b * p1.x + v1b * p2.x,
                        w1b * p0.y + u1b * p1.y + v1b * p2.y
                    };
                    Vector2 q2 = {
                        w2b * p0.x + u2b * p1.x + v2b * p2.x,
                        w2b * p0.y + u2b * p1.y + v2b * p2.y
                    };

                    float hC = fmodf(w0b * h0 + u0b * h1 + v0b * h2 + 360.0f, 360.0f);
                    Color cc = HsvToRgb(hC, SHAPE_SAT_DODECA, 1.0f, alpha);
                    DrawTriangle(q0, q1, q2, cc);

                    if (col + 1 < N - row) {
                        float u3b = (float)(col + 1) / N;
                        float v3b = (float)(row + 1) / N;
                        float w3b = 1.0f - u3b - v3b;
                        Vector2 q3 = {
                            w3b * p0.x + u3b * p1.x + v3b * p2.x,
                            w3b * p0.y + u3b * p1.y + v3b * p2.y
                        };
                        float hC2 = fmodf(w3b * h0 + u3b * h1 + v3b * h2 + 360.0f, 360.0f);
                        Color cc2 = HsvToRgb(hC2, SHAPE_SAT_DODECA, 1.0f, alpha);
                        DrawTriangle(q1, q3, q2, cc2);
                    }
                }
            }
        }

        // Draw pentagon edges
        Color edge = Fade(WHITE, 0.5f * alpha);
        for (int vi = 0; vi < 5; vi++)
            DrawLineV(pv[vi], pv[(vi + 1) % 5], edge);
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

    // --- Fire patches ---
    for (int i = 0; i < MAX_FIRE_PATCHES; i++) {
        FirePatch *fp = &g.firePatches[i];
        if (!fp->active) continue;
        float t = (float)GetTime();
        float life = fp->timer / FLAME_PATCH_LIFETIME;
        float fade = (life > FIRE_PATCH_FADE_THRESH) ? 1.0f : life / FIRE_PATCH_FADE_THRESH;
        float r = fp->radius;
        // scatter embers — deterministic offsets from patch index
        int embers = FIRE_PATCH_EMBER_COUNT;
        for (int j = 0; j < embers; j++) {
            float seed = i * 7.13f + j * 3.71f;
            // each ember drifts in a small loop
            float ox = sinf(seed + t * 1.5f) * r * 0.6f
                     + cosf(seed * 2.3f) * r * 0.3f;
            float oy = cosf(seed * 1.7f + t * 1.8f) * r * 0.5f
                     - (1.0f - life) * r * 0.4f; // drift upward as patch ages
            float eSize = r * (0.3f + 0.25f * sinf(seed * 5.1f + t * FLAME_FLICKER_SPEED));
            Vector2 ep = { fp->pos.x + ox, fp->pos.y + oy };
            // color: outer embers red/orange, inner ones yellow
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
        // soft glow underneath
        DrawCircleV(fp->pos, r * 0.8f,
            Fade((Color){ 255, 60, 10, 255 }, 0.08f * fade));
        // bright core flicker
        float coreFlicker = 0.6f + 0.4f * sinf(t * 14.0f + i * 2.0f);
        DrawCircleV(fp->pos, r * 0.2f * coreFlicker,
            Fade(YELLOW, 0.35f * fade));
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
        }
    }

    // --- Mine Web VFX ---
    for (int i = 0; i < MAX_MINE_WEBS; i++) {
        MineWebVfx *w = &g.vfx.mineWebs[i];
        if (!w->active) continue;
        float progress = 1.0f - (w->timer / MINE_WEB_DURATION);
        float alpha = 1.0f - progress;
        float r = MINE_ROOT_RADIUS;
        // spokes
        for (int s = 0; s < MINE_WEB_SPOKES; s++) {
            float a = s * (2.0f * PI / MINE_WEB_SPOKES);
            Vector2 tip = { w->pos.x + cosf(a) * r,
                            w->pos.y + sinf(a) * r };
            DrawLineEx(w->pos, tip, 1.0f, Fade(WHITE, alpha * 0.6f));
        }
        // concentric rings connecting the spokes
        for (int ring = 1; ring <= MINE_WEB_RINGS; ring++) {
            float rr = r * ring / (float)MINE_WEB_RINGS;
            for (int s = 0; s < MINE_WEB_SPOKES; s++) {
                float a0 = s * (2.0f * PI / MINE_WEB_SPOKES);
                float a1 = (s + 1) * (2.0f * PI / MINE_WEB_SPOKES);
                Vector2 p0 = { w->pos.x + cosf(a0) * rr,
                               w->pos.y + sinf(a0) * rr };
                Vector2 p1 = { w->pos.x + cosf(a1) * rr,
                               w->pos.y + sinf(a1) * rr };
                DrawLineEx(p0, p1, 1.0f, Fade(WHITE, alpha * 0.4f));
            }
        }
    }

    // --- Projectiles ---
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
            // nose cone tip
            Vector2 noseTip = Vector2Add(b->pos, Vector2Scale(fwd, tip));
            // body front (where nose meets body)
            Vector2 frontL = Vector2Add(b->pos, Vector2Scale(perp,  w));
            Vector2 frontR = Vector2Add(b->pos, Vector2Scale(perp, -w));
            // body rear
            Vector2 base = Vector2Subtract(b->pos, Vector2Scale(fwd, len));
            Vector2 rearL = Vector2Add(base, Vector2Scale(perp,  w));
            Vector2 rearR = Vector2Add(base, Vector2Scale(perp, -w));
            // draw body (two triangles for the rect)
            DrawTriangle(frontL, rearL, rearR, SNIPER_BODY_COLOR);
            DrawTriangle(frontL, rearR, frontR, SNIPER_BODY_COLOR);
            // draw nose cone
            DrawTriangle(noseTip, frontR, frontL, SNIPER_TIP_COLOR);
            // tracer trail
            Vector2 trail = Vector2Subtract(base, Vector2Scale(fwd, len * SNIPER_TRAIL_MULT));
            DrawLineEx(base, trail, w * 0.8f, Fade(SNIPER_COLOR, 0.4f));
        } else if (b->type == PROJ_BFG) {
            // pulsing lightning ball
            float pulse = sinf(GetTime() * BFG_PULSE_SPEED) * BFG_PULSE_AMOUNT;
            float outerR = b->size + pulse;
            float innerR = b->size * 0.6f + pulse * 0.5f;
            DrawCircleV(b->pos, outerR, (Color)BFG_GLOW_COLOR);
            DrawCircleV(b->pos, innerR, (Color)BFG_COLOR);
            DrawCircleV(b->pos, innerR * 0.4f, WHITE);
        } else if (b->type == PROJ_GRENADE) {
            // shadow on ground
            DrawCircleV(b->pos, b->size * 0.8f, Fade(BLACK, 0.3f));
            // grenade drawn at visual height offset (negative Y = up on screen)
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

    // --- Explosion rings ---
    for (int i = 0; i < MAX_EXPLOSIVES; i++) {
        Explosive *ex = &g.vfx.explosives[i];
        if (!ex->active) continue;
        float t = ex->timer / ex->duration;
        float alpha = t * EXPLOSION_RING_ALPHA;
        float radius = ROCKET_EXPLOSION_RADIUS * (1.0f - t * EXPLOSION_RING_DECAY);
        DrawCircleLinesV(ex->pos, radius, Fade(ORANGE, alpha));
        DrawCircleLinesV(ex->pos, radius - 2.0f, Fade(RED, alpha * 0.6f));
    }

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
            break;
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
        float progress = 1.0f - (p->sword.timer / p->sword.duration);
        float sweepAngle =
            p->sword.angle - p->sword.arc / 2.0f + p->sword.arc * progress;
        int numSegments = SWORD_DRAW_SEGMENTS;
        float arcHalf = p->sword.arc / 2.0f;
        Color arcColor = p->sword.dashSlash ? SKYBLUE : ORANGE;
        for (int i = 1; i <= numSegments; i++) {
            float segPos = (float)i / numSegments;
            if (segPos > progress) break;

            float t = segPos / progress;
            float alpha = t * t * 0.9f;
            float thick = 2.0f + t * 6.0f;

            float a0 = p->sword.angle - arcHalf
                + p->sword.arc * (float)(i - 1) / numSegments;
            float a1 = p->sword.angle - arcHalf
                + p->sword.arc * (float)i / numSegments;
            Vector2 pt0 = Vector2Add(p->pos,
                (Vector2){ cosf(a0) * p->sword.radius,
                           sinf(a0) * p->sword.radius });
            Vector2 pt1 = Vector2Add(p->pos,
                (Vector2){ cosf(a1) * p->sword.radius,
                           sinf(a1) * p->sword.radius });
            DrawLineEx(pt0, pt1, thick, Fade(arcColor, alpha));
        }
        Vector2 sweepEnd = Vector2Add(
            p->pos,
            (Vector2){
                cosf(sweepAngle) * p->sword.radius,
                sinf(sweepAngle) * p->sword.radius
            });
        DrawLineEx(p->pos, sweepEnd, 3.0f, WHITE);
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

    BeginDrawing();
    ClearBackground(SELECT_BG_COLOR);

    // Title — changes per phase
    int titleFont = (int)(SELECT_TITLE_FONT * ui);
    const char *title = g.selectPhase == 0
        ? "CHOOSE PRIMARY" : "CHOOSE SECONDARY";
    int titleW = MeasureText(title, titleFont);
    int titleY = (int)(sh * SELECT_TITLE_Y);
    DrawText(title, sw / 2 - titleW / 2, titleY, titleFont, WHITE);

    // Hint (below title)
    int hintFont = (int)(SELECT_HINT_FONT * ui);
    const char *hint = "W/S to navigate - Enter or M1 to confirm";
    int hintW = MeasureText(hint, hintFont);
    DrawText(hint, sw / 2 - hintW / 2,
        titleY + titleFont + (int)(SELECT_HINT_GAP * ui), hintFont, Fade(WHITE, 0.5f));

    // Keybinds — two columns on left side
    const char *coreKeys[]  = { "WASD", "Space", "M1", "M2", "Ctrl", "P/Esc", "0" };
    const char *coreDescs[] = { "Move", "Dash", "Primary", "Alt Fire", "Swap Weapon", "Pause", "Exit" };
    int coreCount = 7;
    int keyFont = (int)(SELECT_KEYS_FONT * ui);
    int keySpacing = (int)(SELECT_KEYS_SPACING * ui);
    int keyX = (int)(sw * SELECT_KEYS_X);
    int keyBaseY = (int)(sh * SELECT_KEYS_Y);
    int keyTabW = (int)(60 * ui);

    // Column 1: core controls
    for (int i = 0; i < coreCount; i++) {
        int ky = keyBaseY + i * keySpacing;
        DrawText(coreKeys[i], keyX, ky, keyFont, SELECT_HIGHLIGHT_COLOR);
        DrawText(coreDescs[i], keyX + keyTabW, ky, keyFont, Fade(WHITE, 0.6f));
    }

    // Column 2: abilities (starts to the right of column 1)
    int ablX = keyX + (int)(160 * ui);
    Player *sp = &g.player;
    int ablRow = 0;
    for (int i = 0; i < ABILITY_SLOTS; i++) {
        const char *name = AbilityName(sp->slots[i].ability);
        if (!name) continue;
        int ky = keyBaseY + ablRow * keySpacing;
        DrawText(KeyName(sp->slots[i].key), ablX, ky, keyFont, SELECT_HIGHLIGHT_COLOR);
        DrawText(name, ablX + keyTabW, ky, keyFont, Fade(WHITE, 0.6f));
        ablRow++;
    }

    // Weapons (right column) — ordered by solid face count (4→6→8→12→20)
    const char *names[] = { "SWORD", "REVOLVER", "MACHINE GUN", "SNIPER", "ROCKET" };
    const char *descs[] = {
        "M1 sweep, M2 lunge, dash slash",
        "M1 single, M2 fan hammer, dash reload",
        "M1 burst, M2 minigun, overheat QTE",
        "M1 hip fire, M2 ADS, dash super shot",
        "M1 rocket, M2 detonate, rocket jump",
    };
    // Map display index → WeaponType
    WeaponType selectWeapons[] = {
        WPN_SWORD, WPN_REVOLVER, WPN_GUN, WPN_SNIPER, WPN_ROCKET
    };
    int optFont = (int)(SELECT_OPTION_FONT * ui);
    int descFont = (int)(SELECT_DESC_FONT * ui);
    int spacing = (int)(SELECT_OPTION_SPACING * ui);
    int baseY = (int)(sh * SELECT_OPTIONS_Y);
    int weaponRightX = (int)(sw * SELECT_WEAPONS_X);

    // Draw function per display slot (face count order)
    typedef void (*DrawSolidFn)(Vector2, float, float, float, float, Vector2, float);
    DrawSolidFn solidFns[] = {
        DrawTetra2D,    // Tetrahedron  (4)  — SWORD
        DrawCube2D,     // Cube         (6)  — REVOLVER
        DrawOcta2D,     // Octahedron   (8)  — GUN
        DrawDodeca2D,   // Dodecahedron (12) — SNIPER
        DrawIcosa2D,    // Icosahedron  (20) — ROCKET
    };

    for (int i = 0; i < 5; i++) {
        int y = baseY + i * spacing;
        // Phase 1: gray out already-chosen primary
        bool locked = (g.selectPhase == 1
            && selectWeapons[i] == g.player.primary);
        Color nameColor, descColor;
        if (locked) {
            nameColor = Fade(GREEN, 0.4f);
            descColor = Fade(GREEN, 0.25f);
        } else if (i == g.selectIndex) {
            nameColor = SELECT_HIGHLIGHT_COLOR;
            descColor = WHITE;
        } else {
            nameColor = GRAY;
            descColor = DARKGRAY;
        }

        int nameW = MeasureText(names[i], optFont);
        int nameX = weaponRightX - nameW;
        DrawText(names[i], nameX, y, optFont, nameColor);

        // Selection highlight box
        if (i == g.selectIndex && !locked) {
            int pad = (int)(SELECT_CURSOR_PAD * ui);
            DrawRectangleLinesEx(
                (Rectangle){ nameX - pad, y - pad,
                    nameW + pad * 2, optFont + pad * 2 },
                SELECT_CURSOR_THICK * ui, SELECT_HIGHLIGHT_COLOR);
        }
        // Locked indicator for chosen primary
        if (locked) {
            int pad = (int)(SELECT_CURSOR_PAD * ui);
            DrawRectangleLinesEx(
                (Rectangle){ nameX - pad, y - pad,
                    nameW + pad * 2, optFont + pad * 2 },
                SELECT_CURSOR_THICK * ui, Fade(GREEN, 0.4f));
        }

        // Solid preview next to weapon name
        float solidSize = optFont * 0.35f;
        float previewAlpha = locked ? 0.4f :
            (i == g.selectIndex) ? 1.0f : 0.3f;
        float shadowAlpha = locked ? SHADOW_ALPHA * 0.2f :
            (i == g.selectIndex) ? SHADOW_ALPHA : SHADOW_ALPHA * 0.3f;
        float t = (float)GetTime();
        Vector2 solidPos = { nameX - optFont * 1.0f, y + optFont * 0.5f };
        Vector2 shadowP = { solidPos.x + SHADOW_OFFSET_X, solidPos.y + SHADOW_OFFSET_Y };
        solidFns[i](solidPos, solidSize, t * PLAYER_ROT_SPEED, PLAYER_ROT_TILT, previewAlpha, shadowP, shadowAlpha);

        int descW = MeasureText(descs[i], descFont);
        DrawText(descs[i], weaponRightX - descW,
            y + optFont + (int)(SELECT_DESC_GAP * ui), descFont, descColor);
    }

    EndDrawing();
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

    // Cooldown indicators
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
    {
        if (p->rocket.cooldownTimer > 0) {
            float cdRatio = p->rocket.cooldownTimer / p->rocket.cooldown;
            DrawRectangle(cdX, cdY,
                (int)(cdBarW * (1.0f - cdRatio)), cdBarH, RED);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("ROCKET", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, RED);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("ROCKET", cdLabelX, cdY, cdFontSize, RED);
        }
    }

    // Railgun cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color rgColor = RAILGUN_COLOR;
        if (p->railgun.cooldownTimer > 0) {
            float ratio = 1.0f - (p->railgun.cooldownTimer / RAILGUN_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, rgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("RAIL", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, rgColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("RAIL", cdLabelX, cdY, cdFontSize, rgColor);
        }
    }

    // Sniper cooldown (when sniper is equipped)
    if (p->primary == WPN_SNIPER || p->secondary == WPN_SNIPER) {
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        // Label and color change with mode
        const char *snLabel;
        Color snColor;
        float snCooldown;
        if (p->sniper.superShotReady) {
            snLabel = "SUPER";
            snColor = GOLD;
            snCooldown = SNIPER_AIM_COOLDOWN;
        } else if (p->sniper.aiming) {
            snLabel = "AIM";
            snColor = WHITE;
            snCooldown = SNIPER_AIM_COOLDOWN;
        } else {
            snLabel = "HIP";
            snColor = SNIPER_COLOR;
            snCooldown = SNIPER_HIP_COOLDOWN;
        }
        if (p->sniper.cooldownTimer > 0) {
            float ratio = 1.0f - (p->sniper.cooldownTimer / snCooldown);
            if (ratio > 1.0f) ratio = 1.0f;
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, snColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText(snLabel, cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, snColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText(snLabel, cdLabelX, cdY, cdFontSize, snColor);
        }
    }
    }

    // Grenade cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color grColor = GRENADE_COLOR;
        if (p->grenade.cooldownTimer > 0) {
            float ratio = 1.0f - (p->grenade.cooldownTimer / GRENADE_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, grColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("GREN", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, grColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("GREN", cdLabelX, cdY, cdFontSize, grColor);
        }
    }

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
    {
        Color slColor = SLAM_COLOR;
        if (p->slam.cooldownTimer > 0) {
            float ratio = 1.0f - (p->slam.cooldownTimer / SLAM_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, slColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("SLAM", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, slColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("SLAM", cdLabelX, cdY, cdFontSize, slColor);
        }
    }

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
    {
        Color tuColor = TURRET_COLOR;
        if (p->turretCooldown > 0) {
            float ratio = 1.0f - (p->turretCooldown / TURRET_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, tuColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("TURRT", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, tuColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("TURRT", cdLabelX, cdY, cdFontSize, tuColor);
        }
    }

    // Mine cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color miColor = MINE_COLOR;
        if (p->mineCooldown > 0) {
            float ratio = 1.0f - (p->mineCooldown / MINE_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, miColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("MINE", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, miColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("MINE", cdLabelX, cdY, cdFontSize, miColor);
        }
    }

    // Heal cooldown
    cdY += (int)(HUD_ROW_SPACING * ui);
    {
        Color heColor = HEAL_COLOR;
        if (p->healCooldown > 0) {
            float ratio = 1.0f - (p->healCooldown / HEAL_COOLDOWN);
            DrawRectangle(cdX, cdY, (int)(cdBarW * ratio), cdBarH, heColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, GRAY);
            DrawText("HEAL", cdLabelX, cdY, cdFontSize, GRAY);
        } else {
            DrawRectangle(cdX, cdY, cdBarW, cdBarH, heColor);
            DrawRectangleLines(cdX, cdY, cdBarW, cdBarH, WHITE);
            DrawText("HEAL", cdLabelX, cdY, cdFontSize, heColor);
        }
    }

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

    // Active reload/overheat arc (near player) ------------------------------ /
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

    // Pause overlay
    if (g.paused && !g.gameOver) {
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
        Player *pp = &g.player;
        int ablCount = 0;
        int ablSlots[ABILITY_SLOTS];
        for (int i = 0; i < ABILITY_SLOTS; i++) {
            if (AbilityName(pp->slots[i].ability))
                ablSlots[ablCount++] = i;
        }
        int half = (ablCount + 1) / 2;
        for (int a = 0; a < ablCount; a++) {
            int si = ablSlots[a];
            int colX = (a < half) ? mColX : rColX;
            int row = (a < half) ? a : a - half;
            int ky = pkY + row * pkSpacing;
            DrawText(KeyName(pp->slots[si].key), colX, ky, pkFont, SELECT_HIGHLIGHT_COLOR);
            DrawText(AbilityName(pp->slots[si].ability),
                colX + pkTabW, ky, pkFont, Fade(WHITE, 0.6f));
        }

        // Fullscreen toggle
        int maxRows = lCount > half ? lCount : half;
        int fsY = pkY + (maxRows + 1) * pkSpacing;
        const char *fsLabel = IsWindowFullscreen() ? "[F] Fullscreen: ON" : "[F] Fullscreen: OFF";
        int fsW = MeasureText(fsLabel, pkFont);
        DrawText(fsLabel, sw / 2 - fsW / 2, fsY, pkFont, Fade(WHITE, 0.6f));
    }

    // Game Over overlay
    if (g.gameOver) {
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
