/**
 * Controller Control Center PS4 - Módulo Nativo para Mandos DualShock 4 (v1.5.0)
 * Incluye guía descriptiva en vivo y configuración por ranura de jugador.
 */
import { AppConfig, ControllerTargetMeta } from '../../types';
import {
  DS4_LIGHTBAR_OPTIONS,
  DS4_COLOR_OPTIONS,
  DS4_COLOR_HEX,
  DS4_BRIGHTNESS_OPTIONS,
  DS4_RUMBLE_BOOST_OPTIONS,
  DS4_TOUCHPAD_OPTIONS,
  DS4_GYRO_OPTIONS,
} from '../../constants/options';
import {
  Gamepad,
  Palette,
  Sun,
  Vibrate,
  MousePointerClick,
  Compass,
  ChevronLeft,
  ChevronRight,
  BatteryCharging,
  ShieldCheck,
  Sparkles,
  Info,
} from 'lucide-react';

interface DualShock4TabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  selectedCard: number;
  onSelectCard: (index: number) => void;
  onTriggerRumble?: (phase: number) => void;
  onShowToast?: (msg: string) => void;
  activeTarget?: ControllerTargetMeta;
}

export default function DualShock4Tab({
  config,
  onChangeConfig,
  selectedCard,
  onSelectCard,
  onTriggerRumble,
  onShowToast,
  activeTarget,
}: DualShock4TabProps) {
  const activeHex =
    config.ds4_lightbar_mode === 3
      ? '#1E293B'
      : config.ds4_lightbar_mode === 2
      ? '#10B981'
      : DS4_COLOR_HEX[config.ds4_lightbar_color] || '#0070DC';

  const cards = [
    {
      id: 0,
      title: 'Modo de Barra Luminosa Frontal (DualShock 4)',
      subtitle:
        'Elige entre color oficial del juego, color fijo personalizado, indicador de batería o apagado total.',
      guide:
        'Control del Juego deja que el título decida; Color Personalizado fija el prisma frontal con el color elegido abajo; Apagado maximiza la batería.',
      icon: Palette,
      currentValue: DS4_LIGHTBAR_OPTIONS[config.ds4_lightbar_mode],
      step: (dir: number) => {
        const next =
          (config.ds4_lightbar_mode + dir + DS4_LIGHTBAR_OPTIONS.length) %
          DS4_LIGHTBAR_OPTIONS.length;
        onChangeConfig({ ...config, ds4_lightbar_mode: next });
      },
      code: 'MODO LED',
    },
    {
      id: 1,
      title: 'Color Personalizado del Prisma DualShock 4',
      subtitle:
        'Aplica en vivo cualquier color RGB al prisma frontal de tu mando original de PS4 mediante libScePad.',
      guide:
        'Cada ranura DualShock 4 puede llevar su propio color guardado de forma independiente sin cambiar el mando de otro jugador.',
      icon: Sparkles,
      currentValue: DS4_COLOR_OPTIONS[config.ds4_lightbar_color],
      colorPreview: DS4_COLOR_HEX[config.ds4_lightbar_color],
      step: (dir: number) => {
        const next =
          (config.ds4_lightbar_color + dir + DS4_COLOR_OPTIONS.length) %
          DS4_COLOR_OPTIONS.length;
        onChangeConfig({
          ...config,
          ds4_lightbar_color: next,
          ds4_lightbar_mode:
            config.ds4_lightbar_mode === 0 || config.ds4_lightbar_mode === 3
              ? 1
              : config.ds4_lightbar_mode,
        });
      },
      code: 'RGB PRISMA',
    },
    {
      id: 2,
      title: 'Nivel de Brillo y Ahorro de Batería',
      subtitle:
        'Reduce la potencia luminosa del LED frontal para duplicar la autonomía de tu DualShock 4.',
      guide:
        'En 15% (Tenue) el LED apenas molesta en la oscuridad de la habitación y ahorra batería en mandos PS4 V1 y V2.',
      icon: Sun,
      currentValue: DS4_BRIGHTNESS_OPTIONS[config.ds4_brightness],
      step: (dir: number) => {
        const next =
          (config.ds4_brightness + dir + DS4_BRIGHTNESS_OPTIONS.length) %
          DS4_BRIGHTNESS_OPTIONS.length;
        onChangeConfig({ ...config, ds4_brightness: next });
      },
      code: 'AUTONOMÍA',
    },
    {
      id: 3,
      title: 'Potencia de Motores de Vibración (Pesado / Ligero)',
      subtitle:
        'Ajusta la intensidad de los motores asimétricos internos de los mandos originales CUH-ZCT1 y ZCT2.',
      guide:
        'Regula la fuerza real de vibración de este mando DualShock 4 entre 60% suave, 100% estándar, 120% intenso o apagado.',
      icon: Vibrate,
      currentValue: DS4_RUMBLE_BOOST_OPTIONS[config.ds4_rumble_boost],
      step: (dir: number) => {
        const next =
          (config.ds4_rumble_boost + dir + DS4_RUMBLE_BOOST_OPTIONS.length) %
          DS4_RUMBLE_BOOST_OPTIONS.length;
        onChangeConfig({ ...config, ds4_rumble_boost: next });
      },
      code: 'MOTORES',
    },
    {
      id: 4,
      title: 'Respuesta del Panel Táctil Capacitivo PS4',
      subtitle:
        'Configura si deseas detección táctil multi-touch completa o únicamente el clic mecánico físico.',
      guide:
        'Solo Clic Físico evita toques accidentales con el pulgar sobre el panel central en juegos competitivos.',
      icon: MousePointerClick,
      currentValue: DS4_TOUCHPAD_OPTIONS[config.ds4_touchpad_mode],
      step: (dir: number) => {
        const next =
          (config.ds4_touchpad_mode + dir + DS4_TOUCHPAD_OPTIONS.length) %
          DS4_TOUCHPAD_OPTIONS.length;
        onChangeConfig({ ...config, ds4_touchpad_mode: next });
      },
      code: 'TOUCHPAD',
    },
    {
      id: 5,
      title: 'Filtro de Estabilización del Giroscopio Sixaxis',
      subtitle:
        'Elimina micro-temblores del sensor de movimiento Sixaxis en juegos con apuntado o conducción por giro.',
      guide:
        'Suaviza pequeñas vibraciones de la mano cuando usas apuntado por giroscopio en juegos compatibles de PS4.',
      icon: Compass,
      currentValue: DS4_GYRO_OPTIONS[config.ds4_gyro_deadband],
      step: (dir: number) => {
        const next =
          (config.ds4_gyro_deadband + dir + DS4_GYRO_OPTIONS.length) %
          DS4_GYRO_OPTIONS.length;
        onChangeConfig({ ...config, ds4_gyro_deadband: next });
      },
      code: 'SIXAXIS',
    },
  ];

  const activeCardMeta = cards.find((c) => c.id === selectedCard) || cards[0];

  return (
    <div className="space-y-5">
      {/* Native PS4 DualShock 4 Header Deck */}
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
              <Gamepad className="w-5 h-5" style={{ color: activeHex }} />
            </div>
            <div>
              <h2 className="text-base md:text-lg font-bold text-white tracking-tight">
                Configurador Nativo Sony DualShock 4 (CUH-ZCT1 / ZCT2)
              </h2>
              <p className="text-xs text-slate-400 mt-1">
                Editando:{' '}
                <span className="text-slate-200 font-semibold">
                  {activeTarget?.label || 'Global (Por Defecto)'}
                </span>{' '}
                · Control directo por <span className="font-mono text-cyan-400">libScePad</span> sin interferir con el DualSense
              </p>
            </div>
          </div>

          <div className="flex flex-wrap items-center gap-3">
            <div className="flex items-center gap-2.5 bg-[#070B14] border border-slate-800 px-3 py-1.5 rounded-lg">
              <span className="text-[11px] font-mono text-slate-400">
                PRISMA FRONTAL
              </span>
              <div
                className="w-16 h-3 rounded-sm border border-white/25 transition-all"
                style={{
                  backgroundColor: activeHex,
                  boxShadow:
                    config.ds4_lightbar_mode === 3
                      ? 'none'
                      : `0 0 12px ${activeHex}`,
                  opacity:
                    config.ds4_brightness === 0
                      ? 1
                      : config.ds4_brightness === 1
                      ? 0.65
                      : 0.35,
                }}
              />
            </div>

            <button
              type="button"
              onClick={() => {
                onTriggerRumble?.(3);
                onShowToast?.(
                  `Probando vibración aislada en ${activeTarget?.shortLabel || 'mando activo'}...`
                );
              }}
              className="flex items-center gap-2 px-3.5 py-2 rounded-lg bg-indigo-500/20 hover:bg-indigo-500/30 text-indigo-200 border border-indigo-400/50 text-xs font-semibold transition-colors cursor-pointer"
            >
              <Vibrate className="w-4 h-4 text-indigo-300" />
              <span>Probar Vibración DS4</span>
            </button>
          </div>
        </div>

        {/* Fast Color Swatch Bar */}
        <div className="mt-4 pt-4 border-t border-slate-800/80 flex flex-wrap items-center justify-between gap-3">
          <span className="text-xs text-slate-400 font-medium">
            Selección rápida de color para el LED frontal de tu mando PS4:
          </span>
          <div className="flex items-center gap-2">
            {DS4_COLOR_HEX.map((hex, idx) => {
              const isSelectedColor =
                config.ds4_lightbar_color === idx && config.ds4_lightbar_mode !== 3;
              return (
                <button
                  key={hex}
                  type="button"
                  onClick={() =>
                    onChangeConfig({
                      ...config,
                      ds4_lightbar_mode: 1,
                      ds4_lightbar_color: idx,
                    })
                  }
                  className={`w-7 h-7 rounded-md border transition-transform cursor-pointer ${
                    isSelectedColor
                      ? 'scale-110 border-white ring-2 ring-cyan-400/50'
                      : 'border-white/20 opacity-75 hover:opacity-100'
                  }`}
                  style={{ backgroundColor: hex }}
                  title={DS4_COLOR_OPTIONS[idx]}
                />
              );
            })}
          </div>
        </div>
      </div>

      {/* Live Descriptive Guide */}
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

      {/* DualShock 4 Configuration Cards */}
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
                        · {card.code}
                      </span>
                    </div>
                    <p className="text-xs text-slate-400 mt-1 leading-relaxed">
                      {card.subtitle}
                    </p>
                  </div>
                </div>

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

      {/* Bottom Hardware Info Strip */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-3 pt-1">
        <div className="p-3.5 rounded-xl bg-[#0B1120] border border-slate-800 flex items-center gap-3">
          <BatteryCharging className="w-5 h-5 text-emerald-400 shrink-0" />
          <p className="text-xs text-slate-300">
            <strong className="text-white">Ahorro Energético Real:</strong> El modo{' '}
            <span className="text-cyan-300">Tenue (15%)</span> o{' '}
            <span className="text-cyan-300">Apagado</span> reduce drásticamente el consumo del LED frontal en mandos PS4 V1 y V2.
          </p>
        </div>
        <div className="p-3.5 rounded-xl bg-[#0B1120] border border-slate-800 flex items-center gap-3">
          <ShieldCheck className="w-5 h-5 text-cyan-400 shrink-0" />
          <p className="text-xs text-slate-300">
            <strong className="text-white">Memoria Por Jugador:</strong> Guardado en{' '}
            <code className="text-cyan-300 font-mono">
              {activeTarget?.ps4Filename || 'ds4_slot_1.ini'}
            </code>{' '}
            sin interferir con otros mandos.
          </p>
        </div>
      </div>
    </div>
  );
}
