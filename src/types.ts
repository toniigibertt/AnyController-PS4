/**
 * OneFamily PS4 - Modelos y Definiciones de Tipos en Español
 */

export interface AppConfig {
  pro_profile_index: number;   // 0 = Personalizado, 1 = eSports FPS, 2 = SimRacing, 3 = Modo Eco, 4 = Estándar PS
  deadzone_index: number;      // 0..7 (0%, 5%, 8%, 10%, 12%, 15%, 20%, 25%)
  stick_curve_index: number;   // 0 = Lineal (1:1), 1 = Precisión Exponencial, 2 = Agresiva eSports
  trigger_index: number;       // 0..4 (15, 30, 45, 60, 80)
  rapid_fire_index: number;    // 0 = Desactivado, 1 = Turbo R2, 2 = Turbo Dual L2 + R2
  swap_index: number;          // 0 = Desactivado, 1 = Activado (L1/L2 y R1/R2)
  rumble_index: number;        // 0 = Activado, 1 = Desactivado
  switch_index: number;        // 0 = Por Posición, 1 = Por Letra
  back_button_index: number;   // 0 = Share / Create, 1 = Clic de Touchpad (Fix Issue #68)
  guide_action: number;        // 0 = Botón PS, 1 = Clic de Touchpad
  enable_notifications: number;// 0 = Desactivado, 1 = Activado

  // Opciones Exclusivas DualSense PS5
  ds_bt_mode: number;          // 0 = Modo Seguro (10B Report 0x01), 1 = Modo Extendido (78B Report 0x31)
  ds_lightbar_mode: number;    // 0..5 (Juego, Estático, Batería, Apagado, Arcoíris Dinámico, Pulso Respiración)
  ds_lightbar_color: number;   // 0 = Azul PS, 1 = Rojo, 2 = Verde, 3 = Púrpura, 4 = Cian, 5 = Blanco
  ds_brightness: number;       // 0 = Alto (100%), 1 = Medio (50%), 2 = Bajo (25%)
  ds_trigger_mode: number;     // 0 = Estándar/Apagado, 1 = Disparo Rápido, 2 = Resistencia, 3 = Vibración
  ds_player_leds: number;      // 0 = Ranura de Jugador (1-4), 1 = Todos Encendidos, 2 = Apagados
  ds_mute_action: number;      // 0 = Silenciar Micrófono, 1 = Clic de Touchpad
  ds_speaker_volume: number;   // 0..100 (Predeterminado: 80)

  // Opciones Exclusivas DualShock 4 Nativo (PS4)
  ds4_lightbar_mode: number;   // 0 = Control del Juego, 1 = Color Personalizado, 2 = Nivel de Batería, 3 = Apagado (Ahorro)
  ds4_lightbar_color: number;  // 0 = Azul PS4, 1 = Rojo Carmesí, 2 = Verde Esmeralda, 3 = Púrpura Neón, 4 = Cian Eléctrico, 5 = Ámbar Oro, 6 = Blanco Puro
  ds4_brightness: number;      // 0 = Alto (100%), 1 = Medio (50%), 2 = Tenue (15%)
  ds4_rumble_boost: number;    // 0 = Estándar (100%), 1 = Suave (60%), 2 = Intenso (120%), 3 = Desactivado
  ds4_touchpad_mode: number;   // 0 = Táctil + Clic Completo, 1 = Solo Clic Físico
  ds4_gyro_deadband: number;   // 0 = Alta Precisión, 1 = Filtro Estabilizador, 2 = Filtro Intenso
}

export type TabId = 'settings' | 'dualsense' | 'dualshock4' | 'tester' | 'remap' | 'info';

export interface CalibrationState {
  active: boolean;
  completed: boolean;
  currentFrame: number;
  totalFrames: number;
  maxNoise: number;
  suggestedIndex: number;
}

export interface TesterState {
  rumbleFrames: number;
  rumblePhase: number;        // 0 = apagado, 1 = pesado, 2 = ligero, 3 = ambos
  selectedSlot: number;       // 0..3 (Jugador 1..4)
  testMode: boolean;          // modo aislado para no cambiar de pestaña accidentalmente
  calib: CalibrationState;
}

export interface DualSenseTelemetry {
  connected: boolean;
  connectionType: 'bluetooth' | 'usb';
  batteryPercent: number;
  isCharging: boolean;
  firmwareVersion: string;
  reportMode: '0x01' | '0x31';
  touchpadActive: boolean;
  micMuted: boolean;
  temperature?: number;
}

export interface Ds4ActionMeta {
  id: string;
  key_name: string;
  display_name: string;
  short_name: string;
  icon_path: string;
  description: string;
  category: 'action' | 'shoulder' | 'system' | 'stick';
}

export interface ControllerProfile {
  id: string;
  name: string;
  vendorId: string;
  productId: string;
  category: 'playstation' | 'xbox' | 'nintendo' | 'stadia' | 'generic' | 'arcade' | 'retro';
  assignments: Record<string, number>; // action key -> button number (1..32)
  invertLy: boolean;
  invertRy: boolean;
  rumblePreset: string;
  axes: {
    lx: string;
    ly: string;
    rx: string;
    ry: string;
    l2: string;
    r2: string;
  };
  notes?: string;
}

export interface GamepadBatteryInfo {
  available: boolean;
  level: number; // 0..100
  charging: boolean;
  source: 'api' | 'estimated' | 'usb';
  connectionType: 'bluetooth' | 'usb';
  voltageEstimate?: string;
  statusText?: string;
}

export interface LiveGamepadInput {
  connected: boolean;
  id: string;
  slot: number;
  lx: number;      // -1.0 to 1.0 (or 0..255)
  ly: number;
  rx: number;
  ry: number;
  l2: number;      // 0.0 to 1.0
  r2: number;
  buttons: Record<string, boolean>; // e.g. cross, circle, l1, etc.
  rawButtons: boolean[];
  battery?: GamepadBatteryInfo;
}
