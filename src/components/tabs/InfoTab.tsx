/**
 * Controller Control Center PS4 - Pestaña de Sistema, Red y Descargas (v1.5.3)
 * Documentación completa, visor/importador .INI por mando y notas de versión 1.5.3.
 */
import { useState } from 'react';
import { AppConfig, ControllerTargetMeta } from '../../types';
import {
  generateOneFamilyIni,
  parseOneFamilyIni,
  downloadFile,
  downloadBinaryFile,
} from '../../utils/iniGenerator';
import {
  Info,
  CheckCircle2,
  FileText,
  Download,
  Upload,
  Terminal,
  BookOpen,
  Sparkles,
  Gamepad2,
  ShieldCheck,
  Zap,
} from 'lucide-react';

interface InfoTabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  onShowToast: (msg: string) => void;
  activeTarget?: ControllerTargetMeta;
}

export default function InfoTab({
  config,
  onChangeConfig,
  onShowToast,
  activeTarget,
}: InfoTabProps) {
  const [activeSection, setActiveSection] = useState<
    'overview' | 'guides' | 'ini' | 'changelog'
  >('overview');

  const iniContent = generateOneFamilyIni(config, {
    targetPath: activeTarget?.ps4FullPath,
    targetLabel: activeTarget?.label,
  });

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (event) => {
      const text = event.target?.result as string;
      if (text) {
        const loadedConfig = parseOneFamilyIni(text, config);
        onChangeConfig(loadedConfig);
        onShowToast(
          `Archivo .INI importado y guardado en ${activeTarget?.shortLabel || 'Global'}.`
        );
      }
    };
    reader.readAsText(file);
  };

  return (
    <div className="space-y-5">
      {/* Sub-navigation */}
      <div className="flex flex-wrap items-center gap-2 pb-3 border-b border-slate-800">
        {[
          { id: 'overview', label: 'Resumen y Descargas', icon: Info },
          { id: 'guides', label: 'Guía y Memoria por Mando', icon: BookOpen },
          { id: 'ini', label: 'Visor y Editor .INI', icon: FileText },
          { id: 'changelog', label: 'Novedades v1.5.3', icon: Sparkles },
        ].map((sec) => {
          const Icon = sec.icon;
          const isSelected = activeSection === sec.id;
          return (
            <button
              key={sec.id}
              type="button"
              onClick={() => setActiveSection(sec.id as any)}
              className={`flex items-center gap-2 px-3.5 py-2 rounded-lg text-xs font-semibold border transition-colors cursor-pointer ${
                isSelected
                  ? 'bg-cyan-400 text-slate-950 border-cyan-300 font-bold'
                  : 'bg-[#0B1120] text-slate-300 hover:text-white border-slate-800'
              }`}
            >
              <Icon className="w-3.5 h-3.5" />
              <span>{sec.label}</span>
            </button>
          );
        })}
      </div>

      {/* SECTION 1: OVERVIEW & DOWNLOADS */}
      {activeSection === 'overview' && (
        <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-4">
            <div className="flex items-center gap-3">
              <div className="p-2.5 rounded-lg bg-cyan-500/15 text-cyan-400 border border-cyan-500/30">
                <ShieldCheck className="w-5 h-5" />
              </div>
              <div>
                <h3 className="text-base font-bold text-white">
                  Controller Control Center PS4
                </h3>
                <span className="text-xs text-cyan-300 font-mono">
                  v1.5.3 (01.53) · ID: TONI88888 · ANYCONTROLLER
                </span>
              </div>
            </div>

            <p className="text-xs text-slate-300 leading-relaxed">
              Aplicación Homebrew 1080p para PlayStation 4 con <strong>GoldHEN</strong> y demonio en segundo plano{' '}
              <strong>ANYCONTROLLER</strong>. Incluye probador visual original en 2D para DualSense PS5 y DualShock 4, selector manual entre mandos conectados y memoria independiente persistente por dirección MAC y ranura.
            </p>

            <div className="p-3.5 rounded-lg bg-[#070B14] border border-slate-800 space-y-2 text-xs">
              <div className="flex justify-between gap-2">
                <span className="text-slate-400">Title ID / Content ID:</span>
                <span className="font-mono text-slate-200 font-semibold text-right">
                  TONI88888 (UP9000-TONI88888_00-ANYCONTROLLERPS4)
                </span>
              </div>
              <div className="flex justify-between gap-2">
                <span className="text-slate-400">Payload Demonio:</span>
                <span className="font-mono text-emerald-400 font-semibold text-right">
                  /data/payloads/anycontroller.bin
                </span>
              </div>
              <div className="flex justify-between gap-2">
                <span className="text-slate-400">Perfil Global PS4:</span>
                <span className="font-mono text-cyan-300 text-right">
                  /data/anycontroller/anycontroller.ini
                </span>
              </div>
              <div className="flex justify-between gap-2">
                <span className="text-slate-400">Memoria por Mando:</span>
                <span className="font-mono text-cyan-300 text-right">
                  /data/anycontroller/controllers/ds_mac_*.ini
                </span>
              </div>
            </div>
          </div>

          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-4">
            <div className="flex items-center gap-3">
              <div className="p-2.5 rounded-lg bg-emerald-500/15 text-emerald-400 border border-emerald-500/30">
                <Zap className="w-5 h-5" />
              </div>
              <div>
                <h3 className="text-base font-bold text-white">Compatibilidad GoldHEN</h3>
                <span className="text-xs text-emerald-300 font-semibold">
                  GoldHEN ≥ 2.4b18.10 (Todos los Firmwares)
                </span>
              </div>
            </div>

            <p className="text-xs text-slate-300 leading-relaxed">
              Al iniciar la aplicación en PS4, instala automáticamente <code className="text-cyan-300 font-mono">anycontroller.bin</code> en GoldHEN y lo añade a <code className="text-cyan-300 font-mono">[AutoRun]</code>.
            </p>

            <div className="p-3.5 rounded-lg bg-[#070B14] border border-slate-800 font-mono text-[11px] text-slate-300 space-y-1">
              <div className="text-slate-500"># /data/GoldHEN/payloads.ini</div>
              <div className="text-emerald-400">[AutoRun]</div>
              <div className="text-emerald-400">/data/payloads/anycontroller.bin = 1</div>
            </div>

            <div className="flex items-center gap-2 text-xs text-emerald-400">
              <CheckCircle2 className="w-4 h-4 shrink-0" />
              <span>Verificado en FW 5.05, 6.72, 9.00, 9.60, 11.00, 12.50 y 13.xx.</span>
            </div>
          </div>

          {/* Direct Downloads Card */}
          <div className="md:col-span-2 p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-4">
            <div>
              <h3 className="text-base font-bold text-white flex items-center gap-2">
                <Download className="w-4 h-4 text-cyan-400" />
                Descargas Oficiales v1.5.3 Listas para Instalar en PS4
              </h3>
              <p className="text-xs text-slate-400 mt-1">
                Paquetes compilados, validados y listos para copiar por USB o FTP a tu consola.
              </p>
            </div>

            <div className="p-4 rounded-xl bg-[#070E1D] border border-emerald-500/40 flex flex-col sm:flex-row sm:items-center justify-between gap-4">
              <div className="space-y-1">
                <div className="flex flex-wrap items-center gap-2">
                  <Sparkles className="w-4 h-4 text-emerald-400" />
                  <span className="text-sm font-bold text-white">
                    Pack Todo-en-Uno v1.5.3 (Recomendado)
                  </span>
                </div>
                <p className="text-xs text-slate-300">
                  Incluye <strong>Controller_Control_Center_PS4_v1.5.3.pkg</strong>, el payload{' '}
                  <strong>anycontroller.bin</strong> y <strong>anycontroller.ini</strong>.
                </p>
              </div>
              <button
                type="button"
                onClick={async () => {
                  onShowToast('Descargando Pack Todo-en-Uno v1.5.3...');
                  await downloadBinaryFile(
                    '/Controller_Control_Center_PS4_Pack_v1.5.3.zip',
                    'Controller_Control_Center_PS4_Pack_v1.5.3.zip'
                  );
                  onShowToast('¡Controller_Control_Center_PS4_Pack_v1.5.3.zip descargado!');
                }}
                className="shrink-0 flex items-center gap-2 px-5 py-2.5 rounded-lg bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-bold text-xs transition-colors cursor-pointer"
              >
                <Download className="w-4 h-4" />
                <span>Descargar Pack v1.5.3 (.ZIP)</span>
              </button>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3">
              <button
                type="button"
                onClick={async () => {
                  onShowToast('Descargando Controller_Control_Center_PS4_v1.5.3.pkg...');
                  await downloadBinaryFile(
                    '/Controller_Control_Center_PS4_v1.5.3.pkg',
                    'Controller_Control_Center_PS4_v1.5.3.pkg'
                  );
                  onShowToast('¡Controller_Control_Center_PS4_v1.5.3.pkg descargado!');
                }}
                className="flex items-center justify-between p-3.5 rounded-lg bg-cyan-400 hover:bg-cyan-300 text-slate-950 font-bold text-xs transition-colors cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Descargar .PKG v1.5.3</span>
                  <span className="text-[10px] text-slate-900 font-mono">
                    Validado OpenOrbis
                  </span>
                </div>
                <Download className="w-4 h-4 shrink-0" />
              </button>

              <button
                type="button"
                onClick={async () => {
                  onShowToast('Descargando payload ANYCONTROLLER...');
                  await downloadBinaryFile('/anycontroller.bin', 'anycontroller.bin');
                  onShowToast('¡anycontroller.bin descargado!');
                }}
                className="flex items-center justify-between p-3.5 rounded-lg bg-[#070B14] hover:bg-slate-900 text-white font-bold text-xs border border-slate-800 transition-colors cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Payload GoldHEN (.BIN)</span>
                  <span className="text-[10px] text-slate-400 font-mono">
                    anycontroller.bin
                  </span>
                </div>
                <Download className="w-4 h-4 text-cyan-400 shrink-0" />
              </button>

              <button
                type="button"
                onClick={() => {
                  downloadFile(
                    activeTarget?.ps4Filename || 'anycontroller.ini',
                    iniContent
                  );
                  onShowToast(
                    `${activeTarget?.ps4Filename || 'anycontroller.ini'} descargado.`
                  );
                }}
                className="flex items-center justify-between p-3.5 rounded-lg bg-[#070B14] hover:bg-slate-900 text-white font-bold text-xs border border-slate-800 transition-colors cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Exportar .INI Actual</span>
                  <span className="text-[10px] text-slate-400 font-mono">
                    {activeTarget?.ps4Filename || 'anycontroller.ini'}
                  </span>
                </div>
                <Download className="w-4 h-4 shrink-0 text-emerald-400" />
              </button>

              <button
                type="button"
                onClick={async () => {
                  onShowToast('Descargando código fuente v1.5.3...');
                  await downloadBinaryFile(
                    '/Controller_Control_Center_PS4_Source_v1.5.3.zip',
                    'Controller_Control_Center_PS4_Source_v1.5.3.zip'
                  );
                  onShowToast('¡Código fuente v1.5.3 descargado!');
                }}
                className="flex items-center justify-between p-3.5 rounded-lg bg-[#070B14] hover:bg-slate-900 text-slate-200 font-bold text-xs border border-slate-800 transition-colors cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Código Fuente (.ZIP)</span>
                  <span className="text-[10px] text-slate-400 font-mono">
                    C / C++ v1.5.3
                  </span>
                </div>
                <Download className="w-4 h-4 text-indigo-400 shrink-0" />
              </button>
            </div>
          </div>
        </div>
      )}

      {/* SECTION 2: GUIDES & PER-CONTROLLER MEMORY */}
      {activeSection === 'guides' && (
        <div className="space-y-5">
          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-3">
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <Sparkles className="w-4 h-4 text-cyan-400" />
              Cómo funciona la Memoria Independiente por Mando (v1.5.3)
            </h3>
            <ul className="list-disc list-inside text-xs text-slate-300 space-y-1.5 pl-1">
              <li>
                <strong>Perfil Global (<code className="text-cyan-300 font-mono">anycontroller.ini</code>):</strong> Es la plantilla base que heredan todos los mandos recién conectados mientras no los personalices por separado.
              </li>
              <li>
                <strong>Configuración Independiente por Mando:</strong> Selecciona un mando específico con{' '}
                <kbd className="px-1.5 py-0.5 bg-slate-900 border border-slate-700 rounded">L3</kbd> en PS4 (o arriba en esta web) para ajustar colores, curva, zona muerta (drift), vibración o gatillos sin afectar al otro jugador.
              </li>
              <li>
                <strong>Recuerdo Persistente entre Sesiones:</strong> Cada DualSense PS5 guarda su archivo propio usando su <strong>dirección MAC única</strong> (<code className="text-cyan-300 font-mono">ds_mac_XXXXXXXXXXXX.ini</code>). Al volver a conectarlo mañana o en otra ranura, recupera automáticamente su color, brillo y calibración.
              </li>
              <li>
                <strong>Restaurar a Global:</strong> Pulsa <kbd className="px-1.5 py-0.5 bg-slate-900 border border-slate-700 rounded">R3</kbd> en PS4 o <em>Restaurar a Global</em> aquí para borrar su perfil propio y que vuelva a seguir la configuración Global.
              </li>
            </ul>
          </div>

          <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-3">
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <Gamepad2 className="w-4 h-4 text-emerald-400" />
              Atajos Universales y Navegación sin Solapamientos
            </h3>
            <div className="grid grid-cols-1 sm:grid-cols-3 gap-3 pt-1">
              <div className="p-3 rounded-lg bg-[#070B14] border border-slate-800 space-y-1 text-xs">
                <span className="font-bold text-cyan-300">Cambiar Mando (PS4)</span>
                <p className="text-slate-400">
                  Pulsa <kbd className="px-1 bg-slate-900 rounded">L3</kbd> para rotar entre Global y tus mandos conectados.
                </p>
              </div>
              <div className="p-3 rounded-lg bg-[#070B14] border border-slate-800 space-y-1 text-xs">
                <span className="font-bold text-cyan-300">Modo Prueba Segura</span>
                <p className="text-slate-400">
                  En Laboratorio PS4 mantén <strong>OPTIONS</strong> para salir y liberar L1, R1, L3 y ✕ sin conflictos de botones.
                </p>
              </div>
              <div className="p-3 rounded-lg bg-[#070B14] border border-slate-800 space-y-1 text-xs">
                <span className="font-bold text-cyan-300">Touchpad en Mandos Xbox</span>
                <p className="text-slate-400">
                  Usa el botón <strong>View / Back</strong> o <kbd className="px-1 bg-slate-900 rounded">L1+R1+View</kbd> para abrir mapas.
                </p>
              </div>
            </div>
          </div>
        </div>
      )}

      {/* SECTION 3: INI VIEWER */}
      {activeSection === 'ini' && (
        <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-4">
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
            <div>
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <FileText className="w-4 h-4 text-cyan-400" />
                Previsualización: {activeTarget?.ps4FullPath || '/data/anycontroller/anycontroller.ini'}
              </h3>
              <p className="text-xs text-slate-400">
                Copia este archivo por FTP o USB para aplicar este perfil exacto en tu consola.
              </p>
            </div>

            <div className="flex items-center gap-2">
              <label className="flex items-center gap-2 px-3 py-1.5 rounded-lg bg-[#070B14] hover:bg-slate-900 text-white font-medium text-xs border border-slate-700 cursor-pointer transition-colors">
                <Upload className="w-3.5 h-3.5 text-cyan-400" />
                <span>Cargar .INI</span>
                <input
                  type="file"
                  accept=".ini,.txt"
                  onChange={handleFileUpload}
                  className="hidden"
                />
              </label>

              <button
                type="button"
                onClick={() => {
                  downloadFile(
                    activeTarget?.ps4Filename || 'anycontroller.ini',
                    iniContent
                  );
                  onShowToast(
                    `Archivo ${activeTarget?.ps4Filename || 'anycontroller.ini'} descargado.`
                  );
                }}
                className="flex items-center gap-2 px-3 py-1.5 rounded-lg bg-cyan-400 hover:bg-cyan-300 text-slate-950 font-bold text-xs transition-colors cursor-pointer"
              >
                <Download className="w-3.5 h-3.5" />
                <span>Descargar {activeTarget?.ps4Filename || '.INI'}</span>
              </button>
            </div>
          </div>

          <pre className="p-4 rounded-lg bg-[#050A14] border border-slate-800 font-mono text-xs text-cyan-300 overflow-x-auto max-h-[360px] leading-relaxed">
            {iniContent}
          </pre>
        </div>
      )}

      {/* SECTION 4: CHANGELOG v1.5.3 */}
      {activeSection === 'changelog' && (
        <div className="p-5 rounded-xl bg-[#0B1120] border border-slate-800 space-y-4">
          <h3 className="text-sm font-bold text-white flex items-center gap-2">
            <Terminal className="w-4 h-4 text-cyan-400" />
            Novedades Controller Control Center PS4 v1.5.3 (01.53)
          </h3>

          <div className="space-y-3 text-xs text-slate-300 divide-y divide-slate-800">
            <div className="pt-2">
              <h4 className="font-bold text-cyan-300">
                1. Aislamiento Total DualSense 1 + 2 ("Cero Conexiones a la Nada")
              </h4>
              <p className="text-slate-400 mt-1">
                Corregida la interferencia del gestor de barra LED DualShock 4 durante el arranque Bluetooth: validación por <code className="font-mono text-slate-200">Device ID</code> y <code className="font-mono text-slate-200">bound_user_id</code> para no tocar jamás handles de mandos virtuales, eliminación de la doble pulsación PS y limpieza de pila en carga de perfiles (<code className="font-mono text-slate-200">config.c</code>).
              </p>
            </div>

            <div className="pt-3">
              <h4 className="font-bold text-emerald-400">
                2. Apagado Limpio del DualShock 4 y Flanco PS Puro al Conectar
              </h4>
              <p className="text-slate-400 mt-1">
                Ventana de estabilización de <code className="font-mono text-slate-200">280 ms</code> que evita que mantener pulsado el botón PS al encender el DualSense arruine la transición nativa en Jugador 1 (<code className="font-mono text-slate-200">¿Quién está usando este mando?</code>) o el relevo automático del DS4 en modo Merge.
              </p>
            </div>

            <div className="pt-3">
              <h4 className="font-bold text-emerald-400">
                3. Colores RGB DualShock 4, TV Safe Area 1080p y Mandos 2D Originales
              </h4>
              <p className="text-slate-400 mt-1">
                Conserva intactos los colores/brillo/vibración persistentes del DualShock 4, el visor 2D original PS5 DualSense y PS4 DualShock 4 sin solapamiento de botones, el selector manual de jugador y la memoria independiente por MAC (<code className="font-mono text-slate-200">ds_mac_*.ini</code>) y ranura (<code className="font-mono text-slate-200">ds4_slot_*.ini</code>).
              </p>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
