import { useEffect, useRef, useState } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';
import { AppConfig, LiveGamepadInput } from '../types';
import { DS4_COLOR_HEX, DS_COLOR_HEX } from '../constants/options';
import { Eye, Gamepad2, RotateCw, ChevronDown, ChevronUp, Sparkles } from 'lucide-react';

export type Controller3DModel = 'dualsense' | 'ds4';
type CameraPreset = 'front' | 'studio' | 'triggers';

interface Controller3DViewerProps {
  mode: Controller3DModel;
  onModeChange?: (mode: Controller3DModel) => void;
  liveInput: LiveGamepadInput;
  config: AppConfig;
  rumblePhase: number;
  batteryLevel: number;
}

interface ButtonAnimTarget {
  id: string;
  label: string;
  mesh: THREE.Object3D;
  glowMat: THREE.MeshStandardMaterial;
  ring: THREE.Mesh;
  ringMat: THREE.MeshBasicMaterial;
  restPos: THREE.Vector3;
  pressOffset: THREE.Vector3;
  activeColor: THREE.Color;
  baseEmissive: THREE.Color;
  baseEmissiveIntensity: number;
  pressAmount: number;
  pulseTimer: number;
  wasPressed: boolean;
}

interface TriggerAnimTarget {
  id: string;
  pivot: THREE.Group;
  glowMat: THREE.MeshStandardMaterial;
  restRotX: number;
  maxRotX: number;
  activeColor: THREE.Color;
  ring: THREE.Mesh;
  ringMat: THREE.MeshBasicMaterial;
  pulseTimer: number;
  wasPressed: boolean;
}

interface StickAnimTarget {
  id: 'l3' | 'r3';
  pivot: THREE.Group;
  capMat: THREE.MeshStandardMaterial;
  haloMat: THREE.MeshBasicMaterial;
  clickRing: THREE.Mesh;
  clickRingMat: THREE.MeshBasicMaterial;
  restY: number;
  pulseTimer: number;
  wasPressed: boolean;
}

const CAMERA_PRESETS: Record<CameraPreset, { pos: [number, number, number]; target: [number, number, number]; label: string }> = {
  studio: { pos: [0.35, 2.35, 3.65], target: [0, 0.05, 0], label: 'Estudio 3/4' },
  front: { pos: [0, 1.35, 4.15], target: [0, 0.05, 0], label: 'Frontal' },
  triggers: { pos: [0, 1.85, -3.55], target: [0, 0.18, 0], label: 'Gatillos L2/R2' },
};

const BUTTON_DISPLAY_NAMES: Record<string, string> = {
  cross: '✕ Cruz',
  circle: '○ Círculo',
  square: '□ Cuadrado',
  triangle: '△ Triángulo',
  dpad_up: '▲ Arriba',
  dpad_down: '▼ Abajo',
  dpad_left: '◀ Izq',
  dpad_right: '▶ Der',
  l1: 'L1',
  r1: 'R1',
  l2: 'L2',
  r2: 'R2',
  l3: 'L3 Clic',
  r3: 'R3 Clic',
  share: 'Share / Create',
  options: 'Options',
  ps: 'Botón PS',
  touchpad: 'Touchpad',
};

function createSymbolTexture(
  kind: 'cross' | 'circle' | 'square' | 'triangle' | 'ps' | 'dpad' | 'share' | 'options',
  strokeColor: string,
  angleDeg = 0,
): THREE.CanvasTexture {
  const canvas = document.createElement('canvas');
  canvas.width = 256;
  canvas.height = 256;
  const ctx = canvas.getContext('2d');
  if (ctx) {
    ctx.clearRect(0, 0, 256, 256);

    // Subtle dark glassy cap background
    const grad = ctx.createRadialGradient(128, 96, 16, 128, 128, 128);
    grad.addColorStop(0, '#2a3344');
    grad.addColorStop(1, '#10151f');
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, 256, 256);

    ctx.save();
    ctx.translate(128, 128);
    if (angleDeg !== 0) ctx.rotate((angleDeg * Math.PI) / 180);

    ctx.strokeStyle = strokeColor;
    ctx.fillStyle = strokeColor;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.lineWidth = 18;
    ctx.shadowColor = strokeColor;
    ctx.shadowBlur = 14;

    if (kind === 'cross') {
      const r = 56;
      ctx.beginPath();
      ctx.moveTo(-r, -r);
      ctx.lineTo(r, r);
      ctx.moveTo(r, -r);
      ctx.lineTo(-r, r);
      ctx.stroke();
    } else if (kind === 'circle') {
      ctx.beginPath();
      ctx.arc(0, 0, 62, 0, Math.PI * 2);
      ctx.stroke();
    } else if (kind === 'square') {
      const r = 54;
      ctx.strokeRect(-r, -r, r * 2, r * 2);
    } else if (kind === 'triangle') {
      const r = 64;
      ctx.beginPath();
      ctx.moveTo(0, -r);
      ctx.lineTo(r * 0.92, r * 0.72);
      ctx.lineTo(-r * 0.92, r * 0.72);
      ctx.closePath();
      ctx.stroke();
    } else if (kind === 'dpad') {
      ctx.lineWidth = 10;
      ctx.beginPath();
      ctx.moveTo(0, -48);
      ctx.lineTo(38, 18);
      ctx.lineTo(-38, 18);
      ctx.closePath();
      ctx.fill();
    } else if (kind === 'ps') {
      ctx.font = 'bold 92px system-ui, -apple-system, sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('PS', 0, 6);
    } else if (kind === 'share') {
      ctx.lineWidth = 14;
      ctx.beginPath();
      ctx.moveTo(-30, 12);
      ctx.lineTo(0, -34);
      ctx.lineTo(30, 12);
      ctx.stroke();
    } else if (kind === 'options') {
      ctx.lineWidth = 16;
      for (const y of [-32, 0, 32]) {
        ctx.beginPath();
        ctx.moveTo(-40, y);
        ctx.lineTo(40, y);
        ctx.stroke();
      }
    }
    ctx.restore();
  }

  const tex = new THREE.CanvasTexture(canvas);
  tex.colorSpace = THREE.SRGBColorSpace;
  tex.anisotropy = 8;
  tex.needsUpdate = true;
  return tex;
}

function createStageGridTexture(): THREE.CanvasTexture {
  const canvas = document.createElement('canvas');
  canvas.width = 512;
  canvas.height = 512;
  const ctx = canvas.getContext('2d');
  if (ctx) {
    ctx.fillStyle = '#040914';
    ctx.fillRect(0, 0, 512, 512);

    const radial = ctx.createRadialGradient(256, 256, 18, 256, 256, 240);
    radial.addColorStop(0, 'rgba(14, 165, 233, 0.24)');
    radial.addColorStop(0.55, 'rgba(30, 64, 175, 0.08)');
    radial.addColorStop(1, 'rgba(4, 9, 20, 0)');
    ctx.fillStyle = radial;
    ctx.fillRect(0, 0, 512, 512);

    ctx.strokeStyle = 'rgba(56, 189, 248, 0.22)';
    ctx.lineWidth = 2;
    for (const r of [78, 138, 198, 244]) {
      ctx.beginPath();
      ctx.arc(256, 256, r, 0, Math.PI * 2);
      ctx.stroke();
    }
  }
  const tex = new THREE.CanvasTexture(canvas);
  tex.colorSpace = THREE.SRGBColorSpace;
  tex.needsUpdate = true;
  return tex;
}

function createChassisShape(isDualSense: boolean): THREE.Shape {
  const s = new THREE.Shape();
  if (isDualSense) {
    // Sleek futuristic PS5 DualSense top-down silhouette
    s.moveTo(-1.38, 0.72);
    s.bezierCurveTo(-0.72, 0.94, 0.72, 0.94, 1.38, 0.72);
    s.bezierCurveTo(1.88, 0.54, 2.08, 0.14, 1.98, -0.34);
    s.bezierCurveTo(1.88, -0.78, 1.56, -1.08, 1.24, -1.04);
    s.bezierCurveTo(0.95, -1.00, 0.80, -0.60, 0.42, -0.58);
    s.lineTo(-0.42, -0.58);
    s.bezierCurveTo(-0.80, -0.60, -0.95, -1.00, -1.24, -1.04);
    s.bezierCurveTo(-1.56, -1.08, -1.88, -0.78, -1.98, -0.34);
    s.bezierCurveTo(-2.08, 0.14, -1.88, 0.54, -1.38, 0.72);
  } else {
    // Classic ergonomic DualShock 4 PS4 top-down silhouette
    s.moveTo(-1.32, 0.68);
    s.bezierCurveTo(-0.62, 0.84, 0.62, 0.84, 1.32, 0.68);
    s.bezierCurveTo(1.82, 0.52, 2.02, 0.12, 1.92, -0.32);
    s.bezierCurveTo(1.82, -0.74, 1.50, -0.98, 1.22, -0.95);
    s.bezierCurveTo(0.94, -0.92, 0.80, -0.54, 0.40, -0.54);
    s.lineTo(-0.40, -0.54);
    s.bezierCurveTo(-0.80, -0.54, -0.94, -0.92, -1.22, -0.95);
    s.bezierCurveTo(-1.50, -0.98, -1.82, -0.74, -1.92, -0.32);
    s.bezierCurveTo(-2.02, 0.12, -1.82, 0.52, -1.32, 0.68);
  }
  return s;
}

function createCoreTrimShape(): THREE.Shape {
  // DualSense signature inner black center core wing cutout
  const s = new THREE.Shape();
  s.moveTo(-0.66, 0.78);
  s.lineTo(0.66, 0.78);
  s.bezierCurveTo(0.70, 0.34, 0.78, -0.22, 0.78, -0.60);
  s.lineTo(0.42, -0.58);
  s.lineTo(-0.42, -0.58);
  s.lineTo(-0.78, -0.60);
  s.bezierCurveTo(-0.78, -0.22, -0.70, 0.34, -0.66, 0.78);
  return s;
}

function createTouchpadShape(isDualSense: boolean): THREE.Shape {
  const s = new THREE.Shape();
  if (isDualSense) {
    // Wider angled trapezoidal DualSense touchpad
    s.moveTo(-0.64, 0.36);
    s.lineTo(0.64, 0.36);
    s.lineTo(0.56, -0.24);
    s.lineTo(-0.56, -0.24);
    s.closePath();
  } else {
    // Rounded-corner rectangular DS4 touchpad
    const w = 0.58;
    const h = 0.30;
    const r = 0.06;
    s.moveTo(-w + r, h);
    s.lineTo(w - r, h);
    s.quadraticCurveTo(w, h, w, h - r);
    s.lineTo(w, -h + r);
    s.quadraticCurveTo(w, -h, w - r, -h);
    s.lineTo(-w + r, -h);
    s.quadraticCurveTo(-w, -h, -w, -h + r);
    s.lineTo(-w, h - r);
    s.quadraticCurveTo(-w, h, -w + r, h);
  }
  return s;
}

export default function Controller3DViewer({
  mode,
  onModeChange,
  liveInput,
  config,
  rumblePhase,
  batteryLevel,
}: Controller3DViewerProps) {
  const mountRef = useRef<HTMLDivElement | null>(null);
  const [preset, setPreset] = useState<CameraPreset>('studio');
  const [autoRotate, setAutoRotate] = useState<boolean>(false);
  const [hudExpanded, setHudExpanded] = useState<boolean>(true);
  const [activeClickedIds, setActiveClickedIds] = useState<Record<string, boolean>>({});
  const [webglError, setWebglError] = useState<string | null>(null);

  // Synced refs so the render loop reads latest inputs without rebuilding Three.js scene
  const liveInputRef = useRef<LiveGamepadInput>(liveInput);
  liveInputRef.current = liveInput;

  const configRef = useRef<AppConfig>(config);
  configRef.current = config;

  const rumblePhaseRef = useRef<number>(rumblePhase);
  rumblePhaseRef.current = rumblePhase;

  const batteryRef = useRef<number>(batteryLevel);
  batteryRef.current = batteryLevel;

  const autoRotateRef = useRef<boolean>(autoRotate);
  autoRotateRef.current = autoRotate;

  const clickedIdsRef = useRef<Record<string, boolean>>({});
  const cameraMoveRef = useRef<{
    active: boolean;
    pos: THREE.Vector3;
    target: THREE.Vector3;
  }>({
    active: false,
    pos: new THREE.Vector3(...CAMERA_PRESETS.studio.pos),
    target: new THREE.Vector3(...CAMERA_PRESETS.studio.target),
  });

  const handleSelectPreset = (nextPreset: CameraPreset) => {
    setPreset(nextPreset);
    const p = CAMERA_PRESETS[nextPreset];
    cameraMoveRef.current.pos.set(...p.pos);
    cameraMoveRef.current.target.set(...p.target);
    cameraMoveRef.current.active = true;
  };

  // Trigger brief virtual button test pulse when user clicks directly on 3D control
  const triggerClickPulse = (id: string) => {
    clickedIdsRef.current[id] = true;
    setActiveClickedIds((prev) => ({ ...prev, [id]: true }));
    window.setTimeout(() => {
      delete clickedIdsRef.current[id];
      setActiveClickedIds((prev) => {
        const next = { ...prev };
        delete next[id];
        return next;
      });
    }, 260);
  };

  useEffect(() => {
    const container = mountRef.current;
    if (!container) return;

    const width = Math.max(container.clientWidth, 320);
    const height = Math.max(container.clientHeight, 320);

    let renderer: THREE.WebGLRenderer;
    try {
      renderer = new THREE.WebGLRenderer({
        antialias: true,
        powerPreference: 'high-performance',
        alpha: false,
      });
    } catch {
      setWebglError('WebGL no está disponible en este navegador para mostrar la vista 3D.');
      return;
    }

    // Software WebGL fallback check (SwiftShader / llvmpipe)
    let isSoftwareGl = false;
    try {
      const gl = renderer.getContext();
      const dbg = gl.getExtension('WEBGL_debug_renderer_info');
      const glName = dbg
        ? String(gl.getParameter(dbg.UNMASKED_RENDERER_WEBGL))
        : String(gl.getParameter(gl.RENDERER));
      isSoftwareGl = /SwiftShader|llvmpipe|Software Rasterizer/i.test(glName);
    } catch {
      isSoftwareGl = false;
    }

    if (isSoftwareGl && renderer.capabilities.maxSamples > 0) {
      // Rebuild without AA on software GL
      renderer.dispose();
      renderer = new THREE.WebGLRenderer({
        antialias: false,
        powerPreference: 'default',
        alpha: false,
      });
    }

    renderer.setSize(width, height);
    renderer.setPixelRatio(isSoftwareGl ? 1 : Math.min(window.devicePixelRatio || 1, 2));
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.15;
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.shadowMap.enabled = !isSoftwareGl;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    renderer.domElement.style.display = 'block';
    renderer.domElement.style.width = '100%';
    renderer.domElement.style.height = '100%';
    renderer.domElement.style.cursor = 'grab';
    container.appendChild(renderer.domElement);

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x040914);
    scene.fog = new THREE.FogExp2(0x040914, 0.055);

    const pmremGenerator = new THREE.PMREMGenerator(renderer);
    const roomEnv = new RoomEnvironment();
    const envTarget = pmremGenerator.fromScene(roomEnv, 0.04);
    scene.environment = envTarget.texture;
    roomEnv.dispose();
    pmremGenerator.dispose();

    const camera = new THREE.PerspectiveCamera(40, width / height, 0.1, 40);
    camera.position.set(...CAMERA_PRESETS.studio.pos);

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.target.set(...CAMERA_PRESETS.studio.target);
    controls.enableDamping = !isSoftwareGl;
    controls.dampingFactor = 0.07;
    controls.enablePan = false;
    controls.minDistance = 2.65;
    controls.maxDistance = 6.8;
    controls.minPolarAngle = 0.25;
    controls.maxPolarAngle = Math.PI * 0.85;
    controls.update();

    const cancelCameraPreset = () => {
      cameraMoveRef.current.active = false;
    };
    controls.addEventListener('start', cancelCameraPreset);

    // Three-point + ambient studio lighting
    const hemiLight = new THREE.HemisphereLight(0x9bd0ff, 0x0b1528, 0.65);
    scene.add(hemiLight);

    const keyLight = new THREE.DirectionalLight(0xfff1dc, 2.35);
    keyLight.position.set(3.2, 5.2, 4.4);
    keyLight.castShadow = !isSoftwareGl;
    keyLight.shadow.mapSize.width = 1024;
    keyLight.shadow.mapSize.height = 1024;
    keyLight.shadow.camera.near = 0.5;
    keyLight.shadow.camera.far = 16;
    const d = 3.2;
    keyLight.shadow.camera.left = -d;
    keyLight.shadow.camera.right = d;
    keyLight.shadow.camera.top = d;
    keyLight.shadow.camera.bottom = -d;
    keyLight.shadow.bias = -0.0005;
    keyLight.shadow.normalBias = 0.02;
    scene.add(keyLight);

    const rimLight = new THREE.DirectionalLight(0x38bdf8, 1.35);
    rimLight.position.set(-3.4, 2.6, -3.5);
    scene.add(rimLight);

    const accentPoint = new THREE.PointLight(0x00b4ff, 1.25, 7.5, 1.8);
    accentPoint.position.set(0, 1.4, 0.8);
    scene.add(accentPoint);

    // Studio shadow pedestal beneath the floating controller
    const stageGroup = new THREE.Group();
    const stageTex = createStageGridTexture();
    const stagePlane = new THREE.Mesh(
      new THREE.CircleGeometry(3.2, 48),
      new THREE.MeshStandardMaterial({
        map: stageTex,
        color: 0xffffff,
        roughness: 0.72,
        metalness: 0.2,
        transparent: true,
        opacity: 0.92,
      }),
    );
    stagePlane.rotation.x = -Math.PI / 2;
    stagePlane.position.y = -1.06;
    stagePlane.receiveShadow = !isSoftwareGl;
    stageGroup.add(stagePlane);

    const stageHaloRing = new THREE.Mesh(
      new THREE.RingGeometry(1.75, 1.82, 64),
      new THREE.MeshBasicMaterial({
        color: 0x0ea5e9,
        transparent: true,
        opacity: 0.35,
        side: THREE.DoubleSide,
      }),
    );
    stageHaloRing.rotation.x = -Math.PI / 2;
    stageHaloRing.position.y = -1.05;
    stageGroup.add(stageHaloRing);
    scene.add(stageGroup);

    // =========================================================================
    // PROCEDURAL 3D CONTROLLER ASSEMBLY (DualSense PS5 vs DualShock 4 PS4)
    // =========================================================================
    const isDS5 = mode === 'dualsense';
    const rootFloatGroup = new THREE.Group();
    const controllerGroup = new THREE.Group();
    controllerGroup.rotation.x = 0.42; // Tilted toward viewer for natural ergonomic inspection
    rootFloatGroup.add(controllerGroup);
    scene.add(rootFloatGroup);

    const buttonTargets: ButtonAnimTarget[] = [];
    const triggerTargets: TriggerAnimTarget[] = [];
    const stickTargets: StickAnimTarget[] = [];
    const clickableObjects: THREE.Object3D[] = [];

    // Shared texture generators
    const texCross = createSymbolTexture('cross', '#60a5fa');
    const texCircle = createSymbolTexture('circle', '#f87171');
    const texSquare = createSymbolTexture('square', '#f472b6');
    const texTriangle = createSymbolTexture('triangle', '#34d399');
    const texPs = createSymbolTexture('ps', '#e2e8f0');
    const texDpadUp = createSymbolTexture('dpad', '#cbd5e1', 0);
    const texDpadRight = createSymbolTexture('dpad', '#cbd5e1', 90);
    const texDpadDown = createSymbolTexture('dpad', '#cbd5e1', 180);
    const texDpadLeft = createSymbolTexture('dpad', '#cbd5e1', 270);
    const texShare = createSymbolTexture('share', '#cbd5e1');
    const texOptions = createSymbolTexture('options', '#cbd5e1');

    // Main Chassis Extrusion (lies flat in XZ plane after rotateX(-PI/2))
    const shellShape = createChassisShape(isDS5);
    const shellGeo = new THREE.ExtrudeGeometry(shellShape, {
      depth: 0.28,
      bevelEnabled: true,
      bevelThickness: 0.11,
      bevelSize: 0.09,
      bevelSegments: 5,
      curveSegments: 24,
    });
    shellGeo.center();
    shellGeo.rotateX(-Math.PI / 2);

    const shellMat = isDS5
      ? new THREE.MeshPhysicalMaterial({
          color: 0xeaf0f7,
          roughness: 0.28,
          metalness: 0.06,
          clearcoat: 0.5,
          clearcoatRoughness: 0.22,
        })
      : new THREE.MeshPhysicalMaterial({
          color: 0x18202c,
          roughness: 0.36,
          metalness: 0.18,
          clearcoat: 0.35,
          clearcoatRoughness: 0.3,
        });

    const mainBody = new THREE.Mesh(shellGeo, shellMat);
    mainBody.castShadow = !isSoftwareGl;
    mainBody.receiveShadow = !isSoftwareGl;
    controllerGroup.add(mainBody);

    // DualSense signature black center lower trim wing
    if (isDS5) {
      const coreShape = createCoreTrimShape();
      const coreGeo = new THREE.ExtrudeGeometry(coreShape, {
        depth: 0.26,
        bevelEnabled: true,
        bevelThickness: 0.05,
        bevelSize: 0.04,
        bevelSegments: 3,
      });
      coreGeo.center();
      coreGeo.rotateX(-Math.PI / 2);
      const coreMat = new THREE.MeshStandardMaterial({
        color: 0x10151f,
        roughness: 0.45,
        metalness: 0.22,
      });
      const coreMesh = new THREE.Mesh(coreGeo, coreMat);
      coreMesh.position.set(0, -0.005, 0.10);
      coreMesh.castShadow = !isSoftwareGl;
      controllerGroup.add(coreMesh);
    }

    // Ergonomic sculpted Left & Right Hand Grips
    const gripMat = isDS5
      ? new THREE.MeshStandardMaterial({
          color: 0xdbe3ee,
          roughness: 0.42,
          metalness: 0.05,
        })
      : new THREE.MeshStandardMaterial({
          color: 0x111722,
          roughness: 0.55,
          metalness: 0.15,
        });

    const gripGeo = new THREE.CapsuleGeometry(0.28, 0.85, 8, 18);
    const leftGripGroup = new THREE.Group();
    const leftGrip = new THREE.Mesh(gripGeo, gripMat);
    leftGrip.castShadow = !isSoftwareGl;
    leftGripGroup.add(leftGrip);
    leftGripGroup.position.set(-1.38, -0.22, 0.78);
    leftGripGroup.rotation.set(1.06, 0.18, -0.22);
    controllerGroup.add(leftGripGroup);

    const rightGripGroup = new THREE.Group();
    const rightGrip = new THREE.Mesh(gripGeo, gripMat);
    rightGrip.castShadow = !isSoftwareGl;
    rightGripGroup.add(rightGrip);
    rightGripGroup.position.set(1.38, -0.22, 0.78);
    rightGripGroup.rotation.set(1.06, -0.18, 0.22);
    controllerGroup.add(rightGripGroup);

    // Rumble Motor Glow Pods inside grips (light up visibly when rumblePhase > 0!)
    const leftRumbleGlowMat = new THREE.MeshBasicMaterial({
      color: 0x00e5ff,
      transparent: true,
      opacity: 0,
    });
    const rightRumbleGlowMat = new THREE.MeshBasicMaterial({
      color: 0x38bdf8,
      transparent: true,
      opacity: 0,
    });
    const rumbleRingGeo = new THREE.TorusGeometry(0.31, 0.025, 10, 28);
    const leftRumbleRing = new THREE.Mesh(rumbleRingGeo, leftRumbleGlowMat);
    leftRumbleRing.rotation.x = Math.PI / 2;
    leftRumbleRing.position.y = -0.15;
    leftGripGroup.add(leftRumbleRing);

    const rightRumbleRing = new THREE.Mesh(rumbleRingGeo, rightRumbleGlowMat);
    rightRumbleRing.rotation.x = Math.PI / 2;
    rightRumbleRing.position.y = -0.15;
    rightGripGroup.add(rightRumbleRing);

    // Helper to add reactive pulse halo ring above any button
    const makePulseRing = (radius: number, colorHex: number): { ring: THREE.Mesh; ringMat: THREE.MeshBasicMaterial } => {
      const ringMat = new THREE.MeshBasicMaterial({
        color: colorHex,
        transparent: true,
        opacity: 0,
        side: THREE.DoubleSide,
        depthWrite: false,
      });
      const ring = new THREE.Mesh(new THREE.RingGeometry(radius, radius + 0.045, 28), ringMat);
      ring.rotation.x = -Math.PI / 2;
      ring.visible = false;
      return { ring, ringMat };
    };

    const registerButton = (
      id: string,
      mesh: THREE.Object3D,
      glowMat: THREE.MeshStandardMaterial,
      pressDepth: number,
      activeHex: number,
      ringRadius = 0.16,
      ringYOffset = 0.055,
    ) => {
      const { ring, ringMat } = makePulseRing(ringRadius, activeHex);
      ring.position.set(mesh.position.x, mesh.position.y + ringYOffset, mesh.position.z);
      controllerGroup.add(ring);

      mesh.userData.buttonId = id;
      mesh.traverse((child) => {
        child.userData.buttonId = id;
      });
      clickableObjects.push(mesh);

      buttonTargets.push({
        id,
        label: BUTTON_DISPLAY_NAMES[id] || id,
        mesh,
        glowMat,
        ring,
        ringMat,
        restPos: mesh.position.clone(),
        pressOffset: new THREE.Vector3(0, -pressDepth, 0),
        activeColor: new THREE.Color(activeHex),
        baseEmissive: glowMat.emissive.clone(),
        baseEmissiveIntensity: glowMat.emissiveIntensity,
        pressAmount: 0,
        pulseTimer: 0,
        wasPressed: false,
      });
    };

    const TOP_SURFACE_Y = 0.255;

    // -------------------------------------------------------------------------
    // TOUCHPAD + RGB LIGHTBAR HALOS + PLAYER LEDS
    // -------------------------------------------------------------------------
    const tpShape = createTouchpadShape(isDS5);
    const tpGeo = new THREE.ExtrudeGeometry(tpShape, {
      depth: 0.05,
      bevelEnabled: true,
      bevelThickness: 0.015,
      bevelSize: 0.015,
      bevelSegments: 2,
    });
    tpGeo.center();
    tpGeo.rotateX(-Math.PI / 2);

    const tpMat = new THREE.MeshStandardMaterial({
      color: isDS5 ? 0xf1f5f9 : 0x101622,
      roughness: isDS5 ? 0.25 : 0.42,
      metalness: 0.12,
      emissive: new THREE.Color(0x001a33),
      emissiveIntensity: 0.2,
    });
    const tpMesh = new THREE.Mesh(tpGeo, tpMat);
    tpMesh.position.set(0, TOP_SURFACE_Y + 0.025, -0.35);
    tpMesh.castShadow = !isSoftwareGl;
    controllerGroup.add(tpMesh);
    registerButton('touchpad', tpMesh, tpMat, 0.028, 0x00e5ff, 0.48, 0.045);

    // Dynamic Lightbar Materials (controlled live by config lightbar settings!)
    const lightbarMat = new THREE.MeshStandardMaterial({
      color: 0x00aaff,
      emissive: new THREE.Color(0x0099ff),
      emissiveIntensity: 2.2,
      roughness: 0.15,
      metalness: 0.1,
    });

    if (isDS5) {
      // DualSense dual angled side lightbars flanking the touchpad
      const stripGeo = new THREE.BoxGeometry(0.052, 0.028, 0.60);
      const leftStrip = new THREE.Mesh(stripGeo, lightbarMat);
      leftStrip.position.set(-0.635, TOP_SURFACE_Y + 0.015, -0.35);
      leftStrip.rotation.y = -0.14;
      controllerGroup.add(leftStrip);

      const rightStrip = new THREE.Mesh(stripGeo, lightbarMat);
      rightStrip.position.set(0.635, TOP_SURFACE_Y + 0.015, -0.35);
      rightStrip.rotation.y = 0.14;
      controllerGroup.add(rightStrip);

      // 5 White Player Indicator LEDs under touchpad
      const playerLedMat = new THREE.MeshStandardMaterial({
        color: 0xffffff,
        emissive: new THREE.Color(0xbae6fd),
        emissiveIntensity: 1.5,
      });
      for (let i = -2; i <= 2; i++) {
        const dot = new THREE.Mesh(new THREE.BoxGeometry(0.032, 0.014, 0.018), playerLedMat);
        dot.position.set(i * 0.075, TOP_SURFACE_Y + 0.012, -0.02);
        controllerGroup.add(dot);
      }
    } else {
      // DualShock 4 iconic rear/top lightbar prism + slim touchpad slit
      const ds4Prism = new THREE.Mesh(new THREE.BoxGeometry(0.84, 0.08, 0.06), lightbarMat);
      ds4Prism.position.set(0, 0.06, -0.82);
      controllerGroup.add(ds4Prism);

      const ds4Slit = new THREE.Mesh(new THREE.BoxGeometry(1.02, 0.015, 0.022), lightbarMat);
      ds4Slit.position.set(0, TOP_SURFACE_Y + 0.056, -0.60);
      controllerGroup.add(ds4Slit);
    }

    // -------------------------------------------------------------------------
    // FACE BUTTONS (✕ ○ □ △)
    // -------------------------------------------------------------------------
    const faceBaseX = 1.16;
    const faceBaseZ = -0.14;
    const faceSpacing = 0.23;
    const btnGeo = new THREE.CylinderGeometry(0.105, 0.112, 0.055, 28);
    const decalGeo = new THREE.CircleGeometry(0.095, 24);
    decalGeo.rotateX(-Math.PI / 2);

    const faceDefs: Array<{
      id: 'cross' | 'circle' | 'square' | 'triangle';
      dx: number;
      dz: number;
      tex: THREE.CanvasTexture;
      hex: number;
    }> = [
      { id: 'triangle', dx: 0, dz: -faceSpacing, tex: texTriangle, hex: 0x10b981 },
      { id: 'circle', dx: faceSpacing, dz: 0, tex: texCircle, hex: 0xef4444 },
      { id: 'cross', dx: 0, dz: faceSpacing, tex: texCross, hex: 0x3b82f6 },
      { id: 'square', dx: -faceSpacing, dz: 0, tex: texSquare, hex: 0xec4899 },
    ];

    for (const def of faceDefs) {
      const btnGroup = new THREE.Group();
      const capMat = new THREE.MeshStandardMaterial({
        color: 0x1b2230,
        roughness: 0.2,
        metalness: 0.25,
        emissive: new THREE.Color(def.hex),
        emissiveIntensity: 0.14,
      });
      const bodyMesh = new THREE.Mesh(btnGeo, capMat);
      bodyMesh.castShadow = !isSoftwareGl;
      btnGroup.add(bodyMesh);

      const topMat = new THREE.MeshStandardMaterial({
        map: def.tex,
        roughness: 0.15,
        metalness: 0.1,
        emissive: new THREE.Color(def.hex),
        emissiveIntensity: 0.25,
      });
      const topMesh = new THREE.Mesh(decalGeo, topMat);
      topMesh.position.y = 0.029;
      btnGroup.add(topMesh);

      btnGroup.position.set(faceBaseX + def.dx, TOP_SURFACE_Y + 0.028, faceBaseZ + def.dz);
      controllerGroup.add(btnGroup);
      registerButton(def.id, btnGroup, topMat, 0.036, def.hex, 0.115, 0.036);
    }

    // -------------------------------------------------------------------------
    // D-PAD (CRUCETA 4 DIRECCIONES)
    // -------------------------------------------------------------------------
    const dpadBaseX = -1.16;
    const dpadBaseZ = -0.14;
    const dpadDist = 0.205;
    const dpadArmGeo = new THREE.BoxGeometry(0.155, 0.052, 0.195);
    const dpadTopGeo = new THREE.PlaneGeometry(0.13, 0.15);
    dpadTopGeo.rotateX(-Math.PI / 2);

    const dpadCenter = new THREE.Mesh(
      new THREE.CylinderGeometry(0.11, 0.12, 0.042, 16),
      new THREE.MeshStandardMaterial({ color: 0x171d29, roughness: 0.35, metalness: 0.2 }),
    );
    dpadCenter.position.set(dpadBaseX, TOP_SURFACE_Y + 0.018, dpadBaseZ);
    controllerGroup.add(dpadCenter);

    const dpadDefs: Array<{
      id: 'dpad_up' | 'dpad_down' | 'dpad_left' | 'dpad_right';
      dx: number;
      dz: number;
      rotY: number;
      tex: THREE.CanvasTexture;
    }> = [
      { id: 'dpad_up', dx: 0, dz: -dpadDist, rotY: 0, tex: texDpadUp },
      { id: 'dpad_down', dx: 0, dz: dpadDist, rotY: 0, tex: texDpadDown },
      { id: 'dpad_left', dx: -dpadDist, dz: 0, rotY: Math.PI / 2, tex: texDpadLeft },
      { id: 'dpad_right', dx: dpadDist, dz: 0, rotY: Math.PI / 2, tex: texDpadRight },
    ];

    for (const def of dpadDefs) {
      const armGroup = new THREE.Group();
      const armMat = new THREE.MeshStandardMaterial({
        color: 0x202938,
        roughness: 0.25,
        metalness: 0.22,
        emissive: new THREE.Color(0x004466),
        emissiveIntensity: 0.12,
      });
      const armMesh = new THREE.Mesh(dpadArmGeo, armMat);
      armMesh.rotation.y = def.rotY;
      armMesh.castShadow = !isSoftwareGl;
      armGroup.add(armMesh);

      const topMat = new THREE.MeshStandardMaterial({
        map: def.tex,
        roughness: 0.2,
        metalness: 0.15,
        emissive: new THREE.Color(0x00e5ff),
        emissiveIntensity: 0.1,
      });
      const topDecal = new THREE.Mesh(dpadTopGeo, topMat);
      topDecal.position.y = 0.028;
      armGroup.add(topDecal);

      armGroup.position.set(dpadBaseX + def.dx, TOP_SURFACE_Y + 0.026, dpadBaseZ + def.dz);
      controllerGroup.add(armGroup);
      registerButton(def.id, armGroup, topMat, 0.032, 0x00e5ff, 0.12, 0.034);
    }

    // -------------------------------------------------------------------------
    // ANALOG THUMBSTICKS (L3 & R3) - LIVE TILT & CLICKABLE
    // -------------------------------------------------------------------------
    const stickWellGeo = new THREE.TorusGeometry(0.245, 0.028, 12, 32);
    const stickStemGeo = new THREE.CylinderGeometry(0.068, 0.088, 0.19, 20);
    const stickCapGeo = new THREE.CylinderGeometry(0.185, 0.165, 0.055, 28);
    const stickDishGeo = new THREE.CylinderGeometry(0.145, 0.145, 0.015, 24);
    const stickBaseHaloGeo = new THREE.RingGeometry(0.22, 0.275, 32);
    stickBaseHaloGeo.rotateX(-Math.PI / 2);

    const buildStick = (id: 'l3' | 'r3', x: number, z: number) => {
      const wellMesh = new THREE.Mesh(
        stickWellGeo,
        new THREE.MeshStandardMaterial({ color: 0x0b0f17, roughness: 0.5, metalness: 0.35 }),
      );
      wellMesh.rotation.x = Math.PI / 2;
      wellMesh.position.set(x, TOP_SURFACE_Y + 0.006, z);
      controllerGroup.add(wellMesh);

      const haloMat = new THREE.MeshBasicMaterial({
        color: 0x38bdf8,
        transparent: true,
        opacity: 0.25,
        side: THREE.DoubleSide,
      });
      const haloMesh = new THREE.Mesh(stickBaseHaloGeo, haloMat);
      haloMesh.position.set(x, TOP_SURFACE_Y + 0.012, z);
      controllerGroup.add(haloMesh);

      const pivot = new THREE.Group();
      pivot.position.set(x, TOP_SURFACE_Y - 0.02, z);

      const stem = new THREE.Mesh(
        stickStemGeo,
        new THREE.MeshStandardMaterial({ color: 0x161c26, roughness: 0.35, metalness: 0.4 }),
      );
      stem.position.y = 0.095;
      pivot.add(stem);

      const capMat = new THREE.MeshStandardMaterial({
        color: 0x1b222e,
        roughness: 0.65,
        metalness: 0.12,
        emissive: new THREE.Color(0x0088cc),
        emissiveIntensity: 0.08,
      });
      const cap = new THREE.Mesh(stickCapGeo, capMat);
      cap.position.y = 0.195;
      cap.castShadow = !isSoftwareGl;
      pivot.add(cap);

      const dish = new THREE.Mesh(
        stickDishGeo,
        new THREE.MeshStandardMaterial({ color: 0x11161f, roughness: 0.75, metalness: 0.05 }),
      );
      dish.position.y = 0.222;
      pivot.add(dish);

      pivot.userData.buttonId = id;
      pivot.traverse((c) => {
        c.userData.buttonId = id;
      });
      clickableObjects.push(pivot);
      controllerGroup.add(pivot);

      const { ring: clickRing, ringMat: clickRingMat } = makePulseRing(0.24, 0xf59e0b);
      clickRing.position.set(x, TOP_SURFACE_Y + 0.22, z);
      controllerGroup.add(clickRing);

      stickTargets.push({
        id,
        pivot,
        capMat,
        haloMat,
        clickRing,
        clickRingMat,
        restY: TOP_SURFACE_Y - 0.02,
        pulseTimer: 0,
        wasPressed: false,
      });
    };

    buildStick('l3', -0.52, 0.22);
    buildStick('r3', 0.52, 0.22);

    // -------------------------------------------------------------------------
    // SYSTEM BUTTONS (SHARE / CREATE, OPTIONS, PS HOME)
    // -------------------------------------------------------------------------
    const pillGeo = new THREE.CapsuleGeometry(0.042, 0.09, 6, 12);
    const pillTopGeo = new THREE.PlaneGeometry(0.10, 0.10);
    pillTopGeo.rotateX(-Math.PI / 2);

    const createSystemPill = (
      id: 'share' | 'options',
      x: number,
      z: number,
      rotZ: number,
      tex: THREE.CanvasTexture,
    ) => {
      const grp = new THREE.Group();
      const mat = new THREE.MeshStandardMaterial({
        color: isDS5 ? 0xdbe3ee : 0x263143,
        roughness: 0.28,
        metalness: 0.25,
        emissive: new THREE.Color(0x0088ff),
        emissiveIntensity: 0.12,
      });
      const capsule = new THREE.Mesh(pillGeo, mat);
      capsule.rotation.z = rotZ;
      grp.add(capsule);

      const decal = new THREE.Mesh(
        pillTopGeo,
        new THREE.MeshBasicMaterial({ map: tex, transparent: true, opacity: 0.85 }),
      );
      decal.position.y = 0.045;
      grp.add(decal);

      grp.position.set(x, TOP_SURFACE_Y + 0.02, z);
      controllerGroup.add(grp);
      registerButton(id, grp, mat, 0.028, 0x38bdf8, 0.10, 0.05);
    };

    createSystemPill('share', -0.78, -0.45, 0.25, texShare);
    createSystemPill('options', 0.78, -0.45, -0.25, texOptions);

    // PlayStation Home Center Button
    const psGroup = new THREE.Group();
    const psMat = new THREE.MeshStandardMaterial({
      color: 0x161d29,
      roughness: 0.22,
      metalness: 0.35,
      emissive: new THREE.Color(0x0070d1),
      emissiveIntensity: 0.35,
    });
    const psBase = new THREE.Mesh(new THREE.CylinderGeometry(0.115, 0.122, 0.045, 24), psMat);
    psGroup.add(psBase);

    const psDecal = new THREE.Mesh(
      new THREE.CircleGeometry(0.105, 24).rotateX(-Math.PI / 2),
      new THREE.MeshStandardMaterial({
        map: texPs,
        roughness: 0.18,
        emissive: new THREE.Color(0x38bdf8),
        emissiveIntensity: 0.4,
      }),
    );
    psDecal.position.y = 0.024;
    psGroup.add(psDecal);
    psGroup.position.set(0, TOP_SURFACE_Y + 0.02, 0.16);
    controllerGroup.add(psGroup);
    registerButton('ps', psGroup, psMat, 0.028, 0x0099ff, 0.145, 0.042);

    // -------------------------------------------------------------------------
    // SHOULDER BUMPERS (L1 / R1) & ANALOG TRIGGERS (L2 / R2)
    // -------------------------------------------------------------------------
    const bumperGeo = new THREE.BoxGeometry(0.56, 0.10, 0.22);
    const buildBumper = (id: 'l1' | 'r1', x: number) => {
      const mat = new THREE.MeshStandardMaterial({
        color: 0x1c2433,
        roughness: 0.32,
        metalness: 0.3,
        emissive: new THREE.Color(0x004477),
        emissiveIntensity: 0.12,
      });
      const mesh = new THREE.Mesh(bumperGeo, mat);
      mesh.position.set(x, 0.16, -0.82);
      mesh.rotation.y = x < 0 ? 0.14 : -0.14;
      mesh.castShadow = !isSoftwareGl;
      controllerGroup.add(mesh);
      registerButton(id, mesh, mat, 0.038, 0x00e5ff, 0.24, 0.075);
    };
    buildBumper('l1', -1.16);
    buildBumper('r1', 1.16);

    const triggerGeo = new THREE.BoxGeometry(0.50, 0.21, 0.28);
    const buildTrigger = (id: 'l2' | 'r2', x: number) => {
      const pivot = new THREE.Group();
      pivot.position.set(x, 0.01, -0.86);
      pivot.rotation.y = x < 0 ? 0.14 : -0.14;

      const glowMat = new THREE.MeshStandardMaterial({
        color: 0x151b26,
        roughness: 0.3,
        metalness: 0.38,
        emissive: new THREE.Color(0x003355),
        emissiveIntensity: 0.1,
      });
      const paddle = new THREE.Mesh(triggerGeo, glowMat);
      paddle.position.set(0, -0.06, -0.12);
      paddle.rotation.x = 0.24;
      paddle.castShadow = !isSoftwareGl;
      paddle.userData.buttonId = id;
      pivot.add(paddle);
      pivot.userData.buttonId = id;
      clickableObjects.push(pivot);
      controllerGroup.add(pivot);

      const { ring, ringMat } = makePulseRing(0.24, 0x38bdf8);
      ring.position.set(x, 0.12, -1.04);
      controllerGroup.add(ring);

      triggerTargets.push({
        id,
        pivot,
        glowMat,
        restRotX: 0,
        maxRotX: 0.44,
        activeColor: new THREE.Color(0x00e5ff),
        ring,
        ringMat,
        pulseTimer: 0,
        wasPressed: false,
      });
    };
    buildTrigger('l2', -1.16);
    buildTrigger('r2', 1.16);

    // Raycaster for interactive clicking directly on 3D buttons
    const raycaster = new THREE.Raycaster();
    const pointer = new THREE.Vector2();
    let pointerDownX = 0;
    let pointerDownY = 0;

    const onPointerDown = (e: PointerEvent) => {
      pointerDownX = e.clientX;
      pointerDownY = e.clientY;
      renderer.domElement.style.cursor = 'grabbing';
    };

    const onPointerUp = (e: PointerEvent) => {
      renderer.domElement.style.cursor = 'grab';
      const dist = Math.hypot(e.clientX - pointerDownX, e.clientY - pointerDownY);
      if (dist > 7) return; // Ignore camera orbit drags

      const rect = renderer.domElement.getBoundingClientRect();
      pointer.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      pointer.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;
      raycaster.setFromCamera(pointer, camera);

      const hits = raycaster.intersectObjects(clickableObjects, true);
      if (hits.length > 0) {
        const hitId = hits[0].object.userData.buttonId as string | undefined;
        if (hitId) triggerClickPulse(hitId);
      }
    };

    renderer.domElement.addEventListener('pointerdown', onPointerDown);
    renderer.domElement.addEventListener('pointerup', onPointerUp);

    // ResizeObserver so canvas adapts smoothly across layouts
    const handleResize = () => {
      if (!mountRef.current) return;
      const w = Math.max(mountRef.current.clientWidth, 260);
      const h = Math.max(mountRef.current.clientHeight, 260);
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
      renderer.setSize(w, h);
    };
    const resizeObserver = new ResizeObserver(handleResize);
    resizeObserver.observe(container);

    // Pre-allocated animation temporaries outside render loop
    const clock = new THREE.Clock();
    const tempTargetVec = new THREE.Vector3();
    const tempLightColor = new THREE.Color();
    const tempColdColor = new THREE.Color(0x00e5ff);
    const tempHotColor = new THREE.Color(0xf59e0b);
    const tempOffColor = new THREE.Color(0x001122);
    const tempRingColor = new THREE.Color(0x38bdf8);

    let rafId = 0;
    let timeoutId = 0;
    let disposed = false;

    const computeLightbarColor = (elapsed: number, out: THREE.Color) => {
      const cfg = configRef.current;
      const lbMode = isDS5 ? cfg.ds_lightbar_mode : cfg.ds4_lightbar_mode;
      const lbColorIdx = isDS5 ? cfg.ds_lightbar_color : cfg.ds4_lightbar_color;
      const lbBrightIdx = isDS5 ? cfg.ds_brightness : cfg.ds4_brightness;
      const hexPalette = isDS5 ? DS_COLOR_HEX : DS4_COLOR_HEX;

      if (lbMode === 3) {
        // Off
        out.setHex(0x0a0f18);
        return 0.08;
      }

      const brightScale = lbBrightIdx === 0 ? 1.0 : lbBrightIdx === 1 ? 0.62 : 0.34;
      if (lbMode === 4) {
        // Dynamic RGB Rainbow
        out.setHSL((elapsed * 0.18) % 1, 0.92, 0.54);
        return 2.4 * brightScale;
      }
      if (lbMode === 2) {
        // Battery Indicator
        const bat = batteryRef.current;
        out.setHex(bat > 60 ? 0x10b981 : bat > 20 ? 0xf59e0b : 0xef4444);
        return 2.1 * brightScale;
      }
      if (lbMode === 0) {
        // Game Sync
        out.setHex(0x0070d1);
        return 2.1 * brightScale;
      }

      const safeHex = hexPalette[lbColorIdx] || '#0070D1';
      out.set(safeHex);
      if (lbMode === 5) {
        // Breathing Pulse
        const pulse = 0.55 + 0.45 * Math.sin(elapsed * 2.8);
        return 2.5 * brightScale * pulse;
      }
      return 2.25 * brightScale;
    };

    const tick = () => {
      if (disposed) return;

      const delta = Math.min(clock.getDelta(), 0.05);
      const elapsed = clock.getElapsedTime();
      const input = liveInputRef.current;
      const clicked = clickedIdsRef.current;
      const rPhase = rumblePhaseRef.current;

      // 1. Camera Preset Smooth Interpolation
      if (cameraMoveRef.current.active) {
        const alpha = 1 - Math.exp(-8 * delta);
        camera.position.lerp(cameraMoveRef.current.pos, alpha);
        controls.target.lerp(cameraMoveRef.current.target, alpha);
        if (camera.position.distanceTo(cameraMoveRef.current.pos) < 0.025) {
          cameraMoveRef.current.active = false;
        }
      }

      controls.autoRotate = autoRotateRef.current;
      controls.autoRotateSpeed = 1.65;
      controls.update();

      // 2. Ambient Hover & Haptic Rumble Jitter
      let rumbleX = 0;
      let rumbleY = 0;
      let rumbleRotZ = 0;
      if (rPhase > 0) {
        const freq = rPhase === 2 ? 44 : 26;
        const amp = rPhase === 3 ? 0.032 : 0.021;
        rumbleX = Math.sin(elapsed * freq * 1.3) * amp;
        rumbleY = Math.cos(elapsed * freq) * amp * 0.7;
        rumbleRotZ = Math.sin(elapsed * freq * 0.9) * amp * 1.4;
      }
      leftRumbleGlowMat.opacity = rPhase === 1 || rPhase === 3 ? 0.55 + 0.35 * Math.sin(elapsed * 24) : 0;
      rightRumbleGlowMat.opacity = rPhase === 2 || rPhase === 3 ? 0.55 + 0.35 * Math.sin(elapsed * 36) : 0;

      rootFloatGroup.position.x = rumbleX;
      rootFloatGroup.position.y = Math.sin(elapsed * 1.55) * 0.048 + rumbleY;
      controllerGroup.rotation.x = 0.42 + Math.sin(elapsed * 1.1) * 0.028;
      controllerGroup.rotation.z = Math.cos(elapsed * 0.95) * 0.022 + rumbleRotZ;

      // 3. Lightbar Color & Accent Lighting Sync
      const lbIntensity = computeLightbarColor(elapsed, tempLightColor);
      lightbarMat.color.copy(tempLightColor);
      lightbarMat.emissive.copy(tempLightColor);
      lightbarMat.emissiveIntensity = lbIntensity;
      accentPoint.color.copy(tempLightColor);
      accentPoint.intensity = 0.75 + lbIntensity * 0.45;
      (stageHaloRing.material as THREE.MeshBasicMaterial).color.copy(tempLightColor);

      // 4. Button Animations + Shockwave Pulse Rings
      let anyPressedBoost = false;
      for (let i = 0; i < buttonTargets.length; i++) {
        const bt = buttonTargets[i];
        const pressed = Boolean(input.buttons[bt.id] || clicked[bt.id]);
        if (pressed) anyPressedBoost = true;

        if (pressed && !bt.wasPressed) {
          bt.pulseTimer = 1.0;
        }
        bt.wasPressed = pressed;

        bt.pressAmount = THREE.MathUtils.damp(bt.pressAmount, pressed ? 1 : 0, 24, delta);
        tempTargetVec.copy(bt.restPos).addScaledVector(bt.pressOffset, bt.pressAmount);
        bt.mesh.position.copy(tempTargetVec);

        if (pressed) {
          bt.glowMat.emissive.copy(bt.activeColor);
          bt.glowMat.emissiveIntensity = 2.8;
        } else {
          bt.glowMat.emissive.lerpColors(bt.baseEmissive, bt.activeColor, bt.pressAmount);
          bt.glowMat.emissiveIntensity = THREE.MathUtils.lerp(bt.baseEmissiveIntensity, 2.2, bt.pressAmount);
        }

        if (bt.pulseTimer > 0) {
          bt.pulseTimer = Math.max(0, bt.pulseTimer - delta * 2.6);
          const progress = 1 - bt.pulseTimer;
          const s = 1 + progress * 1.75;
          bt.ring.visible = true;
          bt.ring.scale.set(s, s, s);
          bt.ringMat.opacity = bt.pulseTimer * 0.85;
        } else {
          bt.ring.visible = false;
        }
      }

      // 5. Analog Triggers (L2 / R2) Pivoting & Progressive Glow
      for (let i = 0; i < triggerTargets.length; i++) {
        const tr = triggerTargets[i];
        const rawVal = tr.id === 'l2' ? (input.l2 ?? 0) : (input.r2 ?? 0);
        const val = clicked[tr.id] ? 1 : THREE.MathUtils.clamp(rawVal, 0, 1);
        const pressed = val > 0.12;
        if (pressed) anyPressedBoost = true;
        if (pressed && !tr.wasPressed) tr.pulseTimer = 1.0;
        tr.wasPressed = pressed;

        tr.pivot.rotation.x = THREE.MathUtils.damp(tr.pivot.rotation.x, val * tr.maxRotX, 20, delta);
        if (val > 0.04) {
          tr.glowMat.emissive.lerpColors(tempColdColor, tempHotColor, val);
          tr.glowMat.emissiveIntensity = 0.4 + val * 2.4;
        } else {
          tr.glowMat.emissive.copy(tempOffColor);
          tr.glowMat.emissiveIntensity = 0.1;
        }

        if (tr.pulseTimer > 0) {
          tr.pulseTimer = Math.max(0, tr.pulseTimer - delta * 2.6);
          const progress = 1 - tr.pulseTimer;
          const s = 1 + progress * 1.4;
          tr.ring.visible = true;
          tr.ring.scale.set(s, s, s);
          tr.ringMat.opacity = tr.pulseTimer * 0.75;
        } else {
          tr.ring.visible = false;
        }
      }

      // 6. Live 3D Analog Thumbsticks (L3 / R3 Tilt + Depression + Perimeter Glow)
      for (let i = 0; i < stickTargets.length; i++) {
        const st = stickTargets[i];
        const sx = st.id === 'l3' ? input.lx : input.rx;
        const sy = st.id === 'l3' ? input.ly : input.ry;
        const mag = Math.min(1, Math.hypot(sx, sy));
        const clickedStick = Boolean(input.buttons[st.id] || clicked[st.id]);
        if (clickedStick || mag > 0.18) anyPressedBoost = true;

        if (clickedStick && !st.wasPressed) st.pulseTimer = 1.0;
        st.wasPressed = clickedStick;

        const targetRotX = sy * 0.42;
        const targetRotZ = -sx * 0.42;
        st.pivot.rotation.x = THREE.MathUtils.damp(st.pivot.rotation.x, targetRotX, 24, delta);
        st.pivot.rotation.z = THREE.MathUtils.damp(st.pivot.rotation.z, targetRotZ, 24, delta);
        st.pivot.position.y = THREE.MathUtils.damp(
          st.pivot.position.y,
          clickedStick ? st.restY - 0.04 : st.restY,
          24,
          delta,
        );

        st.haloMat.color.copy(clickedStick ? tempHotColor : tempRingColor);
        st.haloMat.opacity = clickedStick ? 0.92 : 0.22 + mag * 0.68;
        st.capMat.emissive.copy(clickedStick ? tempHotColor : tempColdColor);
        st.capMat.emissiveIntensity = clickedStick ? 1.85 : 0.08 + mag * 0.95;

        if (st.pulseTimer > 0) {
          st.pulseTimer = Math.max(0, st.pulseTimer - delta * 2.5);
          const s = 1 + (1 - st.pulseTimer) * 1.5;
          st.clickRing.visible = true;
          st.clickRing.scale.set(s, s, s);
          st.clickRingMat.opacity = st.pulseTimer * 0.9;
        } else {
          st.clickRing.visible = false;
        }
      }

      if (anyPressedBoost) {
        accentPoint.intensity += 0.9;
      }

      renderer.render(scene, camera);

      if (isSoftwareGl) {
        timeoutId = window.setTimeout(() => {
          rafId = requestAnimationFrame(tick);
        }, 33);
      } else {
        rafId = requestAnimationFrame(tick);
      }
    };

    if (isSoftwareGl) {
      timeoutId = window.setTimeout(() => {
        rafId = requestAnimationFrame(tick);
      }, 0);
    } else {
      rafId = requestAnimationFrame(tick);
    }

    return () => {
      disposed = true;
      cancelAnimationFrame(rafId);
      clearTimeout(timeoutId);
      resizeObserver.disconnect();
      controls.removeEventListener('start', cancelCameraPreset);
      controls.dispose();
      renderer.domElement.removeEventListener('pointerdown', onPointerDown);
      renderer.domElement.removeEventListener('pointerup', onPointerUp);

      scene.traverse((obj) => {
        if ((obj as THREE.Mesh).isMesh) {
          const m = obj as THREE.Mesh;
          m.geometry.dispose();
          if (Array.isArray(m.material)) {
            m.material.forEach((mat) => {
              if ('map' in mat && mat.map) (mat.map as THREE.Texture).dispose();
              mat.dispose();
            });
          } else if (m.material) {
            const mat = m.material as THREE.MeshStandardMaterial;
            if (mat.map) mat.map.dispose();
            mat.dispose();
          }
        }
      });

      envTarget.dispose();
      stageTex.dispose();
      renderer.dispose();
      if (renderer.domElement.parentElement === container) {
        container.removeChild(renderer.domElement);
      }
    };
  }, [mode]);

  // Compute live active inputs list for compact status readout
  const activeButtonsList: string[] = [];
  for (const [key, label] of Object.entries(BUTTON_DISPLAY_NAMES)) {
    if (key === 'l2' && (liveInput.l2 > 0.1 || activeClickedIds.l2)) {
      activeButtonsList.push(`L2 ${Math.round((activeClickedIds.l2 ? 1 : liveInput.l2) * 100)}%`);
    } else if (key === 'r2' && (liveInput.r2 > 0.1 || activeClickedIds.r2)) {
      activeButtonsList.push(`R2 ${Math.round((activeClickedIds.r2 ? 1 : liveInput.r2) * 100)}%`);
    } else if (liveInput.buttons[key] || activeClickedIds[key]) {
      activeButtonsList.push(label);
    }
  }

  return (
    <div className="relative rounded-2xl overflow-hidden border border-sky-500/25 bg-[#040914] shadow-[0_0_40px_rgba(0,112,209,0.2)]">
      {/* Compact Header Strip Above 3D Viewport */}
      <div className="flex flex-wrap items-center justify-between gap-2 px-4 py-3 bg-[#071022] border-b border-white/10">
        <div className="flex items-center gap-2.5">
          <div className="p-1.5 rounded-lg bg-sky-500/15 border border-sky-400/30 text-sky-400">
            <Gamepad2 className="w-4 h-4" />
          </div>
          <div>
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <span>Visor 3D Interactivo en Vivo</span>
              <span className="px-2 py-0.5 rounded-md text-[10px] font-bold uppercase tracking-wider bg-sky-500/20 text-sky-300 border border-sky-400/30">
                {mode === 'dualsense' ? 'PS5 DualSense' : 'PS4 DualShock 4'}
              </span>
            </h3>
            <p className="text-[11px] text-slate-400">
              Arrastra para girar 360°, haz scroll para zoom o pulsa botones en tu mando (o haz clic directo en el modelo 3D).
            </p>
          </div>
        </div>

        {/* Live Pressed Readout */}
        <div className="flex items-center gap-1.5 flex-wrap">
          {activeButtonsList.length > 0 ? (
            activeButtonsList.slice(0, 6).map((label) => (
              <span
                key={label}
                className="px-2 py-1 rounded-md text-[11px] font-bold bg-sky-500 text-slate-950 shadow-[0_0_12px_rgba(56,189,248,0.65)]"
              >
                {label}
              </span>
            ))
          ) : (
            <span className="text-[11px] text-slate-400 font-mono flex items-center gap-1.5 px-2 py-1 rounded-md bg-white/[0.03] border border-white/5">
              <Sparkles className="w-3.5 h-3.5 text-sky-400" />
              Esperando pulsación o clic...
            </span>
          )}
        </div>
      </div>

      {/* 3D WebGL Canvas Mount (Kept completely separate from React HUD nodes) */}
      <div className="relative w-full h-[380px] sm:h-[430px] lg:h-[460px]">
        <div ref={mountRef} className="w-full h-full" />

        {webglError && (
          <div className="absolute inset-0 flex items-center justify-center p-6 bg-[#050b16] text-center">
            <p className="text-sm text-amber-300">{webglError}</p>
          </div>
        )}

        {/* Solid-tinted Bottom Control Dock (no backdrop-filter over WebGL) */}
        <div className="absolute bottom-3 left-1/2 -translate-x-1/2 z-10 flex flex-col items-center gap-1.5">
          {hudExpanded && (
            <div className="flex flex-wrap items-center justify-center gap-1.5 px-3 py-2 rounded-xl bg-[#071022]/95 border border-white/15 shadow-xl">
              {/* PS5 / PS4 Model Switcher */}
              <div className="flex items-center bg-black/50 p-0.5 rounded-lg border border-white/10">
                <button
                  type="button"
                  onClick={() => onModeChange?.('dualsense')}
                  className={`px-2.5 py-1 rounded-md text-[11px] font-bold transition-colors cursor-pointer ${
                    mode === 'dualsense'
                      ? 'bg-sky-500 text-slate-950'
                      : 'text-slate-300 hover:text-white'
                  }`}
                >
                  PS5 DualSense
                </button>
                <button
                  type="button"
                  onClick={() => onModeChange?.('ds4')}
                  className={`px-2.5 py-1 rounded-md text-[11px] font-bold transition-colors cursor-pointer ${
                    mode === 'ds4'
                      ? 'bg-indigo-500 text-white'
                      : 'text-slate-300 hover:text-white'
                  }`}
                >
                  PS4 DualShock 4
                </button>
              </div>

              <div className="h-4 w-[1px] bg-white/10 mx-0.5 hidden sm:block" />

              {/* Camera Angle Presets */}
              <div className="flex items-center gap-1">
                <Eye className="w-3.5 h-3.5 text-sky-400 mr-0.5 hidden sm:inline" />
                {(['studio', 'front', 'triggers'] as CameraPreset[]).map((pKey) => (
                  <button
                    key={pKey}
                    type="button"
                    onClick={() => handleSelectPreset(pKey)}
                    className={`px-2.5 py-1 rounded-lg text-[11px] font-semibold border transition-colors cursor-pointer ${
                      preset === pKey
                        ? 'bg-sky-500/20 text-sky-300 border-sky-400/50'
                        : 'bg-white/[0.04] text-slate-300 border-white/10 hover:bg-white/10'
                    }`}
                  >
                    {CAMERA_PRESETS[pKey].label}
                  </button>
                ))}
              </div>

              <div className="h-4 w-[1px] bg-white/10 mx-0.5 hidden sm:block" />

              {/* Auto-Rotate Toggle */}
              <button
                type="button"
                onClick={() => setAutoRotate((v) => !v)}
                className={`flex items-center gap-1.5 px-2.5 py-1 rounded-lg text-[11px] font-semibold border transition-colors cursor-pointer ${
                  autoRotate
                    ? 'bg-emerald-500/20 text-emerald-300 border-emerald-400/50'
                    : 'bg-white/[0.04] text-slate-300 border-white/10 hover:bg-white/10'
                }`}
              >
                <RotateCw className="w-3 h-3" />
                <span>Auto-Giro</span>
              </button>
            </div>
          )}

          {/* Collapse/Expand Handle */}
          <button
            type="button"
            onClick={() => setHudExpanded((v) => !v)}
            className="flex items-center gap-1 px-2.5 py-0.5 rounded-md bg-[#071022]/95 border border-white/15 text-[10px] font-semibold text-slate-300 hover:text-white cursor-pointer"
          >
            {hudExpanded ? (
              <>
                <ChevronDown className="w-3 h-3" />
                <span>Ocultar Controles 3D</span>
              </>
            ) : (
              <>
                <ChevronUp className="w-3 h-3" />
                <span>Controles 3D</span>
              </>
            )}
          </button>
        </div>
      </div>
    </div>
  );
}
