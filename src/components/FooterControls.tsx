/**
 * Controller Control Center PS4 - Barra de Control Inferior (v1.5.0)
 * Guía limpia y sin solapamientos de atajos en Web y PS4.
 */
export default function FooterControls() {
  const controls = [
    { key: 'Q / E · L1 / R1', label: 'Cambiar Pestaña' },
    { key: '↑ / ↓', label: 'Navegar' },
    { key: 'L3 (PS4)', label: 'Cambiar de Mando' },
    { key: 'R3 (PS4)', label: 'Restaurar a Global' },
    { key: '1 - 6', label: 'Salto Directo' },
  ];

  return (
    <footer className="fixed bottom-0 left-0 right-0 z-20 px-6 py-2.5 bg-[#020b18]/95 backdrop-blur-md border-t border-slate-800">
      <div className="max-w-7xl mx-auto flex flex-wrap items-center justify-center gap-x-6 gap-y-1.5 text-xs text-slate-300">
        {controls.map((ctrl, idx) => (
          <div key={idx} className="flex items-center gap-1.5 font-medium whitespace-nowrap">
            <kbd className="px-2 py-0.5 rounded bg-slate-900 border border-slate-700 text-cyan-300 font-mono text-[11px] font-bold">
              {ctrl.key}
            </kbd>
            <span className="text-slate-400">{ctrl.label}</span>
          </div>
        ))}
      </div>
    </footer>
  );
}
