/**
 * Controller Control Center PS4 - Pestaña de Reasignación de Mandos HID y Perfiles (v1.5.0)
 */
import { useState } from 'react';
import { ControllerProfile, LiveGamepadInput } from '../../types';
import { DS4_ACTIONS } from '../../constants/options';
import { CONTROLLER_PROFILES, findProfileByVidPid } from '../../data/profilesData';
import { generateControllerIni, downloadFile } from '../../utils/iniGenerator';
import {
  Shuffle,
  Download,
  Search,
  SlidersHorizontal,
  ChevronLeft,
  ChevronRight,
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
  const defaultProfile =
    findProfileByVidPid(activeVidPid.vid, activeVidPid.pid) || CONTROLLER_PROFILES[0];
  const [selectedProfile, setSelectedProfile] = useState<ControllerProfile>(defaultProfile);
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [selectedRow, setSelectedRow] = useState<number>(0);
  const [customVid, setCustomVid] = useState<string>(selectedProfile.vendorId);
  const [customPid, setCustomPid] = useState<string>(selectedProfile.productId);

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
    <div className="space-y-5">
      {/* Top Banner */}
      <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 flex flex-col lg:flex-row lg:items-center justify-between gap-4">
        <div>
          <h2 className="text-base md:text-lg font-bold text-white flex flex-wrap items-center gap-2">
            <Shuffle className="w-5 h-5 text-cyan-400" />
            <span>Reasignación de Mandos DirectInput / Generic HID</span>
            <span className="text-xs font-mono font-normal text-cyan-300">
              · {selectedProfile.name}
            </span>
          </h2>
          <p className="text-xs text-slate-400 mt-1">
            Configura cómo mapea cada botón físico hacia PS4. Los perfiles USB se guardan en{' '}
            <code className="text-cyan-400 font-mono">/data/anycontroller/controllers/VID_PID.ini</code>.
          </p>
        </div>

        <button
          type="button"
          onClick={handleDownloadProfileIni}
          className="flex items-center gap-2 px-4 py-2 rounded-lg bg-cyan-400 hover:bg-cyan-300 text-slate-950 font-bold text-xs transition-colors cursor-pointer whitespace-nowrap shrink-0"
        >
          <Download className="w-4 h-4" />
          <span>Descargar Perfil .INI</span>
        </button>
      </div>

      {/* Main Content Layout */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-5">
        {/* Left Column: Preset Catalog */}
        <div className="lg:col-span-4 space-y-4">
          <div className="p-4 rounded-xl bg-[#0B1120] border border-slate-800 space-y-3">
            <h3 className="text-xs font-bold text-slate-300 uppercase tracking-wider flex items-center gap-2">
              <Search className="w-3.5 h-3.5 text-cyan-400" />
              Catálogo de Mandos Compatibles
            </h3>

            <div className="relative">
              <input
                type="text"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                placeholder="Buscar por modelo, VID o PID..."
                className="w-full py-2 pl-8 pr-3 text-xs bg-[#070B14] border border-slate-800 rounded-lg text-white placeholder-slate-500 focus:outline-none focus:border-cyan-400 transition-colors"
              />
              <Search className="w-3.5 h-3.5 text-slate-500 absolute left-2.5 top-2.5" />
            </div>

            <div className="space-y-1.5 max-h-[380px] overflow-y-auto pr-1 no-scrollbar">
              {filteredProfiles.map((p) => {
                const isSelected = selectedProfile.id === p.id;
                return (
                  <button
                    key={p.id}
                    type="button"
                    onClick={() => handleSelectProfile(p)}
                    className={`w-full text-left p-2.5 rounded-lg border text-xs transition-colors cursor-pointer ${
                      isSelected
                        ? 'bg-[#101C34] text-white border-cyan-400/70'
                        : 'bg-[#070B14] hover:bg-slate-900 text-slate-300 border-slate-800'
                    }`}
                  >
                    <div className="font-semibold line-clamp-1">{p.name}</div>
                    <div className="flex items-center gap-2 mt-1 text-[10px] text-slate-400 font-mono tabular-nums">
                      <span>VID 0x{p.vendorId}</span>
                      <span>·</span>
                      <span>PID 0x{p.productId}</span>
                      <span>·</span>
                      <span className="capitalize text-cyan-300">{p.category}</span>
                    </div>
                  </button>
                );
              })}
            </div>

            <div className="pt-2 border-t border-slate-800 space-y-2">
              <span className="text-[11px] font-semibold text-slate-400">
                Identificador USB (VID / PID):
              </span>
              <div className="grid grid-cols-2 gap-2 text-xs font-mono">
                <div>
                  <label className="text-[10px] text-slate-500 block mb-0.5">VENDOR ID</label>
                  <input
                    type="text"
                    value={customVid}
                    onChange={(e) => setCustomVid(e.target.value.toUpperCase())}
                    className="w-full p-1.5 bg-[#070B14] border border-slate-800 rounded-md text-white font-mono text-center"
                    maxLength={4}
                  />
                </div>
                <div>
                  <label className="text-[10px] text-slate-500 block mb-0.5">PRODUCT ID</label>
                  <input
                    type="text"
                    value={customPid}
                    onChange={(e) => setCustomPid(e.target.value.toUpperCase())}
                    className="w-full p-1.5 bg-[#070B14] border border-slate-800 rounded-md text-white font-mono text-center"
                    maxLength={4}
                  />
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* Right Column: Remap Table */}
        <div className="lg:col-span-8">
          <div className="p-4 rounded-xl bg-[#0B1120] border border-slate-800">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 pb-3 border-b border-slate-800">
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <SlidersHorizontal className="w-4 h-4 text-cyan-400" />
                Tabla de Asignación DualShock 4
              </h3>
              <span className="text-[11px] text-slate-400">
                Usa las flechas para elegir el botón físico asignado
              </span>
            </div>

            <div className="divide-y divide-slate-800/80 mt-2">
              {DS4_ACTIONS.map((action, idx) => {
                const assignedBtn = selectedProfile.assignments[action.id] ?? 0;
                const isSelected = selectedRow === idx;
                const isPhysicallyPressed =
                  assignedBtn > 0 && liveInput.rawButtons[assignedBtn - 1];

                return (
                  <div
                    key={action.id}
                    onClick={() => setSelectedRow(idx)}
                    className={`py-2.5 px-3 rounded-lg flex items-center justify-between transition-colors cursor-pointer ${
                      isPhysicallyPressed
                        ? 'bg-emerald-950/60 border border-emerald-400/60'
                        : isSelected
                        ? 'bg-[#101C34] border border-cyan-400/50'
                        : 'hover:bg-slate-900/50 border border-transparent'
                    }`}
                  >
                    <div className="flex items-center gap-3">
                      <div className="w-7 h-7 rounded-md bg-[#070B14] border border-slate-800 flex items-center justify-center overflow-hidden shrink-0">
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
                            <span className="text-[10px] font-mono text-emerald-400 font-bold uppercase">
                              · PULSADO
                            </span>
                          )}
                        </div>
                        <span className="text-[11px] text-slate-400 block line-clamp-1">
                          {action.description}
                        </span>
                      </div>
                    </div>

                    <div className="flex items-center gap-2 shrink-0">
                      <button
                        type="button"
                        onClick={(e) => {
                          e.stopPropagation();
                          handleCycleButton(action.id, -1);
                        }}
                        className="p-1.5 rounded bg-slate-900 hover:bg-cyan-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
                        title="Botón anterior"
                      >
                        <ChevronLeft className="w-3.5 h-3.5" />
                      </button>

                      <div className="min-w-[96px] text-center">
                        <span
                          className={`text-xs font-mono font-bold px-2 py-0.5 rounded tabular-nums ${
                            assignedBtn === 0
                              ? 'bg-red-950/60 text-red-400 border border-red-500/30'
                              : 'bg-[#070B14] text-cyan-300 border border-slate-700'
                          }`}
                        >
                          {assignedBtn === 0 ? 'Sin Asignar' : `Botón ${assignedBtn}`}
                        </span>
                      </div>

                      <button
                        type="button"
                        onClick={(e) => {
                          e.stopPropagation();
                          handleCycleButton(action.id, 1);
                        }}
                        className="p-1.5 rounded bg-slate-900 hover:bg-cyan-500 hover:text-slate-950 text-slate-300 transition-colors cursor-pointer"
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
