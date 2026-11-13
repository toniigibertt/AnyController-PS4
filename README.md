# 🎮 Controller Control Center PS4

<p align="center">
  <img src="ps4_source/app/sce_sys/icon0.png" alt="Controller Control Center PS4 Logo" width="220"/>
</p>

<h2 align="center">Centro de Control de Mandos PS5 DualSense, DualShock 4 y USB/BT HID para PlayStation 4</h2>

<p align="center">
  <b>Aplicación Homebrew PS4 (Title ID: <code>TONI88888</code>) + Demonio/Payload <code>ANYCONTROLLER</code> (<code>anycontroller.bin</code>)</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Versi%C3%B3n-v1.3.8%20(01.38)-00AFFF?style=for-the-badge" alt="Versión v1.3.8"/>
  <img src="https://img.shields.io/badge/Payload-ANYCONTROLLER-6366F1?style=for-the-badge" alt="Payload ANYCONTROLLER"/>
  <img src="https://img.shields.io/badge/Title%20ID-TONI88888-0EA5E9?style=for-the-badge" alt="Title ID TONI88888"/>
  <img src="https://img.shields.io/badge/GoldHEN-%E2%89%A5%202.4b18.10-0070D1?style=for-the-badge&logo=playstation" alt="GoldHEN"/>
  <img src="https://img.shields.io/badge/Sondeo-250%20Hz%20(4ms)-10B981?style=for-the-badge" alt="250 Hz"/>
</p>

---

## 📋 Descripción General

**Controller Control Center PS4** es una suite Homebrew a **1080p / 60 FPS** para consolas **PlayStation 4** con **GoldHEN**, impulsada en segundo plano por el demonio **`ANYCONTROLLER`** (`/data/payloads/anycontroller.bin`).

Permite conectar y personalizar mandos **Sony DualSense / DualSense Edge (PS5)** por Bluetooth nativo o USB, controlar características avanzadas del **DualShock 4 (PS4)** y utilizar mandos de **Xbox, Nintendo Switch y periféricos USB HID genéricos** en todo el sistema PS4 y en cualquier juego con **sondeo a 250 Hz (4 ms)**.

---

## ✨ Características Principales (`v1.3.8`)

- **Multijugador Bluetooth Multi-DualSense (1 a 4 Mandos):**
  - Soporte simultáneo para múltiples mandos PS5 DualSense conectados por Bluetooth sin bloquearse ni desconectarse entre sí.
  - Apertura automática y manual del selector de perfil nativo de PS4 (*"¿Quién está usando este mando?"*) para asignar cada DualSense al Jugador 1, 2, 3 o 4.
  - Arquitectura anti-congelamiento en el bucle Bluetooth (`/dev/bt`) con lectura por lotes, deduplicación por MAC, backoff inteligente y sin escrituras bloqueantes innecesarias en disco.
- **Personalización Exclusiva DualSense (PS5):**
  - Barra LED RGB (Sincronizar con juego, Color estático, Nivel de batería, Apagado, Arcoíris dinámico, Pulso respiración) y brillo configurable.
  - LEDs de jugador, presets de gatillos adaptativos (`L2`/`R2`), botón **Mute** reasignable a Clic de Touchpad y volumen de altavoz.
  - Modo Bluetooth configurable: **Seguro (10B)** o **Extendido (78B)** con giroscopio, acelerómetro y panel táctil.
- **Personalización Nativa DualShock 4 (PS4):**
  - Control de color, brillo por hardware, apagado completo del LED (`0%`), potencia de vibración, modo touchpad y filtro de giroscopio.
- **Ajustes Globales y Laboratorio de Mandos:**
  - Perfiles Pro instantáneos (`eSports FPS`, `SimRacing`, `Modo Eco`, `Estándar`), zona muerta anti-drift, curvas de sticks, umbral de gatillos, turbo/fuego rápido e intercambio `L1/L2` y `R1/R2`.
  - Probador en tiempo real (sticks, gatillos, cruceta, touchpad, vibración y calibración automática de deriva) y editor de mapeo USB HID.
- **Actualizador Online Integrado (GitHub Releases):**
  - Comprueba nuevas versiones estables desde la propia PS4 y descarga los paquetes versionados (`Controller_Control_Center_PS4_vX.Y.Z.pkg`) en `/data/pkg/`.

---

## 📦 Instalación en PlayStation 4 (GoldHEN)

1. Descarga **`Controller_Control_Center_PS4_v1.3.8.pkg`** (o el paquete completo **`Controller_Control_Center_PS4_Pack_v1.3.8.zip`**) desde [**Releases**](https://github.com/toniigibertt/AnyController-PS4/releases).
2. Copia el `.pkg` a la raíz de un USB (exFAT / FAT32) o envíalo por FTP a `/data/pkg/`.
3. En tu PS4, abre **Ajustes → GoldHEN → Package Installer** e instala **`Controller_Control_Center_PS4_v1.3.8.pkg`**.
4. Inicia **Controller Control Center PS4** desde el menú XMB. En el primer arranque se configurará automáticamente:
   - Payload **`ANYCONTROLLER`** en `/data/payloads/anycontroller.bin`
   - Configuración en `/data/anycontroller/anycontroller.ini`
   - Arranque automático en `/data/GoldHEN/payloads.ini` bajo `[AutoRun]`
5. Verifica en tu PS4 que **GoldHEN → PayLoader → Enable AutoRun** esté activado y reinicia o ejecuta el payload para activar el demonio en segundo plano.

---

## 📶 Emparejamiento Bluetooth de Mandos DualSense (PS5)

1. Con el DualSense apagado, mantén pulsados **Create (Share) + PS** durante 3 segundos hasta que parpadee rápidamente en azul.
2. En PS4 ve a **Ajustes → Dispositivos → Dispositivos Bluetooth** y selecciona **Wireless Controller**.
3. Al encenderse o registrarse en PS4, elige el perfil de usuario deseado en pantalla (o pulsa **Create + Cruceta Izquierda** / **[△]** en la pestaña DualSense para abrir de nuevo el selector de perfil).

---

## 🛠️ Compilación desde el Código Fuente

### Requisitos (Linux / WSL)
- **OpenOrbis PS4 Toolchain** (`OO_PS4_TOOLCHAIN` en `/opt/OpenOrbis/PS4Toolchain`)
- `clang`, `clang++`, `ld.lld`, `python3`

### Pasos de compilación
```bash
# 1. Compilar el demonio ANYCONTROLLER (genera ps4_source/bin/anycontroller.bin)
bash ps4_source/build_daemon.sh

# 2. Compilar la app y empaquetar el PKG versionado (Controller_Control_Center_PS4_v1.3.8.pkg)
bash ps4_source/build_app.sh
```

### Aplicación Web Companion (Configurador .INI y descargas)
```bash
npm install
npm run dev
```

---

## 📁 Estructura del Proyecto y Rutas en PS4

| Ruta en el Repositorio | Descripción |
| :--- | :--- |
| `ps4_source/daemon/` | Código fuente en C del payload/demonio **`ANYCONTROLLER`** |
| `ps4_source/app/` | Código fuente en C++ de **Controller Control Center PS4** y recursos PKG |
| `ps4_source/version.properties` | Versión centralizada del proyecto (`VERSION_STRING`, `SFO_VERSION`, `TITLE_ID`) |
| `src/` | Aplicación web en React + TypeScript + Tailwind CSS |

| Ruta en la Consola PS4 | Descripción |
| :--- | :--- |
| `/data/payloads/anycontroller.bin` | Payload del demonio **ANYCONTROLLER** cargado por GoldHEN |
| `/data/anycontroller/anycontroller.ini` | Configuración principal sincronizada en tiempo real |
| `/data/anycontroller/controllers/` | Perfiles `.ini` personalizados por dispositivo USB HID (`VID_PID.ini`) |
| `/data/GoldHEN/payloads.ini` | Registro `[AutoRun]` de GoldHEN |
| `/data/pkg/Controller_Control_Center_PS4_*.pkg` | Paquetes descargados por el actualizador integrado |
