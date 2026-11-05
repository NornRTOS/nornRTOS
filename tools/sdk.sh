#!/bin/bash
# Builds the binary SDK of nornRTOS from this tree into build/sdk: the libraries of every
# board and profile (nornCore, nornNET, nornWIFI where offered), the public headers, the
# profiles, the linker scripts, Espressif's radio libraries with their licences, the
# build fragment norn.mk with its module fragments, the tools, the examples, the licence
# and a MANIFEST. The result is what the public nornSDK repository publishes at a
# release: `make sdk`, then copy build/sdk/ into that repository and tag it.
set -eu
cd "$(dirname "$0")/.."
ROOT=$PWD
SDK=$ROOT/build/sdk
CORE=$ROOT/modules/nornCore
NET=$ROOT/modules/nornNET
WIFI=$ROOT/modules/nornWIFI
BOARDS="esp32 mega2560 uno"
PROFILES="small default large"
rm -rf "$SDK"; mkdir -p "$SDK"

offered() {   # profile config file, board -> 0 if the board section is not an #error
    local f=$1 b=$2
    case $b in esp32) d=-D__XTENSA__;; mega2560) d=-D__AVR_ATmega2560__;; uno) d=-D__AVR_ATmega328P__;; esac
    gcc -E $d -x c "$f" > /dev/null 2>&1
}

echo "== libraries"
for b in $BOARDS; do
    for p in $PROFILES; do
        offered profiles/$p/rtos_config.h $b || { echo "  $b/$p: not offered"; continue; }
        make -s -C "$CORE" lib BOARD=$b PROFILE=$p PROFILE_DIR="$ROOT/profiles/$p" NORN_RTOS="$ROOT" > /dev/null
        mkdir -p "$SDK/lib/$b/$p"
        cp "$CORE/build/lib-$p-$b/libnornCore.a" "$SDK/lib/$b/$p/"
        line="  lib/$b/$p: nornCore"
        if offered profiles/$p/nn_config.h $b; then
            make -s -C "$NET" lib BOARD=$b PROFILE=$p PROFILE_DIR="$ROOT/profiles/$p" NORN_RTOS="$ROOT" > /dev/null
            cp "$CORE/build/lib-$p-$b/libnornNET.a" "$SDK/lib/$b/$p/"; line="$line nornNET"
        fi
        if [ $b = esp32 ] && offered profiles/$p/nw_config.h $b; then
            make -s -C "$WIFI" lib PROFILE=$p PROFILE_DIR="$ROOT/profiles/$p" NORN_RTOS="$ROOT" > /dev/null
            cp "$CORE/build/lib-$p-$b/libnornWIFI.a" "$CORE/build/lib-$p-$b/mod/libnwsupp.a" "$SDK/lib/$b/$p/"; line="$line nornWIFI"
        fi
        echo "$line"
    done
done
mkdir -p "$SDK/lib/esp32/espressif"
cp "$WIFI"/lib/esp32/*.a "$WIFI"/lib/esp32/VERSION "$WIFI"/lib/LICENSE.* "$SDK/lib/esp32/espressif/"

echo "== headers and profiles"
I=$SDK/include
mkdir -p "$I/nornCore/port/avr" "$I/nornCore/port/xtensa" "$I/nornCore/hal/avr" "$I/nornCore/hal/xtensa" "$I/nornNET" "$I/nornWIFI"
cp "$CORE"/kernel/rtos.h "$CORE"/kernel/rtos_types.h "$I/nornCore/"
cp "$CORE"/port/avr/port.h "$I/nornCore/port/avr/"
cp "$CORE"/port/xtensa/port.h "$CORE"/port/xtensa/port_regs.h "$I/nornCore/port/xtensa/"
cp "$CORE"/hal/*.h "$I/nornCore/hal/"; cp "$CORE"/hal/avr/*.h "$I/nornCore/hal/avr/"; cp "$CORE"/hal/xtensa/*.h "$I/nornCore/hal/xtensa/"
cp "$NET"/nn/*.h "$NET"/drv/nn_drv_loop.h "$NET"/drv/nn_drv_slip.h "$NET"/drv/nn_drv_openeth.h "$NET"/os/nn_os.h "$NET"/os/nn_os_nornos.h "$I/nornNET/"
cp "$WIFI"/nw/nw.h "$WIFI"/nw/nw_config.h "$I/nornWIFI/"
cp -r "$WIFI"/include "$I/nornWIFI/esp"
cp -r profiles "$SDK/profiles"

echo "== linker scripts, build fragments, tools, examples, licence"
mkdir -p "$SDK/linker/esp32" "$SDK/modules" "$SDK/tools" "$SDK/examples"
cp "$CORE"/port/xtensa/*.ld "$WIFI"/port/esp32_rom_wifi.ld "$SDK/linker/esp32/"
cp tools/sdk/norn.mk "$SDK/norn.mk"
cp tools/sdk/nornNET.mk tools/sdk/nornWIFI.mk "$SDK/modules/"
cp tools/monitor.py tools/qemu_run.py tools/bin2c.py "$SDK/tools/"
for e in examples/*/; do
    n=$(basename "$e"); mkdir -p "$SDK/examples/$n"
    cp "$e"/*.c "$e"/Makefile "$SDK/examples/$n/"
    [ -f "$e/nw_secrets.example.h" ] && cp "$e/nw_secrets.example.h" "$SDK/examples/$n/"
done
cp LICENSE.md THIRD_PARTY_LICENSES.md docs/sdk.md "$SDK/"; mv "$SDK/sdk.md" "$SDK/README.md"
printf 'build/\n*.map\nexamples/wifi/nw_secrets.h\n' > "$SDK/.gitignore"

echo "== manifest"
ver() { git -C "$1" describe --always --dirty 2>/dev/null || echo unknown; }
{
    echo "nornSDK build $(date -u +%Y-%m-%dT%H:%MZ) from nornRTOS $(ver "$ROOT")"
    echo "nornCore $(sed -n 's/^#define RTOS_VERSION *"\(.*\)"/\1/p' "$CORE/kernel/rtos.h") ($(ver "$CORE"))"
    echo "nornNET ($(ver "$NET")), nornWIFI ($(ver "$WIFI")) with Espressif $(head -1 "$WIFI/lib/esp32/VERSION")"
    echo "toolchain esp32: $("${XTENSA_ROOT:-$HOME/dev/tools/xtensa-esp-elf-16.1.0}/bin/xtensa-esp32-elf-gcc" --version 2>/dev/null | head -1 || echo 'not present')"
    echo "toolchain avr:   $(avr-gcc --version 2>/dev/null | head -1 || echo 'not present')"
    echo "boards: $BOARDS; profiles: $PROFILES (see lib/ for what each board has)"
} > "$SDK/MANIFEST"
cat "$SDK/MANIFEST"
