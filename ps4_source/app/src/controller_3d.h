#pragma once

#include "common/graphics.h"
#include "config_manager.h"
#include <orbis/Pad.h>
#include <stdint.h>

class Controller3DRenderer {
public:
    Controller3DRenderer();

    void update(float dt, uint32_t buttons, int rumblePhase);
    void render(Scene2D* scene, int vx, int vy, int vw, int vh,
                bool isDualSense, const AppConfig& cfg,
                const OrbisPadData& padData, int rumblePhase);

private:
    float m_time;
    float m_pulse[18];      // expanding shockwave ring intensity per button (0..1)
    uint32_t m_prevButtons;
};
