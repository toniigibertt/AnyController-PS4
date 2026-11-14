/**
 * AnyController Studio PS4 - Módulo Exclusivo para Mandos PlayStation 5 (DualSense / DualSense Edge)
 * Configuración dedicada de reporte Bluetooth de 78 bytes, halo RGB táctil, gatillos adaptativos L2/R2 y audio
 */
import { AppConfig, DualSenseTelemetry } from '../../types';
import {
  DS_BT_MODE_OPTIONS,
  DS_LIGHTBAR_OPTIONS,
  DS_COLOR_OPTIONS,
  DS_COLOR_HEX,
  DS_BRIGHTNESS_OPTIONS,
  DS_TRIGGER_OPTIONS,
  DS_PLAYER_LEDS_OPTIONS,
  DS_MUTE_OPTIONS,
} from '../../constants/options';
import {
  Sparkles,
  Bluetooth,
  BatteryCharging,
  BatteryMedium,
  Palette,
  Sun,
  Zap,
  Users,
  Mic,
  Volume2,
  ChevronLeft,
  ChevronRight,
  Cpu,
} from 'lucide-react';

interface DualSenseTabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  selectedCard: number;
  onSelectCard: (index: number) => void;
}

export default function DualSenseTab({
  config,
  onChangeConfig,
  selectedCard,
  onSelectCard,
}: DualSenseTabProps) {
  const telemetry: DualSenseTelemetry = {
    connected: true,
    connectionType: 'bluetooth',
    batteryPercent: 85,
    isCharging: false,
    firmwareVersion: '0x0403 (PS5 DualSense)',
    reportMode: config.ds_bt_mode === 1 ? '0x31' : '0x01',
    touchpadActive: true,
    micMuted: config.ds_mute_action === 0,
    temperature: 28,
  };

  const activeHex = DS_COLOR_HEX[config.ds_lightbar_color] || '#0070D1';

  const cards = [
    {
      id: 0,
      title: 'Modo de Iluminación DualSense (Halo LED Táctil)',
      subtitle:
        'Define el comportamiento del LED lateral que rodea el panel táctil del mando PS5 DualSense.',
      icon: Palette,
      currentValue: DS_LIGHTBAR_OPTIONS[config.ds_lightbar_mode],
      step: (dir: number) => {
        const next =
          (config.ds_lightbar_mode + dir + DS_LIGHTBAR_OPTIONS.length) %
          DS_LIGHTBAR_OPTIONS.length;
        onChangeConfig({ ...config, ds_lightbar_mode: next });
      },
      badge: 'HALO PS5',
    },
    {
      id: 1,
      title: 'Color Fijo del Halo DualSense PS5',
      subtitle:
        'Tono estático personalizado para los laterales del panel táctil del mando de PlayStation 5.',
      icon: Sparkles,
      currentValue: DS_COLOR_OPTIONS[config.ds_lightbar_color],
      colorPreview: DS_COLOR_HEX[config.ds_lightbar_color],
      step: (dir: number) => {
        const next =
          (config.ds_lightbar_color + dir + DS_COLOR_OPTIONS.length) %
          DS_COLOR_OPTIONS.length;
        onChangeConfig({
          ...config,
          ds_lightbar_color: next,
          ds_lightbar_mode:
            config.ds_lightbar_mode === 0 || config.ds_lightbar_mode === 3
              ? 1
              : config.ds_lightbar_mode,
        });
      },
      badge: 'COLOR RGB',
    },
    {
      id: 2,
      title: 'Intensidad del Halo Luminoso PS5',
      subtitle:
        'Potencia de brillo del halo RGB y de los indicadores blancos inferiores del mando DualSense.',
      icon: Sun,
      currentValue: DS_BRIGHTNESS_OPTIONS[config.ds_brightness],
      step: (dir: number) => {
        const next =
          (config.ds_brightness + dir + DS_BRIGHTNESS_OPTIONS.length) %
          DS_BRIGHTNESS_OPTIONS.length;
        onChangeConfig({ ...config, ds_brightness: next });
      },
      badge: 'BRILLO',
    },
    {
      id: 3,
      title: 'Efecto de Gatillos Adaptativos PS5 (L2 / R2)',
      subtitle:
        'Resistencia mecánica en tiempo real exclusiva de los motores de gatillos adaptativos de PlayStation 5.',
      icon: Zap,
      currentValue: DS_TRIGGER_OPTIONS[config.ds_trigger_mode],
      step: (dir: number) => {
        const next =
          (config.ds_trigger_mode + dir + DS_TRIGGER_OPTIONS.length) %
          DS_TRIGGER_OPTIONS.length;
        onChangeConfig({ ...config, ds_trigger_mode: next });
      },
      badge: 'ADAPTATIVOS',
    },
    {
      id: 4,
      title: 'LEDs Blancos Indicadores de Jugador (Bajo Touchpad)',
      subtitle:
        'Configura los 5 puntos LED blancos inferiores exclusivos de los mandos DualSense y DualSense Edge.',
      icon: Users,
      currentValue: DS_PLAYER_LEDS_OPTIONS[config.ds_player_leds],
      step: (dir: number) => {
        const next =
          (config.ds_player_leds + dir + DS_PLAYER_LEDS_OPTIONS.length) %
          DS_PLAYER_LEDS_OPTIONS.length;
        onChangeConfig({ ...config, ds_player_leds: next });
      },
      badge: 'LEDS JUGADOR',
    },
    {
      id: 5,
      title: 'Volumen del Altavoz Integrado DualSense',
      subtitle:
        'Nivel de ganancia de audio del altavoz interno del mando PS5 (0% a 100%).',
      icon: Volume2,
      currentValue: `${config.ds_speaker_volume}%`,
      isSlider: true,
      step: (dir: number) => {
        const next = Math.max(0, Math.min(100, config.ds_speaker_volume + dir * 5));
        onChangeConfig({ ...config, ds_speaker_volume: next });
      },
      badge: 'AUDIO PS5',
    },
    {
      id: 6,
      title: 'Función del Botón Físico Mute (Micrófono PS5)',
      subtitle:
        'Permite reasignar el botón físico inferior de micrófono del DualSense para hacer clic rápido del Touchpad.',
      icon: Mic,
      currentValue: DS_MUTE_OPTIONS[config.ds_mute_action],
      step: (dir: number) => {
        const next =
          (config.ds_mute_action + dir + DS_MUTE_OPTIONS.length) %
          DS_MUTE_OPTIONS.length;
        onChangeConfig({ ...config, ds_mute_action: next });
      },
      badge: config.ds_mute_action === 1 ? 'CLIC TOUCHPAD' : 'MUTE MIC',
    },
    {
      id: 7,
      title: 'Protocolo Bluetooth DualSense (10B vs 78B)',
      subtitle:
        'El Modo Extendido (78B con CRC-32) activa gatillos adaptativos, giroscopio y control del halo LED en PS5.',
      icon: Bluetooth,
      currentValue: DS_BT_MODE_OPTIONS[config.ds_bt_mode],
      step: (dir: number) => {
        const next =
          (config.ds_bt_mode + dir + DS_BT_MODE_OPTIONS.length) %
          DS_BT_MODE_OPTIONS.length;
        onChangeConfig({ ...config, ds_bt_mode: next });
      },
      badge: config.ds_bt_mode === 1 ? '78B EXTENDIDO' : '10B SEGURO',
    },
  ];

  return (
    <div className="space-y-6">
      {/* Exclusive PS5 DualSense Header Deck */}
      <div className="relative overflow-hidden rounded-xl bg-[#0B1120] border border-cyan-500/30 p-5 shadow-xl">
        <div
          className="absolute top-0 left-0 w-1.5 h-full"
          style={{ backgroundColor: activeHex }}
        />
        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-5 pl-2">
          <div className="flex items-start sm:items-center gap-4">
            <div
              className="w-12 h-12 rounded-lg border flex items-center justify-center shrink-0 transition-all"
              style={{
                borderColor: activeHex,
                backgroundColor: `${activeHex}18`,
                boxShadow: `0 0 24px ${activeHex}40`,
              }}
            >
              <Sparkles className="w-6 h-6" style={{ color: activeHex }} />
            </div>
            <div>
              <div className="flex flex-wrap items-center gap-2.5">
                <h2 className="text-base md:text-lg font-bold text-white tracking-tight">
                  Centro Exclusivo PlayStation 5 (DualSense / DualSense Edge)
                </h2>
                <span className="px-2 py-0.5 text-[11px] font-mono uppercase tracking-wider bg-cyan-950/90 text-cyan-300 border border-cyan-500/30 rounded">
                  SOLO MANDOS PS5 (0x0CE6 / 0x0DF2)
                </span>
              </div>
              <p className="text-xs text-slate-400 mt-1">
                Protocolo activo:{' '}
                <span className="font-mono text-cyan-400">
                  Reporte {telemetry.reportMode} (78 Bytes + CRC32)
                </span>{' '}
                • Gatillos Adaptativos L2/R2, LEDs de Jugador y Botón Mute exclusivos de PS5
              </p>
            </div>
          </div>

          {/* Telemetry Readouts */}
          <div className="flex flex-wrap items-center gap-2.5">
            <div className="flex items-center gap-2 px-3 py-2 rounded-lg bg-[#070B14] border border-emerald-500/40 text-xs">
              {telemetry.isCharging ? (
                <BatteryCharging className="w-4 h-4 text-amber-400 animate-pulse" />
              ) : (
                <BatteryMedium className="w-4 h-4 text-emerald-400" />
              )}
              <span className="text-emerald-300 font-semibold font-mono">
                J1: {telemetry.batteryPercent}% Exacto (BT) • J2: Listo Selector Perfil PS4
              </span>
            </div>

            <div className="flex items-center gap-1.5 px-3 py-2 rounded-lg bg-[#070B14] border border-cyan-500/40 text-xs">
              <Cpu className="w-4 h-4 text-cyan-400" />
              <span className="text-cyan-300 font-semibold">
                Multijugador Local 1-4 Mandos (Auto-Selector "¿Quién usa este mando?")
              </span>
            </div>
          </div>
        </div>
      </div>

      {/* DualSense Feature Cards Grid */}
      <div className="grid grid-cols-1 gap-3">
        {cards.map((card) => {
          const Icon = card.icon;
          const isSelected = selectedCard === card.id;

          return (
            <div
              key={card.id}
              onClick={() => onSelectCard(card.id)}
              className={`group relative p-4 rounded-xl border transition-all duration-150 cursor-pointer ${
                isSelected
                  ? 'bg-[#101C34] border-cyan-400/80 shadow-[0_0_20px_rgba(0,175,255,0.15)]'
                  : 'bg-[#0B1120]/90 hover:bg-[#0E172A] border-slate-800 hover:border-slate-700'
              }`}
            >
              {isSelected && (
                <div className="absolute left-0 top-3 bottom-3 w-1 bg-cyan-400 rounded-r" />
              )}
              <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 pl-1">
                <div className="flex items-start gap-3.5 max-w-2xl">
                  <div
                    className={`p-2.5 rounded-lg border transition-colors ${
                      isSelected
                        ? 'bg-cyan-500/20 border-cyan-400/50 text-cyan-300'
                        : 'bg-slate-900 border-slate-800 text-slate-400 group-hover:text-slate-200'
                    }`}
                  >
                    <Icon className="w-5 h-5" />
                  </div>
                  <div>
                    <div className="flex items-center gap-2.5">
                      <h3 className="text-sm md:text-base font-semibold text-white">
                        {card.title}
                      </h3>
                      <span className="text-[10px] px-1.5 py-0.5 rounded bg-slate-900 border border-slate-700 text-cyan-300 font-mono">
                        {card.badge}
                      </span>
                    </div>
                    <p className="text-xs text-slate-400 mt-1 leading-relaxed">
                      {card.subtitle}
                    </p>
                  </div>
                </div>

                {/* Right controls */}
                <div className="flex items-center self-end md:self-center gap-2 bg-[#070B14] p-1.5 rounded-lg border border-slate-800">
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      card.step(-1);
                    }}
                    className="p-1.5 rounded bg-slate-900 hover:bg-cyan-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                    title="Anterior"
                  >
                    <ChevronLeft className="w-4 h-4" />
                  </button>

                  <div className="min-w-[210px] sm:min-w-[270px] px-2 flex items-center justify-center gap-2">
                    {card.colorPreview && (
                      <span
                        className="w-3.5 h-3.5 rounded-sm border border-white/40 shrink-0"
                        style={{ backgroundColor: card.colorPreview }}
                      />
                    )}
                    <span className="text-xs font-semibold text-cyan-300 line-clamp-1">
                      {card.currentValue}
                    </span>
                  </div>

                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      card.step(1);
                    }}
                    className="p-1.5 rounded bg-slate-900 hover:bg-cyan-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                    title="Siguiente"
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
