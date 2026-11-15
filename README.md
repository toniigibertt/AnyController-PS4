# 🎮 Controller Control Center PS4

<p align="center">
  <img src="ps4_source/app/sce_sys/icon0.png" alt="Controller Control Center PS4 Logo" width="220"/>
</p>

<h2 align="center">Centro de Control de Mandos PS5 DualSense, DualShock 4 y USB/BT HID para PlayStation 4</h2>

<p align="center">
  <b>Aplicación Homebrew PS4 (Title ID: <code>TONI88888</code>) + Demonio/Payload <code>ANYCONTROLLER</code> (<code>anycontroller.bin</code>)</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Versi%C3%B3n-v1.5.1%20(01.51)-00AFFF?style=for-the-badge" alt="Versión v1.5.1"/>
  <img src="https://img.shields.io/badge/Payload-ANYCONTROLLER-6366F1?style=for-the-badge" alt="Payload ANYCONTROLLER"/>
  <img src="https://img.shields.io/badge/Title%20ID-TONI88888-0EA5E9?style=for-the-badge" alt="Title ID TONI88888"/>
  <img src="https://img.shields.io/badge/GoldHEN-%E2%89%A5%202.4b18.10-0070D1?style=for-the-badge&logo=playstation" alt="GoldHEN"/>
  <img src="https://img.shields.io/badge/Sondeo-250%20Hz%20(4ms)-10B981?style=for-the-badge" alt="250 Hz"/>
</p>

---

## 📋 Descripción General

**Controller Control Center PS4** es una suite Homebrew a **1080p / 60 FPS** para consolas **PlayStation 4** con **GoldHEN**, impulsada en segundo plano por el demonio **`ANYCONTROLLER`** (`/data/payloads/anycontroller.bin`).

Permite conectar y personalizar múltiples mandos **Sony DualSense / DualSense Edge (PS5)** por Bluetooth nativo o USB, ajustar mandos **DualShock 4 (PS4)** y usar mandos **Xbox, Nintendo Switch y genéricos USB HID** con **sondeo a 250 Hz (4 ms)**.

---

## ✨ Novedades Destacadas (`v1.5.1`)

- **Conexión Directa del 1.er DualSense sin Conflictos:**
  - Corregido el bloqueo del primer DualSense en modo `AUTO`: ahora desconecta y libera el DualShock 4 físico activo y vincula el mando virtual correctamente al perfil del Jugador 1 desde el primer instante.
- **Colores y Ajustes de DualShock 4 en Tiempo Real:**
  - El demonio cede automáticamente el control de la barra LED mientras editas en la app y aplica color, modo RGB, vibración extra y bloqueo táctil en los DualShock 4 al guardar y salir.
- **Encuadre TV Safe Area 100% Visible:**
  - Reajustada toda la interfaz 1080p dentro de los márgenes de overscan para televisor (`X: 96–1824`, `Y: 72–996`), sin elementos cortados arriba ni en los bordes.
- **Botones Sin Conflictos ni Acciones Dobles + Mandos 2D Mejorados:**
  - Eliminadas funciones solapadas en Modo Prueba, pestaña DualSense y barra inferior, y mejorados los trazos, textura micro-punto del touchpad y detalle de las siluetas 2D de **PS5 DualSense** y **PS4 DualShock 4**.
- **Memoria Independiente por Mando (`v1.5.x`):**
  - Cada DualSense guarda su perfil por **dirección MAC** (`ds_mac_<MAC>.ini`) y cada DualShock 4 por **ranura** (`ds4_slot_<1..4>.ini`), además de la plantilla Global (`anycontroller.ini`).

---

## 📦 Instalación en PlayStation 4 (GoldHEN)

1. Descarga **`Controller_Control_Center_PS4_v1.5.1.pkg`** desde [**Releases**](https://github.com/toniigibertt/AnyController-PS4/releases).
2. Copia el `.pkg` a la raíz de un USB (exFAT / FAT32) o envíalo por FTP a `/data/pkg/`.
3. En tu PS4, abre **Ajustes → GoldHEN → Package Installer** e instala **`Controller_Control_Center_PS4_v1.5.1.pkg`**.
4. Inicia **Controller Control Center PS4** desde el menú XMB. En el primer arranque se configurará automáticamente:
   - Payload **`ANYCONTROLLER`** en `/data/payloads/anycontroller.bin`
   - Configuración Global en `/data/anycontroller/anycontroller.ini`
   - Perfiles por mando en `/data/anycontroller/controllers/`
   - Arranque automático en `/data/GoldHEN/payloads.ini` bajo `[AutoRun]`

---

## 🛠️ Compilación desde el Código Fuente

### Requisitos (Linux / WSL)
- **OpenOrbis PS4 Toolchain** (`OO_PS4_TOOLCHAIN` en `/opt/OpenOrbis/PS4Toolchain`)
- `clang`, `clang++`, `ld.lld`, `python3`

### Pasos de compilación
```bash
# 1. Compilar el demonio ANYCONTROLLER (genera ps4_source/bin/anycontroller.bin)
bash ps4_source/build_daemon.sh

# 2. Compilar la app y empaquetar el PKG versionado (Controller_Control_Center_PS4_v1.5.1.pkg)
bash ps4_source/build_app.sh
```

### Aplicación Web Companion (Configurador .INI y descargas)
```bash
npm install
npm run dev
```

---

## 📁 Rutas en la Consola PS4

| Ruta en la Consola PS4 | Descripción |
| :--- | :--- |
| `/data/payloads/anycontroller.bin` | Payload del demonio **ANYCONTROLLER** cargado por GoldHEN |
| `/data/anycontroller/anycontroller.ini` | Configuración Global predeterminada |
| `/data/anycontroller/controllers/ds_mac_<MAC>.ini` | Memoria persistente independiente por dirección MAC de DualSense |
| `/data/anycontroller/controllers/ds4_slot_<1..4>.ini` | Configuración independiente por ranura de DualShock 4 |
| `/data/anycontroller/controllers/<VID>_<PID>.ini` | Mapeo personalizado de botones USB HID |
| `/data/GoldHEN/payloads.ini` | Registro `[AutoRun]` de GoldHEN |
| `/data/pkg/Controller_Control_Center_PS4_*.pkg` | Paquetes descargados por el actualizador integrado |
