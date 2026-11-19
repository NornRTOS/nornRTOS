# esp32

ESP32-D0WDQ6 (NodeMCU-32S style devkit, CP2102 on /dev/ttyUSB0), 240 MHz from the PLL,
no ESP-IDF, no FreeRTOS: the ROM loads the image straight from flash offset 0x1000, code
that does not fit the 128 KB of IRAM runs from flash through the cache. Port in
modules/nornCore/port/xtensa, board HAL in modules/nornCore/hal/xtensa, linker scripts in
modules/nornCore/port/xtensa/*.ld. Toolchain: Espressif xtensa-esp-elf (XTENSA_ROOT) and
esptool in a Python (ESP_PYTHON); Espressif's QEMU (QEMU_ESP) runs every example but
wifi. nornWIFI needs RTOS_CFG_HEAP 1, RTOS_CFG_EVGROUPS 1 and RTOS_CFG_EVT_BITS 32 in
the application's rtos_config.h (examples/wifi).
