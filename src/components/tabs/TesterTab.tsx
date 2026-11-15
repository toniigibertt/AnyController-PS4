/**
 * Controller Control Center PS4 - Laboratorio 2D y Calibración de Deriva (v1.5.0)
 * Prueba de mandos originales PS5/PS4 en 2D, cambio manual de ranura sin conflictos y calibración guiada.
 */
import { useState, useEffect } from 'react';
import {
  AppConfig,
  ConnectedGamepadInfo,
  Controller2DModel,
  LiveGamepadInput,
  TesterState,
} from '../../types';
import {
  DEADZONE_OPTIONS,
  DEADZONE_VALUES,
  TRIGGER_VALUES,
} from '../../constants/options';
import Controller2DViewer from '../Controller2DViewer';
import {
  Activity,
  Vibrate,
  Crosshair,
  CheckCircle2,
  Play,
  RotateCcw,
  Sparkles,
  Info,
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
  connectedGamepads: ConnectedGamepadInfo[];
  hardwareDetected: boolean;
  onSelectSlot: (slot: number) => void;
  onTriggerRumble: (phase: number) => void;
  activeControllerMode?: Controller2DModel;
  onSelectControllerMode?: (mode: Controller2DModel) => void;
  testMode: boolean;
  onToggleTestMode: (enabled: boolean) => void;
}

export default function TesterTab({
  config,
  onChangeConfig,
  liveInput,
  activeSlot,
  connectedGamepads,
  hardwareDetected,
  onSelectSlot,
  onTriggerRumble,
  activeControllerMode = 'dualsense',
  onSelectControllerMode,
  testMode,
  onToggleTestMode,
}: TesterTabProps) {
  const [viewerMode, setViewerMode] = useState<Controller2DModel>(activeControllerMode);

  useEffect(() => {
    setViewerMode(activeControllerMode);
  }, [activeControllerMode]);

  const [tester, setTester] = useState<TesterState>({
    rumbleFrames: 0,
    rumblePhase: 0,
    selectedSlot: activeSlot,
    testMode,
    calib: {
      active: false,
      completed: false,
      currentFrame: 0,
      totalFrames: 180,
      maxNoise: 0,
      suggestedIndex: 4,
    },
  });

  const [simulatedBattery, setSimulatedBattery] = useState<number | null>(null);
  const [simulatedCharging, setSimulatedCharging] = useState<boolean | null>(null);
  const [showBatteryDetails, setShowBatteryDetails] = useState<boolean>(false);

  const activeLevel =
    simulatedBattery !== null ? simulatedBattery : liveInput.battery?.level ?? 82;
  const isCharging =
    simulatedCharging !== null
      ? simulatedCharging
      : liveInput.battery?.charging ?? false;
  const isApiNative = Boolean(
    liveInput.battery?.available && simulatedBattery === null
  );

  const batterySegments = isCharging
    ? 4
    : activeLevel > 75
    ? 4
    : activeLevel > 50
    ? 3
    : activeLevel > 25
    ? 2
    : activeLevel > 10
    ? 1
    : 0;

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
  const triggerThresholdVal = TRIGGER_VALUES[config.trigger_index] ?? 30;

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
    <div className="space-y-5">
      {/* Slot Selector & Isolation Mode Bar */}
      <div className="flex flex-col lg:flex-row items-start lg:items-center justify-between gap-4 p-4 rounded-xl bg-[#0B1120] border border-slate-800">
        <div className="flex flex-wrap items-center gap-2">
          <span className="text-xs font-semibold text-slate-400 mr-1">
            Cambiar de mando manualmente:
          </span>
          {[0, 1, 2, 3].map((slot) => {
            const isSelected = activeSlot === slot;
            const gpInfo = connectedGamepads.find((g) => g.slot === slot);
            return (
              <button
                key={slot}
                type="button"
                onClick={() => onSelectSlot(slot)}
                className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-bold border transition-colors cursor-pointer whitespace-nowrap ${
                  isSelected
                    ? 'bg-cyan-400 text-slate-950 border-cyan-300'
                    : 'bg-[#070B14] hover:bg-slate-900 text-slate-300 border-slate-800'
                }`}
              >
                <span>Jugador {slot + 1}</span>
                {gpInfo && (
                  <span
                    className={`w-1.5 h-1.5 rounded-full ${
                      isSelected ? 'bg-slate-950' : 'bg-emerald-400'
                    }`}
                    title={gpInfo.id}
                  />
                )}
              </button>
            );
          })}
        </div>

        <button
          type="button"
          onClick={() => onToggleTestMode(!testMode)}
          className={`flex items-center gap-2 px-3.5 py-1.5 rounded-lg text-xs font-semibold border transition-colors cursor-pointer whitespace-nowrap ${
            testMode
              ? 'bg-amber-500/20 text-amber-200 border-amber-400/60'
              : 'bg-[#070B14] text-slate-300 border-slate-800 hover:border-slate-700'
          }`}
          title="Bloquea los atajos de teclado Q/E/1..6 para probar entradas libremente sin cambiar de pestaña"
        >
          <Activity className="w-3.5 h-3.5" />
          <span>
            Modo Prueba Libre: {testMode ? 'ACTIVO (Atajos bloqueados)' : 'INACTIVO'}
          </span>
        </button>
      </div>

      {/* Original 2D Interactive Controller Showcase */}
      <Controller2DViewer
        mode={viewerMode}
        onModeChange={(next) => {
          setViewerMode(next);
          onSelectControllerMode?.(next);
        }}
        liveInput={liveInput}
        config={config}
        rumblePhase={tester.rumblePhase}
        batteryLevel={activeLevel}
        hardwareDetected={hardwareDetected}
      />

      {/* Battery Telemetry & Alert Tester */}
      <div
        className={`p-5 rounded-xl border transition-colors ${
          isCharging
            ? 'bg-[#081A18] border-emerald-500/40'
            : activeLevel <= 15
            ? 'bg-[#1D0B10] border-red-500/50'
            : activeLevel <= 30
            ? 'bg-[#1A1409] border-amber-500/40'
            : 'bg-[#0B1120] border-slate-800'
        }`}
      >
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center gap-3.5">
            <div
              className={`p-2.5 rounded-lg border flex items-center justify-center ${
                isCharging
                  ? 'bg-emerald-500/15 text-emerald-400 border-emerald-500/40'
                  : activeLevel <= 15
                  ? 'bg-red-500/15 text-red-400 border-red-500/40'
                  : activeLevel <= 30
                  ? 'bg-amber-500/15 text-amber-400 border-amber-500/40'
                  : 'bg-cyan-500/15 text-cyan-400 border-cyan-500/40'
              }`}
            >
              {isCharging ? (
                <BatteryCharging className="w-5 h-5" />
              ) : activeLevel <= 15 ? (
                <BatteryWarning className="w-5 h-5" />
              ) : activeLevel <= 35 ? (
                <BatteryLow className="w-5 h-5" />
              ) : activeLevel <= 70 ? (
                <BatteryMedium className="w-5 h-5" />
              ) : (
                <BatteryFull className="w-5 h-5" />
              )}
            </div>

            <div>
              <h3 className="text-sm font-bold text-white flex flex-wrap items-center gap-2">
                <span>Nivel de Batería y Alimentación</span>
                <span className="text-xs font-mono font-normal text-slate-400 tabular-nums">
                  · Jugador {activeSlot + 1}
                </span>
              </h3>
              <p className="text-xs text-slate-300 flex flex-wrap items-center gap-2 mt-0.5">
                {isCharging ? (
                  <span className="flex items-center gap-1 text-emerald-400 font-semibold">
                    <Zap className="w-3.5 h-3.5" /> Cable USB conectado (Cargando)
                  </span>
                ) : (
                  <span className="flex items-center gap-1 text-cyan-300">
                    <Wifi className="w-3.5 h-3.5 text-cyan-400" /> Conexión Inalámbrica Bluetooth
                  </span>
                )}
                <span className="text-slate-500">·</span>
                <span className="text-[11px] text-slate-400 font-mono">
                  {isApiNative
                    ? 'Lectura API Hardware'
                    : simulatedBattery !== null
                    ? 'Simulación Activa'
                    : 'Telemetría AnyController'}
                </span>
              </p>
            </div>
          </div>

          <div className="flex flex-wrap items-center gap-4">
            <div className="flex flex-col items-end gap-1">
              <div className="flex items-center gap-2">
                <span className="text-xs font-mono font-bold text-white tabular-nums">
                  {activeLevel}%
                </span>
                <div className="relative flex items-center p-1 rounded-md bg-[#070B14] border border-slate-700 gap-1 w-24 h-6">
                  {[1, 2, 3, 4].map((seg) => {
                    const isFilled = isCharging ? true : seg <= batterySegments;
                    return (
                      <div
                        key={seg}
                        className={`flex-1 h-full rounded-sm transition-colors ${
                          isFilled
                            ? isCharging
                              ? 'bg-emerald-400'
                              : activeLevel <= 15
                              ? 'bg-red-500'
                              : activeLevel <= 30
                              ? 'bg-amber-400'
                              : 'bg-cyan-400'
                            : 'bg-slate-800'
                        }`}
                      />
                    );
                  })}
                </div>
              </div>
              <span className="text-[10px] text-slate-400 font-mono tabular-nums">
                {isCharging
                  ? 'Carga 5V activa'
                  : activeLevel <= 15
                  ? 'Autonomía < 25 min'
                  : activeLevel <= 30
                  ? 'Autonomía ~2-3 h'
                  : 'Autonomía ~6-8 h'}
              </span>
            </div>

            <button
              type="button"
              onClick={() => setShowBatteryDetails(!showBatteryDetails)}
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-[#070B14] hover:bg-slate-900 text-slate-200 font-medium text-xs border border-slate-700 transition-colors cursor-pointer"
            >
              <Sliders className="w-3.5 h-3.5 text-cyan-400" />
              <span>{showBatteryDetails ? 'Ocultar Simulador' : 'Probar Alertas'}</span>
            </button>
          </div>
        </div>

        <div className="mt-3 pt-3 border-t border-slate-800/80 flex flex-col sm:flex-row items-start sm:items-center justify-between gap-2 text-xs">
          <div>
            {activeLevel <= 15 && !isCharging ? (
              <span className="flex items-center gap-1.5 text-red-400 font-semibold">
                <AlertTriangle className="w-4 h-4 shrink-0" />
                Batería muy baja: conecta el cable USB a tu consola PS4 para evitar desconexiones.
              </span>
            ) : activeLevel <= 30 && !isCharging ? (
              <span className="flex items-center gap-1.5 text-amber-300">
                <AlertTriangle className="w-4 h-4 shrink-0" />
                Carga moderada. Conviene recargar el mando tras la partida.
              </span>
            ) : isCharging ? (
              <span className="flex items-center gap-1.5 text-emerald-400 font-medium">
                <CheckCircle2 className="w-4 h-4 shrink-0" />
                Recibiendo alimentación USB correctamente.
              </span>
            ) : (
              <span className="flex items-center gap-1.5 text-slate-300">
                <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0" />
                Batería en estado óptimo (latencia Bluetooth 4 ms).
              </span>
            )}
          </div>
          <span className="text-[11px] text-slate-400 font-mono tabular-nums">
            Aviso en PS4 bajo 15%
          </span>
        </div>

        {showBatteryDetails && (
          <div className="mt-4 pt-4 border-t border-slate-800 space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2">
              <div>
                <h4 className="text-xs font-bold text-white flex items-center gap-1.5">
                  <Sparkles className="w-3.5 h-3.5 text-cyan-400" />
                  Simulador de Batería y Notificaciones PS4
                </h4>
                <p className="text-[11px] text-slate-400">
                  Comprueba los avisos de energía antes de jugar en la consola:
                </p>
              </div>
              {simulatedBattery !== null && (
                <button
                  type="button"
                  onClick={() => {
                    setSimulatedBattery(null);
                    setSimulatedCharging(null);
                  }}
                  className="px-2.5 py-1 rounded-md bg-slate-900 hover:bg-slate-800 text-cyan-300 text-[11px] font-semibold border border-slate-700 cursor-pointer"
                >
                  Restablecer a Valor Real
                </button>
              )}
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div className="p-3 rounded-lg bg-[#070B14] border border-slate-800 space-y-2">
                <div className="flex justify-between text-xs">
                  <span className="text-slate-300 font-semibold">Nivel Manual:</span>
                  <span className="font-mono text-cyan-400 font-bold tabular-nums">
                    {activeLevel}%
                  </span>
                </div>
                <input
                  type="range"
                  min="5"
                  max="100"
                  step="5"
                  value={activeLevel}
                  onChange={(e) => setSimulatedBattery(Number(e.target.value))}
                  className="w-full accent-cyan-400 cursor-pointer"
                />
              </div>

              <div className="p-3 rounded-lg bg-[#070B14] border border-slate-800 space-y-2">
                <span className="text-xs text-slate-300 font-semibold block">
                  Niveles Rápidos:
                </span>
                <div className="flex flex-wrap gap-2">
                  <button
                    type="button"
                    onClick={() => {
                      setSimulatedBattery(10);
                      setSimulatedCharging(false);
                    }}
                    className="px-2.5 py-1 rounded-md bg-red-500/20 hover:bg-red-500/30 text-red-300 border border-red-500/40 text-xs font-bold cursor-pointer"
                  >
                    10% Crítica
                  </button>
                  <button
                    type="button"
                    onClick={() => {
                      setSimulatedBattery(25);
                      setSimulatedCharging(false);
                    }}
                    className="px-2.5 py-1 rounded-md bg-amber-500/20 hover:bg-amber-500/30 text-amber-300 border border-amber-500/40 text-xs font-bold cursor-pointer"
                  >
                    25% Baja
                  </button>
                  <button
                    type="button"
                    onClick={() => {
                      setSimulatedBattery(80);
                      setSimulatedCharging(false);
                    }}
                    className="px-2.5 py-1 rounded-md bg-cyan-500/20 hover:bg-cyan-500/30 text-cyan-300 border border-cyan-500/40 text-xs font-bold cursor-pointer"
                  >
                    80% Óptimo
                  </button>
                  <button
                    type="button"
                    onClick={() => {
                      setSimulatedCharging(!isCharging);
                      if (simulatedBattery === null) setSimulatedBattery(activeLevel);
                    }}
                    className={`flex items-center gap-1 px-2.5 py-1 rounded-md text-xs font-bold border cursor-pointer ${
                      isCharging
                        ? 'bg-emerald-500 text-slate-950 border-emerald-400'
                        : 'bg-slate-900 text-white border-slate-700'
                    }`}
                  >
                    <Usb className="w-3.5 h-3.5" />
                    <span>{isCharging ? 'Cable USB' : 'Pasar a USB'}</span>
                  </button>
                </div>
              </div>
            </div>
          </div>
        )}
      </div>

      {/* Main Grid: Analog Telemetry & Drift Calibration */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-5">
        {/* Left Column: Sticks & Triggers */}
        <div className="lg:col-span-7 space-y-5">
          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800">
            <h3 className="text-sm font-bold text-white flex items-center justify-between gap-2">
              <span className="flex items-center gap-2">
                <Crosshair className="w-4 h-4 text-cyan-400" />
                Telemetría de Palancas Analógicas
              </span>
              <span className="text-[11px] text-cyan-300 font-mono tabular-nums">
                Zona Muerta: {DEADZONE_OPTIONS[config.deadzone_index].split(' ')[0]}
              </span>
            </h3>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-5 mt-4">
              {/* Left Stick */}
              <div className="flex flex-col items-center p-4 rounded-xl bg-[#070B14] border border-slate-800">
                <span className="text-xs font-semibold text-slate-300 mb-2">
                  Palanca Izquierda (L3)
                </span>
                <div className="relative w-36 h-36 rounded-full border-2 border-cyan-500/40 bg-[#050A14] flex items-center justify-center overflow-hidden">
                  <div
                    className="absolute rounded-full border border-cyan-400/30 bg-cyan-500/10 pointer-events-none"
                    style={{
                      width: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                      height: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                    }}
                  />
                  <div className="absolute w-full h-[1px] bg-slate-800" />
                  <div className="absolute h-full w-[1px] bg-slate-800" />
                  <div
                    className={`absolute w-7 h-7 rounded-full transition-transform duration-75 flex items-center justify-center font-bold text-[10px] ${
                      b['l3']
                        ? 'bg-amber-400 text-slate-950 scale-90 ring-4 ring-amber-400/35'
                        : 'bg-cyan-400 text-slate-950'
                    }`}
                    style={{
                      transform: `translate(${lx * 50}px, ${ly * 50}px)`,
                    }}
                  >
                    L
                  </div>
                </div>
                <div className="w-full mt-3 flex justify-between text-[11px] font-mono text-slate-400 tabular-nums">
                  <span>
                    X: <strong className="text-white">{rawLx}</strong> ({Math.round(lx * 100)}%)
                  </span>
                  <span>
                    Y: <strong className="text-white">{rawLy}</strong> ({Math.round(ly * 100)}%)
                  </span>
                </div>
              </div>

              {/* Right Stick */}
              <div className="flex flex-col items-center p-4 rounded-xl bg-[#070B14] border border-slate-800">
                <span className="text-xs font-semibold text-slate-300 mb-2">
                  Palanca Derecha (R3)
                </span>
                <div className="relative w-36 h-36 rounded-full border-2 border-cyan-500/40 bg-[#050A14] flex items-center justify-center overflow-hidden">
                  <div
                    className="absolute rounded-full border border-cyan-400/30 bg-cyan-500/10 pointer-events-none"
                    style={{
                      width: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                      height: `${(DEADZONE_VALUES[config.deadzone_index] / 31) * 60 + 10}%`,
                    }}
                  />
                  <div className="absolute w-full h-[1px] bg-slate-800" />
                  <div className="absolute h-full w-[1px] bg-slate-800" />
                  <div
                    className={`absolute w-7 h-7 rounded-full transition-transform duration-75 flex items-center justify-center font-bold text-[10px] ${
                      b['r3']
                        ? 'bg-amber-400 text-slate-950 scale-90 ring-4 ring-amber-400/35'
                        : 'bg-cyan-400 text-slate-950'
                    }`}
                    style={{
                      transform: `translate(${rx * 50}px, ${ry * 50}px)`,
                    }}
                  >
                    R
                  </div>
                </div>
                <div className="w-full mt-3 flex justify-between text-[11px] font-mono text-slate-400 tabular-nums">
                  <span>
                    X: <strong className="text-white">{rawRx}</strong> ({Math.round(rx * 100)}%)
                  </span>
                  <span>
                    Y: <strong className="text-white">{rawRy}</strong> ({Math.round(ry * 100)}%)
                  </span>
                </div>
              </div>
            </div>
          </div>

          {/* Analog Triggers L2 / R2 */}
          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800">
            <h3 className="text-sm font-bold text-white flex items-center justify-between gap-2">
              <span>Presión Analógica de Gatillos (L2 / R2)</span>
              <span className="text-[11px] text-cyan-300 font-mono tabular-nums">
                Umbral Disparo Digital: {triggerThresholdVal}/255
              </span>
            </h3>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 mt-4">
              <div className="p-3 rounded-lg bg-[#070B14] border border-slate-800 space-y-1.5">
                <div className="flex justify-between text-xs">
                  <span className="font-semibold text-slate-300">Gatillo L2</span>
                  <span className="font-mono text-cyan-400 font-bold tabular-nums">
                    {l2Val} / 255
                  </span>
                </div>
                <div className="w-full h-2.5 bg-slate-900 rounded-sm overflow-hidden">
                  <div
                    className="h-full bg-cyan-400 transition-all duration-75"
                    style={{ width: `${(l2Val / 255) * 100}%` }}
                  />
                </div>
              </div>

              <div className="p-3 rounded-lg bg-[#070B14] border border-slate-800 space-y-1.5">
                <div className="flex justify-between text-xs">
                  <span className="font-semibold text-slate-300">Gatillo R2</span>
                  <span className="font-mono text-cyan-400 font-bold tabular-nums">
                    {r2Val} / 255
                  </span>
                </div>
                <div className="w-full h-2.5 bg-slate-900 rounded-sm overflow-hidden">
                  <div
                    className="h-full bg-cyan-400 transition-all duration-75"
                    style={{ width: `${(r2Val / 255) * 100}%` }}
                  />
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* Right Column: Drift Calibration & Isolated Rumble Test */}
        <div className="lg:col-span-5 space-y-5">
          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-4">
            <div>
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <Sparkles className="w-4 h-4 text-cyan-400" />
                Asistente de Calibración de Deriva (Drift)
              </h3>
              <p className="text-xs text-slate-400 mt-1">
                Calcula la zona muerta ideal solo para el mando seleccionado (Jugador {activeSlot + 1}) sin alterar los demás mandos.
              </p>
            </div>

            <div className="p-4 rounded-xl bg-[#070B14] border border-slate-800 space-y-3">
              {!tester.calib.active && !tester.calib.completed && (
                <div className="space-y-3">
                  <div className="flex items-start gap-2 text-xs text-slate-300">
                    <Info className="w-4 h-4 text-cyan-400 shrink-0 mt-0.5" />
                    <span>
                      Suelta ambas palancas analógicas en reposo y pulsa el botón para analizar el ruido durante 3 segundos.
                    </span>
                  </div>
                  <button
                    type="button"
                    onClick={startCalibration}
                    className="w-full flex items-center justify-center gap-2 py-2.5 rounded-lg bg-cyan-400 hover:bg-cyan-300 text-slate-950 font-bold text-xs transition-colors cursor-pointer"
                  >
                    <Play className="w-4 h-4" />
                    <span>Iniciar Calibración de Ruido</span>
                  </button>
                </div>
              )}

              {tester.calib.active && (
                <div className="space-y-3 text-center py-2">
                  <div className="text-xs font-semibold text-cyan-300">
                    Muestreando centro neutro... No toques las palancas
                  </div>
                  <div className="w-full h-2.5 bg-slate-900 rounded-sm overflow-hidden">
                    <div
                      className="h-full bg-cyan-400 transition-all duration-75"
                      style={{
                        width: `${(tester.calib.currentFrame / tester.calib.totalFrames) * 100}%`,
                      }}
                    />
                  </div>
                  <div className="text-[11px] font-mono text-slate-400 tabular-nums">
                    Ruido Máximo Detectado:{' '}
                    <strong className="text-white">{tester.calib.maxNoise}</strong>
                  </div>
                </div>
              )}

              {tester.calib.completed && (
                <div className="space-y-3">
                  <div className="flex items-center gap-2 text-xs text-emerald-400 font-semibold">
                    <CheckCircle2 className="w-4 h-4" />
                    <span>Calibración Completada</span>
                  </div>
                  <div className="p-3 rounded-lg bg-[#0B1120] border border-slate-800 text-xs text-slate-300 space-y-1">
                    <p>
                      Desviación neutra detectada:{' '}
                      <strong className="text-white font-mono tabular-nums">
                        {tester.calib.maxNoise} / 127
                      </strong>
                    </p>
                    <p>
                      Ajuste recomendado:{' '}
                      <strong className="text-cyan-300">
                        {DEADZONE_OPTIONS[tester.calib.suggestedIndex]}
                      </strong>
                    </p>
                  </div>
                  <div className="flex gap-2">
                    <button
                      type="button"
                      onClick={applySuggestedDeadzone}
                      className="flex-1 py-2 px-3 rounded-lg bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-bold text-xs transition-colors cursor-pointer"
                    >
                      Aplicar a Jugador {activeSlot + 1}
                    </button>
                    <button
                      type="button"
                      onClick={startCalibration}
                      className="p-2 rounded-lg bg-slate-900 hover:bg-slate-800 border border-slate-700 text-white transition-colors cursor-pointer"
                      title="Repetir calibración"
                    >
                      <RotateCcw className="w-4 h-4" />
                    </button>
                  </div>
                </div>
              )}
            </div>
          </div>

          {/* Isolated Rumble Motor Test */}
          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800">
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <Vibrate className="w-4 h-4 text-cyan-400" />
              Prueba de Vibración (Solo Jugador {activeSlot + 1})
            </h3>
            <p className="text-xs text-slate-400 mt-1">
              Envía pulsos independientes al mando activo sin hacer vibrar los otros jugadores.
            </p>

            <div className="grid grid-cols-3 gap-2.5 mt-4">
              <button
                type="button"
                onClick={() => handleTestRumble(1)}
                className={`py-2 px-2.5 rounded-lg text-xs font-semibold border transition-colors cursor-pointer ${
                  tester.rumblePhase === 1
                    ? 'bg-cyan-400 text-slate-950 border-cyan-300'
                    : 'bg-[#070B14] hover:bg-slate-900 text-slate-200 border-slate-800'
                }`}
              >
                Pesado (L)
              </button>
              <button
                type="button"
                onClick={() => handleTestRumble(2)}
                className={`py-2 px-2.5 rounded-lg text-xs font-semibold border transition-colors cursor-pointer ${
                  tester.rumblePhase === 2
                    ? 'bg-cyan-400 text-slate-950 border-cyan-300'
                    : 'bg-[#070B14] hover:bg-slate-900 text-slate-200 border-slate-800'
                }`}
              >
                Ligero (R)
              </button>
              <button
                type="button"
                onClick={() => handleTestRumble(3)}
                className={`py-2 px-2.5 rounded-lg text-xs font-semibold border transition-colors cursor-pointer ${
                  tester.rumblePhase === 3
                    ? 'bg-emerald-500 text-slate-950 border-emerald-400'
                    : 'bg-[#070B14] hover:bg-slate-900 text-slate-200 border-slate-800'
                }`}
              >
                Ambos (100%)
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
