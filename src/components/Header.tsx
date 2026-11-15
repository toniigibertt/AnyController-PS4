/**
 * Controller Control Center PS4 - Encabezado y Barra de Módulos (v1.5.0)
 * Cumple contrato limpio de barra superior y acceso directo a los 6 módulos sin ocultar pestañas.
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

const PKG_VERSION = 'v1.5.0';
const VERSIONED_PKG_NAME = `Controller_Control_Center_PS4_${PKG_VERSION}.pkg`;
const VERSIONED_ZIP_NAME = `Controller_Control_Center_PS4_Pack_${PKG_VERSION}.zip`;

interface HeaderProps {
  currentTab: TabId;
  onSelectTab: (tab: TabId) => void;
  activeControllerMode: 'dualsense' | 'ds4';
  onShowToast?: (msg: string) => void;
}

export default function Header({
  currentTab,
  onSelectTab,
  activeControllerMode,
  onShowToast,
}: HeaderProps) {
  const tabs: {
    id: TabId;
    label: string;
    sublabel: string;
    icon: any;
  }[] = [
    {
      id: 'settings',
      label: 'Ajustes',
      sublabel: 'Ejes y Gatillos',
      icon: Sliders,
    },
    {
      id: 'dualsense',
      label: 'DualSense PS5',
      sublabel: activeControllerMode === 'dualsense' ? 'Activo en J actual' : 'Ajustes PS5',
      icon: Sparkles,
    },
    {
      id: 'dualshock4',
      label: 'DualShock 4',
      sublabel: activeControllerMode === 'ds4' ? 'Activo en J actual' : 'Ajustes PS4',
      icon: Gamepad,
    },
    {
      id: 'tester',
      label: 'Laboratorio 2D',
      sublabel: 'Mandos Originales',
      icon: Gamepad2,
    },
    {
      id: 'remap',
      label: 'Mapeo HID',
      sublabel: 'Perfiles USB',
      icon: Shuffle,
    },
    {
      id: 'info',
      label: 'Sistema y Red',
      sublabel: 'GitHub Updater',
      icon: Info,
    },
  ];

  return (
    <header className="relative z-10 w-full bg-[#070B14]/95 border-b border-slate-800/90 backdrop-blur-md">
      {/* Top Studio Bar */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-3 flex flex-col md:flex-row items-start md:items-center justify-between gap-3">
        {/* Clean Brand Wordmark */}
        <div className="flex items-center gap-3.5 min-w-0">
          <div className="flex items-center justify-center w-11 h-11 rounded-lg overflow-hidden bg-[#0E1A32] border border-cyan-400/70 shrink-0">
            <img
              src="/assets/icons/app_icon.png"
              alt="Controller Control Center PS4"
              className="w-full h-full object-cover"
            />
          </div>
          <div className="min-w-0">
            <h1 className="text-base md:text-lg font-extrabold tracking-tight text-white truncate">
              Controller Control Center{' '}
              <span className="text-cyan-400">PS4</span>
            </h1>
            <div className="flex flex-wrap items-center gap-x-2 text-xs text-slate-400 font-mono tabular-nums">
              <a
                href="https://github.com/toniigibertt/AnyController-PS4/releases"
                target="_blank"
                rel="noreferrer"
                className="text-cyan-300 hover:underline font-semibold"
              >
                {PKG_VERSION} (01.50)
              </a>
              <span>·</span>
              <span>ANYCONTROLLER</span>
              <span>·</span>
              <span>ID: TONI88888</span>
            </div>
          </div>
        </div>

        {/* Clean Single-Line Download Actions */}
        <div className="flex items-center gap-2.5 shrink-0">
          <button
            type="button"
            onClick={async () => {
              onShowToast?.(`Descargando ${VERSIONED_PKG_NAME}...`);
              await downloadBinaryFile(`/${VERSIONED_PKG_NAME}`, VERSIONED_PKG_NAME);
              onShowToast?.(`¡${VERSIONED_PKG_NAME} descargado!`);
            }}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-cyan-400 hover:bg-cyan-300 text-slate-950 font-bold text-xs transition-colors cursor-pointer whitespace-nowrap shrink-0"
            title={`Descargar ${VERSIONED_PKG_NAME} (Title ID: TONI88888)`}
          >
            <Download className="w-4 h-4" />
            <span>Descargar .PKG {PKG_VERSION}</span>
          </button>

          <button
            type="button"
            onClick={async () => {
              onShowToast?.(`Descargando ${VERSIONED_ZIP_NAME}...`);
              await downloadBinaryFile(`/${VERSIONED_ZIP_NAME}`, VERSIONED_ZIP_NAME);
              onShowToast?.(`¡${VERSIONED_ZIP_NAME} descargado!`);
            }}
            className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg bg-[#0E172A] hover:bg-slate-800 text-slate-200 font-semibold text-xs border border-slate-700 transition-colors cursor-pointer whitespace-nowrap shrink-0"
            title={`Descargar ZIP con ${VERSIONED_PKG_NAME}, anycontroller.bin y anycontroller.ini`}
          >
            <Download className="w-3.5 h-3.5 text-cyan-400" />
            <span>Pack {PKG_VERSION} (.ZIP)</span>
          </button>
        </div>
      </div>

      {/* Studio Module Navigation Dock */}
      <div className="border-t border-slate-800/80 bg-[#090E1A]">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 flex items-center justify-between overflow-x-auto no-scrollbar">
          <div className="hidden xl:flex items-center gap-1.5 text-[11px] font-mono text-slate-400 py-2 pr-3 border-r border-slate-800 shrink-0">
            <kbd className="px-1.5 py-0.5 bg-slate-900 border border-slate-700 rounded text-cyan-300">
              Q
            </kbd>
            <span>ANTERIOR</span>
          </div>

          <nav className="flex items-center gap-1 py-1.5 flex-1 justify-start lg:justify-center">
            {tabs.map((tab) => {
              const Icon = tab.icon;
              const isActive = currentTab === tab.id;
              return (
                <button
                  key={tab.id}
                  type="button"
                  onClick={() => onSelectTab(tab.id)}
                  className={`group flex items-center gap-2.5 px-3 py-2 rounded-lg text-left transition-colors cursor-pointer whitespace-nowrap shrink-0 ${
                    isActive
                      ? 'bg-[#10203D] text-white border border-cyan-400/70'
                      : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900/70 border border-transparent'
                  }`}
                >
                  <Icon
                    className={`w-4 h-4 shrink-0 ${
                      isActive ? 'text-cyan-400' : 'text-slate-400 group-hover:text-slate-200'
                    }`}
                  />
                  <div>
                    <div className="text-xs font-bold leading-tight">{tab.label}</div>
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
          </nav>

          <div className="hidden xl:flex items-center gap-1.5 text-[11px] font-mono text-slate-400 py-2 pl-3 border-l border-slate-800 shrink-0">
            <span>SIGUIENTE</span>
            <kbd className="px-1.5 py-0.5 bg-slate-900 border border-slate-700 rounded text-cyan-300">
              E
            </kbd>
          </div>
        </div>
      </div>
    </header>
  );
}
