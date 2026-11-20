#!/bin/bash
# The OTA test of examples/ota on the board (nornInternal/nornCore/ota_plan.md step 2):
#
#   tools/ota_test.sh [PORT]          default /dev/ttyUSB0; needs examples/ota/nw_secrets.h
#
# 1. builds examples/ota in OTA mode twice (a normal image and one that never confirms),
#    makes the two image files, uploads selector, slot table and slot A;
# 2. phase 1: the host serves the normal image once; the board must write slot B, reset,
#    boot it as a trial, confirm it and say RESULT ota PASS from slot 1;
# 3. phase 2: the host serves the never-confirming image once; the board writes slot A,
#    boots it three times, the selector rejects it and slot B runs again (its "no update").
# 4. phase 3: a forged image (one signature byte changed) is rejected at its first boot.
# The serial port must be free (no monitor open). Takes about three minutes.
set -eu
cd "$(dirname "$0")/.."
PORT=${1:-/dev/ttyUSB0}
PY=${ESP_PYTHON:-$HOME/dev/tools/esp-venv/bin/python}
SRV_PORT=$(sed -n 's/.*OTA_SERVER_PORT *\([0-9]*\).*/\1/p' examples/ota/nw_secrets.h)
OUT=modules/nornCore/build/ota-test; mkdir -p "$OUT"

echo "== building"
make -s APP=ota BOARD=esp32 OTA=1 image > "$OUT/build.log" 2>&1
make -s APP=ota BOARD=esp32 OTA=1 EXTRA_CFLAGS=-DOTA_NEVER_CONFIRM image >> "$OUT/build.log" 2>&1
K=modules/nornCore/ota_key.pem
B=modules/nornCore/build/ota-esp32-ota;        $PY modules/nornCore/tools/ota_image.py --key $K $B/nornos_ota.bin  $B/nornos_ota.flash.bin  "$OUT/good.nota"
B=modules/nornCore/build/ota-esp32-extra-ota;  $PY modules/nornCore/tools/ota_image.py --key $K $B/nornos_ota.bin  $B/nornos_ota.flash.bin  "$OUT/bad.nota"
make -s APP=ota BOARD=esp32 OTA=1 PORT="$PORT" upload >> "$OUT/build.log" 2>&1

watch() {   # seconds, log: resets the board and records the console
    "$PY" - "$PORT" "$1" > "$2" <<'PYEOF'
import serial, sys, time
ser = serial.Serial(sys.argv[1], 115200, timeout=0.05)
ser.dtr = False; ser.rts = True; time.sleep(0.1); ser.rts = False
t0 = time.time(); buf = b''
while time.time() - t0 < float(sys.argv[2]):
    d = ser.read(256)
    if d:
        buf += d
        while b'\n' in buf:
            line, buf = buf.split(b'\n', 1)
            print("%7.3f  %s" % (time.time() - t0, line.decode('utf-8', 'replace').rstrip()))
PYEOF
}
phase() {   # name, image, seconds, pattern that must appear
    python3 tools/ota_serve_once.py "$SRV_PORT" "$2" > "$OUT/serve-$1.log" 2>&1 & local srv=$!
    sleep 0.5
    echo "== phase $1: serving $(basename "$2") once, watching $3 s"
    watch "$3" "$OUT/phase-$1.log"
    kill $srv 2>/dev/null || true
    grep -E 'selector|ota:|RESULT' "$OUT/phase-$1.log" | cut -c1-140
    if grep -q "$4" "$OUT/phase-$1.log"; then echo "phase $1: PASS"; else echo "phase $1: FAIL (no '$4')"; exit 1; fi
}
# 4. phase 3: a forged image, the good one with one signature byte changed: written and
#    booted once, the selector says "bad signature", rejects it and the confirmed slot runs.
python3 - "$OUT/good.nota" "$OUT/forged.nota" <<'PYEOF'
import sys
d = bytearray(open(sys.argv[1], "rb").read()); d[48] ^= 0x01; open(sys.argv[2], "wb").write(d)
PYEOF
phase 1 "$OUT/good.nota"   45 'RESULT ota PASS'
phase 2 "$OUT/bad.nota"    70 'rejected, slot [01]'
phase 3 "$OUT/forged.nota" 45 'bad signature'
echo "RESULT ota_test PASS"
