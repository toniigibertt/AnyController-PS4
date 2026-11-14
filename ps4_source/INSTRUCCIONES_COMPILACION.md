# Guía de Compilación de OneFamily PS4 en Español

Esta carpeta contiene todo el código fuente en C++, recursos gráficos, iconos, fuentes y el payload embebido de **OneFamily** traducidos al español y con las correcciones aplicadas.

---

## 📦 Contenido de este Paquete

1. **`app/src/`**: Código fuente C++ de la aplicación compañera a 1080p traducido al español:
   - `ui.cpp`: Menús, interfaz de tarjetas, textos en pantalla y asistente inicial traducidos.
   - `config_manager.cpp`: Opciones de configuración y traducciones de deadzone, gatillos y DualSense.
   - `installer.cpp`: Asistente de instalación automática del payload y configuración para GoldHEN.
   - `main.cpp`: Corrección de errores en la calibración de deriva y mensajes emergentes en español.
2. **`app/assets/payload/onefamily.bin`**: El binario del demonio ya incrustado dentro de la app para que, al abrir el PKG por primera vez en la PS4, se instale automáticamente en `/data/payloads/onefamily.bin` y configure el AutoRun en `/data/GoldHEN/payloads.ini`.
3. **`build_app.ps1`**: Script automático para compilar el paquete PKG en Windows con PowerShell.
4. **`build_app.sh`**: Script automático para compilar el paquete PKG en Linux o WSL.

---

## ⚙️ Requisitos para Compilar el PKG en tu PC

Para compilar el archivo `.pkg` necesitas tener instalado en tu ordenador:

1. **OpenOrbis PS4 Toolchain** (Compilador libre para PlayStation 4):
   - Descárgalo de: [https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
   - Extrae la carpeta (por ejemplo en `C:\OpenOrbis\PS4Toolchain`).
   - Define la variable de entorno `OO_PS4_TOOLCHAIN` apuntando a esa carpeta (o el script te la pedirá al iniciar).
2. **Python 3** (en el PATH del sistema).
3. **.NET Runtime** (para ejecutar `PkgTool.Core` incluido en OpenOrbis).

---

## 🚀 Pasos para Compilar

### En Windows (PowerShell):
1. Abre una ventana de PowerShell en esta carpeta.
2. Ejecuta:
   ```powershell
   ./build_app.ps1
   ```
3. El script compilará automáticamente los archivos `.cpp`, firmará el `eboot.bin` con `create-fself` y empaquetará el PKG final en:
   ```text
   bin/onefamily_companion.pkg
   ```

### En Linux o WSL:
1. Abre una terminal en esta carpeta y da permisos de ejecución:
   ```bash
   chmod +x build_app.sh
   ./build_app.sh
   ```

---

## 🎮 Qué hace el PKG al abrirlo por primera vez en tu PS4

Tal como solicitaste, la aplicación viene configurada exactamente como la original:
1. Al abrir `OneFamily Companion` por primera vez en tu PS4, la app detecta que no está instalada.
2. Copia automáticamente el demonio embebido `onefamily.bin` a la ruta `/data/payloads/onefamily.bin`.
3. Crea el archivo de configuración `/data/onefamily/onefamily.ini` con los ajustes óptimos en español.
4. Registra automáticamente el inicio de OneFamily en `/data/GoldHEN/payloads.ini` bajo `[AutoRun]`.
5. Muestra la pantalla de bienvenida y confirmación en español, lista para usar cualquier mando USB o Bluetooth sin configuración manual.
