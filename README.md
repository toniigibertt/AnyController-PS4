# 🎮 Controller Control Center PS4

<p align="center">
  <img src="ps4_source/app/sce_sys/icon0.png" alt="Controller Control Center PS4 Logo" width="220"/>
</p>

<h2 align="center">Centro de Control de Mandos PS5 DualSense, DualShock 4 y USB/BT HID para PlayStation 4</h2>

<p align="center">
  <b>Aplicación Homebrew PS4 (Title ID: <code>TONI88888</code>) + Demonio/Payload <code>ANYCONTROLLER</code> (<code>anycontroller.bin</code>)</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Versi%C3%B3n-v1.5.3%20(01.53)-00AFFF?style=for-the-badge" alt="Versión v1.5.3"/>
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

## ✨ Novedades Destacadas (`v1.5.3`)

- **Aislamiento Real DualSense 1 + 2 (Sin "Conexiones a la Nada"):**
  - El bucle de barra LED DualShock 4 ya no interfiere ni abre handles consumidores durante el emparejamiento/asignación de mandos virtuales DualSense (verificación por `Device ID` + `bound_user_id` y retorno temprano sin tocar `libScePad` cuando los DS4 están de fábrica).
  - Eliminada la doble pulsación PS colisionada al vincular usuario, añadida ventana de estabilización limpia de `280 ms` al encender el DualSense con PS pulsado, y reducción de uso de pila en `daemon_config_load_device()`.
- **Colores y Ajustes de DualShock 4 Persistentes + Apagado Nativo DS4:**
  - Colores RGB, brillo y vibración del DualShock 4 independientes por ranura (`ds4_slot_<1..4>.ini`), memoria persistente por MAC de DualSense (`ds_mac_<MAC>.ini`), visor 2D original PS5/PS4 y TV Safe Area 1080p sin solapamientos.

---

## 📦 Instalación en PlayStation 4 (GoldHEN)

1. Descarga **`Controller_Control_Center_PS4_v1.5.3.pkg`** desde [**Releases**](https://github.com/toniigibertt/AnyController-PS4/releases).
2. Copia el `.pkg` a la raíz de un USB (exFAT / FAT32) o envíalo por FTP a `/data/pkg/`.
3. En tu PS4, abre **Ajustes → GoldHEN → Package Installer** e instala **`Controller_Control_Center_PS4_v1.5.3.pkg`**.
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

# 2. Compilar la app y empaquetar el PKG versionado (Controller_Control_Center_PS4_v1.5.3.pkg)
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
