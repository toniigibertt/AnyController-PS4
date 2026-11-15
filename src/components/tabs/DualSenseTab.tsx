/**
 * Controller Control Center PS4 - Módulo Exclusivo PS5 DualSense / Edge (v1.5.0)
 * Soporta perfiles independientes por mando (MAC / Ranura) con descripción en vivo por opción.
 */
import { AppConfig, ControllerTargetMeta, DualSenseTelemetry } from '../../types';
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
  Info,
} from 'lucide-react';

interface DualSenseTabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  selectedCard: number;
  onSelectCard: (index: number) => void;
  activeTarget?: ControllerTargetMeta;
}

export default function DualSenseTab({
  config,
  onChangeConfig,
  selectedCard,
  onSelectCard,
  activeTarget,
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

  const activeHex =
    config.ds_lightbar_mode === 3
      ? '#1E293B'
      : DS_COLOR_HEX[config.ds_lightbar_color] || '#0070D1';

  const cards = [
    {
      id: 0,
      title: 'Modo de Iluminación DualSense (Halo LED Táctil)',
      subtitle:
        'Define el comportamiento del LED lateral que rodea el panel táctil del mando PS5 DualSense.',
      guide:
        'Elige color fijo personalizado, indicador de batería real, apagado total, Arcoíris RGB continuo o latido suave. Se conserva en este mando aunque cierres el PKG.',
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
      guide:
        'Permite distinguir visualmente cada DualSense (ej. J1 azul y J2 rojo o verde). Se guarda por dirección MAC para recordarlo en futuras sesiones.',
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
      guide:
        'Bajar a 50% o 25% reduce reflejos en el televisor de noche y prolonga la autonomía de la batería sin apagar el LED.',
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
      guide:
        'Disparo Rápido acorta el recorrido con tope táctil; Resistencia endurece L2/R2 para conducción; Vibración añade pulso motorizado en el gatillo.',
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
      guide:
        'Por defecto muestra el número de jugador real en PS4 (1 a 4 luces blancas), pero puedes encender los 5 LEDs o apagarlos completamente.',
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
      guide:
        'Controla el volumen máximo enviado al altavoz del DualSense en juegos compatibles cuando está conectado por USB o reporte extendido.',
      icon: Volume2,
      currentValue: `${config.ds_speaker_volume}%`,
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
      guide:
        'Útil si prefieres pulsar el pequeño botón central bajo el logo PS para abrir mapas o marcadores sin soltar el pulgar.',
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
      guide:
        'Mantén el Modo Extendido (78B) activado siempre que sea posible; usa 10B solo para diagnóstico en adaptadores Bluetooth muy antiguos.',
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

  const activeCardMeta = cards.find((c) => c.id === selectedCard) || cards[0];

  return (
    <div className="space-y-5">
      {/* DualSense Header Deck */}
      <div className="rounded-xl bg-[#0B1120] border border-slate-800 p-5">
        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-4">
          <div className="flex items-start sm:items-center gap-4">
            <div
              className="w-11 h-11 rounded-lg border flex items-center justify-center shrink-0 transition-colors"
              style={{
                borderColor: activeHex,
                backgroundColor: `${activeHex}18`,
              }}
            >
              <Sparkles className="w-5 h-5" style={{ color: activeHex }} />
            </div>
            <div>
              <h2 className="text-base md:text-lg font-bold text-white tracking-tight">
                Centro Exclusivo PlayStation 5 (DualSense / DualSense Edge)
              </h2>
              <p className="text-xs text-slate-400 mt-1">
                Editando:{' '}
                <span className="text-slate-200 font-semibold">
                  {activeTarget?.label || 'Global (Por Defecto)'}
                </span>{' '}
                · Protocolo:{' '}
                <span className="font-mono text-cyan-400">
                  Reporte {telemetry.reportMode}
                </span>{' '}
                · Memoria por dirección MAC única
              </p>
            </div>
          </div>

          <div className="flex flex-wrap items-center gap-2.5 text-xs">
            <div className="flex items-center gap-2 px-3 py-1.5 rounded-lg bg-[#070B14] border border-slate-800 font-mono tabular-nums text-emerald-300">
              <BatteryMedium className="w-4 h-4 text-emerald-400" />
              <span>Batería BT: {telemetry.batteryPercent}%</span>
            </div>
            <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-[#070B14] border border-slate-800 text-cyan-300">
              <Cpu className="w-4 h-4 text-cyan-400" />
              <span>Multi-DualSense Independiente (1-4)</span>
            </div>
          </div>
        </div>

        {/* Fast Color Swatch Bar */}
        <div className="mt-4 pt-4 border-t border-slate-800/80 flex flex-wrap items-center justify-between gap-3">
          <span className="text-xs text-slate-400 font-medium">
            Selección rápida de color para el halo de este mando PS5:
          </span>
          <div className="flex items-center gap-2">
            {DS_COLOR_HEX.map((hex, idx) => {
              const isSelectedColor =
                config.ds_lightbar_color === idx && config.ds_lightbar_mode !== 3;
              return (
                <button
                  key={hex}
                  type="button"
                  onClick={() =>
                    onChangeConfig({
                      ...config,
                      ds_lightbar_mode: 1,
                      ds_lightbar_color: idx,
                    })
                  }
                  className={`w-7 h-7 rounded-md border transition-transform cursor-pointer ${
                    isSelectedColor
                      ? 'scale-110 border-white ring-2 ring-cyan-400/50'
                      : 'border-white/20 opacity-75 hover:opacity-100'
                  }`}
                  style={{ backgroundColor: hex }}
                  title={DS_COLOR_OPTIONS[idx]}
                />
              );
            })}
          </div>
        </div>
      </div>

      {/* Live Option Guide */}
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

      {/* DualSense Feature Cards */}
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
                        · {card.badge}
                      </span>
                    </div>
                    <p className="text-xs text-slate-400 mt-1 leading-relaxed">
                      {card.subtitle}
                    </p>
                  </div>
                </div>

                {/* Right controls */}
                <div className="flex items-center self-end md:self-center gap-2 bg-[#070B14] p-1.5 rounded-lg border border-slate-800 shrink-0">
                  <button
                    type="button"
                    onClick={(e) => {
                      e.stopPropagation();
                      onSelectCard(card.id);
                      card.step(-1);
                    }}
                    className="p-1.5 rounded bg-slate-900 hover:bg-cyan-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                    title="Anterior"
                  >
                    <ChevronLeft className="w-4 h-4" />
                  </button>

                  <div className="min-w-[200px] sm:min-w-[250px] px-2 flex items-center justify-center gap-2">
                    {card.colorPreview && (
                      <span
                        className="w-3.5 h-3.5 rounded-sm border border-white/40 shrink-0"
                        style={{ backgroundColor: card.colorPreview }}
                      />
                    )}
                    <span className="text-xs font-semibold text-cyan-300 line-clamp-1 tabular-nums">
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
