/**
 * AnyController Studio PS4 - Encabezado y Navegación de Módulos
 * Diseño original oscuro de estudio (Obsidian & Cyber Cyan) con pestañas dinámicas según el mando activo
 */
import { TabId } from '../types';
import {
  Sliders,
  Gamepad2,
  Sparkles,
  Shuffle,
  Info,
  Download,
  Gamepad,
} from 'lucide-react';
import { downloadBinaryFile } from '../utils/iniGenerator';

const PKG_VERSION = 'v1.3.9';
const VERSIONED_PKG_NAME = `Controller_Control_Center_PS4_${PKG_VERSION}.pkg`;
const VERSIONED_ZIP_NAME = `Controller_Control_Center_PS4_Pack_${PKG_VERSION}.zip`;

interface HeaderProps {
  currentTab: TabId;
  onSelectTab: (tab: TabId) => void;
  controllerName: string;
  hardwareDetected: boolean;
  activeControllerMode: 'dualsense' | 'ds4';
  onToggleControllerMode: (mode: 'dualsense' | 'ds4') => void;
  onShowToast?: (msg: string) => void;
}

export default function Header({
  currentTab,
  onSelectTab,
  activeControllerMode,
  onToggleControllerMode,
  onShowToast,
}: HeaderProps) {
  const tabs: {
    id: TabId;
    label: string;
    sublabel: string;
    icon: any;
    shortcut: string;
  }[] = [
    {
      id: 'settings',
      label: 'Ajustes Globales',
      sublabel: 'Ejes y Gatillos',
      icon: Sliders,
      shortcut: '1',
    },
    activeControllerMode === 'dualsense'
      ? {
          id: 'dualsense',
          label: 'DualSense PS5',
          sublabel: 'Mando PS5 Activo',
          icon: Sparkles,
          shortcut: '2',
        }
      : {
          id: 'dualshock4',
          label: 'DualShock 4',
          sublabel: 'Mando PS4 Activo',
          icon: Gamepad,
          shortcut: '2',
        },
    {
      id: 'tester',
      label: 'Laboratorio',
      sublabel: 'Probador en Vivo',
      icon: Gamepad2,
      shortcut: '3',
    },
    {
      id: 'remap',
      label: 'Mapeo USB HID',
      sublabel: 'Perfiles a Medida',
      icon: Shuffle,
      shortcut: '4',
    },
    {
      id: 'info',
      label: 'Sistema y Red',
      sublabel: 'GitHub Updater',
      icon: Info,
      shortcut: '5',
    },
  ];

  return (
    <header className="relative z-10 w-full bg-[#070B14]/95 border-b border-slate-800/90 backdrop-blur-md">
      {/* Top Studio Deck Bar */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-3.5 flex flex-col lg:flex-row items-center justify-between gap-4">
        {/* Brand Identity */}
        <div className="flex items-center gap-3.5">
          <div className="relative flex items-center justify-center w-12 h-12 rounded-xl overflow-hidden bg-[#0E1A32] border-2 border-cyan-400 shadow-[0_0_18px_rgba(0,175,255,0.35)] shrink-0">
            <img
              src="/assets/icons/app_icon.png"
              alt="Controller Control Center PS4"
              className="w-full h-full object-cover"
            />
          </div>
          <div>
            <div className="flex flex-wrap items-center gap-2">
              <h1 className="text-lg md:text-xl font-black tracking-wider text-white uppercase">
                CONTROLLER CONTROL CENTER{' '}
                <span className="text-cyan-400 font-semibold">PS4</span>
              </h1>
              <span className="px-2 py-0.5 text-[11px] font-mono font-bold rounded bg-indigo-950/90 text-indigo-300 border border-indigo-500/40">
                PAYLOAD: ANYCONTROLLER
              </span>
              <span className="px-2 py-0.5 text-[11px] font-mono font-bold rounded bg-slate-900 text-slate-300 border border-slate-700">
                ID: TONI88888
              </span>
              <a
                href="https://github.com/toniigibertt/AnyController-PS4/releases"
                target="_blank"
                rel="noreferrer"
                className="px-2 py-0.5 text-[11px] font-mono font-semibold rounded bg-cyan-950/90 hover:bg-cyan-900 text-cyan-300 border border-cyan-500/40 transition-colors"
                title="Repositorio Oficial en GitHub de toniigibertt (Auto-Updater v1.3.9)"
              >
                v1.3.9 (01.39)
              </a>
            </div>
            <p className="text-xs text-slate-400">
              Motor ANYCONTROLLER • Multijugador DualSense BT (1 a 4 Mandos) • 100% Independiente • Control DS4 y Probador en Vivo
            </p>
          </div>
        </div>

        {/* Downloads & Active Hardware Selector */}
        <div className="flex flex-wrap items-center gap-2.5">
          <button
            onClick={async () => {
              onShowToast?.(`Descargando ${VERSIONED_PKG_NAME} (ID: TONI88888)...`);
              await downloadBinaryFile(
                `/${VERSIONED_PKG_NAME}`,
                VERSIONED_PKG_NAME
              );
              onShowToast?.(`¡${VERSIONED_PKG_NAME} descargado!`);
            }}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-cyan-400 hover:bg-cyan-300 text-slate-950 font-bold text-xs shadow-[0_0_20px_rgba(34,211,238,0.35)] transition-all cursor-pointer"
            title={`Descargar ${VERSIONED_PKG_NAME} (Title ID: TONI88888)`}
          >
            <Download className="w-4 h-4" />
            <span>Descargar .PKG {PKG_VERSION}</span>
          </button>

          <button
            onClick={async () => {
              onShowToast?.(`Descargando ${VERSIONED_ZIP_NAME}...`);
              await downloadBinaryFile(
                `/${VERSIONED_ZIP_NAME}`,
                VERSIONED_ZIP_NAME
              );
              onShowToast?.(`¡${VERSIONED_ZIP_NAME} descargado con éxito!`);
            }}
            className="flex items-center gap-1.5 px-3 py-2 rounded-lg bg-[#0E172A] hover:bg-slate-800 text-cyan-300 font-semibold text-xs border border-slate-700 transition-colors cursor-pointer"
            title={`Descargar ZIP con ${VERSIONED_PKG_NAME}, payload anycontroller.bin y anycontroller.ini`}
          >
            <Download className="w-3.5 h-3.5" />
            <span>Pack {PKG_VERSION} (.ZIP)</span>
          </button>

          {/* Active Controller Context Switcher */}
          <div className="flex items-center bg-[#0B1120] p-1 rounded-lg border border-slate-800 text-xs">
            <button
              onClick={() => onToggleControllerMode('dualsense')}
              className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md font-semibold transition-all cursor-pointer ${
                activeControllerMode === 'dualsense'
                  ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-400/50'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
              title="Simular Mando PS5 DualSense Conectado (Oculta el apartado PS4 y apaga el mando PS4)"
            >
              <Sparkles className="w-3.5 h-3.5" />
              <span>Mando PS5</span>
            </button>
            <button
              onClick={() => onToggleControllerMode('ds4')}
              className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-md font-semibold transition-all cursor-pointer ${
                activeControllerMode === 'ds4'
                  ? 'bg-indigo-500/20 text-indigo-300 border border-indigo-400/50'
                  : 'text-slate-400 hover:text-slate-200'
              }`}
              title="Simular Mando Nativo PS4 DualShock 4 Activo (Oculta el apartado PS5)"
            >
              <Gamepad className="w-3.5 h-3.5" />
              <span>Mando PS4</span>
            </button>
          </div>
        </div>
      </div>

      {/* Distinct Studio Module Dock Bar */}
      <div className="border-t border-slate-800/80 bg-[#090E1A]">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 flex items-center justify-between overflow-x-auto no-scrollbar">
          <div className="hidden xl:flex items-center gap-1.5 text-[11px] font-mono text-slate-400 py-2 pr-3 border-r border-slate-800">
            <kbd className="px-1.5 py-0.5 bg-slate-900 border border-slate-700 rounded text-cyan-300">
              L1 / Q
            </kbd>
            <span>MÓDULOS</span>
          </div>

          <div className="flex items-center gap-1 py-1.5 flex-1 justify-start lg:justify-center">
            {tabs.map((tab) => {
              const Icon = tab.icon;
              const isActive = currentTab === tab.id;
              return (
                <button
                  key={tab.id}
                  onClick={() => onSelectTab(tab.id)}
                  className={`group relative flex items-center gap-2.5 px-3.5 py-2 rounded-lg text-left transition-all cursor-pointer whitespace-nowrap ${
                    isActive
                      ? 'bg-[#10203D] text-white border border-cyan-400/60 shadow-[0_0_16px_rgba(0,175,255,0.2)]'
                      : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/70 border border-transparent'
                  }`}
                >
                  {isActive && (
                    <div className="w-1 h-6 bg-cyan-400 rounded-full shrink-0" />
                  )}
                  <Icon
                    className={`w-4 h-4 shrink-0 ${
                      isActive
                        ? 'text-cyan-400'
                        : 'text-slate-400 group-hover:text-slate-200'
                    }`}
                  />
                  <div>
                    <div className="text-xs font-bold leading-tight">
                      {tab.label}
                    </div>
                    <div
                      className={`text-[10px] leading-tight hidden sm:block ${
                        isActive ? 'text-cyan-300' : 'text-slate-500'
                      }`}
                    >
                      {tab.sublabel}
                    </div>
                  </div>
                </button>
              );
            })}
          </div>

          <div className="hidden xl:flex items-center gap-1.5 text-[11px] font-mono text-slate-400 py-2 pl-3 border-l border-slate-800">
            <span>SIGUIENTE</span>
            <kbd className="px-1.5 py-0.5 bg-slate-900 border border-slate-700 rounded text-cyan-300">
              R1 / E
            </kbd>
          </div>
        </div>
      </div>
    </header>
  );
}
