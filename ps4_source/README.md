# 🎮 Controller Control Center PS4 (Código Fuente Nativo PS4)

Código fuente nativo en **C / C++ (OpenOrbis PS4 Toolchain)** del payload **`ANYCONTROLLER`** (`anycontroller.bin`) y la aplicación Homebrew **`Controller Control Center PS4`** (`Controller_Control_Center_PS4_v1.3.8.pkg`, Title ID `TONI88888`).

---

## ✨ Características Clave (`v1.3.8`)

- **Demonio `ANYCONTROLLER` (`ps4_source/daemon/`)**:
  - Sondeo USB y Bluetooth (`/dev/bt`) a **250 Hz (4 ms)**.
  - Soporte simultáneo **Multi-DualSense BT (1 a 4 ranuras)** con apertura limpia del selector nativo de perfil PS4 (*"¿Quién está usando este mando?"*).
  - Bucle BT optimizado anti-congelamiento, soporte modo Seguro (`0x01`) y Extendido (`0x31` 78 bytes), giroscopio/acelerómetro, touchpad, vibración háptica y gatillos adaptativos.
- **Aplicación PS4 (`ps4_source/app/`)**:
  - Interfaz nativa **1080p / 60 FPS** en español con pestañas dinámicas según el mando activo (**DualSense PS5** o **DualShock 4**).
  - Control de luz LED, brillo, apagado completo en DS4, calibración de drift en vivo, probador con Touchpad/Cruceta y editor de perfiles HID.
  - Actualizador en línea conectado a **GitHub Releases (`toniigibertt/AnyController-PS4`)** que guarda paquetes versionados en `/data/pkg/Controller_Control_Center_PS4_<versión>.pkg`.

---

## 🛠️ Compilación en Linux / WSL

```bash
# 1. Compilar el demonio ANYCONTROLLER (anycontroller.bin)
bash build_daemon.sh

# 2. Compilar la app y generar el PKG con versión (Controller_Control_Center_PS4_v1.3.8.pkg)
bash build_app.sh
```

El script `build_app.sh` lee automáticamente `VERSION_STRING` desde `version.properties` y genera el `.pkg` versionado dentro de `bin/`.

---

## 📁 Rutas en la Consola PS4

| Archivo / Directorio | Descripción |
| :--- | :--- |
| `/data/payloads/anycontroller.bin` | Payload del demonio **ANYCONTROLLER** cargado por GoldHEN |
| `/data/anycontroller/anycontroller.ini` | Archivo principal de configuración |
| `/data/anycontroller/controllers/` | Perfiles `.ini` por dispositivo USB HID (`VID_PID.ini`) |
| `/data/GoldHEN/payloads.ini` | Registro `[AutoRun]` de GoldHEN |
| `/data/pkg/Controller_Control_Center_PS4_*.pkg` | Paquetes descargados por el actualizador desde GitHub |
