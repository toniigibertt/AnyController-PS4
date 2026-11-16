#include "ui.h"
#include "onefamily_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define BG_IMAGE_PATH   "/app0/assets/images/bg_native.png"
#define FONT_PATH       "/app0/assets/fonts/font.ttf"

// Layout Constants (TV Overscan Safe Bounds: X 96..1824, Y 72..996)
static const int SAFE_X = 96;
static const int SAFE_W = 1728;
static const int SIDEBAR_X = 96;
static const int SIDEBAR_W = 296;
static const int CARD_X = 412;
static const int CARD_W = 1412;
static const int CARD_H = 74;
static const int CARD_Y_START = 274;
static const int CARD_Y_STEP = 82;

// Colors - AnyController Studio Original Obsidian & Cyber Cyan Palette
static const Color COLOR_BLUE_GLOW   = { 0, 95, 165 };
static const Color COLOR_BLUE_MAIN   = { 0, 175, 255 };
static const Color COLOR_BLUE_BRIGHT = { 40, 225, 255 };
static const Color COLOR_ACCENT_INDIGO = { 99, 102, 241 };
static const Color COLOR_WHITE       = { 248, 250, 252 };
static const Color COLOR_TEXT_DIM    = { 148, 163, 184 };
static const Color COLOR_CARD_BG     = { 11, 17, 30 };
static const Color COLOR_CARD_BORDER = { 30, 41, 59 };
static const Color COLOR_DIVIDER     = { 24, 35, 52 };
static const Color COLOR_TOAST_BG    = { 8, 26, 48 };
static const Color COLOR_TOAST_BORDER= { 0, 210, 255 };
static const Color COLOR_SIDEBAR_BLUE= { 15, 38, 72 };
static const Color COLOR_SIDEBAR_DARK= { 9, 14, 24 };
static const Color COLOR_BORDER_DIM  = { 28, 39, 56 };

UI::UI() : fontTitle(NULL), fontSubtitle(NULL), fontCardTitle(NULL),
           fontCardDesc(NULL), fontValue(NULL), fontFooter(NULL),
           cached_bg(NULL), bg_width(0), bg_height(0) {}

UI::~UI() {
    if (cached_bg) {
        free(cached_bg);
        cached_bg = NULL;
    }
    free_icon(xboxLogo);
    free_icon(iconGear);
    free_icon(iconInfo);
    free_icon(iconSave);
    free_icon(iconMode);
    free_icon(iconGuide);
    free_icon(iconDeadzone);
    free_icon(iconSwap);
    free_icon(iconRumble);
    free_icon(btnArrows);
    free_icon(btnCross);
    free_icon(btnCircle);
    free_icon(btnSquare);
    free_icon(btnTriangle);
    free_icon(btnOptions);
    free_icon(btnShare);
    free_icon(btnPs);
    free_icon(btnTouchpad);
    free_icon(btnL1);
    free_icon(btnR1);
    free_icon(btnL2);
    free_icon(btnR2);
    free_icon(btnL3);
    free_icon(btnR3);
    free_icon(iconRemap);
}

ImageRGBA UI::load_icon(const char* path) {
    ImageRGBA img;
    int channels = 0;
    unsigned char* data = stbi_load(path, &img.width, &img.height, &channels, 4);
    if (data) {
        int total = img.width * img.height;
        img.pixels = (uint32_t*)malloc(total * sizeof(uint32_t));
        if (img.pixels) {
            uint32_t* src32 = (uint32_t*)data;
            for (int i = 0; i < total; i++) {
                uint32_t p = src32[i];
                uint8_t r = (uint8_t)(p >> 0);
                uint8_t g = (uint8_t)(p >> 8);
                uint8_t b = (uint8_t)(p >> 16);
                uint8_t a = (uint8_t)(p >> 24);
                img.pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            }
        }
        stbi_image_free(data);
    }
    return img;
}

void UI::free_icon(ImageRGBA& img) {
    if (img.pixels) {
        free(img.pixels);
        img.pixels = NULL;
    }
}

void UI::draw_image(Scene2D* scene, const ImageRGBA& img, int startX, int startY) {
    if (!scene || !img.pixels) return;
    for (int y = 0; y < img.height; y++) {
        int py = startY + y;
        if (py < 0 || py >= 1080) continue;
        for (int x = 0; x < img.width; x++) {
            int px = startX + x;
            if (px < 0 || px >= 1920) continue;
            uint32_t p = img.pixels[y * img.width + x];
            uint8_t a = (uint8_t)(p >> 24);
            if (a == 0) continue;
            uint8_t r = (uint8_t)(p >> 16);
            uint8_t g = (uint8_t)(p >> 8);
            uint8_t b = (uint8_t)(p >> 0);
            Color c = { r, g, b };
            scene->DrawPixelBlend(px, py, c, a);
        }
    }
}

bool UI::init(Scene2D* scene) {
    if (!scene) return false;

    // 1. Load clean native background (deep slate-black wallpaper with ambient green glow & controller silhouette)
    int channels = 0;
    unsigned char* data = stbi_load(BG_IMAGE_PATH, &bg_width, &bg_height, &channels, 4);
    if (data && bg_width == 1920 && bg_height == 1080) {
        cached_bg = (uint32_t*)malloc(1920 * 1080 * 4);
        if (cached_bg) {
            uint32_t* src32 = (uint32_t*)data;
            for (int i = 0; i < 1920 * 1080; i++) {
                uint32_t pixel = src32[i];
                uint8_t r = (uint8_t)(pixel >> 0);
                uint8_t g = (uint8_t)(pixel >> 8);
                uint8_t b = (uint8_t)(pixel >> 16);
                cached_bg[i] = 0x80000000 | (r << 16) | (g << 8) | b;
            }
        }
        stbi_image_free(data);
    }

    psBg.init();

    // 2. Load separate clean vector-crisp icons
    xboxLogo      = load_icon("/app0/assets/icons/xbox_logo.png");
    iconGear      = load_icon("/app0/assets/icons/icon_gear.png");
    iconInfo      = load_icon("/app0/assets/icons/icon_info.png");
    iconSave      = load_icon("/app0/assets/icons/icon_save.png");
    iconMode      = load_icon("/app0/assets/icons/card_icon_mode.png");
    iconGuide     = load_icon("/app0/assets/icons/card_icon_guide.png");
    iconDeadzone  = load_icon("/app0/assets/icons/card_icon_deadzone.png");
    iconSwap      = load_icon("/app0/assets/icons/card_icon_swap.png");
    iconRumble    = load_icon("/app0/assets/icons/card_icon_rumble.png");
    btnArrows     = load_icon("/app0/assets/icons/btn_arrows.png");
    btnCross      = load_icon("/app0/assets/icons/btn_cross_blue.png");
    btnCircle     = load_icon("/app0/assets/icons/btn_circle.png");
    btnSquare     = load_icon("/app0/assets/icons/btn_square.png");
    btnTriangle   = load_icon("/app0/assets/icons/btn_triangle.png");
    btnOptions    = load_icon("/app0/assets/icons/btn_options.png");
    btnShare      = load_icon("/app0/assets/icons/btn_share.png");
    btnPs         = load_icon("/app0/assets/icons/btn_ps.png");
    btnTouchpad   = load_icon("/app0/assets/icons/btn_touchpad.png");
    btnL1         = load_icon("/app0/assets/icons/btn_l1.png");
    btnR1         = load_icon("/app0/assets/icons/btn_r1.png");
    btnL2         = load_icon("/app0/assets/icons/btn_l2.png");
    btnR2         = load_icon("/app0/assets/icons/btn_r2.png");
    btnL3         = load_icon("/app0/assets/icons/btn_l3.png");
    btnR3         = load_icon("/app0/assets/icons/btn_r3.png");
    iconRemap     = load_icon("/app0/assets/icons/card_icon_remap.png");

    // 3. Initialize FreeType fonts with legible TV sizes
    scene->InitFont(&fontTitle,     FONT_PATH, 38);
    scene->InitFont(&fontSubtitle,  FONT_PATH, 20);
    scene->InitFont(&fontCardTitle, FONT_PATH, 24);
    scene->InitFont(&fontCardDesc,  FONT_PATH, 18);
    scene->InitFont(&fontValue,     FONT_PATH, 22);
    scene->InitFont(&fontFooter,    FONT_PATH, 18);

    return true;
}

void UI::draw_rounded_rect(Scene2D* scene, int x, int y, int w, int h, int r, Color color) {
    if (!scene) return;
    for (int row = 0; row < h; row++) {
        int dx = r;
        if (row < r) {
            int dy = r - 1 - row;
            dx = (int)round(sqrt((double)(r * r - dy * dy)));
        } else if (row >= h - r) {
            int dy = row - (h - r);
            dx = (int)round(sqrt((double)(r * r - dy * dy)));
        }
        int startX = x + (r - dx);
        int spanW = w - 2 * (r - dx);
        if (spanW > 0) {
            scene->DrawRectangle(startX, y + row, spanW, 1, color);
        }
    }
}

void UI::draw_rounded_outline(Scene2D* scene, int x, int y, int w, int h, int r, Color color, int thickness) {
    if (!scene) return;
    for (int t = 0; t < thickness; t++) {
        int curX = x + t;
        int curY = y + t;
        int curW = w - 2 * t;
        int curH = h - 2 * t;
        int curR = (r > t) ? (r - t) : 1;
        for (int row = 0; row < curH; row++) {
            int dx = curR;
            if (row < curR) {
                int dy = curR - 1 - row;
                dx = (int)round(sqrt((double)(curR * curR - dy * dy)));
            } else if (row >= curH - curR) {
                int dy = row - (curH - curR);
                dx = (int)round(sqrt((double)(curR * curR - dy * dy)));
            }
            int startX = curX + (curR - dx);
            int spanW = curW - 2 * (curR - dx);
            if (spanW > 0) {
                scene->DrawPixel(startX, curY + row, color);
                scene->DrawPixel(startX + spanW - 1, curY + row, color);
            }
        }
        // Ensure continuous top and bottom borders
        scene->DrawRectangle(curX + curR, curY, curW - 2 * curR, 1, color);
        scene->DrawRectangle(curX + curR, curY + curH - 1, curW - 2 * curR, 1, color);
    }
}

void UI::draw_circle(Scene2D* scene, int centerX, int centerY, int radius, Color color) {
    if (!scene || radius <= 0) return;
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        int py = centerY + dy;
        if (py < 0 || py >= 1080) continue;
        int dx = (int)sqrt((double)(r2 - dy * dy));
        int startX = centerX - dx;
        int w = 2 * dx + 1;
        if (startX < 0) {
            w += startX;
            startX = 0;
        }
        if (startX + w > 1920) {
            w = 1920 - startX;
        }
        if (w > 0) {
            scene->DrawRectangle(startX, py, w, 1, color);
        }
    }
}

void UI::draw_circle_outline(Scene2D* scene, int centerX, int centerY, int radius, Color color, int thickness) {
    if (!scene || radius <= 0 || thickness <= 0) return;
    int r_outer2 = radius * radius;
    int r_inner = radius - thickness;
    int r_inner2 = (r_inner > 0) ? (r_inner * r_inner) : 0;

    for (int dy = -radius; dy <= radius; dy++) {
        int py = centerY + dy;
        if (py < 0 || py >= 1080) continue;
        int dy2 = dy * dy;
        if (dy2 > r_outer2) continue;

        int dx_outer = (int)sqrt((double)(r_outer2 - dy2));
        int dx_inner = (dy2 < r_inner2) ? (int)sqrt((double)(r_inner2 - dy2)) : 0;

        // Left segment
        int leftX = centerX - dx_outer;
        int leftW = dx_outer - dx_inner;
        if (leftX < 0) { leftW += leftX; leftX = 0; }
        if (leftX + leftW > 1920) { leftW = 1920 - leftX; }
        if (leftW > 0) {
            scene->DrawRectangle(leftX, py, leftW, 1, color);
        }

        // Right segment
        int rightX = centerX + dx_inner;
        int rightW = dx_outer - dx_inner;
        if (rightX < 0) { rightW += rightX; rightX = 0; }
        if (rightX + rightW > 1920) { rightW = 1920 - rightX; }
        if (rightW > 0) {
            scene->DrawRectangle(rightX, py, rightW, 1, color);
        }
    }
}

void UI::draw_text(Scene2D* scene, const std::string& text, FT_Face face, int startX, int baselineY, Color color) {
    if (!scene || !face) return;
    Color dummyBg = { 0, 0, 0 };
    scene->DrawText((char*)text.c_str(), face, startX, baselineY, dummyBg, color);
}

void UI::draw_text_centered(Scene2D* scene, const std::string& text, FT_Face face, int centerX, int baselineY, Color color) {
    if (!scene || !face) return;
    int totalWidth = scene->GetTextWidth(text.c_str(), face);
    int startX = centerX - (totalWidth / 2);
    Color dummyBg = { 0, 0, 0 };
    scene->DrawText((char*)text.c_str(), face, startX, baselineY, dummyBg, color);
}

void UI::draw_btn_badge(Scene2D* scene, int x, int y, int w, int h, const char* label, bool pressed) {
    if (!scene) return;
    Color bg = pressed ? Color{ 0, 95, 190 } : Color{ 18, 30, 48 };
    Color border = pressed ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM;
    draw_rounded_rect(scene, x, y, w, h, 6, bg);
    draw_rounded_outline(scene, x, y, w, h, 6, border, 1);
    draw_text_centered(scene, label, fontCardDesc, x + w / 2, y + 21, pressed ? COLOR_WHITE : COLOR_TEXT_DIM);
}

void UI::draw_option_guide_box(Scene2D* scene, const char* optionTitle, const char* whatItDoes, const char* currentValEffect, Color accentColor) {
    if (!scene) return;
    const int GX = 412;
    const int GY = 708;
    const int GW = 1412;
    const int GH = 118;

    draw_rounded_rect(scene, GX, GY, GW, GH, 10, Color{ 10, 18, 34 });
    draw_rounded_outline(scene, GX, GY, GW, GH, 10, accentColor, 1);
    draw_rounded_rect(scene, GX, GY, 6, GH, 3, accentColor);

    std::string hdr = std::string("QUÉ CAMBIA ESTA OPCIÓN EN TU MANDO  •  ") + (optionTitle ? optionTitle : "");
    draw_text(scene, hdr, fontFooter, GX + 22, GY + 28, COLOR_BLUE_BRIGHT);
    draw_text(scene, whatItDoes ? whatItDoes : "", fontCardDesc, GX + 22, GY + 60, COLOR_WHITE);

    std::string eff = std::string("Efecto actual: ") + (currentValEffect ? currentValEffect : "");
    draw_text(scene, eff, fontCardDesc, GX + 22, GY + 94, Color{ 0, 255, 160 });
}

void UI::draw_target_selector_strip(Scene2D* scene, const std::vector<ControllerTarget>* targets, int activeTargetIndex) {
    if (!scene) return;
    const int SX = 412;
    const int SY = 148;
    const int SW = 1412;
    const int SH = 40;

    ControllerTarget fallback;
    fallback.shortLabel = "Global (Por Defecto)";
    fallback.detailLabel = "Global (/data/anycontroller/anycontroller.ini)";

    const ControllerTarget* active = &fallback;
    int count = 1;
    if (targets && !targets->empty()) {
        count = (int)targets->size();
        int idx = (activeTargetIndex >= 0 && activeTargetIndex < count) ? activeTargetIndex : 0;
        active = &((*targets)[idx]);
    }

    draw_rounded_rect(scene, SX, SY, SW, SH, 8, Color{ 10, 19, 36 });
    draw_rounded_outline(scene, SX, SY, SW, SH, 8,
                         active->id.empty() ? COLOR_CARD_BORDER : (active->hasCustom ? Color{ 0, 240, 180 } : COLOR_BLUE_MAIN), 1);

    draw_image(scene, btnL3, SX + 10, SY + 4);
    char labelBuf[192];
    snprintf(labelBuf, sizeof(labelBuf), "Editando (%d/%d): %s",
             (targets && !targets->empty()) ? (activeTargetIndex + 1) : 1,
             count,
             active->detailLabel.c_str());
    draw_text(scene, labelBuf, fontCardDesc, SX + 50, SY + 26, COLOR_WHITE);

    if (active->id.empty()) {
        draw_text(scene, "BASE GLOBAL • Herencia automática", fontFooter, SX + 1030, SY + 26, COLOR_BLUE_BRIGHT);
    } else if (active->hasCustom) {
        draw_text(scene, "[MEMORIA PROPIA]", fontFooter, SX + 985, SY + 26, Color{ 0, 255, 160 });
        draw_image(scene, btnR3, SX + 1175, SY + 4);
        draw_text(scene, "Restaurar Global", fontFooter, SX + 1218, SY + 26, COLOR_WHITE);
    } else {
        draw_text(scene, "[HEREDANDO GLOBAL • Al editar crea perfil]", fontFooter, SX + 950, SY + 26, Color{ 245, 195, 70 });
    }
}

void UI::render(Scene2D* scene, const AppConfig& cfg, int currentTabId, int selectedCard,
                int toastFrames, const OrbisPadData& padData, const TesterState& tester,
                const char* toastMsg,
                bool hidConnected, const HidStatus* hidStatus,
                const HidProfile* hidProfile, int remapSelectedRow,
                bool remapTestMode,
                const PkgUpdater* updater,
                bool dualsenseConnected, int dsSelectedCard,
                int ds4SelectedCard, bool dualshock4Connected,
                const std::vector<ControllerTarget>* targets,
                int activeTargetIndex) {
    if (!scene) return;
    (void)dualshock4Connected;

    // 1. Blit native background + Animated PlayStation XMB Wave & Bokeh Sparkles
    psBg.update(0.01666f);
    psBg.render(scene, cached_bg);

    // 2. Original AnyController Studio Top Header Bar (TV Overscan Safe: X=96..1824, Y=72..138)
    draw_rounded_rect(scene, SAFE_X, 72, SAFE_W, 66, 12, Color{ 9, 14, 26 });
    draw_rounded_outline(scene, SAFE_X, 72, SAFE_W, 66, 12, COLOR_CARD_BORDER, 1);
    draw_rounded_rect(scene, SAFE_X, 72, 6, 66, 3, COLOR_BLUE_MAIN);

    draw_rounded_rect(scene, 114, 82, 46, 46, 10, Color{ 15, 34, 64 });
    draw_rounded_outline(scene, 114, 82, 46, 46, 10, COLOR_BLUE_MAIN, 2);
    draw_image(scene, btnPs, 121, 89);

    int titleW = scene->GetTextWidth("CONTROLLER CONTROL CENTER ", fontCardTitle);
    draw_text(scene, "CONTROLLER CONTROL CENTER ", fontCardTitle, 174, 100, COLOR_WHITE);
    draw_text(scene, "PS4", fontCardTitle, 174 + titleW, 100, COLOR_BLUE_BRIGHT);
    draw_text(scene, "ANYCONTROLLER " ONEFAMILY_VERSION_STRING " • DualSense PS5, DualShock 4 Nativo y Memoria por Mando", fontCardDesc, 174, 124, COLOR_TEXT_DIM);

    std::string activeShort = "Global";
    if (targets && activeTargetIndex >= 0 && activeTargetIndex < (int)targets->size()) {
        activeShort = (*targets)[activeTargetIndex].shortLabel;
    }

    char hdrCtrlStr[96];
    snprintf(hdrCtrlStr, sizeof(hdrCtrlStr), "ACTIVO: %s", activeShort.c_str());
    draw_rounded_rect(scene, 1528, 86, 280, 38, 8, Color{ 12, 28, 48 });
    draw_rounded_outline(scene, 1528, 86, 280, 38, 8, COLOR_BLUE_GLOW, 1);
    draw_circle(scene, 1546, 105, 5, Color{ 0, 240, 140 });
    draw_text(scene, hdrCtrlStr, fontCardDesc, 1560, 111, COLOR_WHITE);

    if (updater && updater->get_state() == UPDATER_UPDATE_AVAILABLE) {
        int bX = 1236, bY = 86, bW = 276, bH = 38;
        draw_rounded_rect(scene, bX, bY, bW, bH, 8, Color{ 10, 42, 28 });
        draw_rounded_outline(scene, bX, bY, bW, bH, 8, Color{ 0, 255, 150 }, 2);
        std::string badgeStr = "NUEVA: " + updater->get_latest_version();
        draw_text(scene, badgeStr, fontCardDesc, bX + 16, bY + 25, Color{ 0, 255, 150 });
    }

    // 3. Left Navigation Dock (X=96..392)
    struct SidebarTab {
        int tabId;
        const char* label;
        const char* sublabel;
        const ImageRGBA* icon;
    };
    std::vector<SidebarTab> sidebarTabs;
    sidebarTabs.push_back({ TAB_SETTINGS, "Ajustes Globales", "Zona muerta y ejes", &iconGear });
    if (dualsenseConnected) {
        sidebarTabs.push_back({ TAB_DUALSENSE, "DualSense PS5", "Mando PS5 activo", &btnPs });
        sidebarTabs.push_back({ TAB_DUALSHOCK4, "DualShock 4", "Configurar mando PS4", &iconRumble });
    } else {
        sidebarTabs.push_back({ TAB_DUALSHOCK4, "DualShock 4", "Mando PS4 activo", &iconRumble });
        sidebarTabs.push_back({ TAB_DUALSENSE, "DualSense PS5", "Configurar mando PS5", &btnPs });
    }
    sidebarTabs.push_back({ TAB_TESTER, "Laboratorio", "Probador original 2D", &iconMode });
    if (hidConnected) {
        sidebarTabs.push_back({ TAB_REMAP_HID, "Mapeo USB HID", "Perfiles a medida", &iconRemap });
    }
    sidebarTabs.push_back({ TAB_INFO, "Sistema y Red", "Actualizador GitHub", &iconInfo });

    draw_rounded_rect(scene, SIDEBAR_X, 148, SIDEBAR_W, 50, 10, Color{ 12, 20, 36 });
    draw_rounded_outline(scene, SIDEBAR_X, 148, SIDEBAR_W, 50, 10, COLOR_CARD_BORDER, 1);
    draw_image(scene, btnL1, SIDEBAR_X + 12, 157);
    draw_text_centered(scene, "MÓDULOS", fontCardDesc, SIDEBAR_X + SIDEBAR_W / 2, 180, COLOR_BLUE_BRIGHT);
    draw_image(scene, btnR1, SIDEBAR_X + SIDEBAR_W - 54, 157);

    int totalSidebarTabs = (int)sidebarTabs.size();
    for (int i = 0; i < totalSidebarTabs; i++) {
        int tabY = 208 + i * 78;
        bool isCurrent = (sidebarTabs[i].tabId == currentTabId);
        if (isCurrent) {
            draw_rounded_rect(scene, SIDEBAR_X, tabY, SIDEBAR_W, 68, 10, COLOR_SIDEBAR_BLUE);
            draw_rounded_outline(scene, SIDEBAR_X, tabY, SIDEBAR_W, 68, 10, COLOR_BLUE_MAIN, 2);
            draw_rounded_rect(scene, SIDEBAR_X, tabY + 10, 5, 48, 2, COLOR_BLUE_BRIGHT);
            if (sidebarTabs[i].icon) draw_image(scene, *sidebarTabs[i].icon, SIDEBAR_X + 14, tabY + 12);
            draw_text(scene, sidebarTabs[i].label, fontCardTitle, SIDEBAR_X + 68, tabY + 31, COLOR_WHITE);
            draw_text(scene, sidebarTabs[i].sublabel, fontCardDesc, SIDEBAR_X + 68, tabY + 54, COLOR_BLUE_BRIGHT);
        } else {
            draw_rounded_rect(scene, SIDEBAR_X, tabY, SIDEBAR_W, 68, 10, COLOR_SIDEBAR_DARK);
            draw_rounded_outline(scene, SIDEBAR_X, tabY, SIDEBAR_W, 68, 10, COLOR_BORDER_DIM, 1);
            if (sidebarTabs[i].icon) draw_image(scene, *sidebarTabs[i].icon, SIDEBAR_X + 14, tabY + 12);
            draw_text(scene, sidebarTabs[i].label, fontCardTitle, SIDEBAR_X + 68, tabY + 31, Color{ 203, 213, 225 });
            draw_text(scene, sidebarTabs[i].sublabel, fontCardDesc, SIDEBAR_X + 68, tabY + 54, COLOR_TEXT_DIM);
        }

        if (sidebarTabs[i].tabId == TAB_INFO && updater && updater->get_state() == UPDATER_UPDATE_AVAILABLE) {
            draw_circle(scene, SIDEBAR_X + SIDEBAR_W - 18, tabY + 34, 6, Color{ 0, 255, 150 });
        }
    }

    // 4. Main Content Area
    if (currentTabId == TAB_SETTINGS) {
        draw_target_selector_strip(scene, targets, activeTargetIndex);

        draw_rounded_rect(scene, CARD_X, 196, CARD_W, 68, 12, Color{ 13, 23, 42 });
        draw_rounded_outline(scene, CARD_X, 196, CARD_W, 68, 12, COLOR_CARD_BORDER, 1);
        draw_rounded_rect(scene, CARD_X, 196, 6, 68, 3, COLOR_BLUE_MAIN);
        draw_image(scene, iconGear, CARD_X + 20, 209);
        draw_text(scene, "Motor Pro de Respuesta, Curvas de Joysticks y Turbo", fontCardTitle, CARD_X + 78, 223, COLOR_WHITE);
        draw_text(scene, "Personaliza la base Global o cada mando por separado con L3 (memoria persistente en consola).", fontCardDesc, CARD_X + 78, 249, COLOR_TEXT_DIM);

        // --- SETTINGS VIEW (8 Cards, 5 Visible with Scroll) ---
        const int TOTAL_CARDS = 8;
        const int VISIBLE_CARDS = 5;

        int scrollOffset = 0;
        if (selectedCard >= VISIBLE_CARDS) {
            scrollOffset = selectedCard - VISIBLE_CARDS + 1;
        }
        if (scrollOffset > TOTAL_CARDS - VISIBLE_CARDS) {
            scrollOffset = TOTAL_CARDS - VISIBLE_CARDS;
        }

        const char* cardTitles[TOTAL_CARDS] = {
            "Perfil Maestro Pro (Presets Instantáneos)",
            "Zona Muerta de Palancas Analógicas (Anti-Drift)",
            "Curva de Respuesta de Joysticks (Precisión / eSports)",
            "Umbral de Disparo Digital en Gatillos L2 / R2",
            "Modo Fuego Rápido / Turbo (Disparo Automático)",
            "Inversión Táctica de Hombros (L1/L2 y R1/R2)",
            "Motor General de Vibración Háptica",
            "Esquema de Botones Nintendo Switch (A/B/X/Y)"
        };

        const char* cardDescs[TOTAL_CARDS] = {
            "Aplica al instante configuraciones optimizadas: eSports FPS, SimRacing, Modo Eco o Estándar.",
            "Filtra oscilaciones involuntarias en reposo de las palancas izquierda y derecha.",
            "Elige entre respuesta Lineal 1:1, Exponencial Suave (puntería fina) o Agresiva eSports.",
            "Punto exacto del recorrido analógico (0-255) que activa el disparo digital.",
            "Ráfaga automática de ~15 disparos/s al mantener pulsado a fondo R2 o L2+R2.",
            "Intercambia los botones superiores L1/R1 con los gatillos principales L2/R2.",
            "Activa o silencia globalmente los motores de vibración en todos los mandos.",
            "Por posición física (estilo PlayStation) o por letra grabada en el mando Switch."
        };

        ImageRGBA cardIcons[TOTAL_CARDS] = {
            iconMode, iconDeadzone, iconDeadzone, iconDeadzone,
            iconSwap, iconSwap, iconRumble, iconSwap
        };

        std::string vals[TOTAL_CARDS];
        vals[0] = ConfigManager::PRO_PROFILE_OPTIONS[cfg.pro_profile_index];
        vals[1] = ConfigManager::DEADZONE_OPTIONS[cfg.deadzone_index];
        vals[2] = ConfigManager::STICK_CURVE_OPTIONS[cfg.stick_curve_index];
        vals[3] = ConfigManager::TRIGGER_OPTIONS[cfg.trigger_index];
        vals[4] = ConfigManager::RAPID_FIRE_OPTIONS[cfg.rapid_fire_index];
        vals[5] = ConfigManager::SWAP_OPTIONS[cfg.swap_index];
        vals[6] = ConfigManager::RUMBLE_OPTIONS[cfg.rumble_index];
        vals[7] = ConfigManager::SWITCH_OPTIONS[cfg.switch_index];

        int fullCardW = CARD_W; // 1412px wide studio cards inside overscan safe zone
        int selectorCenterX = CARD_X + fullCardW - 190;

        for (int i = 0; i < VISIBLE_CARDS; i++) {
            int cardIndex = scrollOffset + i;
            if (cardIndex >= TOTAL_CARDS) break;
            int cardY = CARD_Y_START + (i * CARD_Y_STEP);
            bool isSelected = (cardIndex == selectedCard);

            draw_rounded_rect(scene, CARD_X, cardY, fullCardW, CARD_H, 12, isSelected ? Color{ 16, 30, 54 } : COLOR_CARD_BG);

            if (isSelected) {
                draw_rounded_outline(scene, CARD_X - 1, cardY - 1, fullCardW + 2, CARD_H + 2, 12, COLOR_BLUE_GLOW, 2);
                draw_rounded_outline(scene, CARD_X, cardY, fullCardW, CARD_H, 12, COLOR_BLUE_MAIN, 2);
                draw_rounded_rect(scene, CARD_X, cardY + 14, 5, CARD_H - 28, 2, COLOR_BLUE_BRIGHT);
            } else {
                draw_rounded_outline(scene, CARD_X, cardY, fullCardW, CARD_H, 12, COLOR_CARD_BORDER, 1);
            }

            draw_image(scene, cardIcons[cardIndex], CARD_X + 22, cardY + 15);

            draw_text(scene, cardTitles[cardIndex], fontCardTitle, CARD_X + 84, cardY + 31, COLOR_WHITE);
            draw_text(scene, cardDescs[cardIndex],  fontCardDesc,  CARD_X + 84, cardY + 58, COLOR_TEXT_DIM);

            // Sleek Value Selector Box on the right
            int boxW = 300;
            int boxH = 42;
            int boxX = selectorCenterX - boxW / 2;
            int boxY = cardY + (CARD_H - boxH) / 2;
            draw_rounded_rect(scene, boxX, boxY, boxW, boxH, 8, isSelected ? Color{ 10, 22, 42 } : Color{ 8, 13, 24 });
            draw_rounded_outline(scene, boxX, boxY, boxW, boxH, 8, isSelected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM, 1);

            Color textColor  = isSelected ? COLOR_BLUE_BRIGHT : COLOR_WHITE;
            Color arrowColor = isSelected ? COLOR_WHITE : COLOR_TEXT_DIM;

            draw_text_centered(scene, vals[cardIndex], fontValue, selectorCenterX, cardY + 46, textColor);
            draw_text(scene, "<", fontValue, boxX + 14, cardY + 46, arrowColor);
            draw_text(scene, ">", fontValue, boxX + boxW - 26, cardY + 46, arrowColor);
        }

        if (scrollOffset > 0) {
            draw_text(scene, "^", fontCardTitle, CARD_X + fullCardW / 2, 272, COLOR_BLUE_BRIGHT);
        }
        if (scrollOffset + VISIBLE_CARDS < TOTAL_CARDS) {
            draw_text(scene, "v", fontCardTitle, CARD_X + fullCardW / 2, 700, COLOR_BLUE_BRIGHT);
        }

        // Dynamic explanation panel for selected Global Setting
        static const char* settingWhat[TOTAL_CARDS] = {
            "Aplica al instante una combinación completa de zona muerta, curva analógica, gatillos, vibración e iluminación.",
            "Fija el radio central mínimo en las palancas L3 y R3 que el demonio ignora antes de enviar movimiento a la consola.",
            "Cambia la relación entre el ángulo físico de la palanca y la velocidad de movimiento enviada al juego.",
            "Ajusta el punto exacto de recorrido analógico (0-255) desde el que L2 y R2 cuentan como disparo digital.",
            "Genera pulsaciones automáticas repetidas (~15/s) mientras mantienes apretado el gatillo casi a fondo (>= 220).",
            "Intercambia los botones superiores L1/R1 con los gatillos inferiores L2/R2 en todo el sistema PS4.",
            "Interruptor maestro que habilita o silencia globalmente la vibración de motores en todos los mandos.",
            "En mandos Nintendo Switch Pro/Joy-Con decide si A/B/X/Y obedecen la posición PlayStation o la letra impresa."
        };
        int selIdx = (selectedCard >= 0 && selectedCard < TOTAL_CARDS) ? selectedCard : 0;
        std::string settingEff;
        if (selIdx == 0) {
            static const char* proEff[5] = {
                "Personalizado: mantiene tus ajustes individuales actuales sin sobrescribirlos.",
                "eSports FPS: zona muerta 5%, curva Agresiva, gatillos al 6%, Hair Trigger PS5 y motores apagados.",
                "SimRacing: zona muerta 8%, curva Exponencial suave, resistencia continua L2/R2 y vibración al 120%.",
                "Modo Eco: apaga barras LED y vibración/gatillos para maximizar las horas de batería.",
                "Estándar PS4/PS5: restaura los valores equilibrados originales de fábrica para cualquier juego."
            };
            int pi = (cfg.pro_profile_index >= 0 && cfg.pro_profile_index < 5) ? cfg.pro_profile_index : 0;
            settingEff = proEff[pi];
        } else if (selIdx == 1) {
            char buf[192];
            int dzRaw = ConfigManager::DEADZONE_VALUES[cfg.deadzone_index];
            snprintf(buf, sizeof(buf), "Umbral %s (%d/127): cualquier vibración de drift por debajo de este radio se corrige a centro exacto.",
                     vals[1].c_str(), dzRaw);
            settingEff = buf;
        } else if (selIdx == 2) {
            static const char* curveEff[3] = {
                "Lineal (1:1): respuesta directa sin alteraciones, exactamente proporcional al desplazamiento del stick.",
                "Precisión Exponencial: suaviza los movimientos cortos para apuntar fino y acelera al llegar al borde.",
                "Agresiva eSports: alcanza antes la velocidad máxima para giros y reacciones rápidas en shooters."
            };
            int ci = (cfg.stick_curve_index >= 0 && cfg.stick_curve_index < 3) ? cfg.stick_curve_index : 0;
            settingEff = curveEff[ci];
        } else if (selIdx == 3) {
            char buf[192];
            int thRaw = ConfigManager::TRIGGER_VALUES[cfg.trigger_index];
            snprintf(buf, sizeof(buf), "Umbral %s (%d/255): al pulsar L2 o R2 por encima de este punto el botón digital dispara al instante.",
                     vals[3].c_str(), thRaw);
            settingEff = buf;
        } else if (selIdx == 4) {
            static const char* rfEff[3] = {
                "Desactivado: L2 y R2 funcionan de forma normal y continua sin ráfagas automáticas.",
                "Turbo R2: al mantener R2 presionado a fondo dispara ráfagas automáticas continuas (~15 por segundo).",
                "Turbo Dual L2+R2: dispara ráfagas automáticas tanto al apretar L2 como R2 a fondo."
            };
            int ri = (cfg.rapid_fire_index >= 0 && cfg.rapid_fire_index < 3) ? cfg.rapid_fire_index : 0;
            settingEff = rfEff[ri];
        } else if (selIdx == 5) {
            settingEff = (cfg.swap_index != 0)
                ? "Activado: apuntas y disparas con los botones rápidos L1/R1; L2/R2 hacen las funciones superiores."
                : "Desactivado: disposición clásica estándar (L1/R1 arriba y gatillos L2/R2 abajo).";
        } else if (selIdx == 6) {
            settingEff = (cfg.rumble_index == 0)
                ? "Activado: el mando vibra normalmente siguiendo la intensidad enviada por los juegos."
                : "Desactivado: bloquea todas las órdenes de vibración para ahorrar batería o evitar distracciones.";
        } else {
            settingEff = (cfg.switch_index == 0)
                ? "Por Posición: el botón inferior (B) es Cruz (X) y el derecho (A) es Círculo (O), igual que en PS4."
                : "Por Letra: A de Nintendo equivale a Cruz (X) y X equivale a Cuadrado según el texto del botón.";
        }
        draw_option_guide_box(scene, cardTitles[selIdx], settingWhat[selIdx], settingEff.c_str(), COLOR_BLUE_MAIN);

        // 5. Footer for Settings (TV Overscan Safe: Y=844..936, X=96..1824)
        scene->DrawRectangle(SAFE_X, 844, SAFE_W, 2, COLOR_DIVIDER);

        draw_image(scene, iconSave, SAFE_X, 868);
        draw_text(scene, "Controller Control Center PS4 (" ONEFAMILY_VERSION_STRING ")", fontCardTitle, 154, 890, COLOR_WHITE);
        draw_text(scene, "Cada mando guarda memoria independiente • Global es la base", fontFooter, 154, 916, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnL3, 960, 875);
        draw_text(scene, "Mando", fontFooter, 1004, 900, COLOR_WHITE);

        draw_image(scene, btnTriangle, 1110, 875);
        draw_text(scene, "Preset Pro", fontFooter, 1154, 900, Color{ 0, 255, 150 });

        draw_image(scene, btnArrows, 1300, 879);
        draw_text(scene, "Modificar", fontFooter, 1352, 900, COLOR_TEXT_DIM);

        draw_image(scene, btnCross, 1475, 875);
        draw_text(scene, "Guardar", fontFooter, 1519, 900, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnOptions, 1635, 878);
        draw_text(scene, "Salir", fontFooter, 1712, 900, COLOR_TEXT_DIM);

    } else if (currentTabId == TAB_TESTER) {
        bool slotIsDs = dualsenseConnected;
        std::string slotTargetShort = activeShort;
        if (targets) {
            for (size_t ti = 0; ti < targets->size(); ti++) {
                if ((*targets)[ti].slotIndex == tester.selectedSlot) {
                    slotIsDs = (*targets)[ti].isDualSense;
                    slotTargetShort = (*targets)[ti].shortLabel;
                    break;
                }
            }
        }
        controller3D.update(0.01666f, padData.buttons, tester.rumblePhase);
        render_tester(scene, cfg, padData, tester, slotIsDs, slotTargetShort.c_str());

    } else if (currentTabId == TAB_DUALSENSE && hidStatus) {
        render_dualsense(scene, cfg, dsSelectedCard, *hidStatus, targets, activeTargetIndex);

    } else if (currentTabId == TAB_DUALSHOCK4) {
        render_dualshock4(scene, cfg, ds4SelectedCard, padData, tester, targets, activeTargetIndex);

    } else if (currentTabId == TAB_REMAP_HID && hidConnected && hidStatus && hidProfile) {
        render_remap(scene, *hidStatus, *hidProfile, remapSelectedRow, padData, remapTestMode);

    } else {
        // --- INFO VIEW (TV Overscan Safe: X=412..1824, W=1412, Y=148..826) ---
        const int INFO_W = CARD_W;

        // Card 1: Version & Architecture (Y = 148..264, H = 116)
        draw_rounded_rect(scene, CARD_X, 148, INFO_W, 116, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, 148, INFO_W, 116, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconInfo, CARD_X + 22, 164);
        draw_text(scene, "Controller Control Center PS4 (Title ID: TONI88888 • " ONEFAMILY_VERSION_STRING ")", fontCardTitle, CARD_X + 86, 178, COLOR_WHITE);
        draw_text(scene, "Payload Demonio: ANYCONTROLLER (/data/payloads/anycontroller.bin)", fontCardDesc, CARD_X + 86, 208, COLOR_BLUE_BRIGHT);
        draw_text(scene, "Base Global: /data/anycontroller/anycontroller.ini  •  Perfiles: /data/anycontroller/controllers/*.ini", fontCardDesc, CARD_X + 86, 236, COLOR_TEXT_DIM);

        // Card 2: Online Update Status (Y = 274..394, H = 120)
        UpdaterState upState = updater ? updater->get_state() : UPDATER_IDLE;
        Color updateBg = COLOR_CARD_BG;
        Color updateBorder = COLOR_CARD_BORDER;

        if (upState == UPDATER_UPDATE_AVAILABLE) {
            updateBg = Color{ 15, 35, 60 };
            updateBorder = COLOR_BLUE_BRIGHT;
        } else if (upState == UPDATER_DOWNLOAD_SUCCESS) {
            updateBg = Color{ 15, 60, 40 };
            updateBorder = Color{ 0, 255, 150 };
        } else if (upState == UPDATER_DOWNLOADING) {
            updateBg = Color{ 15, 45, 75 };
            updateBorder = COLOR_BLUE_MAIN;
        }

        draw_rounded_rect(scene, CARD_X, 274, INFO_W, 120, 12, updateBg);
        draw_rounded_outline(scene, CARD_X, 274, INFO_W, 120, 12, updateBorder,
                             (upState == UPDATER_UPDATE_AVAILABLE || upState == UPDATER_DOWNLOAD_SUCCESS) ? 2 : 1);
        draw_image(scene, iconSave, CARD_X + 22, 290);

        if (upState == UPDATER_UPDATE_AVAILABLE) {
            std::string titleStr = "Nueva Actualización Disponible: " + updater->get_latest_version();
            draw_text(scene, titleStr, fontCardTitle, CARD_X + 86, 304, Color{ 0, 255, 150 });
            draw_text(scene, "Pulsa [EQUIS (X)] para descargar el PKG directamente en /data/pkg/", fontCardDesc, CARD_X + 86, 334, COLOR_WHITE);
            draw_text(scene, "Listo para instalar en GoldHEN -> Package Installer sin usar PC ni USB", fontCardDesc, CARD_X + 86, 362, COLOR_TEXT_DIM);
        } else if (upState == UPDATER_DOWNLOADING) {
            UpdaterProgress prog = updater->get_progress();
            char dlTitle[128];
            snprintf(dlTitle, sizeof(dlTitle), "Descargando Actualización: %d%% (%llu MB / %llu MB)",
                     prog.percent,
                     (unsigned long long)(prog.bytesReceived / (1024 * 1024)),
                     (unsigned long long)(prog.totalBytes / (1024 * 1024)));
            draw_text(scene, dlTitle, fontCardTitle, CARD_X + 86, 304, COLOR_BLUE_BRIGHT);

            int barX = CARD_X + 86;
            int barY = 324;
            int barW = 1280;
            int barH = 14;
            draw_rounded_rect(scene, barX, barY, barW, barH, 4, Color{ 20, 32, 50 });
            int fillW = (barW * prog.percent) / 100;
            if (fillW > 0) draw_rounded_rect(scene, barX, barY, fillW, barH, 4, COLOR_BLUE_BRIGHT);
            std::string dlPathMsg = "Guardando en " + (updater ? updater->get_target_pkg_path() : std::string("/data/pkg/")) + "...";
            draw_text(scene, dlPathMsg, fontCardDesc, CARD_X + 86, 366, COLOR_TEXT_DIM);
        } else if (upState == UPDATER_DOWNLOAD_SUCCESS) {
            draw_text(scene, "¡Descarga Completada! Paquete Guardado en PS4", fontCardTitle, CARD_X + 86, 304, Color{ 0, 255, 150 });
            std::string savedMsg = "Archivo guardado en: " + (updater ? updater->get_target_pkg_path() : std::string("/data/pkg/"));
            draw_text(scene, savedMsg, fontCardDesc, CARD_X + 86, 334, COLOR_WHITE);
            draw_text(scene, "Abre Ajustes PS4 -> GoldHEN -> Package Installer para instalar.", fontCardDesc, CARD_X + 86, 362, Color{ 0, 255, 150 });
        } else if (upState == UPDATER_UP_TO_DATE) {
            draw_text(scene, "Controller Control Center PS4 está al Día (" ONEFAMILY_VERSION_STRING ")", fontCardTitle, CARD_X + 86, 304, COLOR_WHITE);
            draw_text(scene, "Tienes la última versión instalada y verificada en GitHub Releases.", fontCardDesc, CARD_X + 86, 334, COLOR_BLUE_BRIGHT);
            draw_text(scene, "Pulsa [EQUIS (X)] cuando quieras volver a comprobar actualizaciones en línea.", fontCardDesc, CARD_X + 86, 362, COLOR_TEXT_DIM);
        } else if (upState == UPDATER_CHECKING) {
            draw_text(scene, "Buscando Actualizaciones en GitHub...", fontCardTitle, CARD_X + 86, 304, COLOR_WHITE);
            draw_text(scene, "Conectando con github.com/toniigibertt/AnyController-PS4/releases...", fontCardDesc, CARD_X + 86, 334, COLOR_TEXT_DIM);
            draw_text(scene, "Verificando el último PKG publicado...", fontCardDesc, CARD_X + 86, 362, COLOR_TEXT_DIM);
        } else {
            draw_text(scene, "Servicio de Actualización en Línea (GitHub)", fontCardTitle, CARD_X + 86, 304, COLOR_WHITE);
            draw_text(scene, updater ? updater->get_status_message() : "Modo sin conexión", fontCardDesc, CARD_X + 86, 334, COLOR_TEXT_DIM);
            draw_text(scene, "Pulsa [EQUIS (X)] para comprobar GitHub Releases manualmente.", fontCardDesc, CARD_X + 86, 362, COLOR_TEXT_DIM);
        }

        // Card 3: Per-Controller Memory & Multi-DualSense Guide (Y = 404..552, H = 148)
        draw_rounded_rect(scene, CARD_X, 404, INFO_W, 148, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, 404, INFO_W, 148, 12, COLOR_BLUE_MAIN, 1);
        draw_image(scene, iconMode, CARD_X + 22, 422);
        draw_text(scene, "Configuración Independiente y Memoria Persistente por Mando", fontCardTitle, CARD_X + 86, 436, COLOR_WHITE);
        draw_text(scene, "• Con [L3], cada DualSense guarda sus colores/drift por MAC Bluetooth (ds_mac_XXXX.ini).", fontCardDesc, CARD_X + 86, 468, COLOR_BLUE_BRIGHT);
        draw_text(scene, "• Cada DualShock 4 nativo guarda su perfil por ranura (ds4_slot_1.ini .. ds4_slot_4.ini).", fontCardDesc, CARD_X + 86, 496, COLOR_WHITE);
        draw_text(scene, "• Al reconectar al día siguiente o reiniciar PS4, ANYCONTROLLER restaura todo automáticamente.", fontCardDesc, CARD_X + 86, 524, COLOR_TEXT_DIM);

        // Card 4: PS4 DualSense Integration & Shortcuts (Y = 562..690, H = 128)
        draw_rounded_rect(scene, CARD_X, 562, INFO_W, 128, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, 562, INFO_W, 128, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconGuide, CARD_X + 22, 578);
        draw_text(scene, "Multijugador DualSense PS5 y Atajos del Sistema PS4", fontCardTitle, CARD_X + 86, 594, COLOR_WHITE);
        draw_text(scene, "• Botón PS directo por jugador en cada DualSense (sin mover el perfil del Jugador 1).", fontCardDesc, CARD_X + 86, 626, COLOR_WHITE);
        draw_text(scene, "• [TRIÁNGULO] en DualSense o (Create + Izquierda): abrir selector '¿Quién está usando este mando?'.", fontCardDesc, CARD_X + 86, 656, COLOR_BLUE_BRIGHT);

        // Card 5: Bottom Tips Strip (Y = 700..820, H = 120)
        draw_rounded_rect(scene, CARD_X, 700, INFO_W, 120, 12, Color{ 10, 18, 34 });
        draw_rounded_outline(scene, CARD_X, 700, INFO_W, 120, 12, COLOR_CARD_BORDER, 1);
        draw_rounded_rect(scene, CARD_X, 700, 6, 120, 3, COLOR_BLUE_MAIN);
        draw_text(scene, "HERENCIA AUTOMÁTICA Y CARGA USB-C", fontFooter, CARD_X + 22, 728, COLOR_BLUE_BRIGHT);
        draw_text(scene, "Cualquier mando sin perfil propio hereda automáticamente los Ajustes Globales.", fontCardDesc, CARD_X + 22, 760, COLOR_WHITE);
        draw_text(scene, "Para volver un mando personalizado a Global, selecciónalo con [L3] y pulsa [R3].", fontCardDesc, CARD_X + 22, 792, Color{ 0, 255, 160 });

        // Footer for Info (TV Overscan Safe)
        scene->DrawRectangle(SAFE_X, 844, SAFE_W, 2, COLOR_DIVIDER);
        draw_image(scene, iconInfo, SAFE_X, 868);
        draw_text(scene, "Sistema, Red y Acerca de", fontCardTitle, 154, 890, COLOR_WHITE);
        draw_text(scene, "ANYCONTROLLER " ONEFAMILY_VERSION_STRING " • github.com/toniigibertt/AnyController-PS4", fontFooter, 154, 916, COLOR_TEXT_DIM);

        draw_image(scene, btnCross, 995, 875);
        draw_text(scene, (upState == UPDATER_UPDATE_AVAILABLE) ? "Descargar PKG" : "Buscar Update", fontFooter, 1039, 900, Color{ 0, 255, 150 });

        draw_image(scene, btnSquare, 1225, 875);
        draw_text(scene, "Reinstalar Demonio", fontFooter, 1269, 900, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnCircle, 1480, 875);
        draw_text(scene, "Ajustes", fontFooter, 1524, 900, COLOR_TEXT_DIM);

        draw_image(scene, btnOptions, 1635, 878);
        draw_text(scene, "Salir", fontFooter, 1712, 900, COLOR_TEXT_DIM);
    }

    // 6. Floating Top Toast Notification (Inside TV Overscan Safe Area: Y=76)
    if (toastFrames > 0) {
        int boxW = 920;
        int boxH = 42;
        int boxX = (1920 - boxW) / 2;
        int boxY = 76;

        draw_rounded_rect(scene, boxX, boxY, boxW, boxH, 8, COLOR_TOAST_BG);
        draw_rounded_outline(scene, boxX, boxY, boxW, boxH, 8, COLOR_TOAST_BORDER, 2);
        const char* msg = (toastMsg && toastMsg[0]) ? toastMsg : "Ajustes guardados en /data/anycontroller/";
        draw_text_centered(scene, msg, fontCardDesc, 960, boxY + 28, COLOR_BLUE_BRIGHT);
    }
}

void UI::render_wizard(Scene2D* scene, const InstallResult& res, int frameID) {
    if (!scene) return;

    // Draw background with animated PlayStation XMB waves
    psBg.update(0.01666f);
    psBg.render(scene, cached_bg);

    // Top Header (TV Overscan Safe)
    draw_image(scene, xboxLogo, 108, 82);
    draw_text(scene, "Controller Control Center PS4", fontTitle, 224, 114, COLOR_WHITE);
    draw_text(scene, res.isUpdate ? "Guía de Actualización e Inicio (Payload: ANYCONTROLLER)" : "Guía de Instalación e Inicio de Mandos (Payload: ANYCONTROLLER)", fontSubtitle, 224, 148, COLOR_BLUE_BRIGHT);

    // Main Card
    int cardW = 1600;
    int cardH = 620;
    int cardX = 160;
    int cardY = 178;

    draw_rounded_rect(scene, cardX, cardY, cardW, cardH, 16, COLOR_CARD_BG);
    draw_rounded_outline(scene, cardX - 1, cardY - 1, cardW + 2, cardH + 2, 16, COLOR_BLUE_GLOW, 2);
    draw_rounded_outline(scene, cardX, cardY, cardW, cardH, 16, COLOR_BLUE_MAIN, 2);

    // Title inside card
    draw_image(scene, iconSave, cardX + 36, cardY + 32);
    draw_text(scene, res.isUpdate ? "Actualización de Controller Control Center PS4 Completada" : "Instalación de Controller Control Center PS4 Completada", fontCardTitle, cardX + 100, cardY + 56, COLOR_WHITE);
    draw_text(scene, res.isUpdate ? "Payload ANYCONTROLLER y configuración actualizados con éxito para GoldHEN:" : "Payload ANYCONTROLLER e inicio automático configurados con éxito para GoldHEN:", fontCardDesc, cardX + 100, cardY + 86, COLOR_TEXT_DIM);

    // Status Lines
    int lineY = cardY + 135;
    int lineStep = 40;

    // Item 1: Payload
    if (res.payloadCopied) {
        draw_text(scene, res.isUpdate ? "[OK] Payload ANYCONTROLLER:" : "[OK] Archivo Payload:", fontCardTitle, cardX + 50, lineY, COLOR_BLUE_BRIGHT);
        draw_text(scene, res.isUpdate ? "Actualizado con la versión más reciente (/data/payloads/anycontroller.bin)" : "Instalado en /data/payloads/anycontroller.bin", fontCardDesc, cardX + 350, lineY, COLOR_WHITE);
    } else {
        draw_text(scene, "[FAIL] Archivo Payload:", fontCardTitle, cardX + 50, lineY, Color{ 255, 80, 80 });
        draw_text(scene, "Error al copiar el payload a /data/payloads/anycontroller.bin", fontCardDesc, cardX + 350, lineY, COLOR_WHITE);
    }
    lineY += lineStep;

    // Item 2: Config
    if (res.configCreated) {
        draw_text(scene, "[OK] Configuración:", fontCardTitle, cardX + 50, lineY, COLOR_BLUE_BRIGHT);
        draw_text(scene, res.isUpdate ? "Actualizado /data/anycontroller/anycontroller.ini con los últimos ajustes" : "Inicializado /data/anycontroller/anycontroller.ini con ajustes óptimos", fontCardDesc, cardX + 350, lineY, COLOR_WHITE);
    } else {
        draw_text(scene, "[FAIL] Configuración:", fontCardTitle, cardX + 50, lineY, Color{ 255, 80, 80 });
        draw_text(scene, "Error al crear el archivo de configuración", fontCardDesc, cardX + 350, lineY, COLOR_WHITE);
    }
    lineY += lineStep;

    // Item 3: Auto-boot in payloads.ini
    if (res.autobootConfigured) {
        draw_text(scene, "[OK] Inicio Automático:", fontCardTitle, cardX + 50, lineY, COLOR_BLUE_BRIGHT);
        draw_text(scene, res.isUpdate ? "Verificado en /data/GoldHEN/payloads.ini ([AutoRun])" : "Registrado en /data/GoldHEN/payloads.ini ([AutoRun])", fontCardDesc, cardX + 350, lineY, COLOR_WHITE);
    } else {
        draw_text(scene, "[FAIL] Inicio Automático:", fontCardTitle, cardX + 50, lineY, Color{ 255, 80, 80 });
        draw_text(scene, "Error al registrar inicio automático en payloads.ini", fontCardDesc, cardX + 350, lineY, COLOR_WHITE);
    }

    // Divider
    int divY = cardY + 270;
    scene->DrawRectangle(cardX + 40, divY, cardW - 80, 2, COLOR_DIVIDER);

    // Startup Guide (4 Steps in Spanish)
    if (res.isUpdate) {
        draw_text(scene, "Pasos siguientes para aplicar la actualización:", fontCardTitle, cardX + 50, divY + 38, COLOR_WHITE);

        int stepY = divY + 76;
        draw_text(scene, "1. Pulsa (X) para abrir Controller Control Center PS4 y configurar tu mando.", fontCardDesc, cardX + 50, stepY, COLOR_WHITE);

        stepY += 36;
        draw_text(scene, "2. GoldHEN ejecutará ANYCONTROLLER al arrancar (Ajustes > PayLoader > Enable AutoRun).", fontCardDesc, cardX + 50, stepY, COLOR_WHITE);

        stepY += 36;
        draw_text(scene, "3. ¡Todas las funciones de color LED, gatillos, drift y vibración están activas!", fontCardDesc, cardX + 50, stepY, COLOR_BLUE_BRIGHT);

        stepY += 36;
        draw_text(scene, "4. IMPORTANTE: Reinicia tu PS4 una vez tras actualizar el PKG para cargar el demonio " ONEFAMILY_VERSION_STRING " en RAM.", fontCardDesc, cardX + 50, stepY, COLOR_TEXT_DIM);
    } else {
        draw_text(scene, "Pasos siguientes para activar tu mando en PS4:", fontCardTitle, cardX + 50, divY + 38, COLOR_WHITE);

        int stepY = divY + 76;
        draw_text(scene, "1. Pulsa (X) para abrir Controller Control Center PS4 y personalizar tu mando.", fontCardDesc, cardX + 50, stepY, COLOR_WHITE);

        stepY += 36;
        draw_text(scene, "2. Ejecuta GoldHEN (verifica en Ajustes > PayLoader > Enable AutoRun activado).", fontCardDesc, cardX + 50, stepY, COLOR_WHITE);

        stepY += 36;
        draw_text(scene, "3. El payload ANYCONTROLLER se iniciará automáticamente en cada arranque.", fontCardDesc, cardX + 50, stepY, COLOR_BLUE_BRIGHT);

        stepY += 36;
        draw_text(scene, "4. ¡Conecta tu mando por cable USB o Bluetooth nativo y a jugar!", fontCardDesc, cardX + 50, stepY, COLOR_TEXT_DIM);
    }

    // Footer (TV Overscan Safe)
    scene->DrawRectangle(SAFE_X, 844, SAFE_W, 2, COLOR_DIVIDER);
    draw_image(scene, iconInfo, SAFE_X, 868);
    draw_text(scene, "Instalación Inicial Completada", fontCardTitle, 154, 890, COLOR_WHITE);
    draw_text(scene, "El demonio ANYCONTROLLER arrancará automáticamente en cada reinicio de la consola", fontFooter, 154, 916, COLOR_TEXT_DIM);

    // Button X
    draw_image(scene, btnCross, 1530, 875);
    draw_text(scene, "Abrir Control Center", fontFooter, 1575, 900, COLOR_BLUE_BRIGHT);
}

void UI::render_tester(Scene2D* scene, const AppConfig& cfg, const OrbisPadData& padData, const TesterState& tester, bool dualsenseConnected, const char* targetShortLabel) {
    if (!scene) return;

    char buf[160];
    int deadzone_val = ConfigManager::DEADZONE_VALUES[cfg.deadzone_index];
    int thresh_val = ConfigManager::TRIGGER_VALUES[cfg.trigger_index];

    bool showDualSense = (tester.testerModelOverride == 1) ? true :
                         ((tester.testerModelOverride == 2) ? false : dualsenseConnected);

    // ========================================================
    // TOP BAR (TV Overscan Safe: Y = 148..216, H = 68)
    // ========================================================
    draw_rounded_rect(scene, CARD_X, 148, 1040, 68, 10, COLOR_CARD_BG);
    draw_rounded_outline(scene, CARD_X, 148, 1040, 68, 10, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconMode, CARD_X + 18, 162);
    draw_text(scene, "Laboratorio Original DualSense PS5 y DualShock 4", fontCardTitle, CARD_X + 74, 176, COLOR_WHITE);
    draw_text(scene, "Visor 2D fiel de los dos mandos originales • Iluminación en tiempo real y calibración", fontCardDesc, CARD_X + 74, 203, COLOR_TEXT_DIM);

    int testCardX = CARD_X + 1056;
    int testCardW = 356;
    int testCardH = 68;
    if (tester.testMode) {
        draw_rounded_rect(scene, testCardX, 148, testCardW, testCardH, 10, Color{ 15, 60, 40 });
        draw_rounded_outline(scene, testCardX, 148, testCardW, testCardH, 10, Color{ 0, 255, 150 }, 2);
        draw_image(scene, btnOptions, testCardX + 14, 166);
        draw_text(scene, "MODO PRUEBA: ACTIVO", fontCardTitle, testCardX + 74, 176, Color{ 0, 255, 150 });
        draw_text(scene, "Mantén OPTIONS (0.75s) salir", fontCardDesc, testCardX + 74, 203, COLOR_WHITE);
    } else {
        draw_rounded_rect(scene, testCardX, 148, testCardW, testCardH, 10, Color{ 14, 24, 38 });
        draw_rounded_outline(scene, testCardX, 148, testCardW, testCardH, 10, COLOR_CARD_BORDER, 1);
        draw_image(scene, btnCross, testCardX + 14, 164);
        draw_text(scene, "NAVEGACIÓN LIBRE", fontCardTitle, testCardX + 58, 176, COLOR_WHITE);
        draw_text(scene, "Usa Arriba/Abajo en el panel", fontCardDesc, testCardX + 58, 203, COLOR_BLUE_BRIGHT);
    }

    // ========================================================
    // LEFT COLUMN (X = 412..1070, W = 658)
    //   1) Interactive 5-Row Control Menu (Y = 226..442, H = 216)
    //   2) Stick L3 & R3 Cards (Y = 452..658, H = 206)
    //   3) Triggers L2/L1 & R1/R2 Card (Y = 668..826, H = 158)
    // ========================================================
    const int LCOL_X = CARD_X;
    const int LCOL_W = 658;

    // --- 1) Interactive Control Menu (Zero ambiguous button overlaps!) ---
    draw_rounded_rect(scene, LCOL_X, 226, LCOL_W, 216, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, LCOL_X, 226, LCOL_W, 216, 12, tester.testMode ? Color{ 0, 255, 150 } : COLOR_CARD_BORDER, 1);

    const char* slotConnTag = tester.slotConnected[tester.selectedSlot] ? "Conectado" : "Desconectado";
    static const char* modelModeLabels[3] = { "Automático", "PS5 DualSense", "PS4 DualShock 4" };

    for (int r = 0; r < 5; r++) {
        int ry = 235 + r * 40;
        bool isSel = (!tester.testMode && tester.testerMenuRow == r);
        draw_rounded_rect(scene, LCOL_X + 12, ry, LCOL_W - 24, 35, 7, isSel ? Color{ 18, 42, 76 } : Color{ 10, 16, 28 });
        if (isSel) {
            draw_rounded_outline(scene, LCOL_X + 12, ry, LCOL_W - 24, 35, 7, COLOR_BLUE_BRIGHT, 1);
            draw_rounded_rect(scene, LCOL_X + 12, ry + 6, 4, 23, 2, COLOR_BLUE_BRIGHT);
        }

        if (r == 0) {
            snprintf(buf, sizeof(buf), "Mando a probar: Jugador %d (%s)", tester.selectedSlot + 1, slotConnTag);
            draw_text(scene, buf, fontCardDesc, LCOL_X + 24, ry + 23, isSel ? COLOR_WHITE : COLOR_TEXT_DIM);
            draw_text(scene, "< Izq / Der >", fontFooter, LCOL_X + 528, ry + 23, isSel ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);
        } else if (r == 1) {
            int mv = (tester.testerModelOverride >= 0 && tester.testerModelOverride < 3) ? tester.testerModelOverride : 0;
            snprintf(buf, sizeof(buf), "Mando Visual 2D: %s", modelModeLabels[mv]);
            draw_text(scene, buf, fontCardDesc, LCOL_X + 24, ry + 23, isSel ? COLOR_WHITE : COLOR_TEXT_DIM);
            draw_text(scene, "< Izq / Der >", fontFooter, LCOL_X + 528, ry + 23, isSel ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);
        } else if (r == 2) {
            draw_text(scene, "Modo Prueba Libre (L3/R3/Cruceta sin salir)", fontCardDesc, LCOL_X + 24, ry + 23, isSel ? COLOR_WHITE : COLOR_TEXT_DIM);
            draw_text(scene, tester.testMode ? "[ACTIVO]" : "[X] Activar", fontFooter, LCOL_X + 528, ry + 23,
                      tester.testMode ? Color{ 0, 255, 160 } : (isSel ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM));
        } else if (r == 3) {
            const char* vibState = (tester.rumblePhase == 1) ? "Vibrando L..." :
                                   (tester.rumblePhase == 2) ? "Vibrando R..." :
                                   (tester.rumblePhase == 3) ? "Ambos 100%" : "[X] Probar";
            draw_text(scene, "Probar vibración SOLO en este mando", fontCardDesc, LCOL_X + 24, ry + 23, isSel ? COLOR_WHITE : COLOR_TEXT_DIM);
            draw_text(scene, vibState, fontFooter, LCOL_X + 528, ry + 23,
                      (tester.rumblePhase != 0) ? Color{ 0, 255, 160 } : (isSel ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM));
        } else {
            if (tester.calib.active) {
                snprintf(buf, sizeof(buf), "Analizando drift (%d%% • ruido max %d)",
                         (tester.calib.currentFrame * 100) / tester.calib.totalFrames, tester.calib.maxNoise);
                draw_text(scene, buf, fontCardDesc, LCOL_X + 24, ry + 23, COLOR_BLUE_BRIGHT);
                draw_text(scene, "Suelta sticks", fontFooter, LCOL_X + 528, ry + 23, Color{ 0, 255, 160 });
            } else if (tester.calib.completed) {
                snprintf(buf, sizeof(buf), "Drift detectado: %d -> Zona muerta: %s",
                         tester.calib.maxNoise, ConfigManager::DEADZONE_OPTIONS[tester.calib.suggestedIndex].c_str());
                draw_text(scene, buf, fontCardDesc, LCOL_X + 24, ry + 23, Color{ 0, 255, 160 });
                draw_text(scene, "[X] Aplicar", fontFooter, LCOL_X + 528, ry + 23, Color{ 0, 255, 160 });
            } else {
                draw_text(scene, "Calibrar deriva / anti-drift (3 segundos)", fontCardDesc, LCOL_X + 24, ry + 23, isSel ? COLOR_WHITE : COLOR_TEXT_DIM);
                draw_text(scene, "[X] Iniciar", fontFooter, LCOL_X + 528, ry + 23, isSel ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);
            }
        }
    }

    // --- 2) L3 & R3 Stick Telemetry Cards ---
    int stickR = 44;
    int colCardW = 321;
    int lsX = LCOL_X;
    int rsX = LCOL_X + 337;
    int lsY = 452;
    int lsH = 206;

    if (!padData.connected) {
        draw_rounded_rect(scene, LCOL_X, 452, LCOL_W, 374, 12, Color{ 16, 22, 38 });
        draw_rounded_outline(scene, LCOL_X, 452, LCOL_W, 374, 12, COLOR_BLUE_MAIN, 1);
        snprintf(buf, sizeof(buf), "Jugador %d (Ranura %d): Desconectado", tester.selectedSlot + 1, tester.selectedSlot + 1);
        draw_text(scene, buf, fontCardTitle, LCOL_X + 28, 520, COLOR_WHITE);
        draw_text(scene, "No hay ningún mando físico en esta ranura.", fontCardDesc, LCOL_X + 28, 558, COLOR_TEXT_DIM);
        draw_text(scene, "Usa [Izquierda / Derecha] en la fila 1 para cambiar de mando,", fontCardDesc, LCOL_X + 28, 598, COLOR_BLUE_BRIGHT);
        draw_text(scene, "o pulsa cualquier botón del mando que quieras conectar aquí.", fontCardDesc, LCOL_X + 28, 632, COLOR_TEXT_DIM);
    } else {
        draw_rounded_rect(scene, lsX, lsY, colCardW, lsH, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, lsX, lsY, colCardW, lsH, 12, COLOR_CARD_BORDER, 1);
        draw_text(scene, "Palanca L3", fontCardTitle, lsX + 18, lsY + 34, COLOR_WHITE);

        int lcx = lsX + 68;
        int lcy = lsY + 128;
        draw_circle_outline(scene, lcx, lcy, stickR, COLOR_CARD_BORDER, 2);
        scene->DrawRectangle(lcx - stickR, lcy, 2 * stickR, 1, COLOR_DIVIDER);
        scene->DrawRectangle(lcx, lcy - stickR, 1, 2 * stickR, COLOR_DIVIDER);
        int ldz_r = (deadzone_val * stickR) / 128;
        if (ldz_r > 0) draw_circle_outline(scene, lcx, lcy, ldz_r, COLOR_BLUE_GLOW, 1);

        int rawLX = (int)padData.leftStick.x;
        int rawLY = (int)padData.leftStick.y;
        int lDevX = rawLX - 128;
        int lDevY = rawLY - 128;
        int lsXPos = lcx + (lDevX * stickR) / 128;
        int lsYPos = lcy + (lDevY * stickR) / 128;
        int lDist = (int)sqrt((double)(lDevX * lDevX + lDevY * lDevY));
        bool lInDeadzone = (lDist <= deadzone_val);
        bool l3Pressed = (padData.buttons & ORBIS_PAD_BUTTON_L3) != 0;

        draw_circle(scene, lsXPos, lsYPos, 6, l3Pressed ? Color{ 255, 190, 45 } : (lInDeadzone ? Color{ 40, 80, 130 } : COLOR_BLUE_MAIN));
        draw_circle_outline(scene, lsXPos, lsYPos, 6, COLOR_WHITE, 1);

        int lTextX = lsX + 132;
        snprintf(buf, sizeof(buf), "X:%d (%+d%%)", rawLX, (lDevX * 100) / 128);
        draw_text(scene, buf, fontCardDesc, lTextX, lsY + 84, COLOR_WHITE);
        snprintf(buf, sizeof(buf), "Y:%d (%+d%%)", rawLY, (lDevY * 100) / 128);
        draw_text(scene, buf, fontCardDesc, lTextX, lsY + 112, COLOR_WHITE);
        snprintf(buf, sizeof(buf), "ZM: %s", ConfigManager::DEADZONE_OPTIONS[cfg.deadzone_index].c_str());
        draw_text(scene, buf, fontCardDesc, lTextX, lsY + 140, COLOR_TEXT_DIM);
        draw_text(scene, l3Pressed ? "L3: PULSADO" : (lInDeadzone ? "Estado: Reposo" : "Estado: Activo"),
                  fontCardDesc, lTextX, lsY + 170, (l3Pressed || !lInDeadzone) ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);

        // Right Stick Card
        draw_rounded_rect(scene, rsX, lsY, colCardW, lsH, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, rsX, lsY, colCardW, lsH, 12, COLOR_CARD_BORDER, 1);
        draw_text(scene, "Palanca R3", fontCardTitle, rsX + 18, lsY + 34, COLOR_WHITE);

        int rcx = rsX + 68;
        int rcy = lsY + 128;
        draw_circle_outline(scene, rcx, rcy, stickR, COLOR_CARD_BORDER, 2);
        scene->DrawRectangle(rcx - stickR, rcy, 2 * stickR, 1, COLOR_DIVIDER);
        scene->DrawRectangle(rcx, rcy - stickR, 1, 2 * stickR, COLOR_DIVIDER);
        int rdz_r = (deadzone_val * stickR) / 128;
        if (rdz_r > 0) draw_circle_outline(scene, rcx, rcy, rdz_r, COLOR_BLUE_GLOW, 1);

        int rawRX = (int)padData.rightStick.x;
        int rawRY = (int)padData.rightStick.y;
        int rDevX = rawRX - 128;
        int rDevY = rawRY - 128;
        int rsXPos = rcx + (rDevX * stickR) / 128;
        int rsYPos = rcy + (rDevY * stickR) / 128;
        int rDist = (int)sqrt((double)(rDevX * rDevX + rDevY * rDevY));
        bool rInDeadzone = (rDist <= deadzone_val);
        bool r3Pressed = (padData.buttons & ORBIS_PAD_BUTTON_R3) != 0;

        draw_circle(scene, rsXPos, rsYPos, 6, r3Pressed ? Color{ 255, 190, 45 } : (rInDeadzone ? Color{ 40, 80, 130 } : COLOR_BLUE_MAIN));
        draw_circle_outline(scene, rsXPos, rsYPos, 6, COLOR_WHITE, 1);

        int rTextX = rsX + 132;
        snprintf(buf, sizeof(buf), "X:%d (%+d%%)", rawRX, (rDevX * 100) / 128);
        draw_text(scene, buf, fontCardDesc, rTextX, lsY + 84, COLOR_WHITE);
        snprintf(buf, sizeof(buf), "Y:%d (%+d%%)", rawRY, (rDevY * 100) / 128);
        draw_text(scene, buf, fontCardDesc, rTextX, lsY + 112, COLOR_WHITE);
        snprintf(buf, sizeof(buf), "ZM: %s", ConfigManager::DEADZONE_OPTIONS[cfg.deadzone_index].c_str());
        draw_text(scene, buf, fontCardDesc, rTextX, lsY + 140, COLOR_TEXT_DIM);
        draw_text(scene, r3Pressed ? "R3: PULSADO" : (rInDeadzone ? "Estado: Reposo" : "Estado: Activo"),
                  fontCardDesc, rTextX, lsY + 170, (r3Pressed || !rInDeadzone) ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);

        // --- 3) Triggers & Bumpers Card ---
        int trigX = LCOL_X;
        int trigY = 668;
        int trigW = LCOL_W;
        int trigH = 158;
        draw_rounded_rect(scene, trigX, trigY, trigW, trigH, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, trigX, trigY, trigW, trigH, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconSwap, trigX + 18, trigY + 12);
        draw_text(scene, "Gatillos L2/R2 y Hombros L1/R1", fontCardTitle, trigX + 70, trigY + 34, COLOR_WHITE);

        int rawLT = (int)padData.analogButtons.l2;
        bool l2Btn = (padData.buttons & ORBIS_PAD_BUTTON_L2) != 0;
        bool lbBtn = (padData.buttons & ORBIS_PAD_BUTTON_L1) != 0;

        int ltBarX = trigX + 18;
        int ltBarY = trigY + 94;
        int ltBarW = 200;
        int ltBarH = 16;
        snprintf(buf, sizeof(buf), "L2: %d/255 (%d%%)", rawLT, (rawLT * 100) / 255);
        draw_text(scene, buf, fontCardDesc, ltBarX, trigY + 78, l2Btn ? COLOR_BLUE_BRIGHT : COLOR_WHITE);
        draw_rounded_rect(scene, ltBarX, ltBarY, ltBarW, ltBarH, 4, Color{ 18, 30, 48 });
        int ltFillW = (rawLT * ltBarW) / 255;
        if (ltFillW > 0) draw_rounded_rect(scene, ltBarX, ltBarY, ltFillW, ltBarH, 4, l2Btn ? COLOR_BLUE_MAIN : Color{ 15, 70, 140 });
        scene->DrawRectangle(ltBarX + (thresh_val * ltBarW) / 255, ltBarY - 3, 2, ltBarH + 6, COLOR_WHITE);
        draw_btn_badge(scene, ltBarX + ltBarW + 12, trigY + 82, 70, 32, "L1", lbBtn);

        int rawRT = (int)padData.analogButtons.r2;
        bool r2Btn = (padData.buttons & ORBIS_PAD_BUTTON_R2) != 0;
        bool rbBtn = (padData.buttons & ORBIS_PAD_BUTTON_R1) != 0;

        int rtBarX = trigX + 340;
        int rtBarY = trigY + 94;
        int rtBarW = 200;
        int rtBarH = 16;
        snprintf(buf, sizeof(buf), "R2: %d/255 (%d%%)", rawRT, (rawRT * 100) / 255);
        draw_text(scene, buf, fontCardDesc, rtBarX, trigY + 78, r2Btn ? COLOR_BLUE_BRIGHT : COLOR_WHITE);
        draw_rounded_rect(scene, rtBarX, rtBarY, rtBarW, rtBarH, 4, Color{ 18, 30, 48 });
        int rtFillW = (rawRT * rtBarW) / 255;
        if (rtFillW > 0) draw_rounded_rect(scene, rtBarX, rtBarY, rtFillW, rtBarH, 4, r2Btn ? COLOR_BLUE_MAIN : Color{ 15, 70, 140 });
        scene->DrawRectangle(rtBarX + (thresh_val * rtBarW) / 255, rtBarY - 3, 2, rtBarH + 6, COLOR_WHITE);
        draw_btn_badge(scene, rtBarX + rtBarW + 12, trigY + 82, 70, 32, "R1", rbBtn);
        snprintf(buf, sizeof(buf), "Perfil aplicado en vista: %s", targetShortLabel ? targetShortLabel : "Global");
        draw_text(scene, buf, fontFooter, trigX + 18, trigY + 138, COLOR_TEXT_DIM);
    }

    // ========================================================
    // RIGHT HERO COLUMN: ORIGINAL 2D PS5 DUALSENSE / PS4 DUALSHOCK 4 (X=1086..1824, W=738, Y=226..826)
    // ========================================================
    int viewCardX = 1086;
    int viewCardY = 226;
    int viewCardW = 738;
    int viewCardH = 600;

    draw_rounded_rect(scene, viewCardX, viewCardY, viewCardW, viewCardH, 14, Color{ 7, 12, 24 });
    draw_rounded_outline(scene, viewCardX, viewCardY, viewCardW, viewCardH, 14,
                         tester.testMode ? Color{ 0, 240, 170 } : Color{ 0, 140, 235 }, 2);

    draw_rounded_rect(scene, viewCardX + 14, viewCardY + 10, viewCardW - 28, 42, 8, Color{ 12, 22, 40 });
    draw_image(scene, btnPs, viewCardX + 22, viewCardY + 15);
    draw_text(scene, showDualSense ? "MANDO ORIGINAL PS5 DUALSENSE" : "MANDO ORIGINAL PS4 DUALSHOCK 4",
              fontCardTitle, viewCardX + 64, viewCardY + 38, COLOR_WHITE);

    char slotBadge[40];
    snprintf(slotBadge, sizeof(slotBadge), "JUGADOR %d", tester.selectedSlot + 1);
    draw_rounded_rect(scene, viewCardX + viewCardW - 176, viewCardY + 15, 152, 32, 6, Color{ 16, 40, 74 });
    draw_rounded_outline(scene, viewCardX + viewCardW - 176, viewCardY + 15, 152, 32, 6,
                         showDualSense ? COLOR_BLUE_BRIGHT : Color{ 129, 140, 248 }, 1);
    draw_text_centered(scene, slotBadge, fontSubtitle, viewCardX + viewCardW - 100, viewCardY + 37, COLOR_WHITE);

    // Render crisp Original 2D DualSense or DualShock 4
    controller3D.render(scene, viewCardX + 12, viewCardY + 56, viewCardW - 24, 488,
                        showDualSense, cfg, padData, tester.rumblePhase, tester.selectedSlot);

    // Bottom Live Pressed Buttons Readout Strip (or Test Mode Hold-to-Exit bar)
    int rawLT2 = (int)padData.analogButtons.l2;
    int rawRT2 = (int)padData.analogButtons.r2;
    std::string pressedStr;
    auto addPressed = [&](const char* name) {
        if (!pressedStr.empty()) pressedStr += "  •  ";
        pressedStr += name;
    };
    if (padData.buttons & ORBIS_PAD_BUTTON_CROSS)     addPressed("✕ Cruz");
    if (padData.buttons & ORBIS_PAD_BUTTON_CIRCLE)    addPressed("○ Círculo");
    if (padData.buttons & ORBIS_PAD_BUTTON_SQUARE)    addPressed("□ Cuadrado");
    if (padData.buttons & ORBIS_PAD_BUTTON_TRIANGLE)  addPressed("△ Triángulo");
    if (padData.buttons & ORBIS_PAD_BUTTON_UP)        addPressed("▲ Arriba");
    if (padData.buttons & ORBIS_PAD_BUTTON_DOWN)      addPressed("▼ Abajo");
    if (padData.buttons & ORBIS_PAD_BUTTON_LEFT)      addPressed("◀ Izq");
    if (padData.buttons & ORBIS_PAD_BUTTON_RIGHT)     addPressed("▶ Der");
    if (padData.buttons & ORBIS_PAD_BUTTON_L1)        addPressed("L1");
    if (padData.buttons & ORBIS_PAD_BUTTON_R1)        addPressed("R1");
    if ((padData.buttons & ORBIS_PAD_BUTTON_L2) || rawLT2 > 20) addPressed("L2");
    if ((padData.buttons & ORBIS_PAD_BUTTON_R2) || rawRT2 > 20) addPressed("R2");
    if (padData.buttons & ORBIS_PAD_BUTTON_L3)        addPressed("L3");
    if (padData.buttons & ORBIS_PAD_BUTTON_R3)        addPressed("R3");
    if (padData.buttons & 0x00000001U)                addPressed(showDualSense ? "Create" : "Share");
    if (padData.buttons & ORBIS_PAD_BUTTON_OPTIONS)   addPressed("Options");
    if (padData.buttons & 0x00010000U)                addPressed("PS");
    if (padData.buttons & ORBIS_PAD_BUTTON_TOUCH_PAD) addPressed("Clic Táctil");
    else if (padData.touch.fingers > 0)               addPressed("Dedo Táctil");

    draw_rounded_rect(scene, viewCardX + 14, viewCardY + viewCardH - 52, viewCardW - 28, 40, 8, Color{ 10, 18, 34 });
    draw_rounded_outline(scene, viewCardX + 14, viewCardY + viewCardH - 52, viewCardW - 28, 40, 8,
                         pressedStr.empty() ? COLOR_BORDER_DIM : Color{ 0, 255, 160 }, 1);
    draw_text_centered(scene,
                       pressedStr.empty() ? "Pulsa cualquier botón, mueve sticks o desliza el Touchpad" : pressedStr.c_str(),
                       fontCardDesc,
                       viewCardX + viewCardW / 2,
                       viewCardY + viewCardH - 25,
                       pressedStr.empty() ? COLOR_TEXT_DIM : Color{ 0, 255, 160 });

    // ========================================================
    // FOOTER FOR TESTER (TV Overscan Safe)
    // ========================================================
    scene->DrawRectangle(SAFE_X, 844, SAFE_W, 2, COLOR_DIVIDER);
    draw_image(scene, iconMode, SAFE_X, 868);
    snprintf(buf, sizeof(buf), "Laboratorio Original 2D - Jugador %d (Ranura %d)", tester.selectedSlot + 1, tester.selectedSlot + 1);
    draw_text(scene, buf, fontCardTitle, 154, 890, COLOR_WHITE);

    if (tester.testMode) {
        if (tester.testModeHoldFrames > 0) {
            int pct = (tester.testModeHoldFrames * 100) / 45;
            if (pct > 100) pct = 100;
            snprintf(buf, sizeof(buf), "Saliendo del Modo Prueba Libre: %d%%...", pct);
            draw_text(scene, buf, fontFooter, 154, 916, COLOR_BLUE_BRIGHT);
            draw_rounded_rect(scene, 780, 882, 520, 22, 6, Color{ 16, 28, 48 });
            draw_rounded_rect(scene, 780, 882, (520 * pct) / 100, 22, 6, Color{ 0, 255, 160 });
        } else {
            draw_text(scene, "Modo Prueba Libre ACTIVO: todos los botones solo iluminan el mando en pantalla", fontFooter, 154, 916, Color{ 0, 255, 150 });
        }
        draw_image(scene, btnOptions, 1345, 878);
        draw_text(scene, "Mantén OPTIONS (0.75s) para salir", fontFooter, 1425, 900, Color{ 0, 255, 150 });
    } else {
        draw_text(scene, "Usa Arriba/Abajo en el menú izquierdo para elegir fila sin solapes", fontFooter, 154, 916, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnArrows, 1135, 879);
        draw_text(scene, "Seleccionar", fontFooter, 1188, 900, COLOR_TEXT_DIM);

        draw_image(scene, btnCross, 1355, 875);
        draw_text(scene, "Ejecutar", fontFooter, 1399, 900, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnCircle, 1510, 875);
        draw_text(scene, "Ajustes", fontFooter, 1554, 900, COLOR_TEXT_DIM);

        draw_image(scene, btnOptions, 1635, 878);
        draw_text(scene, "Salir", fontFooter, 1712, 900, COLOR_TEXT_DIM);
    }
}

void UI::render_remap(Scene2D* scene, const HidStatus& status, const HidProfile& profile,
                      int selectedRow, const OrbisPadData& padData, bool testMode) {
    if (!scene) return;

    // Header Area (TV Overscan Safe: Y=148..230)
    draw_rounded_rect(scene, CARD_X, 148, 1040, 82, 10, COLOR_CARD_BG);
    draw_rounded_outline(scene, CARD_X, 148, 1040, 82, 10, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconRemap, CARD_X + 18, 148 + (82 - iconRemap.height) / 2);

    char devTitle[128];
    snprintf(devTitle, sizeof(devTitle), "Dispositivo HID: %s", status.name.c_str());
    draw_text(scene, devTitle, fontCardTitle, CARD_X + 74, 179, COLOR_WHITE);

    char devSub[256];
    if (status.player_slot > 0 && !status.user_name.empty()) {
        snprintf(devSub, sizeof(devSub), "Jugador %d (%s)   •   Hardware: VID 0x%04X | PID 0x%04X   •   Perfil: %04X_%04X.ini",
                 status.player_slot, status.user_name.c_str(), status.vid, status.pid, status.vid, status.pid);
    } else {
        snprintf(devSub, sizeof(devSub), "Hardware: VID 0x%04X | PID 0x%04X   •   Botones: %d   •   Perfil: %04X_%04X.ini",
                 status.vid, status.pid, status.nbuttons, status.vid, status.pid);
    }
    draw_text(scene, devSub, fontCardDesc, CARD_X + 74, 209, COLOR_TEXT_DIM);

    int testCardX = CARD_X + 1056;
    int testCardW = 356;
    int testCardH = 82;

    if (testMode) {
        Color badgeBg = Color{ 15, 60, 40 };
        Color badgeBorder = Color{ 0, 255, 150 };
        draw_rounded_rect(scene, testCardX, 148, testCardW, testCardH, 10, badgeBg);
        draw_rounded_outline(scene, testCardX, 148, testCardW, testCardH, 10, badgeBorder, 2);
        draw_image(scene, btnR3, testCardX + 14, 148 + (testCardH - btnR3.height) / 2);
        draw_text(scene, "MODO PRUEBA: ACTIVO", fontCardTitle, testCardX + 58, 179, Color{ 0, 255, 150 });
        draw_text(scene, "Pulsa R3 para volver", fontCardDesc, testCardX + 58, 209, COLOR_WHITE);
    } else {
        Color badgeBg = Color{ 14, 24, 38 };
        Color badgeBorder = COLOR_CARD_BORDER;
        draw_rounded_rect(scene, testCardX, 148, testCardW, testCardH, 10, badgeBg);
        draw_rounded_outline(scene, testCardX, 148, testCardW, testCardH, 10, badgeBorder, 1);
        draw_image(scene, btnR3, testCardX + 14, 148 + (testCardH - btnR3.height) / 2);
        draw_text(scene, "MODO PRUEBA: APAGADO", fontCardTitle, testCardX + 58, 179, COLOR_WHITE);
        draw_text(scene, "Pulsa R3 para probar botones", fontCardDesc, testCardX + 58, 209, COLOR_BLUE_BRIGHT);
    }

    // Grid of 14 actions (2 columns x 7 rows, safe X=412..1824, Y=244..768)
    const int COL_W = 686;
    const int COL_GAP = 40;
    const int ROW_H = 68;
    const int ROW_GAP = 8;
    const int GRID_Y = 244;

    for (int i = 0; i < ACT_COUNT; i++) {
        int col = (i < 7) ? 0 : 1;
        int row = (i < 7) ? i : (i - 7);
        int itemX = CARD_X + col * (COL_W + COL_GAP);
        int itemY = GRID_Y + row * (ROW_H + ROW_GAP);

        bool isSelected = (!testMode && selectedRow == i);
        bool isPressed = (padData.buttons & HidProfile::ACTIONS[i].ds4_bit) != 0;

        Color bg = isPressed ? Color{ 15, 90, 55 } : (isSelected ? COLOR_SIDEBAR_BLUE : COLOR_CARD_BG);
        Color border = isPressed ? Color{ 0, 255, 150 } : (isSelected ? COLOR_BLUE_BRIGHT : COLOR_CARD_BORDER);

        draw_rounded_rect(scene, itemX, itemY, COL_W, ROW_H, 8, bg);
        draw_rounded_outline(scene, itemX, itemY, COL_W, ROW_H, 8, border, (isSelected || isPressed) ? 2 : 1);

        // Official Button Icon
        const ImageRGBA* iconToDraw = &btnCross;
        switch (i) {
            case ACT_CROSS:    iconToDraw = &btnCross;    break;
            case ACT_CIRCLE:   iconToDraw = &btnCircle;   break;
            case ACT_SQUARE:   iconToDraw = &btnSquare;   break;
            case ACT_TRIANGLE: iconToDraw = &btnTriangle; break;
            case ACT_L1:       iconToDraw = &btnL1;       break;
            case ACT_R1:       iconToDraw = &btnR1;       break;
            case ACT_L2:       iconToDraw = &btnL2;       break;
            case ACT_R2:       iconToDraw = &btnR2;       break;
            case ACT_SHARE:    iconToDraw = &btnShare;    break;
            case ACT_OPTIONS:  iconToDraw = &btnOptions;  break;
            case ACT_PS:       iconToDraw = &btnPs;       break;
            case ACT_TOUCHPAD: iconToDraw = &btnTouchpad; break;
            case ACT_L3:       iconToDraw = &btnL3;       break;
            case ACT_R3:       iconToDraw = &btnR3;       break;
        }

        int iconX = itemX + 18;
        int iconY = itemY + (ROW_H - iconToDraw->height) / 2;
        draw_image(scene, *iconToDraw, iconX, iconY);

        // Action Name
        int textX = iconX + iconToDraw->width + 16;
        draw_text(scene, HidProfile::ACTIONS[i].display_name, fontCardTitle, textX, itemY + 42, isSelected ? COLOR_WHITE : Color{ 210, 225, 245 });

        // Assigned button pill on the right
        int assigned = profile.get_assigned_button(i);
        char btnLabel[32];
        if (assigned == 0) {
            snprintf(btnLabel, sizeof(btnLabel), "Sin asignar");
        } else {
            snprintf(btnLabel, sizeof(btnLabel), "Boton %d", assigned);
        }

        int pillW = 180;
        int pillX = itemX + COL_W - pillW - 16;
        int pillY = itemY + 15;
        int pillH = 38;

        Color pillBg = isSelected ? Color{ 20, 50, 90 } : Color{ 14, 24, 38 };
        Color pillBorder = isSelected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM;
        draw_rounded_rect(scene, pillX, pillY, pillW, pillH, 6, pillBg);
        draw_rounded_outline(scene, pillX, pillY, pillW, pillH, 6, pillBorder, 1);

        if (isSelected) {
            draw_text(scene, "<", fontValue, pillX + 12, pillY + 27, COLOR_BLUE_BRIGHT);
            draw_text_centered(scene, btnLabel, fontCardTitle, pillX + pillW / 2, pillY + 27, COLOR_WHITE);
            draw_text(scene, ">", fontValue, pillX + pillW - 22, pillY + 27, COLOR_BLUE_BRIGHT);
        } else {
            draw_text_centered(scene, btnLabel, fontCardDesc, pillX + pillW / 2, pillY + 26, (assigned == 0) ? Color{ 110, 130, 150 } : (isPressed ? Color{ 0, 255, 150 } : COLOR_TEXT_DIM));
        }
    }

    // Footer with exact PlayStation Button Icons (TV Overscan Safe)
    scene->DrawRectangle(SAFE_X, 844, SAFE_W, 2, COLOR_DIVIDER);

    if (testMode) {
        draw_image(scene, iconMode, SAFE_X, 868);
        draw_text(scene, "Modo Prueba de Entradas HID Activo", fontCardTitle, 154, 890, Color{ 0, 255, 150 });
        draw_text(scene, "Acciones pausadas. Pulsa botones físicos para ver el mapeo en tiempo real.", fontFooter, 154, 916, COLOR_WHITE);

        draw_image(scene, btnR3, 1560, 875);
        draw_text(scene, "Salir Modo Prueba", fontFooter, 1608, 900, Color{ 0, 255, 150 });
    } else {
        draw_image(scene, iconRemap, SAFE_X, 868);
        draw_text(scene, "Reasignación de Botones HID", fontCardTitle, 154, 890, COLOR_WHITE);
        std::string profileMsg = profile.get_current_path().empty() ?
            "Perfil activo en /data/anycontroller/controllers/" :
            ("Perfil activo: " + profile.get_current_path());
        draw_text(scene, profileMsg.c_str(), fontFooter, 154, 916, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnR3, 860, 875);
        draw_text(scene, "Modo Prueba", fontFooter, 904, 900, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnCross, 1055, 875);
        draw_text(scene, "Guardar", fontFooter, 1099, 900, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnSquare, 1205, 875);
        draw_text(scene, "Defecto", fontFooter, 1249, 900, COLOR_TEXT_DIM);

        draw_image(scene, btnTriangle, 1355, 875);
        draw_text(scene, "Recargar", fontFooter, 1399, 900, COLOR_TEXT_DIM);

        draw_image(scene, btnArrows, 1510, 879);
        draw_text(scene, "Valor", fontFooter, 1562, 900, COLOR_TEXT_DIM);

        draw_image(scene, btnOptions, 1635, 878);
        draw_text(scene, "Salir", fontFooter, 1712, 900, COLOR_TEXT_DIM);
    }
}

void UI::render_dualsense(Scene2D* scene, const AppConfig& cfg, int selectedCard, const HidStatus& status, const std::vector<ControllerTarget>* targets, int activeTargetIndex) {
    draw_target_selector_strip(scene, targets, activeTargetIndex);

    // Top Exclusive Banner for PS5 DualSense (TV Overscan Safe: Y=196..264)
    draw_rounded_rect(scene, CARD_X, 196, CARD_W, 68, 12, Color{ 13, 24, 46 });
    draw_rounded_outline(scene, CARD_X, 196, CARD_W, 68, 12, COLOR_BLUE_MAIN, 1);
    draw_rounded_rect(scene, CARD_X, 196, 6, 68, 3, COLOR_BLUE_BRIGHT);
    draw_image(scene, btnPs, CARD_X + 20, 209);
    draw_text(scene, "Centro Exclusivo para Mandos PlayStation 5 (DualSense / DualSense Edge)", fontCardTitle, CARD_X + 78, 223, COLOR_WHITE);
    draw_text(scene, "Cambia de mando con [L3] para guardar colores y gatillos distintos en cada DualSense por MAC Bluetooth.", fontCardDesc, CARD_X + 78, 249, COLOR_BLUE_BRIGHT);

    const int TOTAL_CARDS = 8;
    const int VISIBLE_CARDS = 5;

    int scrollOffset = 0;
    if (selectedCard >= VISIBLE_CARDS) {
        scrollOffset = selectedCard - VISIBLE_CARDS + 1;
    }
    if (scrollOffset > TOTAL_CARDS - VISIBLE_CARDS) {
        scrollOffset = TOTAL_CARDS - VISIBLE_CARDS;
    }

    const char* cardTitles[TOTAL_CARDS] = {
        "Modo de Iluminación DualSense",
        "Color Fijo del Halo DualSense",
        "Intensidad del Halo Luminoso PS5",
        "Gatillos Adaptativos PS5 (L2/R2)",
        "LEDs Blancos de Jugador (Touchpad)",
        "Volumen del Altavoz DualSense",
        "Función del Botón Físico Mute",
        "Protocolo Bluetooth (10B / 78B)"
    };

    const char* cardDescs[TOTAL_CARDS] = {
        "Comportamiento del LED lateral del Touchpad.",
        "Color RGB fijo para el halo del mando PS5.",
        "Potencia de brillo del halo y LEDs de jugador.",
        "Resistencia mecánica en los gatillos L2 y R2.",
        "Configura los 5 LEDs blancos bajo el Touchpad.",
        "Ganancia de audio del altavoz interno PS5.",
        "Usa el botón Mute como clic rápido Touchpad.",
        "Modo 78B activa gatillos, giroscopio y LEDs."
    };

    const ImageRGBA* cardIcons[TOTAL_CARDS] = {
        &iconGear,
        &iconMode,
        &iconGear,
        &iconSwap,
        &iconMode,
        &iconRumble,
        &iconRemap,
        &iconGear
    };

    std::string cardValues[TOTAL_CARDS];
    if (cfg.ds_lightbar_mode >= 0 && cfg.ds_lightbar_mode < (int)ConfigManager::DS_LIGHTBAR_OPTIONS.size()) {
        cardValues[0] = ConfigManager::DS_LIGHTBAR_OPTIONS[cfg.ds_lightbar_mode];
    } else cardValues[0] = "Sincronizar Juego";

    if (cfg.ds_lightbar_color >= 0 && cfg.ds_lightbar_color < (int)ConfigManager::DS_COLOR_OPTIONS.size()) {
        cardValues[1] = ConfigManager::DS_COLOR_OPTIONS[cfg.ds_lightbar_color];
    } else cardValues[1] = "Azul PlayStation";

    if (cfg.ds_brightness >= 0 && cfg.ds_brightness < (int)ConfigManager::DS_BRIGHTNESS_OPTIONS.size()) {
        cardValues[2] = ConfigManager::DS_BRIGHTNESS_OPTIONS[cfg.ds_brightness];
    } else cardValues[2] = "Alto";

    if (cfg.ds_trigger_mode >= 0 && cfg.ds_trigger_mode < (int)ConfigManager::DS_TRIGGER_OPTIONS.size()) {
        cardValues[3] = ConfigManager::DS_TRIGGER_OPTIONS[cfg.ds_trigger_mode];
    } else cardValues[3] = "Estandar / Off";

    if (cfg.ds_player_leds >= 0 && cfg.ds_player_leds < (int)ConfigManager::DS_PLAYER_LEDS_OPTIONS.size()) {
        cardValues[4] = ConfigManager::DS_PLAYER_LEDS_OPTIONS[cfg.ds_player_leds];
    } else cardValues[4] = "Ranura Jugador";

    char volStr[32];
    if (cfg.ds_speaker_volume <= 0) {
        snprintf(volStr, sizeof(volStr), "0%% (Silenciado)");
    } else {
        snprintf(volStr, sizeof(volStr), "%d%%", cfg.ds_speaker_volume);
    }
    cardValues[5] = volStr;

    if (cfg.ds_mute_action >= 0 && cfg.ds_mute_action < (int)ConfigManager::DS_MUTE_OPTIONS.size()) {
        cardValues[6] = ConfigManager::DS_MUTE_OPTIONS[cfg.ds_mute_action];
    } else cardValues[6] = "Por Defecto";

    if (cfg.ds_bt_mode >= 0 && cfg.ds_bt_mode < (int)ConfigManager::DS_BT_MODE_OPTIONS.size()) {
        cardValues[7] = ConfigManager::DS_BT_MODE_OPTIONS[cfg.ds_bt_mode];
    } else cardValues[7] = "Modo Seguro (10 Bytes)";

    static const Color colorPreviews[6] = {
        Color{ 0, 112, 209 },  // Blue
        Color{ 255, 40, 40 },  // Red
        Color{ 40, 220, 40 },  // Green
        Color{ 160, 40, 255 }, // Purple
        Color{ 0, 230, 255 },  // Cyan
        Color{ 245, 245, 245 } // White
    };

    const int DS_CARD_X = CARD_X;
    const int DS_CARD_W = 868;
    const int DS_CARD_H = 74;
    const int DS_CARD_SPACING = 8;

    for (int i = 0; i < VISIBLE_CARDS; i++) {
        int cardIdx = i + scrollOffset;
        if (cardIdx >= TOTAL_CARDS) break;

        int cardY = CARD_Y_START + i * (DS_CARD_H + DS_CARD_SPACING);
        bool isSelected = (cardIdx == selectedCard);

        draw_rounded_rect(scene, DS_CARD_X, cardY, DS_CARD_W, DS_CARD_H, 12, isSelected ? Color{ 16, 30, 54 } : COLOR_CARD_BG);

        if (isSelected) {
            draw_rounded_outline(scene, DS_CARD_X - 1, cardY - 1, DS_CARD_W + 2, DS_CARD_H + 2, 12, COLOR_BLUE_GLOW, 2);
            draw_rounded_outline(scene, DS_CARD_X, cardY, DS_CARD_W, DS_CARD_H, 12, COLOR_BLUE_MAIN, 2);
            draw_rounded_rect(scene, DS_CARD_X, cardY + 12, 5, DS_CARD_H - 24, 2, COLOR_BLUE_BRIGHT);
        } else {
            draw_rounded_outline(scene, DS_CARD_X, cardY, DS_CARD_W, DS_CARD_H, 12, COLOR_CARD_BORDER, 1);
        }

        if (cardIcons[cardIdx]) {
            draw_image(scene, *cardIcons[cardIdx], DS_CARD_X + 18, cardY + 14);
        }

        draw_text(scene, cardTitles[cardIdx], fontCardTitle, DS_CARD_X + 76, cardY + 31, COLOR_WHITE);
        draw_text(scene, cardDescs[cardIdx], fontCardDesc, DS_CARD_X + 76, cardY + 57, COLOR_TEXT_DIM);

        int vBoxW = 284;
        int vBoxH = 42;
        int vBoxX = DS_CARD_X + DS_CARD_W - vBoxW - 16;
        int vBoxY = cardY + (DS_CARD_H - vBoxH) / 2;
        draw_rounded_rect(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? Color{ 10, 22, 42 } : Color{ 8, 13, 24 });
        draw_rounded_outline(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM, 1);

        if (isSelected) {
            draw_text(scene, "<", fontValue, vBoxX + 12, cardY + 45, COLOR_BLUE_BRIGHT);
            draw_text(scene, ">", fontValue, vBoxX + vBoxW - 24, cardY + 45, COLOR_BLUE_BRIGHT);
        }

        if (cardIdx == 1) {
            int c_idx = cfg.ds_lightbar_color;
            if (c_idx >= 0 && c_idx < 6) {
                draw_circle(scene, vBoxX + 34, cardY + 37, 7, colorPreviews[c_idx]);
                draw_circle_outline(scene, vBoxX + 34, cardY + 37, 7, COLOR_WHITE, 1);
            }
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2 + 8, cardY + 45, COLOR_WHITE);
        } else if (cardIdx == 5) {
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2, cardY + 36, COLOR_WHITE);
            int vBarX = vBoxX + 52;
            int vBarY = cardY + 50;
            int vBarW = 180;
            int vBarH = 6;
            draw_rounded_rect(scene, vBarX, vBarY, vBarW, vBarH, 3, Color{ 15, 25, 40 });
            draw_rounded_outline(scene, vBarX, vBarY, vBarW, vBarH, 3, COLOR_BORDER_DIM, 1);
            int fillW = (vBarW * cfg.ds_speaker_volume) / 100;
            if (fillW > 0) {
                Color fillCol = (cfg.ds_speaker_volume > 0) ? COLOR_BLUE_BRIGHT : Color{ 80, 80, 80 };
                draw_rounded_rect(scene, vBarX, vBarY, fillW, vBarH, 3, fillCol);
            }
        } else {
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2, cardY + 45, COLOR_WHITE);
        }
    }

    if (scrollOffset > 0) {
        draw_text(scene, "^", fontCardTitle, DS_CARD_X + DS_CARD_W / 2, 272, COLOR_BLUE_BRIGHT);
    }
    if (scrollOffset + VISIBLE_CARDS < TOTAL_CARDS) {
        draw_text(scene, "v", fontCardTitle, DS_CARD_X + DS_CARD_W / 2, 700, COLOR_BLUE_BRIGHT);
    }

    // Right-Hand PS5 DualSense Multi-Controller Telemetry Box (TV Overscan Safe: Y=274..676, H=402)
    const int BOX_X = 1298;
    const int BOX_Y = 274;
    const int BOX_W = 526;
    const int BOX_H = 402;

    draw_rounded_rect(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BORDER, 1);

    draw_image(scene, btnPs, BOX_X + 20, BOX_Y + 11);
    char dsHeader[64];
    int activeDevs = (int)status.devices.size();
    if (activeDevs > 1) {
        snprintf(dsHeader, sizeof(dsHeader), "Multijugador PS5 (%d Mandos)", activeDevs);
    } else {
        snprintf(dsHeader, sizeof(dsHeader), "Telemetría PS5 DualSense");
    }
    draw_text(scene, dsHeader, fontCardTitle, BOX_X + 70, BOX_Y + 30, COLOR_WHITE);
    draw_text(scene, "Batería Exacta, MAC Guardada y Perfiles PS4", fontCardDesc, BOX_X + 70, BOX_Y + 50, COLOR_BLUE_BRIGHT);
    draw_rounded_rect(scene, BOX_X + 20, BOX_Y + 60, BOX_W - 40, 1, 0, COLOR_CARD_BORDER);

    int numToShow = (activeDevs > 0) ? activeDevs : 1;
    if (numToShow > 4) numToShow = 4;
    int curY = BOX_Y + 68;
    int mpY = BOX_Y + 318; // Reserved bottom 84px exclusively for Multiplayer card

    for (int d = 0; d < numToShow; d++) {
        int slotNum = d + 1;
        int batt = (status.connected && status.battery_pct > 0) ? status.battery_pct : 100;
        bool chg = status.battery_charging;
        int bstat = status.battery_status;
        std::string uname = status.user_name;
        std::string macStr = "";

        if (d < (int)status.devices.size()) {
            slotNum = (status.devices[d].player_slot > 0) ? status.devices[d].player_slot : (d + 1);
            batt = status.devices[d].battery_pct;
            chg = status.devices[d].battery_charging;
            bstat = status.devices[d].battery_status;
            uname = status.devices[d].user_name;
            macStr = status.devices[d].mac;
        }

        // Dynamic heights strictly guaranteed never to collide with mpY (274+68 = 342 .. 592)
        int cardH = (numToShow == 1) ? 114 : ((numToShow == 2) ? 112 : ((numToShow == 3) ? 74 : 55));
        int gap = (numToShow <= 2) ? 8 : 5;
        draw_rounded_rect(scene, BOX_X + 20, curY, BOX_W - 40, cardH, 9, Color{ 14, 24, 42 });
        draw_rounded_outline(scene, BOX_X + 20, curY, BOX_W - 40, cardH, 9, (chg || bstat == 1) ? Color{ 0, 240, 180 } : COLOR_BORDER_DIM, 1);

        draw_circle(scene, BOX_X + 36, curY + 18, 5, Color{ 0, 255, 140 });
        char pTitle[128];
        if (!uname.empty()) {
            snprintf(pTitle, sizeof(pTitle), "J%d • %s%s", slotNum, uname.c_str(),
                     (!macStr.empty() && numToShow <= 2) ? (std::string(" [") + macStr.substr(macStr.size() > 4 ? macStr.size() - 4 : 0) + "]").c_str() : "");
        } else {
            snprintf(pTitle, sizeof(pTitle), "Jugador %d • DualSense BT", slotNum);
        }
        draw_text(scene, pTitle, (numToShow >= 4) ? fontCardDesc : fontCardTitle, BOX_X + 50, curY + 24, COLOR_WHITE);

        if (numToShow <= 3) {
            char bLabel[64];
            Color bTextCol = COLOR_BLUE_BRIGHT;
            if (bstat == 1 || chg) {
                snprintf(bLabel, sizeof(bLabel), "%d%% [CARGANDO USB-C]", batt);
                bTextCol = Color{ 0, 255, 180 };
            } else if (bstat == 2) {
                snprintf(bLabel, sizeof(bLabel), "100%% [COMPLETA]");
                bTextCol = Color{ 0, 255, 140 };
            } else {
                snprintf(bLabel, sizeof(bLabel), "Batería: %d%%", batt);
                bTextCol = (batt <= 20) ? Color{ 255, 80, 80 } : COLOR_BLUE_BRIGHT;
            }
            draw_text(scene, bLabel, fontCardDesc, BOX_X + 36, curY + 44, bTextCol);
        } else {
            char bMini[32];
            snprintf(bMini, sizeof(bMini), "%d%%", batt);
            draw_text(scene, bMini, fontCardDesc, BOX_X + BOX_W - 82, curY + 24, Color{ 0, 255, 160 });
        }

        int barX = BOX_X + 36;
        int barY = (numToShow >= 4) ? (curY + 35) : (curY + 52);
        int barW = BOX_W - 72;
        int barH = (numToShow >= 4) ? 10 : 11;
        draw_rounded_rect(scene, barX, barY, barW, barH, 4, Color{ 10, 16, 28 });
        draw_rounded_outline(scene, barX, barY, barW, barH, 4, COLOR_BORDER_DIM, 1);

        int fillW = (barW * batt) / 100;
        Color battColor = (bstat == 1 || chg) ? Color{ 0, 240, 180 } :
                          ((batt <= 20) ? Color{ 255, 50, 50 } :
                           ((batt <= 50) ? Color{ 245, 190, 30 } : Color{ 0, 225, 120 }));
        if (fillW > 4) {
            draw_rounded_rect(scene, barX + 2, barY + 2, fillW - 4, barH - 4, 3, battColor);
        }

        if (numToShow == 1) {
            draw_text(scene, "Carga USB-C activa mientras juegas por Bluetooth.", fontCardDesc, BOX_X + 36, curY + 94, COLOR_TEXT_DIM);
        }

        curY += cardH + gap;
    }

    // Multiplayer & Profile Selector Action Card at bottom of Telemetry Box
    draw_rounded_rect(scene, BOX_X + 20, mpY, BOX_W - 40, 74, 10, Color{ 15, 32, 58 });
    draw_rounded_outline(scene, BOX_X + 20, mpY, BOX_W - 40, 74, 10, COLOR_BLUE_MAIN, 1);
    draw_image(scene, btnTriangle, BOX_X + 30, mpY + 7);
    draw_text(scene, "Multijugador Local y Perfil PS4", fontCardTitle, BOX_X + 74, mpY + 28, Color{ 0, 255, 160 });
    draw_text(scene, "Pulsa [TRIÁNGULO] para '¿Quién está usando este mando?'.", fontFooter, BOX_X + 30, mpY + 55, COLOR_WHITE);

    static const char* dsWhat[TOTAL_CARDS] = {
        "Configura el efecto luminoso del halo LED RGB que rodea el Touchpad del mando PS5 (control directo vía reporte BT 0x31).",
        "Selecciona el color RGB fijo empleado cuando el modo de iluminación está en Color Estático o Pulso Respiración.",
        "Regula la intensidad luminosa del halo del Touchpad para adaptarlo a habitaciones iluminadas o juego nocturno.",
        "Envía efectos mecánicos reales a los motores internos de los gatillos adaptativos L2 y R2 del DualSense.",
        "Controla los 5 puntos LED blancos situados bajo el Touchpad que muestran el número de jugador activo.",
        "Ajusta con Izquierda/Derecha (0% a 100%) la ganancia del altavoz interno del DualSense guardada en su perfil.",
        "Reasigna la pulsación del botón físico de micrófono (debajo del botón PS) en el DualSense.",
        "Elige entre el paquete Bluetooth estándar compacto (10B) o el reporte extendido completo DualSense (78B)."
    };
    int dsSel = (selectedCard >= 0 && selectedCard < TOTAL_CARDS) ? selectedCard : 0;
    std::string dsEff;
    if (dsSel == 0) {
        static const char* lbEff[6] = {
            "Sincronizar Juego: deja la barra en su color neutro de consola o perfil de jugador PS4.",
            "Color Estático: fija de forma permanente el color RGB elegido abajo en este mando (y se recuerda al volver a conectarlo).",
            "Estado Batería: la barra cambia de verde (>60%) a ámbar y rojo (<20%), o azul/verde pulsante al cargar por USB-C.",
            "Apagado: apaga por completo el halo LED RGB del DualSense para ahorrar batería y evitar reflejos.",
            "Arcoíris Dinámico: el halo del DualSense recorre suavemente todo el espectro de colores RGB en vivo.",
            "Pulso Respiración: enciende y atenúa de forma continua el color fijo que tengas seleccionado."
        };
        int mi = (cfg.ds_lightbar_mode >= 0 && cfg.ds_lightbar_mode < 6) ? cfg.ds_lightbar_mode : 0;
        dsEff = lbEff[mi];
    } else if (dsSel == 1) {
        dsEff = std::string("Color seleccionado: ") + cardValues[1] + " (al cambiar color activa Color Estático automáticamente y queda guardado).";
    } else if (dsSel == 2) {
        static const char* brEff[3] = {
            "Alto (100%): máxima intensidad del halo RGB y de las luces del mando.",
            "Medio (50%): reduce el brillo a la mitad para equilibrar visibilidad y consumo.",
            "Bajo (25%): luz suave discreta ideal para jugar a oscuras sin cansar la vista."
        };
        int bi = (cfg.ds_brightness >= 0 && cfg.ds_brightness < 3) ? cfg.ds_brightness : 0;
        dsEff = brEff[bi];
    } else if (dsSel == 3) {
        static const char* trEff[4] = {
            "Estándar / Off: gatillos libres sin resistencia mecánica añadida.",
            "Disparo Rápido (Click): añade un tope seco tipo ratón en L2/R2 para disparar rápido en FPS.",
            "Resistencia Continua: ofrece dureza progresiva constante en L2/R2, ideal para frenar o acelerar en conducción.",
            "Vibración / Pulso: hace vibrar mecánicamente los propios gatillos cada vez que los pulsas."
        };
        int ti = (cfg.ds_trigger_mode >= 0 && cfg.ds_trigger_mode < 4) ? cfg.ds_trigger_mode : 0;
        dsEff = trEff[ti];
    } else if (dsSel == 4) {
        static const char* plEff[3] = {
            "Ranura Jugador: ilumina el patrón de LEDs blancos acorde al Jugador 1, 2, 3 o 4 en consola.",
            "Todos Encendidos: mantiene activos los 5 LEDs blancos inferiores simultáneamente.",
            "Apagados: apaga todos los LEDs blancos indicadores bajo el Touchpad."
        };
        int pi = (cfg.ds_player_leds >= 0 && cfg.ds_player_leds < 3) ? cfg.ds_player_leds : 0;
        dsEff = plEff[pi];
    } else if (dsSel == 5) {
        char vBuf[192];
        snprintf(vBuf, sizeof(vBuf), "Volumen al %s (Bluetooth PS4 dirige el audio del juego a la TV; usa Izquierda/Derecha hasta 0%% para silenciar).",
                 cardValues[5].c_str());
        dsEff = vBuf;
    } else if (dsSel == 6) {
        dsEff = (cfg.ds_mute_action == 1)
            ? "Clic Touchpad: pulsar el botón físico de micrófono ejecuta al instante el Clic del Panel Táctil en PS4."
            : "Por Defecto: conserva la función estándar de silenciar o reactivar el micrófono del DualSense.";
    } else {
        dsEff = (cfg.ds_bt_mode == 1)
            ? "Modo Extendido (78B): desbloquea RGB completo, gatillos adaptativos, giroscopio, touchpad y batería real (recomendado)."
            : "Modo Seguro (10B): reporte básico de compatibilidad; no permite RGB personalizado ni gatillos adaptativos.";
    }
    draw_option_guide_box(scene, cardTitles[dsSel], dsWhat[dsSel], dsEff.c_str(), COLOR_BLUE_MAIN);

    // Footer (TV Overscan Safe)
    scene->DrawRectangle(SAFE_X, 844, SAFE_W, 2, COLOR_DIVIDER);
    draw_image(scene, btnPs, SAFE_X, 868);
    draw_text(scene, "Módulo Exclusivo PS5 DualSense", fontCardTitle, 154, 890, COLOR_WHITE);
    draw_text(scene, "Cada DualSense recuerda su configuración propia por hardware Bluetooth", fontFooter, 154, 916, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnL3, 935, 875);
    draw_text(scene, "Mando", fontFooter, 979, 900, COLOR_WHITE);

    draw_image(scene, btnTriangle, 1085, 875);
    draw_text(scene, "Elegir Perfil PS4", fontFooter, 1129, 900, Color{ 0, 255, 150 });

    draw_image(scene, btnArrows, 1320, 879);
    draw_text(scene, "Cambiar", fontFooter, 1372, 900, COLOR_TEXT_DIM);

    draw_image(scene, btnCross, 1480, 875);
    draw_text(scene, "Guardar", fontFooter, 1524, 900, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnOptions, 1635, 878);
    draw_text(scene, "Salir", fontFooter, 1712, 900, COLOR_TEXT_DIM);
}

void UI::render_dualshock4(Scene2D* scene, const AppConfig& cfg, int selectedCard, const OrbisPadData& padData, const TesterState& tester, const std::vector<ControllerTarget>* targets, int activeTargetIndex) {
    draw_target_selector_strip(scene, targets, activeTargetIndex);

    // Top Exclusive Banner for Native PS4 DualShock 4 (TV Overscan Safe: Y=196..264)
    draw_rounded_rect(scene, CARD_X, 196, CARD_W, 68, 12, Color{ 16, 22, 44 });
    draw_rounded_outline(scene, CARD_X, 196, CARD_W, 68, 12, COLOR_ACCENT_INDIGO, 1);
    draw_rounded_rect(scene, CARD_X, 196, 6, 68, 3, COLOR_ACCENT_INDIGO);
    draw_image(scene, iconRumble, CARD_X + 20, 209);
    draw_text(scene, "Configurador Nativo DualShock 4 (Mandos Originales PS4 CUH-ZCT1 / ZCT2)", fontCardTitle, CARD_X + 78, 223, COLOR_WHITE);
    draw_text(scene, "Cambia de mando con [L3] para guardar colores, brillo y vibración distintos en cada jugador PS4.", fontCardDesc, CARD_X + 78, 249, COLOR_BLUE_BRIGHT);

    const int TOTAL_CARDS = 6;
    const int VISIBLE_CARDS = 5;

    int scrollOffset = 0;
    if (selectedCard >= VISIBLE_CARDS) {
        scrollOffset = selectedCard - VISIBLE_CARDS + 1;
    }
    if (scrollOffset > TOTAL_CARDS - VISIBLE_CARDS) {
        scrollOffset = TOTAL_CARDS - VISIBLE_CARDS;
    }

    const char* cardTitles[TOTAL_CARDS] = {
        "Modo de Barra Luminosa (DS4)",
        "Color del LED DualShock 4",
        "Nivel de Brillo y Batería",
        "Potencia de Vibración DS4",
        "Respuesta del Panel Táctil",
        "Filtro Giroscopio Sixaxis"
    };

    const char* cardDescs[TOTAL_CARDS] = {
        "Color de juego, fijo, batería o apagado.",
        "Color RGB en vivo para el mando PS4.",
        "Reduce el brillo para ahorrar batería.",
        "Fuerza de los motores del DualShock 4.",
        "Sensibilidad táctil y clic del Touchpad.",
        "Estabiliza el sensor de movimiento PS4."
    };

    const ImageRGBA* cardIcons[TOTAL_CARDS] = {
        &iconMode,
        &iconGear,
        &iconGear,
        &iconRumble,
        &btnTouchpad,
        &iconDeadzone
    };

    std::string cardValues[TOTAL_CARDS];
    if (cfg.ds4_lightbar_mode >= 0 && cfg.ds4_lightbar_mode < (int)ConfigManager::DS4_LIGHTBAR_OPTIONS.size()) {
        cardValues[0] = ConfigManager::DS4_LIGHTBAR_OPTIONS[cfg.ds4_lightbar_mode];
    } else cardValues[0] = "Color Personalizado";

    if (cfg.ds4_lightbar_color >= 0 && cfg.ds4_lightbar_color < (int)ConfigManager::DS4_COLOR_OPTIONS.size()) {
        cardValues[1] = ConfigManager::DS4_COLOR_OPTIONS[cfg.ds4_lightbar_color];
    } else cardValues[1] = "Azul PS4";

    if (cfg.ds4_brightness >= 0 && cfg.ds4_brightness < (int)ConfigManager::DS4_BRIGHTNESS_OPTIONS.size()) {
        cardValues[2] = ConfigManager::DS4_BRIGHTNESS_OPTIONS[cfg.ds4_brightness];
    } else cardValues[2] = "Alto (100%)";

    if (cfg.ds4_rumble_boost >= 0 && cfg.ds4_rumble_boost < (int)ConfigManager::DS4_RUMBLE_BOOST_OPTIONS.size()) {
        cardValues[3] = ConfigManager::DS4_RUMBLE_BOOST_OPTIONS[cfg.ds4_rumble_boost];
    } else cardValues[3] = "Estándar (100%)";

    if (cfg.ds4_touchpad_mode >= 0 && cfg.ds4_touchpad_mode < (int)ConfigManager::DS4_TOUCHPAD_OPTIONS.size()) {
        cardValues[4] = ConfigManager::DS4_TOUCHPAD_OPTIONS[cfg.ds4_touchpad_mode];
    } else cardValues[4] = "Táctil + Clic Completo";

    if (cfg.ds4_gyro_deadband >= 0 && cfg.ds4_gyro_deadband < (int)ConfigManager::DS4_GYRO_OPTIONS.size()) {
        cardValues[5] = ConfigManager::DS4_GYRO_OPTIONS[cfg.ds4_gyro_deadband];
    } else cardValues[5] = "Alta Precisión";

    static const Color ds4Colors[7] = {
        Color{ 0, 112, 220 },   // 0: Azul PS4
        Color{ 239, 68, 68 },   // 1: Rojo Carmesí
        Color{ 16, 185, 129 },  // 2: Verde Esmeralda
        Color{ 168, 85, 247 },  // 3: Púrpura Neón
        Color{ 6, 182, 212 },   // 4: Cian Eléctrico
        Color{ 245, 158, 11 },  // 5: Ámbar Oro
        Color{ 248, 250, 252 }  // 6: Blanco Puro
    };

    const int DS4_CARD_X = CARD_X;
    const int DS4_CARD_W = 868;
    const int DS4_CARD_H = 74;
    const int DS4_CARD_SPACING = 8;

    for (int i = 0; i < VISIBLE_CARDS; i++) {
        int cardIdx = i + scrollOffset;
        if (cardIdx >= TOTAL_CARDS) break;

        int cardY = CARD_Y_START + i * (DS4_CARD_H + DS4_CARD_SPACING);
        bool isSelected = (cardIdx == selectedCard);

        draw_rounded_rect(scene, DS4_CARD_X, cardY, DS4_CARD_W, DS4_CARD_H, 12, isSelected ? Color{ 18, 28, 56 } : COLOR_CARD_BG);

        if (isSelected) {
            draw_rounded_outline(scene, DS4_CARD_X - 1, cardY - 1, DS4_CARD_W + 2, DS4_CARD_H + 2, 12, COLOR_ACCENT_INDIGO, 2);
            draw_rounded_outline(scene, DS4_CARD_X, cardY, DS4_CARD_W, DS4_CARD_H, 12, COLOR_BLUE_BRIGHT, 2);
            draw_rounded_rect(scene, DS4_CARD_X, cardY + 12, 5, DS4_CARD_H - 24, 2, COLOR_BLUE_BRIGHT);
        } else {
            draw_rounded_outline(scene, DS4_CARD_X, cardY, DS4_CARD_W, DS4_CARD_H, 12, COLOR_CARD_BORDER, 1);
        }

        if (cardIcons[cardIdx]) {
            draw_image(scene, *cardIcons[cardIdx], DS4_CARD_X + 18, cardY + 14);
        }

        draw_text(scene, cardTitles[cardIdx], fontCardTitle, DS4_CARD_X + 76, cardY + 31, COLOR_WHITE);
        draw_text(scene, cardDescs[cardIdx], fontCardDesc, DS4_CARD_X + 76, cardY + 57, COLOR_TEXT_DIM);

        int vBoxW = 284;
        int vBoxH = 42;
        int vBoxX = DS4_CARD_X + DS4_CARD_W - vBoxW - 16;
        int vBoxY = cardY + (DS4_CARD_H - vBoxH) / 2;
        draw_rounded_rect(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? Color{ 10, 22, 42 } : Color{ 8, 13, 24 });
        draw_rounded_outline(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM, 1);

        if (isSelected) {
            draw_text(scene, "<", fontValue, vBoxX + 12, cardY + 45, COLOR_BLUE_BRIGHT);
            draw_text(scene, ">", fontValue, vBoxX + vBoxW - 24, cardY + 45, COLOR_BLUE_BRIGHT);
        }

        if (cardIdx == 1) {
            int c_idx = cfg.ds4_lightbar_color;
            if (c_idx >= 0 && c_idx < 7) {
                draw_circle(scene, vBoxX + 34, cardY + 37, 8, ds4Colors[c_idx]);
                draw_circle_outline(scene, vBoxX + 34, cardY + 37, 8, COLOR_WHITE, 1);
            }
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2 + 8, cardY + 45, COLOR_WHITE);
        } else {
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2, cardY + 45, COLOR_WHITE);
        }
    }

    if (scrollOffset > 0) {
        draw_text(scene, "^", fontCardTitle, DS4_CARD_X + DS4_CARD_W / 2, 272, COLOR_BLUE_BRIGHT);
    }
    if (scrollOffset + VISIBLE_CARDS < TOTAL_CARDS) {
        draw_text(scene, "v", fontCardTitle, DS4_CARD_X + DS4_CARD_W / 2, 700, COLOR_BLUE_BRIGHT);
    }

    // Right-Hand Native DualShock 4 Live Preview & Status Panel (TV Overscan Safe: Y=274..676, H=402)
    const int BOX_X = 1298;
    const int BOX_Y = 274;
    const int BOX_W = 526;
    const int BOX_H = 402;

    draw_rounded_rect(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BORDER, 1);

    draw_image(scene, btnPs, BOX_X + 20, BOX_Y + 11);
    draw_text(scene, "Estado DualShock 4 Nativo", fontCardTitle, BOX_X + 70, BOX_Y + 30, COLOR_WHITE);
    draw_text(scene, "Control Persistente vía Demonio + libScePad", fontCardDesc, BOX_X + 70, BOX_Y + 50, COLOR_BLUE_BRIGHT);
    draw_rounded_rect(scene, BOX_X + 20, BOX_Y + 60, BOX_W - 40, 1, 0, COLOR_CARD_BORDER);

    int activeColorIdx = (cfg.ds4_lightbar_color >= 0 && cfg.ds4_lightbar_color < 7) ? cfg.ds4_lightbar_color : 0;
    Color activeLed = ds4Colors[activeColorIdx];
    static uint32_t s_preview_tick = 0;
    s_preview_tick++;
    if (cfg.ds4_lightbar_mode == 3) {
        activeLed = Color{ 25, 30, 40 };
    } else if (cfg.ds4_lightbar_mode == 2) {
        activeLed = Color{ 16, 185, 129 };
    } else if (cfg.ds4_lightbar_mode == 4) {
        uint32_t ph = (s_preview_tick * 6U) % 1536U;
        uint32_t reg = ph / 256U;
        uint32_t rem = ph % 256U;
        switch (reg) {
            case 0:  activeLed = Color{ 255, (uint8_t)rem, 0 }; break;
            case 1:  activeLed = Color{ (uint8_t)(255 - rem), 255, 0 }; break;
            case 2:  activeLed = Color{ 0, 255, (uint8_t)rem }; break;
            case 3:  activeLed = Color{ 0, (uint8_t)(255 - rem), 255 }; break;
            case 4:  activeLed = Color{ (uint8_t)rem, 0, 255 }; break;
            default: activeLed = Color{ 255, 0, (uint8_t)(255 - rem) }; break;
        }
    } else if (cfg.ds4_lightbar_mode == 5) {
        uint32_t ph = (s_preview_tick * 4U) % 512U;
        uint32_t tri = (ph < 256U) ? ph : (511U - ph);
        uint32_t sc = 15U + (tri * 85U) / 255U;
        activeLed = Color{
            (uint8_t)((ds4Colors[activeColorIdx].r * sc) / 100U),
            (uint8_t)((ds4Colors[activeColorIdx].g * sc) / 100U),
            (uint8_t)((ds4Colors[activeColorIdx].b * sc) / 100U)
        };
    }

    if (cfg.ds4_lightbar_mode != 3) {
        if (cfg.ds4_brightness == 1) {
            activeLed.r = (uint8_t)((activeLed.r * 50) / 100);
            activeLed.g = (uint8_t)((activeLed.g * 50) / 100);
            activeLed.b = (uint8_t)((activeLed.b * 50) / 100);
        } else if (cfg.ds4_brightness == 2) {
            activeLed.r = (uint8_t)((activeLed.r * 15) / 100);
            activeLed.g = (uint8_t)((activeLed.g * 15) / 100);
            activeLed.b = (uint8_t)((activeLed.b * 15) / 100);
        }
    }

    draw_text(scene, "Vista Previa Barra de Luz Frontal:", fontCardDesc, BOX_X + 24, BOX_Y + 84, COLOR_WHITE);
    draw_rounded_rect(scene, BOX_X + 24, BOX_Y + 93, BOX_W - 48, 22, 7, activeLed);
    draw_rounded_outline(scene, BOX_X + 24, BOX_Y + 93, BOX_W - 48, 22, 7, COLOR_WHITE, 1);
    if (cfg.ds4_lightbar_mode == 3) {
        draw_text_centered(scene, "LED APAGADO (0% Brillo - Ahorro Total)", fontFooter, BOX_X + BOX_W / 2, BOX_Y + 110, COLOR_TEXT_DIM);
    }

    draw_text(scene, "Mandos Activos por Ranura PS4:", fontCardDesc, BOX_X + 24, BOX_Y + 138, COLOR_WHITE);
    for (int s = 0; s < 4; s++) {
        int sy = BOX_Y + 148 + s * 30;
        bool conn = tester.slotConnected[s];
        draw_rounded_rect(scene, BOX_X + 24, sy, BOX_W - 48, 24, 6, conn ? Color{ 14, 32, 54 } : Color{ 10, 16, 26 });
        draw_circle(scene, BOX_X + 42, sy + 12, 5, conn ? Color{ 0, 240, 140 } : Color{ 90, 100, 115 });
        char slotTxt[96];
        snprintf(slotTxt, sizeof(slotTxt), "Jugador %d (Ranura %d): %s", s + 1, s + 1, conn ? "Mando Activo" : "Sin conexión");
        draw_text(scene, slotTxt, fontFooter, BOX_X + 58, sy + 18, conn ? COLOR_WHITE : COLOR_TEXT_DIM);
    }

    draw_rounded_rect(scene, BOX_X + 20, BOX_Y + 278, BOX_W - 40, 1, 0, COLOR_CARD_BORDER);
    draw_text(scene, "Lectura Instantánea:", fontCardDesc, BOX_X + 24, BOX_Y + 302, COLOR_WHITE);
    char stickInfo[160];
    snprintf(stickInfo, sizeof(stickInfo), "L3:(%d,%d) R3:(%d,%d) | L2:%d R2:%d",
             (int)padData.leftStick.x, (int)padData.leftStick.y,
             (int)padData.rightStick.x, (int)padData.rightStick.y,
             (int)padData.analogButtons.l2, (int)padData.analogButtons.r2);
    draw_text(scene, stickInfo, fontFooter, BOX_X + 24, BOX_Y + 328, COLOR_BLUE_BRIGHT);

    bool tpClick = (padData.buttons & ORBIS_PAD_BUTTON_TOUCH_PAD) != 0;
    bool anyDpad = (padData.buttons & (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN | ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT)) != 0;
    char tpInfo[160];
    snprintf(tpInfo, sizeof(tpInfo), "Táctil: %s (%d dedos) | Cruceta: %s",
             tpClick ? "CLIC ACTIVO" : "Reposo",
             (int)padData.touch.fingers,
             anyDpad ? "PULSADA" : "Reposo");
    draw_text(scene, tpInfo, fontFooter, BOX_X + 24, BOX_Y + 354, (tpClick || anyDpad) ? Color{ 0, 255, 160 } : COLOR_WHITE);
    draw_text(scene, "Pulsa [CUADRADO] para probar la vibración en este mando.", fontFooter, BOX_X + 24, BOX_Y + 382, Color{ 0, 255, 160 });

    static const char* ds4What[TOTAL_CARDS] = {
        "Define cómo controla el demonio la barra de luz frontal del DualShock 4 tanto en la consola como en juegos.",
        "Elige el tono RGB específico que mostrará la barra luminosa cuando uses Color Personalizado o Respiración.",
        "Ajusta la potencia de brillo por hardware del LED del DualShock 4 para jugar de noche y ahorrar batería.",
        "Regula la intensidad de los motores de vibración izquierdo/derecho en juegos y pruebas de hardware.",
        "Configura cómo responde el panel táctil central del DualShock 4: deslizamientos táctiles o solo pulsación física.",
        "Suaviza las micro-vibraciones del sensor de movimiento Sixaxis (giroscopio y acelerómetro)."
    };
    int ds4Sel = (selectedCard >= 0 && selectedCard < TOTAL_CARDS) ? selectedCard : 0;
    std::string ds4Eff;
    if (ds4Sel == 0) {
        static const char* mEff[6] = {
            "Control del Juego: el sistema PS4 y cada juego deciden libremente el color del LED (azul P1, rojo P2, etc.).",
            "Color Personalizado: el demonio fija tu color elegido abajo y lo mantiene activo al salir al menú o jugar.",
            "Nivel de Batería: la barra muestra verde (>60%), ámbar (21-60%) o rojo (<20%) según la carga restante.",
            "Apagado (Ahorro): apaga el LED frontal (0% brillo) para eliminar reflejos en el televisor y ahorrar batería.",
            "Arcoíris Dinámico: cambia progresivamente por todo el espectro RGB mientras usas la consola.",
            "Pulso Respiración: enciende y atenúa de forma suave y constante el color elegido abajo."
        };
        int mi = (cfg.ds4_lightbar_mode >= 0 && cfg.ds4_lightbar_mode < 6) ? cfg.ds4_lightbar_mode : 0;
        ds4Eff = mEff[mi];
    } else if (ds4Sel == 1) {
        ds4Eff = std::string("Color seleccionado: ") + cardValues[1] + " (el demonio mantiene este color fijo en segundo plano para esta ranura).";
    } else if (ds4Sel == 2) {
        static const char* bEff[3] = {
            "Alto (100%): brillo completo estándar de Sony en la barra LED del DualShock 4.",
            "Medio (50%): atenúa la intensidad por hardware a la mitad para mayor comodidad visual.",
            "Tenue (15%): brillo mínimo discreto ideal en habitaciones oscuras y máximo ahorro energético."
        };
        int bi = (cfg.ds4_brightness >= 0 && cfg.ds4_brightness < 3) ? cfg.ds4_brightness : 0;
        ds4Eff = bEff[bi];
    } else if (ds4Sel == 3) {
        static const char* rEff[4] = {
            "Estándar (100%): fuerza de vibración original sin recortes ni amplificación.",
            "Suave (60%): reduce la potencia de los motores al 60% para vibración más discreta y ligera.",
            "Intenso (120%): potencia los golpes suaves hasta un 120% para sentir cada impacto con más fuerza.",
            "Desactivado: apaga por completo los motores de vibración del DualShock 4 (pulsa [Cuadrado] para probar)."
        };
        int ri = (cfg.ds4_rumble_boost >= 0 && cfg.ds4_rumble_boost < 4) ? cfg.ds4_rumble_boost : 0;
        ds4Eff = rEff[ri];
    } else if (ds4Sel == 4) {
        ds4Eff = (cfg.ds4_touchpad_mode == 1)
            ? "Solo Clic Físico: ignora rozamientos capacitivos y únicamente registra al presionar clic del panel."
            : "Táctil + Clic Completo: permite deslizamientos, gestos multi-dedo y pulsación del panel táctil.";
    } else {
        static const char* gEff[3] = {
            "Alta Precisión: lectura directa sin filtrado del giroscopio para apuntado por movimiento.",
            "Filtro Estabilizador: suaviza pequeñas vibraciones de mano en menús o cámara por movimiento.",
            "Filtro Intenso: recorta oscilaciones de sensor para estabilizar mandos con giroscopio nervioso."
        };
        int gi = (cfg.ds4_gyro_deadband >= 0 && cfg.ds4_gyro_deadband < 3) ? cfg.ds4_gyro_deadband : 0;
        ds4Eff = gEff[gi];
    }
    draw_option_guide_box(scene, cardTitles[ds4Sel], ds4What[ds4Sel], ds4Eff.c_str(), COLOR_ACCENT_INDIGO);

    // Footer (TV Overscan Safe)
    scene->DrawRectangle(SAFE_X, 844, SAFE_W, 2, COLOR_DIVIDER);
    draw_image(scene, iconRumble, SAFE_X, 868);
    draw_text(scene, "Módulo Nativo DualShock 4 (PS4)", fontCardTitle, 154, 890, COLOR_WHITE);
    draw_text(scene, "Cada jugador DualShock 4 conserva su configuración propia por ranura", fontFooter, 154, 916, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnL3, 935, 875);
    draw_text(scene, "Mando", fontFooter, 979, 900, COLOR_WHITE);

    draw_image(scene, btnSquare, 1085, 875);
    draw_text(scene, "Probar Vibración", fontFooter, 1129, 900, Color{ 0, 255, 150 });

    draw_image(scene, btnArrows, 1320, 879);
    draw_text(scene, "Ajustar", fontFooter, 1372, 900, COLOR_TEXT_DIM);

    draw_image(scene, btnCross, 1480, 875);
    draw_text(scene, "Guardar", fontFooter, 1524, 900, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnOptions, 1635, 878);
    draw_text(scene, "Salir", fontFooter, 1712, 900, COLOR_TEXT_DIM);
}
