#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#define INI_FILE_PATH                  "/data/anycontroller/anycontroller.ini"
#define INI_FILE_PATH_USER             "/user/data/anycontroller/anycontroller.ini"

struct AppConfig {
    int pro_profile_index;  // 0=Personalizado, 1=eSports FPS, 2=SimRacing, 3=Modo Eco, 4=Estandar PS
    int deadzone_index;     // 0..7 (0%, 5%, 8%, 10%, 12%, 15%, 20%, 25%)
    int stick_curve_index;  // 0=Lineal (1:1), 1=Precision Exponencial, 2=Agresiva eSports
    int trigger_index;      // 0..4 (15, 30, 45, 60, 80)
    int rapid_fire_index;   // 0=Desactivado, 1=Turbo R2, 2=Turbo L2 + R2
    int swap_index;         // 0 = Disabled, 1 = Enabled
    int rumble_index;       // 0 = Enabled, 1 = Disabled
    int switch_index;       // 0 = Positional, 1 = By Letter

    // [DualSense] - Exclusivo PS5 DualSense / DualSense Edge
    int ds_bt_mode;         // 0=Safe Mode (10-byte Report 0x01), 1=Extended Mode (78-byte Report 0x31)
    int ds_lightbar_mode;   // 0=Game Sync, 1=Static Color, 2=Battery, 3=Off, 4=Rainbow, 5=Breathing
    int ds_lightbar_color;  // 0=PlayStation Blue, 1=Red, 2=Green, 3=Purple, 4=Cyan, 5=White
    int ds_brightness;      // 0=High, 1=Medium, 2=Low
    int ds_trigger_mode;    // 0=Standard / Off, 1=Hair Trigger, 2=Resistance, 3=Vibration
    int ds_player_leds;     // 0=Player Slot, 1=All On, 2=Off
    int ds_mute_action;     // 0=Default, 1=Touchpad Click
    int ds_speaker_volume;  // 0..100 (Default: 80)

    // [DualShock4] - Exclusivo Mandos Nativos PS4 (CUH-ZCT1 / CUH-ZCT2)
    int ds4_lightbar_mode;  // 0=Por Defecto (Juego), 1=Color Fijo, 2=Bateria, 3=Apagado, 4=Arcoiris, 5=Respiracion
    int ds4_lightbar_color; // 0=Azul PS4, 1=Rojo Carmesi, 2=Verde Esmeralda, 3=Purpura, 4=Cian, 5=Ambar, 6=Blanco
    int ds4_brightness;     // 0=Alto (100%), 1=Medio (50%), 2=Tenue (15%)
    int ds4_rumble_boost;   // 0=Estandar (100%), 1=Suave (60%), 2=Intenso (120%), 3=Desactivado
    int ds4_touchpad_mode;  // 0=Tactil + Clic Completo, 1=Solo Clic Fisico
    int ds4_gyro_deadband;  // 0=Alta Precision (Sin filtro), 1=Filtro Estabilizador, 2=Filtro Intenso

    AppConfig() {
        set_defaults();
    }

    void set_defaults() {
        pro_profile_index = 0;  // Personalizado
        deadzone_index = 4;     // 12% (15/127)
        stick_curve_index = 0;  // Lineal (1:1)
        trigger_index = 1;      // 30 (12%)
        rapid_fire_index = 0;   // Desactivado
        swap_index = 0;         // Disabled
        rumble_index = 0;       // Enabled
        switch_index = 0;       // Positional
        ds_bt_mode = 1;         // Extended Mode (78-byte Report 0x31)
        ds_lightbar_mode = 1;   // Static Color
        ds_lightbar_color = 0;
        ds_brightness = 0;
        ds_trigger_mode = 0;
        ds_player_leds = 0;
        ds_mute_action = 0;
        ds_speaker_volume = 80;

        ds4_lightbar_mode = 1;  // Color Fijo Personalizado
        ds4_lightbar_color = 0; // Azul PS4
        ds4_brightness = 0;     // Alto (100%)
        ds4_rumble_boost = 0;   // Estandar (100%)
        ds4_touchpad_mode = 0;  // Tactil + Clic Completo
        ds4_gyro_deadband = 0;  // Alta Precision
    }

    void apply_pro_profile(int preset) {
        pro_profile_index = preset;
        if (preset == 1) {
            // eSports Competitivo (FPS)
            deadzone_index = 1;      // 5% (ultra sensible)
            stick_curve_index = 2;   // Agresiva eSports
            trigger_index = 0;       // 15 (6% disparo instantaneo)
            rumble_index = 1;        // Vibracion desactivada para maxima punteria
            ds_trigger_mode = 1;     // Disparo Rapido (Click / Hair Trigger)
            ds_lightbar_mode = 1;
            ds_lightbar_color = 4;   // Cian
            ds4_lightbar_mode = 1;
            ds4_lightbar_color = 4;  // Cian Electrico
            ds4_rumble_boost = 3;    // Desactivado
        } else if (preset == 2) {
            // SimRacing / Conduccion
            deadzone_index = 2;      // 8%
            stick_curve_index = 1;   // Precision Suave (Exponencial)
            trigger_index = 1;       // 30 (12%)
            rapid_fire_index = 0;    // Sin turbo para acelerador progresivo
            rumble_index = 0;        // Vibracion activada
            ds_trigger_mode = 2;     // Resistencia Continua en pedales L2/R2
            ds_lightbar_mode = 1;
            ds_lightbar_color = 1;   // Rojo
            ds4_lightbar_mode = 1;
            ds4_lightbar_color = 1;  // Rojo Carmesi
            ds4_rumble_boost = 2;    // Intenso (120%)
        } else if (preset == 3) {
            // Modo Eco (Ahorro Extremo de Bateria)
            deadzone_index = 4;      // 12%
            stick_curve_index = 0;   // Lineal
            trigger_index = 1;       // 30 (12%)
            rapid_fire_index = 0;
            rumble_index = 1;        // Motores apagados para ahorrar bateria
            ds_trigger_mode = 0;     // Motores de gatillos apagados
            ds_lightbar_mode = 3;    // LED apagado
            ds_brightness = 2;       // Bajo
            ds_player_leds = 2;      // LEDs inferiores apagados
            ds4_lightbar_mode = 3;   // LED frontal apagado
            ds4_brightness = 2;      // Tenue
            ds4_rumble_boost = 3;    // Desactivado
        } else if (preset == 4) {
            // Estandar PlayStation Oficial
            set_defaults();
            pro_profile_index = 4;
        }
    }
};

class ConfigManager {
public:
    static const std::vector<std::string> PRO_PROFILE_OPTIONS;
    static const std::vector<std::string> DEADZONE_OPTIONS;
    static const std::vector<int>         DEADZONE_VALUES;
    static const std::vector<std::string> STICK_CURVE_OPTIONS;
    static const std::vector<std::string> TRIGGER_OPTIONS;
    static const std::vector<int>         TRIGGER_VALUES;
    static const std::vector<std::string> RAPID_FIRE_OPTIONS;
    static const std::vector<std::string> SWAP_OPTIONS;
    static const std::vector<std::string> RUMBLE_OPTIONS;
    static const std::vector<std::string> SWITCH_OPTIONS;

    // DualSense Options (PS5)
    static const std::vector<std::string> DS_BT_MODE_OPTIONS;
    static const std::vector<std::string> DS_LIGHTBAR_OPTIONS;
    static const std::vector<std::string> DS_COLOR_OPTIONS;
    static const std::vector<std::string> DS_BRIGHTNESS_OPTIONS;
    static const std::vector<std::string> DS_TRIGGER_OPTIONS;
    static const std::vector<std::string> DS_PLAYER_LEDS_OPTIONS;
    static const std::vector<std::string> DS_MUTE_OPTIONS;

    // DualShock 4 Options (PS4 Nativo)
    static const std::vector<std::string> DS4_LIGHTBAR_OPTIONS;
    static const std::vector<std::string> DS4_COLOR_OPTIONS;
    static const std::vector<std::string> DS4_BRIGHTNESS_OPTIONS;
    static const std::vector<std::string> DS4_RUMBLE_BOOST_OPTIONS;
    static const std::vector<std::string> DS4_TOUCHPAD_OPTIONS;
    static const std::vector<std::string> DS4_GYRO_OPTIONS;

    ConfigManager();
    bool load();
    bool save();

    AppConfig& getConfig() { return config; }

private:
    AppConfig config;
    bool file_exists(const char* path);
};
