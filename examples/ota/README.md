# examples/ota: an update over the air

The application side of nornCore's OTA (hal/ota.h; the selector and the slots are
`make OTA=1`, nornCore README "Boot and update"). On every boot it says which slot it runs
from and in which state, joins the network, confirms the slot if it is a trial (the proof
of life), and asks the host for `/update.nota`. A 200 is streamed into `ota_write()`
through nornHTTP's body callback, `ota_end()` checks the hash and marks the other slot as
the one to try, and the board resets into it. A 404 is "nothing to do".

    cp nw_secrets.example.h nw_secrets.h          # the network, the host and its port
    make APP=ota BOARD=esp32 OTA=1 upload PORT=/dev/ttyUSB0
    tools/ota_image.py RAM.bin FLASH.bin update.nota   # in modules/nornCore: the file to serve
    tools/ota_serve_once.py 18081 update.nota     # on the host, serves it once

`tools/ota_test.sh` runs the whole thing: the normal image into slot B, booted, confirmed
(RESULT ota PASS), then an image built with `-DOTA_NEVER_CONFIRM` that the selector
rejects after three boots, back to the confirmed slot.

Measured on the board (2026-11-20): a 403 KB image (60 KB RAM image, 343 KB of flash
code) written in 6.7 to 10.9 s over Wi-Fi, the sector erases included; the trial boot's
selector cost 200 ms hash, 60 ms signature check, 25 ms load, 140 ms for the table write.
