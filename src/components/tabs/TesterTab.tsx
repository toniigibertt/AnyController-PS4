/**
 * OneFamily PS4 - Probador de Entradas y Asistente de Calibración (Tester Tab)
 * Prueba de mandos en tiempo real, vibración háptica y calibración guiada de deriva (drift)
 */
import { useState, useEffect } from 'react';
import { AppConfig, LiveGamepadInput, TesterState } from '../../types';
import { DEADZONE_OPTIONS, DEADZONE_VALUES } from '../../constants/options';
import {
  Gamepad2,
  Activity,
  Vibrate,
  Crosshair,
  CheckCircle2,
  Play,
  RotateCcw,
  Sparkles,
  Info,
  Battery,
  BatteryCharging,
  BatteryFull,
  BatteryMedium,
  BatteryLow,
  BatteryWarning,
  Zap,
  Wifi,
  Usb,
  AlertTriangle,
  Sliders,
} from 'lucide-react';

interface TesterTabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  liveInput: LiveGamepadInput;
  activeSlot: number;
  onSelectSlot: (slot: number) => void;
  onTriggerRumble: (phase: number) => void;
}

export default function TesterTab({
  config,
  onChangeConfig,
  liveInput,
  activeSlot,
  onSelectSlot,
  onTriggerRumble,
}: TesterTabProps) {
  const [tester, setTester] = useState<TesterState>({
    rumbleFrames: 0,
    rumblePhase: 0,
    selectedSlot: activeSlot,
    testMode: false,
    calib: {
      active: false,
      completed: false,
      currentFrame: 0,
      totalFrames: 180, // ~3 seconds at 60fps
      maxNoise: 0,
      suggestedIndex: 4,
    },
  });

  // Battery monitoring and simulation states
  const [simulatedBattery, setSimulatedBattery] = useState<number | null>(null);
  const [simulatedCharging, setSimulatedCharging] = useState<boolean | null>(null);
  const [showBatteryDetails, setShowBatteryDetails] = useState<boolean>(false);

  // Active battery metrics
  const activeLevel = simulatedBattery !== null ? simulatedBattery : (liveInput.battery?.level ?? 82);
  const isCharging = simulatedCharging !== null ? simulatedCharging : (liveInput.battery?.charging ?? false);
  const isBluetooth = !isCharging && (liveInput.battery?.connectionType !== 'usb');
  const isApiNative = Boolean(liveInput.battery?.available && simulatedBattery === null);

  // Segment blocks (0..4) matching PS4 System UI
  const batterySegments = isCharging ? 4 : activeLevel > 75 ? 4 : activeLevel > 50 ? 3 : activeLevel > 25 ? 2 : activeLevel > 10 ? 1 : 0;

  // Sticks values mapped to 0..255 and -1..1
  const lx = liveInput.lx;
  const ly = liveInput.ly;
  const rx = liveInput.rx;
  const ry = liveInput.ry;

  const rawLx = Math.round((lx + 1) * 127.5);
  const rawLy = Math.round((ly + 1) * 127.5);
  const rawRx = Math.round((rx + 1) * 127.5);
  const rawRy = Math.round((ry + 1) * 127.5);

  const l2Val = Math.round((liveInput.l2 ?? 0) * 255);
  const r2Val = Math.round((liveInput.r2 ?? 0) * 255);

  // Calibration loop (Fixing the bug from original main.cpp where bestIdx was overwritten)
  useEffect(() => {
    if (!tester.calib.active) return;

    const interval = setInterval(() => {
      setTester((prev) => {
        const cal = prev.calib;
        if (!cal.active) return prev;

        const devLX = Math.abs(rawLx - 128);
        const devLY = Math.abs(rawLy - 128);
        const devRX = Math.abs(rawRx - 128);
        const devRY = Math.abs(rawRy - 128);

        const currentNoise = Math.max(devLX, devLY, devRX, devRY);
        const newMaxNoise = Math.max(cal.maxNoise, currentNoise);
        const nextFrame = cal.currentFrame + 1;

        if (nextFrame >= cal.totalFrames) {
          // Calibration complete!
          // Smallest deadzone >= (newMaxNoise + 2) in DEADZONE_VALUES
          const needed = newMaxNoise + 2;
          let bestIdx = DEADZONE_VALUES.length - 1;
          for (let i = 0; i < DEADZONE_VALUES.length; i++) {
            if (DEADZONE_VALUES[i] >= needed) {
              bestIdx = i;
              break;
            }
          }

          return {
            ...prev,
            calib: {
              ...cal,
              active: false,
              completed: true,
              maxNoise: newMaxNoise,
              suggestedIndex: bestIdx,
              currentFrame: cal.totalFrames,
            },
          };
        }

        return {
          ...prev,
          calib: {
            ...cal,
            currentFrame: nextFrame,
            maxNoise: newMaxNoise,
          },
        };
      });
    }, 16);

    return () => clearInterval(interval);
  }, [tester.calib.active, rawLx, rawLy, rawRx, rawRy]);

  const startCalibration = () => {
    setTester((prev) => ({
      ...prev,
      calib: {
        active: true,
        completed: false,
        currentFrame: 0,
        totalFrames: 180,
        maxNoise: 0,
        suggestedIndex: 4,
      },
    }));
  };

  const applySuggestedDeadzone = () => {
    onChangeConfig({
      ...config,
      deadzone_index: tester.calib.suggestedIndex,
    });
  };

  const handleTestRumble = (phase: number) => {
    setTester((prev) => ({ ...prev, rumblePhase: phase }));
    onTriggerRumble(phase);
    setTimeout(() => {
      setTester((prev) => ({ ...prev, rumblePhase: 0 }));
      onTriggerRumble(0);
    }, 1500);
  };

  const b = liveInput.buttons;

  return (
    <div className="space-y-6">
      {/* Slot Selector & Mode Toggle Bar */}
      <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4 p-4 rounded-2xl bg-black/40 border border-white/10">
        <div className="flex items-center gap-2">
          <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider mr-2">
            Ranura PS4:
          </span>
          {[0, 1, 2, 3].map((slot) => {
            const isSelected = activeSlot === slot;
            return (
              <button
                key={slot}
                onClick={() => onSelectSlot(slot)}
                className={`px-3 py-1.5 rounded-xl text-xs font-bold transition-all cursor-pointer ${
                  isSelected
                    ? 'bg-sky-500 text-slate-950 shadow-[0_0_15px_rgba(56,189,248,0.4)]'
                    : 'bg-white/5 hover:bg-white/10 text-slate-400 hover:text-white border border-white/5'
                }`}
              >
                Jugador {slot + 1}
              </button>
            );
          })}
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => setTester((prev) => ({ ...prev, testMode: !prev.testMode }))}
            className={`flex items-center gap-2 px-3 py-1.5 rounded-xl text-xs font-semibold border transition-all cursor-pointer ${
              tester.testMode
                ? 'bg-amber-500/20 text-amber-300 border-amber-400/50'
                : 'bg-white/5 text-slate-400 border-white/10 hover:text-white'
            }`}
            title="Pulsa R3 o la tecla G para aislar la prueba y que los botones no cambien de menú"
          >
            <Activity className="w-3.5 h-3.5" />
            <span>Modo Aislado [R3]: {tester.testMode ? 'ACTIVO' : 'NORMAL'}</span>
          </button>
        </div>
      </div>

      {/* Visual Battery Level & Power Management Component */}
      <div
        className={`p-5 rounded-2xl border transition-all duration-300 ${
          isCharging
            ? 'bg-gradient-to-br from-emerald-950/40 via-sky-950/30 to-black/60 border-emerald-500/40 shadow-[0_0_25px_rgba(52,211,153,0.15)]'
            : activeLevel <= 15
            ? 'bg-gradient-to-br from-red-950/50 via-red-900/20 to-black/60 border-red-500/60 shadow-[0_0_30px_rgba(239,68,68,0.25)] animate-pulse'
            : activeLevel <= 30
            ? 'bg-gradient-to-br from-amber-950/40 via-black/40 to-black/60 border-amber-500/40 shadow-[0_0_20px_rgba(245,158,11,0.15)]'
            : 'bg-black/40 border-white/10'
        }`}
      >
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          {/* Header & Status Labels */}
          <div className="flex items-center gap-3.5">
            <div
              className={`p-3 rounded-xl border flex items-center justify-center transition-colors ${
                isCharging
                  ? 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40 shadow-[0_0_15px_rgba(52,211,153,0.3)]'
                  : activeLevel <= 15
                  ? 'bg-red-500/20 text-red-400 border-red-500/40 animate-bounce'
                  : activeLevel <= 30
                  ? 'bg-amber-500/20 text-amber-400 border-amber-500/40'
                  : 'bg-sky-500/20 text-sky-400 border-sky-500/40'
              }`}
            >
              {isCharging ? (
                <BatteryCharging className="w-6 h-6 animate-pulse" />
              ) : activeLevel <= 15 ? (
                <BatteryWarning className="w-6 h-6" />
              ) : activeLevel <= 35 ? (
                <BatteryLow className="w-6 h-6" />
              ) : activeLevel <= 70 ? (
                <BatteryMedium className="w-6 h-6" />
              ) : (
                <BatteryFull className="w-6 h-6" />
              )}
            </div>

            <div>
              <div className="flex items-center gap-2.5">
                <h3 className="text-sm font-bold text-white flex items-center gap-2">
                  <span>Nivel de Batería y Alimentación</span>
                  <span className="text-xs px-2 py-0.5 rounded-full bg-white/10 text-slate-300 font-normal">
                    Mando Jugador {activeSlot + 1}
                  </span>
                </h3>
              </div>
              <p className="text-xs text-slate-300 flex items-center gap-2 mt-0.5">
                {isCharging ? (
                  <span className="flex items-center gap-1 text-emerald-400 font-semibold">
                    <Zap className="w-3.5 h-3.5" /> Alimentación continua por Cable USB (Cargando)
                  </span>
                ) : (
                  <span className="flex items-center gap-1 text-sky-300">
                    <Wifi className="w-3.5 h-3.5 text-sky-400" /> Conexión Inalámbrica Bluetooth
                  </span>
                )}
                <span className="text-slate-500">•</span>
                <span className="text-[11px] text-slate-400 font-mono">
                  {isApiNative
                    ? 'Lectura API Hardware Real'
                    : simulatedBattery !== null
                    ? 'Simulación Activa'
                    : 'Telemetría AnyController'}
                </span>
              </p>
            </div>
          </div>

          {/* Quick Metrics & Battery Bar */}
          <div className="flex flex-wrap items-center gap-4">
            {/* 4-Segment PS4 Quick Menu Style Indicator */}
            <div className="flex flex-col items-end gap-1">
              <div className="flex items-center gap-2">
                <span className="text-xs font-mono font-bold text-white">
                  {activeLevel}%
                </span>
                {/* Visual Battery Shell with 4 Segments */}
                <div className="relative flex items-center p-1 rounded-lg bg-black/60 border border-white/20 gap-1 w-28 h-7">
                  {[1, 2, 3, 4].map((seg) => {
                    const isFilled = isCharging ? true : seg <= batterySegments;
                    return (
                      <div
                        key={seg}
                        className={`flex-1 h-full rounded-sm transition-all duration-300 ${
                          isFilled
                            ? isCharging
                              ? 'bg-emerald-400 shadow-[0_0_8px_#34d399]'
                              : activeLevel <= 15
                              ? 'bg-red-500 shadow-[0_0_8px_#ef4444]'
                              : activeLevel <= 30
                              ? 'bg-amber-400 shadow-[0_0_8px_#f59e0b]'
                              : 'bg-sky-400 shadow-[0_0_8px_#38bdf8]'
                            : 'bg-white/5 border border-white/5'
                        }`}
                      />
                    );
                  })}
                  {/* Battery Nipple Tip */}
                  <div className="absolute -right-1.5 top-1/2 -translate-y-1/2 w-1 h-3 rounded-r-sm bg-white/20" />
                </div>
              </div>

              <span className="text-[10px] text-slate-400 font-mono">
                {isCharging
                  ? 'Carga en curso (5V)'
                  : activeLevel <= 15
                  ? 'Autonomía: < 25 min'
                  : activeLevel <= 30
                  ? 'Autonomía: ~2-3 h'
                  : 'Autonomía: ~6-8 h'}
              </span>
            </div>

            {/* Test & Simulation Controls Toggle Button */}
            <button
              onClick={() => setShowBatteryDetails(!showBatteryDetails)}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-white/10 hover:bg-white/20 text-white font-medium text-xs border border-white/15 transition-all cursor-pointer"
              title="Ajustar nivel simulado para probar notificaciones de batería en PS4"
            >
              <Sliders className="w-3.5 h-3.5 text-sky-400" />
              <span>{showBatteryDetails ? 'Ocultar Opciones' : 'Probar Alertas'}</span>
            </button>
          </div>
        </div>

        {/* Dynamic Advice & Charge Warning Notice */}
        <div className="mt-3 pt-3 border-t border-white/10 flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3 text-xs">
          <div className="flex items-center gap-2">
            {activeLevel <= 15 && !isCharging ? (
              <span className="flex items-center gap-1.5 text-red-400 font-bold">
                <AlertTriangle className="w-4 h-4 shrink-0 animate-bounce" />
                ¡ATENCIÓN: BATERÍA MUY BAJA! Conecta el cable USB a la consola PS4 para evitar que el mando se desconecte en plena partida.
              </span>
            ) : activeLevel <= 30 && !isCharging ? (
              <span className="flex items-center gap-1.5 text-amber-300">
                <AlertTriangle className="w-4 h-4 shrink-0" />
                Nivel moderado de carga. AnyController recomienda recargar el mando tras la sesión de juego.
              </span>
            ) : isCharging ? (
              <span className="flex items-center gap-1.5 text-emerald-400 font-medium">
                <CheckCircle2 className="w-4 h-4 shrink-0" />
                Mando recibiendo energía correctamente. Puedes seguir jugando sin preocuparte por la batería.
              </span>
            ) : (
              <span className="flex items-center gap-1.5 text-slate-300">
                <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0" />
                Batería en estado óptimo. Rendimiento Bluetooth al 100% con latencia mínima de 4 ms.
              </span>
            )}
          </div>

          <span className="text-[11px] text-slate-400 font-mono shrink-0">
            Aviso PS4 en pantalla al caer a &lt; 15%
          </span>
        </div>

        {/* Expandable Simulation & Testing Drawer */}
        {showBatteryDetails && (
          <div className="mt-4 pt-4 border-t border-white/10 bg-black/40 -mx-5 -mb-5 p-5 rounded-b-2xl space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2">
              <div className="space-y-0.5">
                <h4 className="text-xs font-bold text-white flex items-center gap-1.5">
                  <Sparkles className="w-3.5 h-3.5 text-sky-400" />
                  Simulador de Umbrales de Batería y Notificaciones PS4
                </h4>
                <p className="text-[11px] text-slate-400">
                  Prueba cómo reacciona la interfaz y comprueba los niveles a los que AnyController avisará en tu televisor:
                </p>
              </div>

              {simulatedBattery !== null && (
                <button
                  onClick={() => {
                    setSimulatedBattery(null);
                    setSimulatedCharging(null);
                  }}
                  className="px-2.5 py-1 rounded-lg bg-white/10 hover:bg-white/20 text-sky-300 text-[11px] font-semibold transition-all cursor-pointer"
                >
                  Restablecer a Valor Real
                </button>
              )}
            </div>

            {/* Slider & Presets */}
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4 pt-1">
              {/* Slider */}
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-2">
                <div className="flex justify-between text-xs">
                  <span className="text-slate-300 font-semibold">Nivel Manual:</span>
                  <span className="font-mono text-sky-400 font-bold">{activeLevel}%</span>
                </div>
                <input
                  type="range"
                  min="5"
                  max="100"
                  step="5"
                  value={activeLevel}
                  onChange={(e) => setSimulatedBattery(Number(e.target.value))}
                  className="w-full accent-sky-400 cursor-pointer"
                />
                <div className="flex justify-between text-[10px] text-slate-500 font-mono">
                  <span className="text-red-400">5% Crítico</span>
                  <span className="text-amber-400">25% Bajo</span>
                  <span className="text-sky-400">50% Medio</span>
                  <span className="text-emerald-400">100% Lleno</span>
                </div>
              </div>

              {/* Quick Presets */}
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-2">
                <span className="text-xs text-slate-300 font-semibold block">Pruebas Rápidas:</span>
                <div className="flex flex-wrap gap-2">
                  <button
                    onClick={() => {
                      setSimulatedBattery(10);
                      setSimulatedCharging(false);
                    }}
                    className="px-2.5 py-1 rounded-lg bg-red-500/20 hover:bg-red-500/30 text-red-300 border border-red-500/40 text-xs font-bold transition-all cursor-pointer"
                  >
                    10% (Alerta Crítica)
                  </button>
                  <button
                    onClick={() => {
                      setSimulatedBattery(25);
                      setSimulatedCharging(false);
                    }}
                    className="px-2.5 py-1 rounded-lg bg-amber-500/20 hover:bg-amber-500/30 text-amber-300 border border-amber-500/40 text-xs font-bold transition-all cursor-pointer"
                  >
                    25% (Aviso Bajo)
                  </button>
                  <button
                    onClick={() => {
                      setSimulatedBattery(80);
                      setSimulatedCharging(false);
                    }}
                    className="px-2.5 py-1 rounded-lg bg-sky-500/20 hover:bg-sky-500/30 text-sky-300 border border-sky-500/40 text-xs font-bold transition-all cursor-pointer"
                  >
                    80% (Óptimo)
                  </button>
                  <button
                    onClick={() => {
                      setSimulatedCharging(!isCharging);
                      if (simulatedBattery === null) setSimulatedBattery(activeLevel);
                    }}
                    className={`flex items-center gap-1 px-2.5 py-1 rounded-lg text-xs font-bold border transition-all cursor-pointer ${
                      isCharging
                        ? 'bg-emerald-500 text-slate-950 border-emerald-400'
                        : 'bg-white/10 text-white border-white/20'
                    }`}
                  >
                    <Usb className="w-3.5 h-3.5" />
                    <span>{isCharging ? 'Conectado por Cable' : 'Cambiar a Cable'}</span>
                  </button>
                </div>
              </div>
            </div>

            {/* Explanatory note */}
            <div className="p-3 rounded-lg bg-white/[0.02] border border-white/5 text-[11px] text-slate-400 flex items-start gap-2">
              <Info className="w-4 h-4 text-sky-400 shrink-0 mt-0.5" />
              <span>
                <strong>¿Cómo funciona en tu consola PS4?</strong> El demonio de AnyController instalado en GoldHEN lee constantemente el paquete HID Bluetooth (reporte de 78 bytes del DualSense). Si la batería cae por debajo del <strong>15%</strong>, la barra de luz parpadeará sutilmente y la PS4 mostrará el aviso del sistema en la esquina para recordarte conectar el cable de carga.
              </span>
            </div>
          </div>
        )}
      </div>

      {/* Main Grid: Visual Gamepad & Real-time Sensors */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Left Column: Visual Sticks & Triggers (7 cols) */}
        <div className="lg:col-span-7 space-y-6">
          {/* Dual Analog Sticks Visualizer */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10">
            <h3 className="text-sm font-bold text-white flex items-center justify-between">
              <span className="flex items-center gap-2">
                <Crosshair className="w-4 h-4 text-sky-400" />
                Telemetría de Palancas Analógicas
              </span>
              <span className="text-[11px] text-sky-300/80 font-mono">
                Zona Muerta Actual: {DEADZONE_OPTIONS[config.deadzone_index].split(' ')[0]}
              </span>
            </h3>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-6 mt-5">
              {/* Left Stick (LX, LY) */}
              <div className="flex flex-col items-center p-4 rounded-xl bg-white/[0.02] border border-white/5">
                <span className="text-xs font-semibold text-slate-300 mb-2">
                  Palanca Izquierda (L3)
                </span>
                <div className="relative w-40 h-40 rounded-full border-2 border-sky-500/40 bg-gradient-to-br from-sky-950/40 to-black flex items-center justify-center overflow-hidden">
                  {/* Deadzone Circle Overlay */}
                  <div
                    className="absolute rounded-full border border-sky-400/30 bg-sky-500/10 pointer-events-none"
                    style={{
                      width: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                      height: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                    }}
                  />
                  {/* Axis Crosshairs */}
                  <div className="absolute w-full h-[1px] bg-white/10" />
                  <div className="absolute h-full w-[1px] bg-white/10" />

                  {/* Stick Position Dot */}
                  <div
                    className={`absolute w-7 h-7 rounded-full shadow-lg transition-transform duration-75 flex items-center justify-center font-bold text-[9px] ${
                      b['l3']
                        ? 'bg-amber-400 text-slate-950 scale-90 ring-4 ring-amber-400/40'
                        : 'bg-sky-400 text-slate-950'
                    }`}
                    style={{
                      transform: `translate(${lx * 56}px, ${ly * 56}px)`,
                    }}
                  >
                    L
                  </div>
                </div>

                <div className="w-full mt-3 flex justify-between text-[11px] font-mono text-slate-400">
                  <span>X: <strong className="text-white">{rawLx}</strong> ({Math.round(lx * 100)}%)</span>
                  <span>Y: <strong className="text-white">{rawLy}</strong> ({Math.round(ly * 100)}%)</span>
                </div>
              </div>

              {/* Right Stick (RX, RY) */}
              <div className="flex flex-col items-center p-4 rounded-xl bg-white/[0.02] border border-white/5">
                <span className="text-xs font-semibold text-slate-300 mb-2">
                  Palanca Derecha (R3)
                </span>
                <div className="relative w-40 h-40 rounded-full border-2 border-sky-500/40 bg-gradient-to-br from-sky-950/40 to-black flex items-center justify-center overflow-hidden">
                  {/* Deadzone Circle Overlay */}
                  <div
                    className="absolute rounded-full border border-sky-400/30 bg-sky-500/10 pointer-events-none"
                    style={{
                      width: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                      height: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                    }}
                  />
                  {/* Axis Crosshairs */}
                  <div className="absolute w-full h-[1px] bg-white/10" />
                  <div className="absolute h-full w-[1px] bg-white/10" />

                  {/* Stick Position Dot */}
                  <div
                    className={`absolute w-7 h-7 rounded-full shadow-lg transition-transform duration-75 flex items-center justify-center font-bold text-[9px] ${
                      b['r3']
                        ? 'bg-amber-400 text-slate-950 scale-90 ring-4 ring-amber-400/40'
                        : 'bg-sky-400 text-slate-950'
                    }`}
                    style={{
                      transform: `translate(${rx * 56}px, ${ry * 56}px)`,
                    }}
                  >
                    R
                  </div>
                </div>

                <div className="w-full mt-3 flex justify-between text-[11px] font-mono text-slate-400">
                  <span>X: <strong className="text-white">{rawRx}</strong> ({Math.round(rx * 100)}%)</span>
                  <span>Y: <strong className="text-white">{rawRy}</strong> ({Math.round(ry * 100)}%)</span>
                </div>
              </div>
            </div>
          </div>

          {/* Analog Triggers (L2 / R2) */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10">
            <h3 className="text-sm font-bold text-white flex items-center justify-between">
              <span>Presión Analógica de Gatillos (L2 / R2)</span>
              <span className="text-[11px] text-sky-300/80 font-mono">
                Umbral Hair Trigger: {config.trigger_index === 0 ? '15' : '30'}
              </span>
            </h3>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 mt-4">
              {/* L2 Bar */}
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-1.5">
                <div className="flex justify-between text-xs">
                  <span className="font-semibold text-slate-300">Gatillo L2</span>
                  <span className="font-mono text-sky-400 font-bold">{l2Val} / 255</span>
                </div>
                <div className="w-full h-3 bg-white/10 rounded-full overflow-hidden">
                  <div
                    className="h-full bg-gradient-to-r from-sky-500 to-cyan-400 transition-all duration-75"
                    style={{ width: `${(l2Val / 255) * 100}%` }}
                  />
                </div>
              </div>

              {/* R2 Bar */}
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-1.5">
                <div className="flex justify-between text-xs">
                  <span className="font-semibold text-slate-300">Gatillo R2</span>
                  <span className="font-mono text-sky-400 font-bold">{r2Val} / 255</span>
                </div>
                <div className="w-full h-3 bg-white/10 rounded-full overflow-hidden">
                  <div
                    className="h-full bg-gradient-to-r from-sky-500 to-cyan-400 transition-all duration-75"
                    style={{ width: `${(r2Val / 255) * 100}%` }}
                  />
                </div>
              </div>
            </div>
          </div>

          {/* Rumble Motor Test */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10">
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <Vibrate className="w-4 h-4 text-sky-400" />
              Prueba de Motores de Vibración (Rumble Háptico)
            </h3>
            <p className="text-xs text-slate-400 mt-1">
              Envía pulsos de prueba a los actuadores físicos conectados a través de la Web Gamepad API.
            </p>

            <div className="grid grid-cols-3 gap-3 mt-4">
              <button
                onClick={() => handleTestRumble(1)}
                className={`py-2 px-3 rounded-xl text-xs font-semibold border transition-all cursor-pointer ${
                  tester.rumblePhase === 1
                    ? 'bg-sky-500 text-slate-950 border-sky-400 shadow-[0_0_15px_#38bdf8]'
                    : 'bg-white/5 hover:bg-white/10 text-slate-200 border-white/10'
                }`}
              >
                Motor Pesado (L)
              </button>
              <button
                onClick={() => handleTestRumble(2)}
                className={`py-2 px-3 rounded-xl text-xs font-semibold border transition-all cursor-pointer ${
                  tester.rumblePhase === 2
                    ? 'bg-sky-500 text-slate-950 border-sky-400 shadow-[0_0_15px_#38bdf8]'
                    : 'bg-white/5 hover:bg-white/10 text-slate-200 border-white/10'
                }`}
              >
                Motor Ligero (R)
              </button>
              <button
                onClick={() => handleTestRumble(3)}
                className={`py-2 px-3 rounded-xl text-xs font-semibold border transition-all cursor-pointer ${
                  tester.rumblePhase === 3
                    ? 'bg-emerald-500 text-slate-950 border-emerald-400 shadow-[0_0_15px_#34d399]'
                    : 'bg-white/5 hover:bg-white/10 text-slate-200 border-white/10'
                }`}
              >
                Ambos Motores (100%)
              </button>
            </div>
          </div>
        </div>

        {/* Right Column: Drift Calibration Wizard & Real-time Buttons (5 cols) */}
        <div className="lg:col-span-5 space-y-6">
          {/* Automated Stick Drift Calibration Wizard */}
          <div className="p-5 rounded-2xl bg-gradient-to-br from-[#061e3d] via-[#021024] to-black border border-sky-500/30 shadow-[0_0_30px_rgba(0,112,209,0.2)]">
            <div className="flex items-center justify-between">
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <Sparkles className="w-4 h-4 text-sky-400" />
                Asistente de Calibración de Deriva (Drift)
              </h3>
            </div>
            <p className="text-xs text-sky-200/70 mt-1">
              Muestrea automáticamente el ruido eléctrico de centrado de tus palancas analógicas para calcular la zona muerta perfecta.
            </p>

            <div className="mt-4 p-4 rounded-xl bg-black/50 border border-white/10 space-y-3">
              {!tester.calib.active && !tester.calib.completed && (
                <div className="space-y-3">
                  <div className="flex items-start gap-2 text-xs text-slate-300">
                    <Info className="w-4 h-4 text-sky-400 shrink-0 mt-0.5" />
                    <span>
                      Suelta las palancas analógicas en su posición neutra de reposo y presiona el botón para iniciar la prueba de 3 segundos.
                    </span>
                  </div>
                  <button
                    onClick={startCalibration}
                    className="w-full flex items-center justify-center gap-2 py-2.5 rounded-xl bg-sky-500 hover:bg-sky-400 text-slate-950 font-bold text-xs shadow-[0_0_15px_rgba(56,189,248,0.4)] transition-all cursor-pointer"
                  >
                    <Play className="w-4 h-4" />
                    <span>Iniciar Calibración de Ruido</span>
                  </button>
                </div>
              )}

              {tester.calib.active && (
                <div className="space-y-3 text-center py-2">
                  <div className="text-xs font-semibold text-sky-300 animate-pulse">
                    Muestreando centro neutro... ¡No toques las palancas!
                  </div>
                  <div className="w-full h-3 bg-white/10 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-gradient-to-r from-sky-400 to-emerald-400 transition-all duration-75"
                      style={{
                        width: `${(tester.calib.currentFrame / tester.calib.totalFrames) * 100}%`,
                      }}
                    />
                  </div>
                  <div className="text-[11px] font-mono text-slate-400">
                    Ruido Máximo Detectado: <strong className="text-white">{tester.calib.maxNoise}</strong>
                  </div>
                </div>
              )}

              {tester.calib.completed && (
                <div className="space-y-3">
                  <div className="flex items-center gap-2 text-xs text-emerald-400 font-semibold">
                    <CheckCircle2 className="w-4 h-4" />
                    <span>¡Calibración Completada con Éxito!</span>
                  </div>
                  <div className="p-3 rounded-lg bg-sky-950/60 border border-sky-500/30 text-xs text-slate-300 space-y-1">
                    <p>
                      Desviación de ruido neutro detectada:{' '}
                      <strong className="text-white font-mono">{tester.calib.maxNoise} / 127</strong>
                    </p>
                    <p>
                      Ajuste óptimo recomendado:{' '}
                      <strong className="text-sky-300">
                        {DEADZONE_OPTIONS[tester.calib.suggestedIndex]}
                      </strong>
                    </p>
                  </div>
                  <div className="flex gap-2">
                    <button
                      onClick={applySuggestedDeadzone}
                      className="flex-1 py-2 px-3 rounded-xl bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-bold text-xs shadow-[0_0_15px_rgba(52,211,153,0.3)] transition-all cursor-pointer"
                    >
                      Aplicar Zona Muerta
                    </button>
                    <button
                      onClick={startCalibration}
                      className="p-2 rounded-xl bg-white/10 hover:bg-white/20 text-white transition-all cursor-pointer"
                      title="Repetir calibración"
                    >
                      <RotateCcw className="w-4 h-4" />
                    </button>
                  </div>
                </div>
              )}
            </div>
          </div>

          {/* Real-time Pressed Buttons Matrix */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10">
            <h3 className="text-sm font-bold text-white flex items-center justify-between mb-4">
              <span className="flex items-center gap-2">
                <Gamepad2 className="w-4 h-4 text-sky-400" />
                Detección de Botones en Vivo
              </span>
              <span className="text-[10px] text-slate-400">Teclado o Mando</span>
            </h3>

            {/* D-Pad & Action Buttons Grid */}
            <div className="grid grid-cols-2 gap-3">
              {/* D-Pad Buttons */}
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-2">
                <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider">
                  Cruceta (D-Pad)
                </span>
                <div className="grid grid-cols-3 gap-1.5 w-28 mx-auto">
                  <div />
                  <div
                    className={`h-7 rounded flex items-center justify-center text-[10px] font-bold border transition-colors ${
                      b['dpad_up']
                        ? 'bg-sky-400 text-slate-950 border-sky-300 shadow-[0_0_10px_#38bdf8]'
                        : 'bg-white/5 text-slate-400 border-white/10'
                    }`}
                  >
                    ▲
                  </div>
                  <div />
                  <div
                    className={`h-7 rounded flex items-center justify-center text-[10px] font-bold border transition-colors ${
                      b['dpad_left']
                        ? 'bg-sky-400 text-slate-950 border-sky-300 shadow-[0_0_10px_#38bdf8]'
                        : 'bg-white/5 text-slate-400 border-white/10'
                    }`}
                  >
                    ◀
                  </div>
                  <div
                    className={`h-7 rounded flex items-center justify-center text-[10px] font-bold border transition-colors ${
                      b['dpad_down']
                        ? 'bg-sky-400 text-slate-950 border-sky-300 shadow-[0_0_10px_#38bdf8]'
                        : 'bg-white/5 text-slate-400 border-white/10'
                    }`}
                  >
                    ▼
                  </div>
                  <div
                    className={`h-7 rounded flex items-center justify-center text-[10px] font-bold border transition-colors ${
                      b['dpad_right']
                        ? 'bg-sky-400 text-slate-950 border-sky-300 shadow-[0_0_10px_#38bdf8]'
                        : 'bg-white/5 text-slate-400 border-white/10'
                    }`}
                  >
                    ▶
                  </div>
                </div>
              </div>

              {/* Action Face Buttons (Cross, Circle, Square, Triangle) */}
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-2">
                <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider">
                  Acción Frontal
                </span>
                <div className="grid grid-cols-2 gap-2">
                  <div
                    className={`py-1.5 px-2 rounded-lg text-xs font-bold flex items-center justify-center gap-1.5 border transition-colors ${
                      b['cross']
                        ? 'bg-blue-500 text-white border-blue-400 shadow-[0_0_10px_#3b82f6]'
                        : 'bg-white/5 text-slate-300 border-white/10'
                    }`}
                  >
                    <span>✕</span> <span className="text-[10px]">Cruz</span>
                  </div>
                  <div
                    className={`py-1.5 px-2 rounded-lg text-xs font-bold flex items-center justify-center gap-1.5 border transition-colors ${
                      b['circle']
                        ? 'bg-red-500 text-white border-red-400 shadow-[0_0_10px_#ef4444]'
                        : 'bg-white/5 text-slate-300 border-white/10'
                    }`}
                  >
                    <span>○</span> <span className="text-[10px]">Círculo</span>
                  </div>
                  <div
                    className={`py-1.5 px-2 rounded-lg text-xs font-bold flex items-center justify-center gap-1.5 border transition-colors ${
                      b['square']
                        ? 'bg-pink-500 text-white border-pink-400 shadow-[0_0_10px_#ec4899]'
                        : 'bg-white/5 text-slate-300 border-white/10'
                    }`}
                  >
                    <span>□</span> <span className="text-[10px]">Cuadrado</span>
                  </div>
                  <div
                    className={`py-1.5 px-2 rounded-lg text-xs font-bold flex items-center justify-center gap-1.5 border transition-colors ${
                      b['triangle']
                        ? 'bg-emerald-500 text-slate-950 border-emerald-400 shadow-[0_0_10px_#10b981]'
                        : 'bg-white/5 text-slate-300 border-white/10'
                    }`}
                  >
                    <span>△</span> <span className="text-[10px]">Triángulo</span>
                  </div>
                </div>
              </div>
            </div>

            {/* System & Shoulder Buttons Badges */}
            <div className="grid grid-cols-4 gap-2 mt-3">
              {[
                { name: 'L1', active: b['l1'] },
                { name: 'R1', active: b['r1'] },
                { name: 'Share', active: b['share'] },
                { name: 'Options', active: b['options'] },
                { name: 'Botón PS', active: b['ps'] },
                { name: 'Touchpad', active: b['touchpad'] },
                { name: 'L3 Click', active: b['l3'] },
                { name: 'R3 Click', active: b['r3'] },
              ].map((btn, idx) => (
                <div
                  key={idx}
                  className={`py-1.5 px-1 rounded-lg text-[10px] font-bold text-center border transition-colors ${
                    btn.active
                      ? 'bg-sky-400 text-slate-950 border-sky-300 shadow-[0_0_8px_#38bdf8]'
                      : 'bg-white/5 text-slate-400 border-white/10'
                  }`}
                >
                  {btn.name}
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
