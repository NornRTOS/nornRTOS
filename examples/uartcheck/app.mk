# Serial link check for the AVR boards: 200 numbered lines of a fixed pattern, once per
# reset, so the receiving side can count what arrived intact. UARTCHECK_U2X=1 sets the
# UART to double speed (UBRR 16, 117647 baud, +2.1 %) after uart_init, the setting of the
# Arduino USB-serial firmware; without it the driver's own setting is used (UBRR 8, 111111
# baud, -3.5 %). From the nornRTOS directory:
#   make APP=uartcheck BOARD=uno [EXTRA_CFLAGS=-DUARTCHECK_U2X=1]
BOARD ?= uno
ONLY_BOARDS = uno mega2560
