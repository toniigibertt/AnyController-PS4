/**
 * AnyController Studio PS4 - Módulo Nativo para Mandos DualShock 4 (PS4)
 * Control directo de barra de luz frontal, ahorro de batería, motores de vibración y Sixaxis
 */
import { AppConfig } from '../../types';
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
} from 'lucide-react';

interface DualShock4TabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  selectedCard: number;
  onSelectCard: (index: number) => void;
  onTriggerRumble?: (phase: number) => void;
  onShowToast?: (msg: string) => void;
}

export default function DualShock4Tab({
  config,
  onChangeConfig,
  selectedCard,
  onSelectCard,
  onTriggerRumble,
  onShowToast,
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

  return (
    <div className="space-y-6">
      {/* Native PS4 DualShock 4 Studio Header Deck */}
      <div className="relative overflow-hidden rounded-xl bg-[#0B1120] border border-indigo-500/30 p-5 shadow-xl">
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
              <Gamepad className="w-6 h-6" style={{ color: activeHex }} />
            </div>
            <div>
              <div className="flex flex-wrap items-center gap-2.5">
                <h2 className="text-base md:text-lg font-bold text-white tracking-tight">
                  Configurador Nativo Sony DualShock 4 (PS4)
                </h2>
                <span className="px-2 py-0.5 text-[11px] font-mono uppercase tracking-wider bg-indigo-950/90 text-indigo-300 border border-indigo-500/30 rounded">
                  CUH-ZCT1 / CUH-ZCT2
                </span>
              </div>
              <p className="text-xs text-slate-400 mt-1">
                Control nativo directo vía <span className="font-mono text-cyan-400">libScePad</span> • Sin emulación intermedia • Compatible con todos los mandos originales de PS4
              </p>
            </div>
          </div>

          {/* Interactive Lightbar Prism Preview & Rumble Test */}
          <div className="flex flex-wrap items-center gap-3">
            <div className="flex items-center gap-3 bg-[#070B14] border border-slate-800 px-3.5 py-2 rounded-lg">
              <span className="text-[11px] font-mono uppercase tracking-wider text-slate-400">
                Prisma Frontal:
              </span>
              <div
                className="w-20 h-3.5 rounded-sm border border-white/30 transition-all duration-300"
                style={{
                  backgroundColor: activeHex,
                  boxShadow:
                    config.ds4_lightbar_mode === 3
                      ? 'none'
                      : `0 0 14px ${activeHex}`,
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
              onClick={() => {
                onTriggerRumble?.(3);
                onShowToast?.('Probando motores de vibración del mando DualShock 4...');
              }}
              className="flex items-center gap-2 px-3.5 py-2 rounded-lg bg-indigo-600/20 hover:bg-indigo-600/30 text-indigo-200 border border-indigo-500/40 text-xs font-semibold transition-colors cursor-pointer"
            >
              <Vibrate className="w-4 h-4 text-indigo-400" />
              <span>Probar Vibración DS4</span>
            </button>
          </div>
        </div>

        {/* Fast Color Swatch Bar */}
        <div className="mt-4 pt-4 border-t border-slate-800/80 flex flex-wrap items-center justify-between gap-3 pl-2">
          <span className="text-xs text-slate-400 font-medium">
            Selección rápida de color para el LED frontal de tu mando PS4:
          </span>
          <div className="flex items-center gap-2">
            {DS4_COLOR_HEX.map((hex, idx) => {
              const isSelectedColor =
                config.ds4_lightbar_color === idx &&
                config.ds4_lightbar_mode !== 3;
              return (
                <button
                  key={hex}
                  onClick={() =>
                    onChangeConfig({
                      ...config,
                      ds4_lightbar_mode: 1,
                      ds4_lightbar_color: idx,
                    })
                  }
                  className={`w-7 h-7 rounded-md border transition-transform cursor-pointer flex items-center justify-center ${
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

      {/* DualShock 4 Configuration Cards */}
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
                        {card.code}
                      </span>
                    </div>
                    <p className="text-xs text-slate-400 mt-1 leading-relaxed">
                      {card.subtitle}
                    </p>
                  </div>
                </div>

                {/* Value Selector Box */}
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

      {/* Bottom Hardware Info Strip */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-3 pt-1">
        <div className="p-3.5 rounded-xl bg-[#0B1120]/80 border border-slate-800 flex items-center gap-3">
          <BatteryCharging className="w-5 h-5 text-emerald-400 shrink-0" />
          <p className="text-xs text-slate-300">
            <strong className="text-white">Ahorro Energético Real:</strong> El modo{' '}
            <span className="text-cyan-300">Tenue (15%)</span> o{' '}
            <span className="text-cyan-300">Apagado</span> reduce drásticamente el consumo del LED frontal en mandos PS4 V1 y V2.
          </p>
        </div>
        <div className="p-3.5 rounded-xl bg-[#0B1120]/80 border border-slate-800 flex items-center gap-3">
          <ShieldCheck className="w-5 h-5 text-cyan-400 shrink-0" />
          <p className="text-xs text-slate-300">
            <strong className="text-white">Separación Total:</strong> Estos ajustes se guardan en la sección{' '}
            <code className="text-cyan-300 font-mono">[DualShock4]</code> sin interferir con tu mando PS5 DualSense.
          </p>
        </div>
      </div>
    </div>
  );
}
