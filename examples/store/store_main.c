/**
 * @file store_main.c
 * nornStore on nornCore's flash driver, with nosh and its `config` command: a
 * configuration that survives a reset and can be read and changed from the console.
 *
 * At boot the store is loaded and printed (the sequence number of the image and the
 * values); then the boot counter is incremented and the hostname set from it, and the
 * image is saved. Every boot therefore shows the previous boot's values and a sequence
 * number one higher. Then the shell takes over: `config` lists the objects, `config set
 * network.ip 10.0.0.5` and `config save` change one for the next boot.
 *
 * Build from the nornRTOS directory: make APP=store BOARD=esp32, then run it twice on the
 * same flash file:
 *
 *   python3 modules/nornSHELL/tools/qemu_talk.py modules/nornCore/build/store-esp32/nornos_store.bin \
 *       --flash build/store.flash --send config --send "config info"
 *
 * STORE_WITHOUT_STORE builds the same application without the store (the shell stays);
 * the image must then contain no store_ or hal_flash symbol. STORE_NO_CMD keeps the
 * store but does not register `config`, for the command's cost (tools/size_check.sh).
 */

#include "rtos.h"
#include "uart.h"
#include "board.h"
#include "nosh.h"

#ifndef STORE_WITHOUT_STORE
#include "store.h"
#include "store_cmd.h"
#include "flash.h"

enum { ID_HOSTNAME = 0x0101, ID_BOOTS = 0x0102, ID_IP = 0x0201 };

static store_obj_t _objs[] =
{
    STORE_OBJ(ID_HOSTNAME, STORE_STR, 24),
    STORE_OBJ(ID_BOOTS,    STORE_U32,  4),
    STORE_OBJ(ID_IP,       STORE_IP4,  4),
};
static uint8_t _values[32];
#ifndef STORE_NO_CMD
static const store_name_t _names[] =
{
    { ID_HOSTNAME, "system.hostname" }, { ID_BOOTS, "system.boots" }, { ID_IP, "network.ip" },
};
#endif
#endif

#define APP_PRIO        2u
#define SHELL_PRIO      1u

static const gpio_pin_t _led = BOARD_PIN_LED;
static const char *const _taskNames[] = { "idle", "nosh", "app" };

/* ---- the application task ------------------------------------------------------------ */

#ifndef STORE_WITHOUT_STORE
static void putResult(const char *what, int res)
{
    uart_putsP(what);
    uart_putsP(res == 0 ? HAL_FSTR(": ok\r\n") : HAL_FSTR(": error "));
    if(res != 0)
    {
        uart_putU32((uint32_t)res);
        uart_putsP(HAL_FSTR("\r\n"));
    }
}
#endif

static void task(void *arg)
{
    (void)arg;
    gpio_output(&_led);

#ifndef STORE_WITHOUT_STORE
    {
        char host[25];
        uint32_t boots = 0;
        int res;

        uart_putsP(HAL_FSTR("store: image seq ")); uart_putU32(store_seq());
        uart_putsP(HAL_FSTR(" in block ")); uart_putI32(store_activeBlock());
        uart_putsP(HAL_FSTR("\r\n"));

        (void)store_getStr(ID_HOSTNAME, host, sizeof host);
        (void)store_getU32(ID_BOOTS, &boots, NULL);
        uart_putsP(HAL_FSTR("store: hostname '")); uart_puts(host);
        uart_putsP(HAL_FSTR("' boots ")); uart_putU32(boots);
        uart_putsP(HAL_FSTR("\r\n"));

        /* this boot's values: one more boot, a hostname that says so */
        ++ boots;
        host[0] = 'n'; host[1] = 'o'; host[2] = 'r'; host[3] = 'n'; host[4] = '-';
        host[5] = (char)('0' + boots / 100u % 10u);
        host[6] = (char)('0' + boots / 10u % 10u);
        host[7] = (char)('0' + boots % 10u);
        host[8] = '\0';
        (void)store_setStr(ID_HOSTNAME, host);
        (void)store_setU32(ID_BOOTS, boots);
        res = store_save();
        putResult(HAL_FSTR("store: save"), res);
        if(res == STORE_ERR_FLASH)
            putResult(HAL_FSTR("flash: last result"), hal_flashLastResult());
    }
#else
    uart_putsP(HAL_FSTR("store: not enabled in this build\r\n"));
#endif

    for(;;)
    {
        (void)rtos_waitPeriodic(RTOS_MS(500));
        gpio_toggle(&_led);
    }
}

int main(void)
{
    uart_init(UART_BAUD_DEFAULT);
    uart_putsP(HAL_FSTR("\r\nnornRTOS store example\r\n"));
    rtos_init();
#ifndef STORE_WITHOUT_STORE
    /* nornCore's flash HAL as the driver, hal_flashInit() included */
    putResult(HAL_FSTR("store: init"), store_initHal(_objs, sizeof _objs / sizeof *_objs, _values, sizeof _values));
#endif
    RTOS_TASK(APP_PRIO, task, NULL, RTOS_STACK_DEFAULT);
    nosh_uartInit();                            /* the console UART as the transport */
    NOSH_ENABLE(&nosh_uartIo, SHELL_PRIO, RTOS_STACK_DEFAULT);
    nosh_setTaskNames(_taskNames, sizeof _taskNames / sizeof *_taskNames);
#if !defined(STORE_WITHOUT_STORE)  &&  !defined(STORE_NO_CMD)
    nosh_add(&store_cmd_config);
    store_cmd_setNames(_names, sizeof _names / sizeof *_names);
#endif
    rtos_start();
    return 0;
}
