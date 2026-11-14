/**
 * OneFamily PS4 - Barra de Control Inferior Estilo PlayStation 4
 * Guía de botones físicos y atajos de teclado para la navegación
 */
export default function FooterControls() {
  const controls = [
    { key: 'L1 / R1', label: 'Cambiar Pestaña' },
    { key: '↑ / ↓', label: 'Navegar Opciones' },
    { key: '← / →', label: 'Modificar Valor' },
    { key: '✕ Cruz / Enter', label: 'Guardar / Seleccionar' },
    { key: '○ Círculo / Esc', label: 'Atrás' },
    { key: 'R3 / Tecla G', label: 'Modo Aislado' },
  ];

  return (
    <footer className="fixed bottom-0 left-0 right-0 z-20 px-6 py-2.5 bg-[#020b18]/90 backdrop-blur-md border-t border-white/10">
      <div className="max-w-7xl mx-auto flex flex-wrap items-center justify-center gap-x-6 gap-y-2 text-xs text-slate-300">
        {controls.map((ctrl, idx) => (
          <div key={idx} className="flex items-center gap-1.5 font-medium">
            <kbd className="px-2 py-0.5 rounded bg-sky-950/80 border border-sky-400/30 text-sky-300 font-mono text-[11px] font-bold shadow-sm">
              {ctrl.key}
            </kbd>
            <span className="text-slate-400">{ctrl.label}</span>
          </div>
        ))}
      </div>
    </footer>
  );
}
