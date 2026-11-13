/**
 * OneFamily PS4 - Fondo Animado Procedural Estilo PS4 XMB
 * Recreación en Canvas de las ondas de PlayStation 4 y partículas flotantes (ps_background.cpp)
 */
import { useEffect, useRef } from 'react';

export default function PS4Background() {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let animId: number;
    let width = (canvas.width = window.innerWidth);
    let height = (canvas.height = window.innerHeight);

    const handleResize = () => {
      if (!canvas) return;
      width = canvas.width = window.innerWidth;
      height = canvas.height = window.innerHeight;
    };
    window.addEventListener('resize', handleResize);

    // Particle nodes
    interface Particle {
      x: number;
      y: number;
      radius: number;
      alpha: number;
      vx: number;
      vy: number;
      shape: number; // 0=dot, 1=cross, 2=circle
    }

    const particles: Particle[] = Array.from({ length: 45 }, () => ({
      x: Math.random() * width,
      y: Math.random() * height,
      radius: 1.5 + Math.random() * 2.5,
      alpha: 0.15 + Math.random() * 0.45,
      vx: (Math.random() - 0.5) * 0.35,
      vy: -0.2 - Math.random() * 0.4,
      shape: Math.floor(Math.random() * 3),
    }));

    let frame = 0;

    const render = () => {
      frame += 0.012;

      // Deep PS4 background gradient
      const bgGrad = ctx.createLinearGradient(0, 0, width, height);
      bgGrad.addColorStop(0, '#020b18');
      bgGrad.addColorStop(0.4, '#041733');
      bgGrad.addColorStop(0.8, '#062550');
      bgGrad.addColorStop(1, '#020e20');
      ctx.fillStyle = bgGrad;
      ctx.fillRect(0, 0, width, height);

      // Draw flowing sine waves (PlayStation XMB Ribbon Wave)
      for (let w = 0; w < 3; w++) {
        ctx.save();
        ctx.beginPath();
        const baseOffset = height * (0.65 + w * 0.08);
        const freq = 0.0018 + w * 0.0007;
        const amp = 45 + w * 25;
        const speed = frame * (1.2 + w * 0.4);

        ctx.moveTo(0, height);
        for (let x = 0; x <= width; x += 12) {
          const y = baseOffset + Math.sin(x * freq + speed) * amp + Math.cos(x * freq * 0.6 - speed * 0.5) * (amp * 0.4);
          if (x === 0) ctx.lineTo(0, y);
          else ctx.lineTo(x, y);
        }
        ctx.lineTo(width, height);
        ctx.closePath();

        const waveGrad = ctx.createLinearGradient(0, baseOffset - amp, 0, height);
        if (w === 0) {
          waveGrad.addColorStop(0, 'rgba(0, 112, 209, 0.18)');
          waveGrad.addColorStop(0.5, 'rgba(0, 80, 170, 0.08)');
          waveGrad.addColorStop(1, 'rgba(0, 20, 50, 0)');
        } else if (w === 1) {
          waveGrad.addColorStop(0, 'rgba(0, 168, 255, 0.12)');
          waveGrad.addColorStop(0.6, 'rgba(0, 112, 209, 0.05)');
          waveGrad.addColorStop(1, 'rgba(0, 15, 40, 0)');
        } else {
          waveGrad.addColorStop(0, 'rgba(70, 130, 240, 0.08)');
          waveGrad.addColorStop(1, 'rgba(0, 10, 30, 0)');
        }

        ctx.fillStyle = waveGrad;
        ctx.fill();

        // Wave crest highlight line
        ctx.beginPath();
        for (let x = 0; x <= width; x += 12) {
          const y = baseOffset + Math.sin(x * freq + speed) * amp + Math.cos(x * freq * 0.6 - speed * 0.5) * (amp * 0.4);
          if (x === 0) ctx.moveTo(0, y);
          else ctx.lineTo(x, y);
        }
        ctx.strokeStyle = w === 1 ? 'rgba(0, 220, 255, 0.28)' : 'rgba(0, 140, 255, 0.2)';
        ctx.lineWidth = w === 1 ? 1.5 : 1;
        ctx.stroke();
        ctx.restore();
      }

      // Draw floating PlayStation glowing bokeh particles
      for (const p of particles) {
        p.x += p.vx;
        p.y += p.vy;
        if (p.y < -20) {
          p.y = height + 20;
          p.x = Math.random() * width;
        }
        if (p.x < -20) p.x = width + 20;
        if (p.x > width + 20) p.x = -20;

        ctx.save();
        ctx.globalAlpha = p.alpha;
        if (p.shape === 0) {
          // Circular particle
          ctx.beginPath();
          ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2);
          ctx.fillStyle = '#00D4FF';
          ctx.shadowColor = '#00B4D8';
          ctx.shadowBlur = 8;
          ctx.fill();
        } else if (p.shape === 1) {
          // Cross symbol
          ctx.strokeStyle = '#60A5FA';
          ctx.lineWidth = 1.2;
          ctx.beginPath();
          ctx.moveTo(p.x - 3, p.y - 3);
          ctx.lineTo(p.x + 3, p.y + 3);
          ctx.moveTo(p.x + 3, p.y - 3);
          ctx.lineTo(p.x - 3, p.y + 3);
          ctx.stroke();
        } else {
          // Circle symbol
          ctx.strokeStyle = '#F87171';
          ctx.lineWidth = 1.2;
          ctx.beginPath();
          ctx.arc(p.x, p.y, 3, 0, Math.PI * 2);
          ctx.stroke();
        }
        ctx.restore();
      }

      animId = requestAnimationFrame(render);
    };

    animId = requestAnimationFrame(render);
    return () => {
      cancelAnimationFrame(animId);
      window.removeEventListener('resize', handleResize);
    };
  }, []);

  return <canvas ref={canvasRef} className="fixed inset-0 pointer-events-none z-0" />;
}
