#include "controller_3d.h"
#include <math.h>
#include <string.h>

namespace {

struct Vec2 {
    float x, y;
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

struct ClipRect {
    int x1, y1, x2, y2;
};

static inline void blend_px(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                            int x, int y, Color c, uint8_t alpha) {
    if (alpha == 0 || x < cl.x1 || x > cl.x2 || y < cl.y1 || y > cl.y2) return;
    if (x < 0 || x >= fbW || y < 0 || y >= fbH) return;
    int idx = y * fbW + x;
    if (alpha >= 255) {
        fb[idx] = 0x80000000U | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b;
        return;
    }
    uint32_t bg = fb[idx];
    uint32_t bgR = (bg >> 16) & 0xFF;
    uint32_t bgG = (bg >> 8) & 0xFF;
    uint32_t bgB = bg & 0xFF;
    uint32_t inv = 255U - alpha;
    uint32_t r = (c.r * alpha + bgR * inv) / 255U;
    uint32_t g = (c.g * alpha + bgG * inv) / 255U;
    uint32_t b = (c.b * alpha + bgB * inv) / 255U;
    fb[idx] = 0x80000000U | (r << 16) | (g << 8) | b;
}

static inline void add_px(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                          int x, int y, Color c, uint8_t alpha) {
    if (alpha == 0 || x < cl.x1 || x > cl.x2 || y < cl.y1 || y > cl.y2) return;
    if (x < 0 || x >= fbW || y < 0 || y >= fbH) return;
    int idx = y * fbW + x;
    uint32_t bg = fb[idx];
    uint32_t bgR = (bg >> 16) & 0xFF;
    uint32_t bgG = (bg >> 8) & 0xFF;
    uint32_t bgB = bg & 0xFF;
    uint32_t r = bgR + (c.r * alpha) / 255U; if (r > 255U) r = 255U;
    uint32_t g = bgG + (c.g * alpha) / 255U; if (g > 255U) g = 255U;
    uint32_t b = bgB + (c.b * alpha) / 255U; if (b > 255U) b = 255U;
    fb[idx] = 0x80000000U | (r << 16) | (g << 8) | b;
}

static void fill_circle_aa(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                           float cx, float cy, float r, Color col, uint8_t alpha = 255) {
    int x0 = (int)floorf(cx - r - 1.5f);
    int x1 = (int)ceilf(cx + r + 1.5f);
    int y0 = (int)floorf(cy - r - 1.5f);
    int y1 = (int)ceilf(cy + r + 1.5f);
    if (x0 < cl.x1) x0 = cl.x1;
    if (x1 > cl.x2) x1 = cl.x2;
    if (y0 < cl.y1) y0 = cl.y1;
    if (y1 > cl.y2) y1 = cl.y2;

    for (int y = y0; y <= y1; y++) {
        float dy = (float)y - cy;
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - cx;
            float d = sqrtf(dx * dx + dy * dy);
            float cov = clampf(r + 0.5f - d, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, col, (uint8_t)(alpha * cov));
            }
        }
    }
}

static void fill_circle_shaded(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                               float cx, float cy, float r, Color topCol, Color botCol, uint8_t alpha = 255) {
    int x0 = (int)floorf(cx - r - 1.5f);
    int x1 = (int)ceilf(cx + r + 1.5f);
    int y0 = (int)floorf(cy - r - 1.5f);
    int y1 = (int)ceilf(cy + r + 1.5f);
    if (x0 < cl.x1) x0 = cl.x1;
    if (x1 > cl.x2) x1 = cl.x2;
    if (y0 < cl.y1) y0 = cl.y1;
    if (y1 > cl.y2) y1 = cl.y2;

    for (int y = y0; y <= y1; y++) {
        float dy = (float)y - cy;
        float t = clampf((dy + r) / (2.0f * r + 0.001f), 0.0f, 1.0f);
        Color rowCol = lerp_color(topCol, botCol, t);
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - cx;
            float d = sqrtf(dx * dx + dy * dy);
            float cov = clampf(r + 0.5f - d, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, rowCol, (uint8_t)(alpha * cov));
            }
        }
    }
}

static void stroke_circle_aa(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                             float cx, float cy, float r, float thickness, Color col, uint8_t alpha = 255) {
    float halfT = thickness * 0.5f;
    float rOut = r + halfT + 1.5f;
    int x0 = (int)floorf(cx - rOut);
    int x1 = (int)ceilf(cx + rOut);
    int y0 = (int)floorf(cy - rOut);
    int y1 = (int)ceilf(cy + rOut);
    if (x0 < cl.x1) x0 = cl.x1;
    if (x1 > cl.x2) x1 = cl.x2;
    if (y0 < cl.y1) y0 = cl.y1;
    if (y1 > cl.y2) y1 = cl.y2;

    for (int y = y0; y <= y1; y++) {
        float dy = (float)y - cy;
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - cx;
            float d = fabsf(sqrtf(dx * dx + dy * dy) - r);
            float cov = clampf(halfT + 0.6f - d, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, col, (uint8_t)(alpha * cov));
            }
        }
    }
}

static void draw_glow_disc(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                           float cx, float cy, float r, Color col, uint8_t peakAlpha) {
    int x0 = (int)floorf(cx - r);
    int x1 = (int)ceilf(cx + r);
    int y0 = (int)floorf(cy - r);
    int y1 = (int)ceilf(cy + r);
    if (x0 < cl.x1) x0 = cl.x1;
    if (x1 > cl.x2) x1 = cl.x2;
    if (y0 < cl.y1) y0 = cl.y1;
    if (y1 > cl.y2) y1 = cl.y2;
    for (int y = y0; y <= y1; y++) {
        float dy = (float)y - cy;
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - cx;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < r) {
                float norm = 1.0f - (d / r);
                uint8_t a = (uint8_t)(peakAlpha * norm * norm);
                add_px(fb, fbW, fbH, cl, x, y, col, a);
            }
        }
    }
}

static float dist_point_to_seg(float px, float py, float x0, float y0, float x1, float y1) {
    float vx = x1 - x0;
    float vy = y1 - y0;
    float len2 = vx * vx + vy * vy;
    if (len2 < 0.0001f) {
        float dx = px - x0, dy = py - y0;
        return sqrtf(dx * dx + dy * dy);
    }
    float t = clampf(((px - x0) * vx + (py - y0) * vy) / len2, 0.0f, 1.0f);
    float projX = x0 + vx * t;
    float projY = y0 + vy * t;
    float dx = px - projX, dy = py - projY;
    return sqrtf(dx * dx + dy * dy);
}

static void draw_capsule_shaded(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                                float x0, float y0, float x1, float y1,
                                float r0, float r1,
                                Color topCol, Color botCol, uint8_t alpha = 255) {
    float maxR = (r0 > r1) ? r0 : r1;
    int bx0 = (int)floorf(((x0 < x1) ? x0 : x1) - maxR - 2.0f);
    int bx1 = (int)ceilf(((x0 > x1) ? x0 : x1) + maxR + 2.0f);
    int by0 = (int)floorf(((y0 < y1) ? y0 : y1) - maxR - 2.0f);
    int by1 = (int)ceilf(((y0 > y1) ? y0 : y1) + maxR + 2.0f);
    if (bx0 < cl.x1) bx0 = cl.x1;
    if (bx1 > cl.x2) bx1 = cl.x2;
    if (by0 < cl.y1) by0 = cl.y1;
    if (by1 > cl.y2) by1 = cl.y2;

    float vx = x1 - x0;
    float vy = y1 - y0;
    float len2 = vx * vx + vy * vy;
    float spanY = (float)(by1 - by0 + 1);

    for (int y = by0; y <= by1; y++) {
        float fy = clampf((float)(y - by0) / spanY, 0.0f, 1.0f);
        Color rowCol = lerp_color(topCol, botCol, fy);
        for (int x = bx0; x <= bx1; x++) {
            float t = (len2 > 0.0001f) ? clampf((((float)x - x0) * vx + ((float)y - y0) * vy) / len2, 0.0f, 1.0f) : 0.0f;
            float projX = x0 + vx * t;
            float projY = y0 + vy * t;
            float rad = r0 + (r1 - r0) * t;
            float dx = (float)x - projX;
            float dy = (float)y - projY;
            float d = sqrtf(dx * dx + dy * dy);
            float cov = clampf(rad + 0.6f - d, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, rowCol, (uint8_t)(alpha * cov));
            }
        }
    }
}

static void draw_line_aa(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                         float x0, float y0, float x1, float y1,
                         float thickness, Color col, uint8_t alpha = 255) {
    float halfT = thickness * 0.5f;
    int bx0 = (int)floorf(((x0 < x1) ? x0 : x1) - halfT - 2.0f);
    int bx1 = (int)ceilf(((x0 > x1) ? x0 : x1) + halfT + 2.0f);
    int by0 = (int)floorf(((y0 < y1) ? y0 : y1) - halfT - 2.0f);
    int by1 = (int)ceilf(((y0 > y1) ? y0 : y1) + halfT + 2.0f);
    if (bx0 < cl.x1) bx0 = cl.x1;
    if (bx1 > cl.x2) bx1 = cl.x2;
    if (by0 < cl.y1) by0 = cl.y1;
    if (by1 > cl.y2) by1 = cl.y2;

    for (int y = by0; y <= by1; y++) {
        for (int x = bx0; x <= bx1; x++) {
            float d = dist_point_to_seg((float)x, (float)y, x0, y0, x1, y1);
            float cov = clampf(halfT + 0.6f - d, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, col, (uint8_t)(alpha * cov));
            }
        }
    }
}

static void draw_round_rect_aa(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                               float rx, float ry, float rw, float rh, float rad,
                               Color topCol, Color botCol, uint8_t alpha = 255) {
    if (rw <= 0.0f || rh <= 0.0f) return;
    float r = clampf(rad, 0.0f, ((rw < rh ? rw : rh) * 0.5f));
    float cx = rx + rw * 0.5f;
    float cy = ry + rh * 0.5f;
    float hx = rw * 0.5f - r;
    float hy = rh * 0.5f - r;

    int x0 = (int)floorf(rx - 1.5f);
    int x1 = (int)ceilf(rx + rw + 1.5f);
    int y0 = (int)floorf(ry - 1.5f);
    int y1 = (int)ceilf(ry + rh + 1.5f);
    if (x0 < cl.x1) x0 = cl.x1;
    if (x1 > cl.x2) x1 = cl.x2;
    if (y0 < cl.y1) y0 = cl.y1;
    if (y1 > cl.y2) y1 = cl.y2;

    for (int y = y0; y <= y1; y++) {
        float t = clampf(((float)y - ry) / (rh + 0.001f), 0.0f, 1.0f);
        Color rowCol = lerp_color(topCol, botCol, t);
        float qy = fabsf((float)y - cy) - hy;
        for (int x = x0; x <= x1; x++) {
            float qx = fabsf((float)x - cx) - hx;
            float ax = (qx > 0.0f) ? qx : 0.0f;
            float ay = (qy > 0.0f) ? qy : 0.0f;
            float outside = sqrtf(ax * ax + ay * ay);
            float inside = (qx > qy) ? qx : qy;
            if (inside > 0.0f) inside = 0.0f;
            float sdf = outside + inside - r;
            float cov = clampf(0.6f - sdf, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, rowCol, (uint8_t)(alpha * cov));
            }
        }
    }
}

static void stroke_round_rect_aa(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                                 float rx, float ry, float rw, float rh, float rad,
                                 float thickness, Color col, uint8_t alpha = 255) {
    if (rw <= 0.0f || rh <= 0.0f) return;
    float r = clampf(rad, 0.0f, ((rw < rh ? rw : rh) * 0.5f));
    float cx = rx + rw * 0.5f;
    float cy = ry + rh * 0.5f;
    float hx = rw * 0.5f - r;
    float hy = rh * 0.5f - r;
    float halfT = thickness * 0.5f;

    int x0 = (int)floorf(rx - halfT - 1.5f);
    int x1 = (int)ceilf(rx + rw + halfT + 1.5f);
    int y0 = (int)floorf(ry - halfT - 1.5f);
    int y1 = (int)ceilf(ry + rh + halfT + 1.5f);
    if (x0 < cl.x1) x0 = cl.x1;
    if (x1 > cl.x2) x1 = cl.x2;
    if (y0 < cl.y1) y0 = cl.y1;
    if (y1 > cl.y2) y1 = cl.y2;

    for (int y = y0; y <= y1; y++) {
        float qy = fabsf((float)y - cy) - hy;
        for (int x = x0; x <= x1; x++) {
            float qx = fabsf((float)x - cx) - hx;
            float ax = (qx > 0.0f) ? qx : 0.0f;
            float ay = (qy > 0.0f) ? qy : 0.0f;
            float outside = sqrtf(ax * ax + ay * ay);
            float inside = (qx > qy) ? qx : qy;
            if (inside > 0.0f) inside = 0.0f;
            float sdf = fabsf(outside + inside - r);
            float cov = clampf(halfT + 0.6f - sdf, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, col, (uint8_t)(alpha * cov));
            }
        }
    }
}

static void fill_poly_shaded(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                             const Vec2* pts, int count, Color topCol, Color botCol, uint8_t alpha = 255) {
    if (count < 3) return;
    float minY = pts[0].y, maxY = pts[0].y;
    for (int i = 1; i < count; i++) {
        if (pts[i].y < minY) minY = pts[i].y;
        if (pts[i].y > maxY) maxY = pts[i].y;
    }
    int yStart = (int)ceilf(minY);
    int yEnd   = (int)floorf(maxY);
    if (yStart < cl.y1) yStart = cl.y1;
    if (yEnd   > cl.y2) yEnd   = cl.y2;

    float nodeX[32];
    float spanY = (maxY - minY > 0.001f) ? (maxY - minY) : 1.0f;

    for (int y = yStart; y <= yEnd; y++) {
        float fy = (float)y;
        int nodes = 0;
        int j = count - 1;
        for (int i = 0; i < count; i++) {
            if (((pts[i].y < fy && pts[j].y >= fy) || (pts[j].y < fy && pts[i].y >= fy)) && nodes < 32) {
                nodeX[nodes++] = pts[i].x + (fy - pts[i].y) / (pts[j].y - pts[i].y) * (pts[j].x - pts[i].x);
            }
            j = i;
        }
        for (int i = 0; i < nodes - 1; i++) {
            for (int k = i + 1; k < nodes; k++) {
                if (nodeX[k] < nodeX[i]) {
                    float tmp = nodeX[i]; nodeX[i] = nodeX[k]; nodeX[k] = tmp;
                }
            }
        }
        Color rowCol = lerp_color(topCol, botCol, clampf((fy - minY) / spanY, 0.0f, 1.0f));
        for (int i = 0; i + 1 < nodes; i += 2) {
            int xs = (int)ceilf(nodeX[i]);
            int xe = (int)floorf(nodeX[i + 1]);
            if (xs < cl.x1) xs = cl.x1;
            if (xe > cl.x2) xe = cl.x2;
            if (xs <= xe) {
                for (int x = xs; x <= xe; x++) {
                    blend_px(fb, fbW, fbH, cl, x, y, rowCol, alpha);
                }
            }
        }
    }
}

static void fill_ellipse_shaded(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                                float cx, float cy, float rx, float ry,
                                Color topCol, Color botCol, uint8_t alpha = 255) {
    int x0 = (int)floorf(cx - rx - 1.5f);
    int x1 = (int)ceilf(cx + rx + 1.5f);
    int y0 = (int)floorf(cy - ry - 1.5f);
    int y1 = (int)ceilf(cy + ry + 1.5f);
    if (x0 < cl.x1) x0 = cl.x1;
    if (x1 > cl.x2) x1 = cl.x2;
    if (y0 < cl.y1) y0 = cl.y1;
    if (y1 > cl.y2) y1 = cl.y2;

    float minR = (rx < ry) ? rx : ry;
    for (int y = y0; y <= y1; y++) {
        float dy = ((float)y - cy) / (ry + 0.001f);
        float t = clampf((dy + 1.0f) * 0.5f, 0.0f, 1.0f);
        Color rowCol = lerp_color(topCol, botCol, t);
        for (int x = x0; x <= x1; x++) {
            float dx = ((float)x - cx) / (rx + 0.001f);
            float e = sqrtf(dx * dx + dy * dy);
            float cov = clampf((1.0f - e) * minR + 0.5f, 0.0f, 1.0f);
            if (cov > 0.0f) {
                blend_px(fb, fbW, fbH, cl, x, y, rowCol, (uint8_t)(alpha * cov));
            }
        }
    }
}

// Compute live lightbar color matching PS5 DualSense & PS4 DualShock 4 settings
static Color compute_lightbar_color(bool isDualSense, const AppConfig& cfg, float timeSec, int battPct) {
    static const Color dsCols[6] = {
        { 0, 112, 220 },  // Azul PlayStation
        { 239, 68, 68 },  // Rojo
        { 16, 200, 110 }, // Verde
        { 168, 85, 247 }, // Purpura
        { 6, 220, 255 },  // Cian
        { 245, 248, 252 } // Blanco
    };
    static const Color ds4Cols[7] = {
        { 0, 112, 220 },  // Azul PS4
        { 239, 68, 68 },  // Rojo Carmesi
        { 16, 185, 129 }, // Verde Esmeralda
        { 168, 85, 247 }, // Purpura Neon
        { 6, 182, 212 },  // Cian Electrico
        { 245, 158, 11 }, // Ambar Oro
        { 248, 250, 252 } // Blanco Puro
    };

    int mode = isDualSense ? cfg.ds_lightbar_mode : cfg.ds4_lightbar_mode;
    int colIdx = isDualSense ? cfg.ds_lightbar_color : cfg.ds4_lightbar_color;
    int br = isDualSense ? cfg.ds_brightness : cfg.ds4_brightness;

    Color base = { 0, 112, 220 };
    if (isDualSense) {
        if (colIdx >= 0 && colIdx < 6) base = dsCols[colIdx];
    } else {
        if (colIdx >= 0 && colIdx < 7) base = ds4Cols[colIdx];
    }

    if (mode == 3) {
        return Color{ 16, 20, 28 }; // Off
    } else if (mode == 2) {
        if (battPct <= 20) base = Color{ 239, 50, 50 };
        else if (battPct <= 55) base = Color{ 245, 175, 20 };
        else base = Color{ 16, 230, 130 };
    } else if (mode == 4) {
        float hue = fmodf(timeSec * 0.28f, 1.0f);
        base = hsv_to_rgb(hue, 0.92f, 1.0f);
    } else if (mode == 5) {
        float pulse = 0.22f + 0.78f * (0.5f + 0.5f * sinf(timeSec * 2.8f));
        base = Color{
            clampu8((int)(base.r * pulse)),
            clampu8((int)(base.g * pulse)),
            clampu8((int)(base.b * pulse))
        };
    }

    float scale = (br == 1) ? 0.65f : ((br == 2) ? 0.35f : 1.0f);
    return Color{
        clampu8((int)(base.r * scale)),
        clampu8((int)(base.g * scale)),
        clampu8((int)(base.b * scale))
    };
}

// Draw clean crisp mini bitmap labels ("L1", "L2", "R1", "R2", "SHARE", "CREATE", "OPTIONS", "PS")
static void draw_tiny_char(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                           int x, int y, int scale, char ch, Color col, uint8_t alpha = 255) {
    // 5x7 bitmaps encoded row by row (MSB bit 4 to bit 0)
    static const uint8_t glyphL[7] = { 0x10,0x10,0x10,0x10,0x10,0x10,0x1F };
    static const uint8_t glyphR[7] = { 0x1E,0x11,0x11,0x1E,0x14,0x12,0x11 };
    static const uint8_t glyph1[7] = { 0x04,0x0C,0x04,0x04,0x04,0x04,0x0E };
    static const uint8_t glyph2[7] = { 0x0E,0x11,0x01,0x06,0x08,0x10,0x1F };
    static const uint8_t glyph3[7] = { 0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E };
    static const uint8_t glyphP[7] = { 0x1E,0x11,0x11,0x1E,0x10,0x10,0x10 };
    static const uint8_t glyphS[7] = { 0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E };
    static const uint8_t glyphC[7] = { 0x0E,0x11,0x10,0x10,0x10,0x11,0x0E };
    static const uint8_t glyphO[7] = { 0x0E,0x11,0x11,0x11,0x11,0x11,0x0E };
    static const uint8_t glyphH[7] = { 0x11,0x11,0x11,0x1F,0x11,0x11,0x11 };
    static const uint8_t glyphA[7] = { 0x0E,0x11,0x11,0x1F,0x11,0x11,0x11 };
    static const uint8_t glyphE[7] = { 0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F };
    static const uint8_t glyphT[7] = { 0x1F,0x04,0x04,0x04,0x04,0x04,0x04 };
    static const uint8_t glyphN[7] = { 0x11,0x19,0x15,0x13,0x11,0x11,0x11 };
    static const uint8_t glyphI[7] = { 0x0E,0x04,0x04,0x04,0x04,0x04,0x0E };
    static const uint8_t glyphM[7] = { 0x11,0x1B,0x15,0x15,0x11,0x11,0x11 };
    static const uint8_t glyphU[7] = { 0x11,0x11,0x11,0x11,0x11,0x11,0x0E };
    static const uint8_t glyphDash[7] = { 0x00,0x00,0x00,0x1F,0x00,0x00,0x00 };

    const uint8_t* rows = NULL;
    switch (ch) {
        case 'L': rows = glyphL; break;
        case 'R': rows = glyphR; break;
        case '1': rows = glyph1; break;
        case '2': rows = glyph2; break;
        case '3': rows = glyph3; break;
        case 'P': rows = glyphP; break;
        case 'S': rows = glyphS; break;
        case 'C': rows = glyphC; break;
        case 'O': rows = glyphO; break;
        case 'H': rows = glyphH; break;
        case 'A': rows = glyphA; break;
        case 'E': rows = glyphE; break;
        case 'T': rows = glyphT; break;
        case 'N': rows = glyphN; break;
        case 'I': rows = glyphI; break;
        case 'M': rows = glyphM; break;
        case 'U': rows = glyphU; break;
        case '-': rows = glyphDash; break;
        default: return;
    }

    for (int ry = 0; ry < 7; ry++) {
        uint8_t bits = rows[ry];
        for (int rx = 0; rx < 5; rx++) {
            if (bits & (1 << (4 - rx))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        blend_px(fb, fbW, fbH, cl, x + rx * scale + sx, y + ry * scale + sy, col, alpha);
                    }
                }
            }
        }
    }
}

static void draw_tiny_text(uint32_t* fb, int fbW, int fbH, const ClipRect& cl,
                           int cx, int cy, int scale, const char* txt, Color col, uint8_t alpha = 255) {
    if (!txt) return;
    int len = (int)strlen(txt);
    int charW = 5 * scale;
    int gap = scale;
    int totalW = len * charW + (len > 0 ? (len - 1) * gap : 0);
    int startX = cx - totalW / 2;
    int startY = cy - (7 * scale) / 2;
    for (int i = 0; i < len; i++) {
        if (txt[i] != ' ') {
            draw_tiny_char(fb, fbW, fbH, cl, startX + i * (charW + gap), startY, scale, txt[i], col, alpha);
        }
    }
}

} // namespace

Controller3DRenderer::Controller3DRenderer() : m_time(0.0f), m_prevButtons(0) {
    memset(m_pulse, 0, sizeof(m_pulse));
}

void Controller3DRenderer::update(float dt, uint32_t buttons, int rumblePhase) {
    m_time += dt;

    static const uint32_t btnMasks[18] = {
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
        0x00000001U,              // Share / Create
        ORBIS_PAD_BUTTON_OPTIONS, // Options
        0x00010000U,              // PS Button
        ORBIS_PAD_BUTTON_TOUCH_PAD
    };

    uint32_t just = buttons & ~m_prevButtons;
    m_prevButtons = buttons;

    for (int i = 0; i < 18; i++) {
        if (just & btnMasks[i]) {
            m_pulse[i] = 1.0f;
        } else if (m_pulse[i] > 0.0f) {
            m_pulse[i] -= dt * 2.8f;
            if (m_pulse[i] < 0.0f) m_pulse[i] = 0.0f;
        }
    }
    (void)rumblePhase;
}

void Controller3DRenderer::render(Scene2D* scene, int vx, int vy, int vw, int vh,
                                  bool isDualSense, const AppConfig& cfg,
                                  const OrbisPadData& padData, int rumblePhase,
                                  int playerSlot) {
    if (!scene) return;
    uint32_t* fb = scene->GetActiveFrameBuffer();
    if (!fb) return;

    int fbW = scene->GetWidth();
    int fbH = scene->GetHeight();
    ClipRect cl = {
        vx < 0 ? 0 : vx,
        vy < 0 ? 0 : vy,
        (vx + vw - 1 >= fbW) ? (fbW - 1) : (vx + vw - 1),
        (vy + vh - 1 >= fbH) ? (fbH - 1) : (vy + vh - 1)
    };

    float cx = vx + vw * 0.5f;
    // Slight haptic vibration shake if active
    float shakeX = 0.0f, shakeY = 0.0f;
    if (rumblePhase != 0) {
        float amp = (rumblePhase == 3) ? 3.5f : 2.3f;
        shakeX = sinf(m_time * 58.0f) * amp;
        shakeY = cosf(m_time * 47.0f) * amp * 0.7f;
    }
    cx += shakeX;

    // Live inputs
    uint32_t b = padData.buttons;
    bool bCross    = (b & ORBIS_PAD_BUTTON_CROSS) != 0;
    bool bCircle   = (b & ORBIS_PAD_BUTTON_CIRCLE) != 0;
    bool bSquare   = (b & ORBIS_PAD_BUTTON_SQUARE) != 0;
    bool bTriangle = (b & ORBIS_PAD_BUTTON_TRIANGLE) != 0;
    bool bUp       = (b & ORBIS_PAD_BUTTON_UP) != 0;
    bool bDown     = (b & ORBIS_PAD_BUTTON_DOWN) != 0;
    bool bLeft     = (b & ORBIS_PAD_BUTTON_LEFT) != 0;
    bool bRight    = (b & ORBIS_PAD_BUTTON_RIGHT) != 0;
    bool bL1       = (b & ORBIS_PAD_BUTTON_L1) != 0;
    bool bR1       = (b & ORBIS_PAD_BUTTON_R1) != 0;
    bool bL2       = (b & ORBIS_PAD_BUTTON_L2) != 0;
    bool bR2       = (b & ORBIS_PAD_BUTTON_R2) != 0;
    bool bL3       = (b & ORBIS_PAD_BUTTON_L3) != 0;
    bool bR3       = (b & ORBIS_PAD_BUTTON_R3) != 0;
    bool bShare    = (b & 0x00000001U) != 0;
    bool bOptions  = (b & ORBIS_PAD_BUTTON_OPTIONS) != 0;
    bool bPs       = (b & 0x00010000U) != 0;
    bool bTpClick  = (b & ORBIS_PAD_BUTTON_TOUCH_PAD) != 0;

    float l2Frac = clampf((float)padData.analogButtons.l2 / 255.0f, 0.0f, 1.0f);
    float r2Frac = clampf((float)padData.analogButtons.r2 / 255.0f, 0.0f, 1.0f);
    Color lightCol = compute_lightbar_color(isDualSense, cfg, m_time, 100);

    // =========================================================================
    // 1. TOP DEDICATED SHOULDER TELEMETRY STRIP (L2 / L1 + R1 / R2)
    // =========================================================================
    float shoulderY = vy + 10.0f;
    float shoulderH = 62.0f;
    float shoulderW = clampf(vw - 36.0f, 440.0f, 680.0f);
    float shoulderX = cx - shoulderW * 0.5f;

    draw_round_rect_aa(fb, fbW, fbH, cl, shoulderX, shoulderY, shoulderW, shoulderH, 10.0f,
                       Color{ 11, 20, 36 }, Color{ 8, 14, 26 }, 230);
    stroke_round_rect_aa(fb, fbW, fbH, cl, shoulderX, shoulderY, shoulderW, shoulderH, 10.0f,
                         1.2f, Color{ 34, 52, 82 }, 220);

    // Left shoulder L2 (analog trigger) + L1 (bumper)
    {
        float l2X = shoulderX + 18.0f;
        float l2W = 132.0f;
        float barY = shoulderY + 11.0f;
        float barH = 40.0f;
        bool actL2 = bL2 || l2Frac > 0.05f;
        draw_round_rect_aa(fb, fbW, fbH, cl, l2X, barY, l2W, barH, 8.0f,
                           Color{ 18, 28, 44 }, Color{ 12, 18, 30 }, 255);
        if (l2Frac > 0.01f) {
            Color fillCol = actL2 ? Color{ 0, 210, 255 } : Color{ 20, 110, 190 };
            draw_round_rect_aa(fb, fbW, fbH, cl, l2X + 3.0f, barY + 3.0f, (l2W - 6.0f) * l2Frac, barH - 6.0f, 5.0f,
                               fillCol, lerp_color(fillCol, Color{ 10, 70, 140 }, 0.4f), 255);
        }
        stroke_round_rect_aa(fb, fbW, fbH, cl, l2X, barY, l2W, barH, 8.0f,
                             actL2 ? 2.2f : 1.3f, actL2 ? Color{ 40, 235, 255 } : Color{ 56, 78, 112 }, 255);
        draw_tiny_text(fb, fbW, fbH, cl, (int)(l2X + l2W * 0.5f), (int)(barY + barH * 0.5f), 2, "L2",
                       actL2 ? Color{ 255, 255, 255 } : Color{ 195, 210, 230 }, 255);

        float l1X = l2X + l2W + 12.0f;
        float l1W = 92.0f;
        draw_round_rect_aa(fb, fbW, fbH, cl, l1X, barY + 4.0f, l1W, barH - 8.0f, 7.0f,
                           bL1 ? Color{ 0, 185, 255 } : Color{ 22, 34, 54 },
                           bL1 ? Color{ 0, 120, 215 } : Color{ 15, 24, 38 }, 255);
        stroke_round_rect_aa(fb, fbW, fbH, cl, l1X, barY + 4.0f, l1W, barH - 8.0f, 7.0f,
                             bL1 ? 2.2f : 1.3f, bL1 ? Color{ 130, 245, 255 } : Color{ 62, 84, 118 }, 255);
        draw_tiny_text(fb, fbW, fbH, cl, (int)(l1X + l1W * 0.5f), (int)(barY + barH * 0.5f), 2, "L1",
                       bL1 ? Color{ 255, 255, 255 } : Color{ 195, 210, 230 }, 255);
        if (m_pulse[8] > 0.02f) {
            stroke_round_rect_aa(fb, fbW, fbH, cl, l1X - 4.0f * (1.0f - m_pulse[8]), barY,
                                 l1W + 8.0f * (1.0f - m_pulse[8]), barH, 8.0f, 2.0f,
                                 Color{ 0, 240, 255 }, (uint8_t)(200 * m_pulse[8]));
        }
    }

    // Right shoulder R1 (bumper) + R2 (analog trigger)
    {
        float r2W = 132.0f;
        float r2X = shoulderX + shoulderW - 18.0f - r2W;
        float barY = shoulderY + 11.0f;
        float barH = 40.0f;
        bool actR2 = bR2 || r2Frac > 0.05f;
        draw_round_rect_aa(fb, fbW, fbH, cl, r2X, barY, r2W, barH, 8.0f,
                           Color{ 18, 28, 44 }, Color{ 12, 18, 30 }, 255);
        if (r2Frac > 0.01f) {
            float fw = (r2W - 6.0f) * r2Frac;
            Color fillCol = actR2 ? Color{ 0, 210, 255 } : Color{ 20, 110, 190 };
            draw_round_rect_aa(fb, fbW, fbH, cl, r2X + r2W - 3.0f - fw, barY + 3.0f, fw, barH - 6.0f, 5.0f,
                               fillCol, lerp_color(fillCol, Color{ 10, 70, 140 }, 0.4f), 255);
        }
        stroke_round_rect_aa(fb, fbW, fbH, cl, r2X, barY, r2W, barH, 8.0f,
                             actR2 ? 2.2f : 1.3f, actR2 ? Color{ 40, 235, 255 } : Color{ 56, 78, 112 }, 255);
        draw_tiny_text(fb, fbW, fbH, cl, (int)(r2X + r2W * 0.5f), (int)(barY + barH * 0.5f), 2, "R2",
                       actR2 ? Color{ 255, 255, 255 } : Color{ 195, 210, 230 }, 255);

        float r1W = 92.0f;
        float r1X = r2X - 12.0f - r1W;
        draw_round_rect_aa(fb, fbW, fbH, cl, r1X, barY + 4.0f, r1W, barH - 8.0f, 7.0f,
                           bR1 ? Color{ 0, 185, 255 } : Color{ 22, 34, 54 },
                           bR1 ? Color{ 0, 120, 215 } : Color{ 15, 24, 38 }, 255);
        stroke_round_rect_aa(fb, fbW, fbH, cl, r1X, barY + 4.0f, r1W, barH - 8.0f, 7.0f,
                             bR1 ? 2.2f : 1.3f, bR1 ? Color{ 130, 245, 255 } : Color{ 62, 84, 118 }, 255);
        draw_tiny_text(fb, fbW, fbH, cl, (int)(r1X + r1W * 0.5f), (int)(barY + barH * 0.5f), 2, "R1",
                       bR1 ? Color{ 255, 255, 255 } : Color{ 195, 210, 230 }, 255);
        if (m_pulse[9] > 0.02f) {
            stroke_round_rect_aa(fb, fbW, fbH, cl, r1X - 4.0f * (1.0f - m_pulse[9]), barY,
                                 r1W + 8.0f * (1.0f - m_pulse[9]), barH, 8.0f, 2.0f,
                                 Color{ 0, 240, 255 }, (uint8_t)(200 * m_pulse[9]));
        }
    }

    // Center USB-C Port & Active Lightbar Preview chip in Shoulder Strip
    {
        float usbW = 36.0f, usbH = 12.0f;
        draw_round_rect_aa(fb, fbW, fbH, cl, cx - usbW * 0.5f, shoulderY + 14.0f, usbW, usbH, 6.0f,
                           Color{ 20, 28, 42 }, Color{ 10, 14, 22 }, 255);
        stroke_round_rect_aa(fb, fbW, fbH, cl, cx - usbW * 0.5f, shoulderY + 14.0f, usbW, usbH, 6.0f,
                             1.2f, Color{ 90, 115, 148 }, 255);
        draw_round_rect_aa(fb, fbW, fbH, cl, cx - 22.0f, shoulderY + 34.0f, 44.0f, 10.0f, 5.0f,
                           lightCol, lightCol, 255);
        draw_glow_disc(fb, fbW, fbH, cl, cx, shoulderY + 39.0f, 34.0f, lightCol, 90);
    }

    // =========================================================================
    // 2. MAIN ORIGINAL 2D CONTROLLER FRONT VIEW (PS5 DualSense OR PS4 DualShock 4)
    // =========================================================================
    float bodyAreaTop = shoulderY + shoulderH + 14.0f;
    float bodyAreaH = (vy + vh) - bodyAreaTop - 6.0f;
    float S = clampf(bodyAreaH / 345.0f, 0.72f, 1.32f);
    if (vw < (int)(560.0f * S)) {
        S = (float)vw / 560.0f;
    }
    float cy = bodyAreaTop + bodyAreaH * 0.45f + shakeY;

    // Ambient backdrop glow under controller matching LED color
    draw_glow_disc(fb, fbW, fbH, cl, cx, cy - 10.0f * S, 240.0f * S, lightCol, 45);

    if (isDualSense) {
        // ---------------------------------------------------------------------
        // AUTHENTIC PS5 DUALSENSE SILHOUETTE & DETAILS
        // ---------------------------------------------------------------------
        Color shellTop   = { 246, 248, 252 };
        Color shellMid   = { 226, 232, 241 };
        Color shellBot   = { 196, 205, 220 };
        Color coreDark   = { 22, 26, 36 };
        Color coreDark2  = { 14, 17, 25 };

        // Left & Right L1/R1 top shoulder lips visible above the upper horns
        draw_capsule_shaded(fb, fbW, fbH, cl, cx - 186.0f * S, cy - 102.0f * S, cx - 118.0f * S, cy - 114.0f * S,
                            24.0f * S, 22.0f * S,
                            bL1 ? Color{ 0, 190, 255 } : Color{ 38, 44, 58 },
                            bL1 ? Color{ 0, 120, 220 } : Color{ 20, 24, 34 }, 255);
        draw_capsule_shaded(fb, fbW, fbH, cl, cx + 118.0f * S, cy - 114.0f * S, cx + 186.0f * S, cy - 102.0f * S,
                            22.0f * S, 24.0f * S,
                            bR1 ? Color{ 0, 190, 255 } : Color{ 38, 44, 58 },
                            bR1 ? Color{ 0, 120, 220 } : Color{ 20, 24, 34 }, 255);

        // DualSense Lower Black Bridge Trim behind white outer wings
        Vec2 bridgePoly[8] = {
            { cx - 148.0f * S, cy - 72.0f * S },
            { cx + 148.0f * S, cy - 72.0f * S },
            { cx + 138.0f * S, cy + 52.0f * S },
            { cx + 78.0f * S,  cy + 78.0f * S },
            { cx,              cy + 82.0f * S },
            { cx - 78.0f * S,  cy + 78.0f * S },
            { cx - 138.0f * S, cy + 52.0f * S },
            { cx - 148.0f * S, cy - 72.0f * S }
        };
        fill_poly_shaded(fb, fbW, fbH, cl, bridgePoly, 7, coreDark, coreDark2, 255);

        // Tapered Left & Right DualSense Pearl White Grips
        draw_capsule_shaded(fb, fbW, fbH, cl,
                            cx - 178.0f * S, cy - 26.0f * S,
                            cx - 214.0f * S, cy + 128.0f * S,
                            58.0f * S, 36.0f * S, shellMid, shellBot, 255);
        draw_capsule_shaded(fb, fbW, fbH, cl,
                            cx + 178.0f * S, cy - 26.0f * S,
                            cx + 214.0f * S, cy + 128.0f * S,
                            58.0f * S, 36.0f * S, shellMid, shellBot, 255);

        // Inner Black Grip Seam Accent along handles (classic DualSense two-tone curve)
        draw_capsule_shaded(fb, fbW, fbH, cl,
                            cx - 145.0f * S, cy + 12.0f * S,
                            cx - 178.0f * S, cy + 114.0f * S,
                            18.0f * S, 11.0f * S, Color{ 24, 29, 40 }, Color{ 16, 20, 28 }, 240);
        draw_capsule_shaded(fb, fbW, fbH, cl,
                            cx + 145.0f * S, cy + 12.0f * S,
                            cx + 178.0f * S, cy + 114.0f * S,
                            18.0f * S, 11.0f * S, Color{ 24, 29, 40 }, Color{ 16, 20, 28 }, 240);

        // Left & Right White Wing Shoulders (DPad Wing + Face Button Wing)
        fill_ellipse_shaded(fb, fbW, fbH, cl, cx - 156.0f * S, cy - 42.0f * S, 86.0f * S, 68.0f * S, shellTop, shellMid, 255);
        fill_ellipse_shaded(fb, fbW, fbH, cl, cx + 156.0f * S, cy - 42.0f * S, 86.0f * S, 68.0f * S, shellTop, shellMid, 255);

        // Upper White Center Crown flanking touchpad
        Vec2 upperShell[6] = {
            { cx - 155.0f * S, cy - 102.0f * S },
            { cx + 155.0f * S, cy - 102.0f * S },
            { cx + 148.0f * S, cy + 12.0f * S },
            { cx + 96.0f * S,  cy + 40.0f * S },
            { cx - 96.0f * S,  cy + 40.0f * S },
            { cx - 148.0f * S, cy + 12.0f * S }
        };
        fill_poly_shaded(fb, fbW, fbH, cl, upperShell, 6, shellTop, shellMid, 255);

        // Matte Black Center U-Plate (frames Touchpad bottom & Analog Sticks like genuine DualSense)
        Vec2 centerPlate[8] = {
            { cx - 118.0f * S, cy + 4.0f * S },
            { cx + 118.0f * S, cy + 4.0f * S },
            { cx + 128.0f * S, cy + 56.0f * S },
            { cx + 72.0f * S,  cy + 80.0f * S },
            { cx - 72.0f * S,  cy + 80.0f * S },
            { cx - 128.0f * S, cy + 56.0f * S },
            { cx - 118.0f * S, cy + 4.0f * S }
        };
        fill_poly_shaded(fb, fbW, fbH, cl, centerPlate, 6, Color{ 26, 31, 42 }, Color{ 16, 19, 27 }, 255);

        // Analog Stick Circular Wells embedded in U-Plate
        fill_circle_shaded(fb, fbW, fbH, cl, cx - 74.0f * S, cy + 36.0f * S, 42.0f * S,
                           Color{ 22, 26, 36 }, Color{ 13, 16, 24 }, 255);
        fill_circle_shaded(fb, fbW, fbH, cl, cx + 74.0f * S, cy + 36.0f * S, 42.0f * S,
                           Color{ 22, 26, 36 }, Color{ 13, 16, 24 }, 255);

        // Touchpad Surround (Matte Grey/Black Edge + Glowing Dual V Lightbar Slits)
        float tpTopW = 196.0f * S;
        float tpBotW = 166.0f * S;
        float tpTopY = cy - 102.0f * S;
        float tpBotY = cy - 8.0f * S;

        // Glow under slanted touchpad lightbar slits
        for (int sIdx = 0; sIdx < 5; sIdx++) {
            float frac = (float)sIdx / 4.0f;
            float gy = tpTopY + (tpBotY - tpTopY) * frac;
            float gxOff = (tpTopW * 0.5f) * (1.0f - frac) + (tpBotW * 0.5f) * frac + 2.0f * S;
            draw_glow_disc(fb, fbW, fbH, cl, cx - gxOff, gy, 24.0f * S, lightCol, 85);
            draw_glow_disc(fb, fbW, fbH, cl, cx + gxOff, gy, 24.0f * S, lightCol, 85);
        }

        // Actual Pearl-White Touchpad Surface
        Vec2 tpPoly[4] = {
            { cx - tpTopW * 0.5f, tpTopY + 2.0f * S },
            { cx + tpTopW * 0.5f, tpTopY + 2.0f * S },
            { cx + tpBotW * 0.5f, tpBotY },
            { cx - tpBotW * 0.5f, tpBotY }
        };
        Color tpTop = bTpClick ? Color{ 120, 235, 255 } : Color{ 244, 246, 250 };
        Color tpBot = bTpClick ? Color{ 40, 170, 235 }  : Color{ 218, 224, 234 };
        fill_poly_shaded(fb, fbW, fbH, cl, tpPoly, 4, tpTop, tpBot, 255);

        // Dual Slanted RGB Lightbars framing the DualSense Touchpad
        draw_line_aa(fb, fbW, fbH, cl, cx - tpTopW * 0.5f - 2.0f * S, tpTopY + 3.0f * S,
                     cx - tpBotW * 0.5f - 2.0f * S, tpBotY - 1.0f * S, 5.2f * S, lightCol, 255);
        draw_line_aa(fb, fbW, fbH, cl, cx + tpTopW * 0.5f + 2.0f * S, tpTopY + 3.0f * S,
                     cx + tpBotW * 0.5f + 2.0f * S, tpBotY - 1.0f * S, 5.2f * S, lightCol, 255);
        // Bottom Touchpad hairline shadow
        draw_line_aa(fb, fbW, fbH, cl, cx - tpBotW * 0.5f, tpBotY, cx + tpBotW * 0.5f, tpBotY, 2.0f, Color{ 95, 110, 130 }, 200);

        // 5 Crisp White Player Indicator LEDs right below Touchpad center
        {
            int slotsToLit = 1;
            if (cfg.ds_player_leds == 2) slotsToLit = 0;
            else if (cfg.ds_player_leds == 1) slotsToLit = 5;
            else slotsToLit = clampf((float)(playerSlot + 1), 1.0f, 4.0f);
            float ledY = tpBotY - 6.0f * S;
            for (int li = 0; li < 5; li++) {
                float lx = cx + (li - 2) * 10.0f * S;
                bool on = false;
                if (slotsToLit == 5) on = true;
                else if (slotsToLit == 1) on = (li == 2);
                else if (slotsToLit == 2) on = (li == 1 || li == 3);
                else if (slotsToLit == 3) on = (li == 1 || li == 2 || li == 3);
                else if (slotsToLit >= 4) on = (li == 0 || li == 1 || li == 3 || li == 4);
                fill_circle_aa(fb, fbW, fbH, cl, lx, ledY, 2.2f * S,
                               on ? Color{ 255, 255, 255 } : Color{ 120, 130, 145 }, 255);
                if (on) draw_glow_disc(fb, fbW, fbH, cl, lx, ledY, 6.0f * S, Color{ 180, 235, 255 }, 140);
            }
        }

        // Touchpad Finger Points
        for (int f = 0; f < 2; f++) {
            if (f < (int)padData.touch.fingers) {
                float fx = clampf((float)padData.touch.touch[f].x / 1920.0f, 0.0f, 1.0f);
                float fy = clampf((float)padData.touch.touch[f].y / 942.0f, 0.0f, 1.0f);
                float halfW = (tpTopW * (1.0f - fy) + tpBotW * fy) * 0.44f;
                float px = cx - halfW + fx * (2.0f * halfW);
                float py = (tpTopY + 10.0f * S) + fy * ((tpBotY - tpTopY) - 18.0f * S);
                draw_glow_disc(fb, fbW, fbH, cl, px, py, 16.0f * S, Color{ 0, 240, 255 }, 160);
                fill_circle_aa(fb, fbW, fbH, cl, px, py, 6.0f * S, Color{ 0, 255, 190 }, 255);
            }
        }

        // Vertical Create & Options Buttons flanking Touchpad
        {
            float crX = cx - tpTopW * 0.5f - 17.0f * S;
            float opX = cx + tpTopW * 0.5f + 17.0f * S;
            float btnY0 = cy - 90.0f * S;
            float btnY1 = cy - 66.0f * S;
            Color crCol = bShare   ? Color{ 0, 240, 255 } : Color{ 205, 212, 224 };
            Color opCol = bOptions ? Color{ 0, 240, 255 } : Color{ 205, 212, 224 };
            draw_capsule_shaded(fb, fbW, fbH, cl, crX, btnY0, crX - 2.0f * S, btnY1, 5.2f * S, 5.2f * S, crCol, Color{ 150, 160, 175 }, 255);
            draw_capsule_shaded(fb, fbW, fbH, cl, opX, btnY0, opX + 2.0f * S, btnY1, 5.2f * S, 5.2f * S, opCol, Color{ 150, 160, 175 }, 255);
            if (bShare)   draw_glow_disc(fb, fbW, fbH, cl, crX, (btnY0 + btnY1) * 0.5f, 18.0f * S, Color{ 0, 240, 255 }, 170);
            if (bOptions) draw_glow_disc(fb, fbW, fbH, cl, opX, (btnY0 + btnY1) * 0.5f, 18.0f * S, Color{ 0, 240, 255 }, 170);
        }

        // PS Button + Horizontal Mute Pill in Center Black Plate
        {
            float psY = cy + 32.0f * S;
            if (bPs) draw_glow_disc(fb, fbW, fbH, cl, cx, psY, 24.0f * S, Color{ 0, 220, 255 }, 210);
            fill_circle_aa(fb, fbW, fbH, cl, cx, psY, 12.0f * S, bPs ? Color{ 0, 210, 255 } : Color{ 44, 52, 68 }, 255);
            stroke_circle_aa(fb, fbW, fbH, cl, cx, psY, 12.0f * S, 1.3f, bPs ? Color{ 255, 255, 255 } : Color{ 120, 134, 160 }, 255);
            draw_tiny_text(fb, fbW, fbH, cl, (int)cx, (int)psY, (S > 0.9f ? 1 : 1), "PS",
                           bPs ? Color{ 255, 255, 255 } : Color{ 210, 222, 240 }, 255);

            // Mute Pill right under PS button
            float muteY = cy + 56.0f * S;
            draw_round_rect_aa(fb, fbW, fbH, cl, cx - 12.0f * S, muteY, 24.0f * S, 6.0f * S, 3.0f * S,
                               Color{ 245, 165, 35 }, Color{ 210, 125, 15 }, 230);
        }

        // Left D-Pad Recessed Well + 4 Crisp Directional Keys
        {
            float dcx = cx - 156.0f * S;
            float dcy = cy - 42.0f * S;
            fill_circle_shaded(fb, fbW, fbH, cl, dcx, dcy, 44.0f * S, Color{ 212, 220, 232 }, Color{ 186, 196, 212 }, 255);
            stroke_circle_aa(fb, fbW, fbH, cl, dcx, dcy, 44.0f * S, 1.3f, Color{ 150, 162, 182 }, 220);

            struct DDir { float ox, oy; bool on; int pIdx; };
            DDir dirs[4] = {
                { 0.0f, -21.0f * S, bUp, 4 },
                { 0.0f,  21.0f * S, bDown, 5 },
                { -21.0f * S, 0.0f, bLeft, 6 },
                {  21.0f * S, 0.0f, bRight, 7 }
            };
            Color dpBaseTop = { 244, 247, 252 };
            Color dpBaseBot = { 196, 206, 222 };
            // Cross arms
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx - 10.5f * S, dcy - 31.0f * S, 21.0f * S, 22.0f * S, 5.0f * S,
                               bUp ? Color{ 0, 225, 255 } : dpBaseTop, bUp ? Color{ 0, 155, 235 } : dpBaseBot, 255);
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx - 10.5f * S, dcy + 9.0f * S, 21.0f * S, 22.0f * S, 5.0f * S,
                               bDown ? Color{ 0, 225, 255 } : dpBaseTop, bDown ? Color{ 0, 155, 235 } : dpBaseBot, 255);
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx - 31.0f * S, dcy - 10.5f * S, 22.0f * S, 21.0f * S, 5.0f * S,
                               bLeft ? Color{ 0, 225, 255 } : dpBaseTop, bLeft ? Color{ 0, 155, 235 } : dpBaseBot, 255);
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx + 9.0f * S, dcy - 10.5f * S, 22.0f * S, 21.0f * S, 5.0f * S,
                               bRight ? Color{ 0, 225, 255 } : dpBaseTop, bRight ? Color{ 0, 155, 235 } : dpBaseBot, 255);
            fill_circle_aa(fb, fbW, fbH, cl, dcx, dcy, 10.0f * S, Color{ 216, 224, 236 }, 255);

            for (int di = 0; di < 4; di++) {
                float px = dcx + dirs[di].ox, py = dcy + dirs[di].oy;
                if (dirs[di].on) draw_glow_disc(fb, fbW, fbH, cl, px, py, 20.0f * S, Color{ 0, 235, 255 }, 185);
                if (m_pulse[dirs[di].pIdx] > 0.02f) {
                    stroke_circle_aa(fb, fbW, fbH, cl, px, py, 14.0f * S + (1.0f - m_pulse[dirs[di].pIdx]) * 14.0f * S,
                                     2.0f, Color{ 0, 245, 255 }, (uint8_t)(210 * m_pulse[dirs[di].pIdx]));
                }
            }
        }

        // Right Face Cluster: Original DualSense Clear/Pearl Jewel Buttons with Colored PlayStation Symbols
        {
            float fcx = cx + 156.0f * S;
            float fcy = cy - 42.0f * S;
            float off = 29.0f * S;
            float btnR = 14.5f * S;

            struct FaceSpec { float x, y; bool on; int pIdx; Color symCol; int symType; };
            FaceSpec faces[4] = {
                { fcx,       fcy - off, bTriangle, 3, Color{ 45, 225, 160 }, 0 }, // Triangle
                { fcx + off, fcy,       bCircle,   1, Color{ 255, 75, 95 },  1 }, // Circle
                { fcx,       fcy + off, bCross,    0, Color{ 70, 175, 255 }, 2 }, // Cross
                { fcx - off, fcy,       bSquare,   2, Color{ 245, 115, 215 },3 }  // Square
            };
            for (int fi = 0; fi < 4; fi++) {
                FaceSpec& f = faces[fi];
                if (f.on) {
                    draw_glow_disc(fb, fbW, fbH, cl, f.x, f.y, btnR * 2.1f, f.symCol, 220);
                }
                if (m_pulse[f.pIdx] > 0.02f) {
                    stroke_circle_aa(fb, fbW, fbH, cl, f.x, f.y, btnR + (1.0f - m_pulse[f.pIdx]) * 18.0f * S,
                                     2.4f, f.symCol, (uint8_t)(230 * m_pulse[f.pIdx]));
                }
                Color capTop = f.on ? lerp_color(f.symCol, Color{ 255, 255, 255 }, 0.25f) : Color{ 242, 245, 250 };
                Color capBot = f.on ? f.symCol : Color{ 204, 212, 226 };
                fill_circle_shaded(fb, fbW, fbH, cl, f.x, f.y, btnR, capTop, capBot, 255);
                stroke_circle_aa(fb, fbW, fbH, cl, f.x, f.y, btnR, 1.3f,
                                 f.on ? Color{ 255, 255, 255 } : Color{ 152, 165, 186 }, 255);

                Color glyphCol = f.on ? Color{ 255, 255, 255 } : f.symCol;
                float gr = btnR * 0.48f;
                if (f.symType == 0) {
                    draw_line_aa(fb, fbW, fbH, cl, f.x, f.y - gr, f.x + gr * 0.9f, f.y + gr * 0.72f, 2.2f, glyphCol, 255);
                    draw_line_aa(fb, fbW, fbH, cl, f.x + gr * 0.9f, f.y + gr * 0.72f, f.x - gr * 0.9f, f.y + gr * 0.72f, 2.2f, glyphCol, 255);
                    draw_line_aa(fb, fbW, fbH, cl, f.x - gr * 0.9f, f.y + gr * 0.72f, f.x, f.y - gr, 2.2f, glyphCol, 255);
                } else if (f.symType == 1) {
                    stroke_circle_aa(fb, fbW, fbH, cl, f.x, f.y, gr * 0.85f, 2.2f, glyphCol, 255);
                } else if (f.symType == 2) {
                    draw_line_aa(fb, fbW, fbH, cl, f.x - gr * 0.72f, f.y - gr * 0.72f, f.x + gr * 0.72f, f.y + gr * 0.72f, 2.3f, glyphCol, 255);
                    draw_line_aa(fb, fbW, fbH, cl, f.x + gr * 0.72f, f.y - gr * 0.72f, f.x - gr * 0.72f, f.y + gr * 0.72f, 2.3f, glyphCol, 255);
                } else {
                    stroke_round_rect_aa(fb, fbW, fbH, cl, f.x - gr * 0.76f, f.y - gr * 0.76f, gr * 1.52f, gr * 1.52f, 1.5f, 2.2f, glyphCol, 255);
                }
            }
        }
    } else {
        // ---------------------------------------------------------------------
        // AUTHENTIC PS4 DUALSHOCK 4 JET-BLACK SILHOUETTE & DETAILS
        // ---------------------------------------------------------------------
        Color bodyTop  = { 44, 50, 64 };
        Color bodyMid  = { 28, 33, 44 };
        Color bodyBot  = { 18, 22, 30 };

        // Left & Right L1/R1 Bumper Caps above upper horns
        draw_capsule_shaded(fb, fbW, fbH, cl, cx - 182.0f * S, cy - 104.0f * S, cx - 118.0f * S, cy - 114.0f * S,
                            23.0f * S, 20.0f * S,
                            bL1 ? Color{ 0, 195, 255 } : Color{ 56, 64, 82 },
                            bL1 ? Color{ 0, 125, 215 } : Color{ 26, 31, 42 }, 255);
        draw_capsule_shaded(fb, fbW, fbH, cl, cx + 118.0f * S, cy - 114.0f * S, cx + 182.0f * S, cy - 104.0f * S,
                            20.0f * S, 23.0f * S,
                            bR1 ? Color{ 0, 195, 255 } : Color{ 56, 64, 82 },
                            bR1 ? Color{ 0, 125, 215 } : Color{ 26, 31, 42 }, 255);

        // DualShock 4 Unibody Tapered Cylindrical Side Grips
        draw_capsule_shaded(fb, fbW, fbH, cl,
                            cx - 170.0f * S, cy - 28.0f * S,
                            cx - 208.0f * S, cy + 124.0f * S,
                            56.0f * S, 35.0f * S, bodyMid, bodyBot, 255);
        draw_capsule_shaded(fb, fbW, fbH, cl,
                            cx + 170.0f * S, cy - 28.0f * S,
                            cx + 208.0f * S, cy + 124.0f * S,
                            56.0f * S, 35.0f * S, bodyMid, bodyBot, 255);

        // Circular Left & Right Button Wells
        fill_ellipse_shaded(fb, fbW, fbH, cl, cx - 152.0f * S, cy - 42.0f * S, 82.0f * S, 66.0f * S, bodyTop, bodyMid, 255);
        fill_ellipse_shaded(fb, fbW, fbH, cl, cx + 152.0f * S, cy - 42.0f * S, 82.0f * S, 66.0f * S, bodyTop, bodyMid, 255);

        // Center DualShock 4 Chassis
        Vec2 ds4Center[6] = {
            { cx - 146.0f * S, cy - 102.0f * S },
            { cx + 146.0f * S, cy - 102.0f * S },
            { cx + 136.0f * S, cy + 62.0f * S },
            { cx + 76.0f * S,  cy + 78.0f * S },
            { cx - 76.0f * S,  cy + 78.0f * S },
            { cx - 136.0f * S, cy + 62.0f * S }
        };
        fill_poly_shaded(fb, fbW, fbH, cl, ds4Center, 6, bodyTop, bodyBot, 255);

        // Glossy Dark Inner Stick Rings (DualShock 4 raised thumbstick bezels)
        fill_circle_shaded(fb, fbW, fbH, cl, cx - 74.0f * S, cy + 36.0f * S, 42.0f * S, Color{ 38, 44, 56 }, Color{ 20, 24, 34 }, 255);
        fill_circle_shaded(fb, fbW, fbH, cl, cx + 74.0f * S, cy + 36.0f * S, 42.0f * S, Color{ 38, 44, 56 }, Color{ 20, 24, 34 }, 255);

        // Signature Wide Top Prism Lightbar between shoulders + CUH-ZCT2 Lightbar Slit
        draw_glow_disc(fb, fbW, fbH, cl, cx, cy - 106.0f * S, 76.0f * S, lightCol, 130);
        draw_round_rect_aa(fb, fbW, fbH, cl, cx - 84.0f * S, cy - 110.0f * S, 168.0f * S, 11.0f * S, 5.0f * S,
                           lightCol, lerp_color(lightCol, Color{ 255, 255, 255 }, 0.25f), 255);
        stroke_round_rect_aa(fb, fbW, fbH, cl, cx - 84.0f * S, cy - 110.0f * S, 168.0f * S, 11.0f * S, 5.0f * S,
                             1.0f, Color{ 16, 20, 28 }, 200);

        // Rectangular Classic Touchpad with textured dots + top ZCT2 light slit
        float tpX = cx - 86.0f * S;
        float tpY = cy - 94.0f * S;
        float tpW = 172.0f * S;
        float tpH = 82.0f * S;
        draw_round_rect_aa(fb, fbW, fbH, cl, tpX, tpY, tpW, tpH, 8.0f * S,
                           bTpClick ? Color{ 0, 195, 255 } : Color{ 34, 40, 52 },
                           bTpClick ? Color{ 0, 130, 220 } : Color{ 22, 26, 35 }, 255);
        stroke_round_rect_aa(fb, fbW, fbH, cl, tpX, tpY, tpW, tpH, 8.0f * S, 1.4f,
                             bTpClick ? Color{ 130, 245, 255 } : Color{ 62, 72, 92 }, 255);
        // Thin ZCT2 lightbar strip near top edge of Touchpad
        draw_round_rect_aa(fb, fbW, fbH, cl, tpX + 10.0f * S, tpY + 6.0f * S, tpW - 20.0f * S, 4.0f * S, 2.0f,
                           lightCol, lightCol, 220);

        // Touchpad Finger Points
        for (int f = 0; f < 2; f++) {
            if (f < (int)padData.touch.fingers) {
                float fx = clampf((float)padData.touch.touch[f].x / 1920.0f, 0.0f, 1.0f);
                float fy = clampf((float)padData.touch.touch[f].y / 942.0f, 0.0f, 1.0f);
                float px = tpX + 10.0f * S + fx * (tpW - 20.0f * S);
                float py = tpY + 14.0f * S + fy * (tpH - 22.0f * S);
                draw_glow_disc(fb, fbW, fbH, cl, px, py, 16.0f * S, Color{ 0, 240, 255 }, 170);
                fill_circle_aa(fb, fbW, fbH, cl, px, py, 6.0f * S, Color{ 0, 255, 190 }, 255);
            }
        }

        // Vertical Oval SHARE & OPTIONS Buttons
        {
            float shX = tpX - 18.0f * S;
            float opX = tpX + tpW + 18.0f * S;
            float btnY0 = cy - 82.0f * S;
            float btnY1 = cy - 58.0f * S;
            draw_capsule_shaded(fb, fbW, fbH, cl, shX, btnY0, shX, btnY1, 6.0f * S, 6.0f * S,
                                bShare ? Color{ 0, 235, 255 } : Color{ 62, 72, 90 }, Color{ 34, 40, 52 }, 255);
            draw_capsule_shaded(fb, fbW, fbH, cl, opX, btnY0, opX, btnY1, 6.0f * S, 6.0f * S,
                                bOptions ? Color{ 0, 235, 255 } : Color{ 62, 72, 90 }, Color{ 34, 40, 52 }, 255);
            if (bShare)   draw_glow_disc(fb, fbW, fbH, cl, shX, (btnY0 + btnY1) * 0.5f, 18.0f * S, Color{ 0, 240, 255 }, 170);
            if (bOptions) draw_glow_disc(fb, fbW, fbH, cl, opX, (btnY0 + btnY1) * 0.5f, 18.0f * S, Color{ 0, 240, 255 }, 170);
        }

        // Speaker Grille Dots + Round PlayStation Button
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 5; col++) {
                fill_circle_aa(fb, fbW, fbH, cl, cx + (col - 2) * 6.0f * S, cy + (4.0f + row * 6.0f) * S,
                               1.4f * S, Color{ 10, 12, 18 }, 230);
            }
        }
        {
            float psY = cy + 36.0f * S;
            if (bPs) draw_glow_disc(fb, fbW, fbH, cl, cx, psY, 24.0f * S, Color{ 0, 220, 255 }, 210);
            fill_circle_aa(fb, fbW, fbH, cl, cx, psY, 12.0f * S, bPs ? Color{ 0, 200, 255 } : Color{ 38, 44, 56 }, 255);
            stroke_circle_aa(fb, fbW, fbH, cl, cx, psY, 12.0f * S, 1.3f, bPs ? Color{ 255, 255, 255 } : Color{ 110, 124, 148 }, 255);
            draw_tiny_text(fb, fbW, fbH, cl, (int)cx, (int)psY, 1, "PS",
                           bPs ? Color{ 255, 255, 255 } : Color{ 205, 218, 236 }, 255);
        }

        // Left D-Pad Recessed Circular Well + Dark Glossy Cross Keys
        {
            float dcx = cx - 152.0f * S;
            float dcy = cy - 42.0f * S;
            fill_circle_shaded(fb, fbW, fbH, cl, dcx, dcy, 44.0f * S, Color{ 34, 40, 52 }, Color{ 20, 24, 33 }, 255);
            stroke_circle_aa(fb, fbW, fbH, cl, dcx, dcy, 44.0f * S, 1.2f, Color{ 58, 68, 88 }, 200);

            Color dkTop = { 54, 62, 78 };
            Color dkBot = { 30, 35, 46 };
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx - 10.5f * S, dcy - 31.0f * S, 21.0f * S, 22.0f * S, 4.5f * S,
                               bUp ? Color{ 0, 225, 255 } : dkTop, bUp ? Color{ 0, 150, 230 } : dkBot, 255);
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx - 10.5f * S, dcy + 9.0f * S, 21.0f * S, 22.0f * S, 4.5f * S,
                               bDown ? Color{ 0, 225, 255 } : dkTop, bDown ? Color{ 0, 150, 230 } : dkBot, 255);
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx - 31.0f * S, dcy - 10.5f * S, 22.0f * S, 21.0f * S, 4.5f * S,
                               bLeft ? Color{ 0, 225, 255 } : dkTop, bLeft ? Color{ 0, 150, 230 } : dkBot, 255);
            draw_round_rect_aa(fb, fbW, fbH, cl, dcx + 9.0f * S, dcy - 10.5f * S, 22.0f * S, 21.0f * S, 4.5f * S,
                               bRight ? Color{ 0, 225, 255 } : dkTop, bRight ? Color{ 0, 150, 230 } : dkBot, 255);
            fill_circle_aa(fb, fbW, fbH, cl, dcx, dcy, 10.0f * S, Color{ 38, 44, 56 }, 255);

            struct DDir { float ox, oy; bool on; int pIdx; };
            DDir dirs[4] = {
                { 0.0f, -21.0f * S, bUp, 4 },
                { 0.0f,  21.0f * S, bDown, 5 },
                { -21.0f * S, 0.0f, bLeft, 6 },
                {  21.0f * S, 0.0f, bRight, 7 }
            };
            for (int di = 0; di < 4; di++) {
                float px = dcx + dirs[di].ox, py = dcy + dirs[di].oy;
                if (dirs[di].on) draw_glow_disc(fb, fbW, fbH, cl, px, py, 20.0f * S, Color{ 0, 235, 255 }, 185);
                if (m_pulse[dirs[di].pIdx] > 0.02f) {
                    stroke_circle_aa(fb, fbW, fbH, cl, px, py, 14.0f * S + (1.0f - m_pulse[dirs[di].pIdx]) * 14.0f * S,
                                     2.0f, Color{ 0, 245, 255 }, (uint8_t)(210 * m_pulse[dirs[di].pIdx]));
                }
            }
        }

        // Right Face Cluster: Original DualShock 4 Glossy Black Caps with Iconic PlayStation Colors
        {
            float fcx = cx + 152.0f * S;
            float fcy = cy - 42.0f * S;
            float off = 29.0f * S;
            float btnR = 14.5f * S;

            struct FaceSpec { float x, y; bool on; int pIdx; Color symCol; int symType; };
            FaceSpec faces[4] = {
                { fcx,       fcy - off, bTriangle, 3, Color{ 50, 225, 160 }, 0 },
                { fcx + off, fcy,       bCircle,   1, Color{ 255, 75, 95 },  1 },
                { fcx,       fcy + off, bCross,    0, Color{ 85, 175, 255 }, 2 },
                { fcx - off, fcy,       bSquare,   2, Color{ 245, 120, 220 },3 }
            };
            for (int fi = 0; fi < 4; fi++) {
                FaceSpec& f = faces[fi];
                if (f.on) {
                    draw_glow_disc(fb, fbW, fbH, cl, f.x, f.y, btnR * 2.1f, f.symCol, 220);
                }
                if (m_pulse[f.pIdx] > 0.02f) {
                    stroke_circle_aa(fb, fbW, fbH, cl, f.x, f.y, btnR + (1.0f - m_pulse[f.pIdx]) * 18.0f * S,
                                     2.4f, f.symCol, (uint8_t)(230 * m_pulse[f.pIdx]));
                }
                Color capTop = f.on ? lerp_color(f.symCol, Color{ 255, 255, 255 }, 0.25f) : Color{ 44, 52, 68 };
                Color capBot = f.on ? f.symCol : Color{ 24, 28, 38 };
                fill_circle_shaded(fb, fbW, fbH, cl, f.x, f.y, btnR, capTop, capBot, 255);
                stroke_circle_aa(fb, fbW, fbH, cl, f.x, f.y, btnR, 1.3f,
                                 f.on ? Color{ 255, 255, 255 } : Color{ 78, 90, 112 }, 255);

                Color glyphCol = f.on ? Color{ 255, 255, 255 } : f.symCol;
                float gr = btnR * 0.48f;
                if (f.symType == 0) {
                    draw_line_aa(fb, fbW, fbH, cl, f.x, f.y - gr, f.x + gr * 0.9f, f.y + gr * 0.72f, 2.2f, glyphCol, 255);
                    draw_line_aa(fb, fbW, fbH, cl, f.x + gr * 0.9f, f.y + gr * 0.72f, f.x - gr * 0.9f, f.y + gr * 0.72f, 2.2f, glyphCol, 255);
                    draw_line_aa(fb, fbW, fbH, cl, f.x - gr * 0.9f, f.y + gr * 0.72f, f.x, f.y - gr, 2.2f, glyphCol, 255);
                } else if (f.symType == 1) {
                    stroke_circle_aa(fb, fbW, fbH, cl, f.x, f.y, gr * 0.85f, 2.2f, glyphCol, 255);
                } else if (f.symType == 2) {
                    draw_line_aa(fb, fbW, fbH, cl, f.x - gr * 0.72f, f.y - gr * 0.72f, f.x + gr * 0.72f, f.y + gr * 0.72f, 2.3f, glyphCol, 255);
                    draw_line_aa(fb, fbW, fbH, cl, f.x + gr * 0.72f, f.y - gr * 0.72f, f.x - gr * 0.72f, f.y + gr * 0.72f, 2.3f, glyphCol, 255);
                } else {
                    stroke_round_rect_aa(fb, fbW, fbH, cl, f.x - gr * 0.76f, f.y - gr * 0.76f, gr * 1.52f, gr * 1.52f, 1.5f, 2.2f, glyphCol, 255);
                }
            }
        }
    }

    // =========================================================================
    // 3. LIVE L3 & R3 ANALOG THUMBSTICKS (Both Models)
    // =========================================================================
    {
        float dzMax = 20.0f * S;
        float lx = ((float)padData.leftStick.x - 128.0f) / 128.0f;
        float ly = ((float)padData.leftStick.y - 128.0f) / 128.0f;
        float rx = ((float)padData.rightStick.x - 128.0f) / 128.0f;
        float ry = ((float)padData.rightStick.y - 128.0f) / 128.0f;

        struct StickSpec { float wx, wy, dx, dy; bool clicked; int pIdx; };
        StickSpec sticks[2] = {
            { cx - 74.0f * S, cy + 36.0f * S, lx, ly, bL3, 12 },
            { cx + 74.0f * S, cy + 36.0f * S, rx, ry, bR3, 13 }
        };

        for (int si = 0; si < 2; si++) {
            StickSpec& st = sticks[si];
            float mag = sqrtf(st.dx * st.dx + st.dy * st.dy);
            // Recessed stick socket & travel boundary ring
            fill_circle_aa(fb, fbW, fbH, cl, st.wx, st.wy, 33.0f * S, Color{ 10, 12, 18 }, 240);
            stroke_circle_aa(fb, fbW, fbH, cl, st.wx, st.wy, 33.0f * S, 1.2f,
                             (mag > 0.14f || st.clicked) ? Color{ 40, 210, 255 } : Color{ 62, 74, 96 }, 230);

            float capX = st.wx + st.dx * dzMax;
            float capY = st.wy + st.dy * dzMax;

            // Stick stem & live shadow
            if (mag > 0.05f) {
                draw_line_aa(fb, fbW, fbH, cl, st.wx, st.wy, capX, capY, 14.0f * S, Color{ 18, 22, 32 }, 255);
            }
            if (st.clicked) {
                draw_glow_disc(fb, fbW, fbH, cl, capX, capY, 36.0f * S, Color{ 255, 195, 50 }, 220);
            } else if (mag > 0.15f) {
                draw_glow_disc(fb, fbW, fbH, cl, capX, capY, 32.0f * S, Color{ 0, 220, 255 }, 140);
            }
            if (m_pulse[st.pIdx] > 0.02f) {
                stroke_circle_aa(fb, fbW, fbH, cl, capX, capY, 27.0f * S + (1.0f - m_pulse[st.pIdx]) * 18.0f * S,
                                 2.2f, Color{ 255, 200, 55 }, (uint8_t)(230 * m_pulse[st.pIdx]));
            }

            Color capTop = st.clicked ? Color{ 255, 200, 60 } : Color{ 52, 58, 72 };
            Color capBot = st.clicked ? Color{ 195, 130, 15 } : Color{ 24, 28, 36 };
            fill_circle_shaded(fb, fbW, fbH, cl, capX, capY, 25.0f * S, capTop, capBot, 255);
            stroke_circle_aa(fb, fbW, fbH, cl, capX, capY, 25.0f * S, 1.8f,
                             st.clicked ? Color{ 255, 245, 180 } : (mag > 0.14f ? Color{ 60, 230, 255 } : Color{ 88, 100, 122 }), 255);
            // Textured thumb dome inner rim
            stroke_circle_aa(fb, fbW, fbH, cl, capX, capY, 16.5f * S, 1.3f,
                             st.clicked ? Color{ 255, 245, 180 } : Color{ 70, 78, 94 }, 210);
        }
    }

    // =========================================================================
    // 4. LEFT / RIGHT GRIP HAPTIC RUMBLE WAVES
    // =========================================================================
    if (rumblePhase != 0) {
        float waveP = fmodf(m_time * 3.8f, 1.0f);
        float wR = 44.0f * S + waveP * 48.0f * S;
        uint8_t wa = (uint8_t)(220.0f * (1.0f - waveP));
        if (rumblePhase == 1 || rumblePhase == 3) {
            stroke_circle_aa(fb, fbW, fbH, cl, cx - 205.0f * S, cy + 95.0f * S, wR, 3.0f, Color{ 0, 255, 175 }, wa);
        }
        if (rumblePhase == 2 || rumblePhase == 3) {
            stroke_circle_aa(fb, fbW, fbH, cl, cx + 205.0f * S, cy + 95.0f * S, wR, 3.0f, Color{ 0, 210, 255 }, wa);
        }
    }
}
