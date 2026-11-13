/**
 * OneFamily PS4 - Pestaña de Reasignación de Mandos HID y Perfiles (Remap Tab)
 * Tabla de asignación interactiva, detector en vivo de botones y exportador de .ini
 */
import { useState } from 'react';
import { ControllerProfile, Ds4ActionMeta, LiveGamepadInput } from '../../types';
import { DS4_ACTIONS } from '../../constants/options';
import { CONTROLLER_PROFILES, findProfileByVidPid } from '../../data/profilesData';
import { generateControllerIni, downloadFile } from '../../utils/iniGenerator';
import {
  Shuffle,
  Download,
  Search,
  CheckCircle,
  Plus,
  RotateCcw,
  SlidersHorizontal,
  ChevronLeft,
  ChevronRight,
  Info,
} from 'lucide-react';

interface RemapTabProps {
  liveInput: LiveGamepadInput;
  activeVidPid: { vid: string; pid: string };
  onShowToast: (msg: string) => void;
}

export default function RemapTab({
  liveInput,
  activeVidPid,
  onShowToast,
}: RemapTabProps) {
  // Currently selected profile to edit
  const defaultProfile = findProfileByVidPid(activeVidPid.vid, activeVidPid.pid) || CONTROLLER_PROFILES[0];
  const [selectedProfile, setSelectedProfile] = useState<ControllerProfile>(defaultProfile);
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [selectedRow, setSelectedRow] = useState<number>(0);
  const [customVid, setCustomVid] = useState<string>(selectedProfile.vendorId);
  const [customPid, setCustomPid] = useState<string>(selectedProfile.productId);

  // Filter profiles based on search
  const filteredProfiles = CONTROLLER_PROFILES.filter((p) => {
    const q = searchQuery.toLowerCase();
    return (
      p.name.toLowerCase().includes(q) ||
      p.vendorId.toLowerCase().includes(q) ||
      p.productId.toLowerCase().includes(q) ||
      p.category.toLowerCase().includes(q)
    );
  });

  const handleSelectProfile = (p: ControllerProfile) => {
    setSelectedProfile(p);
    setCustomVid(p.vendorId);
    setCustomPid(p.productId);
  };

  const handleCycleButton = (actionKey: string, dir: number) => {
    const currentBtn = selectedProfile.assignments[actionKey] ?? 0;
    const maxButtons = 16;
    let nextBtn = currentBtn + dir;
    if (nextBtn > maxButtons) nextBtn = 0;
    if (nextBtn < 0) nextBtn = maxButtons;

    setSelectedProfile((prev) => ({
      ...prev,
      assignments: {
        ...prev.assignments,
        [actionKey]: nextBtn,
      },
    }));
  };

  const handleDownloadProfileIni = () => {
    const iniContent = generateControllerIni({
      ...selectedProfile,
      vendorId: customVid,
      productId: customPid,
    });
    const filename = `${customVid.toUpperCase()}_${customPid.toUpperCase()}.ini`;
    downloadFile(filename, iniContent);
    onShowToast(`Perfil descargado: /data/anycontroller/controllers/${filename}`);
  };

  return (
    <div className="space-y-6">
      {/* Top Banner: Profile Selector and Hardware Info */}
      <div className="p-5 rounded-2xl bg-gradient-to-r from-[#03152f] via-[#052148] to-black border border-sky-500/30">
        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-2">
              <h2 className="text-base md:text-lg font-bold text-white flex items-center gap-2">
                <Shuffle className="w-5 h-5 text-sky-400" />
                Reasignación de Mandos DirectInput / Generic HID
              </h2>
              <span className="text-[10px] px-2 py-0.5 rounded-full bg-sky-500/20 text-sky-300 font-mono">
                {selectedProfile.name}
              </span>
            </div>
            <p className="text-xs text-slate-300 mt-1">
              Configura cómo mapea cada botón físico del mando hacia las acciones de PlayStation 4. Los cambios se guardan en{' '}
              <code className="text-sky-400 font-mono">/data/anycontroller/controllers/VID_PID.ini</code>.
            </p>
          </div>

          <div className="flex items-center gap-2">
            <button
              onClick={handleDownloadProfileIni}
              className="flex items-center gap-2 px-4 py-2 rounded-xl bg-sky-500 hover:bg-sky-400 text-slate-950 font-bold text-xs shadow-[0_0_15px_rgba(56,189,248,0.4)] transition-all cursor-pointer"
            >
              <Download className="w-4 h-4" />
              <span>Descargar Perfil .INI</span>
            </button>
          </div>
        </div>
      </div>

      {/* Main Content Layout: Presets Catalog & Remap Table */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Left Column: Preset Controller Selector (4 cols) */}
        <div className="lg:col-span-4 space-y-4">
          <div className="p-4 rounded-2xl bg-black/40 border border-white/10 space-y-3">
            <h3 className="text-xs font-bold text-slate-300 uppercase tracking-wider flex items-center gap-2">
              <Search className="w-3.5 h-3.5 text-sky-400" />
              Catálogo de Mandos Compatibles
            </h3>

            {/* Search Input */}
            <div className="relative">
              <input
                type="text"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                placeholder="Buscar por modelo, VID o PID..."
                className="w-full py-2 pl-8 pr-3 text-xs bg-white/5 border border-white/10 rounded-xl text-white placeholder-slate-500 focus:outline-none focus:border-sky-400 transition-colors"
              />
              <Search className="w-3.5 h-3.5 text-slate-500 absolute left-2.5 top-2.5" />
            </div>

            {/* Profiles List */}
            <div className="space-y-1.5 max-h-[380px] overflow-y-auto pr-1 no-scrollbar">
              {filteredProfiles.map((p) => {
                const isSelected = selectedProfile.id === p.id;
                return (
                  <button
                    key={p.id}
                    onClick={() => handleSelectProfile(p)}
                    className={`w-full text-left p-2.5 rounded-xl border text-xs transition-all cursor-pointer ${
                      isSelected
                        ? 'bg-sky-500/20 text-white border-sky-400/60 shadow-[0_0_12px_rgba(56,189,248,0.25)]'
                        : 'bg-white/[0.02] hover:bg-white/5 text-slate-300 border-white/5 hover:border-white/15'
                    }`}
                  >
                    <div className="font-semibold line-clamp-1">{p.name}</div>
                    <div className="flex items-center gap-2 mt-1 text-[10px] text-slate-400 font-mono">
                      <span>VID: 0x{p.vendorId}</span>
                      <span>PID: 0x{p.productId}</span>
                      <span className="capitalize px-1.5 py-0.2 bg-white/10 rounded text-[9px]">
                        {p.category}
                      </span>
                    </div>
                  </button>
                );
              })}
            </div>

            {/* Custom VID/PID Override */}
            <div className="pt-2 border-t border-white/10 space-y-2">
              <span className="text-[11px] font-semibold text-slate-400">
                Identificador de Dispositivo USB (VID / PID):
              </span>
              <div className="grid grid-cols-2 gap-2 text-xs font-mono">
                <div>
                  <label className="text-[10px] text-slate-500 block mb-0.5">VENDOR ID</label>
                  <input
                    type="text"
                    value={customVid}
                    onChange={(e) => setCustomVid(e.target.value.toUpperCase())}
                    className="w-full p-1.5 bg-black/60 border border-white/10 rounded-lg text-white font-mono text-center"
                    maxLength={4}
                  />
                </div>
                <div>
                  <label className="text-[10px] text-slate-500 block mb-0.5">PRODUCT ID</label>
                  <input
                    type="text"
                    value={customPid}
                    onChange={(e) => setCustomPid(e.target.value.toUpperCase())}
                    className="w-full p-1.5 bg-black/60 border border-white/10 rounded-lg text-white font-mono text-center"
                    maxLength={4}
                  />
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* Right Column: Interactive Remap Table (8 cols) */}
        <div className="lg:col-span-8 space-y-4">
          <div className="p-4 rounded-2xl bg-black/40 border border-white/10">
            <div className="flex items-center justify-between pb-3 border-b border-white/10">
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <SlidersHorizontal className="w-4 h-4 text-sky-400" />
                Tabla de Asignación DualShock 4
              </h3>
              <span className="text-[11px] text-slate-400">
                Presiona flechas o haz clic para cambiar el botón físico asignado
              </span>
            </div>

            {/* Action rows */}
            <div className="divide-y divide-white/5 mt-2">
              {DS4_ACTIONS.map((action, idx) => {
                const assignedBtn = selectedProfile.assignments[action.id] ?? 0;
                const isSelected = selectedRow === idx;

                // Check if this action's mapped physical button is currently pressed on live controller
                const isPhysicallyPressed =
                  assignedBtn > 0 && liveInput.rawButtons[assignedBtn - 1];

                return (
                  <div
                    key={action.id}
                    onClick={() => setSelectedRow(idx)}
                    className={`py-2 px-3 rounded-xl flex items-center justify-between transition-colors cursor-pointer ${
                      isSelected
                        ? 'bg-sky-950/60 ring-1 ring-sky-400/50'
                        : 'hover:bg-white/[0.03]'
                    } ${
                      isPhysicallyPressed
                        ? 'bg-emerald-950/70 border border-emerald-400 shadow-[0_0_15px_rgba(52,211,153,0.3)]'
                        : ''
                    }`}
                  >
                    {/* Action Name & Icon */}
                    <div className="flex items-center gap-3">
                      <div className="w-6 h-6 rounded-lg bg-black/40 border border-white/10 flex items-center justify-center overflow-hidden">
                        <img
                          src={action.icon_path}
                          alt={action.short_name}
                          className="w-4 h-4 object-contain"
                          onError={(e) => {
                            (e.target as HTMLElement).style.display = 'none';
                          }}
                        />
                      </div>
                      <div>
                        <div className="flex items-center gap-2">
                          <span className="text-xs font-semibold text-white">
                            {action.display_name}
                          </span>
                          {isPhysicallyPressed && (
                            <span className="text-[9px] px-1.5 py-0.5 rounded bg-emerald-500 text-slate-950 font-bold uppercase tracking-wider animate-pulse">
                              Pulsado
                            </span>
                          )}
                        </div>
                        <span className="text-[10px] text-slate-400 block line-clamp-1">
                          {action.description}
                        </span>
                      </div>
                    </div>

                    {/* Assigned Button Controls */}
                    <div className="flex items-center gap-2">
                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          handleCycleButton(action.id, -1);
                        }}
                        className="p-1 rounded-md bg-white/5 hover:bg-sky-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                        title="Botón anterior"
                      >
                        <ChevronLeft className="w-3.5 h-3.5" />
                      </button>

                      <div className="min-w-[100px] text-center">
                        <span
                          className={`text-xs font-mono font-bold px-2 py-0.5 rounded ${
                            assignedBtn === 0
                              ? 'bg-red-950/60 text-red-400 border border-red-500/30'
                              : 'bg-sky-950/60 text-sky-300 border border-sky-500/30'
                          }`}
                        >
                          {assignedBtn === 0 ? 'Sin Asignar' : `Botón ${assignedBtn}`}
                        </span>
                      </div>

                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          handleCycleButton(action.id, 1);
                        }}
                        className="p-1 rounded-md bg-white/5 hover:bg-sky-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                        title="Botón siguiente"
                      >
                        <ChevronRight className="w-3.5 h-3.5" />
                      </button>
                    </div>
                  </div>
                );
              })}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
