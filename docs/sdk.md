# nornSDK

The binary SDK of nornRTOS: the kernel of nornCore as a library per board and
configuration profile, the public headers, the linker scripts, the tools and a build
fragment that turns a few lines of Makefile into an image. The module sources are not
part of it; what an application needs is here.

    include/nornCore/        rtos.h and rtos_types.h, port/<arch>/port.h, hal/*.h
    profiles/<profile>/      rtos_config.h, nn_config.h, nw_config.h: the frozen configuration
                             the libraries were built from, one section per board
    include/nornNET/         the stack's headers
    include/nornWIFI/        nw.h, nw_config.h; esp/ the ESP-IDF headers the radio API uses
                             (Apache-2.0)
    lib/<board>/<profile>/   libnornCore.a, libnornNET.a, libnornWIFI.a + libnwsupp.a (the
                             supplicant), all built from the same profile
    lib/esp32/espressif/     Espressif's binary radio libraries with their licences
    linker/esp32/            memory layouts, flash window, section rules, ROM symbols
    modules/                 nornNET.mk, nornWIFI.mk: what MODULES = ... pulls in
    tools/                   monitor.py (serial), qemu_run.py (Espressif QEMU), bin2c.py,
                             (the bundle is built by `make sdk` in the nornRTOS tree)
    examples/hello/          one periodic task, the smallest application
    examples/semaphore/      two tasks, a semaphore and a timer; the hooks as functions
    examples/basics/         nornCore's tc01: every basic primitive, on every board
    examples/timing/         nornCore's tc04: switch, interrupt latency, jitter
    examples/bench/          nornCore's tc10: the cycle benchmark (Mega, ESP32)
    examples/net/            nornNET over the open_eth MAC in QEMU: pings the gateway
    examples/wifi/           nornWIFI: join, DHCP, ping the gateway (board, nw_secrets.h)
    norn.mk                  the build fragment
    MANIFEST                 what this SDK was built from: module versions, toolchains

## Boards and profiles

    board      profiles                nornNET            nornWIFI         notes
    esp32      small, default, large   all three          default, large   ESP32-D0WDQ6, 240 MHz
    mega2560   small, default, large   all three (SLIP)   -                Arduino Mega, 16 MHz
    uno        small, default          -                  -                Arduino UNO, 2 KB RAM

A profile fixes the number of tasks, the tick and event widths, every optional feature
and the priority map; the application chooses its tasks, their stacks and priorities
up to RTOS_PRIO_APP_MAX, and defines the hooks it wants as functions (rtos_idleHook,
rtos_stackWarnHook, rtos_panicHook, rtos_assertFailed, rtos_traceHook). `default` has
everything on; `small` is the lean kernel for little programs (no statistics,
assertions, heap, event groups, suspend or watchdog: nornNET fits, nornWIFI does not);
`large` adds the most tasks, tracing and (on the ESP32) tickless idle for development
and diagnosis. The profile's rtos_config.h is the
documentation of what is in: read it before choosing. rtos_init checks that the library
and the application were built from the same profile and stops with RTOS_PANIC_CONFIG if
not.

## Using it

    cd examples/semaphore
    make BOARD=esp32                      the image, build/esp32-default/semaphore.bin
    make BOARD=esp32 upload PORT=/dev/ttyUSB0
    make BOARD=esp32 monitor
    make BOARD=esp32 sim                  Espressif's QEMU (QEMU_ESP=...)
    make BOARD=mega2560 sim               simavr
    make BOARD=uno PROFILE=small

An application Makefile:

    NORN_SDK = /path/to/nornSDK
    APP      = blink
    BOARD   ?= esp32
    PROFILE ?= default
    MODULES  = nornNET nornWIFI     # optional: the modules' libraries of the same profile
    SRC      = blink.c
    include $(NORN_SDK)/norn.mk

A module brings its own profile configuration (nn_config.h, nw_config.h: the buffers,
the task stack, the radio options the library was built with) and its task priorities
from the kernel profile's map; the application creates its own tasks below
RTOS_PRIO_APP_MAX. nornWIFI needs nornNET and a profile with the heap, event groups and
32 event bits (default or large). On the ESP32 the stack, the supplicant, the radio
libraries and libc run from flash; the kernel and the radio adapter (nw_osi, nw_netdev,
nw_supplicant) stay in IRAM. Right after joining, the first unicast frame to the access
point is lost now and then while the access point sets up its block-ack session with the
station; the wifi example sends one unmeasured ping first for that reason, and an application that
needs its first frame delivered should do the same or retry.

Toolchains: Espressif xtensa-esp-elf (XTENSA_ROOT) with esptool in a Python
(ESP_PYTHON), and avr-gcc, avr-libc, avrdude, simavr on the PATH. The libraries are
built with the compiler versions named in MANIFEST and without LTO; use the same major
version.

## Cost per profile

Measured with examples/bench (nornCore's cycle benchmark built against the libraries)
on the boards, 2026-11-04; the source build of the same benchmark with its own lean
configuration is the reference. Cycles per context switch (semaphore give and take, the
low task to the high one), and the path from the give in an interrupt to the first
instruction of the woken task:

    board / profile        switch    isr -> task   image text+data+bss (basics)
    esp32 small            347       343           8698 + 24 + 4372
    esp32 default          440       437           9910 + 28 + 5852
    esp32 large            472       467
    esp32 source, tc10     405       417           (15 tasks' worth less bookkeeping)
    mega2560 small         315       317           4936 + 168 + 807
    mega2560 default       472       485           6692 + 170 + 1002
    mega2560 source, tc10  423       446

The library itself costs nothing: the differences are the profiles. `small` (no run time
statistics, assertions, stack monitor, watchdog, event groups or suspend) is the fastest
kernel there is; `default` pays for the canary check at every switch, the statistics and
the assertions, about 70 cycles on the ESP32 and 50 on the Mega over the source test's
own configuration; `large` adds the trace hook call at every switch and wake-up. The API
costs without a switch are the same within a few cycles in every build. Interrupt entry
is unchanged by the profile (0.9 us average on the ESP32, 3 us on the Mega).

## What is not in this release

AMP (a second image on the ESP32's APP CPU) needs the nornCore build; nornNET on the UNO
and nornWIFI on the small profile have no libraries (no room, respectively no heap).

## Licence

nornRTOS, and with it this SDK, is licensed under the Business Source License 1.1 with
the Additional Use Grant in LICENSE.md: production use in your own products is allowed,
offering it as an operating system, kernel or SDK of your own is not, and each version
becomes Apache 2.0 four years after its release. Third-party components (Espressif's
radio libraries, the ESP-IDF headers, the supplicant, the toolchain runtimes) keep their
own licences, listed in THIRD_PARTY_LICENSES.md.

## Releases

`make sdk` in the nornRTOS tree builds this bundle (tools/sdk.sh): the libraries from the
modules at their checked out commits, headers, profiles, linker scripts, tools, examples,
licence and MANIFEST. A release is that run on tagged modules, copied here and tagged.
