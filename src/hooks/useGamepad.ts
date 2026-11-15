/**
 * Controller Control Center PS4 - Hook de Mandos en Tiempo Real (Web Gamepad API v1.5.0)
 * Soporte multi-mando real con selección manual sin robo de ranura, vibración por jugador y teclado
 */
import { useEffect, useRef, useState, useCallback } from 'react';
import { ConnectedGamepadInfo, ControllerFamily, LiveGamepadInput } from '../types';

const parseGamepadId = (idStr: string) => {
  const match =
    idStr.match(
      /(?:vendor[:\s]+|vid[_\s]+)([0-9a-fA-F]{4}).*?(?:product[:\s]+|pid[_\s]+)([0-9a-fA-F]{4})/i
    ) || idStr.match(/([0-9a-fA-F]{4})[-_:]([0-9a-fA-F]{4})/);
  if (match) {
    return { vid: match[1].toUpperCase(), pid: match[2].toUpperCase() };
  }
  const lower = idStr.toLowerCase();
  if (lower.includes('dualsense edge')) return { vid: '054C', pid: '0DF2' };
  if (lower.includes('dualsense')) return { vid: '054C', pid: '0CE6' };
  if (lower.includes('dualshock 4') || lower.includes('wireless controller')) {
    return { vid: '054C', pid: '09CC' };
  }
  if (lower.includes('dualshock 3') || lower.includes('sixaxis')) {
    return { vid: '054C', pid: '0268' };
  }
  if (lower.includes('xbox series')) return { vid: '045E', pid: '0B12' };
  if (lower.includes('xbox one')) return { vid: '045E', pid: '02EA' };
  if (lower.includes('xbox 360')) return { vid: '045E', pid: '028E' };
  if (lower.includes('switch pro')) return { vid: '057E', pid: '2009' };
  if (lower.includes('stadia')) return { vid: '18D1', pid: '9400' };
  if (lower.includes('logitech')) return { vid: '046D', pid: 'C216' };
  if (lower.includes('8bitdo')) return { vid: '2DC8', pid: '3106' };
  return { vid: '054C', pid: '0CE6' };
};

const detectFamily = (idStr: string, vid: string, pid: string): ControllerFamily => {
  const lower = idStr.toLowerCase();
  if (
    lower.includes('dualsense') ||
    (vid === '054C' && (pid === '0CE6' || pid === '0DF2'))
  ) {
    return 'dualsense';
  }
  if (
    lower.includes('dualshock') ||
    lower.includes('054c') ||
    (vid === '054C' && (pid === '05C4' || pid === '09CC' || pid === '0BA0'))
  ) {
    return 'ds4';
  }
  return 'generic';
};

export function useGamepad() {
  const [activeSlot, setActiveSlot] = useState<number>(0);
  const [connectedGamepads, setConnectedGamepads] = useState<ConnectedGamepadInfo[]>([]);
  const [gamepadName, setGamepadName] = useState<string>('Mando Virtual / Teclado');
  const [hardwareDetected, setHardwareDetected] = useState<boolean>(false);
  const [deviceVidPid, setDeviceVidPid] = useState<{ vid: string; pid: string }>({
    vid: '054C',
    pid: '0CE6',
  });

  const keyboardButtonsRef = useRef<Record<string, boolean>>({});
  const keyboardTriggersRef = useRef<{ l2: number; r2: number }>({ l2: 0, r2: 0 });
  const userPickedSlotRef = useRef<boolean>(false);

  const selectSlotManually = useCallback((slot: number) => {
    userPickedSlotRef.current = true;
    setActiveSlot(Math.max(0, Math.min(3, slot)));
  }, []);

  const liveStateRef = useRef<LiveGamepadInput>({
    connected: true,
    id: 'Virtual DualSense (Teclado / Clic)',
    slot: 0,
    lx: 0,
    ly: 0,
    rx: 0,
    ry: 0,
    l2: 0,
    r2: 0,
    buttons: {},
    rawButtons: [],
  });

  const [displayInput, setDisplayInput] = useState<LiveGamepadInput>(liveStateRef.current);

  // Keyboard testing fallback
  useEffect(() => {
    const mapKey = (code: string, pressed: boolean) => {
      const btn = keyboardButtonsRef.current;
      switch (code) {
        case 'KeyW':
        case 'ArrowUp':
          btn['dpad_up'] = pressed;
          break;
        case 'KeyS':
        case 'ArrowDown':
          btn['dpad_down'] = pressed;
          break;
        case 'KeyA':
        case 'ArrowLeft':
          btn['dpad_left'] = pressed;
          break;
        case 'KeyD':
        case 'ArrowRight':
          btn['dpad_right'] = pressed;
          break;
        case 'KeyK':
        case 'Space':
        case 'Enter':
          btn['cross'] = pressed;
          break;
        case 'KeyL':
        case 'Escape':
        case 'Backspace':
          btn['circle'] = pressed;
          break;
        case 'KeyJ':
          btn['square'] = pressed;
          break;
        case 'KeyI':
          btn['triangle'] = pressed;
          break;
        case 'KeyQ':
          btn['l1'] = pressed;
          break;
        case 'KeyE':
          btn['r1'] = pressed;
          break;
        case 'Digit1':
          keyboardTriggersRef.current.l2 = pressed ? 1 : 0;
          btn['l2'] = pressed;
          break;
        case 'Digit2':
          keyboardTriggersRef.current.r2 = pressed ? 1 : 0;
          btn['r2'] = pressed;
          break;
        case 'KeyZ':
          btn['share'] = pressed;
          break;
        case 'KeyX':
          btn['options'] = pressed;
          break;
        case 'KeyC':
          btn['ps'] = pressed;
          break;
        case 'KeyV':
          btn['touchpad'] = pressed;
          break;
        case 'KeyM':
          btn['mute'] = pressed;
          break;
        case 'KeyF':
          btn['l3'] = pressed;
          break;
        case 'KeyG':
          btn['r3'] = pressed;
          break;
      }
    };

    const handleKeyDown = (e: KeyboardEvent) => {
      if (['INPUT', 'TEXTAREA', 'SELECT'].includes((e.target as HTMLElement)?.tagName)) return;
      mapKey(e.code, true);
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (['INPUT', 'TEXTAREA', 'SELECT'].includes((e.target as HTMLElement)?.tagName)) return;
      mapKey(e.code, false);
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, []);

  // Multi-controller Gamepad polling loop (respects manual activeSlot strictly)
  useEffect(() => {
    let animId: number;
    let lastListSig = '';

    const pollGamepads = () => {
      const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
      const foundList: ConnectedGamepadInfo[] = [];

      for (let i = 0; i < Math.min(4, gamepads.length); i++) {
        const gp = gamepads[i];
        if (gp && gp.connected) {
          const { vid, pid } = parseGamepadId(gp.id);
          const idLower = gp.id.toLowerCase();
          const isBluetooth =
            idLower.includes('wireless') ||
            idLower.includes('bluetooth') ||
            idLower.includes('bt');
          foundList.push({
            slot: i,
            id: gp.id,
            vid,
            pid,
            family: detectFamily(gp.id, vid, pid),
            connectionType: isBluetooth ? 'bluetooth' : 'usb',
          });
        }
      }

      const listSig = foundList.map((g) => `${g.slot}:${g.id}`).join('|');
      if (listSig !== lastListSig) {
        lastListSig = listSig;
        setConnectedGamepads(foundList);
      }

      // If user hasn't explicitly chosen a slot yet and activeSlot is empty, attach to first connected pad once
      let effectiveSlot = activeSlot;
      const gpInActiveSlot = gamepads[effectiveSlot];
      if ((!gpInActiveSlot || !gpInActiveSlot.connected) && !userPickedSlotRef.current && foundList.length > 0) {
        effectiveSlot = foundList[0].slot;
        setActiveSlot(effectiveSlot);
      }

      const targetGp = gamepads[effectiveSlot];
      const kbBtn = keyboardButtonsRef.current;

      if (targetGp && targetGp.connected) {
        setHardwareDetected(true);
        setGamepadName(targetGp.id);
        const { vid, pid } = parseGamepadId(targetGp.id);
        setDeviceVidPid({ vid, pid });

        const b = targetGp.buttons;
        const a = targetGp.axes;
        const idLower = targetGp.id.toLowerCase();
        const isBluetooth =
          idLower.includes('wireless') ||
          idLower.includes('bluetooth') ||
          idLower.includes('bt');
        const connectionType: 'bluetooth' | 'usb' = isBluetooth ? 'bluetooth' : 'usb';

        let batteryLevel = 85;
        let isCharging = !isBluetooth;
        let batteryAvailable = false;
        let batterySource: 'api' | 'estimated' | 'usb' = isBluetooth ? 'estimated' : 'usb';

        const rawBattery = (targetGp as any).battery;
        if (rawBattery && typeof rawBattery.level === 'number') {
          batteryAvailable = true;
          batteryLevel = Math.round(rawBattery.level * 100);
          isCharging = !!rawBattery.charging;
          batterySource = 'api';
        } else if (typeof (targetGp as any).batteryLevel === 'number') {
          batteryAvailable = true;
          batteryLevel = Math.round(
            (targetGp as any).batteryLevel <= 1
              ? (targetGp as any).batteryLevel * 100
              : (targetGp as any).batteryLevel
          );
          batterySource = 'api';
        }

        const l2Val = Math.max(b[6] ? b[6].value : 0, keyboardTriggersRef.current.l2);
        const r2Val = Math.max(b[7] ? b[7].value : 0, keyboardTriggersRef.current.r2);

        liveStateRef.current = {
          connected: true,
          id: targetGp.id,
          slot: effectiveSlot,
          lx: a.length > 0 ? a[0] : 0,
          ly: a.length > 1 ? a[1] : 0,
          rx: a.length > 2 ? a[2] : 0,
          ry: a.length > 3 ? a[3] : 0,
          l2: l2Val,
          r2: r2Val,
          battery: {
            available: batteryAvailable,
            level: batteryLevel,
            charging: isCharging,
            source: batterySource,
            connectionType,
            voltageEstimate: (3.5 + (batteryLevel / 100) * 0.7).toFixed(2) + 'V',
            statusText: isCharging
              ? 'Conectado por Cable (Cargando)'
              : batteryLevel > 50
              ? 'Batería Óptima'
              : batteryLevel > 20
              ? 'Batería Media (Recarga aconsejada)'
              : 'Batería Crítica (<20%) - Recargar',
          },
          buttons: {
            cross: !!(b[0]?.pressed || kbBtn['cross']),
            circle: !!(b[1]?.pressed || kbBtn['circle']),
            square: !!(b[2]?.pressed || kbBtn['square']),
            triangle: !!(b[3]?.pressed || kbBtn['triangle']),
            l1: !!(b[4]?.pressed || kbBtn['l1']),
            r1: !!(b[5]?.pressed || kbBtn['r1']),
            l2: l2Val > 0.1 || !!(b[6]?.pressed || kbBtn['l2']),
            r2: r2Val > 0.1 || !!(b[7]?.pressed || kbBtn['r2']),
            share: !!(b[8]?.pressed || kbBtn['share']),
            options: !!(b[9]?.pressed || kbBtn['options']),
            l3: !!(b[10]?.pressed || kbBtn['l3']),
            r3: !!(b[11]?.pressed || kbBtn['r3']),
            dpad_up: !!(b[12]?.pressed || kbBtn['dpad_up']),
            dpad_down: !!(b[13]?.pressed || kbBtn['dpad_down']),
            dpad_left: !!(b[14]?.pressed || kbBtn['dpad_left']),
            dpad_right: !!(b[15]?.pressed || kbBtn['dpad_right']),
            ps: !!(b[16]?.pressed || kbBtn['ps']),
            touchpad: !!(b[17]?.pressed || kbBtn['touchpad']),
            mute: !!(b[18]?.pressed || kbBtn['mute']),
          },
          rawButtons: b.map((btn) => btn.pressed),
        };
      } else {
        setHardwareDetected(false);
        setGamepadName(
          foundList.length > 0
            ? `Ranura J${effectiveSlot + 1} libre (Modo Teclado)`
            : 'Mando Virtual / Teclado'
        );
        liveStateRef.current = {
          connected: true,
          id: `Virtual J${effectiveSlot + 1}`,
          slot: effectiveSlot,
          lx: 0,
          ly: 0,
          rx: 0,
          ry: 0,
          l2: keyboardTriggersRef.current.l2,
          r2: keyboardTriggersRef.current.r2,
          buttons: { ...kbBtn },
          rawButtons: [],
        };
      }

      setDisplayInput({ ...liveStateRef.current });
      animId = requestAnimationFrame(pollGamepads);
    };

    animId = requestAnimationFrame(pollGamepads);
    return () => cancelAnimationFrame(animId);
  }, [activeSlot]);

  // Isolated per-slot rumble trigger
  const triggerRumble = useCallback(
    (phase: number, targetSlot?: number) => {
      const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
      let strong = 0;
      let weak = 0;

      if (phase === 1) {
        strong = 1.0;
      } else if (phase === 2) {
        weak = 1.0;
      } else if (phase === 3) {
        strong = 1.0;
        weak = 1.0;
      }

      const slotToVibrate = targetSlot !== undefined ? targetSlot : activeSlot;
      const targets =
        slotToVibrate >= 0 && slotToVibrate < 4
          ? [gamepads[slotToVibrate]]
          : Array.from(gamepads);

      for (const gp of targets) {
        if (gp && gp.connected && (gp as any).vibrationActuator) {
          try {
            (gp as any).vibrationActuator.playEffect('dual-rumble', {
              startDelay: 0,
              duration: phase === 0 ? 50 : 1400,
              weakMagnitude: weak,
              strongMagnitude: strong,
            });
          } catch {
            // Ignore unsupported actuator
          }
        }
      }
    },
    [activeSlot]
  );

  return {
    activeSlot,
    setActiveSlot: selectSlotManually,
    connectedGamepads,
    hasGamepad: connectedGamepads.length > 0,
    hardwareDetected,
    gamepadName,
    deviceVidPid,
    liveInput: displayInput,
    triggerRumble,
  };
}
