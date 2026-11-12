#!/bin/bash
# The size contract of the optional modules (nornSHELL, nornStore): an application that
# does not enable a module gets a kernel that is identical to the byte and an image
# without a function of the module, and the cost of what it does enable is logged. Run
# from the nornRTOS directory:
#
#   tools/size_check.sh                 all boards (uno mega2560 esp32)
#   BOARDS="esp32" tools/size_check.sh  a subset
#   PER_COMMAND=0 tools/size_check.sh   skip the per-command builds (six more per board)
#   LOG=path tools/size_check.sh        append the cost rows to a markdown file as well
#
# Steps, 0 to 2 hard (exit 1), the rest informational:
#   0. no module source includes the kernel's private header or touches its tables, and
#      no module names another module's symbols (the application wires them together)
#   1. examples/shell built with and without nosh (SHELL_WITHOUT_NOSH), examples/store
#      built with and without the store (STORE_WITHOUT_STORE): the kernel+port objects of
#      each pair are identical
#   2. the image built without a module holds no function or data of it (symbols of type
#      T, t, D, d, B, b, R, r; the ROM addresses a linker script PROVIDEs are type A and
#      cost nothing)
#   3. the cost of each module in its image (text/data/bss delta) and its symbols by size
#   4. the cost of each nosh command (help only, then one command at a time,
#      SHELL_ONLY_<CMD> in examples/shell/shell.c) and of the `config` command
#      (STORE_NO_CMD in examples/store/store_main.c), text delta
set -u
cd "$(dirname "$0")/.."
BOARDS=${BOARDS:-"uno mega2560 esp32"}
PER_COMMAND=${PER_COMMAND:-1}
COMMANDS="UPTIME PS MEM SCHED"
XTENSA_ROOT=${XTENSA_ROOT:-$HOME/dev/tools/xtensa-esp-elf-16.1.0}
CORE=modules/nornCore
BUILD=$CORE/build
fail=0
today=$(date +%F)

size_tool() { case $1 in esp32) echo "$XTENSA_ROOT/bin/xtensa-esp32-elf-size";; *) echo avr-size;; esac; }
nm_tool()   { case $1 in esp32) echo "$XTENSA_ROOT/bin/xtensa-esp32-elf-nm";;   *) echo avr-nm;;   esac; }
objs()  { "$(size_tool "$2")" -t "$1"/kernel/*.o "$1"/port/*/*.o | tail -1 | awk '{print $1, $2, $3}'; }
image() { "$(size_tool "$2")" "$1" | tail -1 | awk '{print $1, $2, $3}'; }
build() {   # app board [extra make args] -> 0/1, log in build/size-check/
    local app=$1 board=$2; shift 2
    mkdir -p build/size-check
    make -s APP="$app" BOARD="$board" "$@" > "build/size-check/$app-$board$*.log" 2>&1
}
# Functions and data (not absolute or debug symbols) matching a pattern in an image.
code_syms() {   # elf board pattern
    "$(nm_tool "$2")" "$1" | awk '$2 ~ /^[TtDdBbRr]$/' | grep -c "$3"
}

echo "== step 0: module sources against the kernel's internals and each other"
rule() {   # description grep-args... ; a hit is a failure
    local what=$1; shift
    local hits; hits=$(grep -rn "$@" 2>/dev/null)
    if [ -n "$hits" ]; then echo "FAIL: $what:"; echo "$hits"; fail=1; else echo "ok: $what"; fi
}
rule "nornSHELL stays out of the kernel"   "#include.*rtos_kernel.h\|rtos_tcbAry\|rtos_pCurrent" modules/nornSHELL/src modules/nornSHELL/include
rule "nornStore stays out of the kernel"   "#include.*rtos_kernel.h\|rtos_tcbAry\|rtos_pCurrent" modules/nornStore/src modules/nornStore/include
rule "nornSHELL does not know nornStore"   "store_"  modules/nornSHELL/src modules/nornSHELL/include
rule "nornNET does not know nornStore"     "store_"  modules/nornNET/nn modules/nornNET/os
rule "nornStore does not know nornNET"     "nn_"     modules/nornStore/src modules/nornStore/include

# One module in one example: the pair of builds, the two hard checks, the cost.
#   module_check <board> <app> <module label> <without-make-variable> <without-define> <symbol pattern>
module_check() {
    local board=$1 app=$2 label=$3 var=$4 def=$5 pat=$6
    local with without syms t1 d1 b1 t0 d0 b0
    rm -rf "$BUILD/$app-$board-extra"
    if ! build "$app" "$board"; then echo "   FAIL: $app build failed, see build/size-check/"; fail=1; return 1; fi
    if ! build "$app" "$board" "$var=1" "EXTRA_CFLAGS=-D$def"; then echo "   FAIL: $app without $label failed"; fail=1; return 1; fi
    with=$(objs "$BUILD/$app-$board" "$board")
    without=$(objs "$BUILD/$app-$board-extra" "$board")
    syms=$(code_syms "$BUILD/$app-$board-extra/nornos_$app.elf" "$board" "$pat")
    echo "   $app: kernel+port objects (text data bss) with $label [$with]  without [$without]"
    if [ "$with" != "$without" ]; then echo "   FAIL: enabling $label changed the kernel+port objects"; fail=1
    else echo "   ok: kernel+port objects identical with and without $label"; fi
    if [ "$syms" != 0 ]; then echo "   FAIL: $syms $label symbols with code or data in the image built without it"; fail=1
    else echo "   ok: no $label code or data without $label"; fi
    read -r t1 d1 b1 <<< "$(image "$BUILD/$app-$board/nornos_$app.elf" "$board")"
    read -r t0 d0 b0 <<< "$(image "$BUILD/$app-$board-extra/nornos_$app.elf" "$board")"
    echo "   $app: image with $label [$t1 $d1 $b1]  without [$t0 $d0 $b0]  cost text $((t1 - t0)) data $((d1 - d0)) bss $((b1 - b0))"
    [ -n "${LOG:-}" ] && echo "| $board | $today | $label in $app | $t1 / $d1 / $b1 | $t0 / $d0 / $b0 | +$((t1 - t0)) | +$((d1 + b1 - d0 - b0)) |" >> "$LOG"
    echo "   $label symbols by size (largest):"
    "$(nm_tool "$board")" --size-sort -S -t d "$BUILD/$app-$board/nornos_$app.elf" | awk '$3 ~ /^[TtDdBb]$/' | grep -i "$pat" | awk '{printf "     %6d %s %s\n", $2, $3, $4}' | tail -8
    return 0
}

for board in $BOARDS; do
    echo "== $board"
    module_check "$board" shell nosh  SHELL_WITHOUT_NOSH  SHELL_WITHOUT_NOSH  "nosh\|cmdHelp\|shellTask\|toDecimal" || continue
    module_check "$board" store store STORE_WITHOUT_STORE STORE_WITHOUT_STORE "store_\|hal_flash\|eeprom_\|blockValid\|blockWrite\|forEachRecord\|cmdConfig\|parseValue" || continue

    [ "$PER_COMMAND" = 1 ] || continue
    # Step 4a: the nosh commands. Every EXTRA_CFLAGS build lands in the same -extra
    # directory and make does not see a flag change, so it is removed between variants.
    rm -rf "$BUILD/shell-$board-extra"
    if ! build shell "$board" EXTRA_CFLAGS=-DSHELL_ONLY_NONE; then echo "   FAIL: help-only build failed"; fail=1; continue; fi
    read -r th dh bh <<< "$(image "$BUILD/shell-$board-extra/nornos_shell.elf" "$board")"
    read -r t1 d1 b1 <<< "$(image "$BUILD/shell-$board/nornos_shell.elf" "$board")"
    echo "   nosh per command (text/data/bss over help only [$th $dh $bh]):"
    for cmd in $COMMANDS; do
        rm -rf "$BUILD/shell-$board-extra"
        if ! build shell "$board" EXTRA_CFLAGS=-DSHELL_ONLY_$cmd; then echo "   FAIL: SHELL_ONLY_$cmd build failed"; fail=1; continue; fi
        read -r tc dc bc <<< "$(image "$BUILD/shell-$board-extra/nornos_shell.elf" "$board")"
        printf '     %-7s +%d / %+d / %+d\n' "$(echo "$cmd" | tr 'A-Z' 'a-z')" $((tc - th)) $((dc - dh)) $((bc - bh))
        [ -n "${LOG:-}" ] && echo "| $board | $today | nosh $(echo "$cmd" | tr 'A-Z' 'a-z') | +$((tc - th)) | $((dc - dh)) | $((bc - bh)) |" >> "$LOG"
    done
    echo "     default set: +$((t1 - th)) text over help only"

    # Step 4b: the config command: the store example with the store but without it.
    rm -rf "$BUILD/store-$board-extra"
    if ! build store "$board" EXTRA_CFLAGS=-DSTORE_NO_CMD; then echo "   FAIL: STORE_NO_CMD build failed"; fail=1; continue; fi
    read -r t0 d0 b0 <<< "$(image "$BUILD/store-$board-extra/nornos_store.elf" "$board")"
    read -r t1 d1 b1 <<< "$(image "$BUILD/store-$board/nornos_store.elf" "$board")"
    echo "   config command: +$((t1 - t0)) / $((d1 - d0)) / $((b1 - b0)) text/data/bss over the store without it"
    [ -n "${LOG:-}" ] && echo "| $board | $today | config command | +$((t1 - t0)) | $((d1 - d0)) | $((b1 - b0)) |" >> "$LOG"
done

if [ $fail -eq 0 ]; then echo "SIZE CHECK PASS"; else echo "SIZE CHECK FAIL"; fi
exit $fail
