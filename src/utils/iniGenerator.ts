/**
 * AnyController PS4 - Generador y Analizador de Archivos de Configuración INI
 * Compatible 1:1 con el daemon en C de PlayStation 4 (/data/anycontroller/anycontroller.ini)
 */
import { AppConfig, ControllerProfile } from '../types';
import { DEADZONE_VALUES, TRIGGER_VALUES } from '../constants/options';

export function generateOneFamilyIni(cfg: AppConfig): string {
  const deadzoneVal = DEADZONE_VALUES[cfg.deadzone_index] ?? 15;
  const triggerVal = TRIGGER_VALUES[cfg.trigger_index] ?? 30;
  const swapVal = cfg.swap_index !== 0 ? 1 : 0;
  const rumbleVal = cfg.rumble_index === 0 ? 1 : 0;
  const switchVal = cfg.switch_index === 0 ? 1 : 0;
  const backActionVal = cfg.back_button_index === 1 ? 'touchpad' : 'share';
  const guideActionVal = cfg.guide_action === 1 ? 'touchpad' : 'ps';

  return `# ==============================================================================
# ANYCONTROLLER Daemon - Archivo de Configuración del Sistema (v1.3.9)
# Generado por Controller Control Center PS4 (Title ID: TONI88888)
# Ubicación en PS4: /data/anycontroller/anycontroller.ini
# ==============================================================================

[General]
pro_profile = ${cfg.pro_profile_index ?? 0}
enable_rumble = ${rumbleVal}
enable_notifications = ${cfg.enable_notifications}
guide_action = ${guideActionVal}
enable_mode_combo = 1
enable_homebrew = 1

[Deadzone]
stick_deadzone = ${deadzoneVal}
stick_curve = ${cfg.stick_curve_index ?? 0}
trigger_threshold = ${triggerVal}
rapid_fire = ${cfg.rapid_fire_index ?? 0}

[Remap]
swap_triggers_bumpers = ${swapVal}
switch_positional_layout = ${switchVal}
back_button_action = ${backActionVal}

[DualSense]
dualsense_bt_mode = ${cfg.ds_bt_mode}
dualsense_lightbar_mode = ${cfg.ds_lightbar_mode}
dualsense_lightbar_color = ${cfg.ds_lightbar_color}
dualsense_brightness = ${cfg.ds_brightness}
dualsense_trigger_mode = ${cfg.ds_trigger_mode}
dualsense_player_leds = ${cfg.ds_player_leds}
dualsense_mute_action = ${cfg.ds_mute_action}
dualsense_speaker_volume = ${cfg.ds_speaker_volume}

[DualShock4]
ds4_lightbar_mode = ${cfg.ds4_lightbar_mode}
ds4_lightbar_color = ${cfg.ds4_lightbar_color}
ds4_brightness = ${cfg.ds4_brightness}
ds4_rumble_boost = ${cfg.ds4_rumble_boost}
ds4_touchpad_mode = ${cfg.ds4_touchpad_mode}
ds4_gyro_deadband = ${cfg.ds4_gyro_deadband}
`;
}

export function generateControllerIni(profile: ControllerProfile): string {
  const vid = profile.vendorId.replace('0x', '').toUpperCase().padStart(4, '0');
  const pid = profile.productId.replace('0x', '').toUpperCase().padStart(4, '0');

  let buttonsSection = '';
  // Map buttons 1 to 16 (or up to 32)
  for (let b = 1; b <= 16; b++) {
    let target = 'none';
    for (const [actionKey, btnNum] of Object.entries(profile.assignments)) {
      if (btnNum === b) {
        target = actionKey;
        break;
      }
    }
    buttonsSection += `button${b} = ${target}\n`;
  }

  return `# ==============================================================================
# AnyController - Perfil de Mando Personalizado
# Archivo: /data/anycontroller/controllers/${vid}_${pid}.ini
# ==============================================================================

[Info]
name = ${profile.name}
vendor_id = 0x${vid}
product_id = 0x${pid}

[Buttons]
${buttonsSection.trim()}

[Axes]
lx = ${profile.axes.lx || 'x'}
ly = ${profile.axes.ly || 'y'}
rx = ${profile.axes.rx || 'z'}
ry = ${profile.axes.ry || 'rz'}
l2 = ${profile.axes.l2 || 'rx'}
r2 = ${profile.axes.r2 || 'ry'}

[Invert]
invert_ly = ${profile.invertLy ? 1 : 0}
invert_ry = ${profile.invertRy ? 1 : 0}

[Rumble]
# Presets admitidos: auto, logitech, dragonrise, greenasia, betop, shanwan, 8bitdo, dinput05, none
rumble = ${profile.rumblePreset || 'auto'}
`;
}

export function parseOneFamilyIni(content: string, baseConfig: AppConfig): AppConfig {
  const config = { ...baseConfig };
  const lines = content.split(/\r?\n/);

  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#') || trimmed.startsWith(';') || trimmed.startsWith('[')) {
      continue;
    }

    const eqIdx = trimmed.indexOf('=');
    if (eqIdx === -1) continue;

    const key = trimmed.slice(0, eqIdx).trim().toLowerCase();
    const rawVal = trimmed.slice(eqIdx + 1).split(/[#;]/)[0].trim().toLowerCase();

    if (key === 'pro_profile') {
      config.pro_profile_index = Math.max(0, Math.min(4, parseInt(rawVal, 10) || 0));
    } else if (key === 'stick_deadzone') {
      const val = parseInt(rawVal, 10);
      const idx = DEADZONE_VALUES.findIndex((v) => v >= val);
      if (idx !== -1) config.deadzone_index = idx;
    } else if (key === 'stick_curve') {
      config.stick_curve_index = Math.max(0, Math.min(2, parseInt(rawVal, 10) || 0));
    } else if (key === 'trigger_threshold') {
      const val = parseInt(rawVal, 10);
      const idx = TRIGGER_VALUES.findIndex((v) => v >= val);
      if (idx !== -1) config.trigger_index = idx;
    } else if (key === 'rapid_fire') {
      config.rapid_fire_index = Math.max(0, Math.min(2, parseInt(rawVal, 10) || 0));
    } else if (key === 'enable_rumble') {
      config.rumble_index = rawVal === '0' || rawVal === 'false' ? 1 : 0;
    } else if (key === 'swap_triggers_bumpers') {
      config.swap_index = rawVal === '1' || rawVal === 'true' ? 1 : 0;
    } else if (key === 'switch_positional_layout') {
      config.switch_index = rawVal === '0' || rawVal === 'false' ? 1 : 0;
    } else if (key === 'back_button_action' || key === 'view_action') {
      config.back_button_index = rawVal === 'touchpad' || rawVal === '1' ? 1 : 0;
    } else if (key === 'guide_action') {
      config.guide_action = rawVal === 'touchpad' || rawVal === '1' ? 1 : 0;
    } else if (key === 'enable_notifications') {
      config.enable_notifications = rawVal === '0' || rawVal === 'false' ? 0 : 1;
    } else if (key === 'dualsense_bt_mode') {
      config.ds_bt_mode = parseInt(rawVal, 10) || 0;
    } else if (key === 'dualsense_lightbar_mode') {
      config.ds_lightbar_mode = parseInt(rawVal, 10) || 0;
    } else if (key === 'dualsense_lightbar_color') {
      config.ds_lightbar_color = parseInt(rawVal, 10) || 0;
    } else if (key === 'dualsense_brightness') {
      config.ds_brightness = parseInt(rawVal, 10) || 0;
    } else if (key === 'dualsense_trigger_mode') {
      config.ds_trigger_mode = parseInt(rawVal, 10) || 0;
    } else if (key === 'dualsense_player_leds') {
      config.ds_player_leds = parseInt(rawVal, 10) || 0;
    } else if (key === 'dualsense_mute_action') {
      config.ds_mute_action = parseInt(rawVal, 10) || 0;
    } else if (key === 'dualsense_speaker_volume') {
      config.ds_speaker_volume = Math.max(0, Math.min(100, parseInt(rawVal, 10) || 80));
    } else if (key === 'ds4_lightbar_mode') {
      config.ds4_lightbar_mode = parseInt(rawVal, 10) || 0;
    } else if (key === 'ds4_lightbar_color') {
      config.ds4_lightbar_color = parseInt(rawVal, 10) || 0;
    } else if (key === 'ds4_brightness') {
      config.ds4_brightness = parseInt(rawVal, 10) || 0;
    } else if (key === 'ds4_rumble_boost') {
      config.ds4_rumble_boost = parseInt(rawVal, 10) || 0;
    } else if (key === 'ds4_touchpad_mode') {
      config.ds4_touchpad_mode = parseInt(rawVal, 10) || 0;
    } else if (key === 'ds4_gyro_deadband') {
      config.ds4_gyro_deadband = parseInt(rawVal, 10) || 0;
    }
  }

  return config;
}

export function downloadFile(filename: string, text: string) {
  const element = document.createElement('a');
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
  element.setAttribute('download', filename);
  element.style.display = 'none';
  document.body.appendChild(element);
  element.click();
  document.body.removeChild(element);
}

export async function downloadBinaryFile(url: string, filename: string): Promise<boolean> {
  try {
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const blob = await res.blob();
    const blobUrl = URL.createObjectURL(blob);
    const element = document.createElement('a');
    element.href = blobUrl;
    element.download = filename;
    element.style.display = 'none';
    document.body.appendChild(element);
    element.click();
    document.body.removeChild(element);
    setTimeout(() => URL.revokeObjectURL(blobUrl), 2000);
    return true;
  } catch {
    const element = document.createElement('a');
    element.href = url;
    element.download = filename;
    element.target = '_blank';
    element.style.display = 'none';
    document.body.appendChild(element);
    element.click();
    document.body.removeChild(element);
    return false;
  }
}
