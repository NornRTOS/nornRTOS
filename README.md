# nornRTOS

nornRTOS is a real time operating system for small microcontrollers: a kernel
(nornCore), a TCP/IP stack (nornNET), the ESP32's WiFi radio without ESP-IDF or FreeRTOS
(nornWIFI), and more modules to come. This repository is the tree that holds them
together: the modules as git submodules, the configuration profiles, the boards, the
examples, the tools, and the build of the binary SDK.

    modules/        nornCore, nornNET, nornWIFI (submodules; see .gitmodules and tools/submodules.sh)
    profiles/       small, default, large: the frozen kernel and module configurations,
                    one directory per profile with a section per board in each file
    boards/         what each board is and what it has (esp32, avr: mega2560 and uno)
    examples/       hello, semaphore, basics, timing, bench, net, wifi; each builds from
                    the sources here (app.mk) and against the binary SDK (Makefile)
    tools/          sdk.sh (the binary SDK into build/sdk), sdk/ (its build fragments),
                    submodules.sh, monitor.py, qemu_run.py, bin2c.py
    docs/           sdk.md, the user's guide to the binary SDK
    LICENSE.md      Business Source License 1.1 with Tenforward AB's Additional Use Grant
    THIRD_PARTY_LICENSES.md

## Building from source

    make APP=hello BOARD=esp32 PROFILE=default         image in modules/nornCore/build/hello-esp32
    make APP=hello BOARD=esp32 sim                     Espressif's QEMU
    make APP=basics BOARD=mega2560 PROFILE=small sim   simavr
    make APP=wifi BOARD=esp32 upload PORT=/dev/ttyUSB0
    make APP=wifi BOARD=esp32 monitor
    make APP=myapp APPDIR=/path/to/myapp               an application kept elsewhere

An application is a directory with its sources and an app.mk (BOARD, MODULES, what runs
from flash); it compiles against the profile's rtos_config.h, nn_config.h and
nw_config.h unless it carries its own. The build is nornCore's Makefile with the
profile directory and this tree passed in; PORT, EXTRA_CFLAGS, SIM_SECONDS and CPU_MHZ
pass through.

## The binary SDK

    make lib BOARD=esp32 PROFILE=default    the libraries of one profile
    make sdk                                build/sdk: libraries for every board and
                                            profile, headers, profiles, linker scripts,
                                            norn.mk, tools, examples, licence, MANIFEST

build/sdk is what the public nornSDK repository publishes at a release; docs/sdk.md (the
bundle's README) explains how an application uses it. Sources stay here.

## Tests

    make test       nornCore's suite: host unit tests, simavr, QEMU, and the boards that
                    are connected (tools/test_all.sh in modules/nornCore)

nornNET and nornWIFI have their own `make host-test` / `make test-all` in modules/.

## Licence

Business Source License 1.1 with the Additional Use Grant in LICENSE.md: production use
in your own products, no offering it as an operating system, kernel or SDK of your own,
Apache 2.0 four years after each release. Third-party components keep their own
licences, THIRD_PARTY_LICENSES.md.
