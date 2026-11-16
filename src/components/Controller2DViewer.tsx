/**
 * Controller Control Center PS4 - Visor Interactivo Original 2D en Tiempo Real (v1.5.2)
 * Siluetas originales de alta fidelidad de Sony PS5 DualSense y PS4 DualShock 4
 * con barra superior L2/L1/R1/R2, barra LED animada y respuesta lumínica al pulsar o hacer clic.
 */
import { useEffect, useState } from 'react';
import { AppConfig, Controller2DModel, LiveGamepadInput } from '../types';
import { DS_COLOR_HEX, DS4_COLOR_HEX } from '../constants/options';
import { Gamepad2, Sparkles, Gamepad, Vibrate, RotateCcw } from 'lucide-react';

interface Controller2DViewerProps {
  mode: Controller2DModel;
  onModeChange: (next: Controller2DModel) => void;
  liveInput: LiveGamepadInput;
  config: AppConfig;
  rumblePhase: number;
  batteryLevel: number;
  hardwareDetected?: boolean;
}

function getEffectiveLightbarColor(
  mode: Controller2DModel,
  config: AppConfig,
  batteryLevel: number,
  timeMs: number
): { hex: string; opacity: number; off: boolean; label: string } {
  if (mode === 'dualsense') {
    const lbMode = config.ds_lightbar_mode;
    const bri =
      config.ds_brightness === 0 ? 1 : config.ds_brightness === 1 ? 0.65 : 0.35;
    if (lbMode === 3) {
      return { hex: '#1E293B', opacity: 0.25, off: true, label: 'Apagado' };
    }
    if (lbMode === 2) {
      const hex =
        batteryLevel > 60 ? '#10B981' : batteryLevel > 20 ? '#F59E0B' : '#EF4444';
      return { hex, opacity: bri, off: false, label: `Batería ${batteryLevel}%` };
    }
    if (lbMode === 4) {
      const hue = Math.floor((timeMs * 0.08) % 360);
      return { hex: `hsl(${hue}, 92%, 58%)`, opacity: bri, off: false, label: 'Arcoíris RGB' };
    }
    if (lbMode === 5) {
      const wave = 0.35 + 0.65 * (0.5 + 0.5 * Math.sin(timeMs * 0.004));
      return {
        hex: DS_COLOR_HEX[config.ds_lightbar_color] || '#0070D1',
        opacity: bri * wave,
        off: false,
        label: 'Pulso Respiración',
      };
    }
    return {
      hex: DS_COLOR_HEX[config.ds_lightbar_color] || '#0070D1',
      opacity: bri,
      off: false,
      label: lbMode === 0 ? 'Sincronizado Juego' : 'Estático',
    };
  } else {
    const lbMode = config.ds4_lightbar_mode;
    const bri =
      config.ds4_brightness === 0 ? 1 : config.ds4_brightness === 1 ? 0.65 : 0.35;
    if (lbMode === 3) {
      return { hex: '#1E293B', opacity: 0.25, off: true, label: 'Apagado (Eco)' };
    }
    if (lbMode === 2) {
      const hex =
        batteryLevel > 60 ? '#10B981' : batteryLevel > 20 ? '#F59E0B' : '#EF4444';
      return { hex, opacity: bri, off: false, label: `Batería ${batteryLevel}%` };
    }
    if (lbMode === 4) {
      const hue = Math.floor((timeMs * 0.08) % 360);
      return { hex: `hsl(${hue}, 92%, 58%)`, opacity: bri, off: false, label: 'Arcoíris RGB' };
    }
    if (lbMode === 5) {
      const wave = 0.35 + 0.65 * (0.5 + 0.5 * Math.sin(timeMs * 0.004));
      return {
        hex: DS4_COLOR_HEX[config.ds4_lightbar_color] || '#0070DC',
        opacity: bri * wave,
        off: false,
        label: 'Pulso Respiración',
      };
    }
    return {
      hex: DS4_COLOR_HEX[config.ds4_lightbar_color] || '#0070DC',
      opacity: bri,
      off: false,
      label: lbMode === 0 ? 'Control del Juego' : 'Personalizado',
    };
  }
}

export default function Controller2DViewer({
  mode,
  onModeChange,
  liveInput,
  config,
  rumblePhase,
  batteryLevel,
  hardwareDetected = false,
}: Controller2DViewerProps) {
  const [animTime, setAnimTime] = useState<number>(Date.now());
  const [mousePressed, setMousePressed] = useState<Record<string, boolean>>({});

  // Smooth 30fps loop only while rainbow/pulse animation or rumble is running
  const needsLoop =
    rumblePhase > 0 ||
    (mode === 'dualsense' &&
      (config.ds_lightbar_mode === 4 || config.ds_lightbar_mode === 5)) ||
    (mode === 'ds4' &&
      (config.ds4_lightbar_mode === 4 || config.ds4_lightbar_mode === 5));

  useEffect(() => {
    if (!needsLoop) return;
    let rafId: number;
    const tick = () => {
      setAnimTime(Date.now());
      rafId = requestAnimationFrame(tick);
    };
    rafId = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(rafId);
  }, [needsLoop]);

  const isPressed = (key: string) =>
    Boolean(liveInput.buttons[key] || mousePressed[key]);

  const l2Pct = Math.max(
    Math.min(1, liveInput.l2 || 0),
    mousePressed['l2'] ? 1 : 0
  );
  const r2Pct = Math.max(
    Math.min(1, liveInput.r2 || 0),
    mousePressed['r2'] ? 1 : 0
  );

  const pressHandler = (key: string) => ({
    onPointerDown: () => setMousePressed((p) => ({ ...p, [key]: true })),
    onPointerUp: () => setMousePressed((p) => ({ ...p, [key]: false })),
    onPointerLeave: () => setMousePressed((p) => ({ ...p, [key]: false })),
  });

  const lightbar = getEffectiveLightbarColor(mode, config, batteryLevel, animTime);

  const activeList: string[] = [];
  if (isPressed('l2')) activeList.push(`L2 ${Math.round(l2Pct * 100)}%`);
  if (isPressed('l1')) activeList.push('L1');
  if (isPressed('r1')) activeList.push('R1');
  if (isPressed('r2')) activeList.push(`R2 ${Math.round(r2Pct * 100)}%`);
  if (isPressed('dpad_up')) activeList.push('Cruceta Arriba');
  if (isPressed('dpad_down')) activeList.push('Cruceta Abajo');
  if (isPressed('dpad_left')) activeList.push('Cruceta Izq.');
  if (isPressed('dpad_right')) activeList.push('Cruceta Der.');
  if (isPressed('cross')) activeList.push('Cruz (✕)');
  if (isPressed('circle')) activeList.push('Círculo (○)');
  if (isPressed('square')) activeList.push('Cuadrado (□)');
  if (isPressed('triangle')) activeList.push('Triángulo (△)');
  if (isPressed('l3')) activeList.push('L3');
  if (isPressed('r3')) activeList.push('R3');
  if (isPressed('share')) activeList.push(mode === 'dualsense' ? 'Create' : 'Share');
  if (isPressed('options')) activeList.push('Options');
  if (isPressed('touchpad')) activeList.push('Touchpad');
  if (isPressed('ps')) activeList.push('Botón PS');
  if (mode === 'dualsense' && isPressed('mute')) activeList.push('Mute Mic');

  const rumbleOffset =
    rumblePhase > 0 ? Math.sin(animTime * 0.08) * (rumblePhase === 3 ? 2.5 : 1.5) : 0;

  return (
    <section className="rounded-xl bg-[#0B1120] border border-slate-800 p-5 space-y-5">
      {/* Header Row */}
      <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-4 pb-4 border-b border-slate-800">
        <div>
          <div className="flex flex-wrap items-center gap-2.5">
            <h3 className="text-sm md:text-base font-bold text-white flex items-center gap-2">
              <Gamepad2 className="w-4 h-4 text-cyan-400" />
              <span>Probador Visual Original en Tiempo Real</span>
            </h3>
            <span className="text-xs font-mono text-slate-400 tabular-nums">
              J{liveInput.slot + 1} · {hardwareDetected ? 'Hardware Conectado' : 'Teclado / Clic Directo'}
            </span>
          </div>
          <p className="text-xs text-slate-400 mt-1">
            Pulsa cualquier botón en tu mando, usa el teclado o haz clic directamente sobre el dibujo para iluminar cada control.
          </p>
        </div>

        {/* Clean Model Switcher */}
        <div className="flex flex-wrap items-center gap-2">
          <div className="flex items-center bg-[#070B14] p-1 rounded-lg border border-slate-800 text-xs">
            <button
              type="button"
              onClick={() => onModeChange('dualsense')}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md font-semibold transition-colors cursor-pointer whitespace-nowrap ${
                mode === 'dualsense'
                  ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-400/50'
                  : 'text-slate-400 hover:text-slate-200 border border-transparent'
              }`}
            >
              <Sparkles className="w-3.5 h-3.5" />
              <span>PS5 DualSense</span>
            </button>
            <button
              type="button"
              onClick={() => onModeChange('ds4')}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-md font-semibold transition-colors cursor-pointer whitespace-nowrap ${
                mode === 'ds4'
                  ? 'bg-indigo-500/20 text-indigo-300 border border-indigo-400/50'
                  : 'text-slate-400 hover:text-slate-200 border border-transparent'
              }`}
            >
              <Gamepad className="w-3.5 h-3.5" />
              <span>PS4 DualShock 4</span>
            </button>
          </div>

          {Object.values(mousePressed).some(Boolean) && (
            <button
              type="button"
              onClick={() => setMousePressed({})}
              className="p-2 rounded-lg bg-slate-900 hover:bg-slate-800 text-slate-300 border border-slate-700 cursor-pointer"
              title="Limpiar botones clicados"
            >
              <RotateCcw className="w-3.5 h-3.5" />
            </button>
          )}
        </div>
      </div>

      {/* Top Shoulder Controls Bar: L2, L1, USB-C + Lightbar Status, R1, R2 */}
      <div className="grid grid-cols-12 items-center gap-3 max-w-3xl mx-auto">
        {/* L2 Analog Trigger */}
        <button
          type="button"
          {...pressHandler('l2')}
          className={`col-span-3 sm:col-span-3 rounded-lg border p-2.5 text-left transition-all cursor-pointer select-none ${
            isPressed('l2')
              ? 'bg-cyan-500/20 border-cyan-400 shadow-[0_0_16px_rgba(34,211,238,0.35)]'
              : 'bg-[#070B14] border-slate-800 hover:border-slate-700'
          }`}
        >
          <div className="flex items-center justify-between text-xs font-mono mb-1.5">
            <span className={isPressed('l2') ? 'text-cyan-300 font-bold' : 'text-slate-300 font-bold'}>
              L2
            </span>
            <span className="text-slate-400 tabular-nums">{Math.round(l2Pct * 100)}%</span>
          </div>
          <div className="w-full h-2 rounded-sm bg-slate-900 overflow-hidden">
            <div
              className="h-full bg-cyan-400 transition-all duration-75"
              style={{ width: `${l2Pct * 100}%` }}
            />
          </div>
        </button>

        {/* L1 Bumper */}
        <button
          type="button"
          {...pressHandler('l1')}
          className={`col-span-3 sm:col-span-2 rounded-lg border py-2.5 px-3 text-center font-mono text-xs font-bold transition-all cursor-pointer select-none ${
            isPressed('l1')
              ? 'bg-cyan-400 text-slate-950 border-cyan-300 shadow-[0_0_16px_rgba(34,211,238,0.45)] translate-y-0.5'
              : 'bg-[#070B14] text-slate-300 border-slate-800 hover:border-slate-700'
          }`}
        >
          L1
        </button>

        {/* Center Top Edge: Lightbar Preview */}
        <div className="col-span-12 sm:col-span-2 order-first sm:order-none flex flex-col items-center justify-center py-1">
          <div
            className="w-full max-w-[110px] h-2.5 rounded-sm border border-white/20 transition-all duration-150"
            style={{
              backgroundColor: lightbar.hex,
              opacity: lightbar.opacity,
              boxShadow: lightbar.off ? 'none' : `0 0 14px ${lightbar.hex}`,
            }}
          />
          <span className="text-[10px] font-mono text-slate-400 mt-1 truncate max-w-full">
            LED: {lightbar.label}
          </span>
        </div>

        {/* R1 Bumper */}
        <button
          type="button"
          {...pressHandler('r1')}
          className={`col-span-3 sm:col-span-2 rounded-lg border py-2.5 px-3 text-center font-mono text-xs font-bold transition-all cursor-pointer select-none ${
            isPressed('r1')
              ? 'bg-cyan-400 text-slate-950 border-cyan-300 shadow-[0_0_16px_rgba(34,211,238,0.45)] translate-y-0.5'
              : 'bg-[#070B14] text-slate-300 border-slate-800 hover:border-slate-700'
          }`}
        >
          R1
        </button>

        {/* R2 Analog Trigger */}
        <button
          type="button"
          {...pressHandler('r2')}
          className={`col-span-3 sm:col-span-3 rounded-lg border p-2.5 text-left transition-all cursor-pointer select-none ${
            isPressed('r2')
              ? 'bg-cyan-500/20 border-cyan-400 shadow-[0_0_16px_rgba(34,211,238,0.35)]'
              : 'bg-[#070B14] border-slate-800 hover:border-slate-700'
          }`}
        >
          <div className="flex items-center justify-between text-xs font-mono mb-1.5">
            <span className={isPressed('r2') ? 'text-cyan-300 font-bold' : 'text-slate-300 font-bold'}>
              R2
            </span>
            <span className="text-slate-400 tabular-nums">{Math.round(r2Pct * 100)}%</span>
          </div>
          <div className="w-full h-2 rounded-sm bg-slate-900 overflow-hidden">
            <div
              className="h-full bg-cyan-400 transition-all duration-75"
              style={{ width: `${r2Pct * 100}%` }}
            />
          </div>
        </button>
      </div>

      {/* Main 2D Original Controller Canvas */}
      <div className="relative flex flex-col items-center justify-center rounded-xl bg-[#070B14] border border-slate-800/90 py-6 px-3 overflow-hidden">
        <div
          className="w-full max-w-[680px] transition-transform duration-75"
          style={{ transform: `translateX(${rumbleOffset}px)` }}
        >
          {mode === 'dualsense' ? (
            <svg
              viewBox="0 0 680 420"
              className="w-full h-auto select-none"
              role="img"
              aria-label="Mando Original Sony PlayStation 5 DualSense en 2D"
            >
              <defs>
                <linearGradient id="dsWhiteShell" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#FFFFFF" />
                  <stop offset="45%" stopColor="#EEF2F7" />
                  <stop offset="100%" stopColor="#CBD5E1" />
                </linearGradient>
                <linearGradient id="dsShellRim" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#FFFFFF" stopOpacity="0.85" />
                  <stop offset="50%" stopColor="#94A3B8" stopOpacity="0.45" />
                  <stop offset="100%" stopColor="#334155" stopOpacity="0.8" />
                </linearGradient>
                <linearGradient id="dsBlackTrim" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#26334D" />
                  <stop offset="50%" stopColor="#141C2E" />
                  <stop offset="100%" stopColor="#090D18" />
                </linearGradient>
                <radialGradient id="dsWellGrad" cx="50%" cy="35%" r="65%">
                  <stop offset="0%" stopColor="#E2E8F0" />
                  <stop offset="100%" stopColor="#CBD5E1" />
                </radialGradient>
                <radialGradient id="dsStickCap" cx="45%" cy="32%" r="68%">
                  <stop offset="0%" stopColor="#475569" />
                  <stop offset="65%" stopColor="#1E293B" />
                  <stop offset="100%" stopColor="#0F172A" />
                </radialGradient>
                <pattern id="dsTouchDots" width="9" height="9" patternUnits="userSpaceOnUse">
                  <circle cx="4.5" cy="4.5" r="1.05" fill="#94A3B8" opacity="0.45" />
                </pattern>
                <filter id="neonGlow" x="-30%" y="-30%" width="160%" height="160%">
                  <feGaussianBlur stdDeviation="5.5" result="blur" />
                  <feMerge>
                    <feMergeNode in="blur" />
                    <feMergeNode in="SourceGraphic" />
                  </feMerge>
                </filter>
                <filter id="bodyDrop" x="-10%" y="-10%" width="120%" height="125%">
                  <feDropShadow dx="0" dy="10" stdDeviation="10" floodColor="#000000" floodOpacity="0.6" />
                </filter>
              </defs>

              {/* Subtle Shoulder Caps Behind Top Edge */}
              <path
                d="M 135 58 C 145 36, 200 34, 222 48 L 226 68 L 128 74 Z"
                fill={isPressed('l1') ? '#22D3EE' : '#0F172A'}
                stroke={isPressed('l1') ? '#67E8F9' : '#475569'}
                strokeWidth="2"
                className="cursor-pointer transition-colors"
                {...pressHandler('l1')}
              />
              <path
                d="M 545 58 C 535 36, 480 34, 458 48 L 454 68 L 552 74 Z"
                fill={isPressed('r1') ? '#22D3EE' : '#0F172A'}
                stroke={isPressed('r1') ? '#67E8F9' : '#475569'}
                strokeWidth="2"
                className="cursor-pointer transition-colors"
                {...pressHandler('r1')}
              />

              {/* DualSense Two-Tone Outer White Body */}
              <path
                d="M 130 68 C 165 56, 215 62, 240 66 L 440 66 C 465 62, 515 56, 550 68
                   C 590 84, 628 190, 636 286 C 642 356, 594 388, 554 374
                   C 526 364, 502 318, 476 284 C 452 256, 434 252, 340 252
                   C 246 252, 228 256, 204 284 C 178 318, 154 364, 126 374
                   C 86 388, 38 356, 44 286 C 52 190, 90 84, 130 68 Z"
                fill="url(#dsWhiteShell)"
                stroke="#475569"
                strokeWidth="2.6"
                filter="url(#bodyDrop)"
              />
              {/* Specular Top Rim Edge */}
              <path
                d="M 132 71 C 167 60, 215 65, 240 69 L 440 69 C 465 65, 513 60, 548 71
                   C 586 86, 623 189, 631 284 C 636 348, 593 380, 555 369"
                fill="none"
                stroke="url(#dsShellRim)"
                strokeWidth="1.8"
                pointerEvents="none"
              />

              {/* DualSense Recessed Left & Right Control Wells */}
              <circle cx="145" cy="154" r="56" fill="url(#dsWellGrad)" stroke="#CBD5E1" strokeWidth="1.5" />
              <circle cx="535" cy="154" r="56" fill="url(#dsWellGrad)" stroke="#CBD5E1" strokeWidth="1.5" />

              {/* DualSense Dark Obsidian Lower Wing / Trim Collar */}
              <path
                d="M 196 206 C 218 196, 250 198, 276 200 L 404 200 C 430 198, 462 196, 484 206
                   C 496 226, 482 264, 466 284 C 446 264, 424 260, 340 260
                   C 256 260, 234 264, 214 284 C 198 264, 184 226, 196 206 Z"
                fill="url(#dsBlackTrim)"
                stroke="#3B4B6B"
                strokeWidth="1.6"
              />

              {/* Sculpted Grip Accent Seam Lines */}
              <path
                d="M 88 228 C 110 265, 132 310, 146 355"
                fill="none"
                stroke="#94A3B8"
                strokeWidth="1.5"
                opacity="0.7"
              />
              <path
                d="M 592 228 C 570 265, 548 310, 534 355"
                fill="none"
                stroke="#94A3B8"
                strokeWidth="1.5"
                opacity="0.7"
              />
              {/* Inner Black Grip Contours */}
              <path
                d="M 170 282 C 150 315, 138 345, 124 366"
                fill="none"
                stroke="#1E293B"
                strokeWidth="4"
                strokeLinecap="round"
                opacity="0.55"
              />
              <path
                d="M 510 282 C 530 315, 542 345, 556 366"
                fill="none"
                stroke="#1E293B"
                strokeWidth="4"
                strokeLinecap="round"
                opacity="0.55"
              />

              {/* Rumble Wave Feedback on Grips */}
              {(rumblePhase === 1 || rumblePhase === 3) && (
                <g opacity="0.85" stroke="#22D3EE" strokeWidth="2.5" fill="none">
                  <path d="M 48 250 C 32 276, 30 308, 42 332" />
                  <path d="M 34 242 C 14 274, 12 312, 28 342" />
                </g>
              )}
              {(rumblePhase === 2 || rumblePhase === 3) && (
                <g opacity="0.85" stroke="#34D399" strokeWidth="2.5" fill="none">
                  <path d="M 632 250 C 648 276, 650 308, 638 332" />
                  <path d="M 646 242 C 666 274, 668 312, 652 342" />
                </g>
              )}

              {/* Touchpad Lightbar Halo Left & Right Flanking Strips */}
              {!lightbar.off && (
                <g opacity={lightbar.opacity} filter="url(#neonGlow)">
                  <polygon
                    points="228,67 241,67 254,174 243,174"
                    fill={lightbar.hex}
                  />
                  <polygon
                    points="452,67 439,67 426,174 437,174"
                    fill={lightbar.hex}
                  />
                </g>
              )}

              {/* Trapezoidal Touchpad with Pearl Micro-dot Texture */}
              <polygon
                points="238,68 442,68 428,174 252,174"
                fill={isPressed('touchpad') ? '#0E7490' : '#F1F5F9'}
                stroke={isPressed('touchpad') ? '#22D3EE' : '#94A3B8'}
                strokeWidth="2"
                className="cursor-pointer transition-colors"
                {...pressHandler('touchpad')}
              />
              <polygon
                points="242,72 438,72 425,170 255,170"
                fill="url(#dsTouchDots)"
                pointerEvents="none"
              />
              {/* Bright core inside Touchpad V-Lightbars */}
              {!lightbar.off && (
                <g opacity={Math.min(1, lightbar.opacity + 0.15)} pointerEvents="none">
                  <line x1="236" y1="69" x2="250" y2="173" stroke="#FFFFFF" strokeWidth="1.6" opacity="0.8" />
                  <line x1="444" y1="69" x2="430" y2="173" stroke="#FFFFFF" strokeWidth="1.6" opacity="0.8" />
                </g>
              )}
              <text
                x="340"
                y="124"
                textAnchor="middle"
                fontSize="10"
                fontFamily="JetBrains Mono, monospace"
                fontWeight="700"
                fill={isPressed('touchpad') ? '#ECFEFF' : '#64748B'}
                pointerEvents="none"
              >
                {isPressed('touchpad') ? 'TOUCHPAD ACTIVO' : 'TOUCHPAD'}
              </text>

              {/* 5 Player Indicator LEDs Below Touchpad */}
              {[-2, -1, 0, 1, 2].map((pos, idx) => {
                const slotNum = liveInput.slot + 1;
                const ledsMode = config.ds_player_leds;
                let lit = false;
                if (ledsMode === 1) lit = true;
                else if (ledsMode === 0) {
                  if (slotNum === 1) lit = idx === 2;
                  else if (slotNum === 2) lit = idx === 1 || idx === 3;
                  else if (slotNum === 3) lit = idx === 0 || idx === 2 || idx === 4;
                  else lit = idx !== 2;
                }
                return (
                  <rect
                    key={idx}
                    x={340 + pos * 11 - 2.5}
                    y={180}
                    width="5"
                    height="2.6"
                    rx="1"
                    fill={lit ? '#FFFFFF' : '#475569'}
                    opacity={lit ? 0.95 : 0.4}
                  />
                );
              })}

              {/* Create & Options Pill Buttons */}
              <g className="cursor-pointer" {...pressHandler('share')}>
                <rect
                  x="204"
                  y="90"
                  width="13"
                  height="26"
                  rx="6.5"
                  transform="rotate(-10 210 103)"
                  fill={isPressed('share') ? '#22D3EE' : '#E2E8F0'}
                  stroke={isPressed('share') ? '#06B6D4' : '#64748B'}
                  strokeWidth="2"
                />
                <text
                  x="210"
                  y="81"
                  textAnchor="middle"
                  fontSize="8"
                  fontWeight="700"
                  fill={isPressed('share') ? '#22D3EE' : '#475569'}
                >
                  CREATE
                </text>
              </g>

              <g className="cursor-pointer" {...pressHandler('options')}>
                <rect
                  x="463"
                  y="90"
                  width="13"
                  height="26"
                  rx="6.5"
                  transform="rotate(10 469 103)"
                  fill={isPressed('options') ? '#22D3EE' : '#E2E8F0'}
                  stroke={isPressed('options') ? '#06B6D4' : '#64748B'}
                  strokeWidth="2"
                />
                <text
                  x="469"
                  y="81"
                  textAnchor="middle"
                  fontSize="8"
                  fontWeight="700"
                  fill={isPressed('options') ? '#22D3EE' : '#475569'}
                >
                  OPTIONS
                </text>
              </g>

              {/* D-Pad (Left Side) with Sculpted Directional Chevrons */}
              <g>
                <g className="cursor-pointer" {...pressHandler('dpad_up')}>
                  <polygon
                    points="135,112 155,112 155,140 145,148 135,140"
                    fill={isPressed('dpad_up') ? '#22D3EE' : '#F8FAFC'}
                    stroke={isPressed('dpad_up') ? '#06B6D4' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="141,126 145,120 149,126" fill="none" stroke={isPressed('dpad_up') ? '#FFFFFF' : '#94A3B8'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
                <g className="cursor-pointer" {...pressHandler('dpad_down')}>
                  <polygon
                    points="135,168 145,160 155,168 155,196 135,196"
                    fill={isPressed('dpad_down') ? '#22D3EE' : '#F8FAFC'}
                    stroke={isPressed('dpad_down') ? '#06B6D4' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="141,182 145,188 149,182" fill="none" stroke={isPressed('dpad_down') ? '#FFFFFF' : '#94A3B8'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
                <g className="cursor-pointer" {...pressHandler('dpad_left')}>
                  <polygon
                    points="102,144 130,144 138,154 130,164 102,164"
                    fill={isPressed('dpad_left') ? '#22D3EE' : '#F8FAFC'}
                    stroke={isPressed('dpad_left') ? '#06B6D4' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="118,150 112,154 118,158" fill="none" stroke={isPressed('dpad_left') ? '#FFFFFF' : '#94A3B8'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
                <g className="cursor-pointer" {...pressHandler('dpad_right')}>
                  <polygon
                    points="160,144 188,144 188,164 160,164 152,154"
                    fill={isPressed('dpad_right') ? '#22D3EE' : '#F8FAFC'}
                    stroke={isPressed('dpad_right') ? '#06B6D4' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="172,150 178,154 172,158" fill="none" stroke={isPressed('dpad_right') ? '#FFFFFF' : '#94A3B8'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
              </g>

              {/* PlayStation Action Buttons (Right Side) */}
              <g>
                {/* Triangle */}
                <g className="cursor-pointer" {...pressHandler('triangle')}>
                  <circle
                    cx="535"
                    cy="118"
                    r="18"
                    fill={isPressed('triangle') ? '#10B981' : '#F8FAFC'}
                    stroke={isPressed('triangle') ? '#6EE7B7' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="535" cy="118" r="14.5" fill="none" stroke="#FFFFFF" strokeWidth="1" opacity="0.7" />
                  <polygon
                    points="535,107 545,125 525,125"
                    fill="none"
                    stroke={isPressed('triangle') ? '#FFFFFF' : '#10B981'}
                    strokeWidth="2.6"
                    strokeLinejoin="round"
                  />
                </g>

                {/* Circle */}
                <g className="cursor-pointer" {...pressHandler('circle')}>
                  <circle
                    cx="573"
                    cy="154"
                    r="18"
                    fill={isPressed('circle') ? '#EF4444' : '#F8FAFC'}
                    stroke={isPressed('circle') ? '#FCA5A5' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="573" cy="154" r="14.5" fill="none" stroke="#FFFFFF" strokeWidth="1" opacity="0.7" />
                  <circle
                    cx="573"
                    cy="154"
                    r="9"
                    fill="none"
                    stroke={isPressed('circle') ? '#FFFFFF' : '#EF4444'}
                    strokeWidth="2.6"
                  />
                </g>

                {/* Cross */}
                <g className="cursor-pointer" {...pressHandler('cross')}>
                  <circle
                    cx="535"
                    cy="190"
                    r="18"
                    fill={isPressed('cross') ? '#2563EB' : '#F8FAFC'}
                    stroke={isPressed('cross') ? '#93C5FD' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="535" cy="190" r="14.5" fill="none" stroke="#FFFFFF" strokeWidth="1" opacity="0.7" />
                  <path
                    d="M 527 182 L 543 198 M 543 182 L 527 198"
                    stroke={isPressed('cross') ? '#FFFFFF' : '#3B82F6'}
                    strokeWidth="2.8"
                    strokeLinecap="round"
                  />
                </g>

                {/* Square */}
                <g className="cursor-pointer" {...pressHandler('square')}>
                  <circle
                    cx="497"
                    cy="154"
                    r="18"
                    fill={isPressed('square') ? '#EC4899' : '#F8FAFC'}
                    stroke={isPressed('square') ? '#F9A8D4' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="497" cy="154" r="14.5" fill="none" stroke="#FFFFFF" strokeWidth="1" opacity="0.7" />
                  <rect
                    x="489"
                    y="146"
                    width="16"
                    height="16"
                    fill="none"
                    stroke={isPressed('square') ? '#FFFFFF' : '#EC4899'}
                    strokeWidth="2.6"
                  />
                </g>
              </g>

              {/* Left Thumbstick (L3) with Compass Ticks & Concentric Grip Rings */}
              <g className="cursor-pointer" {...pressHandler('l3')}>
                <circle
                  cx="256"
                  cy="226"
                  r="34"
                  fill="#070B14"
                  stroke={isPressed('l3') ? '#F59E0B' : '#3B4B6B'}
                  strokeWidth={isPressed('l3') ? '3' : '2'}
                />
                <path d="M 256 192 v 5 M 256 255 v 5 M 222 226 h 5 M 285 226 h 5" stroke="#475569" strokeWidth="1.5" />
                <circle
                  cx={256 + liveInput.lx * 13}
                  cy={226 + liveInput.ly * 13}
                  r="23"
                  fill={isPressed('l3') ? '#F59E0B' : 'url(#dsStickCap)'}
                  stroke={isPressed('l3') ? '#FDE68A' : '#64748B'}
                  strokeWidth="2.2"
                />
                <circle
                  cx={256 + liveInput.lx * 13}
                  cy={226 + liveInput.ly * 13}
                  r="17"
                  fill="none"
                  stroke={isPressed('l3') ? '#FEF3C7' : '#475569'}
                  strokeWidth="1.1"
                />
                <circle
                  cx={256 + liveInput.lx * 13}
                  cy={226 + liveInput.ly * 13}
                  r="11.5"
                  fill={isPressed('l3') ? '#D97706' : '#0F172A'}
                />
              </g>

              {/* Right Thumbstick (R3) with Compass Ticks & Concentric Grip Rings */}
              <g className="cursor-pointer" {...pressHandler('r3')}>
                <circle
                  cx="424"
                  cy="226"
                  r="34"
                  fill="#070B14"
                  stroke={isPressed('r3') ? '#F59E0B' : '#3B4B6B'}
                  strokeWidth={isPressed('r3') ? '3' : '2'}
                />
                <path d="M 424 192 v 5 M 424 255 v 5 M 390 226 h 5 M 453 226 h 5" stroke="#475569" strokeWidth="1.5" />
                <circle
                  cx={424 + liveInput.rx * 13}
                  cy={226 + liveInput.ry * 13}
                  r="23"
                  fill={isPressed('r3') ? '#F59E0B' : 'url(#dsStickCap)'}
                  stroke={isPressed('r3') ? '#FDE68A' : '#64748B'}
                  strokeWidth="2.2"
                />
                <circle
                  cx={424 + liveInput.rx * 13}
                  cy={226 + liveInput.ry * 13}
                  r="17"
                  fill="none"
                  stroke={isPressed('r3') ? '#FEF3C7' : '#475569'}
                  strokeWidth="1.1"
                />
                <circle
                  cx={424 + liveInput.rx * 13}
                  cy={226 + liveInput.ry * 13}
                  r="11.5"
                  fill={isPressed('r3') ? '#D97706' : '#0F172A'}
                />
              </g>

              {/* Center Speaker Dots & PS Button */}
              <g fill="#05080F" opacity="0.85">
                {[-12, -6, 0, 6, 12].map((dx) => (
                  <circle key={dx} cx={340 + dx} cy="197" r="1.4" />
                ))}
              </g>
              <g className="cursor-pointer" {...pressHandler('ps')}>
                <circle
                  cx="340"
                  cy="218"
                  r="14"
                  fill={isPressed('ps') ? '#0070D1' : '#0F172A'}
                  stroke={isPressed('ps') ? '#38BDF8' : '#64748B'}
                  strokeWidth="2"
                />
                <text
                  x="340"
                  y="222"
                  textAnchor="middle"
                  fontSize="9"
                  fontWeight="800"
                  fill={isPressed('ps') ? '#FFFFFF' : '#CBD5E1'}
                >
                  PS
                </text>
              </g>

              {/* DualSense Mic Mute Button */}
              <g className="cursor-pointer" {...pressHandler('mute')}>
                <rect
                  x="326"
                  y="240"
                  width="28"
                  height="7"
                  rx="3.5"
                  fill={
                    isPressed('mute')
                      ? '#F59E0B'
                      : config.ds_mute_action === 1
                      ? '#F59E0B'
                      : '#334155'
                  }
                  stroke="#64748B"
                  strokeWidth="1"
                />
              </g>
            </svg>
          ) : (
            <svg
              viewBox="0 0 680 420"
              className="w-full h-auto select-none"
              role="img"
              aria-label="Mando Original Sony PlayStation 4 DualShock 4 en 2D"
            >
              <defs>
                <linearGradient id="ds4ObsidianShell" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#2D3B53" />
                  <stop offset="50%" stopColor="#182235" />
                  <stop offset="100%" stopColor="#0A101E" />
                </linearGradient>
                <linearGradient id="ds4ShellRim" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#94A3B8" stopOpacity="0.6" />
                  <stop offset="100%" stopColor="#1E293B" stopOpacity="0.3" />
                </linearGradient>
                <linearGradient id="ds4PadSurface" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor="#243044" />
                  <stop offset="100%" stopColor="#0F172A" />
                </linearGradient>
                <radialGradient id="ds4StickCap" cx="45%" cy="32%" r="68%">
                  <stop offset="0%" stopColor="#475569" />
                  <stop offset="65%" stopColor="#1E293B" />
                  <stop offset="100%" stopColor="#0B1120" />
                </radialGradient>
                <pattern id="ds4TouchDots" width="9" height="9" patternUnits="userSpaceOnUse">
                  <circle cx="4.5" cy="4.5" r="1" fill="#475569" opacity="0.55" />
                </pattern>
                <filter id="ds4Glow" x="-30%" y="-30%" width="160%" height="160%">
                  <feGaussianBlur stdDeviation="5.5" result="blur" />
                  <feMerge>
                    <feMergeNode in="blur" />
                    <feMergeNode in="SourceGraphic" />
                  </feMerge>
                </filter>
                <filter id="ds4Drop" x="-10%" y="-10%" width="120%" height="125%">
                  <feDropShadow dx="0" dy="10" stdDeviation="10" floodColor="#000000" floodOpacity="0.65" />
                </filter>
              </defs>

              {/* Shoulder Bumpers Behind Top Profile */}
              <path
                d="M 134 62 C 145 40, 200 38, 224 52 L 226 70 L 128 78 Z"
                fill={isPressed('l1') ? '#22D3EE' : '#0B1120'}
                stroke={isPressed('l1') ? '#67E8F9' : '#475569'}
                strokeWidth="2"
                className="cursor-pointer transition-colors"
                {...pressHandler('l1')}
              />
              <path
                d="M 546 62 C 535 40, 480 38, 456 52 L 454 70 L 552 78 Z"
                fill={isPressed('r1') ? '#22D3EE' : '#0B1120'}
                stroke={isPressed('r1') ? '#67E8F9' : '#475569'}
                strokeWidth="2"
                className="cursor-pointer transition-colors"
                {...pressHandler('r1')}
              />

              {/* DualShock 4 Classic Sculpted Shell */}
              <path
                d="M 128 70 C 166 56, 214 64, 238 66 L 442 66 C 466 64, 514 56, 552 70
                   C 596 88, 622 198, 626 292 C 629 354, 586 382, 548 368
                   C 516 356, 498 308, 474 274 C 454 252, 426 248, 340 248
                   C 254 248, 226 252, 206 274 C 182 308, 164 356, 132 368
                   C 94 382, 51 354, 54 292 C 58 198, 84 88, 128 70 Z"
                fill="url(#ds4ObsidianShell)"
                stroke="#475569"
                strokeWidth="2.6"
                filter="url(#ds4Drop)"
              />
              <path
                d="M 130 73 C 166 60, 214 67, 238 69 L 442 69 C 466 67, 514 60, 550 73
                   C 592 90, 617 197, 621 290"
                fill="none"
                stroke="url(#ds4ShellRim)"
                strokeWidth="1.6"
                pointerEvents="none"
              />

              {/* Circular Wing Insets around D-pad & Face Buttons */}
              <circle cx="145" cy="154" r="60" fill="#101829" stroke="#3B4B6B" strokeWidth="1.6" />
              <circle cx="535" cy="154" r="60" fill="#101829" stroke="#3B4B6B" strokeWidth="1.6" />

              {/* Rumble Wave Feedback on Grips */}
              {(rumblePhase === 1 || rumblePhase === 3) && (
                <g opacity="0.85" stroke="#22D3EE" strokeWidth="2.5" fill="none">
                  <path d="M 48 250 C 32 276, 30 308, 42 332" />
                  <path d="M 34 242 C 14 274, 12 312, 28 342" />
                </g>
              )}
              {(rumblePhase === 2 || rumblePhase === 3) && (
                <g opacity="0.85" stroke="#34D399" strokeWidth="2.5" fill="none">
                  <path d="M 632 250 C 648 276, 650 308, 638 332" />
                  <path d="M 646 242 C 666 274, 668 312, 652 342" />
                </g>
              )}

              {/* Signature Top Prism Lightbar */}
              {!lightbar.off && (
                <rect
                  x="260"
                  y="60"
                  width="160"
                  height="6"
                  rx="3"
                  fill={lightbar.hex}
                  opacity={lightbar.opacity}
                  filter="url(#ds4Glow)"
                />
              )}

              {/* Rectangular DualShock 4 Touchpad */}
              <rect
                x="244"
                y="70"
                width="192"
                height="98"
                rx="8"
                fill={isPressed('touchpad') ? '#0E7490' : 'url(#ds4PadSurface)'}
                stroke={isPressed('touchpad') ? '#22D3EE' : '#475569'}
                strokeWidth="2"
                className="cursor-pointer transition-colors"
                {...pressHandler('touchpad')}
              />
              <rect
                x="250"
                y="86"
                width="180"
                height="76"
                rx="4"
                fill="url(#ds4TouchDots)"
                pointerEvents="none"
              />

              {/* CUH-ZCT2 Lightbar Slit Across Upper Touchpad */}
              {!lightbar.off && (
                <g opacity={lightbar.opacity} filter="url(#ds4Glow)">
                  <rect x="262" y="76" width="156" height="5.5" rx="2.75" fill={lightbar.hex} />
                  <rect x="268" y="77.5" width="144" height="2" rx="1" fill="#FFFFFF" opacity="0.65" />
                </g>
              )}

              <text
                x="340"
                y="128"
                textAnchor="middle"
                fontSize="10"
                fontFamily="JetBrains Mono, monospace"
                fontWeight="700"
                fill={isPressed('touchpad') ? '#ECFEFF' : '#94A3B8'}
                pointerEvents="none"
              >
                {isPressed('touchpad') ? 'TOUCHPAD ACTIVO' : 'PANEL TÁCTIL DS4'}
              </text>

              {/* Share & Options Pill Buttons */}
              <g className="cursor-pointer" {...pressHandler('share')}>
                <rect
                  x="212"
                  y="88"
                  width="14"
                  height="28"
                  rx="7"
                  fill={isPressed('share') ? '#22D3EE' : '#1E293B'}
                  stroke={isPressed('share') ? '#67E8F9' : '#64748B'}
                  strokeWidth="2"
                />
                <text
                  x="219"
                  y="80"
                  textAnchor="middle"
                  fontSize="8"
                  fontWeight="700"
                  fill={isPressed('share') ? '#22D3EE' : '#94A3B8'}
                >
                  SHARE
                </text>
              </g>

              <g className="cursor-pointer" {...pressHandler('options')}>
                <rect
                  x="454"
                  y="88"
                  width="14"
                  height="28"
                  rx="7"
                  fill={isPressed('options') ? '#22D3EE' : '#1E293B'}
                  stroke={isPressed('options') ? '#67E8F9' : '#64748B'}
                  strokeWidth="2"
                />
                <text
                  x="461"
                  y="80"
                  textAnchor="middle"
                  fontSize="8"
                  fontWeight="700"
                  fill={isPressed('options') ? '#22D3EE' : '#94A3B8'}
                >
                  OPTIONS
                </text>
              </g>

              {/* Classic DS4 D-Pad with Directional Chevrons */}
              <g>
                <g className="cursor-pointer" {...pressHandler('dpad_up')}>
                  <polygon
                    points="135,112 155,112 155,140 145,148 135,140"
                    fill={isPressed('dpad_up') ? '#22D3EE' : '#1E293B'}
                    stroke={isPressed('dpad_up') ? '#67E8F9' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="141,126 145,120 149,126" fill="none" stroke={isPressed('dpad_up') ? '#FFFFFF' : '#64748B'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
                <g className="cursor-pointer" {...pressHandler('dpad_down')}>
                  <polygon
                    points="135,168 145,160 155,168 155,196 135,196"
                    fill={isPressed('dpad_down') ? '#22D3EE' : '#1E293B'}
                    stroke={isPressed('dpad_down') ? '#67E8F9' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="141,182 145,188 149,182" fill="none" stroke={isPressed('dpad_down') ? '#FFFFFF' : '#64748B'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
                <g className="cursor-pointer" {...pressHandler('dpad_left')}>
                  <polygon
                    points="102,144 130,144 138,154 130,164 102,164"
                    fill={isPressed('dpad_left') ? '#22D3EE' : '#1E293B'}
                    stroke={isPressed('dpad_left') ? '#67E8F9' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="118,150 112,154 118,158" fill="none" stroke={isPressed('dpad_left') ? '#FFFFFF' : '#64748B'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
                <g className="cursor-pointer" {...pressHandler('dpad_right')}>
                  <polygon
                    points="160,144 188,144 188,164 160,164 152,154"
                    fill={isPressed('dpad_right') ? '#22D3EE' : '#1E293B'}
                    stroke={isPressed('dpad_right') ? '#67E8F9' : '#64748B'}
                    strokeWidth="2"
                  />
                  <polyline points="172,150 178,154 172,158" fill="none" stroke={isPressed('dpad_right') ? '#FFFFFF' : '#64748B'} strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
                </g>
              </g>

              {/* PlayStation Action Buttons */}
              <g>
                <g className="cursor-pointer" {...pressHandler('triangle')}>
                  <circle
                    cx="535"
                    cy="118"
                    r="18"
                    fill={isPressed('triangle') ? '#10B981' : '#1E293B'}
                    stroke={isPressed('triangle') ? '#6EE7B7' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="535" cy="118" r="14.5" fill="none" stroke="#94A3B8" strokeWidth="1" opacity="0.3" />
                  <polygon
                    points="535,107 545,125 525,125"
                    fill="none"
                    stroke={isPressed('triangle') ? '#FFFFFF' : '#34D399'}
                    strokeWidth="2.6"
                    strokeLinejoin="round"
                  />
                </g>

                <g className="cursor-pointer" {...pressHandler('circle')}>
                  <circle
                    cx="573"
                    cy="154"
                    r="18"
                    fill={isPressed('circle') ? '#EF4444' : '#1E293B'}
                    stroke={isPressed('circle') ? '#FCA5A5' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="573" cy="154" r="14.5" fill="none" stroke="#94A3B8" strokeWidth="1" opacity="0.3" />
                  <circle
                    cx="573"
                    cy="154"
                    r="9"
                    fill="none"
                    stroke={isPressed('circle') ? '#FFFFFF' : '#F87171'}
                    strokeWidth="2.6"
                  />
                </g>

                <g className="cursor-pointer" {...pressHandler('cross')}>
                  <circle
                    cx="535"
                    cy="190"
                    r="18"
                    fill={isPressed('cross') ? '#2563EB' : '#1E293B'}
                    stroke={isPressed('cross') ? '#93C5FD' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="535" cy="190" r="14.5" fill="none" stroke="#94A3B8" strokeWidth="1" opacity="0.3" />
                  <path
                    d="M 527 182 L 543 198 M 543 182 L 527 198"
                    stroke={isPressed('cross') ? '#FFFFFF' : '#60A5FA'}
                    strokeWidth="2.8"
                    strokeLinecap="round"
                  />
                </g>

                <g className="cursor-pointer" {...pressHandler('square')}>
                  <circle
                    cx="497"
                    cy="154"
                    r="18"
                    fill={isPressed('square') ? '#EC4899' : '#1E293B'}
                    stroke={isPressed('square') ? '#F9A8D4' : '#64748B'}
                    strokeWidth="2"
                  />
                  <circle cx="497" cy="154" r="14.5" fill="none" stroke="#94A3B8" strokeWidth="1" opacity="0.3" />
                  <rect
                    x="489"
                    y="146"
                    width="16"
                    height="16"
                    fill="none"
                    stroke={isPressed('square') ? '#FFFFFF' : '#F472B6'}
                    strokeWidth="2.6"
                  />
                </g>
              </g>

              {/* Left Thumbstick (L3) */}
              <g className="cursor-pointer" {...pressHandler('l3')}>
                <circle
                  cx="256"
                  cy="222"
                  r="34"
                  fill="#070B14"
                  stroke={isPressed('l3') ? '#F59E0B' : '#3B4B6B'}
                  strokeWidth={isPressed('l3') ? '3' : '2'}
                />
                <path d="M 256 188 v 5 M 256 251 v 5 M 222 222 h 5 M 285 222 h 5" stroke="#475569" strokeWidth="1.5" />
                <circle
                  cx={256 + liveInput.lx * 13}
                  cy={222 + liveInput.ly * 13}
                  r="23"
                  fill={isPressed('l3') ? '#F59E0B' : 'url(#ds4StickCap)'}
                  stroke={isPressed('l3') ? '#FDE68A' : '#64748B'}
                  strokeWidth="2.2"
                />
                <circle
                  cx={256 + liveInput.lx * 13}
                  cy={222 + liveInput.ly * 13}
                  r="17"
                  fill="none"
                  stroke={isPressed('l3') ? '#FEF3C7' : '#475569'}
                  strokeWidth="1.1"
                />
                <circle
                  cx={256 + liveInput.lx * 13}
                  cy={222 + liveInput.ly * 13}
                  r="11.5"
                  fill={isPressed('l3') ? '#D97706' : '#0B1120'}
                />
              </g>

              {/* Right Thumbstick (R3) */}
              <g className="cursor-pointer" {...pressHandler('r3')}>
                <circle
                  cx="424"
                  cy="222"
                  r="34"
                  fill="#070B14"
                  stroke={isPressed('r3') ? '#F59E0B' : '#3B4B6B'}
                  strokeWidth={isPressed('r3') ? '3' : '2'}
                />
                <path d="M 424 188 v 5 M 424 251 v 5 M 390 222 h 5 M 453 222 h 5" stroke="#475569" strokeWidth="1.5" />
                <circle
                  cx={424 + liveInput.rx * 13}
                  cy={222 + liveInput.ry * 13}
                  r="23"
                  fill={isPressed('r3') ? '#F59E0B' : 'url(#ds4StickCap)'}
                  stroke={isPressed('r3') ? '#FDE68A' : '#64748B'}
                  strokeWidth="2.2"
                />
                <circle
                  cx={424 + liveInput.rx * 13}
                  cy={222 + liveInput.ry * 13}
                  r="17"
                  fill="none"
                  stroke={isPressed('r3') ? '#FEF3C7' : '#475569'}
                  strokeWidth="1.1"
                />
                <circle
                  cx={424 + liveInput.rx * 13}
                  cy={222 + liveInput.ry * 13}
                  r="11.5"
                  fill={isPressed('r3') ? '#D97706' : '#0B1120'}
                />
              </g>

              {/* Center Speaker Dots & PS Button */}
              <g fill="#05080F" opacity="0.85">
                {[-12, -6, 0, 6, 12].map((dx) => (
                  <circle key={dx} cx={340 + dx} cy="190" r="1.4" />
                ))}
              </g>
              <g className="cursor-pointer" {...pressHandler('ps')}>
                <circle
                  cx="340"
                  cy="214"
                  r="14"
                  fill={isPressed('ps') ? '#0070D1' : '#0B1120'}
                  stroke={isPressed('ps') ? '#38BDF8' : '#64748B'}
                  strokeWidth="2"
                />
                <text
                  x="340"
                  y="218"
                  textAnchor="middle"
                  fontSize="9"
                  fontWeight="800"
                  fill={isPressed('ps') ? '#FFFFFF' : '#CBD5E1'}
                >
                  PS
                </text>
              </g>
            </svg>
          )}
        </div>

        {/* Bottom Clean Status Strip */}
        <div className="w-full mt-3 pt-3 border-t border-slate-800/80 flex flex-col sm:flex-row items-center justify-between gap-2 text-xs">
          <div className="flex items-center gap-2 text-slate-300">
            <span className="text-slate-400">Entradas activas:</span>
            <span className="font-mono font-semibold text-cyan-300">
              {activeList.length > 0 ? activeList.join(' · ') : 'En reposo (pulsa cualquier botón o haz clic arriba)'}
            </span>
          </div>

          <div className="flex items-center gap-3 font-mono text-[11px] text-slate-400 tabular-nums">
            {rumblePhase > 0 && (
              <span className="flex items-center gap-1 text-emerald-400 font-semibold">
                <Vibrate className="w-3.5 h-3.5" />
                Vibrando
              </span>
            )}
            <span>L: ({liveInput.lx.toFixed(2)}, {liveInput.ly.toFixed(2)})</span>
            <span>R: ({liveInput.rx.toFixed(2)}, {liveInput.ry.toFixed(2)})</span>
          </div>
        </div>
      </div>
    </section>
  );
}
