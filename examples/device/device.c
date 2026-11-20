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
 *   nornSHELL   nosh on UART0 with help, uptime, ps, mem, sched, `log`, `config` from
 *               nornStore, `wifi` from nornWIFI (status, scan, join, leave), `net` from
 *               nornNET, and one command of this application: `report` (send one now,
 *               `report status` for the counters)
 *
 * The console speaks in nornCore's log classes (hal/log.h): errors, warnings and the
 * progress lines as INFO, the request and the reply of every report as DEBUG. The level
 * is `system.loglvl` in the store: `config set system.loglvl 2` quiets everything but
 * errors and warnings at once (store_onChange) and `config save` keeps it; `log` shows
 * and sets it until the next reset, and `log wifi warning` quietens the radio alone. DEBUG is compiled in here (app.mk, LOG_CFG_MAX) so
 * that `config set system.loglvl 4` shows the traffic; by default it is not.
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
#include "log.h"
#include "board.h"
#include "nn_net.h"
#include "nn_ip.h"
#include "nn_dhcp.h"
#include "nn_tcp.h"
#include "nw.h"
#include "nw_cmd.h"
#include "nn_cmd.h"
#include "norn_http.h"
#include "norn_net_transport.h"
#include "nosh.h"
#include "store.h"
#include "store_cmd.h"
#include "nw_secrets.h"

#define SHELL_PRIO      1u
#define APP_PRIO        2u

/* ---- the configuration ------------------------------------------------------------- */

enum
{
    ID_HOSTNAME = 0x0101, ID_BOOTS = 0x0102,
    ID_SSID = 0x0201, ID_PSK = 0x0202,
    ID_SERVER_IP = 0x0301, ID_SERVER_PORT = 0x0302, ID_INTERVAL = 0x0303,
    ID_LOGLVL = 0x0103
};

static store_obj_t _objs[] =
{
    STORE_OBJ(ID_HOSTNAME,    STORE_STR, 24),
    STORE_OBJ(ID_BOOTS,       STORE_U32,  4),
    STORE_OBJ(ID_SSID,        STORE_STR, 32),
    STORE_OBJ(ID_PSK,         STORE_STR, 64),
    STORE_OBJ(ID_SERVER_IP,   STORE_IP4,  4),
    STORE_OBJ(ID_SERVER_PORT, STORE_U16,  2),
    STORE_OBJ(ID_INTERVAL,    STORE_U32,  4),
    STORE_OBJ(ID_LOGLVL,      STORE_U8,   1),
};
static uint8_t _values[140];
static const store_name_t _names[] =
{
    { ID_HOSTNAME, "system.hostname" }, { ID_BOOTS, "system.boots" },
    { ID_SSID, "wifi.ssid" }, { ID_PSK, "wifi.psk" },
    { ID_SERVER_IP, "server.ip" }, { ID_SERVER_PORT, "server.port" },
    { ID_INTERVAL, "report.interval" }, { ID_LOGLVL, "system.loglvl" },
};

/** First boot: the objects the image has no record of get the defaults of nw_secrets.h
    and are saved, so that `config` shows them and the console can change them. A value
    that came from flash is kept (store_default*). */
static void storeDefaults(void)
{
    const uint8_t ip[4] = { DEVICE_DEFAULT_SERVER_IP };

    (void)store_defaultStr(ID_SSID, DEVICE_DEFAULT_SSID);
    (void)store_defaultStr(ID_PSK, DEVICE_DEFAULT_PASSWORD);
    (void)store_default(ID_SERVER_IP, ip, 4);
    (void)store_defaultU32(ID_SERVER_PORT, DEVICE_DEFAULT_SERVER_PORT);
    (void)store_defaultU32(ID_INTERVAL, DEVICE_DEFAULT_INTERVAL_S);
    (void)store_defaultStr(ID_HOSTNAME, "norn-device");
    (void)store_defaultU32(ID_LOGLVL, LOG_INFO);
}

/** The log level lives in the store; the stored value is applied at boot and every time
    it is set (store_onChange), so `config set system.loglvl 2` acts at once. */
static void applyLogLevel(void)
{
    uint32_t v = LOG_INFO;
    (void)store_getU32(ID_LOGLVL, &v, NULL);
    log_setLevel((uint8_t)v);                   /* the application's lines and the radio's */
}

/** Modules with a level of their own, for `log wifi warning` from the console. */
static const log_module_t _logModules[] = { { "wifi", &nw_log_level } };

static void onStoreChange(uint16_t id, void *arg)
{
    (void)arg;
    if(id == ID_LOGLVL)
        applyLogLevel();
}

/** store_config.h names this as STORE_CFG_VALIDATE_HOOK: a level the log does not know
    is refused by store_set (and so by `config set`), not clamped later. */
bool deviceValidate(uint16_t id, const void *pData, uint8_t len)
{
    (void)len;
    return id != ID_LOGLVL  ||  *(const uint8_t *)pData <= LOG_DEBUG;
}

/* ---- the network and the reports ------------------------------------------------------ */

static nn_tcp_t _socket;
static uint8_t _tcpRx[1024], _tcpTx[1024];
static uint8_t _body[256];
NORN_HTTP_BUFFERS(_httpConfig, 256, 512, 384);  /* parser line, receive, request */
static norn_http _http;
static char _json[160], _hostHeader[24];

static volatile uint32_t _reportsOk, _reportsFailed;
static volatile unsigned _lastStatus;
static volatile int _lastError;

#define NOTIFY_REPORT   0x01u

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
    norn_http_net_context net;
    norn_http_transport transport;
    norn_http_body_buffer output = { _body, sizeof _body, 0 };
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

    LOG_D(uart_puts("report: POST http://"); uart_puts(_hostHeader); uart_puts("/api "); uart_puts(_json));

    if(nn_tcpOpen(&_socket, _tcpRx, sizeof _tcpRx, _tcpTx, sizeof _tcpTx) != NN_OK)
    { LOGS_E("report: TCP open failed"); return false; }
    if(nn_tcpConnect(&_socket, server, (uint16_t)port, 5000) != NN_OK)
    {
        LOG_W(uart_puts("report: TCP connect to "); uart_puts(_hostHeader);
              uart_puts(" failed (server running? address and port?)"));
        (void)nn_tcpAbort(&_socket);
        return false;
    }
    if(norn_http_net_simple(&_http, &_httpConfig, &transport, &net, &_socket, &output) != NORN_HTTP_OK)
    { (void)nn_tcpAbort(&_socket); return false; }

    norn_http_request_init(&request, "POST", "/api", _hostHeader, NULL, 0);
    (void)norn_http_add_header(&request, "Content-Type", "application/json");
    (void)norn_http_set_body(&request, _json, _jsonLen);
    request.close = true;
    result = norn_http_execute(&_http, &request, &response);
    if(result != NORN_HTTP_OK)
    {
        _lastError = result;
        LOG_E(uart_puts("report: HTTP error -"); uart_putU32((uint32_t)-result));
        (void)nn_tcpAbort(&_socket);
        return false;
    }
    _lastStatus = response.status;
    _lastError = 0;
    if(response.status == 201)
        LOG_I(uart_puts("report: HTTP 201, "); uart_putU32(_reportsOk + 1u); uart_puts(" reports"));
    else
        LOG_W(uart_puts("report: HTTP "); uart_putU32(response.status));
    LOG_D(uart_puts("report: reply "); for(i = 0; i < output.length; ++i) uart_putc((char)_body[i]));
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
    bool addressShown = false;
    (void)arg;

    if(nw_startNet() != ESP_OK) { LOGS_E("device: Wi-Fi start failed"); return; }

    /* Join with the stored credentials, without waiting: the station retries by itself,
       and `wifi join` from the console can change the network at any time. */
    (void)storedCredentials(ssid, sizeof ssid, psk, sizeof psk, NULL);
    LOG_I(uart_puts("device: joining '"); uart_puts(ssid); uart_putc('\''));
    if(nw_connectStart(ssid, psk[0] != '\0' ? psk : NULL) != ESP_OK)
        LOGS_E("device: connect start failed; fix wifi.ssid / wifi.psk with config, save, `wifi join`");

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
        /* an address: DHCP is started at the first pass, then waited for in 500 ms steps */
        {
            const esp_err_t e = nw_dhcp(500);
            if(e == ESP_FAIL) { LOGS_E("device: DHCP start failed"); return; }
            if(e != ESP_OK) continue;
        }
        if(!addressShown)
        {
            char s[16];
            LOG_I(uart_puts("device: address "); uart_puts(nn_ip4ToStr(s, nw_iface()->addr));
                  uart_puts(" gateway "); uart_puts(nn_ip4ToStr(s, nw_iface()->gw)));
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
    if(argc == 2  &&  nosh_strEqP(argv[1], NOSH_STR("status")))
    {
        nosh_putsP(NOSH_STR("reports ")); nosh_putU32(_reportsOk);
        nosh_putsP(NOSH_STR(" ok, ")); nosh_putU32(_reportsFailed);
        nosh_putsP(NOSH_STR(" failed, last HTTP status ")); nosh_putU32(_lastStatus);
        if(_lastError != 0) { nosh_putsP(NOSH_STR(", last error ")); nosh_putI32(_lastError); }
        nosh_putEol();
        return;
    }
    if(argc != 1)
    {
        nosh_putsP(NOSH_STR("usage: report [status]"));
        nosh_putEol();
        return;
    }
    rtos_notify(APP_PRIO, NOTIFY_REPORT, RTOS_NOTIFY_OR);
    nosh_putsP(NOSH_STR("report requested"));
    nosh_putEol();
}
NOSH_CMD_STATIC(_cmdReport, "report", "send a report now; `report status` for the counters", cmdReport);

static const char *const _taskNames[] = { "idle", "nosh", "device", "net", "wifi-svc", "wifi" };

/* ---- main ---------------------------------------------------------------------------- */

int main(void)
{
    uint32_t boots = 0;
    int res;

    uart_init(UART_BAUD_DEFAULT);
    uart_puts("\r\nnornRTOS device: nornCore, nornStore, nornWIFI, nornNET, nornHTTP, nornSHELL\r\n");
    rtos_init();
    nn_init();
    nw_init();

    /* the configuration first: the tasks are created from it */
    res = store_initHal(_objs, sizeof _objs / sizeof *_objs, _values, sizeof _values);
    if(res != STORE_OK) LOG_E(uart_puts("device: store_init error "); uart_putU32((uint32_t)res));
    storeDefaults();
    log_setModules(_logModules, sizeof _logModules / sizeof *_logModules);
    applyLogLevel();                            /* from here on the console follows the store */
    store_onChange(onStoreChange, NULL);
    (void)store_getU32(ID_BOOTS, &boots, NULL);
    (void)store_setU32(ID_BOOTS, boots + 1u);
    res = store_save();
    LOG_I(uart_puts("device: config seq "); uart_putU32(store_seq()); uart_puts(", boot "); uart_putU32(boots + 1u));
    if(res != STORE_OK) LOG_E(uart_puts("device: config save failed, error "); uart_putI32(res));

    RTOS_TASK(APP_PRIO, appTask, NULL, 4096);

    (void)nosh_enableUart(SHELL_PRIO);          /* the console UART, the default set */
    nosh_setTaskNames(_taskNames, sizeof _taskNames / sizeof *_taskNames);
    nosh_add(&nosh_cmd_log);
    nosh_add(&store_cmd_config);
    store_cmd_setNames(_names, sizeof _names / sizeof *_names);
    nosh_add(&nw_cmd_wifi);
    nw_cmd_setCredentials(storedCredentials, NULL);
    nosh_add(&nn_cmd_net);
    nosh_add(&_cmdReport);

    rtos_start();
    return 0;
}
