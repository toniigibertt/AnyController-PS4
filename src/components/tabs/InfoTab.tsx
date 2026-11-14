/**
 * OneFamily PS4 - Pestaña de Información, Guías y Actualizador en Español (Info Tab)
 * Documentación completa, guía de atajos de Touchpad, instalador de payload y visor de .INI
 */
import { useState } from 'react';
import { AppConfig } from '../../types';
import { generateOneFamilyIni, parseOneFamilyIni, downloadFile, downloadBinaryFile } from '../../utils/iniGenerator';
import {
  Info,
  CheckCircle2,
  FileText,
  Download,
  Upload,
  Terminal,
  HelpCircle,
  ExternalLink,
  BookOpen,
  Cpu,
  Layers,
  Sparkles,
  Gamepad2,
  ShieldCheck,
  Zap,
} from 'lucide-react';

interface InfoTabProps {
  config: AppConfig;
  onChangeConfig: (newConfig: AppConfig) => void;
  onShowToast: (msg: string) => void;
}

export default function InfoTab({
  config,
  onChangeConfig,
  onShowToast,
}: InfoTabProps) {
  const [activeSection, setActiveSection] = useState<'overview' | 'guides' | 'ini' | 'changelog'>('overview');
  const iniContent = generateOneFamilyIni(config);

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (event) => {
      const text = event.target?.result as string;
      if (text) {
        const loadedConfig = parseOneFamilyIni(text, config);
        onChangeConfig(loadedConfig);
        onShowToast('¡Archivo anycontroller.ini cargado y aplicado correctamente!');
      }
    };
    reader.readAsText(file);
  };

  return (
    <div className="space-y-6">
      {/* Navigation Sub-tabs */}
      <div className="flex flex-wrap items-center gap-2 pb-2 border-b border-white/10">
        {[
          { id: 'overview', label: 'Resumen y Estado', icon: Info },
          { id: 'guides', label: 'Guía de Conexión y Atajos', icon: BookOpen },
          { id: 'ini', label: 'Visor y Editor .INI', icon: FileText },
          { id: 'changelog', label: 'Novedades y Mejoras (v1.3.9)', icon: Sparkles },
        ].map((sec) => {
          const Icon = sec.icon;
          const isSelected = activeSection === sec.id;
          return (
            <button
              key={sec.id}
              onClick={() => setActiveSection(sec.id as any)}
              className={`flex items-center gap-2 px-3.5 py-1.5 rounded-xl text-xs font-semibold transition-all cursor-pointer ${
                isSelected
                  ? 'bg-sky-500 text-slate-950 font-bold shadow-[0_0_12px_rgba(56,189,248,0.3)]'
                  : 'bg-black/40 text-slate-400 hover:text-white border border-white/5'
              }`}
            >
              <Icon className="w-3.5 h-3.5" />
              <span>{sec.label}</span>
            </button>
          );
        })}
      </div>

      {/* SECTION 1: OVERVIEW & STATUS */}
      {activeSection === 'overview' && (
        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          {/* Card: AnyController Overview */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10 space-y-4">
            <div className="flex items-center gap-3">
              <div className="p-2.5 rounded-xl bg-sky-500/20 text-sky-400 border border-sky-500/30">
                <ShieldCheck className="w-6 h-6" />
              </div>
              <div>
                <h3 className="text-base font-bold text-white">Controller Control Center PS4 (ID: TONI88888)</h3>
                <span className="text-xs text-sky-300 font-semibold font-mono">
                  Versión: v1.3.9 / SFO 01.39 • Payload: ANYCONTROLLER
                </span>
              </div>
            </div>

            <p className="text-xs text-slate-300 leading-relaxed">
              <strong>Controller Control Center PS4 v1.3.9</strong> (motor demonio <strong>ANYCONTROLLER</strong>) es una aplicación compañera a 1080p 100% independiente para consolas PlayStation 4 con <strong>GoldHEN</strong>.
              Incluye <strong>Soporte Bluetooth Multi-DualSense real de 4 Ranuras simultáneas</strong> con respuesta inmediata del 2.º DualSense, detección instantánea de desconexión Bluetooth, cambio automático e inteligente entre las pestañas DualSense PS5 y DualShock 4, y renderizado tipográfico mejorado sin solapamientos.
            </p>

            <div className="p-3.5 rounded-xl bg-white/[0.02] border border-white/5 space-y-2 text-xs">
              <div className="flex justify-between">
                <span className="text-slate-400">Title ID / Content ID:</span>
                <span className="font-mono text-indigo-300 font-bold">TONI88888 (UP9000-TONI88888_00-ANYCONTROLLERPS4)</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Payload Demonio:</span>
                <span className="font-mono text-emerald-400 font-bold">ANYCONTROLLER (/data/payloads/anycontroller.bin)</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Ruta de Configuración PS4:</span>
                <span className="font-mono text-sky-300">/data/anycontroller/anycontroller.ini</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Tasa de Sondeo (Polling Rate):</span>
                <span className="font-mono text-emerald-400 font-bold">250 Hz (4ms)</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Proyecto:</span>
                <span className="text-slate-300 font-medium">Controller Control Center PS4 - toniigibertt/AnyController-PS4</span>
              </div>
            </div>
          </div>

          {/* Card: GoldHEN Compatibility & Auto-Run */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10 space-y-4">
            <div className="flex items-center gap-3">
              <div className="p-2.5 rounded-xl bg-emerald-500/20 text-emerald-400 border border-emerald-500/30">
                <Zap className="w-6 h-6" />
              </div>
              <div>
                <h3 className="text-base font-bold text-white">Compatibilidad GoldHEN</h3>
                <span className="text-xs text-emerald-300 font-semibold">
                  GoldHEN ≥ 2.4b18.10 (Todos los Firmwares)
                </span>
              </div>
            </div>

            <p className="text-xs text-slate-300 leading-relaxed">
              El demonio se carga automáticamente al encender la consola agregándolo a la lista de inicio automático <code className="text-sky-300 font-mono">payloads.ini</code> de GoldHEN.
            </p>

            <div className="p-3.5 rounded-xl bg-black/60 border border-white/10 font-mono text-[11px] text-slate-300 space-y-1">
              <div className="text-slate-500"># Ubicación del Payload en el disco interno de PS4:</div>
              <div className="text-sky-300">/data/payloads/anycontroller.bin</div>
              <div className="text-slate-500 mt-2"># Entorno de inicio en /data/GoldHEN/payloads.ini:</div>
              <div className="text-emerald-400">[AutoRun]</div>
              <div className="text-emerald-400">/data/payloads/anycontroller.bin = 1</div>
            </div>

            <div className="flex items-center gap-2 text-xs text-emerald-400">
              <CheckCircle2 className="w-4 h-4 shrink-0" />
              <span>Soporte verificado en FW 5.05, 6.72, 9.00, 9.60, 11.00, 12.50, 13.02 y 13.52.</span>
            </div>
          </div>

          {/* New Card: Direct Downloads for PS4 */}
          <div className="md:col-span-2 p-5 rounded-2xl bg-gradient-to-r from-sky-950/60 via-[#0a2750]/50 to-black/60 border border-sky-400/40 space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div>
                <h3 className="text-base font-bold text-white flex items-center gap-2">
                  <Download className="w-5 h-5 text-sky-400" />
                  Descargas Oficiales y Paquete Listo para Instalar en PS4
                </h3>
                <p className="text-xs text-slate-300">
                  Archivos compilados listos para tu consola. <strong>No necesitas compilar nada en tu PC.</strong>
                </p>
              </div>
            </div>

            {/* Featured All-in-one Pack for DualSense */}
            <div className="p-4 rounded-xl bg-gradient-to-r from-emerald-950/80 to-sky-950/80 border border-emerald-400/50 shadow-[0_0_20px_rgba(52,211,153,0.2)] flex flex-col sm:flex-row sm:items-center justify-between gap-4">
              <div className="space-y-1">
                <div className="flex items-center gap-2">
                  <Sparkles className="w-4 h-4 text-emerald-400" />
                  <span className="text-sm font-bold text-white">Pack Todo-en-Uno Mando PS5 DualSense</span>
                  <span className="text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/20 text-emerald-300 border border-emerald-500/30 font-semibold">
                    Recomendado
                  </span>
                </div>
                <p className="text-xs text-slate-300">
                  Incluye el <strong>Controller_Control_Center_PS4_v1.3.9.pkg</strong>, el <strong>payload ANYCONTROLLER (anycontroller.bin)</strong> y la <strong>configuración anycontroller.ini</strong> en un solo archivo.
                </p>
              </div>
              <button
                onClick={async () => {
                  onShowToast('Descargando Pack Todo-en-Uno v1.3.9 (PKG + bin + ini)...');
                  await downloadBinaryFile('/Controller_Control_Center_PS4_Pack_v1.3.9.zip', 'Controller_Control_Center_PS4_Pack_v1.3.9.zip');
                  onShowToast('¡Controller_Control_Center_PS4_Pack_v1.3.9.zip descargado con éxito!');
                }}
                className="shrink-0 flex items-center gap-2 px-5 py-2.5 rounded-xl bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-bold text-xs shadow-[0_0_15px_rgba(52,211,153,0.4)] transition-all text-center cursor-pointer"
              >
                <Download className="w-4 h-4" />
                <span>Descargar Pack v1.3.9 (.ZIP)</span>
              </button>
            </div>

            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3 pt-1">
              <button
                onClick={async () => {
                  onShowToast('Iniciando descarga directa de Controller_Control_Center_PS4_v1.3.9.pkg...');
                  await downloadBinaryFile('/Controller_Control_Center_PS4_v1.3.9.pkg', 'Controller_Control_Center_PS4_v1.3.9.pkg');
                  onShowToast('¡Controller_Control_Center_PS4_v1.3.9.pkg descargado con éxito!');
                }}
                className="flex items-center justify-between p-3.5 rounded-xl bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-bold text-xs shadow-[0_0_15px_rgba(52,211,153,0.3)] transition-all cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Descargar .PKG v1.3.9</span>
                  <span className="text-[10px] text-slate-900 font-normal">Controller_Control_Center_PS4_v1.3.9.pkg</span>
                </div>
                <Download className="w-4 h-4 shrink-0" />
              </button>

              <button
                onClick={async () => {
                  onShowToast('Descargando payload ANYCONTROLLER (anycontroller.bin)...');
                  await downloadBinaryFile('/anycontroller.bin', 'anycontroller.bin');
                  onShowToast('¡anycontroller.bin descargado!');
                }}
                className="flex items-center justify-between p-3.5 rounded-xl bg-white/10 hover:bg-white/20 text-white font-bold text-xs border border-white/15 transition-all cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Payload ANYCONTROLLER (.BIN)</span>
                  <span className="text-[10px] text-slate-400 font-normal">anycontroller.bin (GoldHEN)</span>
                </div>
                <Download className="w-4 h-4 text-sky-400 shrink-0" />
              </button>

              <button
                onClick={() => {
                  downloadFile('anycontroller.ini', iniContent);
                  onShowToast('anycontroller.ini descargado listo para transferir a PS4');
                }}
                className="flex items-center justify-between p-3.5 rounded-xl bg-white/10 hover:bg-white/20 text-white font-bold text-xs border border-white/15 transition-all cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Configuración (.INI)</span>
                  <span className="text-[10px] text-slate-300 font-normal">anycontroller.ini (Tus ajustes)</span>
                </div>
                <Download className="w-4 h-4 shrink-0 text-emerald-400" />
              </button>

              <button
                onClick={async () => {
                  onShowToast('Descargando código fuente v1.3.9 en español...');
                  await downloadBinaryFile('/Controller_Control_Center_PS4_Source_v1.3.9.zip', 'Controller_Control_Center_PS4_Source_v1.3.9.zip');
                  onShowToast('¡Código fuente descargado!');
                }}
                className="flex items-center justify-between p-3.5 rounded-xl bg-purple-500/20 hover:bg-purple-500/30 text-purple-200 font-bold text-xs border border-purple-500/40 transition-all cursor-pointer"
              >
                <div className="flex flex-col text-left">
                  <span>Código Fuente C++ (.ZIP)</span>
                  <span className="text-[10px] text-purple-300 font-normal">Fuentes v1.3.9 + build_app.sh</span>
                </div>
                <Download className="w-4 h-4 text-purple-400 shrink-0" />
              </button>
            </div>

            {/* Practical Guide on Applying Changes */}
            <div className="mt-4 p-4 rounded-xl bg-black/60 border border-white/10 space-y-3">
              <h4 className="text-xs font-bold text-sky-300 uppercase tracking-wider flex items-center gap-1.5">
                <Info className="w-4 h-4" />
                ¿Cómo se aplican los cambios a tu PlayStation 4?
              </h4>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4 text-xs text-slate-300">
                <div className="p-3 rounded-lg bg-white/[0.02] border border-white/5 space-y-1.5">
                  <span className="font-bold text-emerald-400 flex items-center gap-1">
                    <CheckCircle2 className="w-3.5 h-3.5" /> Método 1 (Automático desde el PKG)
                  </span>
                  <p className="text-slate-400 leading-relaxed">
                    Al abrir <strong>Controller Control Center PS4</strong> en tu PS4, el instalador copia automáticamente el payload <strong>ANYCONTROLLER</strong> (<code className="text-sky-300 font-mono">/data/payloads/anycontroller.bin</code>), configura <code className="text-sky-300 font-mono">/data/GoldHEN/payloads.ini</code> y guarda todos los cambios en <code className="text-sky-300 font-mono">/data/anycontroller/anycontroller.ini</code>.
                  </p>
                </div>

                <div className="p-3 rounded-lg bg-white/[0.02] border border-white/5 space-y-1.5">
                  <span className="font-bold text-purple-400 flex items-center gap-1">
                    <Terminal className="w-3.5 h-3.5" /> Método 2 (Actualizaciones Directas por GitHub)
                  </span>
                  <p className="text-slate-400 leading-relaxed">
                    Al subir <code className="text-sky-300 font-mono">Controller_Control_Center_PS4_v1.3.9.pkg</code> a las Releases de tu repositorio <code className="text-sky-300 font-mono">toniigibertt/AnyController-PS4</code>, la aplicación en PS4 detecta nuevas versiones y las descarga directamente con su número de versión a <code className="text-sky-300 font-mono">/data/pkg/</code>.
                  </p>
                </div>
              </div>
            </div>
          </div>
        </div>
      )}

      {/* SECTION 2: CONNECTION GUIDES & SHORTCUTS */}
      {activeSection === 'guides' && (
        <div className="space-y-6">
          {/* Guide 1: DualSense Bluetooth Pairing */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10 space-y-3">
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <Sparkles className="w-4 h-4 text-sky-400" />
              Emparejamiento de Mando Sony DualSense (PS5) por Bluetooth Nativo
            </h3>
            <ol className="list-decimal list-inside text-xs text-slate-300 space-y-1.5 pl-2">
              <li>Apaga el mando DualSense si está encendido.</li>
              <li>Mantén pulsados simultáneamente el <strong>Botón Crear (Share)</strong> y el <strong>Botón PS</strong> durante 3 segundos hasta que la barra de luz parpadee con destellos azules dobles rápidos (Modo Emparejamiento).</li>
              <li>En tu consola PS4, ve a <em>Ajustes &gt; Dispositivos &gt; Dispositivos Bluetooth</em>.</li>
              <li>Selecciona <code className="text-sky-300">Wireless Controller</code> de la lista y confirma la vinculación.</li>
              <li>¡Listo! AnyController activará automáticamente el reporte de 78 bytes con giroscopio, touchpad, gatillos adaptativos y barra de luz RGB.</li>
            </ol>
          </div>

          {/* Guide 2: Universal Touchpad Emulation Combos */}
          <div className="p-5 rounded-2xl bg-black/40 border border-white/10 space-y-3">
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <Gamepad2 className="w-4 h-4 text-emerald-400" />
              Atajos para Emular el Panel Táctil (Touchpad) y Menú PS en Mandos Xbox / PC / Switch
            </h3>
            <p className="text-xs text-slate-300">
              Dado que los mandos de Xbox y Nintendo no disponen de panel táctil físico, AnyController incluye combinaciones universales libres de conflictos:
            </p>

            <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-3 pt-2">
              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-1 text-xs">
                <span className="font-bold text-sky-300">Clic de Touchpad Directo</span>
                <p className="text-slate-400">Pulsa el botón <strong>View / Back</strong> (Mapeo directo predeterminado).</p>
              </div>

              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-1 text-xs">
                <span className="font-bold text-sky-300">Atajo Combo Touchpad</span>
                <p className="text-slate-400">Mantén <kbd className="px-1.5 py-0.5 bg-white/10 rounded">L1 + R1 + View</kbd> para forzar el clic del panel táctil en cualquier juego.</p>
              </div>

              <div className="p-3 rounded-xl bg-white/[0.02] border border-white/5 space-y-1 text-xs">
                <span className="font-bold text-sky-300">Botón PS Home Alternativo</span>
                <p className="text-slate-400">Pulsa el <strong>Botón Guía Xbox / Home</strong> o la combinación <kbd className="px-1.5 py-0.5 bg-white/10 rounded">L1 + R1 + Menu</kbd>.</p>
              </div>
            </div>
          </div>
        </div>
      )}

      {/* SECTION 3: INI VIEWER & GENERATOR */}
      {activeSection === 'ini' && (
        <div className="p-5 rounded-2xl bg-black/40 border border-white/10 space-y-4">
          <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
            <div>
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <FileText className="w-4 h-4 text-sky-400" />
                Previsualización en Vivo de /data/anycontroller/anycontroller.ini
              </h3>
              <p className="text-xs text-slate-400">
                Archivo listo para copiar a tu consola PS4 mediante cliente FTP o memoria USB.
              </p>
            </div>

            <div className="flex items-center gap-2">
              <label className="flex items-center gap-2 px-3 py-1.5 rounded-xl bg-white/10 hover:bg-white/20 text-white font-medium text-xs border border-white/10 cursor-pointer transition-colors">
                <Upload className="w-3.5 h-3.5 text-sky-400" />
                <span>Cargar .INI</span>
                <input
                  type="file"
                  accept=".ini,.txt"
                  onChange={handleFileUpload}
                  className="hidden"
                />
              </label>

              <button
                onClick={() => {
                  downloadFile('anycontroller.ini', iniContent);
                  onShowToast('Archivo anycontroller.ini descargado correctamente');
                }}
                className="flex items-center gap-2 px-3 py-1.5 rounded-xl bg-sky-500 hover:bg-sky-400 text-slate-950 font-bold text-xs transition-colors cursor-pointer"
              >
                <Download className="w-3.5 h-3.5" />
                <span>Descargar .INI</span>
              </button>
            </div>
          </div>

          <pre className="p-4 rounded-xl bg-black/80 border border-white/10 font-mono text-xs text-sky-300 overflow-x-auto max-h-[350px] leading-relaxed">
            {iniContent}
          </pre>
        </div>
      )}

      {/* SECTION 4: CHANGELOG & BUGFIXES */}
      {activeSection === 'changelog' && (
        <div className="p-5 rounded-2xl bg-black/40 border border-white/10 space-y-4">
          <h3 className="text-sm font-bold text-white flex items-center gap-2">
            <Sparkles className="w-4 h-4 text-sky-400" />
            Novedades de Controller Control Center PS4 v1.3.9 (Payload: ANYCONTROLLER • ID: TONI88888)
          </h3>

          <div className="space-y-3 text-xs text-slate-300 divide-y divide-white/5">
            <div className="pt-2">
              <h4 className="font-bold text-emerald-400">
                1. Respuesta Inmediata del 2.º DualSense y Vinculación Automática a Usuario PS4
              </h4>
              <p className="text-slate-400 mt-0.5">
                Corregida la asignación del 2.º DualSense (<code className="text-sky-300 font-mono">slot &gt;= 1</code>) en modo automático: el demonio vincula automáticamente un segundo perfil registrado de PS4 mediante <code className="text-sky-300 font-mono">sceMbusBindDeviceWithUserId</code>, y cuando solo hay 1 usuario con sesión iniciada redirige de forma transparente las entradas activas a la ranura activa para que el 2.º mando responda desde el primer instante.
              </p>
            </div>

            <div className="pt-3">
              <h4 className="font-bold text-emerald-400">
                2. Detección Instantánea de Desconexión Bluetooth y Cambio Automático PS5 / PS4
              </h4>
              <p className="text-slate-400 mt-0.5">
                Procesamiento inmediato del evento de desconexión <code className="text-sky-300 font-mono">0x19</code> en <code className="text-sky-300 font-mono">/dev/bt</code> y reducción del tiempo de espera de desconexión a 2.5 segundos (antes 30s + 10 min de gracia). Ahora el aviso de desconexión aparece al instante y la aplicación cambia automáticamente entre DualSense PS5 y DualShock 4 manteniendo ambas pestañas accesibles.
              </p>
            </div>

            <div className="pt-3">
              <h4 className="font-bold text-emerald-400">
                3. Corrección Visual de Textos, Cajas de Valor y Botones de Acción
              </h4>
              <p className="text-slate-400 mt-0.5">
                Mejorado el espaciado de glifos FreeType (<code className="text-sky-300 font-mono">advanceX</code> redondeado + tracking), añadidas cajas delimitadas en las tarjetas de DualSense y DualShock 4 sin solapamientos de texto, y corregidos todos los botones (<code className="text-sky-300 font-mono">L2/R2</code> y <code className="text-sky-300 font-mono">Cruceta</code> en Laboratorio, <code className="text-sky-300 font-mono">Triángulo</code> en Sistema/DualSense y <code className="text-sky-300 font-mono">Círculo</code> sin cerrar la app).
              </p>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
