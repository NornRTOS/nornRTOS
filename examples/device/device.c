/**
 * @file device.c
 * Every module of nornRTOS in one application: a small connected device.
 *
 *   nornStore   the configuration (Wi-Fi network and password, the server's address and
 *               port, the hostname, the report interval, a boot counter) in the last two
 *               sectors of the flash; first-boot defaults from nw_secrets.h
 *   nornWIFI    joins the network with the stored credentials and keeps the link up
 *   nornNET     an address by DHCP, TCP to the server
 *   nornHTTP    POST /api with a JSON report every interval; the test server echoes it
 *   nornSHELL   nosh on UART0 with help, uptime, ps, mem, sched, `config` from nornStore,
 *               `wifi` from nornWIFI (status, scan, join, leave), and two commands of this
 *               application: `report` (send one now) and `net` (address, counters)
 *
 * Tasks: nosh 1, this application 2, nornNET 3, nornWIFI service 4 and radio 5. The
 * shell never touches the socket: `report` notifies the application task, which owns the
 * network. Everything is changed from the console, nothing needs a reboot: the hostname,
 * the interval and the server take effect at the next report, and a new network is
 * `config set wifi.ssid ...`, `config set wifi.psk ...`, `config save`, `wifi join`
 * (which reads the saved credentials through the callback this file gives the command).
 * The application task follows the link: it starts DHCP once the station is associated
 * and reports only while the link is up.
 *
 * Run from the nornRTOS root, see README.md: the secrets file, the test server, then
 *   make APP=device BOARD=esp32 upload PORT=/dev/ttyUSB0 && make APP=device BOARD=esp32 monitor
 */

#include "rtos.h"
#include "uart.h"
#include "board.h"
#include "nn_net.h"
#include "nn_ip.h"
#include "nn_dhcp.h"
#include "nn_tcp.h"
#include "nw.h"
#include "nw_cmd.h"
#include "norn_http.h"
#include "norn_net_transport.h"
#include "nosh.h"
#include "store.h"
#include "store_cmd.h"
#include "flash.h"
#include "nw_secrets.h"

#define SHELL_PRIO      1u
#define APP_PRIO        2u

/* ---- the configuration ------------------------------------------------------------- */

enum
{
    ID_HOSTNAME = 0x0101, ID_BOOTS = 0x0102,
    ID_SSID = 0x0201, ID_PSK = 0x0202,
    ID_SERVER_IP = 0x0301, ID_SERVER_PORT = 0x0302, ID_INTERVAL = 0x0303
};

static store_obj_t _objs[] =
{
    { ID_HOSTNAME,    STORE_STR, 1, 24, 0, 0, 0, 0 },
    { ID_BOOTS,       STORE_U32, 1,  4, 0, 0, 0, 0 },
    { ID_SSID,        STORE_STR, 1, 32, 0, 0, 0, 0 },
    { ID_PSK,         STORE_STR, 1, 64, 0, 0, 0, 0 },
    { ID_SERVER_IP,   STORE_IP4, 1,  4, 0, 0, 0, 0 },
    { ID_SERVER_PORT, STORE_U16, 1,  2, 0, 0, 0, 0 },
    { ID_INTERVAL,    STORE_U32, 1,  4, 0, 0, 0, 0 },
};
static uint8_t _values[140];
static const store_name_t _names[] =
{
    { ID_HOSTNAME, "system.hostname" }, { ID_BOOTS, "system.boots" },
    { ID_SSID, "wifi.ssid" }, { ID_PSK, "wifi.psk" },
    { ID_SERVER_IP, "server.ip" }, { ID_SERVER_PORT, "server.port" },
    { ID_INTERVAL, "report.interval" },
};
static const store_flash_t _flash =
{
    hal_flashRead, hal_flashWrite, hal_flashEraseBlock,
    HAL_FLASH_BLOCK_SIZE, HAL_FLASH_BLOCKS, HAL_FLASH_WRITE_ALIGN
};

/** First boot: the objects the image has no record of get the defaults of nw_secrets.h
    and are saved, so that `config` shows them and the console can change them. */
static void storeDefaults(void)
{
    const uint8_t ip[4] = { DEVICE_DEFAULT_SERVER_IP };
    bool set;
    uint32_t v;

    if(!(store_objById(ID_SSID)->flags & STORE_F_FROM_FLASH))
        (void)store_setStr(ID_SSID, DEVICE_DEFAULT_SSID);
    if(!(store_objById(ID_PSK)->flags & STORE_F_FROM_FLASH))
        (void)store_setStr(ID_PSK, DEVICE_DEFAULT_PASSWORD);
    if(!(store_objById(ID_SERVER_IP)->flags & STORE_F_FROM_FLASH))
        (void)store_set(ID_SERVER_IP, ip, 4);
    if(store_getU32(ID_SERVER_PORT, &v, &set) == STORE_OK  &&  !set)
        (void)store_setU32(ID_SERVER_PORT, DEVICE_DEFAULT_SERVER_PORT);
    if(store_getU32(ID_INTERVAL, &v, &set) == STORE_OK  &&  !set)
        (void)store_setU32(ID_INTERVAL, DEVICE_DEFAULT_INTERVAL_S);
    if(!(store_objById(ID_HOSTNAME)->flags & STORE_F_FROM_FLASH))
        (void)store_setStr(ID_HOSTNAME, "norn-device");
}

/* ---- the network and the reports ------------------------------------------------------ */

static uint8_t _appStack[4096];
static nn_iface_t _iface;
static nn_dhcp_t _dhcp;
static rtos_sem_t _dhcpReady = RTOS_SEM_INIT(0);
static nn_tcp_t _socket;
static uint8_t _tcpRx[1024], _tcpTx[1024];
static char _line[256], _requestBytes[384];
static uint8_t _receive[512], _body[256];
static norn_http _http;
static char _json[160], _hostHeader[24];

static volatile uint32_t _reportsOk, _reportsFailed;
static volatile unsigned _lastStatus;
static volatile int _lastError;

#define NOTIFY_REPORT   0x01u

static void onDhcp(nn_dhcp_t *pClient, bool bound, void *arg)
{
    (void)pClient; (void)arg;
    if(bound)
        rtos_semGive(&_dhcpReady);
}

static uint32_t nowMs(void *arg)
{
    (void)arg;
    return nn_os_now();
}

/* A few bytes of string building, since the application owns the JSON (nornHTTP does
   not): append a string or a number to _json, never past its end. */
static size_t _jsonLen;
static void jsonPut(const char *s)
{
    while(*s != '\0'  &&  _jsonLen < sizeof _json - 1u)
        _json[_jsonLen++] = *s++;
    _json[_jsonLen] = '\0';
}
static void jsonPutU32(uint32_t n)
{
    char buf[11];
    uint8_t i = sizeof buf - 1u;
    buf[i] = '\0';
    do { buf[--i] = (char)('0' + n % 10u); n /= 10u; } while(n != 0);
    jsonPut(&buf[i]);
}
static void jsonPutI32(int32_t n)
{
    if(n < 0) { jsonPut("-"); jsonPutU32(0u - (uint32_t)n); }
    else jsonPutU32((uint32_t)n);
}

/** The report of this moment: who, how many boots, how long up, how loaded, how strong. */
static void buildReport(void)
{
    char host[25];
    uint32_t boots = 0;
    (void)store_getStr(ID_HOSTNAME, host, sizeof host);
    (void)store_getU32(ID_BOOTS, &boots, NULL);
    _jsonLen = 0;
    jsonPut("{\"host\":\""); jsonPut(host);
    jsonPut("\",\"boots\":"); jsonPutU32(boots);
    jsonPut(",\"uptime_s\":"); jsonPutU32((uint32_t)rtos_getTick() / (uint32_t)RTOS_CFG_TICK_HZ);
    jsonPut(",\"load\":"); jsonPutU32(rtos_getCpuLoad());
    jsonPut(",\"rssi\":"); jsonPutI32(nw_rssi());
    jsonPut(",\"reports\":"); jsonPutU32(_reportsOk);
    jsonPut("}");
}

/** One report: connect, POST /api with the JSON, print what came back, close. */
static bool report(void)
{
    norn_http_net_context net = { &_socket, nowMs, NULL };
    norn_http_transport transport;
    norn_http_body_buffer output = { _body, sizeof _body, 0 };
    norn_http_config config = {0};
    norn_http_request request;
    norn_http_response response;
    uint8_t ip[4];
    uint32_t port = 0;
    nn_ip4_t server;
    char ipStr[16];
    int result;
    size_t i;

    (void)store_get(ID_SERVER_IP, ip, sizeof ip, NULL);
    (void)store_getU32(ID_SERVER_PORT, &port, NULL);
    server = nn_ip4(ip[0], ip[1], ip[2], ip[3]);
    /* Host header "a.b.c.d:port", built from the stored values */
    {
        size_t n = 0;
        const char *p = nn_ip4ToStr(ipStr, server);
        while(*p != '\0') _hostHeader[n++] = *p++;
        _hostHeader[n++] = ':';
        _jsonLen = 0; jsonPutU32(port);                 /* borrow the JSON builder */
        for(p = _json; *p != '\0'; ) _hostHeader[n++] = *p++;
        _hostHeader[n] = '\0';
    }
    buildReport();

    uart_puts("report: POST http://"); uart_puts(_hostHeader); uart_puts("/api ");
    uart_puts(_json); uart_puts("\r\n");

    if(nn_tcpOpen(&_socket, _tcpRx, sizeof _tcpRx, _tcpTx, sizeof _tcpTx) != NN_OK)
    { uart_puts("report: TCP open failed\r\n"); return false; }
    if(nn_tcpConnect(&_socket, server, (uint16_t)port, 5000) != NN_OK)
    {
        uart_puts("report: TCP connect failed (server running? address and port?)\r\n");
        (void)nn_tcpAbort(&_socket);
        return false;
    }
    if(norn_http_net_transport_init(&transport, &net) != 0) { (void)nn_tcpAbort(&_socket); return false; }
    norn_http_parser_config_init(&config.parser, _line, sizeof _line);
    config.parser.on_body = norn_http_buffer_body;
    config.parser.callback_context = &output;
    config.parser.max_body_bytes = sizeof _body;
    config.receive_buffer = _receive;      config.receive_capacity = sizeof _receive;
    config.request_buffer = _requestBytes; config.request_capacity = sizeof _requestBytes;
    config.timeout_ms = 5000;
    if(norn_http_init(&_http, &config, &transport) != NORN_HTTP_OK) { (void)nn_tcpAbort(&_socket); return false; }

    norn_http_request_init(&request, "POST", "/api", _hostHeader, NULL, 0);
    (void)norn_http_add_header(&request, "Content-Type", "application/json");
    (void)norn_http_set_body(&request, _json, _jsonLen);
    request.close = true;
    result = norn_http_execute(&_http, &request, &response);
    if(result != NORN_HTTP_OK)
    {
        _lastError = result;
        uart_puts("report: HTTP error -"); uart_putU32((uint32_t)-result); uart_puts("\r\n");
        (void)nn_tcpAbort(&_socket);
        return false;
    }
    _lastStatus = response.status;
    _lastError = 0;
    uart_puts("report: HTTP "); uart_putU32(response.status); uart_puts(" ");
    for(i = 0; i < output.length; ++i) uart_putc((char)_body[i]);
    uart_puts("\r\n");
    (void)nn_tcpClose(&_socket);
    return response.status == 201;
}

/** `wifi join` without arguments: the credentials the store holds. The command knows
    nothing of nornStore; this is where the application wires the two. */
static bool storedCredentials(char *ssid, size_t ssidSize, char *psk, size_t pskSize, void *arg)
{
    (void)arg;
    return store_getStr(ID_SSID, ssid, (uint8_t)ssidSize) == STORE_OK
           &&  store_getStr(ID_PSK, psk, (uint8_t)pskSize) == STORE_OK;
}

static void appTask(void *arg)
{
    char ssid[33], psk[65];
    bool dhcpStarted = false, addressShown = false;
    (void)arg;

    if(nw_start() != ESP_OK) { uart_puts("device: Wi-Fi start failed\r\n"); return; }
    if(nn_ifaceAdd(nw_netdev(), &_iface, nn_ip4(0,0,0,0), nn_ip4(0,0,0,0), nn_ip4(0,0,0,0)) != NN_OK)
    { uart_puts("device: interface failed\r\n"); return; }

    /* Join with the stored credentials, without waiting: the station retries by itself,
       and `wifi join` from the console can change the network at any time. */
    (void)storedCredentials(ssid, sizeof ssid, psk, sizeof psk, NULL);
    uart_puts("device: joining '"); uart_puts(ssid); uart_puts("'\r\n");
    if(nw_connectStart(ssid, psk[0] != '\0' ? psk : NULL) != ESP_OK)
        uart_puts("device: connect start failed; fix wifi.ssid / wifi.psk with config, save, `wifi join`\r\n");

    for(;;)
    {
        uint32_t intervalS = DEVICE_DEFAULT_INTERVAL_S;
        uint16_t notified;

        if(!nw_isConnected())
        {
            /* down: look again soon; nornNET follows the link and DHCP confirms or renews
               the address by itself once the station is back */
            (void)rtos_notifyWait(&notified, RTOS_MS(500));
            continue;
        }
        if(!dhcpStarted)
        {
            if(nn_dhcpStart(&_dhcp, &_iface, onDhcp, NULL) != NN_OK) { uart_puts("device: DHCP start failed\r\n"); return; }
            dhcpStarted = true;
        }
        if(!nn_dhcpIsBound(&_dhcp))
        {
            (void)rtos_semTake(&_dhcpReady, RTOS_MS(500));
            continue;
        }
        if(!addressShown)
        {
            char s[16];
            uart_puts("device: address "); uart_puts(nn_ip4ToStr(s, _iface.addr));
            uart_puts(" gateway "); uart_puts(nn_ip4ToStr(s, _iface.gw)); uart_puts("\r\n");
            addressShown = true;
        }

        if(report()) ++ _reportsOk; else ++ _reportsFailed;
        (void)store_getU32(ID_INTERVAL, &intervalS, NULL);
        if(intervalS == 0) intervalS = 1;
        if(intervalS > 3600) intervalS = 3600;
        /* until the next interval, or until `report` on the console says now */
        (void)rtos_notifyWait(&notified, RTOS_MS(intervalS * 1000ul));
    }
}

/* ---- the application's console commands ----------------------------------------------- */

static void cmdReport(uint8_t argc, const char *const argv[])
{
    (void)argc; (void)argv;
    rtos_notify(APP_PRIO, NOTIFY_REPORT, RTOS_NOTIFY_OR);
    nosh_putsP(NOSH_STR("report requested"));
    nosh_putEol();
}
NOSH_CMD_STATIC(_cmdReport, "report", "send a report to the server now", cmdReport);

static void cmdNet(uint8_t argc, const char *const argv[])
{
    char s[16];
    (void)argc; (void)argv;
    nosh_putsP(NOSH_STR("address ")); nosh_puts(nn_ip4ToStr(s, _iface.addr));
    nosh_putsP(NOSH_STR(" mask ")); nosh_puts(nn_ip4ToStr(s, _iface.mask));
    nosh_putsP(NOSH_STR(" gateway ")); nosh_puts(nn_ip4ToStr(s, _iface.gw));
    nosh_putsP(nn_dhcpIsBound(&_dhcp) ? NOSH_STR(" (dhcp bound)") : NOSH_STR(" (dhcp pending)"));
    nosh_putEol();
    nosh_putsP(NOSH_STR("reports ")); nosh_putU32(_reportsOk);
    nosh_putsP(NOSH_STR(" ok, ")); nosh_putU32(_reportsFailed);
    nosh_putsP(NOSH_STR(" failed, last HTTP status ")); nosh_putU32(_lastStatus);
    if(_lastError != 0) { nosh_putsP(NOSH_STR(", last error ")); nosh_putI32(_lastError); }
    nosh_putEol();
}
NOSH_CMD_STATIC(_cmdNet, "net", "the address and the report counters", cmdNet);

/* ---- the shell's transport: UART0 by interrupt into a queue ------------------------- */

static uint8_t _rxStorage[96];
static rtos_queue_t _rxQueue;
static void uartRx(uint8_t c, void *arg) { (void)arg; (void)rtos_queueSendFromIsr(&_rxQueue, &c); }
static size_t ioRead(void *buf, size_t n) { (void)n; return rtos_queueReceive(&_rxQueue, buf, RTOS_FOREVER) ? 1u : 0u; }
static void ioWrite(const void *buf, size_t n) { const char *p = buf; while(n-- > 0) uart_putc(*p++); }
static const nosh_io_t _uartIo = { ioRead, ioWrite };
static const char *const _taskNames[] = { "idle", "nosh", "device", "net", "wifi-svc", "wifi" };

/* ---- hooks and main ------------------------------------------------------------------ */

void rtos_assertFailed(const char *file, uint16_t lineNumber)
{
    (void)rtos_port_enterCritical();
    uart_puts("ASSERT: "); uart_putsP(file); uart_putc(':'); uart_putU32(lineNumber);
    uart_puts("\r\n"); rtos_port_halt();
}

void rtos_panicHook(const rtos_panicInfo_t *pInfo)
{
    uart_puts("\r\nPANIC reason "); uart_putU32(pInfo->reason);
    uart_puts(" task "); uart_putU32(pInfo->task);
    uart_puts(" arg1 "); uart_putU32(pInfo->arg1);
    uart_puts(" arg2 "); uart_putU32(pInfo->arg2); uart_puts("\r\n");
    uart_flush();
}

int main(void)
{
    uint32_t boots = 0;
    int res;

    uart_init(115200);
    uart_puts("\r\nnornRTOS device: nornCore, nornStore, nornWIFI, nornNET, nornHTTP, nornSHELL\r\n");
    rtos_init();
    nn_init();
    nw_init(NW_HEAP_SPARE_START, NW_HEAP_SPARE_SIZE);

    /* the configuration first: the tasks are created from it */
    (void)hal_flashInit();
    res = store_init(&_flash, _objs, sizeof _objs / sizeof *_objs, _values, sizeof _values);
    if(res != STORE_OK) { uart_puts("device: store_init error "); uart_putU32((uint32_t)res); uart_puts("\r\n"); }
    storeDefaults();
    (void)store_getU32(ID_BOOTS, &boots, NULL);
    (void)store_setU32(ID_BOOTS, boots + 1u);
    res = store_save();
    uart_puts("device: config seq "); uart_putU32(store_seq());
    uart_puts(", boot "); uart_putU32(boots + 1u);
    uart_puts(res == STORE_OK ? "\r\n" : ", save failed\r\n");

    rtos_createTask(APP_PRIO, appTask, NULL, _appStack, sizeof _appStack);

    rtos_queueInit(&_rxQueue, _rxStorage, 1, sizeof _rxStorage);
    uart_onReceive(uartRx, NULL);
    NOSH_ENABLE_DEFAULT(&_uartIo, SHELL_PRIO, 2048);
    nosh_setTaskNames(_taskNames, sizeof _taskNames / sizeof *_taskNames);
    nosh_add(&store_cmd_config);
    store_cmd_setNames(_names, sizeof _names / sizeof *_names);
    nosh_add(&nw_cmd_wifi);
    nw_cmd_setCredentials(storedCredentials, NULL);
    nosh_add(&_cmdReport);
    nosh_add(&_cmdNet);

    rtos_start();
    return 0;
}
