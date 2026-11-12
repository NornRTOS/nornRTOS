# nornRTOS

nornRTOS is a real time operating system for small microcontrollers: a kernel
(nornCore), a TCP/IP stack (nornNET), the ESP32's WiFi radio without ESP-IDF or FreeRTOS
(nornWIFI), an HTTP client (nornHTTP), a shell (nornSHELL) and a configuration store
(nornStore). Every module but the kernel is optional: an application that does not
enable one gets a kernel identical to the byte and no code of it (tools/size_check.sh). This repository is the tree that holds them
together: the modules as git submodules, the boards, the examples and the tools. The
user documentation (the kernel's design, porting, the generated API reference) is its
own repository, nornDocs (https://github.com/NornRTOS/nornDocs); each repository here
keeps its README and the documents of its own development.

    modules/        nornCore, nornNET, nornWIFI, nornHTTP, nornSHELL, nornStore (submodules;
                    see .gitmodules and tools/submodules.sh)
    boards/         what each board is and what it has (esp32, avr: mega2560 and uno)
    examples/       hello, semaphore, basics, timing, bench, net, wifi, wifi_http, shell,
                    store, device: applications built from the module sources, each with
                    its own configuration; device uses every module at once
    tools/          submodules.sh, size_check.sh, monitor.py, qemu_run.py, bin2c.py
    LICENSE.md      Business Source License 1.1 with Tenforward AB's Additional Use Grant
    THIRD_PARTY_LICENSES.md

## Building

    make APP=hello BOARD=esp32                         image in modules/nornCore/build/hello-esp32
    make APP=hello BOARD=esp32 sim                     Espressif's QEMU
    make APP=basics BOARD=mega2560 sim                 simavr
    make APP=wifi BOARD=esp32 upload PORT=/dev/ttyUSB0
    make APP=wifi BOARD=esp32 monitor
    make APP=myapp APPDIR=/path/to/myapp               an application kept elsewhere

An application is a directory with its sources, an app.mk (BOARD, MODULES, what runs
from flash) and its own rtos_config.h (start from modules/nornCore/config/rtos_config.template.h;
the nornCore README has three measured sets of values to begin with),
plus nn_config.h and nw_config.h for the modules it uses: the kernel and the modules are
compiled from source with exactly that configuration, as the examples are. The build is
nornCore's Makefile with this tree passed in; PORT, EXTRA_CFLAGS, SIM_SECONDS and
CPU_MHZ pass through. The hooks an application may define are functions: rtos_idleHook,
rtos_stackWarnHook, rtos_panicHook, rtos_assertFailed, rtos_traceHook (the examples show
them).

## Tests

    make test       nornCore's suite: host unit tests, simavr, QEMU, and the boards that
                    are connected (tools/test_all.sh in modules/nornCore)

nornNET and nornWIFI have their own `make host-test` / `make test-all` in modules/.

## Licence

Business Source License 1.1 with the Additional Use Grant in LICENSE.md: production use
in your own products, no offering it as an operating system, kernel or SDK of your own,
Apache 2.0 four years after each release. Third-party components keep their own
licences, THIRD_PARTY_LICENSES.md.
