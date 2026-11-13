<#
    OneFamily PS4 - Script de Compilación Automática del PKG en Español
    Compatible con OpenOrbis PS4 Toolchain (Windows / PowerShell)
#>

$PROJECT = $PSScriptRoot
$APP_DIR = "$PROJECT/app"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "    OneFamily PS4 - Compilador del Paquete PKG en Español" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Detect OpenOrbis Toolchain
$TOOLCHAIN = $env:OO_PS4_TOOLCHAIN
if (-not $TOOLCHAIN -or -not (Test-Path $TOOLCHAIN)) {
    $searchPaths = @(
        "C:/OpenOrbis/PS4Toolchain",
        "C:/tools/OpenOrbis/PS4Toolchain",
        "D:/OpenOrbis/PS4Toolchain",
        "$env:USERPROFILE/OpenOrbis/PS4Toolchain",
        "$PROJECT/tools/OpenOrbis/PS4Toolchain",
        "$env:USERPROFILE/Downloads/OpenOrbis-PS4-Toolchain"
    )
    foreach ($p in $searchPaths) {
        if (Test-Path $p) {
            $TOOLCHAIN = $p
            break
        }
    }
}

if (-not $TOOLCHAIN -or -not (Test-Path $TOOLCHAIN)) {
    Write-Host ""
    Write-Host "[!] ATENCIÓN: No se detectó OpenOrbis PS4 Toolchain en tu PC." -ForegroundColor Yellow
    Write-Host "Para crear un nuevo archivo .PKG para PS4 se requiere este compilador." -ForegroundColor White
    Write-Host "Enlace de descarga: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "💡 NOTA IMPORTANTE:" -ForegroundColor Green
    Write-Host "Si solo quieres usar la app en tu PS4, NO necesitas compilar nada." -ForegroundColor Green
    Write-Host "Puedes descargar e instalar directamente 'onefamily_companion.pkg'." -ForegroundColor Green
    Write-Host ""
    $TOOLCHAIN = Read-Host "Si ya tienes OpenOrbis descomprimido, introduce su ruta (o pulsa Enter para salir)"
    if (-not $TOOLCHAIN -or -not (Test-Path $TOOLCHAIN)) {
        Write-Host "Compilación detenida. Usa el archivo .pkg oficial descargado." -ForegroundColor Yellow
        exit 0
    }
}

Write-Host "[+] Usando OpenOrbis Toolchain en: $TOOLCHAIN" -ForegroundColor Green

# 2. Check param.sfo (already pre-generated)
Write-Host "`n=== Verificando param.sfo en Español ===" -ForegroundColor Cyan
if (Test-Path "$APP_DIR/sce_sys/param.sfo") {
    Write-Host "[+] param.sfo ya está generado y listo para empaquetar" -ForegroundColor Green
} else {
    Write-Host "[+] Generando param.sfo con Python..." -ForegroundColor Cyan
    $PYTHON = "python"
    try { & python "$APP_DIR/make_sfo_app.py" } catch {
        try { & python3 "$APP_DIR/make_sfo_app.py" } catch {
            Write-Host "[!] No se pudo ejecutar make_sfo_app.py pero continuando..." -ForegroundColor Yellow
        }
    }
}

# 3. Detect create-fself & PkgTool
$CREATE_FSELF = "$TOOLCHAIN/bin/windows/create-fself.exe"
if (-not (Test-Path $CREATE_FSELF)) {
    $CREATE_FSELF = "$TOOLCHAIN/bin/create-fself.exe"
}
if (-not (Test-Path $CREATE_FSELF)) {
    $CREATE_FSELF = "create-fself"
}

$PKGTOOL = "$TOOLCHAIN/bin/windows/PkgTool.Core.dll"
if (-not (Test-Path $PKGTOOL)) {
    $PKGTOOL = "$TOOLCHAIN/bin/PkgTool.Core.dll"
}

$TITLE_ID = "XBOX00001"
$CONTENT_ID = "UP9000-XBOX00001_00-XBOXCONFIGURATOR"

# Create required build output directories
$dirs = @("$APP_DIR/obj", "$APP_DIR/bin", "$PROJECT/bin", "$APP_DIR/assets/payload")
foreach ($d in $dirs) {
    if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d | Out-Null }
}

# 4. Ensure payload is bundled
Write-Host "`n=== Verificando Payload de Demonio (onefamily.bin) ===" -ForegroundColor Cyan
if (-not (Test-Path "$APP_DIR/assets/payload/onefamily.bin")) {
    if (Test-Path "$PROJECT/bin/onefamily.bin") {
        Copy-Item "$PROJECT/bin/onefamily.bin" "$APP_DIR/assets/payload/onefamily.bin" -Force
    } elseif (Test-Path "$PROJECT/app/assets/payload/onefamily.bin") {
        Copy-Item "$PROJECT/app/assets/payload/onefamily.bin" "$APP_DIR/assets/payload/onefamily.bin" -Force
    }
}
Write-Host "[+] onefamily.bin listo en app/assets/payload/onefamily.bin" -ForegroundColor Green

# 6. Compile App Sources
Write-Host "`n=== Compilando Fuentes C++ de OneFamily (en Espanol) ===" -ForegroundColor Cyan
$CXXFLAGS = @(
    "--target=x86_64-pc-freebsd12-elf",
    "-fPIC",
    "-funwind-tables",
    "-fno-exceptions",
    "-fno-rtti",
    "-c",
    "-Wall",
    "-O2",
    "-DGRAPHICS_USES_FONT",
    "-isysroot", $TOOLCHAIN,
    "-isystem", "$TOOLCHAIN/include",
    "-isystem", "$TOOLCHAIN/include/c++/v1",
    "-I$APP_DIR/src",
    "-I$APP_DIR/src/common",
    "-I$PROJECT/include"
)

$srcFiles = @(
    "$APP_DIR/src/main.cpp",
    "$APP_DIR/src/config_manager.cpp",
    "$APP_DIR/src/installer.cpp",
    "$APP_DIR/src/hid_profile.cpp",
    "$APP_DIR/src/pkg_updater.cpp",
    "$APP_DIR/src/ps_background.cpp",
    "$APP_DIR/src/ui.cpp",
    "$APP_DIR/src/common/graphics.cpp",
    "$APP_DIR/src/common/controller.cpp",
    "$APP_DIR/src/common/png.cpp"
)

$objFiles = @()
$CLANGPP = "$TOOLCHAIN/bin/windows/clang++.exe"
if (-not (Test-Path $CLANGPP)) { $CLANGPP = "clang++" }

foreach ($src in $srcFiles) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
    $obj = "$APP_DIR/obj/$name.o"
    $objFiles += $obj
    Write-Host "  [+] Compilando $name.cpp -> $name.o" -ForegroundColor DarkGray
    & $CLANGPP @CXXFLAGS -o $obj $src
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Error compilando $src"
        exit 1
    }
}

# 7. Link ELF
Write-Host "`n=== Enlazando Ejecutable ELF ===" -ForegroundColor Cyan
$LDFLAGS = @(
    "-m", "elf_x86_64",
    "-pie",
    "--script", "$TOOLCHAIN/link.x",
    "--eh-frame-hdr",
    "-L$TOOLCHAIN/lib",
    "$TOOLCHAIN/lib/crt1.o"
)
$LIBS = @(
    "-lc", "-lkernel", "-lc++", "-lSceVideoOut", "-lSceSysmodule",
    "-lSceFreeType", "-lScePad", "-lSceUserService", "-lSceSystemService",
    "-lSceNet", "-lSceNetCtl", "-lSceSsl", "-lSceHttp"
)
$ELF = "$APP_DIR/bin/app.elf"
$OELF = "$APP_DIR/bin/app.oelf"
$EBOOT = "$APP_DIR/eboot.bin"

$LDLLD = "$TOOLCHAIN/bin/windows/ld.lld.exe"
if (-not (Test-Path $LDLLD)) { $LDLLD = "ld.lld" }

& $LDLLD @LDFLAGS @objFiles -o $ELF @LIBS
if ($LASTEXITCODE -ne 0) {
    Write-Error "Fallo al enlazar el archivo ELF"
    exit 1
}

# 8. Create eboot.bin via create-fself
Write-Host "`n=== Firmando eboot.bin con create-fself ===" -ForegroundColor Cyan
& $CREATE_FSELF -in "$ELF" -out "$OELF" -eboot "$EBOOT" -paid 0x3800000000000011
if ($LASTEXITCODE -ne 0) {
    Write-Error "Fallo al crear eboot.bin"
    exit 1
}
Write-Host "[+] eboot.bin creado correctamente" -ForegroundColor Green

# 9. Build PKG
Write-Host "`n=== Empaquetando PKG para PS4 via PkgTool.Core ===" -ForegroundColor Cyan
Push-Location "$APP_DIR"
& dotnet --roll-forward LatestMajor "$PKGTOOL" pkg_build pkg.gp4 .
if ($LASTEXITCODE -ne 0) {
    Write-Error "Fallo al construir el PKG"
    Pop-Location
    exit 1
}

Write-Host "`n=== Validando Integridad del PKG ===" -ForegroundColor Cyan
& dotnet --roll-forward LatestMajor "$PKGTOOL" pkg_validate "$CONTENT_ID.pkg"
Pop-Location

Copy-Item "$APP_DIR/$CONTENT_ID.pkg" "$PROJECT/bin/onefamily_companion.pkg" -Force
Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host " [OK] COMPILACION COMPLETADA CON EXITO!" -ForegroundColor Green
Write-Host " Paquete listo para instalar en tu PS4:" -ForegroundColor Green
Write-Host "   -> $PROJECT/bin/onefamily_companion.pkg" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Green
