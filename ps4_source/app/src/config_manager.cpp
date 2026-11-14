#include "config_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <orbis/libkernel.h>

const std::vector<std::string> ConfigManager::PRO_PROFILE_OPTIONS = {
    "Personalizado", "eSports FPS", "SimRacing", "Modo Eco", "Estándar PS4/PS5"
};

const std::vector<std::string> ConfigManager::DEADZONE_OPTIONS = {
    "0%", "5%", "8%", "10%", "12%", "15%", "20%", "25%"
};

const std::vector<int> ConfigManager::DEADZONE_VALUES = {
    0, 6, 10, 13, 15, 19, 25, 32
};

const std::vector<std::string> ConfigManager::STICK_CURVE_OPTIONS = {
    "Lineal (1:1)", "Precisión Exponencial", "Agresiva eSports"
};

const std::vector<std::string> ConfigManager::SWAP_OPTIONS = {
    "Desactivado", "Activado"
};

const std::vector<std::string> ConfigManager::RUMBLE_OPTIONS = {
    "Activado", "Desactivado"
};

const std::vector<std::string> ConfigManager::TRIGGER_OPTIONS = {
    "15 (6%)", "30 (12%)", "45 (18%)", "60 (24%)", "80 (31%)"
};

const std::vector<int> ConfigManager::TRIGGER_VALUES = {
    15, 30, 45, 60, 80
};

const std::vector<std::string> ConfigManager::RAPID_FIRE_OPTIONS = {
    "Desactivado", "Turbo R2", "Turbo Dual L2+R2"
};

const std::vector<std::string> ConfigManager::SWITCH_OPTIONS = {
    "Por Posición", "Por Letra"
};

const std::vector<std::string> ConfigManager::DS_BT_MODE_OPTIONS = {
    "Modo Seguro (10B)", "Modo Extendido (78B)"
};

const std::vector<std::string> ConfigManager::DS_LIGHTBAR_OPTIONS = {
    "Sincronizar Juego", "Color Estático", "Estado Batería", "Apagado", "Arcoíris Dinámico", "Pulso Respiración"
};

const std::vector<std::string> ConfigManager::DS_COLOR_OPTIONS = {
    "Azul PlayStation", "Rojo", "Verde", "Púrpura", "Cian", "Blanco"
};

const std::vector<std::string> ConfigManager::DS_BRIGHTNESS_OPTIONS = {
    "Alto (100%)", "Medio (50%)", "Bajo (25%)"
};

const std::vector<std::string> ConfigManager::DS_TRIGGER_OPTIONS = {
    "Estándar / Off", "Disparo Rápido (Click)", "Resistencia Continua", "Vibración / Pulso"
};

const std::vector<std::string> ConfigManager::DS_PLAYER_LEDS_OPTIONS = {
    "Ranura Jugador", "Todos Encendidos", "Apagados"
};

const std::vector<std::string> ConfigManager::DS_MUTE_OPTIONS = {
    "Por Defecto", "Clic Touchpad"
};

const std::vector<std::string> ConfigManager::DS4_LIGHTBAR_OPTIONS = {
    "Control del Juego", "Color Personalizado", "Nivel de Batería", "Apagado (Ahorro)", "Arcoíris Dinámico", "Pulso Respiración"
};

const std::vector<std::string> ConfigManager::DS4_COLOR_OPTIONS = {
    "Azul PS4", "Rojo Carmesí", "Verde Esmeralda", "Púrpura Neón", "Cian Eléctrico", "Ámbar Oro", "Blanco Puro"
};

const std::vector<std::string> ConfigManager::DS4_BRIGHTNESS_OPTIONS = {
    "Alto (100%)", "Medio (50%)", "Tenue (15%)"
};

const std::vector<std::string> ConfigManager::DS4_RUMBLE_BOOST_OPTIONS = {
    "Estándar (100%)", "Suave (60%)", "Intenso (120%)", "Desactivado"
};

const std::vector<std::string> ConfigManager::DS4_TOUCHPAD_OPTIONS = {
    "Táctil + Clic Completo", "Solo Clic Físico"
};

const std::vector<std::string> ConfigManager::DS4_GYRO_OPTIONS = {
    "Alta Precisión", "Filtro Estabilizador", "Filtro Intenso"
};

ConfigManager::ConfigManager() {
    config.set_defaults();
}

bool ConfigManager::file_exists(const char* path) {
    int fd = sceKernelOpen(path, O_RDONLY, 0);
    if (fd >= 0) {
        sceKernelClose(fd);
        return true;
    }
    return false;
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool ConfigManager::load() {
    const char* path_to_load = INI_FILE_PATH;
    if (!file_exists(INI_FILE_PATH)) {
        if (file_exists(INI_FILE_PATH_USER)) {
            path_to_load = INI_FILE_PATH_USER;
        } else {
            // First run: file does not exist yet! Create it with defaults.
            config.set_defaults();
            save();
            return true;
        }
    }

    // File exists: load and parse current values
    config.set_defaults();

    int fd = sceKernelOpen(path_to_load, O_RDONLY, 0);
    if (fd < 0) {
        return false;
    }

    char buf[4096];
    memset(buf, 0, sizeof(buf));
    ssize_t bytes_read = (ssize_t)sceKernelRead(fd, buf, sizeof(buf) - 1);
    sceKernelClose(fd);

    if (bytes_read <= 0) {
        return false;
    }
    buf[bytes_read] = '\0';

    char* line = strtok(buf, "\r\n");
    while (line != NULL) {
        std::string s = trim(line);
        if (!s.empty() && s[0] != '#' && s[0] != ';' && s[0] != '[') {
            size_t eq = s.find('=');
            if (eq != std::string::npos) {
                std::string key = trim(s.substr(0, eq));
                std::string val = trim(s.substr(eq + 1));

                size_t cpos = val.find('#');
                if (cpos == std::string::npos) cpos = val.find(';');
                if (cpos != std::string::npos) val = trim(val.substr(0, cpos));

                if (strcasecmp(key.c_str(), "pro_profile") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)PRO_PROFILE_OPTIONS.size()) config.pro_profile_index = v;
                } else if (strcasecmp(key.c_str(), "stick_deadzone") == 0 || strcasecmp(key.c_str(), "deadzone") == 0) {
                    int dz = atoi(val.c_str());
                    int best_idx = 4; // default 12%
                    int min_diff = 999;
                    for (size_t i = 0; i < DEADZONE_VALUES.size(); i++) {
                        int diff = abs(DEADZONE_VALUES[i] - dz);
                        if (diff < min_diff) {
                            min_diff = diff;
                            best_idx = (int)i;
                        }
                    }
                    config.deadzone_index = best_idx;
                } else if (strcasecmp(key.c_str(), "stick_curve") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)STICK_CURVE_OPTIONS.size()) config.stick_curve_index = v;
                } else if (strcasecmp(key.c_str(), "swap_triggers_bumpers") == 0) {
                    int v = atoi(val.c_str());
                    config.swap_index = (v != 0) ? 1 : 0;
                } else if (strcasecmp(key.c_str(), "enable_rumble") == 0 || strcasecmp(key.c_str(), "rumble") == 0) {
                    int v = atoi(val.c_str());
                    config.rumble_index = (v != 0) ? 0 : 1; // 0=Enabled, 1=Disabled
                } else if (strcasecmp(key.c_str(), "trigger_threshold") == 0) {
                    int th = atoi(val.c_str());
                    int best_idx = 1; // default 30
                    int min_diff = 999;
                    for (size_t i = 0; i < TRIGGER_VALUES.size(); i++) {
                        int diff = abs(TRIGGER_VALUES[i] - th);
                        if (diff < min_diff) {
                            min_diff = diff;
                            best_idx = (int)i;
                        }
                    }
                    config.trigger_index = best_idx;
                } else if (strcasecmp(key.c_str(), "rapid_fire") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)RAPID_FIRE_OPTIONS.size()) config.rapid_fire_index = v;
                } else if (strcasecmp(key.c_str(), "switch_positional_layout") == 0) {
                    int v = atoi(val.c_str());
                    config.switch_index = (v != 0) ? 0 : 1; // 0=Positional (1), 1=By Letter (0)
                } else if (strcasecmp(key.c_str(), "dualsense_bt_mode") == 0 || strcasecmp(key.c_str(), "bt_mode") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS_BT_MODE_OPTIONS.size()) config.ds_bt_mode = v;
                } else if (strcasecmp(key.c_str(), "dualsense_lightbar_mode") == 0 || strcasecmp(key.c_str(), "lightbar_mode") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS_LIGHTBAR_OPTIONS.size()) config.ds_lightbar_mode = v;
                } else if (strcasecmp(key.c_str(), "dualsense_lightbar_color") == 0 || strcasecmp(key.c_str(), "lightbar_color") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS_COLOR_OPTIONS.size()) config.ds_lightbar_color = v;
                } else if (strcasecmp(key.c_str(), "dualsense_brightness") == 0 || strcasecmp(key.c_str(), "brightness") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS_BRIGHTNESS_OPTIONS.size()) config.ds_brightness = v;
                } else if (strcasecmp(key.c_str(), "dualsense_trigger_mode") == 0 || strcasecmp(key.c_str(), "trigger_mode") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS_TRIGGER_OPTIONS.size()) config.ds_trigger_mode = v;
                } else if (strcasecmp(key.c_str(), "dualsense_player_leds") == 0 || strcasecmp(key.c_str(), "player_leds") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS_PLAYER_LEDS_OPTIONS.size()) config.ds_player_leds = v;
                } else if (strcasecmp(key.c_str(), "dualsense_mute_action") == 0 || strcasecmp(key.c_str(), "mute_action") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS_MUTE_OPTIONS.size()) config.ds_mute_action = v;
                } else if (strcasecmp(key.c_str(), "dualsense_speaker_volume") == 0 || strcasecmp(key.c_str(), "speaker_volume") == 0) {
                    int v = atoi(val.c_str());
                    if (v < 0) v = 0;
                    if (v > 100) v = 100;
                    config.ds_speaker_volume = v;
                } else if (strcasecmp(key.c_str(), "ds4_lightbar_mode") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS4_LIGHTBAR_OPTIONS.size()) config.ds4_lightbar_mode = v;
                } else if (strcasecmp(key.c_str(), "ds4_lightbar_color") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS4_COLOR_OPTIONS.size()) config.ds4_lightbar_color = v;
                } else if (strcasecmp(key.c_str(), "ds4_brightness") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS4_BRIGHTNESS_OPTIONS.size()) config.ds4_brightness = v;
                } else if (strcasecmp(key.c_str(), "ds4_rumble_boost") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS4_RUMBLE_BOOST_OPTIONS.size()) config.ds4_rumble_boost = v;
                } else if (strcasecmp(key.c_str(), "ds4_touchpad_mode") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS4_TOUCHPAD_OPTIONS.size()) config.ds4_touchpad_mode = v;
                } else if (strcasecmp(key.c_str(), "ds4_gyro_deadband") == 0) {
                    int v = atoi(val.c_str());
                    if (v >= 0 && v < (int)DS4_GYRO_OPTIONS.size()) config.ds4_gyro_deadband = v;
                }
            }
        }
        line = strtok(NULL, "\r\n");
    }

    return true;
}

bool ConfigManager::save() {
    mkdir("/data", 0777);
    mkdir("/data/anycontroller", 0777);
    mkdir("/user", 0777);
    mkdir("/user/data", 0777);
    mkdir("/user/data/anycontroller", 0777);

    char out[2560];
    int deadzone_val = DEADZONE_VALUES[config.deadzone_index];
    int trigger_val = TRIGGER_VALUES[config.trigger_index];
    int swap_val = (config.swap_index != 0) ? 1 : 0;
    int rumble_val = (config.rumble_index == 0) ? 1 : 0;
    int switch_val = (config.switch_index == 0) ? 1 : 0;

    int len = snprintf(out, sizeof(out),
        "# ANYCONTROLLER Daemon - Archivo de Configuracion\n"
        "# Generado por Controller Control Center PS4 (v1.3.9)\n"
        "# Ubicacion en PS4: /data/anycontroller/anycontroller.ini\n"
        "\n"
        "[Deadzone]\n"
        "stick_deadzone = %d\n"
        "stick_curve = %d\n"
        "trigger_threshold = %d\n"
        "\n"
        "[General]\n"
        "mode = 0\n"
        "pro_profile = %d\n"
        "enable_rumble = %d\n"
        "rapid_fire = %d\n"
        "\n"
        "[Remap]\n"
        "swap_triggers_bumpers = %d\n"
        "switch_positional_layout = %d\n"
        "\n"
        "[DualSense]\n"
        "dualsense_bt_mode = %d\n"
        "dualsense_lightbar_mode = %d\n"
        "dualsense_lightbar_color = %d\n"
        "dualsense_brightness = %d\n"
        "dualsense_trigger_mode = %d\n"
        "dualsense_player_leds = %d\n"
        "dualsense_mute_action = %d\n"
        "dualsense_speaker_volume = %d\n"
        "\n"
        "[DualShock4]\n"
        "ds4_lightbar_mode = %d\n"
        "ds4_lightbar_color = %d\n"
        "ds4_brightness = %d\n"
        "ds4_rumble_boost = %d\n"
        "ds4_touchpad_mode = %d\n"
        "ds4_gyro_deadband = %d\n",
        deadzone_val,
        config.stick_curve_index,
        trigger_val,
        config.pro_profile_index,
        rumble_val,
        config.rapid_fire_index,
        swap_val,
        switch_val,
        config.ds_bt_mode,
        config.ds_lightbar_mode,
        config.ds_lightbar_color,
        config.ds_brightness,
        config.ds_trigger_mode,
        config.ds_player_leds,
        config.ds_mute_action,
        config.ds_speaker_volume,
        config.ds4_lightbar_mode,
        config.ds4_lightbar_color,
        config.ds4_brightness,
        config.ds4_rumble_boost,
        config.ds4_touchpad_mode,
        config.ds4_gyro_deadband
    );

    const char* target_paths[2] = {
        INI_FILE_PATH,
        INI_FILE_PATH_USER
    };

    bool any_written = false;
    for (int i = 0; i < 2; i++) {
        int fd = sceKernelOpen(target_paths[i], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd >= 0) {
            if (len > 0) {
                ssize_t wr = sceKernelWrite(fd, out, (size_t)len);
                if (wr == len) any_written = true;
            }
            sceKernelClose(fd);
        }
    }

    return any_written;
}
