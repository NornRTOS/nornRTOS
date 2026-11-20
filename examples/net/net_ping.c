/**
 * @file net_ping.c
 * nornNET in QEMU: register the loopback and the open_eth device, get an address for the
 * interface from QEMU's DHCP server (10.0.2.2 on the user mode network), ping the gateway
 * twenty times and our own loopback once, print the result. Then nosh takes the console
 * with nornNET's `net` command, so the stack can be looked at and steered: `net`, `net
 * dhcp release`, `net dhcp start`, `net ping 10.0.2.2`, `net arp`, `net stats`.
 *
 * NET_WITHOUT_NOSH builds the same application without the shell: the RESULT line is the
 * end of it, and the image must be free of every nosh and `net` command symbol
 * (tools/size_check.sh).
 */

#include "rtos.h"
#include "uart.h"
#include "testresult.h"
#include "nn_net.h"
#include "nn_ip.h"
#include "nn_arp.h"
#include "nn_icmp.h"
#include "nn_dhcp.h"
#include "nn_drv_openeth.h"
#include "nn_drv_loop.h"
#ifndef NET_WITHOUT_NOSH
#include "nosh.h"
#include "nn_cmd.h"
#endif

#define TASK_MAIN   1u
#define TASK_SHELL  2u
#define PINGS       20u
#define PING_ID     0x4E31u
#define DHCP_WAIT_S 15u

static nn_netdev_t _eth, _lo;
static nn_iface_t _ifEth, _ifLo;
static nn_dhcp_t _dhcp;
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
    char s[16];
    (void)arg;

    /* an address from QEMU's DHCP server */
    if(nn_dhcpStart(&_dhcp, &_ifEth, NULL, NULL) != NN_OK)
        uart_puts("net: DHCP start failed\r\n");
    for(i = 0; i < DHCP_WAIT_S * 10u  &&  !nn_dhcpIsBound(&_dhcp); ++i)
        rtos_delay(RTOS_MS(100));
    uart_puts("net: address "); uart_puts(nn_ip4ToStr(s, _ifEth.addr));
    uart_puts(" gateway "); uart_puts(nn_ip4ToStr(s, _ifEth.gw));
    uart_puts(nn_dhcpIsBound(&_dhcp) ? " (dhcp bound)\r\n" : " (dhcp not bound)\r\n");
    tc_expectRange("dhcp bound", nn_dhcpIsBound(&_dhcp) ? 1u : 0u, 1, 1);

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
    nn_lock();
    nn_icmpOnEchoReply(NULL, NULL);         /* `net ping` has its own handler */
    nn_unlock();

    uart_puts("net: replies "); uart_putU32(_replies); uart_puts("/"); uart_putU32(PINGS);
    uart_puts(" rtt max "); uart_putU32(_rttMax); uart_puts(" ms, loopback "); uart_putU32(_loReplies);
    uart_puts("\r\n");
    tc_expectRange("replies", _replies, PINGS - 1, PINGS);     /* the first may lose the ARP race */
    tc_expectRange("loopback", _loReplies, 1, 1);
#ifdef NET_WITHOUT_NOSH
    tc_finish("net");
#else
    /* the RESULT line as tc_finish prints it, but the system stays up for the console */
    uart_puts("RESULT net "); uart_puts(tc_failures == 0 ? "PASS\r\n" : "FAIL\r\n");
    for(;;)
        rtos_delay(RTOS_MS(1000));
#endif
}

#ifndef NET_WITHOUT_NOSH
static const char *const _taskNames[] = { "idle", "main", "nosh", "net" };
#endif

int main(void)
{
    uart_init(UART_BAUD_DEFAULT);
    uart_puts("\r\nnornRTOS net\r\n");
    rtos_init();
    nn_init();
    nn_drv_loopRegister(&_lo);
    nn_ifaceAdd(&_lo, &_ifLo, nn_ip4(127, 0, 0, 1), nn_ip4(255, 0, 0, 0), nn_ipAny());
    if(nn_drv_openethRegister(&_eth, _mac) != NN_OK)
    {
        uart_puts("net: no open_eth MAC (this example runs in QEMU)\r\nRESULT net FAIL\r\n");
        for(;;) ;
    }
    nn_ifaceAdd(&_eth, &_ifEth, nn_ipAny(), nn_ipAny(), nn_ipAny());  /* DHCP fills it in */
    RTOS_TASK(TASK_MAIN, mainTask, NULL, RTOS_STACK_LARGE);
#ifndef NET_WITHOUT_NOSH
    if(nosh_enableUart(TASK_SHELL) == NOSH_OK)  /* the console UART, the default set, a 2 KB stack */
    {
        nosh_setTaskNames(_taskNames, sizeof _taskNames / sizeof *_taskNames);
#ifndef NET_NO_CMD      /* nosh without `net`: what the command alone costs (size_check) */
        nosh_add(&nn_cmd_net);
#endif
    }
#endif
    rtos_start();
    return 0;
}
