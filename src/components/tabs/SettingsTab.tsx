/**
 * Controller Control Center PS4 - Pestaña de Ajustes Globales y Por Mando (v1.5.0)
 * Incluye guía descriptiva en vivo por opción y sincronización por archivo objetivo.
 */
import { AppConfig, ControllerTargetMeta } from '../../types';
import {
  PRO_PROFILE_OPTIONS,
  DEADZONE_OPTIONS,
  STICK_CURVE_OPTIONS,
  TRIGGER_OPTIONS,
  RAPID_FIRE_OPTIONS,
  SWAP_OPTIONS,
  RUMBLE_OPTIONS,
  SWITCH_OPTIONS,
  BACK_BUTTON_OPTIONS,
  NOTIFICATION_OPTIONS,
} from '../../constants/options';
import {
  ChevronLeft,
  ChevronRight,
  Save,
  Download,
  Sliders,
  Zap,
  Repeat,
  Volume2,
  Gamepad,
  Share2,
  Bell,
  Award,
  Activity,
  Flame,
  Info,
} from 'lucide-react';

interface SettingsTabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  onSave: () => void;
  onDownloadIni: () => void;
  selectedCard: number;
  onSelectCard: (index: number) => void;
  activeTarget?: ControllerTargetMeta;
}

export default function SettingsTab({
  config,
  onChangeConfig,
  onSave,
  onDownloadIni,
  selectedCard,
  onSelectCard,
  activeTarget,
}: SettingsTabProps) {
  const applyProPreset = (presetIdx: number) => {
    const next = { ...config, pro_profile_index: presetIdx };
    if (presetIdx === 1) {
      next.deadzone_index = 1;
      next.stick_curve_index = 2;
      next.trigger_index = 0;
      next.ds_trigger_mode = 1;
    } else if (presetIdx === 2) {
      next.deadzone_index = 2;
      next.stick_curve_index = 1;
      next.trigger_index = 4;
      next.ds_trigger_mode = 2;
    } else if (presetIdx === 3) {
      next.rumble_index = 1;
      next.ds_lightbar_mode = 3;
      next.ds_player_leds = 2;
      next.ds4_lightbar_mode = 3;
    } else if (presetIdx === 4) {
      next.deadzone_index = 4;
      next.stick_curve_index = 0;
      next.trigger_index = 1;
      next.rapid_fire_index = 0;
      next.swap_index = 0;
      next.rumble_index = 0;
      next.ds_lightbar_mode = 1;
      next.ds_trigger_mode = 0;
    }
    onChangeConfig(next);
  };

  const cards = [
    {
      id: 0,
      title: 'Perfil Rápido Pro (Presets Instantáneos)',
      subtitle:
        'Aplica al instante configuraciones optimizadas para eSports FPS, SimRacing, Ahorro Extremo o Estándar.',
      guide:
        'Sobrescribe zona muerta, curva de palancas y respuesta de gatillos en un paso. Al modificar cualquier ajuste manual después, vuelve a Personalizado.',
      icon: Award,
      currentValue: PRO_PROFILE_OPTIONS[config.pro_profile_index ?? 0],
      step: (dir: number) => {
        const next =
          ((config.pro_profile_index ?? 0) + dir + PRO_PROFILE_OPTIONS.length) %
          PRO_PROFILE_OPTIONS.length;
        applyProPreset(next);
      },
      tag: 'PRO PRESET',
    },
    {
      id: 1,
      title: 'Zona Muerta de Palancas Analógicas (Anti-Drift)',
      subtitle:
        'Filtra oscilaciones involuntarias en reposo de las palancas izquierda y derecha.',
      guide:
        'Si el personaje o la cámara se mueven solos sin tocar el stick (drift), sube este porcentaje poco a poco hasta que quede completamente estable en reposo.',
      icon: Sliders,
      currentValue: DEADZONE_OPTIONS[config.deadzone_index],
      step: (dir: number) => {
        const next =
          (config.deadzone_index + dir + DEADZONE_OPTIONS.length) %
          DEADZONE_OPTIONS.length;
        onChangeConfig({ ...config, pro_profile_index: 0, deadzone_index: next });
      },
      tag: '0% A 25%',
    },
    {
      id: 2,
      title: 'Curva de Respuesta de Joysticks (LX/LY y RX/RY)',
      subtitle:
        'Elige entre respuesta Lineal 1:1, Precisión Exponencial (Francotirador/Conducción) o Agresiva eSports.',
      guide:
        'Lineal 1:1 mantiene el tacto original de Sony; Exponencial suaviza los movimientos finos en el centro; Agresiva acelera el giro en inclinaciones medias.',
      icon: Activity,
      currentValue: STICK_CURVE_OPTIONS[config.stick_curve_index ?? 0],
      step: (dir: number) => {
        const next =
          ((config.stick_curve_index ?? 0) + dir + STICK_CURVE_OPTIONS.length) %
          STICK_CURVE_OPTIONS.length;
        onChangeConfig({ ...config, pro_profile_index: 0, stick_curve_index: next });
      },
      tag:
        (config.stick_curve_index ?? 0) === 0
          ? 'LINEAL 1:1'
          : (config.stick_curve_index ?? 0) === 1
          ? 'PRECISIÓN'
          : 'AGRESIVA',
    },
    {
      id: 3,
      title: 'Umbral de Disparo Digital en Gatillos L2 / R2',
      subtitle:
        'Punto exacto del recorrido analógico (0-255) que activa el disparo digital.',
      guide:
        'Un valor bajo (15 o 30) activa L2/R2 casi al rozar el gatillo (ideal para shooters). Valores altos (60 o 80) conservan recorrido para acelerador progresivo.',
      icon: Zap,
      currentValue: TRIGGER_OPTIONS[config.trigger_index],
      step: (dir: number) => {
        const next =
          (config.trigger_index + dir + TRIGGER_OPTIONS.length) %
          TRIGGER_OPTIONS.length;
        onChangeConfig({ ...config, pro_profile_index: 0, trigger_index: next });
      },
      tag: '6% A 31%',
    },
    {
      id: 4,
      title: 'Fuego Rápido / Modo Turbo Automático (Rapid-Fire)',
      subtitle:
        'Pulsa repetidamente R2 o L2+R2 a velocidad ultra-rápida (15 disparos/seg) mientras mantienes apretado el gatillo.',
      guide:
        'Convierte armas semiautomáticas o pulsaciones repetitivas en disparo continuo sin cansar el dedo mientras mantienes presionado R2.',
      icon: Flame,
      currentValue: RAPID_FIRE_OPTIONS[config.rapid_fire_index ?? 0],
      step: (dir: number) => {
        const next =
          ((config.rapid_fire_index ?? 0) + dir + RAPID_FIRE_OPTIONS.length) %
          RAPID_FIRE_OPTIONS.length;
        onChangeConfig({ ...config, rapid_fire_index: next });
      },
      tag: (config.rapid_fire_index ?? 0) > 0 ? 'TURBO ACTIVO' : 'APAGADO',
    },
    {
      id: 5,
      title: 'Inversión Táctica de Hombros (L1/L2 y R1/R2)',
      subtitle:
        'Intercambia los botones superiores L1/R1 con los gatillos principales L2/R2.',
      guide:
        'Al activarlo, apuntarás y dispararás con L1/R1 (estilo clásico PS3) incluso en juegos que no ofrecen esa opción en su menú interno.',
      icon: Repeat,
      currentValue: SWAP_OPTIONS[config.swap_index],
      step: (dir: number) => {
        const next =
          (config.swap_index + dir + SWAP_OPTIONS.length) % SWAP_OPTIONS.length;
        onChangeConfig({ ...config, swap_index: next });
      },
      tag: config.swap_index === 1 ? 'ACTIVADO' : 'DESACTIVADO',
    },
    {
      id: 6,
      title: 'Motor General de Vibración Háptica',
      subtitle:
        'Activa o silencia globalmente los motores de vibración de este perfil de mando.',
      guide:
        'Si desactivas la vibración, este mando no vibrará en ningún juego, ahorrando batería y evitando distracciones en modo competitivo.',
      icon: Volume2,
      currentValue: RUMBLE_OPTIONS[config.rumble_index],
      step: (dir: number) => {
        const next =
          (config.rumble_index + dir + RUMBLE_OPTIONS.length) %
          RUMBLE_OPTIONS.length;
        onChangeConfig({ ...config, rumble_index: next });
      },
      tag: config.rumble_index === 0 ? 'ACTIVADO' : 'SILENCIADO',
    },
    {
      id: 7,
      title: 'Esquema de Botones Nintendo Switch (A/B/X/Y)',
      subtitle:
        'Por posición física (estilo PlayStation) o por letra grabada en el mando Switch.',
      guide:
        'Por Posición mantiene el botón inferior como Cruz (✕) igual que un mando PlayStation; Por Letra respeta A/B/X/Y tal como está serigrafiado en Switch.',
      icon: Gamepad,
      currentValue: SWITCH_OPTIONS[config.switch_index],
      step: (dir: number) => {
        const next =
          (config.switch_index + dir + SWITCH_OPTIONS.length) %
          SWITCH_OPTIONS.length;
        onChangeConfig({ ...config, switch_index: next });
      },
      tag: config.switch_index === 0 ? 'POSICIÓN' : 'SERIGRAFÍA',
    },
    {
      id: 8,
      title: 'Acción del Botón Atrás / View (Mandos Xbox y PC)',
      subtitle:
        'Configura si el botón Atrás/View abre el mapa del juego (Touchpad) o saca capturas (Share).',
      guide:
        'Recomendado en Clic de Touchpad para poder abrir mapas, marcadores e inventarios desde mandos de Xbox/PC que no tienen panel táctil.',
      icon: Share2,
      currentValue: BACK_BUTTON_OPTIONS[config.back_button_index],
      step: (dir: number) => {
        const next =
          (config.back_button_index + dir + BACK_BUTTON_OPTIONS.length) %
          BACK_BUTTON_OPTIONS.length;
        onChangeConfig({ ...config, back_button_index: next });
      },
      tag: config.back_button_index === 1 ? 'TOUCHPAD' : 'SHARE',
    },
    {
      id: 9,
      title: 'Avisos Emergentes en Consola (GoldHEN)',
      subtitle:
        'Muestra una notificación en la esquina de PS4 con el nombre del mando, jugador y batería al conectarlo.',
      guide:
        'Desactívalo si prefieres jugar sin avisos en pantalla cuando conectas un segundo mando o cuando la batería baja del umbral.',
      icon: Bell,
      currentValue:
        NOTIFICATION_OPTIONS[config.enable_notifications === 1 ? 0 : 1],
      step: () => {
        const next = config.enable_notifications === 1 ? 0 : 1;
        onChangeConfig({ ...config, enable_notifications: next });
      },
      tag: config.enable_notifications === 1 ? 'VISIBLES' : 'OCULTOS',
    },
  ];

  const activeCardMeta = cards.find((c) => c.id === selectedCard) || cards[0];

  return (
    <div className="space-y-5">
      {/* Studio Top Banner & Actions (Clean 1px border layout) */}
      <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4 p-5 rounded-xl bg-[#0B1120] border border-slate-800">
        <div>
          <h2 className="text-base md:text-lg font-bold text-white tracking-tight">
            Motor de Respuesta, Ejes Analógicos y Remapeo
          </h2>
          <p className="text-xs text-slate-400 mt-1">
            Perfil activo:{' '}
            <span className="text-slate-200 font-semibold">
              {activeTarget?.label || 'Global (Por Defecto)'}
            </span>{' '}
            · Archivo PS4:{' '}
            <code className="text-cyan-400 font-mono">
              {activeTarget?.ps4FullPath || '/data/anycontroller/anycontroller.ini'}
            </code>
          </p>
        </div>
        <div className="flex items-center gap-2.5 shrink-0">
          <button
            type="button"
            onClick={onSave}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-cyan-400 hover:bg-cyan-300 text-slate-950 font-bold text-xs transition-colors cursor-pointer whitespace-nowrap"
          >
            <Save className="w-4 h-4" />
            <span>Guardar Memoria</span>
          </button>
          <button
            type="button"
            onClick={onDownloadIni}
            className="flex items-center gap-2 px-3.5 py-2 rounded-lg bg-[#0E172A] hover:bg-slate-800 text-cyan-300 font-semibold text-xs border border-slate-700 transition-colors cursor-pointer whitespace-nowrap"
            title="Descargar archivo .ini listo para tu consola PS4"
          >
            <Download className="w-4 h-4 text-cyan-400" />
            <span>Descargar .INI</span>
          </button>
        </div>
      </div>

      {/* Live Descriptive Option Guide */}
      <div className="p-4 rounded-xl bg-[#070E1D] border border-cyan-500/30 flex items-start gap-3">
        <Info className="w-4 h-4 text-cyan-400 shrink-0 mt-0.5" />
        <div className="text-xs">
          <div className="font-bold text-cyan-300">
            Qué cambia: {activeCardMeta.title}
          </div>
          <p className="text-slate-300 mt-0.5 leading-relaxed">
            {activeCardMeta.guide}
          </p>
        </div>
      </div>

      {/* Studio Configuration Cards */}
      <div className="grid grid-cols-1 gap-2.5">
        {cards.map((card) => {
          const Icon = card.icon;
          const isSelected = selectedCard === card.id;

          return (
            <div
              key={card.id}
              onClick={() => onSelectCard(card.id)}
              className={`group p-4 rounded-xl border transition-colors cursor-pointer ${
                isSelected
                  ? 'bg-[#101C34] border-cyan-400/80'
                  : 'bg-[#0B1120]/90 hover:bg-[#0E172A] border-slate-800 hover:border-slate-700'
              }`}
            >
              <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
                <div className="flex items-start gap-3.5 max-w-2xl">
                  <div
                    className={`p-2.5 rounded-lg border transition-colors shrink-0 ${
                      isSelected
                        ? 'bg-cyan-500/20 border-cyan-400/50 text-cyan-300'
                        : 'bg-slate-900 border-slate-800 text-slate-400 group-hover:text-slate-200'
                    }`}
                  >
                    <Icon className="w-5 h-5" />
                  </div>
                  <div>
                    <div className="flex flex-wrap items-center gap-2">
                      <h3 className="text-sm md:text-base font-semibold text-white">
                        {card.title}
                      </h3>
                      <span className="text-[10px] font-mono text-cyan-300/90 tabular-nums">
                        · {card.tag}
                      </span>
                    </div>
                    <p className="text-xs text-slate-400 mt-1 leading-relaxed">
                      {card.subtitle}
                    </p>
                  </div>
                </div>

                {/* Right: Value Stepper Controls */}
                <div className="flex items-center self-end md:self-center gap-2 bg-[#070B14] p-1.5 rounded-lg border border-slate-800 shrink-0">
                  <button
                    type="button"
                    onClick={(e) => {
                      e.stopPropagation();
                      onSelectCard(card.id);
                      card.step(-1);
                    }}
                    className="p-1.5 rounded bg-slate-900 hover:bg-cyan-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                    title="Opción anterior"
                  >
                    <ChevronLeft className="w-4 h-4" />
                  </button>

                  <div className="min-w-[200px] sm:min-w-[250px] px-2 text-center">
                    <span className="text-xs font-semibold text-cyan-300 line-clamp-1">
                      {card.currentValue}
                    </span>
                  </div>

                  <button
                    type="button"
                    onClick={(e) => {
                      e.stopPropagation();
                      onSelectCard(card.id);
                      card.step(1);
                    }}
                    className="p-1.5 rounded bg-slate-900 hover:bg-cyan-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                    title="Opción siguiente"
                  >
                    <ChevronRight className="w-4 h-4" />
                  </button>
                </div>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}
