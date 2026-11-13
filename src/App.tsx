/**
 * AnyController Studio PS4 - Centro de Control de Mandos PS5 DualSense, DualShock 4 Nativo y Periféricos HID
 */
import { useState, useEffect } from 'react';
import { AppConfig, TabId } from './types';
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

const LOCAL_STORAGE_KEY = 'anycontroller_ps4_config';

export default function App() {
  const [currentTab, setCurrentTab] = useState<TabId>('settings');
  const [selectedCard, setSelectedCard] = useState<number>(0);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [activeControllerMode, setActiveControllerMode] = useState<'dualsense' | 'ds4'>('dualsense');

  // Load config from localStorage if available, or fall back to defaults
  const [config, setConfig] = useState<AppConfig>(() => {
    try {
      const saved =
        localStorage.getItem(LOCAL_STORAGE_KEY) ||
        localStorage.getItem('onefamily_ps4_config');
      if (saved) {
        return { ...DEFAULT_CONFIG, ...JSON.parse(saved) };
      }
    } catch {
      // Fallback
    }
    return DEFAULT_CONFIG;
  });

  const {
    activeSlot,
    setActiveSlot,
    hardwareDetected,
    gamepadName,
    deviceVidPid,
    liveInput,
    triggerRumble,
  } = useGamepad();

  // Auto-detect controller family when physical gamepad connects in browser
  useEffect(() => {
    if (!hardwareDetected) return;
    const lower = gamepadName.toLowerCase();
    if (lower.includes('dualsense') || lower.includes('0ce6') || lower.includes('0df2')) {
      setActiveControllerMode('dualsense');
    } else if (lower.includes('dualshock') || lower.includes('05c4') || lower.includes('09cc')) {
      setActiveControllerMode('ds4');
    }
  }, [hardwareDetected, gamepadName]);

  // Dynamically switch controller tab if the active controller mode changes
  useEffect(() => {
    if (activeControllerMode === 'dualsense' && currentTab === 'dualshock4') {
      setCurrentTab('dualsense');
      setSelectedCard(0);
    } else if (activeControllerMode === 'ds4' && currentTab === 'dualsense') {
      setCurrentTab('dualshock4');
      setSelectedCard(0);
    }
  }, [activeControllerMode, currentTab]);

  const handleToggleControllerMode = (mode: 'dualsense' | 'ds4') => {
    setActiveControllerMode(mode);
    if (mode === 'dualsense') {
      setToastMessage(
        'Mando PS5 DualSense activo: Mando PS4 desconectado automáticamente del perfil Jugador 1'
      );
      if (currentTab === 'dualshock4') {
        setCurrentTab('dualsense');
        setSelectedCard(0);
      }
    } else {
      setToastMessage(
        'Mando PS4 DualShock 4 activo: Mostrando configuraciones exclusivas de PS4'
      );
      if (currentTab === 'dualsense') {
        setCurrentTab('dualshock4');
        setSelectedCard(0);
      }
    }
  };

  const handleUpdateConfig = (newConfig: AppConfig) => {
    setConfig(newConfig);
    try {
      localStorage.setItem(LOCAL_STORAGE_KEY, JSON.stringify(newConfig));
    } catch {
      // ignore
    }
  };

  const handleSave = () => {
    handleUpdateConfig(config);
    setToastMessage(
      '¡Ajustes guardados y sincronizados en /data/anycontroller/anycontroller.ini!'
    );
  };

  const handleDownloadIni = () => {
    const iniContent = generateOneFamilyIni(config);
    downloadFile('anycontroller.ini', iniContent);
    setToastMessage(
      'Descargando archivo /data/anycontroller/anycontroller.ini...'
    );
  };

  // Tab navigation order matching native PS4 app (context-aware: only active controller tab appears)
  const tabsList: TabId[] = [
    'settings',
    activeControllerMode === 'dualsense' ? 'dualsense' : 'dualshock4',
    'tester',
    'remap',
    'info',
  ];

  // Global Keyboard shortcuts for PS4 Controller feel (Q/E for tabs, 1..5 for direct jump)
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (['INPUT', 'TEXTAREA'].includes((e.target as HTMLElement)?.tagName))
        return;

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
        setCurrentTab(activeControllerMode === 'dualsense' ? 'dualsense' : 'dualshock4');
      } else if (e.code === 'Digit3') {
        setCurrentTab('tester');
      } else if (e.code === 'Digit4') {
        setCurrentTab('remap');
      } else if (e.code === 'Digit5') {
        setCurrentTab('info');
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [currentTab, activeControllerMode]);

  return (
    <div className="relative min-h-screen text-slate-100 font-sans selection:bg-cyan-400 selection:text-slate-950 overflow-x-hidden pb-16">
      {/* Procedural Animated PS4 Background */}
      <PS4Background />

      {/* Main Container */}
      <div className="relative z-10 flex flex-col min-h-screen">
        {/* Navigation & Header */}
        <Header
          currentTab={currentTab}
          onSelectTab={(tab) => {
            setCurrentTab(tab);
            setSelectedCard(0);
          }}
          controllerName={gamepadName}
          hardwareDetected={hardwareDetected}
          activeControllerMode={activeControllerMode}
          onToggleControllerMode={handleToggleControllerMode}
          onShowToast={setToastMessage}
        />

        {/* Tab Content Container */}
        <main className="flex-1 max-w-7xl w-full mx-auto p-4 sm:p-6 md:p-8">
          {currentTab === 'settings' && (
            <SettingsTab
              config={config}
              onChangeConfig={handleUpdateConfig}
              onSave={handleSave}
              onDownloadIni={handleDownloadIni}
              selectedCard={selectedCard}
              onSelectCard={setSelectedCard}
            />
          )}

          {currentTab === 'dualsense' && activeControllerMode === 'dualsense' && (
            <DualSenseTab
              config={config}
              onChangeConfig={handleUpdateConfig}
              selectedCard={selectedCard}
              onSelectCard={setSelectedCard}
            />
          )}

          {currentTab === 'dualshock4' && activeControllerMode === 'ds4' && (
            <DualShock4Tab
              config={config}
              onChangeConfig={handleUpdateConfig}
              selectedCard={selectedCard}
              onSelectCard={setSelectedCard}
              onTriggerRumble={triggerRumble}
              onShowToast={setToastMessage}
            />
          )}

          {currentTab === 'tester' && (
            <TesterTab
              config={config}
              onChangeConfig={handleUpdateConfig}
              liveInput={liveInput}
              activeSlot={activeSlot}
              onSelectSlot={setActiveSlot}
              onTriggerRumble={triggerRumble}
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
              config={config}
              onChangeConfig={handleUpdateConfig}
              onShowToast={setToastMessage}
            />
          )}
        </main>

        {/* PlayStation 4 Footer Button Legend */}
        <FooterControls />

        {/* Notification Toast */}
        {toastMessage && (
          <Toast message={toastMessage} onClose={() => setToastMessage(null)} />
        )}
      </div>
    </div>
  );
}
