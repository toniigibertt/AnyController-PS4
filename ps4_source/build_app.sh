#!/usr/bin/env bash
# Controller Control Center PS4 (Payload: ANYCONTROLLER) - Compilador PKG (Linux / WSL)
set -e

PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$PROJECT/app"

echo "=========================================================="
echo "  Controller Control Center PS4 - Compilador PKG"
echo "=========================================================="

# 1. Detect OpenOrbis Toolchain
if [ -z "$OO_PS4_TOOLCHAIN" ]; then
    if [ -d "/opt/OpenOrbis/PS4Toolchain" ]; then
        OO_PS4_TOOLCHAIN="/opt/OpenOrbis/PS4Toolchain"
    elif [ -d "$HOME/OpenOrbis/PS4Toolchain" ]; then
        OO_PS4_TOOLCHAIN="$HOME/OpenOrbis/PS4Toolchain"
    else
        echo "[!] Variable OO_PS4_TOOLCHAIN no definida."
        read -p "Introduce la ruta de tu PS4Toolchain: " OO_PS4_TOOLCHAIN
    fi
fi

export OO_PS4_TOOLCHAIN="$OO_PS4_TOOLCHAIN"
export TOOLCHAIN="$OO_PS4_TOOLCHAIN"
echo "[+] Usando OpenOrbis Toolchain en: $TOOLCHAIN"

CONTENT_ID="UP9000-TONI88888_00-ANYCONTROLLERPS4"

mkdir -p "$APP_DIR/obj" "$APP_DIR/bin" "$PROJECT/bin" "$APP_DIR/assets/payload"

# 2. Ensure payload
rm -f "$APP_DIR/assets/payload/onefamily.bin"
if [ -f "$PROJECT/bin/anycontroller.bin" ]; then
    cp "$PROJECT/bin/anycontroller.bin" "$APP_DIR/assets/payload/anycontroller.bin"
fi

# 3. Generate param.sfo
echo "[+] Generando param.sfo en Español..."
python3 "$APP_DIR/make_sfo_app.py"

# 4. Compile C++ sources
CXXFLAGS=(
    "--target=x86_64-pc-freebsd12-elf"
    "-fPIC"
    "-funwind-tables"
    "-fno-exceptions"
    "-fno-rtti"
    "-c"
    "-Wall"
    "-O2"
    "-DGRAPHICS_USES_FONT"
    "-isysroot" "$TOOLCHAIN"
    "-isystem" "$TOOLCHAIN/include"
    "-isystem" "$TOOLCHAIN/include/c++/v1"
    "-I$APP_DIR/src"
    "-I$APP_DIR/src/common"
    "-I$PROJECT/include"
)

srcFiles=(
    "$APP_DIR/src/main.cpp"
    "$APP_DIR/src/config_manager.cpp"
    "$APP_DIR/src/installer.cpp"
    "$APP_DIR/src/hid_profile.cpp"
    "$APP_DIR/src/pkg_updater.cpp"
    "$APP_DIR/src/ps_background.cpp"
    "$APP_DIR/src/ui.cpp"
    "$APP_DIR/src/common/graphics.cpp"
    "$APP_DIR/src/common/controller.cpp"
    "$APP_DIR/src/common/png.cpp"
)

objFiles=()
CLANGPP="$TOOLCHAIN/bin/linux/clang++"
if [ ! -f "$CLANGPP" ]; then CLANGPP="clang++"; fi

for src in "${srcFiles[@]}"; do
    filename=$(basename -- "$src")
    name="${filename%.*}"
    obj="$APP_DIR/obj/$name.o"
    objFiles+=("$obj")
    echo "  [+] Compilando $name.cpp..."
    "$CLANGPP" "${CXXFLAGS[@]}" -o "$obj" "$src"
done

# 5. Link ELF
echo "[+] Enlazando ELF..."
LDFLAGS=(
    "-m" "elf_x86_64"
    "-pie"
    "--script" "$TOOLCHAIN/link.x"
    "--eh-frame-hdr"
    "-L$TOOLCHAIN/lib"
    "$TOOLCHAIN/lib/crt1.o"
)
LIBS=(
    "-lc" "-lkernel" "-lc++" "-lSceVideoOut" "-lSceSysmodule"
    "-lSceFreeType" "-lScePad" "-lSceUserService" "-lSceSystemService"
    "-lSceNet" "-lSceNetCtl" "-lSceSsl" "-lSceHttp"
)

ELF="$APP_DIR/bin/app.elf"
OELF="$APP_DIR/bin/app.oelf"
EBOOT="$APP_DIR/eboot.bin"

LDLLD="$TOOLCHAIN/bin/linux/ld.lld"
if [ ! -f "$LDLLD" ]; then LDLLD="ld.lld"; fi

"$LDLLD" "${LDFLAGS[@]}" "${objFiles[@]}" -o "$ELF" "${LIBS[@]}"

# 6. Create fself
CREATE_FSELF="$TOOLCHAIN/bin/linux/create-fself"
if [ ! -f "$CREATE_FSELF" ]; then CREATE_FSELF="create-fself"; fi
"$CREATE_FSELF" -in "$ELF" -out "$OELF" -eboot "$EBOOT" -paid 0x3800000000000011

# 7. Build versioned PKG
PKGTOOL="$TOOLCHAIN/bin/linux/PkgTool.Core"
if [ ! -f "$PKGTOOL" ]; then PKGTOOL="$TOOLCHAIN/bin/PkgTool.Core.dll"; fi

if [ -d "/opt/OpenOrbis/libssl11/usr/lib/x86_64-linux-gnu" ]; then
    export LD_LIBRARY_PATH="/opt/OpenOrbis/libssl11/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
fi

VERSION_STRING=$(grep -E '^VERSION_STRING=' "$PROJECT/version.properties" | cut -d'=' -f2 | tr -d '[:space:]')
if [ -z "$VERSION_STRING" ]; then VERSION_STRING="v1.3.8"; fi
VERSIONED_PKG_NAME="Controller_Control_Center_PS4_${VERSION_STRING}.pkg"

cd "$APP_DIR"
if [[ "$PKGTOOL" == *.dll ]]; then
    dotnet --roll-forward LatestMajor "$PKGTOOL" pkg_build pkg.gp4 .
else
    "$PKGTOOL" pkg_build pkg.gp4 .
fi

rm -f "$PROJECT/bin/onefamily_companion.pkg" "$PROJECT/bin/anycontroller_companion.pkg" "$PROJECT/bin/anycontroller_ps4.pkg"
cp "$APP_DIR/$CONTENT_ID.pkg" "$PROJECT/bin/$VERSIONED_PKG_NAME"
echo "=========================================================="
echo " [OK] COMPILACIÓN COMPLETADA CON ÉXITO!"
echo " Paquete Controller Control Center PS4 ($VERSION_STRING):"
echo "   -> $PROJECT/bin/$VERSIONED_PKG_NAME"
echo " Payload ANYCONTROLLER:"
echo "   -> $PROJECT/bin/anycontroller.bin"
echo "=========================================================="
