/**
 * @file net_ping.c
 * nornNET: register the loopback and the open_eth device (QEMU), give the interface an
 * address on QEMU's user mode network, ping its gateway twenty times and our own
 * loopback once, print the result.
 */

#include "rtos.h"
#include "uart.h"
#include "testresult.h"
#include "nn_net.h"
#include "nn_ip.h"
#include "nn_arp.h"
#include "nn_icmp.h"
#include "nn_drv_openeth.h"
#include "nn_drv_loop.h"

#define TASK_MAIN   1u
#define PINGS       20u
#define PING_ID     0x4E31u

static uint8_t _stack[RTOS_STACK_LARGE];
static nn_netdev_t _eth, _lo;
static nn_iface_t _ifEth, _ifLo;
static const uint8_t _mac[6] = { 0x02, 0x4E, 0x4E, 0x00, 0x00, 0x20 };
static volatile uint32_t _replies, _loReplies, _rttMax;
static volatile rtos_tick_t _sentAt;

static void onReply(nn_ip4_t from, uint16_t id, uint16_t seq, const uint8_t *p, uint16_t len, void *arg)
{
    (void)seq; (void)p; (void)len; (void)arg;
    if(id != PING_ID)
        return;
    if(nn_ip4Equal(from, nn_ip4(10, 0, 2, 2)))
    {
        const uint32_t rtt = rtos_getTick() - _sentAt;
        ++ _replies;
        if(rtt > _rttMax) _rttMax = rtt;
    }
    else
        ++ _loReplies;
    rtos_notify(TASK_MAIN, 1, RTOS_NOTIFY_OR);
}

static void mainTask(void *arg)
{
    uint32_t i;
    (void)arg;
    nn_lock();
    nn_icmpOnEchoReply(onReply, NULL);
    nn_unlock();
    for(i=1; i<=PINGS; ++i)
    {
        nn_lock();
        _sentAt = rtos_getTick();
        nn_icmpEchoRequest(nn_ip4(10, 0, 2, 2), PING_ID, (uint16_t)i, NULL, 32);
        nn_unlock();
        rtos_notifyTake(1000);
        rtos_delay(RTOS_MS(50));
    }
    nn_lock();
    nn_icmpEchoRequest(nn_ip4(127, 0, 0, 1), PING_ID, 100, NULL, 8);
    nn_unlock();
    rtos_notifyTake(200);

    uart_puts("net: replies "); uart_putU32(_replies); uart_puts("/"); uart_putU32(PINGS);
    uart_puts(" rtt max "); uart_putU32(_rttMax); uart_puts(" ms, loopback "); uart_putU32(_loReplies);
    uart_puts("\r\n");
    tc_expectRange("replies", _replies, PINGS - 1, PINGS);     /* the first may lose the ARP race */
    tc_expectRange("loopback", _loReplies, 1, 1);
    tc_finish("net");
}

void rtos_assertFailed(const char *file, uint16_t line)
{
    (void)rtos_port_enterCritical();
    uart_puts("\r\nASSERTION FAILED: "); uart_putsP(file); uart_puts(", line "); uart_putU32(line); uart_puts("\r\n");
    rtos_port_halt();
}

/** The panic hook as every application should have one: say what happened before the
    halt (the kernel's default is silent). */
void rtos_panicHook(const rtos_panicInfo_t *pInfo)
{
    uart_puts("\r\nPANIC reason "); uart_putU32(pInfo->reason);
    uart_puts(" task "); uart_putU32(pInfo->task);
    uart_puts(" arg1 "); uart_putU32(pInfo->arg1);
    uart_puts(" arg2 "); uart_putU32(pInfo->arg2);
    uart_puts(" (reason 6 = built against another configuration than the kernel)\r\n");
    uart_flush();
}

int main(void)
{
    uart_init(115200);
    uart_puts("\r\nnornRTOS net\r\n");
    rtos_init();
    nn_init();
    nn_drv_loopRegister(&_lo);
    nn_ifaceAdd(&_lo, &_ifLo, nn_ip4(127, 0, 0, 1), nn_ip4(255, 0, 0, 0), nn_ip4(0, 0, 0, 0));
    if(nn_drv_openethRegister(&_eth, _mac) != NN_OK)
    {
        uart_puts("net: no open_eth MAC (this example runs in QEMU)\r\nRESULT net FAIL\r\n");
        for(;;) ;
    }
    nn_ifaceAdd(&_eth, &_ifEth, nn_ip4(10, 0, 2, 15), nn_ip4(255, 255, 255, 0), nn_ip4(10, 0, 2, 2));
    rtos_createTask(TASK_MAIN, mainTask, NULL, _stack, sizeof(_stack));
    rtos_start();
    return 0;
}
