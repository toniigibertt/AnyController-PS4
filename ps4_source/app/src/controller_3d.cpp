#include "controller_3d.h"
#include <math.h>
#include <string.h>

namespace {

struct Vec3 {
    float x, y, z;
};

struct Vec2 {
    float x, y;
};

struct ProjVert {
    float sx, sy, depth;
};

static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

static inline uint8_t clampu8(int v) {
    return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

static inline Color lerp_color(Color a, Color b, float t) {
    t = clampf(t, 0.0f, 1.0f);
    return Color{
        clampu8((int)(a.r + (b.r - a.r) * t)),
        clampu8((int)(a.g + (b.g - a.g) * t)),
        clampu8((int)(a.b + (b.b - a.b) * t))
    };
}

static Color hsv_to_rgb(float h, float s, float v) {
    float r = v, g = v, b = v;
    float f = h * 6.0f;
    int i = (int)floorf(f);
    float frac = f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * frac);
    float t = v * (1.0f - s * (1.0f - frac));
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return Color{ clampu8((int)(r * 255.0f)), clampu8((int)(g * 255.0f)), clampu8((int)(b * 255.0f)) };
}

struct Xform3D {
    float yaw;
    float pitch;
    float roll;
    float ox, oy, oz;
    float cx, cy;
    float scale;
    float fov;

    ProjVert project(Vec3 p) const {
        // Rotate around X (pitch), Y (yaw), Z (roll)
        float cp = cosf(pitch), sp = sinf(pitch);
        float y1 = p.y * cp - p.z * sp;
        float z1 = p.y * sp + p.z * cp;
        float x1 = p.x;

        float cyaw = cosf(yaw), syaw = sinf(yaw);
        float x2 = x1 * cyaw + z1 * syaw;
        float z2 = -x1 * syaw + z1 * cyaw;
        float y2 = y1;

        float cr = cosf(roll), sr = sinf(roll);
        float x3 = x2 * cr - y2 * sr;
        float y3 = x2 * sr + y2 * cr;
        float z3 = z2;

        float wx = x3 + ox;
        float wy = y3 + oy;
        float wz = z3 + oz;

        float camDist = fov + wz;
        if (camDist < 0.8f) camDist = 0.8f;
        float persp = fov / camDist;

        return ProjVert{
            cx + wx * scale * persp,
            cy + wy * scale * persp,
            wz
        };
    }

    Vec3 rotate_normal(Vec3 n) const {
        float cp = cosf(pitch), sp = sinf(pitch);
        float y1 = n.y * cp - n.z * sp;
        float z1 = n.y * sp + n.z * cp;
        float x1 = n.x;

        float cyaw = cosf(yaw), syaw = sinf(yaw);
        float x2 = x1 * cyaw + z1 * syaw;
        float z2 = -x1 * syaw + z1 * cyaw;
        float y2 = y1;

        float cr = cosf(roll), sr = sinf(roll);
        return Vec3{ x2 * cr - y2 * sr, x2 * sr + y2 * cr, z2 };
    }
};

// Fast clipped blend pixel helper writing directly to Scene2D framebuffer
static inline void blend_pixel_fb(uint32_t* fb, int fbW, int fbH, int x, int y, Color c, uint8_t alpha) {
    if (x < 0 || x >= fbW || y < 0 || y >= fbH || alpha == 0) return;
    int idx = y * fbW + x;
    if (alpha >= 255) {
        fb[idx] = 0x80000000U | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
        return;
    }
    uint32_t bg = fb[idx];
    uint32_t bgR = (bg >> 16) & 0xFF;
    uint32_t bgG = (bg >> 8) & 0xFF;
    uint32_t bgB = bg & 0xFF;
    uint32_t inv = 255 - alpha;
    uint32_t r = (c.r * alpha + bgR * inv) / 255;
    uint32_t g = (c.g * alpha + bgG * inv) / 255;
    uint32_t b = (c.b * alpha + bgB * inv) / 255;
    fb[idx] = 0x80000000U | (r << 16) | (g << 8) | b;
}

static inline void add_pixel_fb(uint32_t* fb, int fbW, int fbH, int x, int y, Color c, uint8_t alpha) {
    if (x < 0 || x >= fbW || y < 0 || y >= fbH || alpha == 0) return;
    int idx = y * fbW + x;
    uint32_t bg = fb[idx];
    uint32_t bgR = (bg >> 16) & 0xFF;
    uint32_t bgG = (bg >> 8) & 0xFF;
    uint32_t bgB = bg & 0xFF;
    uint32_t r = bgR + (c.r * alpha) / 255; if (r > 255) r = 255;
    uint32_t g = bgG + (c.g * alpha) / 255; if (g > 255) g = 255;
    uint32_t b = bgB + (c.b * alpha) / 255; if (b > 255) b = 255;
    fb[idx] = 0x80000000U | (r << 16) | (g << 8) | b;
}

static void fill_triangle(uint32_t* fb, int fbW, int fbH,
                          int clipX1, int clipY1, int clipX2, int clipY2,
                          Vec2 p0, Vec2 p1, Vec2 p2, Color col, uint8_t alpha = 255) {
    if (p0.y > p1.y) { Vec2 t = p0; p0 = p1; p1 = t; }
    if (p1.y > p2.y) { Vec2 t = p1; p1 = p2; p2 = t; }
    if (p0.y > p1.y) { Vec2 t = p0; p0 = p1; p1 = t; }

    int minY = (int)ceilf(p0.y);
    int maxY = (int)floorf(p2.y);
    if (minY < clipY1) minY = clipY1;
    if (maxY > clipY2) maxY = clipY2;
    if (minY > maxY) return;

    float totalH = p2.y - p0.y;
    if (totalH < 0.001f) return;

    uint32_t solid = 0x80000000U | ((uint32_t)col.r << 16) | ((uint32_t)col.g << 8) | col.b;

    for (int y = minY; y <= maxY; y++) {
        bool secondHalf = ((float)y > p1.y) || (p1.y == p0.y);
        float segH = secondHalf ? (p2.y - p1.y) : (p1.y - p0.y);
        if (segH < 0.0001f) segH = 0.0001f;

        float alphaA = ((float)y - p0.y) / totalH;
        float alphaB = secondHalf ? (((float)y - p1.y) / segH) : (((float)y - p0.y) / segH);

        float ax = p0.x + (p2.x - p0.x) * alphaA;
        float bx = secondHalf ? (p1.x + (p2.x - p1.x) * alphaB) : (p0.x + (p1.x - p0.x) * alphaB);

        if (ax > bx) { float t = ax; ax = bx; bx = t; }
        int xStart = (int)ceilf(ax);
        int xEnd = (int)floorf(bx);
        if (xStart < clipX1) xStart = clipX1;
        if (xEnd > clipX2) xEnd = clipX2;
        if (xStart > xEnd) continue;

        if (alpha >= 255) {
            uint32_t* row = fb + y * fbW;
            for (int x = xStart; x <= xEnd; x++) row[x] = solid;
        } else {
            for (int x = xStart; x <= xEnd; x++) blend_pixel_fb(fb, fbW, fbH, x, y, col, alpha);
        }
    }
}

static void fill_quad(uint32_t* fb, int fbW, int fbH,
                      int cx1, int cy1, int cx2, int cy2,
                      Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color col, uint8_t alpha = 255) {
    fill_triangle(fb, fbW, fbH, cx1, cy1, cx2, cy2, a, b, c, col, alpha);
    fill_triangle(fb, fbW, fbH, cx1, cy1, cx2, cy2, a, c, d, col, alpha);
}

static void draw_line_aa(uint32_t* fb, int fbW, int fbH,
                         int cx1, int cy1, int cx2, int cy2,
                         float x0, float y0, float x1, float y1,
                         float thickness, Color col, uint8_t alpha = 255) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    int steps = (int)ceilf(len * 1.5f);
    int r = (int)ceilf(thickness * 0.5f);
    for (int s = 0; s <= steps; s++) {
        float t = (float)s / steps;
        int px = (int)roundf(x0 + dx * t);
        int py = (int)roundf(y0 + dy * t);
        for (int oy = -r; oy <= r; oy++) {
            int yy = py + oy;
            if (yy < cy1 || yy > cy2) continue;
            for (int ox = -r; ox <= r; ox++) {
                int xx = px + ox;
                if (xx < cx1 || xx > cx2) continue;
                if (ox * ox + oy * oy <= r * r + 1) {
                    blend_pixel_fb(fb, fbW, fbH, xx, yy, col, alpha);
                }
            }
        }
    }
}

static void draw_glow_disc(uint32_t* fb, int fbW, int fbH,
                           int cx1, int cy1, int cx2, int cy2,
                           float cx, float cy, float radius, Color col, uint8_t maxAlpha, bool additive = false) {
    int x0 = (int)floorf(cx - radius);
    int x1 = (int)ceilf(cx + radius);
    int y0 = (int)floorf(cy - radius);
    int y1 = (int)ceilf(cy + radius);
    if (x0 < cx1) x0 = cx1;
    if (x1 > cx2) x1 = cx2;
    if (y0 < cy1) y0 = cy1;
    if (y1 > cy2) y1 = cy2;
    float r2 = radius * radius;
    for (int y = y0; y <= y1; y++) {
        float dy = (float)y - cy;
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - cx;
            float d2 = dx * dx + dy * dy;
            if (d2 >= r2) continue;
            float k = 1.0f - sqrtf(d2) / radius;
            uint8_t a = (uint8_t)(k * k * maxAlpha);
            if (additive) add_pixel_fb(fb, fbW, fbH, x, y, col, a);
            else blend_pixel_fb(fb, fbW, fbH, x, y, col, a);
        }
    }
}

static void draw_ellipse_filled(uint32_t* fb, int fbW, int fbH,
                                int cx1, int cy1, int cx2, int cy2,
                                float cx, float cy, float rx, float ry, Color col, uint8_t alpha = 255) {
    int x0 = (int)floorf(cx - rx - 1);
    int x1 = (int)ceilf(cx + rx + 1);
    int y0 = (int)floorf(cy - ry - 1);
    int y1 = (int)ceilf(cy + ry + 1);
    if (x0 < cx1) x0 = cx1;
    if (x1 > cx2) x1 = cx2;
    if (y0 < cy1) y0 = cy1;
    if (y1 > cy2) y1 = cy2;
    float invRx2 = 1.0f / (rx * rx + 0.001f);
    float invRy2 = 1.0f / (ry * ry + 0.001f);
    for (int y = y0; y <= y1; y++) {
        float dy = (float)y - cy;
        float dy2 = dy * dy * invRy2;
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - cx;
            float val = dx * dx * invRx2 + dy2;
            if (val <= 1.0f) {
                float edge = (val > 0.85f) ? ((1.0f - val) / 0.15f) : 1.0f;
                blend_pixel_fb(fb, fbW, fbH, x, y, col, (uint8_t)(alpha * edge));
            }
        }
    }
}

static void draw_ring_ellipse(uint32_t* fb, int fbW, int fbH,
                              int cx1, int cy1, int cx2, int cy2,
                              float cx, float cy, float rx, float ry,
                              float thickness, Color col, uint8_t alpha, bool additive = false) {
    float rOuterX = rx + thickness;
    float rOuterY = ry + thickness;
    int x0 = (int)floorf(cx - rOuterX - 1);
    int x1 = (int)ceilf(cx + rOuterX + 1);
    int y0 = (int)floorf(cy - rOuterY - 1);
    int y1 = (int)ceilf(cy + rOuterY + 1);
    if (x0 < cx1) x0 = cx1;
    if (x1 > cx2) x1 = cx2;
    if (y0 < cy1) y0 = cy1;
    if (y1 > cy2) y1 = cy2;

    float invRx = 1.0f / (rx + 0.001f);
    float invRy = 1.0f / (ry + 0.001f);
    float halfT = thickness * 0.5f / (rx > 1.0f ? rx : 1.0f);
    for (int y = y0; y <= y1; y++) {
        float ny = ((float)y - cy) * invRy;
        for (int x = x0; x <= x1; x++) {
            float nx = ((float)x - cx) * invRx;
            float d = sqrtf(nx * nx + ny * ny);
            float dist = fabsf(d - 1.0f);
            if (dist <= halfT) {
                float w = 1.0f - (dist / (halfT + 0.0001f));
                uint8_t a = (uint8_t)(alpha * w);
                if (additive) add_pixel_fb(fb, fbW, fbH, x, y, col, a);
                else blend_pixel_fb(fb, fbW, fbH, x, y, col, a);
            }
        }
    }
}

static Color shade_surface(Color baseCol, Vec3 normalWorld, Color rimCol) {
    // Key directional light from top-left-front
    Vec3 lightDir = { -0.35f, -0.55f, -0.75f };
    float ndotl = -(normalWorld.x * lightDir.x + normalWorld.y * lightDir.y + normalWorld.z * lightDir.z);
    ndotl = clampf(ndotl, 0.0f, 1.0f);

    float rim = clampf(1.0f - fabsf(normalWorld.z), 0.0f, 1.0f);
    rim = rim * rim * 0.35f;

    float k = 0.42f + ndotl * 0.68f;
    int r = (int)(baseCol.r * k + rimCol.r * rim);
    int g = (int)(baseCol.g * k + rimCol.g * rim);
    int b = (int)(baseCol.b * k + rimCol.b * rim);
    return Color{ clampu8(r), clampu8(g), clampu8(b) };
}

static Color compute_lightbar_color(bool isDS5, const AppConfig& cfg, float timeSec, uint8_t batteryPct) {
    int mode = isDS5 ? cfg.ds_lightbar_mode : cfg.ds4_lightbar_mode;
    int colorIdx = isDS5 ? cfg.ds_lightbar_color : cfg.ds4_lightbar_color;
    int brightIdx = isDS5 ? cfg.ds_brightness : cfg.ds4_brightness;

    if (mode == 3) return Color{ 16, 24, 38 };

    float bright = (brightIdx == 0) ? 1.0f : ((brightIdx == 1) ? 0.65f : 0.35f);
    Color base = { 0, 145, 255 };

    if (mode == 4) {
        float h = fmodf(timeSec * 0.22f, 1.0f);
        base = hsv_to_rgb(h, 0.92f, 1.0f);
    } else if (mode == 2) {
        if (batteryPct > 60) base = Color{ 16, 220, 140 };
        else if (batteryPct > 20) base = Color{ 255, 180, 30 };
        else base = Color{ 255, 55, 65 };
    } else if (mode == 0) {
        base = Color{ 0, 150, 255 };
    } else {
        static const Color DS5_COLS[6] = {
            { 0, 112, 209 }, { 255, 34, 51 }, { 0, 230, 118 },
            { 179, 136, 255 }, { 0, 229, 255 }, { 245, 250, 255 }
        };
        static const Color DS4_COLS[7] = {
            { 0, 112, 220 }, { 239, 68, 68 }, { 16, 185, 129 },
            { 168, 85, 247 }, { 6, 182, 212 }, { 245, 158, 11 }, { 248, 250, 252 }
        };
        if (isDS5) base = DS5_COLS[(colorIdx >= 0 && colorIdx < 6) ? colorIdx : 0];
        else base = DS4_COLS[(colorIdx >= 0 && colorIdx < 7) ? colorIdx : 0];

        if (mode == 5) {
            bright *= (0.48f + 0.52f * (0.5f + 0.5f * sinf(timeSec * 3.0f)));
        }
    }

    return Color{
        clampu8((int)(base.r * bright)),
        clampu8((int)(base.g * bright)),
        clampu8((int)(base.b * bright))
    };
}

} // namespace

Controller3DRenderer::Controller3DRenderer() : m_time(0.0f), m_prevButtons(0) {
    memset(m_pulse, 0, sizeof(m_pulse));
}

void Controller3DRenderer::update(float dt, uint32_t buttons, int /*rumblePhase*/) {
    m_time += dt;

    static const uint32_t TRACKED_BITS[18] = {
        ORBIS_PAD_BUTTON_CROSS,
        ORBIS_PAD_BUTTON_CIRCLE,
        ORBIS_PAD_BUTTON_SQUARE,
        ORBIS_PAD_BUTTON_TRIANGLE,
        ORBIS_PAD_BUTTON_UP,
        ORBIS_PAD_BUTTON_DOWN,
        ORBIS_PAD_BUTTON_LEFT,
        ORBIS_PAD_BUTTON_RIGHT,
        ORBIS_PAD_BUTTON_L1,
        ORBIS_PAD_BUTTON_R1,
        ORBIS_PAD_BUTTON_L2,
        ORBIS_PAD_BUTTON_R2,
        ORBIS_PAD_BUTTON_L3,
        ORBIS_PAD_BUTTON_R3,
        0x00000001U,               // Share / Create
        ORBIS_PAD_BUTTON_OPTIONS,
        0x00010000U,               // PS Button
        ORBIS_PAD_BUTTON_TOUCH_PAD
    };

    uint32_t rising = buttons & ~m_prevButtons;
    m_prevButtons = buttons;

    for (int i = 0; i < 18; i++) {
        if (rising & TRACKED_BITS[i]) {
            m_pulse[i] = 1.0f;
        } else if (m_pulse[i] > 0.0f) {
            m_pulse[i] -= dt * 2.4f;
            if (m_pulse[i] < 0.0f) m_pulse[i] = 0.0f;
        }
    }
}

void Controller3DRenderer::render(Scene2D* scene, int vx, int vy, int vw, int vh,
                                  bool isDualSense, const AppConfig& cfg,
                                  const OrbisPadData& padData, int rumblePhase) {
    if (!scene) return;
    uint32_t* fb = scene->GetActiveFrameBuffer();
    if (!fb) return;
    int fbW = scene->GetWidth();
    int fbH = scene->GetHeight();

    int cx1 = vx < 0 ? 0 : vx;
    int cy1 = vy < 0 ? 0 : vy;
    int cx2 = (vx + vw - 1) >= fbW ? (fbW - 1) : (vx + vw - 1);
    int cy2 = (vy + vh - 1) >= fbH ? (fbH - 1) : (vy + vh - 1);
    if (cx1 >= cx2 || cy1 >= cy2) return;

    // 1. Dynamic Lightbar Color & Ambient Pedestal Studio Glow
    Color lbColor = compute_lightbar_color(isDualSense, cfg, m_time, 85);
    float centerX = vx + vw * 0.5f;
    float centerY = vy + vh * 0.50f;

    // Pedestal studio glow & holographic orbit rings under controller
    draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                   centerX, centerY + 115.0f, 220.0f, lbColor, 58, true);
    draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                      centerX, centerY + 128.0f, 225.0f, 52.0f, 2.5f, lbColor, 95, true);
    draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                      centerX, centerY + 128.0f, 165.0f, 36.0f, 1.5f, Color{ 56, 189, 248 }, 65, true);

    // 2. Smooth 3D Idle Orbital Float + Rumble Vibration Shake
    float bobY = sinf(m_time * 1.9f) * 0.038f;
    float rumbleX = 0.0f, rumbleY = 0.0f, rumbleRoll = 0.0f;
    if (rumblePhase > 0) {
        float freq = (rumblePhase == 2) ? 46.0f : 28.0f;
        float amp = (rumblePhase == 3) ? 0.030f : 0.020f;
        rumbleX = sinf(m_time * freq * 1.3f) * amp;
        rumbleY = cosf(m_time * freq) * amp * 0.8f;
        rumbleRoll = sinf(m_time * freq * 0.9f) * amp * 1.6f;
    }

    Xform3D xf;
    xf.yaw = sinf(m_time * 0.85f) * 0.24f;
    xf.pitch = 0.44f + sinf(m_time * 1.15f) * 0.055f;
    xf.roll = cosf(m_time * 0.70f) * 0.045f + rumbleRoll;
    xf.ox = rumbleX;
    xf.oy = bobY + rumbleY;
    xf.oz = 0.0f;
    xf.cx = centerX;
    xf.cy = centerY - 6.0f;
    xf.scale = (float)vw * 0.265f;
    xf.fov = 5.4f;

    // Ground soft shadow ellipse below floating controller
    draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                        centerX + rumbleX * 60.0f, centerY + 126.0f,
                        195.0f - bobY * 140.0f, 36.0f - bobY * 35.0f,
                        Color{ 3, 6, 14 }, 165);

    // =========================================================================
    // 3. 3D CHASSIS SILHOUETTE EXTRUSION (16-POINT ERGONOMIC OUTLINE + GRIPS)
    // =========================================================================
    // Coordinates in controller local space: X in [-1.55..1.55], Y in [-0.88..1.05] (down=+Y)
    static const Vec2 DS5_OUTLINE[16] = {
        { -0.78f, -0.70f }, {  0.78f, -0.70f },
        {  1.15f, -0.60f }, {  1.42f, -0.34f },
        {  1.52f,  0.04f }, {  1.48f,  0.54f },
        {  1.32f,  0.96f }, {  1.06f,  1.02f },
        {  0.84f,  0.82f }, {  0.58f,  0.46f },
        { -0.58f,  0.46f }, { -0.84f,  0.82f },
        { -1.06f,  1.02f }, { -1.32f,  0.96f },
        { -1.48f,  0.54f }, { -1.52f,  0.04f },
    };
    static const Vec2 DS5_LEFT_SHOULDER = { -1.42f, -0.34f };
    static const Vec2 DS5_TOP_LEFT_BRIDGE = { -1.15f, -0.60f };

    // Use uniform 18-point closed contour for smooth 3D extrusion
    Vec2 contour[18] = {
        DS5_OUTLINE[0], DS5_OUTLINE[1], DS5_OUTLINE[2], DS5_OUTLINE[3],
        DS5_OUTLINE[4], DS5_OUTLINE[5], DS5_OUTLINE[6], DS5_OUTLINE[7],
        DS5_OUTLINE[8], DS5_OUTLINE[9], DS5_OUTLINE[10], DS5_OUTLINE[11],
        DS5_OUTLINE[12], DS5_OUTLINE[13], DS5_OUTLINE[14], DS5_OUTLINE[15],
        DS5_LEFT_SHOULDER, DS5_TOP_LEFT_BRIDGE
    };

    if (!isDualSense) {
        // DualShock 4 slightly slimmer rounded silhouette
        for (int i = 0; i < 18; i++) {
            contour[i].x *= 0.96f;
            contour[i].y *= 0.93f;
        }
    }

    const float Z_FRONT = -0.16f;
    const float Z_BACK  =  0.22f;

    ProjVert pFront[18], pBack[18];
    for (int i = 0; i < 18; i++) {
        pFront[i] = xf.project(Vec3{ contour[i].x, contour[i].y, Z_FRONT });
        pBack[i]  = xf.project(Vec3{ contour[i].x, contour[i].y, Z_BACK });
    }

    Color shellFront = isDualSense ? Color{ 226, 234, 246 } : Color{ 30, 38, 52 };
    Color shellSide  = isDualSense ? Color{ 108, 122, 146 } : Color{ 16, 22, 34 };
    Color gripCore   = isDualSense ? Color{ 18, 24, 36 }    : Color{ 20, 26, 38 };

    // Back shoulders: L2 / R2 pivoting analog triggers & L1 / R1 bumpers (behind main shell)
    uint32_t btns = padData.buttons;
    float l2Frac = (float)padData.analogButtons.l2 / 255.0f;
    float r2Frac = (float)padData.analogButtons.r2 / 255.0f;
    bool bL1 = (btns & ORBIS_PAD_BUTTON_L1) != 0;
    bool bR1 = (btns & ORBIS_PAD_BUTTON_R1) != 0;
    bool bL2 = (btns & ORBIS_PAD_BUTTON_L2) != 0 || l2Frac > 0.1f;
    bool bR2 = (btns & ORBIS_PAD_BUTTON_R2) != 0 || r2Frac > 0.1f;

    auto draw_shoulder_assembly = [&](bool isLeft, bool bumperPressed, float trigFrac, bool trigActive, float pulseB, float pulseT) {
        float sx = isLeft ? -1.02f : 1.02f;
        float w = 0.25f;

        // L2/R2 Trigger block (rotates downward / inward smoothly with analog pressure)
        float tzShift = trigFrac * 0.08f;
        float tyShift = trigFrac * 0.06f;
        Vec3 t0{ sx - w, -0.78f + tyShift, Z_FRONT + 0.04f + tzShift };
        Vec3 t1{ sx + w, -0.78f + tyShift, Z_FRONT + 0.04f + tzShift };
        Vec3 t2{ sx + w * 0.92f, -0.68f + tyShift, Z_BACK + 0.02f };
        Vec3 t3{ sx - w * 0.92f, -0.68f + tyShift, Z_BACK + 0.02f };
        ProjVert pt0 = xf.project(t0), pt1 = xf.project(t1), pt2 = xf.project(t2), pt3 = xf.project(t3);
        Color tCol = trigActive ? lerp_color(Color{ 0, 210, 255 }, Color{ 255, 165, 30 }, trigFrac) : Color{ 26, 34, 48 };
        fill_quad(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                  Vec2{ pt0.sx, pt0.sy }, Vec2{ pt1.sx, pt1.sy }, Vec2{ pt2.sx, pt2.sy }, Vec2{ pt3.sx, pt3.sy }, tCol);
        if (trigActive || pulseT > 0.0f) {
            float glowCx = (pt0.sx + pt1.sx) * 0.5f;
            float glowCy = (pt0.sy + pt2.sy) * 0.5f;
            draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, glowCx, glowCy, 36.0f + trigFrac * 18.0f, tCol, (uint8_t)(85 + trigFrac * 110), true);
            if (pulseT > 0.0f) {
                draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, glowCx, glowCy,
                                  24.0f + (1.0f - pulseT) * 32.0f, 14.0f + (1.0f - pulseT) * 18.0f,
                                  2.5f, tCol, (uint8_t)(pulseT * 210), true);
            }
        }

        // L1/R1 Bumper bar (depresses slightly inward)
        float bDrop = bumperPressed ? 0.04f : 0.0f;
        Vec3 b0{ sx - w * 1.05f, -0.71f + bDrop, Z_FRONT - 0.01f };
        Vec3 b1{ sx + w * 1.05f, -0.71f + bDrop, Z_FRONT - 0.01f };
        Vec3 b2{ sx + w * 1.02f, -0.61f + bDrop, Z_FRONT + 0.06f };
        Vec3 b3{ sx - w * 1.02f, -0.61f + bDrop, Z_FRONT + 0.06f };
        ProjVert pb0 = xf.project(b0), pb1 = xf.project(b1), pb2 = xf.project(b2), pb3 = xf.project(b3);
        Color bCol = bumperPressed ? Color{ 0, 235, 255 } : Color{ 44, 55, 74 };
        fill_quad(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                  Vec2{ pb0.sx, pb0.sy }, Vec2{ pb1.sx, pb1.sy }, Vec2{ pb2.sx, pb2.sy }, Vec2{ pb3.sx, pb3.sy }, bCol);
        if (bumperPressed || pulseB > 0.0f) {
            float gx = (pb0.sx + pb1.sx) * 0.5f;
            float gy = (pb0.sy + pb2.sy) * 0.5f;
            if (bumperPressed) draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, gx, gy, 34.0f, Color{ 0, 235, 255 }, 165, true);
            if (pulseB > 0.0f) {
                draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, gx, gy,
                                  22.0f + (1.0f - pulseB) * 30.0f, 12.0f + (1.0f - pulseB) * 16.0f,
                                  2.5f, Color{ 0, 235, 255 }, (uint8_t)(pulseB * 220), true);
            }
        }
    };

    draw_shoulder_assembly(true, bL1, l2Frac, bL2, m_pulse[8], m_pulse[10]);
    draw_shoulder_assembly(false, bR1, r2Frac, bR2, m_pulse[9], m_pulse[11]);

    // Draw side extrusion walls (only back-facing/visible edges based on winding/depth)
    for (int i = 0; i < 18; i++) {
        int j = (i + 1) % 18;
        float avgZ = (pFront[i].depth + pBack[i].depth + pFront[j].depth + pBack[j].depth) * 0.25f;
        Vec2 edge{ contour[j].x - contour[i].x, contour[j].y - contour[i].y };
        float len = sqrtf(edge.x * edge.x + edge.y * edge.y);
        Vec3 nLocal{ edge.y / (len + 0.001f), -edge.x / (len + 0.001f), 0.15f };
        Vec3 nWorld = xf.rotate_normal(nLocal);
        if (nWorld.z < 0.35f || avgZ > -0.35f) {
            Color wallCol = shade_surface(shellSide, nWorld, lbColor);
            fill_quad(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                      Vec2{ pFront[i].sx, pFront[i].sy },
                      Vec2{ pFront[j].sx, pFront[j].sy },
                      Vec2{ pBack[j].sx,  pBack[j].sy },
                      Vec2{ pBack[i].sx,  pBack[i].sy }, wallCol);
        }
    }

    // DualSense Rumble Motor Grip Glow (when rumblePhase > 0)
    if (rumblePhase == 1 || rumblePhase == 3) {
        ProjVert lg = xf.project(Vec3{ -1.24f, 0.72f, 0.02f });
        draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, lg.sx, lg.sy, 68.0f, Color{ 0, 230, 255 }, 130, true);
    }
    if (rumblePhase == 2 || rumblePhase == 3) {
        ProjVert rg = xf.project(Vec3{ 1.24f, 0.72f, 0.02f });
        draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, rg.sx, rg.sy, 68.0f, Color{ 56, 189, 248 }, 130, true);
    }

    // Draw Shaded Front Chassis Face
    Vec3 frontNormal = xf.rotate_normal(Vec3{ 0.0f, -0.15f, -1.0f });
    Color shadedFront = shade_surface(shellFront, frontNormal, lbColor);
    ProjVert pCenter = xf.project(Vec3{ 0.0f, -0.05f, Z_FRONT });
    for (int i = 0; i < 18; i++) {
        int j = (i + 1) % 18;
        fill_triangle(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                      Vec2{ pCenter.sx, pCenter.sy },
                      Vec2{ pFront[i].sx, pFront[i].sy },
                      Vec2{ pFront[j].sx, pFront[j].sy }, shadedFront);
    }

    // Specular rim contour highlight on front face
    Color rimHighlight = isDualSense ? Color{ 255, 255, 255 } : Color{ 82, 104, 138 };
    for (int i = 0; i < 18; i++) {
        int j = (i + 1) % 18;
        draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                     pFront[i].sx, pFront[i].sy, pFront[j].sx, pFront[j].sy,
                     2.0f, rimHighlight, 130);
    }

    // DualSense Black Center Core Trim Wing (or DS4 subtle dark inset bridge)
    if (isDualSense) {
        Vec3 cVerts[6] = {
            { -0.58f, -0.66f, Z_FRONT - 0.008f },
            {  0.58f, -0.66f, Z_FRONT - 0.008f },
            {  0.66f,  0.45f, Z_FRONT - 0.008f },
            {  0.58f,  0.46f, Z_FRONT - 0.008f },
            { -0.58f,  0.46f, Z_FRONT - 0.008f },
            { -0.66f,  0.45f, Z_FRONT - 0.008f }
        };
        ProjVert pc[6];
        for (int i = 0; i < 6; i++) pc[i] = xf.project(cVerts[i]);
        Color coreShaded = shade_surface(gripCore, frontNormal, lbColor);
        fill_quad(fb, fbW, fbH, cx1, cy1, cx2, cy2,
                  Vec2{ pc[0].sx, pc[0].sy }, Vec2{ pc[1].sx, pc[1].sy }, Vec2{ pc[2].sx, pc[2].sy }, Vec2{ pc[5].sx, pc[5].sy }, coreShaded);
    }

    // =========================================================================
    // 4. TOUCHPAD + REACTIVE RGB LIGHTBARS + PLAYER LEDS + TOUCH DOTS
    // =========================================================================
    bool tpClicked = (btns & ORBIS_PAD_BUTTON_TOUCH_PAD) != 0;
    float tpDrop = tpClicked ? 0.025f : 0.0f;
    float tpTopW = isDualSense ? 0.52f : 0.46f;
    float tpBotW = isDualSense ? 0.45f : 0.46f;
    float tpTopY = -0.64f;
    float tpBotY = -0.14f;

    ProjVert tp0 = xf.project(Vec3{ -tpTopW, tpTopY, Z_FRONT - 0.025f + tpDrop });
    ProjVert tp1 = xf.project(Vec3{  tpTopW, tpTopY, Z_FRONT - 0.025f + tpDrop });
    ProjVert tp2 = xf.project(Vec3{  tpBotW, tpBotY, Z_FRONT - 0.025f + tpDrop });
    ProjVert tp3 = xf.project(Vec3{ -tpBotW, tpBotY, Z_FRONT - 0.025f + tpDrop });

    Color tpColor = tpClicked ? Color{ 0, 225, 190 } : (isDualSense ? Color{ 234, 240, 250 } : Color{ 18, 24, 34 });
    if (!tpClicked && padData.touch.fingers > 0) tpColor = lerp_color(tpColor, Color{ 0, 195, 255 }, 0.35f);
    fill_quad(fb, fbW, fbH, cx1, cy1, cx2, cy2,
              Vec2{ tp0.sx, tp0.sy }, Vec2{ tp1.sx, tp1.sy }, Vec2{ tp2.sx, tp2.sy }, Vec2{ tp3.sx, tp3.sy },
              shade_surface(tpColor, frontNormal, lbColor));

    // RGB Lightbar Strips
    if (isDualSense) {
        draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, tp0.sx - 2, tp0.sy, tp3.sx - 2, tp3.sy, 4.5f, lbColor, 245);
        draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, tp1.sx + 2, tp1.sy, tp2.sx + 2, tp2.sy, 4.5f, lbColor, 245);
        draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, (tp0.sx + tp3.sx) * 0.5f, (tp0.sy + tp3.sy) * 0.5f, 34.0f, lbColor, 110, true);
        draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, (tp1.sx + tp2.sx) * 0.5f, (tp1.sy + tp2.sy) * 0.5f, 34.0f, lbColor, 110, true);

        // 5 Player White LEDs under Touchpad
        for (int i = -2; i <= 2; i++) {
            ProjVert pl = xf.project(Vec3{ i * 0.065f, -0.08f, Z_FRONT - 0.02f });
            draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, pl.sx, pl.sy, 3.0f, 2.0f, Color{ 225, 248, 255 }, 235);
        }
    } else {
        // DualShock 4 Front & Slit Lightbar
        ProjVert lbL = xf.project(Vec3{ -0.42f, -0.68f, Z_FRONT - 0.02f });
        ProjVert lbR = xf.project(Vec3{  0.42f, -0.68f, Z_FRONT - 0.02f });
        draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, lbL.sx, lbL.sy, lbR.sx, lbR.sy, 6.0f, lbColor, 250);
        draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, tp0.sx + 8, tp0.sy + 3, tp1.sx - 8, tp1.sy + 3, 2.5f, lbColor, 220);
        draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, (lbL.sx + lbR.sx) * 0.5f, (lbL.sy + lbR.sy) * 0.5f, 48.0f, lbColor, 120, true);
    }

    // Live Multi-touch fingers mapped straight onto the 3D touchpad plane
    for (int f = 0; f < (int)padData.touch.fingers && f < 2; f++) {
        float u = clampf((float)padData.touch.touch[f].x / 1920.0f, 0.0f, 1.0f);
        float v = clampf((float)padData.touch.touch[f].y / 942.0f, 0.0f, 1.0f);
        float lx = (-tpTopW + 2.0f * tpTopW * u) * (1.0f - v * 0.1f);
        float ly = tpTopY + (tpBotY - tpTopY) * v;
        ProjVert ptDot = xf.project(Vec3{ lx, ly, Z_FRONT - 0.032f + tpDrop });
        Color fCol = (f == 0) ? Color{ 0, 245, 255 } : Color{ 255, 190, 20 };
        draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, ptDot.sx, ptDot.sy, 16.0f, fCol, 180, true);
        draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, ptDot.sx, ptDot.sy, 5.5f, 4.5f, Color{ 255, 255, 255 }, 250);
    }
    if (tpClicked || m_pulse[17] > 0.0f) {
        ProjVert tpc = xf.project(Vec3{ 0.0f, -0.39f, Z_FRONT - 0.035f });
        if (tpClicked) draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, tpc.sx, tpc.sy, 54.0f, Color{ 0, 255, 195 }, 135, true);
        if (m_pulse[17] > 0.0f) {
            draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, tpc.sx, tpc.sy,
                              44.0f + (1.0f - m_pulse[17]) * 48.0f,
                              24.0f + (1.0f - m_pulse[17]) * 26.0f,
                              2.5f, Color{ 0, 255, 195 }, (uint8_t)(m_pulse[17] * 220), true);
        }
    }

    // =========================================================================
    // 5. D-PAD (CRUCETA 4 DIRECCIONES)
    // =========================================================================
    float dpCX = -0.96f;
    float dpCY = -0.14f;
    auto draw_dpad_arm = [&](float dx, float dy, bool pressed, float pulse, int dir) {
        float drop = pressed ? 0.025f : 0.0f;
        ProjVert p = xf.project(Vec3{ dpCX + dx, dpCY + dy, Z_FRONT - 0.032f + drop });
        Color col = pressed ? Color{ 0, 235, 255 } : Color{ 38, 48, 66 };
        if (pressed) draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 24.0f, Color{ 0, 235, 255 }, 185, true);
        draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 11.5f, 9.5f, col, 255);
        draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 11.5f, 9.5f, 1.5f,
                          pressed ? Color{ 220, 255, 255 } : Color{ 80, 96, 122 }, 200);
        // Tiny directional arrow tick
        Color tickCol = pressed ? Color{ 255, 255, 255 } : Color{ 165, 185, 210 };
        if (dir == 0) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 4, p.sy + 2, p.sx, p.sy - 3, 1.5f, tickCol);
        if (dir == 0) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 4, p.sy + 2, p.sx, p.sy - 3, 1.5f, tickCol);
        if (dir == 1) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 4, p.sy - 2, p.sx, p.sy + 3, 1.5f, tickCol);
        if (dir == 1) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 4, p.sy - 2, p.sx, p.sy + 3, 1.5f, tickCol);
        if (dir == 2) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 2, p.sy - 4, p.sx - 3, p.sy, 1.5f, tickCol);
        if (dir == 2) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 2, p.sy + 4, p.sx - 3, p.sy, 1.5f, tickCol);
        if (dir == 3) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 2, p.sy - 4, p.sx + 3, p.sy, 1.5f, tickCol);
        if (dir == 3) draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 2, p.sy + 4, p.sx + 3, p.sy, 1.5f, tickCol);

        if (pulse > 0.0f) {
            draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy,
                              12.0f + (1.0f - pulse) * 24.0f, 10.0f + (1.0f - pulse) * 20.0f,
                              2.5f, Color{ 0, 235, 255 }, (uint8_t)(pulse * 225), true);
        }
    };

    ProjVert dpMid = xf.project(Vec3{ dpCX, dpCY, Z_FRONT - 0.02f });
    draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, dpMid.sx, dpMid.sy, 32.0f, 26.0f, Color{ 16, 22, 34 }, 215);
    draw_dpad_arm(0.0f, -0.17f, (btns & ORBIS_PAD_BUTTON_UP) != 0,    m_pulse[4], 0);
    draw_dpad_arm(0.0f,  0.17f, (btns & ORBIS_PAD_BUTTON_DOWN) != 0,  m_pulse[5], 1);
    draw_dpad_arm(-0.17f, 0.0f, (btns & ORBIS_PAD_BUTTON_LEFT) != 0,  m_pulse[6], 2);
    draw_dpad_arm(0.17f,  0.0f, (btns & ORBIS_PAD_BUTTON_RIGHT) != 0, m_pulse[7], 3);

    // =========================================================================
    // 6. FACE BUTTONS (✕ ○ □ △) WITH AUTHENTIC SYMBOLS & GLOW SHOCKWAVES
    // =========================================================================
    float fbCX = 0.96f;
    float fbCY = -0.14f;
    auto draw_face_btn = [&](float dx, float dy, bool pressed, float pulse, Color neon, int kind) {
        float drop = pressed ? 0.028f : 0.0f;
        ProjVert p = xf.project(Vec3{ fbCX + dx, fbCY + dy, Z_FRONT - 0.036f + drop });
        if (pressed) {
            draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 26.0f, neon, 200, true);
        }
        Color capCol = pressed ? neon : Color{ 28, 35, 48 };
        draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 12.5f, 10.5f, capCol, 255);
        draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 12.5f, 10.5f, 1.5f,
                          pressed ? Color{ 255, 255, 255 } : neon, pressed ? 240 : 120);

        Color symCol = pressed ? Color{ 255, 255, 255 } : neon;
        if (kind == 0) { // Cross X
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 5, p.sy - 4, p.sx + 5, p.sy + 4, 1.8f, symCol);
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 5, p.sy - 4, p.sx - 5, p.sy + 4, 1.8f, symCol);
        } else if (kind == 1) { // Circle O
            draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 5.5f, 4.6f, 2.0f, symCol, 255);
        } else if (kind == 2) { // Square []
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 5, p.sy - 4, p.sx + 5, p.sy - 4, 1.8f, symCol);
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 5, p.sy - 4, p.sx + 5, p.sy + 4, 1.8f, symCol);
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 5, p.sy + 4, p.sx - 5, p.sy + 4, 1.8f, symCol);
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 5, p.sy + 4, p.sx - 5, p.sy - 4, 1.8f, symCol);
        } else { // Triangle /\
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy - 5, p.sx + 5.5f, p.sy + 4, 1.8f, symCol);
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx + 5.5f, p.sy + 4, p.sx - 5.5f, p.sy + 4, 1.8f, symCol);
            draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx - 5.5f, p.sy + 4, p.sx, p.sy - 5, 1.8f, symCol);
        }

        if (pulse > 0.0f) {
            draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy,
                              13.0f + (1.0f - pulse) * 28.0f, 11.0f + (1.0f - pulse) * 24.0f,
                              2.5f, neon, (uint8_t)(pulse * 235), true);
        }
    };

    draw_face_btn(0.0f,  0.19f, (btns & ORBIS_PAD_BUTTON_CROSS) != 0,    m_pulse[0], Color{ 59, 130, 246 }, 0);
    draw_face_btn(0.20f, 0.0f,  (btns & ORBIS_PAD_BUTTON_CIRCLE) != 0,   m_pulse[1], Color{ 239, 68, 68 },  1);
    draw_face_btn(-0.20f,0.0f,  (btns & ORBIS_PAD_BUTTON_SQUARE) != 0,   m_pulse[2], Color{ 236, 72, 153 }, 2);
    draw_face_btn(0.0f, -0.19f, (btns & ORBIS_PAD_BUTTON_TRIANGLE) != 0, m_pulse[3], Color{ 16, 185, 129 }, 3);

    // =========================================================================
    // 7. SYSTEM BUTTONS (SHARE/CREATE, OPTIONS, PS BUTTON)
    // =========================================================================
    auto draw_sys_pill = [&](float x, float y, bool pressed, float pulse, float r, Color neon) {
        ProjVert p = xf.project(Vec3{ x, y, Z_FRONT - 0.025f + (pressed ? 0.02f : 0.0f) });
        if (pressed) draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, 22.0f, neon, 190, true);
        draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, r, r * 0.72f,
                            pressed ? neon : (isDualSense ? Color{ 210, 220, 236 } : Color{ 44, 56, 75 }), 255);
        draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy, r, r * 0.72f, 1.2f, neon, pressed ? 240 : 90);
        if (pulse > 0.0f) {
            draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, p.sx, p.sy,
                              r + (1.0f - pulse) * 22.0f, r * 0.72f + (1.0f - pulse) * 16.0f,
                              2.0f, neon, (uint8_t)(pulse * 220), true);
        }
    };

    draw_sys_pill(-0.62f, -0.45f, (btns & 0x00000001U) != 0,              m_pulse[14], 7.0f,  Color{ 0, 225, 255 });
    draw_sys_pill( 0.62f, -0.45f, (btns & ORBIS_PAD_BUTTON_OPTIONS) != 0, m_pulse[15], 7.0f,  Color{ 0, 225, 255 });
    draw_sys_pill( 0.0f,   0.20f, (btns & 0x00010000U) != 0,              m_pulse[16], 11.5f, Color{ 0, 165, 255 });

    // =========================================================================
    // 8. 3D TILTING ANALOG THUMBSTICKS (L3 & R3)
    // =========================================================================
    auto draw_stick_3d = [&](float bx, float by, uint8_t rawX, uint8_t rawY, bool clicked, float pulse) {
        float nx = ((float)rawX - 128.0f) / 128.0f;
        float ny = ((float)rawY - 128.0f) / 128.0f;
        float mag = clampf(sqrtf(nx * nx + ny * ny), 0.0f, 1.0f);

        // Concave well base & reactive halo ring
        ProjVert well = xf.project(Vec3{ bx, by, Z_FRONT - 0.015f });
        Color haloCol = clicked ? Color{ 255, 185, 30 } : Color{ 0, 225, 255 };
        draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, well.sx, well.sy, 26.0f, 21.0f, Color{ 10, 14, 24 }, 240);
        draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, well.sx, well.sy, 26.5f, 21.5f, 2.2f,
                          haloCol, (uint8_t)(70 + (clicked ? 185 : mag * 170)), true);

        // Tilted 3D stick shaft & top concave mushroom cap
        float tipHeight = clicked ? 0.14f : 0.21f;
        float tiltX = bx + nx * 0.14f;
        float tiltY = by + ny * 0.14f;
        ProjVert cap = xf.project(Vec3{ tiltX, tiltY, Z_FRONT - tipHeight });

        // Stem cylinder shaft
        draw_line_aa(fb, fbW, fbH, cx1, cy1, cx2, cy2, well.sx, well.sy, cap.sx, cap.sy, 11.0f, Color{ 20, 26, 38 }, 255);

        if (clicked || mag > 0.15f) {
            draw_glow_disc(fb, fbW, fbH, cx1, cy1, cx2, cy2, cap.sx, cap.sy, 30.0f, haloCol, (uint8_t)(clicked ? 175 : mag * 110), true);
        }

        Color capCol = clicked ? Color{ 255, 190, 45 } : Color{ 32, 41, 58 };
        draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, cap.sx, cap.sy, 22.0f, 18.0f, capCol, 255);
        draw_ellipse_filled(fb, fbW, fbH, cx1, cy1, cx2, cy2, cap.sx, cap.sy, 15.0f, 12.0f, Color{ 17, 23, 34 }, 230);
        draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, cap.sx, cap.sy, 22.0f, 18.0f, 2.0f,
                          clicked ? Color{ 255, 240, 180 } : lerp_color(Color{ 85, 100, 125 }, Color{ 0, 235, 255 }, mag), 230);

        if (pulse > 0.0f) {
            draw_ring_ellipse(fb, fbW, fbH, cx1, cy1, cx2, cy2, cap.sx, cap.sy,
                              23.0f + (1.0f - pulse) * 32.0f, 19.0f + (1.0f - pulse) * 26.0f,
                              2.5f, Color{ 255, 190, 45 }, (uint8_t)(pulse * 235), true);
        }
    };

    draw_stick_3d(-0.44f, 0.22f, padData.leftStick.x,  padData.leftStick.y,  (btns & ORBIS_PAD_BUTTON_L3) != 0, m_pulse[12]);
    draw_stick_3d( 0.44f, 0.22f, padData.rightStick.x, padData.rightStick.y, (btns & ORBIS_PAD_BUTTON_R3) != 0, m_pulse[13]);
}
