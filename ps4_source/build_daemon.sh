#!/usr/bin/env bash
set -e

export OO_PS4_TOOLCHAIN="/opt/OpenOrbis/PS4Toolchain"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DAEMON_DIR="$SCRIPT_DIR/daemon"
BUILD_DIR="$DAEMON_DIR/obj"
BIN_DIR="$SCRIPT_DIR/bin"
PAYLOAD_ASSET_DIR="$SCRIPT_DIR/app/assets/payload"

mkdir -p "$BUILD_DIR" "$BIN_DIR" "$PAYLOAD_ASSET_DIR"
rm -f "$BUILD_DIR"/*.o

CC="clang"
LD="ld.lld"

CFLAGS=(
    "--target=x86_64-pc-freebsd12-elf"
    "-fPIC"
    "-funwind-tables"
    "-fno-delete-null-pointer-checks"
    "-c"
    "-Wall"
    "-isysroot" "$OO_PS4_TOOLCHAIN"
    "-isystem" "$OO_PS4_TOOLCHAIN/include"
    "-isystem" "$OO_PS4_TOOLCHAIN/include/orbis/_types"
    "-I$DAEMON_DIR/include"
    "-I$SCRIPT_DIR/include"
    "-D__PS4__"
    "-D__ORBIS__"
)

SRCS=(
    "$DAEMON_DIR/src/daemon_syscalls.c"
    "$DAEMON_DIR/src/main.c"
    "$DAEMON_DIR/src/vpad.c"
    "$DAEMON_DIR/src/usb_driver.c"
    "$DAEMON_DIR/src/config.c"
    "$DAEMON_DIR/src/log.c"
    "$DAEMON_DIR/src/controller_detect.c"
    "$DAEMON_DIR/src/translator.c"
    "$DAEMON_DIR/src/generic_hid.c"
    "$DAEMON_DIR/src/bt_driver.c"
)

OBJS=()
echo "[1/3] Compiling AnyController Daemon C sources..."
for src in "${SRCS[@]}"; do
    base="$(basename "${src%.*}")"
    obj="$BUILD_DIR/$base.o"
    echo "  CC $base.c -> daemon/obj/$base.o"
    "$CC" "${CFLAGS[@]}" -o "$obj" "$src"
    OBJS+=("$obj")
done

echo "[2/3] Linking AnyController Daemon ELF..."
"$LD" -m elf_x86_64 -pie \
    --script "$OO_PS4_TOOLCHAIN/link.x" \
    --eh-frame-hdr \
    -L"$OO_PS4_TOOLCHAIN/lib" \
    -o "$BIN_DIR/anycontroller.elf" \
    "${OBJS[@]}" \
    -lc -lkernel

echo "[3/3] Preparing Payload File (anycontroller.bin)..."
cp "$BIN_DIR/anycontroller.elf" "$BIN_DIR/anycontroller.bin"
cp "$BIN_DIR/anycontroller.bin" "$PAYLOAD_ASSET_DIR/anycontroller.bin"
rm -f "$BIN_DIR/onefamily.bin" "$PAYLOAD_ASSET_DIR/onefamily.bin"

echo "SUCCESS: Built $BIN_DIR/anycontroller.bin ($(stat -c%s "$BIN_DIR/anycontroller.bin") bytes)"
