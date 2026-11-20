# avr

mega2560 (ATmega2560, Arduino Mega, 8 KB RAM, /dev/ttyACM0) and uno (ATmega328P, 2 KB
RAM). 16 MHz, 1 kHz tick on a 16 bit timer. Port in modules/nornCore/port/avr, board HAL
in modules/nornCore/hal/avr. Toolchain: avr-gcc, avr-libc, avrdude, simavr (runs every
example cycle accurately). nornNET runs on the Mega over SLIP; the UNO's 2 KB of RAM
has room for the kernel only. The original Mega bootloader does not survive a watchdog
reset: keep RTOS_CFG_WATCHDOG off, or use a bootloader that disables the watchdog.
