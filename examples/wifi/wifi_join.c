/**
 * @file wifi_join.c
 * nornWIFI: start the radio, join the access point named in nw_secrets.h, take an
 * address by DHCP over nornNET, ping the gateway twenty times and report. The task
 * priorities of the modules are in nn_config.h and rtos_config.h here.
 */

#include "rtos.h"
#include "uart.h"
#include "testresult.h"
#include "nn_net.h"
#include "nn_ip.h"
#include "nn_icmp.h"
#include "nn_dhcp.h"
#include "nn_arp.h"
#include "nw.h"
#include "nw_secrets.h"

#define TASK_MAIN   1u
#ifndef PINGS
# define PINGS      20u
#endif
#define PING_ID     0x5731u

static uint8_t _stack[RTOS_STACK_LARGE];
static nn_iface_t _if;
static nn_dhcp_t _dhcp;
static rtos_sem_t _dhcpSem = RTOS_SEM_INIT(0);
static volatile uint32_t _replies, _rttMax;
static volatile uint32_t _sentAt;

static void onDhcp(nn_dhcp_t *pD, bool bound, void *arg)
{
    (void)pD; (void)arg;
    if(bound)
        rtos_semGive(&_dhcpSem);
}

static void onReply(nn_ip4_t from, uint16_t id, uint16_t seq, const uint8_t *p, uint16_t len, void *arg)
{
    (void)from; (void)seq; (void)p; (void)len; (void)arg;
    if(id == PING_ID)
    {
        const uint32_t rtt = (rtos_getTimeUs() - _sentAt) / 1000u;
        ++ _replies;
        if(rtt > _rttMax) _rttMax = rtt;
        rtos_notify(TASK_MAIN, 1, RTOS_NOTIFY_OR);
    }
}

static void putIp(nn_ip4_t a)
{
    uart_putU32(a.b[0]); uart_putc('.'); uart_putU32(a.b[1]); uart_putc('.');
    uart_putU32(a.b[2]); uart_putc('.'); uart_putU32(a.b[3]);
}

static void mainTask(void *arg)
{
    esp_err_t err;
    uint32_t i;
    (void)arg;

    tc_expect("nw_start", nw_start() == ESP_OK);
    nn_ifaceAdd(nw_netdev(), &_if, nn_ip4(0, 0, 0, 0), nn_ip4(0, 0, 0, 0), nn_ip4(0, 0, 0, 0));
    err = nw_connect(NW_SECRET_SSID, NW_SECRET_PASSWORD, 20000);
    uart_puts("wifi: connect -> 0x"); uart_putHex8((uint8_t)(err >> 8)); uart_putHex8((uint8_t)err);
    uart_puts(", rssi -"); uart_putU32((uint32_t)-nw_rssi()); uart_puts(" dBm\r\n");
    tc_expect("connected", err == ESP_OK);
    if(err == ESP_OK)
    {
        tc_expect("dhcpStart", nn_dhcpStart(&_dhcp, &_if, onDhcp, NULL) == NN_OK);
        tc_expect("dhcp bound", rtos_semTake(&_dhcpSem, RTOS_MS(15000)));
        uart_puts("wifi: address "); putIp(_if.addr); uart_puts(" gateway "); putIp(_if.gw); uart_puts("\r\n");
        nn_lock();
        nn_icmpOnEchoReply(onReply, NULL);
        nn_unlock();
        /* Warm up, not measured: the first unicast frame after joining resolves the
           gateway's address and is now and then lost by the access point while its
           block-ack session with the station is being set up (measured 2026-11-04: the
           first ping in about one run of three, nothing else). */
        nn_lock();
        _sentAt = rtos_getTimeUs();
        nn_icmpEchoRequest(_if.gw, PING_ID, 0, NULL, 32);
        nn_unlock();
        rtos_notifyTake(RTOS_MS(1000));
        _replies = 0; _rttMax = 0;
        for(i=1; i<=PINGS; ++i)
        {
            const uint32_t before = _replies;
            nn_lock();
            _sentAt = rtos_getTimeUs();
            nn_icmpEchoRequest(_if.gw, PING_ID, (uint16_t)i, NULL, 32);
            nn_unlock();
            rtos_notifyTake(RTOS_MS(1000));
            uart_puts("wifi: ping "); uart_putU32(i);
            if(_replies > before) { uart_puts(" reply after "); uart_putU32((rtos_getTimeUs() - _sentAt) / 1000u); uart_puts(" ms\r\n"); }
            else uart_puts(" no reply within 1000 ms\r\n");
            rtos_delay(RTOS_MS(200));
        }
        uart_puts("wifi: ping "); uart_putU32(_replies); uart_puts("/"); uart_putU32(PINGS);
        uart_puts(" rtt max "); uart_putU32(_rttMax); uart_puts(" ms\r\n");
        {
            nn_arpStats_t as; nn_icmpStats_t cs; nn_bufStats_t bs; nw_stats_t ws;
            const nn_netdev_t *pDev = nw_netdev();
            nn_lock(); nn_arpGetStats(&as); nn_icmpGetStats(&cs); nn_bufGetStats(&bs); nn_unlock();
            nw_getStats(&ws);
            uart_puts("wifi: arp req "); uart_putU32(as.txRequests); uart_puts(" resolved "); uart_putU32(as.resolved);
            uart_puts(" timeouts "); uart_putU32(as.timeouts);
            uart_puts(" | icmp txReq "); uart_putU32(cs.txEchoRequests); uart_puts(" rxRep "); uart_putU32(cs.rxEchoReplies);
            uart_puts(" | dev rx "); uart_putU32(pDev->stats.rxFrames); uart_puts(" drop "); uart_putU32(pDev->stats.rxDropped);
            uart_puts(" tx "); uart_putU32(pDev->stats.txFrames); uart_puts(" err "); uart_putU32(pDev->stats.txErrors);
            uart_puts(" | bufs min "); uart_putU32(bs.minFree); uart_puts("/"); uart_putU32(bs.numBufs);
            uart_puts(" failed "); uart_putU32(bs.allocFailed);
            uart_puts(" | wifi rxDropped "); uart_putU32(ws.rxDropped); uart_puts("\r\n");
        }
        tc_expectRange("replies", _replies, PINGS - 1, PINGS);     /* one 100 ms stall is the AP's */
        nn_dhcpStop(&_dhcp);
        (void)nw_disconnect();
    }
    tc_finish("wifi");
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
    uart_puts("\r\nnornRTOS wifi\r\n");
    rtos_init();
    nn_init();
    nw_init(NW_HEAP_SPARE_START, NW_HEAP_SPARE_SIZE);
    rtos_createTask(TASK_MAIN, mainTask, NULL, _stack, sizeof(_stack));
    rtos_start();
    return 0;
}
