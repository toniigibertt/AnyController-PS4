/**
 * OneFamily PS4 - Hook de Mandos en Tiempo Real (Web Gamepad API)
 * Detección de hardware real, vibración dual-motor háptica y simulación por teclado
 */
import { useEffect, useRef, useState, useCallback } from 'react';
import { LiveGamepadInput } from '../types';

export function useGamepad() {
  const [activeSlot, setActiveSlot] = useState<number>(0);
  const [hasGamepad, setHasGamepad] = useState<boolean>(false);
  const [gamepadName, setGamepadName] = useState<string>('Mando Virtual / Teclado');
  const [hardwareDetected, setHardwareDetected] = useState<boolean>(false);
  const [deviceVidPid, setDeviceVidPid] = useState<{ vid: string; pid: string }>({ vid: '054C', pid: '0CE6' });

  const liveStateRef = useRef<LiveGamepadInput>({
    connected: true,
    id: 'Virtual DualSense (Teclado / API)',
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

  // Parse VID & PID from Gamepad ID string if available (e.g., "Vendor: 045e Product: 028e" or "054c-0ce6")
  const parseGamepadId = (idStr: string) => {
    const match = idStr.match(/(?:vendor[:\s]+|vid[_\s]+)([0-9a-fA-F]{4}).*?(?:product[:\s]+|pid[_\s]+)([0-9a-fA-F]{4})/i)
      || idStr.match(/([0-9a-fA-F]{4})[-_:]([0-9a-fA-F]{4})/);
    if (match) {
      return { vid: match[1].toUpperCase(), pid: match[2].toUpperCase() };
    }
    // Heuristic detection based on controller names
    const lower = idStr.toLowerCase();
    if (lower.includes('dualsense edge')) return { vid: '054C', pid: '0DF2' };
    if (lower.includes('dualsense')) return { vid: '054C', pid: '0CE6' };
    if (lower.includes('dualshock 4') || lower.includes('wireless controller')) return { vid: '054C', pid: '09CC' };
    if (lower.includes('dualshock 3') || lower.includes('sixaxis')) return { vid: '054C', pid: '0268' };
    if (lower.includes('xbox series')) return { vid: '045E', pid: '0B12' };
    if (lower.includes('xbox one')) return { vid: '045E', pid: '02EA' };
    if (lower.includes('xbox 360')) return { vid: '045E', pid: '028E' };
    if (lower.includes('switch pro')) return { vid: '057E', pid: '2009' };
    if (lower.includes('stadia')) return { vid: '18D1', pid: '9400' };
    if (lower.includes('logitech')) return { vid: '046D', pid: 'C216' };
    if (lower.includes('8bitdo')) return { vid: '2DC8', pid: '3106' };
    return { vid: '054C', pid: '0CE6' };
  };

  // Keyboard navigation fallback
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Don't intercept if user is typing in an input
      if (['INPUT', 'TEXTAREA'].includes((e.target as HTMLElement)?.tagName)) return;

      const btn = liveStateRef.current.buttons;
      switch (e.code) {
        case 'KeyW':
        case 'ArrowUp':
          btn['dpad_up'] = true;
          break;
        case 'KeyS':
        case 'ArrowDown':
          btn['dpad_down'] = true;
          break;
        case 'KeyA':
        case 'ArrowLeft':
          btn['dpad_left'] = true;
          break;
        case 'KeyD':
        case 'ArrowRight':
          btn['dpad_right'] = true;
          break;
        case 'KeyK':
        case 'Space':
        case 'Enter':
          btn['cross'] = true;
          break;
        case 'KeyL':
        case 'Escape':
        case 'Backspace':
          btn['circle'] = true;
          break;
        case 'KeyJ':
          btn['square'] = true;
          break;
        case 'KeyI':
          btn['triangle'] = true;
          break;
        case 'KeyQ':
          btn['l1'] = true;
          break;
        case 'KeyE':
          btn['r1'] = true;
          break;
        case 'Digit1':
          liveStateRef.current.l2 = 1.0;
          btn['l2'] = true;
          break;
        case 'Digit2':
          liveStateRef.current.r2 = 1.0;
          btn['r2'] = true;
          break;
        case 'KeyZ':
          btn['share'] = true;
          break;
        case 'KeyX':
          btn['options'] = true;
          break;
        case 'KeyC':
          btn['ps'] = true;
          break;
        case 'KeyV':
          btn['touchpad'] = true;
          break;
        case 'KeyF':
          btn['l3'] = true;
          break;
        case 'KeyG':
          btn['r3'] = true;
          break;
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (['INPUT', 'TEXTAREA'].includes((e.target as HTMLElement)?.tagName)) return;
      const btn = liveStateRef.current.buttons;
      switch (e.code) {
        case 'KeyW':
        case 'ArrowUp':
          btn['dpad_up'] = false;
          break;
        case 'KeyS':
        case 'ArrowDown':
          btn['dpad_down'] = false;
          break;
        case 'KeyA':
        case 'ArrowLeft':
          btn['dpad_left'] = false;
          break;
        case 'KeyD':
        case 'ArrowRight':
          btn['dpad_right'] = false;
          break;
        case 'KeyK':
        case 'Space':
        case 'Enter':
          btn['cross'] = false;
          break;
        case 'KeyL':
        case 'Escape':
        case 'Backspace':
          btn['circle'] = false;
          break;
        case 'KeyJ':
          btn['square'] = false;
          break;
        case 'KeyI':
          btn['triangle'] = false;
          break;
        case 'KeyQ':
          btn['l1'] = false;
          break;
        case 'KeyE':
          btn['r1'] = false;
          break;
        case 'Digit1':
          liveStateRef.current.l2 = 0;
          btn['l2'] = false;
          break;
        case 'Digit2':
          liveStateRef.current.r2 = 0;
          btn['r2'] = false;
          break;
        case 'KeyZ':
          btn['share'] = false;
          break;
        case 'KeyX':
          btn['options'] = false;
          break;
        case 'KeyC':
          btn['ps'] = false;
          break;
        case 'KeyV':
          btn['touchpad'] = false;
          break;
        case 'KeyF':
          btn['l3'] = false;
          break;
        case 'KeyG':
          btn['r3'] = false;
          break;
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, []);

  // Hardware Gamepad polling loop
  useEffect(() => {
    let animId: number;

    const pollGamepads = () => {
      const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
      let found = false;

      // Check the active slot first, or any connected gamepad
      for (let i = 0; i < gamepads.length; i++) {
        const gp = gamepads[i];
        if (gp && gp.connected) {
          found = true;
          setHardwareDetected(true);
          setHasGamepad(true);
          setGamepadName(gp.id);
          const { vid, pid } = parseGamepadId(gp.id);
          setDeviceVidPid({ vid, pid });

          // Map inputs from standard Gamepad layout
          const b = gp.buttons;
          const a = gp.axes;

          // Detect connection type (Bluetooth vs USB)
          const idLower = gp.id.toLowerCase();
          const isBluetooth = idLower.includes('wireless') || idLower.includes('bluetooth') || idLower.includes('bt');
          const connectionType: 'bluetooth' | 'usb' = isBluetooth ? 'bluetooth' : 'usb';

          // Check browser-level Gamepad battery (experimental API or WebHID)
          let batteryLevel = 85;
          let isCharging = !isBluetooth;
          let batteryAvailable = false;
          let batterySource: 'api' | 'estimated' | 'usb' = isBluetooth ? 'estimated' : 'usb';

          const rawBattery = (gp as any).battery;
          if (rawBattery && typeof rawBattery.level === 'number') {
            batteryAvailable = true;
            batteryLevel = Math.round(rawBattery.level * 100);
            isCharging = !!rawBattery.charging;
            batterySource = 'api';
          } else if (typeof (gp as any).batteryLevel === 'number') {
            batteryAvailable = true;
            batteryLevel = Math.round((gp as any).batteryLevel <= 1 ? (gp as any).batteryLevel * 100 : (gp as any).batteryLevel);
            batterySource = 'api';
          }

          liveStateRef.current = {
            connected: true,
            id: gp.id,
            slot: i,
            lx: a.length > 0 ? a[0] : 0,
            ly: a.length > 1 ? a[1] : 0,
            rx: a.length > 2 ? a[2] : 0,
            ry: a.length > 3 ? a[3] : 0,
            l2: b[6] ? b[6].value : 0,
            r2: b[7] ? b[7].value : 0,
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
              cross: b[0]?.pressed || false,
              circle: b[1]?.pressed || false,
              square: b[2]?.pressed || false,
              triangle: b[3]?.pressed || false,
              l1: b[4]?.pressed || false,
              r1: b[5]?.pressed || false,
              l2: (b[6]?.value ?? 0) > 0.1 || b[6]?.pressed || false,
              r2: (b[7]?.value ?? 0) > 0.1 || b[7]?.pressed || false,
              share: b[8]?.pressed || false,
              options: b[9]?.pressed || false,
              l3: b[10]?.pressed || false,
              r3: b[11]?.pressed || false,
              dpad_up: b[12]?.pressed || false,
              dpad_down: b[13]?.pressed || false,
              dpad_left: b[14]?.pressed || false,
              dpad_right: b[15]?.pressed || false,
              ps: b[16]?.pressed || false,
              touchpad: b[17]?.pressed || false,
            },
            rawButtons: b.map((btn) => btn.pressed),
          };
          break;
        }
      }

      if (!found) {
        setHardwareDetected(false);
        // Leave virtual inputs active
      }

      setDisplayInput({ ...liveStateRef.current });
      animId = requestAnimationFrame(pollGamepads);
    };

    animId = requestAnimationFrame(pollGamepads);
    return () => cancelAnimationFrame(animId);
  }, [activeSlot]);

  // Rumble trigger function
  const triggerRumble = useCallback((phase: number) => {
    const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
    let strong = 0;
    let weak = 0;

    if (phase === 1) {
      strong = 1.0;
      weak = 0.0;
    } else if (phase === 2) {
      strong = 0.0;
      weak = 1.0;
    } else if (phase === 3) {
      strong = 1.0;
      weak = 1.0;
    }

    for (const gp of gamepads) {
      if (gp && gp.connected && (gp as any).vibrationActuator) {
        try {
          (gp as any).vibrationActuator.playEffect('dual-rumble', {
            startDelay: 0,
            duration: phase === 0 ? 50 : 1500,
            weakMagnitude: weak,
            strongMagnitude: strong,
          });
        } catch {
          // Ignore if unsupported
        }
      }
    }
  }, []);

  return {
    activeSlot,
    setActiveSlot,
    hasGamepad,
    hardwareDetected,
    gamepadName,
    deviceVidPid,
    liveInput: displayInput,
    triggerRumble,
  };
}
