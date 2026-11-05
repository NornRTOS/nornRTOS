# avr

mega2560 (ATmega2560, Arduino Mega, 8 KB RAM, /dev/ttyACM0) and uno (ATmega328P, 2 KB
RAM). 16 MHz, 1 kHz tick on a 16 bit timer. Port in modules/nornCore/port/avr, board HAL
in modules/nornCore/hal/avr. Toolchain: avr-gcc, avr-libc, avrdude, simavr (runs every
example cycle accurately). Profiles: small and default on both, large on the Mega;
nornNET (SLIP) on the Mega's default profile. The original Mega bootloader does not
survive a watchdog reset: the watchdog is off in the profiles.
