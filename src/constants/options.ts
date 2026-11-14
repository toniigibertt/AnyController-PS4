/**
 * OneFamily PS4 - Constantes y Opciones Configurables en Español
 */
import { AppConfig, Ds4ActionMeta } from '../types';

export const DEFAULT_CONFIG: AppConfig = {
  pro_profile_index: 0,   // Personalizado
  deadzone_index: 4,      // 12% (15/127)
  stick_curve_index: 0,   // Lineal (1:1)
  trigger_index: 1,       // 30 (12%)
  rapid_fire_index: 0,    // Desactivado
  swap_index: 0,          // Desactivado
  rumble_index: 0,        // Activado
  switch_index: 0,        // Por Posición
  back_button_index: 1,   // Clic de Touchpad (Fix Issue #68 para PC/Xbox)
  guide_action: 0,        // Botón PS nativo
  enable_notifications: 1,// Notificaciones activadas
  ds_bt_mode: 1,          // Modo Extendido 78B
  ds_lightbar_mode: 1,    // Color Estático Personalizado
  ds_lightbar_color: 0,   // Azul PlayStation
  ds_brightness: 0,       // Alto
  ds_trigger_mode: 0,     // Estándar
  ds_player_leds: 0,      // Ranura de jugador
  ds_mute_action: 0,      // Por defecto
  ds_speaker_volume: 80,  // 80%
  ds4_lightbar_mode: 1,   // Color Personalizado
  ds4_lightbar_color: 0,  // Azul PS4
  ds4_brightness: 0,      // Alto (100%)
  ds4_rumble_boost: 0,    // Estándar (100%)
  ds4_touchpad_mode: 0,   // Táctil + Clic Completo
  ds4_gyro_deadband: 0,   // Alta Precisión
};

export const PRO_PROFILE_OPTIONS = [
  'Personalizado (Ajustes manuales libres)',
  'eSports Competitivo FPS (Zona muerta 5%, Curva Agresiva, Gatillo Hair Trigger 15)',
  'SimRacing / Conducción (Curva Exponencial Suave, Gatillo Progresivo 80, Resistencia L2/R2)',
  'Modo Eco / Ahorro Extremo (LEDs apagados y vibración reducida para doblar batería)',
  'Estándar PlayStation (Valores equilibrados de fábrica)',
];

export const STICK_CURVE_OPTIONS = [
  'Lineal 1:1 (Respuesta directa original sin aceleración artificial)',
  'Precisión Exponencial (Micro-apuntado suave en el centro para Francotirador / Conducción)',
  'Agresiva eSports (Giro rápido instantáneo para combates cercanos y Battle Royale)',
];

export const RAPID_FIRE_OPTIONS = [
  'Desactivado (Disparo normal según pulsación física)',
  'Turbo R2 Activo (Fuego rápido automático a 15 disparos/seg al mantener R2)',
  'Turbo Dual L2 + R2 (Pulsación rápida automática en ambos gatillos)',
];

export const DEADZONE_OPTIONS = [
  '0% (Sin zona muerta - Rango analógico puro)',
  '5% (Mínima - Gran reactividad competitiva)',
  '8% (Baja - Sensibilidad fina)',
  '10% (Recomendada - Balance ideal)',
  '12% (Predeterminada AnyController - Estable)',
  '15% (Anti-Deriva media - Palancas con desgaste)',
  '20% (Anti-Deriva alta - Gran holgura de centrado)',
  '25% (Máxima - Elimina deriva severa en sticks viejos)',
];

export const DEADZONE_VALUES = [0, 6, 10, 12, 15, 19, 25, 31];

export const TRIGGER_OPTIONS = [
  '15 (6% - Hair Trigger instantáneo para disparos rápidos)',
  '30 (12% - Predeterminado AnyController para respuesta ágil)',
  '45 (18% - Sensibilidad media estándar)',
  '60 (24% - Rango progresivo amplio)',
  '80 (31% - Recorrido profundo para aceleradores de carreras)',
];

export const TRIGGER_VALUES = [15, 30, 45, 60, 80];

export const SWAP_OPTIONS = [
  'Desactivado (Estándar: L1/R1 superiores y L2/R2 gatillos)',
  'Activado (Intercambiar L1↔L2 y R1↔R2 para apuntar/disparar con botones superiores)',
];

export const RUMBLE_OPTIONS = [
  'Activado (Vibración completa en todos los motores de retroalimentación)',
  'Desactivado (Silenciar y deshabilitar motores de vibración)',
];

export const SWITCH_OPTIONS = [
  'Por Posición (Estilo Xbox/PlayStation: Botón inferior = Aceptar, Botón derecho = Cancelar)',
  'Por Letra (Distribución original Nintendo: A físico derecha = Aceptar, B físico abajo = Cancelar)',
];

export const BACK_BUTTON_OPTIONS = [
  'Botón Share / Create (Capturar pantalla, guardar videoclip y compartir)',
  'Clic de Touchpad (Recomendado para PC/Xbox: Abrir mapa en GTA V, Warzone, The Last of Us)',
];

export const GUIDE_ACTION_OPTIONS = [
  'Botón PS Nativo (Abre menú rápido y vuelve al menú principal de PS4)',
  'Clic de Panel Táctil (Alternativa de navegación sin botón táctil físico)',
];

export const NOTIFICATION_OPTIONS = [
  'Activadas (Muestra notificación emergente GoldHEN con nombre del mando y batería)',
  'Desactivadas (Modo silencioso sin avisos en la pantalla de la consola)',
];

export const DS_BT_MODE_OPTIONS = [
  'Modo Seguro (Reporte 0x01 de 10 bytes: Estabilidad básica Bluetooth)',
  'Modo Extendido (Reporte 0x31 de 78 bytes: Giroscopio 6-ejes, Touchpad, Halo RGB y Gatillos PS5)',
];

export const DS_LIGHTBAR_OPTIONS = [
  'Sincronizar con el Juego (Adapta el color según la salud y eventos del título en ejecución)',
  'Color Estático Personalizado (Mantener siempre el color seleccionado en el halo DualSense)',
  'Indicador de Batería Exacto (Verde > 60%, Amarillo 20-60%, Rojo < 20%)',
  'Apagado (Desactiva el halo LED del mando PS5 para ahorrar batería)',
  'Arcoíris Dinámico RGB (Ciclo cromático suave continuo a 60 FPS en el halo táctil)',
  'Pulso Respiración (Latido luminoso suave con el color personalizado elegido)',
];

export const DS_COLOR_OPTIONS = [
  'Azul PlayStation Clásico (#0070D1)',
  'Rojo Fuego (#FF2233)',
  'Verde Esmeralda (#00E676)',
  'Púrpura Neón (#B388FF)',
  'Cian Eléctrico (#00E5FF)',
  'Blanco Glaciar (#FFFFFF)',
];

export const DS_COLOR_HEX = [
  '#0070D1',
  '#FF2233',
  '#00E676',
  '#B388FF',
  '#00E5FF',
  '#FFFFFF',
];

export const DS_BRIGHTNESS_OPTIONS = [
  'Brillo Alto (100% de intensidad en halo táctil)',
  'Brillo Medio (50% de intensidad equilibrada)',
  'Brillo Tenue (25% - Modo noche y ahorro energético)',
];

export const DS_TRIGGER_OPTIONS = [
  'Estándar / Desactivado (Recorrido normal de gatillo sin tensión extra)',
  'Disparo Rápido / Hair Trigger (Tope corto mecánico táctil calibrado al 100% de fuerza)',
  'Resistencia Progresiva (Tensión continua suave del 40% al apretar el gatillo)',
  'Vibración Háptica (Pulsación de resistencia a 12 Hz y amplitud dinámica en L2/R2)',
];

export const DS_PLAYER_LEDS_OPTIONS = [
  'Ranura de Jugador (1 a 4 puntos LED blancos bajo el Touchpad PS5)',
  'Todos los LEDs Encendidos (5 indicadores inferiores encendidos)',
  'Apagados (Sin indicadores luminosos inferiores)',
];

export const DS_MUTE_OPTIONS = [
  'Por Defecto (Silenciar / Activar micrófono del DualSense con indicador naranja)',
  'Clic de Panel Táctil (Permite pulsar el botón Mute inferior para hacer clic de Touchpad)',
];

export const DS4_LIGHTBAR_OPTIONS = [
  'Control del Juego (Color dinámico oficial asignado por el juego de PS4)',
  'Color Personalizado (Fijar permanentemente el color RGB elegido abajo en el prisma frontal)',
  'Nivel de Batería (Verde = Carga alta, Ámbar = Media, Rojo = Recargar pronto)',
  'Apagado / Ahorro Extremo (Apaga el LED frontal del DualShock 4 para duplicar la batería)',
  'Arcoíris Dinámico RGB (Transición continua de espectro completo en la barra PS4)',
  'Pulso Respiración (Efecto latido suave con tu color favorito en el DualShock 4)',
];

export const DS4_COLOR_OPTIONS = [
  'Azul PS4 Original (#0070DC)',
  'Rojo Carmesí (#EF4444)',
  'Verde Esmeralda (#10B981)',
  'Púrpura Neón (#A855F7)',
  'Cian Eléctrico (#06B6D4)',
  'Ámbar Oro (#F59E0B)',
  'Blanco Puro (#F8FAFC)',
];

export const DS4_COLOR_HEX = [
  '#0070DC',
  '#EF4444',
  '#10B981',
  '#A855F7',
  '#06B6D4',
  '#F59E0B',
  '#F8FAFC',
];

export const DS4_BRIGHTNESS_OPTIONS = [
  'Alto (100% - Iluminación intensa del prisma frontal)',
  'Medio (50% - Equilibrio recomendado entre visibilidad y autonomía)',
  'Tenue (15% - Mínimo reflejo en pantalla y máximo ahorro de batería)',
];

export const DS4_RUMBLE_BOOST_OPTIONS = [
  'Estándar (100% - Fuerza original de los motores pesado y ligero del DualShock 4)',
  'Suave (60% - Vibración atenuada para sesiones largas sin fatiga)',
  'Intenso (120% - Refuerzo de pulso en motores asimétricos)',
  'Desactivado (0% - Apagar por completo la vibración del mando nativo)',
];

export const DS4_TOUCHPAD_OPTIONS = [
  'Táctil + Clic Completo (Gestos capacitivos multi-touch y botón mecánico activos)',
  'Solo Clic Físico (Ignora roces involuntarios en la superficie táctil)',
];

export const DS4_GYRO_OPTIONS = [
  'Alta Precisión (Sin filtro adicional en el acelerómetro y giroscopio Sixaxis)',
  'Filtro Estabilizador (Suaviza micro-temblores al apuntar con movimiento)',
  'Filtro Intenso (Máxima amortiguación de movimiento para conducción y cámara)',
];

export const DS4_ACTIONS: Ds4ActionMeta[] = [
  {
    id: 'cross',
    key_name: 'cross',
    display_name: 'Cruz (X)',
    short_name: 'Cruz',
    icon_path: '/assets/icons/btn_cross_blue.png',
    description: 'Botón de acción principal / Aceptar en PlayStation',
    category: 'action',
  },
  {
    id: 'circle',
    key_name: 'circle',
    display_name: 'Círculo (O)',
    short_name: 'Círculo',
    icon_path: '/assets/icons/btn_circle.png',
    description: 'Botón secundario / Cancelar o Retroceder',
    category: 'action',
  },
  {
    id: 'square',
    key_name: 'square',
    display_name: 'Cuadrado (□)',
    short_name: 'Cuadrado',
    icon_path: '/assets/icons/btn_square.png',
    description: 'Recarga, ataque ligero o interacción contextual',
    category: 'action',
  },
  {
    id: 'triangle',
    key_name: 'triangle',
    display_name: 'Triángulo (△)',
    short_name: 'Triángulo',
    icon_path: '/assets/icons/btn_triangle.png',
    description: 'Cambio de arma, salto secundario o vista',
    category: 'action',
  },
  {
    id: 'l1',
    key_name: 'l1',
    display_name: 'L1 (Botón Superior Izquierdo)',
    short_name: 'L1',
    icon_path: '/assets/icons/btn_l1.png',
    description: 'Bumper izquierdo / Rueda de armas o habilidades',
    category: 'shoulder',
  },
  {
    id: 'r1',
    key_name: 'r1',
    display_name: 'R1 (Botón Superior Derecho)',
    short_name: 'R1',
    icon_path: '/assets/icons/btn_r1.png',
    description: 'Bumper derecho / Granada o ataque primario',
    category: 'shoulder',
  },
  {
    id: 'l2',
    key_name: 'l2',
    display_name: 'L2 (Gatillo Analógico Izquierdo)',
    short_name: 'L2',
    icon_path: '/assets/icons/btn_l2.png',
    description: 'Apuntar con mira (ADS) o freno analógico',
    category: 'shoulder',
  },
  {
    id: 'r2',
    key_name: 'r2',
    display_name: 'R2 (Gatillo Analógico Derecho)',
    short_name: 'R2',
    icon_path: '/assets/icons/btn_r2.png',
    description: 'Disparar arma o acelerador analógico',
    category: 'shoulder',
  },
  {
    id: 'share',
    key_name: 'create',
    display_name: 'Share / Create',
    short_name: 'Share',
    icon_path: '/assets/icons/btn_share.png',
    description: 'Compartir capturas, vídeos y menú de transmisión',
    category: 'system',
  },
  {
    id: 'options',
    key_name: 'options',
    display_name: 'Options (Start)',
    short_name: 'Options',
    icon_path: '/assets/icons/btn_options.png',
    description: 'Pausa del juego y menú de opciones',
    category: 'system',
  },
  {
    id: 'ps',
    key_name: 'ps',
    display_name: 'Botón PS (Home)',
    short_name: 'PS Home',
    icon_path: '/assets/icons/btn_ps.png',
    description: 'Regreso al menú principal de PS4 y menú rápido',
    category: 'system',
  },
  {
    id: 'touchpad',
    key_name: 'touchpad',
    display_name: 'Clic de Touchpad (Panel Táctil)',
    short_name: 'Touchpad',
    icon_path: '/assets/icons/btn_touchpad.png',
    description: 'Abrir mapa del juego, diario o marcador',
    category: 'system',
  },
  {
    id: 'l3',
    key_name: 'l3',
    display_name: 'L3 (Clic Palanca Izquierda)',
    short_name: 'L3',
    icon_path: '/assets/icons/btn_l3.png',
    description: 'Correr (Sprint) o alternar agacharse',
    category: 'stick',
  },
  {
    id: 'r3',
    key_name: 'r3',
    display_name: 'R3 (Clic Palanca Derecha)',
    short_name: 'R3',
    icon_path: '/assets/icons/btn_r3.png',
    description: 'Ataque cuerpo a cuerpo o centrar cámara',
    category: 'stick',
  },
];
