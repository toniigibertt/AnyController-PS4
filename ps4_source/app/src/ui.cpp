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

// Layout Constants
static const int SIDEBAR_X = 56;
static const int SIDEBAR_W = 320;
static const int CARD_X = 410;
static const int CARD_W = 1140;
static const int CARD_H = 92;
static const int CARD_Y_START = 280;
static const int CARD_Y_STEP = 104;

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

void UI::render(Scene2D* scene, const AppConfig& cfg, int currentTabId, int selectedCard,
                int toastFrames, const OrbisPadData& padData, const TesterState& tester,
                const char* toastMsg,
                bool hidConnected, const HidStatus* hidStatus,
                const HidProfile* hidProfile, int remapSelectedRow,
                bool remapTestMode,
                const PkgUpdater* updater,
                bool dualsenseConnected, int dsSelectedCard,
                int ds4SelectedCard, bool dualshock4Connected) {
    if (!scene) return;

    // 1. Blit native background + Animated PlayStation XMB Wave & Bokeh Sparkles
    psBg.update(0.01666f);
    psBg.render(scene, cached_bg);

    // 2. Original AnyController Studio Top Header Bar (Clean Glass Studio Deck)
    draw_rounded_rect(scene, 56, 48, 1808, 76, 12, Color{ 9, 14, 26 });
    draw_rounded_outline(scene, 56, 48, 1808, 76, 12, COLOR_CARD_BORDER, 1);
    // Left Cyan Accent Bar inside Header
    draw_rounded_rect(scene, 56, 48, 6, 76, 3, COLOR_BLUE_MAIN);

    // Studio Geometric Emblem
    draw_rounded_rect(scene, 82, 62, 48, 48, 10, Color{ 15, 34, 64 });
    draw_rounded_outline(scene, 82, 62, 48, 48, 10, COLOR_BLUE_MAIN, 2);
    draw_image(scene, btnPs, 90, 70);

    int titleW = scene->GetTextWidth("CONTROLLER CONTROL CENTER ", fontCardTitle);
    draw_text(scene, "CONTROLLER CONTROL CENTER ", fontCardTitle, 148, 82, COLOR_WHITE);
    draw_text(scene, "PS4", fontCardTitle, 148 + titleW, 82, COLOR_BLUE_BRIGHT);
    draw_text(scene, "Motor ANYCONTROLLER • DualSense PS5, DualShock 4 Nativo y Periféricos HID", fontCardDesc, 148, 108, COLOR_TEXT_DIM);

    // Right status indicator in top bar showing active controller type
    const char* activeCtrlLabel = dualsenseConnected ? "MANDO: PS5 DUALSENSE" : "MANDO: PS4 DUALSHOCK 4";
    draw_rounded_rect(scene, 1570, 66, 272, 40, 8, Color{ 12, 28, 48 });
    draw_rounded_outline(scene, 1570, 66, 272, 40, 8, COLOR_BLUE_GLOW, 1);
    draw_circle(scene, 1590, 86, 5, Color{ 0, 240, 140 });
    draw_text(scene, activeCtrlLabel, fontCardDesc, 1606, 92, COLOR_WHITE);

    // Header Persistent Update Badge (visible across all tabs if an update is available)
    if (updater && updater->get_state() == UPDATER_UPDATE_AVAILABLE) {
        int bX = 1280, bY = 66, bW = 275, bH = 40;
        draw_rounded_rect(scene, bX, bY, bW, bH, 8, Color{ 10, 42, 28 });
        draw_rounded_outline(scene, bX, bY, bW, bH, 8, Color{ 0, 255, 150 }, 2);
        std::string badgeStr = "NUEVA: " + updater->get_latest_version();
        draw_text(scene, badgeStr, fontCardDesc, bX + 18, bY + 26, Color{ 0, 255, 150 });
    }

    // 3. Left Navigation Dock (shows both DualSense PS5 and DualShock 4, ordering active controller first)
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
    sidebarTabs.push_back({ TAB_TESTER, "Laboratorio", "Probador y calibración", &iconMode });
    if (hidConnected) {
        sidebarTabs.push_back({ TAB_REMAP_HID, "Mapeo USB HID", "Perfiles a medida", &iconRemap });
    }
    sidebarTabs.push_back({ TAB_INFO, "Sistema y Red", "Actualizador GitHub", &iconInfo });

    // Navigation Dock Container Header
    draw_rounded_rect(scene, SIDEBAR_X, 145, SIDEBAR_W, 56, 10, Color{ 12, 20, 36 });
    draw_rounded_outline(scene, SIDEBAR_X, 145, SIDEBAR_W, 56, 10, COLOR_CARD_BORDER, 1);
    draw_image(scene, btnL1, SIDEBAR_X + 16, 157);
    draw_text_centered(scene, "MÓDULOS", fontCardDesc, SIDEBAR_X + SIDEBAR_W / 2, 180, COLOR_BLUE_BRIGHT);
    draw_image(scene, btnR1, SIDEBAR_X + SIDEBAR_W - 58, 157);

    int totalSidebarTabs = (int)sidebarTabs.size();
    for (int i = 0; i < totalSidebarTabs; i++) {
        int tabY = 216 + i * 86;
        bool isCurrent = (sidebarTabs[i].tabId == currentTabId);
        if (isCurrent) {
            draw_rounded_rect(scene, SIDEBAR_X, tabY, SIDEBAR_W, 74, 10, COLOR_SIDEBAR_BLUE);
            draw_rounded_outline(scene, SIDEBAR_X, tabY, SIDEBAR_W, 74, 10, COLOR_BLUE_MAIN, 2);
            // Active vertical bar indicator
            draw_rounded_rect(scene, SIDEBAR_X, tabY + 10, 5, 54, 2, COLOR_BLUE_BRIGHT);
            if (sidebarTabs[i].icon) draw_image(scene, *sidebarTabs[i].icon, SIDEBAR_X + 18, tabY + 15);
            draw_text(scene, sidebarTabs[i].label, fontCardTitle, SIDEBAR_X + 74, tabY + 34, COLOR_WHITE);
            draw_text(scene, sidebarTabs[i].sublabel, fontCardDesc, SIDEBAR_X + 74, tabY + 58, COLOR_BLUE_BRIGHT);
        } else {
            draw_rounded_rect(scene, SIDEBAR_X, tabY, SIDEBAR_W, 74, 10, COLOR_SIDEBAR_DARK);
            draw_rounded_outline(scene, SIDEBAR_X, tabY, SIDEBAR_W, 74, 10, COLOR_BORDER_DIM, 1);
            if (sidebarTabs[i].icon) draw_image(scene, *sidebarTabs[i].icon, SIDEBAR_X + 18, tabY + 15);
            draw_text(scene, sidebarTabs[i].label, fontCardTitle, SIDEBAR_X + 74, tabY + 34, Color{ 203, 213, 225 });
            draw_text(scene, sidebarTabs[i].sublabel, fontCardDesc, SIDEBAR_X + 74, tabY + 58, COLOR_TEXT_DIM);
        }

        if (sidebarTabs[i].tabId == TAB_INFO && updater && updater->get_state() == UPDATER_UPDATE_AVAILABLE) {
            draw_circle(scene, SIDEBAR_X + SIDEBAR_W - 22, tabY + 37, 6, Color{ 0, 255, 150 });
        }
    }

    // 4. Main Content Area
    if (currentTabId == TAB_SETTINGS) {
        // Section Banner for Global Settings
        draw_rounded_rect(scene, CARD_X, 145, CARD_W + 300, 110, 12, Color{ 13, 23, 42 });
        draw_rounded_outline(scene, CARD_X, 145, CARD_W + 300, 110, 12, COLOR_CARD_BORDER, 1);
        draw_rounded_rect(scene, CARD_X, 145, 6, 110, 3, COLOR_BLUE_MAIN);
        draw_image(scene, iconGear, CARD_X + 26, 175);
        draw_text(scene, "Motor Pro de Respuesta, Curvas de Joysticks y Turbo", fontCardTitle, CARD_X + 88, 190, COLOR_WHITE);
        draw_text(scene, "Perfiles rápidos eSports/SimRacing/Eco, curvas analógicas y fuego rápido en tiempo real (/data/anycontroller/anycontroller.ini).", fontCardDesc, CARD_X + 88, 222, COLOR_TEXT_DIM);

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

        int fullCardW = CARD_W + 300; // 1440px wide studio cards
        int selectorCenterX = CARD_X + fullCardW - 210;

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

            draw_image(scene, cardIcons[cardIndex], CARD_X + 24, cardY + 22);

            draw_text(scene, cardTitles[cardIndex], fontCardTitle, CARD_X + 88, cardY + 38, COLOR_WHITE);
            draw_text(scene, cardDescs[cardIndex],  fontCardDesc,  CARD_X + 88, cardY + 68, COLOR_TEXT_DIM);

            // Sleek Value Selector Box on the right
            int boxW = 310;
            int boxH = 48;
            int boxX = selectorCenterX - boxW / 2;
            int boxY = cardY + (CARD_H - boxH) / 2;
            draw_rounded_rect(scene, boxX, boxY, boxW, boxH, 8, isSelected ? Color{ 10, 22, 42 } : Color{ 8, 13, 24 });
            draw_rounded_outline(scene, boxX, boxY, boxW, boxH, 8, isSelected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM, 1);

            Color textColor  = isSelected ? COLOR_BLUE_BRIGHT : COLOR_WHITE;
            Color arrowColor = isSelected ? COLOR_WHITE : COLOR_TEXT_DIM;

            draw_text_centered(scene, vals[cardIndex], fontValue, selectorCenterX, cardY + 54, textColor);
            draw_text(scene, "<", fontValue, boxX + 16, cardY + 54, arrowColor);
            draw_text(scene, ">", fontValue, boxX + boxW - 28, cardY + 54, arrowColor);
        }

        if (scrollOffset > 0) {
            draw_text(scene, "^", fontCardTitle, CARD_X + fullCardW / 2, 275, COLOR_BLUE_BRIGHT);
        }
        if (scrollOffset + VISIBLE_CARDS < TOTAL_CARDS) {
            draw_text(scene, "v", fontCardTitle, CARD_X + fullCardW / 2, CARD_Y_START + VISIBLE_CARDS * CARD_Y_STEP - 2, COLOR_BLUE_BRIGHT);
        }

        // 5. Footer for Settings
        scene->DrawRectangle(56, 865, 1808, 2, COLOR_DIVIDER);

        draw_image(scene, iconSave, 56, 890);
        draw_text(scene, "Controller Control Center PS4 (Motor ANYCONTROLLER)", fontCardTitle, 116, 912, COLOR_WHITE);
        draw_text(scene, "Sincronización instantánea con /data/anycontroller/anycontroller.ini", fontFooter, 116, 938, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnTriangle, 1160, 897);
        draw_text(scene, "Preset Pro", fontFooter, 1205, 922, Color{ 0, 255, 150 });

        draw_image(scene, btnArrows, 1370, 901);
        draw_text(scene, "Modificar", fontFooter, 1424, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnCross, 1540, 897);
        draw_text(scene, "Aplicar", fontFooter, 1585, 922, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnOptions, 1690, 900);
        draw_text(scene, "Salir", fontFooter, 1770, 922, COLOR_TEXT_DIM);

    } else if (currentTabId == TAB_TESTER) {
        // --- INPUT TESTER & CALIBRATION VIEW ---
        render_tester(scene, cfg, padData, tester);

    } else if (currentTabId == TAB_DUALSENSE && hidStatus) {
        // --- DUALSENSE PS5 EXCLUSIVE MANAGEMENT VIEW ---
        render_dualsense(scene, cfg, dsSelectedCard, *hidStatus);

    } else if (currentTabId == TAB_DUALSHOCK4) {
        // --- DUALSHOCK 4 NATIVE PS4 CONTROLLER VIEW ---
        render_dualshock4(scene, cfg, ds4SelectedCard, padData, tester);

    } else if (currentTabId == TAB_REMAP_HID && hidConnected && hidStatus && hidProfile) {
        // --- REMAP HID VIEW ---
        render_remap(scene, *hidStatus, *hidProfile, remapSelectedRow, padData, remapTestMode);

    } else {
        // --- INFO VIEW ---
        // Info Card 1: Overview & Version
        draw_rounded_rect(scene, CARD_X, 280, CARD_W, 140, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, 280, CARD_W, 140, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconInfo, CARD_X + 24, 298);
        draw_text(scene, "Controller Control Center PS4 (Title ID: TONI88888)", fontCardTitle, CARD_X + 90, 312, COLOR_WHITE);
        draw_text(scene, "Versión: " ONEFAMILY_VERSION_SFO " / " ONEFAMILY_VERSION_STRING " (Edición Pro Multijugador en Español)", fontCardDesc, CARD_X + 90, 342, COLOR_BLUE_BRIGHT);
        draw_text(scene, "Payload Demonio: ANYCONTROLLER (/data/payloads/anycontroller.bin)", fontCardDesc, CARD_X + 90, 372, COLOR_TEXT_DIM);
        draw_text(scene, "Configuración: /data/anycontroller/anycontroller.ini", fontCardDesc, CARD_X + 90, 400, COLOR_TEXT_DIM);

        // Info Card 2: Online Update Status Card
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

        draw_rounded_rect(scene, CARD_X, 436, CARD_W, 130, 12, updateBg);
        draw_rounded_outline(scene, CARD_X, 436, CARD_W, 130, 12, updateBorder, (upState == UPDATER_UPDATE_AVAILABLE || upState == UPDATER_DOWNLOAD_SUCCESS) ? 2 : 1);
        draw_image(scene, iconSave, CARD_X + 24, 454);

        if (upState == UPDATER_UPDATE_AVAILABLE) {
            std::string titleStr = "Nueva Actualizacion Disponible: " + updater->get_latest_version();
            draw_text(scene, titleStr, fontCardTitle, CARD_X + 90, 468, Color{ 0, 255, 150 });
            draw_text(scene, "Pulsa [TRIANGULO] para descargar el paquete a /data/pkg/", fontCardDesc, CARD_X + 90, 498, COLOR_WHITE);
            draw_text(scene, "Tamano: ~8.5 MB - Listo para instalar en GoldHEN Package Installer", fontCardDesc, CARD_X + 90, 526, COLOR_TEXT_DIM);
        } else if (upState == UPDATER_DOWNLOADING) {
            UpdaterProgress prog = updater->get_progress();
            char dlTitle[128];
            snprintf(dlTitle, sizeof(dlTitle), "Descargando Actualizacion: %d%% (%llu MB / %llu MB)",
                     prog.percent,
                     (unsigned long long)(prog.bytesReceived / (1024 * 1024)),
                     (unsigned long long)(prog.totalBytes / (1024 * 1024)));
            draw_text(scene, dlTitle, fontCardTitle, CARD_X + 90, 468, COLOR_BLUE_BRIGHT);

            // Progress bar
            int barX = CARD_X + 90;
            int barY = 486;
            int barW = 1200;
            int barH = 14;
            draw_rounded_rect(scene, barX, barY, barW, barH, 4, Color{ 20, 32, 50 });
            int fillW = (barW * prog.percent) / 100;
            if (fillW > 0) {
                draw_rounded_rect(scene, barX, barY, fillW, barH, 4, COLOR_BLUE_BRIGHT);
            }
            std::string dlPathMsg = "Guardando en " + (updater ? updater->get_target_pkg_path() : std::string("/data/pkg/")) + "...";
            draw_text(scene, dlPathMsg, fontCardDesc, CARD_X + 90, 526, COLOR_TEXT_DIM);
        } else if (upState == UPDATER_DOWNLOAD_SUCCESS) {
            draw_text(scene, "Descarga Completada! Paquete Guardado", fontCardTitle, CARD_X + 90, 468, Color{ 0, 255, 150 });
            std::string savedMsg = "Paquete guardado en: " + (updater ? updater->get_target_pkg_path() : std::string("/data/pkg/"));
            draw_text(scene, savedMsg, fontCardDesc, CARD_X + 90, 498, COLOR_WHITE);
            draw_text(scene, "Abre Ajustes PS4 -> GoldHEN -> Package Installer y pulsa (X) para instalar", fontCardDesc, CARD_X + 90, 526, Color{ 0, 255, 150 });
        } else if (upState == UPDATER_UP_TO_DATE) {
            draw_text(scene, "Controller Control Center PS4 esta al Dia", fontCardTitle, CARD_X + 90, 468, COLOR_WHITE);
            draw_text(scene, "Tienes la version mas reciente instalada y verificada.", fontCardDesc, CARD_X + 90, 498, COLOR_BLUE_BRIGHT);
            draw_text(scene, "Actualizaciones automaticas directas desde GitHub Releases", fontCardDesc, CARD_X + 90, 526, COLOR_TEXT_DIM);
        } else if (upState == UPDATER_CHECKING) {
            draw_text(scene, "Buscando Actualizaciones en GitHub...", fontCardTitle, CARD_X + 90, 468, COLOR_WHITE);
            draw_text(scene, "Conectando con GitHub Releases...", fontCardDesc, CARD_X + 90, 498, COLOR_TEXT_DIM);
            draw_text(scene, "Verificando nuevos paquetes PKG en GitHub...", fontCardDesc, CARD_X + 90, 526, COLOR_TEXT_DIM);
        } else {
            draw_text(scene, "Servicio de Actualizacion en Linea (GitHub)", fontCardTitle, CARD_X + 90, 468, COLOR_WHITE);
            draw_text(scene, updater ? updater->get_status_message() : "Modo sin conexion (red local)", fontCardDesc, CARD_X + 90, 498, COLOR_TEXT_DIM);
            draw_text(scene, "GitHub: https://github.com/toniigibertt/AnyController-PS4", fontCardDesc, CARD_X + 90, 526, COLOR_TEXT_DIM);
        }

        // Info Card 3: Supported Controllers & Connectivity
        draw_rounded_rect(scene, CARD_X, 582, CARD_W, 120, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, 582, CARD_W, 120, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconMode, CARD_X + 24, 600);
        draw_text(scene, "Multijugador Local (1-4 Mandos) y Carga USB-C", fontCardTitle, CARD_X + 90, 614, COLOR_WHITE);
        draw_text(scene, "- Al encender un 2º DualSense se abre automáticamente el selector de perfil de PS4.", fontCardDesc, CARD_X + 90, 642, COLOR_TEXT_DIM);
        draw_text(scene, "- Carga USB-C a máxima potencia mientras juegas por Bluetooth con telemetría exacta.", fontCardDesc, CARD_X + 90, 670, COLOR_TEXT_DIM);

        // Info Card 4: Modes & Shortcuts
        draw_rounded_rect(scene, CARD_X, 718, CARD_W, 126, 12, COLOR_CARD_BG);
        draw_rounded_outline(scene, CARD_X, 718, CARD_W, 126, 12, COLOR_CARD_BORDER, 1);
        draw_image(scene, iconGuide, CARD_X + 24, 736);
        draw_text(scene, "Atajos del Mando Virtual e Integración en PS4", fontCardTitle, CARD_X + 90, 750, COLOR_WHITE);
        draw_text(scene, "- Create + Izquierda (o [TRIÁNGULO] en pestaña DualSense): Abrir Selector de Perfil PS4.", fontCardDesc, CARD_X + 90, 778, COLOR_BLUE_BRIGHT);
        draw_text(scene, "- Create + Options o Arriba = Clic Touchpad | Create + Abajo = Botón PS nativo.", fontCardDesc, CARD_X + 90, 806, COLOR_TEXT_DIM);

        // 5. Footer for Info
        scene->DrawRectangle(60, 865, 1800, 2, COLOR_DIVIDER);

        draw_image(scene, iconInfo, 60, 890);
        draw_text(scene, "Informacion y Ayuda", fontCardTitle, 120, 912, COLOR_WHITE);
        draw_text(scene, "Pulsa [L1] / [R1] para cambiar entre pestanas", fontFooter, 120, 938, COLOR_TEXT_DIM);

        // Navigation Hints
        draw_image(scene, btnTriangle, 980, 897);
        draw_text(scene, (upState == UPDATER_UPDATE_AVAILABLE) ? "Descargar PKG" : "Buscar Update", fontFooter, 1025, 922, Color{ 0, 255, 150 });

        draw_image(scene, btnSquare, 1230, 897);
        draw_text(scene, "Reinstalar Demonio", fontFooter, 1275, 922, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnL1, 1525, 901);
        draw_text(scene, "Pestanas", fontFooter, 1580, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnOptions, 1700, 900);
        draw_text(scene, "Salir", fontFooter, 1780, 922, COLOR_TEXT_DIM);
    }

    // 6. Toast Notification if active
    if (toastFrames > 0) {
        int boxW = 860;
        int boxH = 46;
        int boxX = (1920 - boxW) / 2;
        int boxY = 804;

        draw_rounded_rect(scene, boxX, boxY, boxW, boxH, 8, COLOR_TOAST_BG);
        draw_rounded_outline(scene, boxX, boxY, boxW, boxH, 8, COLOR_TOAST_BORDER, 2);
        const char* msg = (toastMsg && toastMsg[0]) ? toastMsg : "Ajustes guardados en /data/anycontroller/anycontroller.ini";
        draw_text_centered(scene, msg, fontCardTitle, 960, boxY + 31, COLOR_BLUE_BRIGHT);
    }
}

void UI::render_wizard(Scene2D* scene, const InstallResult& res, int frameID) {
    if (!scene) return;

    // Draw background with animated PlayStation XMB waves
    psBg.update(0.01666f);
    psBg.render(scene, cached_bg);

    // Top Header
    draw_image(scene, xboxLogo, 60, 60);
    draw_text(scene, "Controller Control Center PS4", fontTitle, 175, 95, COLOR_WHITE);
    draw_text(scene, res.isUpdate ? "Guía de Actualización e Inicio (Payload: ANYCONTROLLER)" : "Guía de Instalación e Inicio de Mandos (Payload: ANYCONTROLLER)", fontSubtitle, 175, 128, COLOR_BLUE_BRIGHT);

    // Main Card
    int cardW = 1420;
    int cardH = 580;
    int cardX = (1920 - cardW) / 2;
    int cardY = 210;

    draw_rounded_rect(scene, cardX, cardY, cardW, cardH, 16, COLOR_CARD_BG);
    draw_rounded_outline(scene, cardX - 1, cardY - 1, cardW + 2, cardH + 2, 16, COLOR_BLUE_GLOW, 2);
    draw_rounded_outline(scene, cardX, cardY, cardW, cardH, 16, COLOR_BLUE_MAIN, 2);

    // Title inside card
    draw_image(scene, iconSave, cardX + 40, cardY + 36);
    draw_text(scene, res.isUpdate ? "Actualización de Controller Control Center PS4 Completada" : "Instalación de Controller Control Center PS4 Completada", fontCardTitle, cardX + 105, cardY + 60, COLOR_WHITE);
    draw_text(scene, res.isUpdate ? "Payload ANYCONTROLLER y configuración actualizados con éxito para GoldHEN:" : "Payload ANYCONTROLLER e inicio automático configurados con éxito para GoldHEN:", fontCardDesc, cardX + 105, cardY + 90, COLOR_TEXT_DIM);

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
        draw_text(scene, "4. Reinicia tu PS4 si es la primera vez que activas el demonio en segundo plano.", fontCardDesc, cardX + 50, stepY, COLOR_TEXT_DIM);
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

    // Footer
    scene->DrawRectangle(60, 865, 1800, 2, COLOR_DIVIDER);
    draw_image(scene, iconInfo, 60, 890);
    draw_text(scene, "Instalación Inicial Completada", fontCardTitle, 120, 912, COLOR_WHITE);
    draw_text(scene, "El demonio ANYCONTROLLER arrancará automáticamente en cada reinicio de la consola", fontFooter, 120, 938, COLOR_TEXT_DIM);

    // Button X
    draw_image(scene, btnCross, 1500, 897);
    draw_text(scene, "Abrir Control Center", fontFooter, 1545, 922, COLOR_BLUE_BRIGHT);
}

void UI::render_tester(Scene2D* scene, const AppConfig& cfg, const OrbisPadData& padData, const TesterState& tester) {
    if (!scene) return;

    char buf[128];
    int deadzone_val = ConfigManager::DEADZONE_VALUES[cfg.deadzone_index];
    int thresh_val = ConfigManager::TRIGGER_VALUES[cfg.trigger_index];

    // ========================================================
    // HEADER AREA: INFO CARD & TEST MODE R3 BADGE
    // ========================================================
    // Card 1: Tester Info (X = CARD_X, Y = 135, W = 1060, H = 76)
    draw_rounded_rect(scene, CARD_X, 135, 1060, 76, 10, COLOR_CARD_BG);
    draw_rounded_outline(scene, CARD_X, 135, 1060, 76, 10, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconMode, CARD_X + 20, 135 + (76 - iconMode.height) / 2);

    draw_text(scene, "Probador de Entradas en Hardware Real", fontCardTitle, CARD_X + 75, 164, COLOR_WHITE);
    draw_text(scene, "Telemetría de palancas en tiempo real, rangos de gatillos y estado de botones", fontCardDesc, CARD_X + 75, 194, COLOR_TEXT_DIM);

    // Card 2: Test Mode R3 Badge Card (X = CARD_X + 1080 = 1460, Y = 135, W = 360, H = 76)
    int testCardX = CARD_X + 1080;
    int testCardW = 360;
    int testCardH = 76;

    if (tester.testMode) {
        Color badgeBg = Color{ 15, 60, 40 };
        Color badgeBorder = Color{ 0, 255, 150 };
        draw_rounded_rect(scene, testCardX, 135, testCardW, testCardH, 10, badgeBg);
        draw_rounded_outline(scene, testCardX, 135, testCardW, testCardH, 10, badgeBorder, 2);
        draw_image(scene, btnR3, testCardX + 16, 135 + (testCardH - btnR3.height) / 2);
        draw_text(scene, "MODO PRUEBA: ACTIVO", fontCardTitle, testCardX + 60, 164, Color{ 0, 255, 150 });
        draw_text(scene, "Pulsa R3 para volver", fontCardDesc, testCardX + 60, 194, COLOR_WHITE);
    } else {
        Color badgeBg = Color{ 14, 24, 38 };
        Color badgeBorder = COLOR_CARD_BORDER;
        draw_rounded_rect(scene, testCardX, 135, testCardW, testCardH, 10, badgeBg);
        draw_rounded_outline(scene, testCardX, 135, testCardW, testCardH, 10, badgeBorder, 1);
        draw_image(scene, btnR3, testCardX + 16, 135 + (testCardH - btnR3.height) / 2);
        draw_text(scene, "MODO PRUEBA: APAGADO", fontCardTitle, testCardX + 60, 164, COLOR_WHITE);
        draw_text(scene, "Pulsa R3 para aislar entradas", fontCardDesc, testCardX + 60, 194, COLOR_BLUE_BRIGHT);
    }

    // ========================================================
    // CONTROLLER SLOT SELECTION BAR (Player 1..4)
    // ========================================================
    int barX = CARD_X;
    int barY = 222;

    draw_image(scene, btnL2, barX + 6, barY + 9);

    int pillsStartX = barX + 46;
    int pillW = 220;
    int pillGap = 12;
    for (int s = 0; s < 4; s++) {
        int px = pillsStartX + s * (pillW + pillGap);
        int py = barY + 4;
        int ph = 38;
        bool isSel = (s == tester.selectedSlot);
        bool isConn = tester.slotConnected[s];

        Color bg = isSel ? COLOR_SIDEBAR_BLUE : COLOR_CARD_BG;
        Color border = isSel ? COLOR_BLUE_BRIGHT : COLOR_CARD_BORDER;
        draw_rounded_rect(scene, px, py, pillW, ph, 8, bg);
        draw_rounded_outline(scene, px, py, pillW, ph, 8, border, isSel ? 2 : 1);

        // Status LED dot
        Color dotColor = isConn ? Color{ 0, 255, 150 } : Color{ 80, 95, 110 };
        draw_circle(scene, px + 16, py + 19, 5, dotColor);

        char slotLabel[48];
        snprintf(slotLabel, sizeof(slotLabel), "Jugador %d %s", s + 1, isConn ? "(Activo)" : "(Inactivo)");
        draw_text(scene, slotLabel, fontSubtitle, px + 30, py + 26, isSel ? COLOR_WHITE : (isConn ? Color{ 200, 220, 240 } : COLOR_TEXT_DIM));
    }

    int r2X = pillsStartX + 4 * pillW + 3 * pillGap + 10;
    draw_image(scene, btnR2, r2X, barY + 9);
    if (tester.testMode) {
        draw_text(scene, "Cambio bloqueado (Modo Prueba)", fontCardDesc, r2X + 42, barY + 27, Color{ 0, 255, 150 });
    } else {
        draw_text(scene, "[L2/R2] o [IZQ/DER]: Cambiar Jugador", fontCardDesc, r2X + 42, barY + 27, COLOR_TEXT_DIM);
    }

    // Check if selected controller slot is connected
    if (!padData.connected) {
        int warnX = CARD_X;
        int warnY = 280;
        int warnW = CARD_W;
        int warnH = 560;

        Color warnBg = { 20, 26, 42 };
        Color warnBorder = { 0, 112, 220 };

        draw_rounded_rect(scene, warnX, warnY, warnW, warnH, 12, warnBg);
        draw_rounded_outline(scene, warnX, warnY, warnW, warnH, 12, warnBorder, 2);

        draw_image(scene, iconInfo, warnX + 40, warnY + 42);
        char noPadTitle[64];
        snprintf(noPadTitle, sizeof(noPadTitle), "Ranura %d (Jugador %d): Desconectado", tester.selectedSlot + 1, tester.selectedSlot + 1);
        draw_text(scene, noPadTitle, fontCardTitle, warnX + 105, warnY + 68, COLOR_WHITE);

        int textY = warnY + 130;
        draw_text(scene, "El Probador opera en modo hardware puro para aislar entradas por ranura de jugador.",
                  fontCardDesc, warnX + 40, textY, COLOR_TEXT_DIM);
        textY += 40;
        snprintf(buf, sizeof(buf), "No hay ningún mando activo actualmente en la Ranura %d (Jugador %d).", tester.selectedSlot + 1, tester.selectedSlot + 1);
        draw_text(scene, buf, fontCardDesc, warnX + 40, textY, COLOR_WHITE);
        textY += 40;
        draw_text(scene, "Pulsa [L2] / [R2] o [IZQ] / [DER] para cambiar a otra ranura de jugador,",
                  fontCardDesc, warnX + 40, textY, COLOR_BLUE_BRIGHT);
        textY += 36;
        draw_text(scene, "o conecta un mando DualSense (Bluetooth) o USB para asignarlo a esta ranura.",
                  fontCardDesc, warnX + 40, textY, COLOR_TEXT_DIM);

        // Footer for disconnected state
        scene->DrawRectangle(60, 865, 1800, 2, COLOR_DIVIDER);
        draw_image(scene, iconMode, 60, 890);
        snprintf(buf, sizeof(buf), "Probador de Entradas - Ranura %d (Jugador %d)", tester.selectedSlot + 1, tester.selectedSlot + 1);
        draw_text(scene, buf, fontCardTitle, 120, 912, COLOR_WHITE);
        draw_text(scene, "Esperando conexión de mando en esta ranura...", fontFooter, 120, 938, Color{ 220, 80, 60 });

        draw_image(scene, btnArrows, 1500, 901);
        draw_text(scene, "Cambiar Ranura", fontFooter, 1545, 922, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnOptions, 1680, 900);
        draw_text(scene, "Salir", fontFooter, 1760, 922, COLOR_TEXT_DIM);
        return;
    }

    // ========================================================
    // 1. ANALOG STICKS (ROW 1, Y: 280, H: 235)
    // ========================================================
    int stickR = 56;

    // --- Left Stick Card ---
    int lsX = CARD_X;
    int lsW = 555;
    int lsY = 280;
    int lsH = 235;

    draw_rounded_rect(scene, lsX, lsY, lsW, lsH, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, lsX, lsY, lsW, lsH, 12, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconDeadzone, lsX + 20, lsY + 16);
    draw_text(scene, "Palanca Izquierda (L3)", fontCardTitle, lsX + 70, lsY + 38, COLOR_WHITE);

    // Left Reticle
    int lcx = lsX + 105;
    int lcy = lsY + 140;

    // Crosshair & circle
    draw_circle_outline(scene, lcx, lcy, stickR, COLOR_CARD_BORDER, 2);
    scene->DrawRectangle(lcx - stickR, lcy, 2 * stickR, 1, COLOR_DIVIDER);
    scene->DrawRectangle(lcx, lcy - stickR, 1, 2 * stickR, COLOR_DIVIDER);

    // Deadzone circle
    int ldz_r = (deadzone_val * stickR) / 128;
    if (ldz_r > 0) {
        draw_circle_outline(scene, lcx, lcy, ldz_r, COLOR_BLUE_GLOW, 1);
    }

    // Left Stick coordinates
    int rawLX = (int)padData.leftStick.x;
    int rawLY = (int)padData.leftStick.y;
    int lDevX = rawLX - 128;
    int lDevY = rawLY - 128;
    int lsXPos = lcx + (lDevX * stickR) / 128;
    int lsYPos = lcy + (lDevY * stickR) / 128;
    int lDist = (int)sqrt((double)(lDevX * lDevX + lDevY * lDevY));
    bool lInDeadzone = (lDist <= deadzone_val);
    Color lCursorColor = lInDeadzone ? Color{ 40, 80, 130 } : COLOR_BLUE_MAIN;

    draw_circle(scene, lsXPos, lsYPos, 7, lCursorColor);
    draw_circle_outline(scene, lsXPos, lsYPos, 7, COLOR_WHITE, 1);

    // Left Stick Info Text
    int textX = lsX + 195;
    snprintf(buf, sizeof(buf), "X: %d  (%+d%%)", rawLX, (lDevX * 100) / 128);
    draw_text(scene, buf, fontCardDesc, textX, lsY + 85, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "Y: %d  (%+d%%)", rawLY, (lDevY * 100) / 128);
    draw_text(scene, buf, fontCardDesc, textX, lsY + 115, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "Zona Muerta: %s (val: %d)",
             ConfigManager::DEADZONE_OPTIONS[cfg.deadzone_index].c_str(), deadzone_val);
    draw_text(scene, buf, fontCardDesc, textX, lsY + 145, COLOR_TEXT_DIM);

    bool l3Pressed = (padData.buttons & ORBIS_PAD_BUTTON_L3) != 0;
    snprintf(buf, sizeof(buf), "Clic L3: %s", l3Pressed ? "PULSADO" : "Reposo");
    draw_text(scene, buf, fontCardDesc, textX, lsY + 175, l3Pressed ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);

    draw_text(scene, lInDeadzone ? "Salida: REPOSO (En Zona Muerta)" : "Salida: ENTRADA ACTIVA",
              fontCardDesc, textX, lsY + 205, lInDeadzone ? COLOR_TEXT_DIM : COLOR_BLUE_BRIGHT);

    // --- Right Stick Card ---
    int rsX = lsX + lsW + 30; // 410 + 555 + 30 = 995
    int rsW = 555;
    int rsY = 280;
    int rsH = 235;

    draw_rounded_rect(scene, rsX, rsY, rsW, rsH, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, rsX, rsY, rsW, rsH, 12, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconDeadzone, rsX + 20, rsY + 16);
    draw_text(scene, "Palanca Derecha (R3)", fontCardTitle, rsX + 70, rsY + 38, COLOR_WHITE);

    // Right Reticle
    int rcx = rsX + 105;
    int rcy = rsY + 140;

    draw_circle_outline(scene, rcx, rcy, stickR, COLOR_CARD_BORDER, 2);
    scene->DrawRectangle(rcx - stickR, rcy, 2 * stickR, 1, COLOR_DIVIDER);
    scene->DrawRectangle(rcx, rcy - stickR, 1, 2 * stickR, COLOR_DIVIDER);

    int rdz_r = (deadzone_val * stickR) / 128;
    if (rdz_r > 0) {
        draw_circle_outline(scene, rcx, rcy, rdz_r, COLOR_BLUE_GLOW, 1);
    }

    // Right Stick coordinates
    int rawRX = (int)padData.rightStick.x;
    int rawRY = (int)padData.rightStick.y;
    int rDevX = rawRX - 128;
    int rDevY = rawRY - 128;
    int rsXPos = rcx + (rDevX * stickR) / 128;
    int rsYPos = rcy + (rDevY * stickR) / 128;
    int rDist = (int)sqrt((double)(rDevX * rDevX + rDevY * rDevY));
    bool rInDeadzone = (rDist <= deadzone_val);
    Color rCursorColor = rInDeadzone ? Color{ 40, 80, 130 } : COLOR_BLUE_MAIN;

    draw_circle(scene, rsXPos, rsYPos, 7, rCursorColor);
    draw_circle_outline(scene, rsXPos, rsYPos, 7, COLOR_WHITE, 1);

    // Right Stick Info Text
    int rTextX = rsX + 195;
    snprintf(buf, sizeof(buf), "X: %d  (%+d%%)", rawRX, (rDevX * 100) / 128);
    draw_text(scene, buf, fontCardDesc, rTextX, rsY + 85, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "Y: %d  (%+d%%)", rawRY, (rDevY * 100) / 128);
    draw_text(scene, buf, fontCardDesc, rTextX, rsY + 115, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "Zona Muerta: %s (val: %d)",
             ConfigManager::DEADZONE_OPTIONS[cfg.deadzone_index].c_str(), deadzone_val);
    draw_text(scene, buf, fontCardDesc, rTextX, rsY + 145, COLOR_TEXT_DIM);

    bool r3Pressed = (padData.buttons & ORBIS_PAD_BUTTON_R3) != 0;
    snprintf(buf, sizeof(buf), "Clic R3: %s", r3Pressed ? "PULSADO" : "Reposo");
    draw_text(scene, buf, fontCardDesc, rTextX, rsY + 175, r3Pressed ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);

    draw_text(scene, rInDeadzone ? "Salida: REPOSO (En Zona Muerta)" : "Salida: ENTRADA ACTIVA",
              fontCardDesc, rTextX, rsY + 205, rInDeadzone ? COLOR_TEXT_DIM : COLOR_BLUE_BRIGHT);

    // ========================================================
    // 2. TRIGGERS & BUMPERS (ROW 2, Y: 530, H: 155) - NO OVERLAP
    // ========================================================
    int trigX = CARD_X;
    int trigY = 530;
    int trigW = 1140;
    int trigH = 155;

    draw_rounded_rect(scene, trigX, trigY, trigW, trigH, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, trigX, trigY, trigW, trigH, 12, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconSwap, trigX + 24, trigY + 16);
    draw_text(scene, "Gatillos y Protectores (L2 / R2 / L1 / R1)", fontCardTitle, trigX + 78, trigY + 38, COLOR_WHITE);

    // Left Trigger & Bumper (LT & LB)
    int rawLT = (int)padData.analogButtons.l2;
    bool l2Btn = (padData.buttons & ORBIS_PAD_BUTTON_L2) != 0;
    bool lbBtn = (padData.buttons & ORBIS_PAD_BUTTON_L1) != 0;

    int ltBarX = trigX + 30;
    int ltBarY = trigY + 98;
    int ltBarW = 390;
    int ltBarH = 20;

    snprintf(buf, sizeof(buf), "L2: %d / 255 (%d%%)%s", rawLT, (rawLT * 100) / 255, l2Btn ? " [CLIC]" : "");
    draw_text(scene, buf, fontCardDesc, ltBarX, trigY + 84, l2Btn ? COLOR_BLUE_BRIGHT : COLOR_WHITE);

    draw_rounded_rect(scene, ltBarX, ltBarY, ltBarW, ltBarH, 4, Color{ 18, 30, 48 });
    int ltFillW = (rawLT * ltBarW) / 255;
    if (ltFillW > 0) {
        draw_rounded_rect(scene, ltBarX, ltBarY, ltFillW, ltBarH, 4, l2Btn ? COLOR_BLUE_MAIN : Color{ 15, 60, 120 });
    }
    // Threshold marker
    int ltMarkerX = ltBarX + (thresh_val * ltBarW) / 255;
    scene->DrawRectangle(ltMarkerX, ltBarY - 3, 2, ltBarH + 6, COLOR_WHITE);

    // LB Bumper pill (generous spacing beside LT bar)
    int lbPillX = trigX + 440;
    int lbPillY = trigY + 84;
    draw_btn_badge(scene, lbPillX, lbPillY, 95, 34, "L1 / LB", lbBtn);

    // Right Trigger & Bumper (RT & RB)
    int rawRT = (int)padData.analogButtons.r2;
    bool r2Btn = (padData.buttons & ORBIS_PAD_BUTTON_R2) != 0;
    bool rbBtn = (padData.buttons & ORBIS_PAD_BUTTON_R1) != 0;

    int rtBarX = trigX + 590;
    int rtBarY = trigY + 98;
    int rtBarW = 390;
    int rtBarH = 20;

    snprintf(buf, sizeof(buf), "R2: %d / 255 (%d%%)%s", rawRT, (rawRT * 100) / 255, r2Btn ? " [CLIC]" : "");
    draw_text(scene, buf, fontCardDesc, rtBarX, trigY + 84, r2Btn ? COLOR_BLUE_BRIGHT : COLOR_WHITE);

    draw_rounded_rect(scene, rtBarX, rtBarY, rtBarW, rtBarH, 4, Color{ 18, 30, 48 });
    int rtFillW = (rawRT * rtBarW) / 255;
    if (rtFillW > 0) {
        draw_rounded_rect(scene, rtBarX, rtBarY, rtFillW, rtBarH, 4, r2Btn ? COLOR_BLUE_MAIN : Color{ 15, 60, 120 });
    }
    // Threshold marker
    int rtMarkerX = rtBarX + (thresh_val * rtBarW) / 255;
    scene->DrawRectangle(rtMarkerX, rtBarY - 3, 2, rtBarH + 6, COLOR_WHITE);

    // RB Bumper pill
    int rbPillX = trigX + 1000;
    int rbPillY = trigY + 84;
    draw_btn_badge(scene, rbPillX, rbPillY, 95, 34, "R1 / RB", rbBtn);

    // ========================================================
    // 3. VIBRATION TEST & STICK DRIFT CALIBRATION (ROW 3, Y: 700, H: 150)
    // ========================================================
    // Left Box: Vibration Feedback Test
    int vibX = CARD_X;
    int vibY = 700;
    int vibW = 555;
    int vibH = 150;

    draw_rounded_rect(scene, vibX, vibY, vibW, vibH, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, vibX, vibY, vibW, vibH, 12, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconRumble, vibX + 24, vibY + 16);
    draw_text(scene, "Prueba de Vibracion y Motores Hapticos", fontCardTitle, vibX + 78, vibY + 38, COLOR_WHITE);

    draw_image(scene, btnSquare, vibX + 30, vibY + 68);
    draw_text(scene, "Probar Motores Dobles de Vibracion", fontCardDesc, vibX + 76, vibY + 90, COLOR_WHITE);

    if (tester.rumblePhase == 0) {
        draw_text(scene, "Prueba motor izquierdo (fuerte) y derecho (suave)", fontCardDesc, vibX + 30, vibY + 124, COLOR_TEXT_DIM);
    } else if (tester.rumblePhase == 1) {
        draw_text(scene, "[PROBANDO] Motor Izquierdo (Fuerte / Frecuencia Baja)...", fontCardDesc, vibX + 30, vibY + 124, COLOR_BLUE_BRIGHT);
    } else if (tester.rumblePhase == 2) {
        draw_text(scene, "[PROBANDO] Motor Derecho (Suave / Frecuencia Alta)...", fontCardDesc, vibX + 30, vibY + 124, COLOR_BLUE_BRIGHT);
    } else {
        draw_text(scene, "[PROBANDO] Ambos Motores Activos (Fuerza Maxima)...", fontCardDesc, vibX + 30, vibY + 124, COLOR_BLUE_BRIGHT);
    }

    // Right Box: Stick Drift Auto-Calibration
    int calX = vibX + vibW + 30; // 995
    int calY = 700;
    int calW = 555;
    int calH = 150;

    draw_rounded_rect(scene, calX, calY, calW, calH, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, calX, calY, calW, calH, 12, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconDeadzone, calX + 24, calY + 16);
    draw_text(scene, "Calibracion Automatica de Deriva (Drift)", fontCardTitle, calX + 78, calY + 38, COLOR_WHITE);

    if (tester.calib.active) {
        int progressW = (tester.calib.currentFrame * 495) / tester.calib.totalFrames;
        draw_rounded_rect(scene, calX + 30, calY + 68, 495, 16, 4, Color{ 18, 30, 48 });
        draw_rounded_rect(scene, calX + 30, calY + 68, progressW, 16, 4, COLOR_BLUE_MAIN);

        snprintf(buf, sizeof(buf), "Muestreando reposo: %d%%  (Ruido maximo: %d)",
                 (tester.calib.currentFrame * 100) / tester.calib.totalFrames, tester.calib.maxNoise);
        draw_text(scene, buf, fontCardDesc, calX + 30, calY + 124, COLOR_BLUE_BRIGHT);
    } else if (tester.calib.completed) {
        snprintf(buf, sizeof(buf), "Deriva: %d -> Zona Muerta Sugerida: %s",
                 tester.calib.maxNoise, ConfigManager::DEADZONE_OPTIONS[tester.calib.suggestedIndex].c_str());
        draw_text(scene, buf, fontCardDesc, calX + 30, calY + 86, COLOR_WHITE);

        draw_image(scene, btnCross, calX + 30, calY + 102);
        draw_text(scene, "Aplicar y guardar zona muerta recomendada", fontCardDesc, calX + 76, calY + 124, COLOR_BLUE_BRIGHT);
    } else {
        draw_image(scene, btnTriangle, calX + 30, calY + 68);
        draw_text(scene, "Analizar Deriva de Palancas en Reposo (3s)", fontCardDesc, calX + 76, calY + 90, COLOR_WHITE);
        draw_text(scene, "Mide el ruido de los sticks para calcular la zona muerta optima", fontCardDesc, calX + 30, calY + 124, COLOR_TEXT_DIM);
    }

    // ========================================================
    // 3B. RIGHT COLUMN: TOUCHPAD, D-PAD (CRUCETA) & ACTION BUTTONS (X: 1565, Y: 280, W: 285, H: 570)
    // ========================================================
    int tpCardX = 1565;
    int tpCardY = 280;
    int tpCardW = 285;
    int tpCardH = 570;

    draw_rounded_rect(scene, tpCardX, tpCardY, tpCardW, tpCardH, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, tpCardX, tpCardY, tpCardW, tpCardH, 12, COLOR_CARD_BORDER, 1);
    draw_image(scene, btnTouchpad, tpCardX + 18, tpCardY + 16);
    draw_text(scene, "Panel Táctil y Cruceta", fontCardTitle, tpCardX + 66, tpCardY + 38, COLOR_WHITE);

    // Interactive 2D Touchpad Surface (W: 249, H: 112)
    bool tpClicked = (padData.buttons & ORBIS_PAD_BUTTON_TOUCH_PAD) != 0;
    int tpBoxX = tpCardX + 18;
    int tpBoxY = tpCardY + 58;
    int tpBoxW = 249;
    int tpBoxH = 112;

    Color tpSurfBg = tpClicked ? Color{ 16, 75, 58 } : Color{ 12, 20, 34 };
    Color tpSurfBorder = tpClicked ? Color{ 0, 255, 160 } : (padData.touch.fingers > 0 ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM);
    draw_rounded_rect(scene, tpBoxX, tpBoxY, tpBoxW, tpBoxH, 8, tpSurfBg);
    draw_rounded_outline(scene, tpBoxX, tpBoxY, tpBoxW, tpBoxH, 8, tpSurfBorder, (tpClicked || padData.touch.fingers > 0) ? 2 : 1);

    // Subtle touchpad center divider
    scene->DrawRectangle(tpBoxX + tpBoxW / 2, tpBoxY + 8, 1, tpBoxH - 16, COLOR_DIVIDER);

    if (padData.touch.fingers > 0) {
        for (int f = 0; f < (int)padData.touch.fingers && f < 2; f++) {
            int tx = (int)padData.touch.touch[f].x;
            int ty = (int)padData.touch.touch[f].y;
            if (tx < 0) tx = 0;
            if (tx > 1919) tx = 1919;
            if (ty < 0) ty = 0;
            if (ty > 941) ty = 941;
            int dotX = tpBoxX + 8 + (tx * (tpBoxW - 16)) / 1920;
            int dotY = tpBoxY + 8 + (ty * (tpBoxH - 16)) / 942;
            Color fCol = (f == 0) ? Color{ 0, 240, 255 } : Color{ 255, 180, 0 };
            draw_circle(scene, dotX, dotY, 7, fCol);
            draw_circle_outline(scene, dotX, dotY, 7, COLOR_WHITE, 1);
        }
        snprintf(buf, sizeof(buf), "Táctil: (%d, %d)%s",
                 (int)padData.touch.touch[0].x, (int)padData.touch.touch[0].y,
                 tpClicked ? " [CLIC]" : "");
        draw_text_centered(scene, buf, fontCardDesc, tpBoxX + tpBoxW / 2, tpBoxY + tpBoxH + 22,
                           tpClicked ? Color{ 0, 255, 160 } : COLOR_BLUE_BRIGHT);
    } else {
        draw_text_centered(scene, tpClicked ? "¡CLIC TÁCTIL PULSADO!" : "Panel Táctil (Botón Grande)",
                           fontCardDesc, tpBoxX + tpBoxW / 2, tpBoxY + tpBoxH / 2 + 6,
                           tpClicked ? Color{ 0, 255, 160 } : COLOR_TEXT_DIM);
        draw_text_centered(scene, tpClicked ? "Estado: CLIC MECÁNICO ACTIVO" : "Toca o pulsa el panel central",
                           fontCardDesc, tpBoxX + tpBoxW / 2, tpBoxY + tpBoxH + 22,
                           tpClicked ? Color{ 0, 255, 160 } : COLOR_TEXT_DIM);
    }

    // D-Pad (Cruceta) Live 4-Way Visualizer
    draw_text(scene, "Cruceta Digital (D-Pad):", fontCardDesc, tpCardX + 18, tpCardY + 218, COLOR_WHITE);
    bool dUp    = (padData.buttons & ORBIS_PAD_BUTTON_UP) != 0;
    bool dDown  = (padData.buttons & ORBIS_PAD_BUTTON_DOWN) != 0;
    bool dLeft  = (padData.buttons & ORBIS_PAD_BUTTON_LEFT) != 0;
    bool dRight = (padData.buttons & ORBIS_PAD_BUTTON_RIGHT) != 0;

    int dpCenterX = tpCardX + tpCardW / 2;
    int dpTopY = tpCardY + 230;
    draw_btn_badge(scene, dpCenterX - 38, dpTopY,      76, 32, "ARRIBA", dUp);
    draw_btn_badge(scene, dpCenterX - 118, dpTopY + 38, 74, 32, "IZQ",    dLeft);
    draw_btn_badge(scene, dpCenterX + 44,  dpTopY + 38, 74, 32, "DER",    dRight);
    draw_btn_badge(scene, dpCenterX - 38, dpTopY + 76, 76, 32, "ABAJO",  dDown);

    // Face & System Buttons Live Matrix
    draw_text(scene, "Botones Frontales y Sistema:", fontCardDesc, tpCardX + 18, tpCardY + 366, COLOR_WHITE);
    bool bCross    = (padData.buttons & ORBIS_PAD_BUTTON_CROSS) != 0;
    bool bCircle   = (padData.buttons & ORBIS_PAD_BUTTON_CIRCLE) != 0;
    bool bSquare   = (padData.buttons & ORBIS_PAD_BUTTON_SQUARE) != 0;
    bool bTriangle = (padData.buttons & ORBIS_PAD_BUTTON_TRIANGLE) != 0;
    bool bShare    = (padData.buttons & 0x00000001U) != 0;
    bool bOptions  = (padData.buttons & ORBIS_PAD_BUTTON_OPTIONS) != 0;

    int col1X = tpCardX + 18;
    int col2X = tpCardX + 148;
    int btnW = 119;
    int btnH = 34;

    draw_btn_badge(scene, col1X, tpCardY + 380, btnW, btnH, "X (Cruz)",     bCross);
    draw_btn_badge(scene, col2X, tpCardY + 380, btnW, btnH, "O (Círculo)",  bCircle);
    draw_btn_badge(scene, col1X, tpCardY + 422, btnW, btnH, "Cuadrado",     bSquare);
    draw_btn_badge(scene, col2X, tpCardY + 422, btnW, btnH, "Triángulo",    bTriangle);
    draw_btn_badge(scene, col1X, tpCardY + 464, btnW, btnH, "Share/Create", bShare);
    draw_btn_badge(scene, col2X, tpCardY + 464, btnW, btnH, "Options",      bOptions);
    draw_btn_badge(scene, col1X, tpCardY + 506, 249,  btnH, "Clic Panel Táctil (Botón Grande)", tpClicked);

    // ========================================================
    // 4. FOOTER FOR TESTER
    // ========================================================
    scene->DrawRectangle(60, 865, 1800, 2, COLOR_DIVIDER);

    draw_image(scene, iconMode, 60, 890);
    snprintf(buf, sizeof(buf), "Probador de Entradas - Ranura %d (Jugador %d)", tester.selectedSlot + 1, tester.selectedSlot + 1);
    draw_text(scene, buf, fontCardTitle, 120, 912, COLOR_WHITE);

    if (tester.testMode) {
        draw_text(scene, "Modo Prueba Activo - Todas las entradas aisladas, navegacion bloqueada", fontFooter, 120, 938, Color{ 0, 255, 150 });

        draw_image(scene, btnR3, 1600, 897);
        draw_text(scene, "Salir Modo Prueba", fontFooter, 1645, 922, Color{ 0, 255, 150 });
    } else {
        draw_text(scene, "Modo puro por hardware (entradas independientes por jugador)", fontFooter, 120, 938, COLOR_BLUE_BRIGHT);

        // Hints with authentic button icons (generous horizontal spacing so labels never touch)
        draw_image(scene, btnR3, 760, 897);
        draw_text(scene, "Modo Prueba", fontFooter, 804, 922, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnArrows, 965, 901);
        draw_text(scene, "Ranura", fontFooter, 1018, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnSquare, 1125, 897);
        draw_text(scene, "Vibrar", fontFooter, 1170, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnTriangle, 1275, 897);
        draw_text(scene, "Calibrar", fontFooter, 1320, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnCross, 1450, 897);
        draw_text(scene, "Aplicar", fontFooter, 1495, 922, tester.calib.completed ? COLOR_BLUE_BRIGHT : COLOR_TEXT_DIM);

        draw_image(scene, btnOptions, 1650, 900);
        draw_text(scene, "Salir", fontFooter, 1730, 922, COLOR_TEXT_DIM);
    }
}

void UI::render_remap(Scene2D* scene, const HidStatus& status, const HidProfile& profile,
                      int selectedRow, const OrbisPadData& padData, bool testMode) {
    if (!scene) return;

    // Header Area: Sdoppiata in 2 Card affiancate e indipendenti
    // Card 1: Device info (X = CARD_X, Y = 135, W = 1060, H = 88)
    draw_rounded_rect(scene, CARD_X, 135, 1060, 88, 10, COLOR_CARD_BG);
    draw_rounded_outline(scene, CARD_X, 135, 1060, 88, 10, COLOR_CARD_BORDER, 1);
    draw_image(scene, iconRemap, CARD_X + 20, 135 + (88 - iconRemap.height) / 2);

    char devTitle[128];
    snprintf(devTitle, sizeof(devTitle), "Dispositivo HID: %s", status.name.c_str());
    draw_text(scene, devTitle, fontCardTitle, CARD_X + 75, 168, COLOR_WHITE);

    char devSub[256];
    if (status.player_slot > 0 && !status.user_name.empty()) {
        snprintf(devSub, sizeof(devSub), "Jugador %d (%s)   •   Hardware: VID 0x%04X | PID 0x%04X   •   Perfil: %04X_%04X.ini",
                 status.player_slot, status.user_name.c_str(), status.vid, status.pid, status.vid, status.pid);
    } else {
        snprintf(devSub, sizeof(devSub), "Hardware: VID 0x%04X | PID 0x%04X   •   Botones: %d   •   Perfil: %04X_%04X.ini",
                 status.vid, status.pid, status.nbuttons, status.vid, status.pid);
    }
    draw_text(scene, devSub, fontCardDesc, CARD_X + 75, 198, COLOR_TEXT_DIM);

    // Card 2: Test Mode R3 Badge Card (X = 1500, Y = 135, W = 360, H = 88)
    int testCardX = CARD_X + 1080;
    int testCardW = 360;
    int testCardH = 88;

    if (testMode) {
        Color badgeBg = Color{ 15, 60, 40 };
        Color badgeBorder = Color{ 0, 255, 150 };
        draw_rounded_rect(scene, testCardX, 135, testCardW, testCardH, 10, badgeBg);
        draw_rounded_outline(scene, testCardX, 135, testCardW, testCardH, 10, badgeBorder, 2);
        draw_image(scene, btnR3, testCardX + 16, 135 + (testCardH - btnR3.height) / 2);
        draw_text(scene, "MODO PRUEBA: ACTIVO", fontCardTitle, testCardX + 60, 168, Color{ 0, 255, 150 });
        draw_text(scene, "Pulsa R3 para volver", fontCardDesc, testCardX + 60, 198, COLOR_WHITE);
    } else {
        Color badgeBg = Color{ 14, 24, 38 };
        Color badgeBorder = COLOR_CARD_BORDER;
        draw_rounded_rect(scene, testCardX, 135, testCardW, testCardH, 10, badgeBg);
        draw_rounded_outline(scene, testCardX, 135, testCardW, testCardH, 10, badgeBorder, 1);
        draw_image(scene, btnR3, testCardX + 16, 135 + (testCardH - btnR3.height) / 2);
        draw_text(scene, "MODO PRUEBA: APAGADO", fontCardTitle, testCardX + 60, 168, COLOR_WHITE);
        draw_text(scene, "Pulsa R3 para probar botones", fontCardDesc, testCardX + 60, 198, COLOR_BLUE_BRIGHT);
    }

    // Grid of 14 actions (2 columns x 7 rows, H = 70, GAP = 10, fine a Y = 788)
    const int COL_W = 690;
    const int COL_GAP = 60;
    const int ROW_H = 70;
    const int ROW_GAP = 10;
    const int GRID_Y = 238;

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
        draw_text(scene, HidProfile::ACTIONS[i].display_name, fontCardTitle, textX, itemY + 44, isSelected ? COLOR_WHITE : Color{ 210, 225, 245 });

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
        int pillY = itemY + 16;
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

    // Footer with exact PlayStation Button Icons
    scene->DrawRectangle(60, 865, 1800, 2, COLOR_DIVIDER);

    if (testMode) {
        draw_image(scene, iconMode, 60, 890);
        draw_text(scene, "Modo Prueba de Entradas HID Activo", fontCardTitle, 120, 912, Color{ 0, 255, 150 });
        draw_text(scene, "Acciones pausadas. Pulsa botones fisicos para ver el mapeo en tiempo real.", fontFooter, 120, 938, COLOR_WHITE);

        // Footer command icons for Test Mode
        draw_image(scene, btnR3, 1580, 897);
        draw_text(scene, "Salir Modo Prueba", fontFooter, 1630, 922, Color{ 0, 255, 150 });
    } else {
        draw_image(scene, iconRemap, 60, 890);
        draw_text(scene, "Reasignacion de Botones HID", fontCardTitle, 120, 912, COLOR_WHITE);
        std::string profileMsg = profile.get_current_path().empty() ?
            "Perfil activo en /data/anycontroller/controllers/" :
            ("Perfil activo: " + profile.get_current_path());
        draw_text(scene, profileMsg.c_str(), fontFooter, 120, 938, COLOR_BLUE_BRIGHT);

        // Footer command icons for Edit Mode
        draw_image(scene, btnR3, 880, 897);
        draw_text(scene, "Modo Prueba", fontFooter, 925, 922, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnCross, 1085, 897);
        draw_text(scene, "Guardar", fontFooter, 1130, 922, COLOR_BLUE_BRIGHT);

        draw_image(scene, btnSquare, 1240, 897);
        draw_text(scene, "Defecto", fontFooter, 1285, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnTriangle, 1395, 897);
        draw_text(scene, "Recargar", fontFooter, 1440, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnArrows, 1555, 901);
        draw_text(scene, "Valor", fontFooter, 1608, 922, COLOR_TEXT_DIM);

        draw_image(scene, btnOptions, 1705, 900);
        draw_text(scene, "Salir", fontFooter, 1785, 922, COLOR_TEXT_DIM);
    }
}

void UI::render_dualsense(Scene2D* scene, const AppConfig& cfg, int selectedCard, const HidStatus& status) {
    // Top Exclusive Banner for PS5 DualSense
    draw_rounded_rect(scene, 410, 145, 1440, 110, 12, Color{ 13, 24, 46 });
    draw_rounded_outline(scene, 410, 145, 1440, 110, 12, COLOR_BLUE_MAIN, 1);
    draw_rounded_rect(scene, 410, 145, 6, 110, 3, COLOR_BLUE_BRIGHT);
    draw_image(scene, btnPs, 436, 175);
    draw_text(scene, "Centro Exclusivo para Mandos PlayStation 5 (DualSense / DualSense Edge)", fontCardTitle, 496, 190, COLOR_WHITE);
    draw_text(scene, "Control dedicado de Gatillos Adaptativos L2/R2, Altavoz Interno, LEDs de Jugador, Botón Mute y Reportes BT de 78 Bytes.", fontCardDesc, 496, 222, COLOR_BLUE_BRIGHT);

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

    // Static color preview table
    static const Color colorPreviews[6] = {
        Color{ 0, 112, 209 },  // Blue
        Color{ 255, 40, 40 },  // Red
        Color{ 40, 220, 40 },  // Green
        Color{ 160, 40, 255 }, // Purple
        Color{ 0, 230, 255 },  // Cyan
        Color{ 245, 245, 245 } // White
    };

    const int DS_CARD_X = 410;
    const int DS_CARD_W = 880;
    const int DS_CARD_H = 96;
    const int DS_CARD_SPACING = 14;

    for (int i = 0; i < VISIBLE_CARDS; i++) {
        int cardIdx = i + scrollOffset;
        if (cardIdx >= TOTAL_CARDS) break;

        int cardY = 280 + i * (DS_CARD_H + DS_CARD_SPACING);
        bool isSelected = (cardIdx == selectedCard);

        draw_rounded_rect(scene, DS_CARD_X, cardY, DS_CARD_W, DS_CARD_H, 12, isSelected ? Color{ 16, 30, 54 } : COLOR_CARD_BG);

        if (isSelected) {
            draw_rounded_outline(scene, DS_CARD_X - 1, cardY - 1, DS_CARD_W + 2, DS_CARD_H + 2, 12, COLOR_BLUE_GLOW, 2);
            draw_rounded_outline(scene, DS_CARD_X, cardY, DS_CARD_W, DS_CARD_H, 12, COLOR_BLUE_MAIN, 2);
            draw_rounded_rect(scene, DS_CARD_X, cardY + 14, 5, DS_CARD_H - 28, 2, COLOR_BLUE_BRIGHT);
        } else {
            draw_rounded_outline(scene, DS_CARD_X, cardY, DS_CARD_W, DS_CARD_H, 12, COLOR_CARD_BORDER, 1);
        }

        if (cardIcons[cardIdx]) {
            draw_image(scene, *cardIcons[cardIdx], DS_CARD_X + 20, cardY + 24);
        }

        draw_text(scene, cardTitles[cardIdx], fontCardTitle, DS_CARD_X + 80, cardY + 38, COLOR_WHITE);
        draw_text(scene, cardDescs[cardIdx], fontCardDesc, DS_CARD_X + 80, cardY + 70, COLOR_TEXT_DIM);

        int vBoxW = 290;
        int vBoxH = 48;
        int vBoxX = DS_CARD_X + DS_CARD_W - vBoxW - 18;
        int vBoxY = cardY + (DS_CARD_H - vBoxH) / 2;
        draw_rounded_rect(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? Color{ 10, 22, 42 } : Color{ 8, 13, 24 });
        draw_rounded_outline(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM, 1);

        if (isSelected) {
            draw_text(scene, "<", fontValue, vBoxX + 12, cardY + 54, COLOR_BLUE_BRIGHT);
            draw_text(scene, ">", fontValue, vBoxX + vBoxW - 24, cardY + 54, COLOR_BLUE_BRIGHT);
        }

        if (cardIdx == 1) {
            int c_idx = cfg.ds_lightbar_color;
            if (c_idx >= 0 && c_idx < 6) {
                draw_circle(scene, vBoxX + 36, cardY + 48, 7, colorPreviews[c_idx]);
                draw_circle_outline(scene, vBoxX + 36, cardY + 48, 7, COLOR_WHITE, 1);
            }
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2 + 8, cardY + 54, COLOR_WHITE);
        } else if (cardIdx == 5) {
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2, cardY + 44, COLOR_WHITE);
            int vBarX = vBoxX + 55;
            int vBarY = cardY + 60;
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
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2, cardY + 54, COLOR_WHITE);
        }
    }

    if (scrollOffset > 0) {
        draw_text(scene, "^", fontCardTitle, DS_CARD_X + DS_CARD_W / 2, 275, COLOR_BLUE_BRIGHT);
    }
    if (scrollOffset + VISIBLE_CARDS < TOTAL_CARDS) {
        draw_text(scene, "v", fontCardTitle, DS_CARD_X + DS_CARD_W / 2, 280 + VISIBLE_CARDS * (DS_CARD_H + DS_CARD_SPACING) - 2, COLOR_BLUE_BRIGHT);
    }

    // Right-Hand PS5 DualSense Multi-Controller Telemetry & Battery Box
    const int BOX_X = 1310;
    const int BOX_Y = 280;
    const int BOX_W = 540;
    const int BOX_H = 536;

    draw_rounded_rect(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BORDER, 1);

    draw_image(scene, btnPs, BOX_X + 24, BOX_Y + 22);
    char dsHeader[64];
    int activeDevs = (int)status.devices.size();
    if (activeDevs > 1) {
        snprintf(dsHeader, sizeof(dsHeader), "Multijugador PS5 (%d Mandos)", activeDevs);
    } else {
        snprintf(dsHeader, sizeof(dsHeader), "Telemetría PS5 DualSense");
    }
    draw_text(scene, dsHeader, fontCardTitle, BOX_X + 76, BOX_Y + 40, COLOR_WHITE);
    draw_text(scene, "Batería Exacta, Carga USB-C y Perfiles PS4 en Vivo", fontCardDesc, BOX_X + 76, BOX_Y + 62, COLOR_BLUE_BRIGHT);

    draw_rounded_rect(scene, BOX_X + 24, BOX_Y + 76, BOX_W - 48, 1, 0, COLOR_CARD_BORDER);

    // Build list of up to 4 slots to display
    int numToShow = (activeDevs > 0) ? activeDevs : 1;
    if (numToShow > 3) numToShow = 3; // Fit up to 3 detailed cards cleanly in box
    int curY = BOX_Y + 90;

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

        int cardH = (numToShow == 1) ? 132 : 96;
        draw_rounded_rect(scene, BOX_X + 22, curY, BOX_W - 44, cardH, 10, Color{ 14, 24, 42 });
        draw_rounded_outline(scene, BOX_X + 22, curY, BOX_W - 44, cardH, 10, (chg || bstat == 1) ? Color{ 0, 240, 180 } : COLOR_BORDER_DIM, 1);

        // Player Slot + Assigned PS4 User Profile
        draw_circle(scene, BOX_X + 40, curY + 24, 5, Color{ 0, 255, 140 });
        char pTitle[128];
        if (!uname.empty()) {
            snprintf(pTitle, sizeof(pTitle), "Jugador %d • Perfil: %s", slotNum, uname.c_str());
        } else {
            snprintf(pTitle, sizeof(pTitle), "Jugador %d • DualSense BT", slotNum);
        }
        draw_text(scene, pTitle, fontCardTitle, BOX_X + 54, curY + 30, COLOR_WHITE);

        // Exact Battery % + Charging State Badge
        char bLabel[64];
        Color bTextCol = COLOR_BLUE_BRIGHT;
        if (bstat == 1 || chg) {
            snprintf(bLabel, sizeof(bLabel), "%d%% [CARGANDO USB-C]", batt);
            bTextCol = Color{ 0, 255, 180 };
        } else if (bstat == 2) {
            snprintf(bLabel, sizeof(bLabel), "100%% [CARGA COMPLETA]");
            bTextCol = Color{ 0, 255, 140 };
        } else {
            snprintf(bLabel, sizeof(bLabel), "Batería: %d%%", batt);
            bTextCol = (batt <= 20) ? Color{ 255, 80, 80 } : COLOR_BLUE_BRIGHT;
        }
        draw_text(scene, bLabel, fontCardDesc, BOX_X + 40, curY + 58, bTextCol);

        // Battery Progress Bar
        int barX = BOX_X + 40;
        int barY = curY + 68;
        int barW = BOX_W - 80;
        int barH = 14;
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
            draw_text(scene, "Carga USB-C activa: Puedes cargar por cable mientras juegas por BT.", fontCardDesc, BOX_X + 40, curY + 112, COLOR_TEXT_DIM);
        }

        curY += cardH + 10;
    }

    // Multiplayer & Profile Selector Action Card at bottom of Telemetry Box
    int mpY = BOX_Y + 394;
    draw_rounded_rect(scene, BOX_X + 22, mpY, BOX_W - 44, 122, 10, Color{ 15, 32, 58 });
    draw_rounded_outline(scene, BOX_X + 22, mpY, BOX_W - 44, 122, 10, COLOR_BLUE_MAIN, 1);
    draw_image(scene, btnTriangle, BOX_X + 36, mpY + 16);
    draw_text(scene, "Multijugador Local y Cambio de Perfil PS4", fontCardTitle, BOX_X + 82, mpY + 38, Color{ 0, 255, 160 });
    draw_text(scene, "Pulsa [TRIÁNGULO] aquí o (Create + Cruceta Izq) en el mando", fontCardDesc, BOX_X + 36, mpY + 70, COLOR_WHITE);
    draw_text(scene, "para abrir '¿Quién está usando este mando?' y elegir perfil.", fontCardDesc, BOX_X + 36, mpY + 98, COLOR_BLUE_BRIGHT);

    // Footer
    scene->DrawRectangle(56, 865, 1808, 2, COLOR_DIVIDER);

    draw_image(scene, btnPs, 56, 890);
    draw_text(scene, "Módulo Exclusivo PS5 DualSense", fontCardTitle, 116, 912, COLOR_WHITE);
    draw_text(scene, "Soporte hasta 4 mandos DualSense, batería en vivo y efectos RGB Arcoíris/Respiración", fontFooter, 116, 938, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnTriangle, 1110, 897);
    draw_text(scene, "Elegir Perfil PS4", fontFooter, 1155, 922, Color{ 0, 255, 150 });

    draw_image(scene, btnArrows, 1370, 901);
    draw_text(scene, "Cambiar", fontFooter, 1424, 922, COLOR_TEXT_DIM);

    draw_image(scene, btnCross, 1540, 897);
    draw_text(scene, "Guardar", fontFooter, 1585, 922, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnOptions, 1690, 900);
    draw_text(scene, "Salir", fontFooter, 1770, 922, COLOR_TEXT_DIM);
}

void UI::render_dualshock4(Scene2D* scene, const AppConfig& cfg, int selectedCard, const OrbisPadData& padData, const TesterState& tester) {
    // Top Exclusive Banner for Native PS4 DualShock 4
    draw_rounded_rect(scene, 410, 145, 1440, 110, 12, Color{ 16, 22, 44 });
    draw_rounded_outline(scene, 410, 145, 1440, 110, 12, COLOR_ACCENT_INDIGO, 1);
    draw_rounded_rect(scene, 410, 145, 6, 110, 3, COLOR_ACCENT_INDIGO);
    draw_image(scene, iconRumble, 436, 175);
    draw_text(scene, "Configurador Nativo DualShock 4 (Mandos Originales PS4 CUH-ZCT1 / ZCT2)", fontCardTitle, 496, 190, COLOR_WHITE);
    draw_text(scene, "Personaliza al instante la barra de luz frontal del mando PS4, brillo de ahorro de batería, motores de vibración y giroscopio.", fontCardDesc, 496, 222, COLOR_BLUE_BRIGHT);

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

    const int DS4_CARD_X = 410;
    const int DS4_CARD_W = 880;
    const int DS4_CARD_H = 96;
    const int DS4_CARD_SPACING = 14;

    for (int i = 0; i < VISIBLE_CARDS; i++) {
        int cardIdx = i + scrollOffset;
        if (cardIdx >= TOTAL_CARDS) break;

        int cardY = 280 + i * (DS4_CARD_H + DS4_CARD_SPACING);
        bool isSelected = (cardIdx == selectedCard);

        draw_rounded_rect(scene, DS4_CARD_X, cardY, DS4_CARD_W, DS4_CARD_H, 12, isSelected ? Color{ 18, 28, 56 } : COLOR_CARD_BG);

        if (isSelected) {
            draw_rounded_outline(scene, DS4_CARD_X - 1, cardY - 1, DS4_CARD_W + 2, DS4_CARD_H + 2, 12, COLOR_ACCENT_INDIGO, 2);
            draw_rounded_outline(scene, DS4_CARD_X, cardY, DS4_CARD_W, DS4_CARD_H, 12, COLOR_BLUE_BRIGHT, 2);
            draw_rounded_rect(scene, DS4_CARD_X, cardY + 14, 5, DS4_CARD_H - 28, 2, COLOR_BLUE_BRIGHT);
        } else {
            draw_rounded_outline(scene, DS4_CARD_X, cardY, DS4_CARD_W, DS4_CARD_H, 12, COLOR_CARD_BORDER, 1);
        }

        if (cardIcons[cardIdx]) {
            draw_image(scene, *cardIcons[cardIdx], DS4_CARD_X + 20, cardY + 24);
        }

        draw_text(scene, cardTitles[cardIdx], fontCardTitle, DS4_CARD_X + 80, cardY + 38, COLOR_WHITE);
        draw_text(scene, cardDescs[cardIdx], fontCardDesc, DS4_CARD_X + 80, cardY + 70, COLOR_TEXT_DIM);

        int vBoxW = 290;
        int vBoxH = 48;
        int vBoxX = DS4_CARD_X + DS4_CARD_W - vBoxW - 18;
        int vBoxY = cardY + (DS4_CARD_H - vBoxH) / 2;
        draw_rounded_rect(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? Color{ 10, 22, 42 } : Color{ 8, 13, 24 });
        draw_rounded_outline(scene, vBoxX, vBoxY, vBoxW, vBoxH, 8, isSelected ? COLOR_BLUE_BRIGHT : COLOR_BORDER_DIM, 1);

        if (isSelected) {
            draw_text(scene, "<", fontValue, vBoxX + 12, cardY + 54, COLOR_BLUE_BRIGHT);
            draw_text(scene, ">", fontValue, vBoxX + vBoxW - 24, cardY + 54, COLOR_BLUE_BRIGHT);
        }

        if (cardIdx == 1) {
            int c_idx = cfg.ds4_lightbar_color;
            if (c_idx >= 0 && c_idx < 7) {
                draw_circle(scene, vBoxX + 36, cardY + 48, 8, ds4Colors[c_idx]);
                draw_circle_outline(scene, vBoxX + 36, cardY + 48, 8, COLOR_WHITE, 1);
            }
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2 + 8, cardY + 54, COLOR_WHITE);
        } else {
            draw_text_centered(scene, cardValues[cardIdx], fontValue, vBoxX + vBoxW / 2, cardY + 54, COLOR_WHITE);
        }
    }

    // Right-Hand Native DualShock 4 Live Preview & Status Panel
    const int BOX_X = 1310;
    const int BOX_Y = 280;
    const int BOX_W = 540;
    const int BOX_H = 536;

    draw_rounded_rect(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BG);
    draw_rounded_outline(scene, BOX_X, BOX_Y, BOX_W, BOX_H, 12, COLOR_CARD_BORDER, 1);

    draw_image(scene, btnPs, BOX_X + 24, BOX_Y + 22);
    draw_text(scene, "Estado DualShock 4 Nativo", fontCardTitle, BOX_X + 76, BOX_Y + 40, COLOR_WHITE);
    draw_text(scene, "Control Directo vía libScePad (API Oficial PS4)", fontCardDesc, BOX_X + 76, BOX_Y + 62, COLOR_BLUE_BRIGHT);

    draw_rounded_rect(scene, BOX_X + 24, BOX_Y + 76, BOX_W - 48, 1, 0, COLOR_CARD_BORDER);

    // Visual Lightbar Prism Preview
    int activeColorIdx = (cfg.ds4_lightbar_color >= 0 && cfg.ds4_lightbar_color < 7) ? cfg.ds4_lightbar_color : 0;
    Color activeLed = ds4Colors[activeColorIdx];
    static uint32_t s_preview_tick = 0;
    s_preview_tick++;
    if (cfg.ds4_lightbar_mode == 3) {
        activeLed = Color{ 25, 30, 40 };
    } else if (cfg.ds4_lightbar_mode == 2) {
        activeLed = Color{ 16, 185, 129 };
    } else if (cfg.ds4_lightbar_mode == 4) {
        // Dynamic Rainbow preview
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
        // Breathing pulse preview
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
            activeLed.r = (uint8_t)((activeLed.r * 45) / 100);
            activeLed.g = (uint8_t)((activeLed.g * 45) / 100);
            activeLed.b = (uint8_t)((activeLed.b * 45) / 100);
        } else if (cfg.ds4_brightness == 2) {
            activeLed.r = (uint8_t)((activeLed.r * 20) / 100);
            activeLed.g = (uint8_t)((activeLed.g * 20) / 100);
            activeLed.b = (uint8_t)((activeLed.b * 20) / 100);
        }
    }

    draw_text(scene, "Vista Previa de Barra de Luz Frontal:", fontCardTitle, BOX_X + 30, BOX_Y + 112, COLOR_WHITE);
    draw_rounded_rect(scene, BOX_X + 30, BOX_Y + 128, BOX_W - 60, 34, 10, activeLed);
    draw_rounded_outline(scene, BOX_X + 30, BOX_Y + 128, BOX_W - 60, 34, 10, COLOR_WHITE, 1);
    if (cfg.ds4_lightbar_mode == 3) {
        draw_text_centered(scene, "LED APAGADO (0% Brillo - Ahorro Total)", fontCardDesc, BOX_X + BOX_W / 2, BOX_Y + 151, COLOR_TEXT_DIM);
    }

    // Active Slots Status
    draw_text(scene, "Mandos PS4 Detectados en Consola:", fontCardTitle, BOX_X + 30, BOX_Y + 202, COLOR_WHITE);
    for (int s = 0; s < 4; s++) {
        int sy = BOX_Y + 220 + s * 42;
        bool conn = tester.slotConnected[s];
        draw_rounded_rect(scene, BOX_X + 30, sy, BOX_W - 60, 34, 6, conn ? Color{ 14, 32, 54 } : Color{ 10, 16, 26 });
        draw_circle(scene, BOX_X + 50, sy + 17, 5, conn ? Color{ 0, 240, 140 } : Color{ 90, 100, 115 });
        char slotTxt[96];
        snprintf(slotTxt, sizeof(slotTxt), "Jugador %d (Ranura %d): %s", s + 1, s + 1, conn ? "DualShock 4 Activo" : "Sin conexión");
        draw_text(scene, slotTxt, fontCardDesc, BOX_X + 68, sy + 23, conn ? COLOR_WHITE : COLOR_TEXT_DIM);
    }

    // Live Analog, Touchpad & D-Pad Telemetry Mini-Readout
    draw_rounded_rect(scene, BOX_X + 24, BOX_Y + 402, BOX_W - 48, 1, 0, COLOR_CARD_BORDER);
    draw_text(scene, "Lectura Instantánea del Mando Activo:", fontCardTitle, BOX_X + 30, BOX_Y + 430, COLOR_WHITE);
    char stickInfo[160];
    snprintf(stickInfo, sizeof(stickInfo), "L3:(%d,%d) R3:(%d,%d) | L2:%d R2:%d",
             (int)padData.leftStick.x, (int)padData.leftStick.y,
             (int)padData.rightStick.x, (int)padData.rightStick.y,
             (int)padData.analogButtons.l2, (int)padData.analogButtons.r2);
    draw_text(scene, stickInfo, fontCardDesc, BOX_X + 30, BOX_Y + 458, COLOR_BLUE_BRIGHT);

    bool tpClick = (padData.buttons & ORBIS_PAD_BUTTON_TOUCH_PAD) != 0;
    bool anyDpad = (padData.buttons & (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN | ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT)) != 0;
    char tpInfo[160];
    snprintf(tpInfo, sizeof(tpInfo), "Panel Táctil: %s (%d dedos: %d,%d) | Cruceta: %s",
             tpClick ? "CLIC ACTIVO" : "Reposo",
             (int)padData.touch.fingers,
             (int)padData.touch.touch[0].x, (int)padData.touch.touch[0].y,
             anyDpad ? "PULSADA" : "Reposo");
    draw_text(scene, tpInfo, fontCardDesc, BOX_X + 30, BOX_Y + 484, (tpClick || anyDpad) ? Color{ 0, 255, 160 } : COLOR_WHITE);
    draw_text(scene, "Pulsa [CUADRADO] para probar la vibración del DualShock 4.", fontCardDesc, BOX_X + 30, BOX_Y + 510, Color{ 0, 255, 160 });

    // Footer
    scene->DrawRectangle(56, 865, 1808, 2, COLOR_DIVIDER);
    draw_image(scene, iconRumble, 56, 890);
    draw_text(scene, "Módulo Nativo DualShock 4 (PS4)", fontCardTitle, 116, 912, COLOR_WHITE);
    draw_text(scene, "Los cambios de color y brillo se aplican inmediatamente a tu mando PS4", fontFooter, 116, 938, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnSquare, 1160, 897);
    draw_text(scene, "Probar Vibración", fontFooter, 1205, 922, Color{ 0, 255, 150 });

    draw_image(scene, btnArrows, 1370, 901);
    draw_text(scene, "Ajustar", fontFooter, 1424, 922, COLOR_TEXT_DIM);

    draw_image(scene, btnCross, 1540, 897);
    draw_text(scene, "Guardar", fontFooter, 1585, 922, COLOR_BLUE_BRIGHT);

    draw_image(scene, btnOptions, 1690, 900);
    draw_text(scene, "Salir", fontFooter, 1770, 922, COLOR_TEXT_DIM);
}
