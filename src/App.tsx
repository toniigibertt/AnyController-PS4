/**
 * Controller Control Center PS4 - Centro de Control Multi-Mando y Memoria Independiente (v1.5.0)
 */
import { useState, useEffect, useMemo } from 'react';
import {
  AppConfig,
  ControllerTargetMeta,
  ProfileStore,
  TabId,
} from './types';
import { DEFAULT_CONFIG } from './constants/options';
import { generateOneFamilyIni, downloadFile } from './utils/iniGenerator';
import { useGamepad } from './hooks/useGamepad';
import PS4Background from './components/PS4Background';
import Header from './components/Header';
import FooterControls from './components/FooterControls';
import Toast from './components/Toast';
import SettingsTab from './components/tabs/SettingsTab';
import TesterTab from './components/tabs/TesterTab';
import DualSenseTab from './components/tabs/DualSenseTab';
import DualShock4Tab from './components/tabs/DualShock4Tab';
import RemapTab from './components/tabs/RemapTab';
import InfoTab from './components/tabs/InfoTab';
import { Users, RotateCcw, Download, HardDrive } from 'lucide-react';

const PROFILE_STORE_KEY = 'anycontroller_ps4_profiles_v150';
const LEGACY_CONFIG_KEY = 'anycontroller_ps4_config';

function loadInitialStore(): ProfileStore {
  try {
    const raw = localStorage.getItem(PROFILE_STORE_KEY);
    if (raw) {
      const parsed = JSON.parse(raw);
      return {
        global: { ...DEFAULT_CONFIG, ...(parsed.global || {}) },
        targets: parsed.targets || {},
        macBySlot: parsed.macBySlot || {},
      };
    }
    const legacyRaw =
      localStorage.getItem(LEGACY_CONFIG_KEY) ||
      localStorage.getItem('onefamily_ps4_config');
    if (legacyRaw) {
      return {
        global: { ...DEFAULT_CONFIG, ...JSON.parse(legacyRaw) },
        targets: {},
        macBySlot: {},
      };
    }
  } catch {
    // Fallback
  }
  return { global: DEFAULT_CONFIG, targets: {}, macBySlot: {} };
}

export default function App() {
  const [currentTab, setCurrentTab] = useState<TabId>('settings');
  const [selectedCard, setSelectedCard] = useState<number>(0);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [activeTargetId, setActiveTargetId] = useState<string>('global');
  const [manualViewerMode, setManualViewerMode] = useState<'dualsense' | 'ds4'>('dualsense');
  const [testerIsolated, setTesterIsolated] = useState<boolean>(false);

  const [store, setStore] = useState<ProfileStore>(loadInitialStore);

  const {
    activeSlot,
    setActiveSlot,
    connectedGamepads,
    hardwareDetected,
    gamepadName,
    deviceVidPid,
    liveInput,
    triggerRumble,
  } = useGamepad();

  // Persist entire profile store
  useEffect(() => {
    try {
      localStorage.setItem(PROFILE_STORE_KEY, JSON.stringify(store));
      localStorage.setItem(LEGACY_CONFIG_KEY, JSON.stringify(store.global));
    } catch {
      // Ignore storage quota errors
    }
  }, [store]);

  // Build available target list (Global + J1..J4)
  const targetList: ControllerTargetMeta[] = useMemo(() => {
    const list: ControllerTargetMeta[] = [
      {
        id: 'global',
        label: 'Global (Por Defecto)',
        shortLabel: 'Global',
        family: 'all',
        ps4Filename: 'anycontroller.ini',
        ps4FullPath: '/data/anycontroller/anycontroller.ini',
        hasCustomConfig: false,
      },
    ];

    for (let slot = 0; slot < 4; slot++) {
      const gp = connectedGamepads.find((g) => g.slot === slot);
      const id = `slot_${slot}`;
      const family = gp ? gp.family : slot === 0 ? 'dualsense' : 'ds4';
      const customMac = (store.macBySlot?.[slot] || '')
        .replace(/[^0-9a-fA-F]/g, '')
        .toUpperCase();
      const ps4Filename =
        family === 'dualsense' && customMac.length === 12
          ? `ds_mac_${customMac}.ini`
          : family === 'dualsense'
          ? `ds_mac_MANDO_J${slot + 1}.ini`
          : `ds4_slot_${slot + 1}.ini`;

      const shortHw = gp
        ? gp.family === 'dualsense'
          ? 'DualSense PS5'
          : gp.family === 'ds4'
          ? 'DualShock 4'
          : 'USB HID'
        : 'Disponible';

      list.push({
        id,
        slotIndex: slot,
        label: `Jugador ${slot + 1} · ${shortHw}`,
        shortLabel: gp ? `J${slot + 1} · ${shortHw}` : `J${slot + 1}`,
        family,
        ps4Filename,
        ps4FullPath: `/data/anycontroller/controllers/${ps4Filename}`,
        hasCustomConfig: Boolean(store.targets[id]),
      });
    }

    return list;
  }, [connectedGamepads, store.targets, store.macBySlot]);

  const activeTarget =
    targetList.find((t) => t.id === activeTargetId) || targetList[0];

  // Effective config for currently selected target: own profile if saved, else dynamically inherits Global
  const activeConfig: AppConfig = useMemo(() => {
    if (activeTarget.id === 'global') return store.global;
    return store.targets[activeTarget.id]
      ? { ...DEFAULT_CONFIG, ...store.targets[activeTarget.id] }
      : store.global;
  }, [activeTarget.id, store]);

  // Determine visual hardware family for current target
  const activeControllerMode: 'dualsense' | 'ds4' = useMemo(() => {
    if (activeTarget.family === 'dualsense') return 'dualsense';
    if (activeTarget.family === 'ds4') return 'ds4';
    return manualViewerMode;
  }, [activeTarget.family, manualViewerMode]);

  // Select target manually (also switches activeSlot in useGamepad if J1..J4)
  const handleSelectTarget = (targetId: string) => {
    setActiveTargetId(targetId);
    const meta = targetList.find((t) => t.id === targetId);
    if (meta && meta.slotIndex !== undefined) {
      setActiveSlot(meta.slotIndex);
      if (meta.family === 'dualsense') setManualViewerMode('dualsense');
      else if (meta.family === 'ds4') setManualViewerMode('ds4');
    }
  };

  // Update active target config (editing a specific controller saves its own independent memory)
  const handleUpdateConfig = (newConfig: AppConfig) => {
    setStore((prev) => {
      if (activeTarget.id === 'global') {
        return { ...prev, global: newConfig };
      }
      return {
        ...prev,
        targets: {
          ...prev.targets,
          [activeTarget.id]: newConfig,
        },
      };
    });
  };

  const handleRestoreTargetToGlobal = () => {
    if (activeTarget.id === 'global') return;
    setStore((prev) => {
      const nextTargets = { ...prev.targets };
      delete nextTargets[activeTarget.id];
      return { ...prev, targets: nextTargets };
    });
    setToastMessage(
      `${activeTarget.shortLabel} restaurado para heredar la configuración Global.`
    );
  };

  const handleSave = () => {
    handleUpdateConfig(activeConfig);
    setToastMessage(
      `Perfil guardado en memoria para ${activeTarget.shortLabel} (${activeTarget.ps4FullPath}).`
    );
  };

  const handleDownloadIni = () => {
    const iniContent = generateOneFamilyIni(activeConfig, {
      targetPath: activeTarget.ps4FullPath,
      targetLabel: activeTarget.label,
    });
    downloadFile(activeTarget.ps4Filename, iniContent);
    setToastMessage(
      `Descargado ${activeTarget.ps4Filename} para copiar en ${activeTarget.ps4FullPath}`
    );
  };

  const tabsList: TabId[] = [
    'settings',
    'dualsense',
    'dualshock4',
    'tester',
    'remap',
    'info',
  ];

  // Global keyboard shortcuts (disabled while user toggles Modo Prueba Libre in TesterTab)
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (
        testerIsolated ||
        ['INPUT', 'TEXTAREA', 'SELECT'].includes((e.target as HTMLElement)?.tagName)
      ) {
        return;
      }

      if (e.code === 'KeyQ') {
        const curIdx = tabsList.indexOf(currentTab);
        const prevIdx = (curIdx - 1 + tabsList.length) % tabsList.length;
        setCurrentTab(tabsList[prevIdx]);
        setSelectedCard(0);
      } else if (e.code === 'KeyE') {
        const curIdx = tabsList.indexOf(currentTab);
        const nextIdx = (curIdx + 1) % tabsList.length;
        setCurrentTab(tabsList[nextIdx]);
        setSelectedCard(0);
      } else if (e.code === 'Digit1') {
        setCurrentTab('settings');
      } else if (e.code === 'Digit2') {
        setCurrentTab('dualsense');
      } else if (e.code === 'Digit3') {
        setCurrentTab('dualshock4');
      } else if (e.code === 'Digit4') {
        setCurrentTab('tester');
      } else if (e.code === 'Digit5') {
        setCurrentTab('remap');
      } else if (e.code === 'Digit6') {
        setCurrentTab('info');
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [currentTab, testerIsolated]);

  return (
    <div className="relative min-h-screen text-slate-100 font-sans selection:bg-cyan-400 selection:text-slate-950 overflow-x-hidden pb-16">
      <PS4Background />

      <div className="relative z-10 flex flex-col min-h-screen">
        <Header
          currentTab={currentTab}
          onSelectTab={(tab) => {
            setCurrentTab(tab);
            setSelectedCard(0);
          }}
          activeControllerMode={activeControllerMode}
          onShowToast={setToastMessage}
        />

        {/* Workspace Multi-Controller Target & Persistent Memory Bar */}
        <div className="border-b border-slate-800/80 bg-[#050A14]/90 backdrop-blur-sm">
          <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-2.5 flex flex-col lg:flex-row lg:items-center justify-between gap-2.5">
            <div className="flex flex-wrap items-center gap-1.5">
              <span className="flex items-center gap-1.5 text-xs font-semibold text-slate-400 mr-1.5">
                <Users className="w-3.5 h-3.5 text-cyan-400" />
                <span>Editando mando:</span>
              </span>
              {targetList.map((t) => {
                const isActive = t.id === activeTarget.id;
                const isConnectedSlot =
                  t.slotIndex !== undefined &&
                  connectedGamepads.some((g) => g.slot === t.slotIndex);
                return (
                  <button
                    key={t.id}
                    type="button"
                    onClick={() => handleSelectTarget(t.id)}
                    className={`flex items-center gap-1.5 px-2.5 py-1 rounded-md text-xs font-semibold border transition-colors cursor-pointer whitespace-nowrap ${
                      isActive
                        ? 'bg-cyan-500/20 text-cyan-200 border-cyan-400/70'
                        : 'bg-[#0B1120] text-slate-400 hover:text-slate-200 border-slate-800 hover:border-slate-700'
                    }`}
                  >
                    <span>{t.shortLabel}</span>
                    {isConnectedSlot && (
                      <span
                        className="w-1.5 h-1.5 rounded-full bg-emerald-400"
                        title="Conectado en este navegador"
                      />
                    )}
                    {t.hasCustomConfig && (
                      <span className="text-[10px] font-mono text-amber-300">
                        · Propio
                      </span>
                    )}
                  </button>
                );
              })}
            </div>

            {/* Active Target Memory Status & Quick Actions */}
            <div className="flex flex-wrap items-center gap-2 text-xs">
              <div className="flex items-center gap-1.5 text-slate-400 font-mono text-[11px]">
                <HardDrive className="w-3.5 h-3.5 text-cyan-400 shrink-0" />
                <span>
                  {activeTarget.id === 'global'
                    ? 'Base para mandos nuevos'
                    : activeTarget.hasCustomConfig
                    ? 'Memoria propia independiente activa'
                    : 'Hereda de Global (toca cualquier ajuste para separarlo)'}
                </span>
              </div>

              {activeTarget.id !== 'global' && activeTarget.hasCustomConfig && (
                <button
                  type="button"
                  onClick={handleRestoreTargetToGlobal}
                  className="flex items-center gap-1 px-2.5 py-1 rounded-md bg-slate-900 hover:bg-slate-800 text-amber-300 border border-amber-500/40 font-semibold transition-colors cursor-pointer"
                >
                  <RotateCcw className="w-3 h-3" />
                  <span>Restaurar a Global</span>
                </button>
              )}

              <button
                type="button"
                onClick={handleDownloadIni}
                className="flex items-center gap-1 px-2.5 py-1 rounded-md bg-slate-900 hover:bg-slate-800 text-cyan-300 border border-slate-700 font-mono text-[11px] transition-colors cursor-pointer"
                title={`Exportar ${activeTarget.ps4FullPath}`}
              >
                <Download className="w-3 h-3" />
                <span>{activeTarget.ps4Filename}</span>
              </button>
            </div>
          </div>
        </div>

        {/* Tab Content Container */}
        <main className="flex-1 max-w-7xl w-full mx-auto p-4 sm:p-6 md:p-8">
          {currentTab === 'settings' && (
            <SettingsTab
              config={activeConfig}
              onChangeConfig={handleUpdateConfig}
              onSave={handleSave}
              onDownloadIni={handleDownloadIni}
              selectedCard={selectedCard}
              onSelectCard={setSelectedCard}
              activeTarget={activeTarget}
            />
          )}

          {currentTab === 'dualsense' && (
            <DualSenseTab
              config={activeConfig}
              onChangeConfig={handleUpdateConfig}
              selectedCard={selectedCard}
              onSelectCard={setSelectedCard}
              activeTarget={activeTarget}
            />
          )}

          {currentTab === 'dualshock4' && (
            <DualShock4Tab
              config={activeConfig}
              onChangeConfig={handleUpdateConfig}
              selectedCard={selectedCard}
              onSelectCard={setSelectedCard}
              onTriggerRumble={(phase) =>
                triggerRumble(phase, activeTarget.slotIndex ?? activeSlot)
              }
              onShowToast={setToastMessage}
              activeTarget={activeTarget}
            />
          )}

          {currentTab === 'tester' && (
            <TesterTab
              config={activeConfig}
              onChangeConfig={handleUpdateConfig}
              liveInput={liveInput}
              activeSlot={activeSlot}
              connectedGamepads={connectedGamepads}
              hardwareDetected={hardwareDetected}
              onSelectSlot={(slot) => {
                setActiveSlot(slot);
                setActiveTargetId(`slot_${slot}`);
              }}
              onTriggerRumble={(phase) => triggerRumble(phase, activeSlot)}
              activeControllerMode={activeControllerMode}
              onSelectControllerMode={(next) => setManualViewerMode(next)}
              testMode={testerIsolated}
              onToggleTestMode={setTesterIsolated}
            />
          )}

          {currentTab === 'remap' && (
            <RemapTab
              liveInput={liveInput}
              activeVidPid={deviceVidPid}
              onShowToast={setToastMessage}
            />
          )}

          {currentTab === 'info' && (
            <InfoTab
              config={activeConfig}
              onChangeConfig={handleUpdateConfig}
              onShowToast={setToastMessage}
              activeTarget={activeTarget}
            />
          )}
        </main>

        <FooterControls />

        {toastMessage && (
          <Toast message={toastMessage} onClose={() => setToastMessage(null)} />
        )}
      </div>
    </div>
  );
}
