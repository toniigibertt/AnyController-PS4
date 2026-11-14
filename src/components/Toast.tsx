/**
 * OneFamily PS4 - Notificación Emergente Estilo PlayStation 4 (GoldHEN Toast)
 */
import { useEffect } from 'react';
import { CheckCircle2, X } from 'lucide-react';

interface ToastProps {
  message: string;
  onClose: () => void;
  duration?: number;
}

export default function Toast({ message, onClose, duration = 3500 }: ToastProps) {
  useEffect(() => {
    const timer = setTimeout(() => {
      onClose();
    }, duration);
    return () => clearTimeout(timer);
  }, [duration, onClose]);

  return (
    <div className="fixed top-6 right-6 z-50 animate-in fade-in slide-in-from-top-4 duration-200">
      <div className="flex items-center gap-3 px-4 py-3 rounded-2xl bg-[#03152d]/95 border border-sky-400/60 shadow-[0_0_30px_rgba(0,140,255,0.4)] backdrop-blur-xl text-white">
        <div className="p-1.5 rounded-xl bg-emerald-500/20 text-emerald-400 border border-emerald-500/30">
          <CheckCircle2 className="w-5 h-5" />
        </div>
        <div className="pr-2">
          <h4 className="text-xs font-bold text-sky-300 uppercase tracking-wider">
            Notificación GoldHEN
          </h4>
          <p className="text-xs font-medium text-slate-100">{message}</p>
        </div>
        <button
          onClick={onClose}
          className="p-1 rounded-lg text-slate-400 hover:text-white hover:bg-white/10 transition-colors cursor-pointer"
        >
          <X className="w-4 h-4" />
        </button>
      </div>
    </div>
  );
}
