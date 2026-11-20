/* Over the air update over Wi-Fi (hal/ota.h, an OTA build: make OTA=1). On every boot the
   application says which slot it runs from and in which state. It joins the network, and
   if the running slot is a trial it confirms it there (the proof of life: the network is
   reachable). Then it asks the host for /update.nota; a 200 is streamed straight into
   ota_write(), ota_end() checks the hash and marks the other slot as the one to try, and
   the board resets into it. A 404 means nothing to do, and the task ends.

   Built with -DOTA_NEVER_CONFIRM the image never confirms and resets itself after two
   seconds instead: the selector boots it OTA_TRIALS times, rejects it and goes back. That
   is the roll back test. tools/ota_serve_once.py on the host serves the file once, so the
   board does not fetch the same image again after the reset.

   The test (tools/ota_test.sh): slot A runs; the host serves a normal image: B is written,
   booted as a trial, confirmed, "RESULT ota PASS" from slot 1. Then the host serves the
   never-confirming image: A is written, booted three times, rejected, and B, confirmed,
   prints the same PASS after the roll back. */
#include <string.h>
#include "rtos.h"
#include "uart.h"
#include "ota.h"
#include "nn_net.h"
#include "nn_ip.h"
#include "nn_dhcp.h"
#include "nw.h"
#include "nw_secrets.h"
#include "norn_http.h"
#include "norn_net_transport.h"

#define MAIN_TASK 1u
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define HTTP_HOST OTA_SERVER_IP ":" STRINGIFY(OTA_SERVER_PORT)

static nn_tcp_t socket;
static uint8_t tcp_rx[2048], tcp_tx[512];
NORN_HTTP_BUFFERS(config, 256, 1024, 256);
static norn_http http;
static norn_http_body_buffer sink = { NULL, 0, 0 };
static int write_error;

static int on_body(void *context, const uint8_t *data, size_t length)
{
    (void)context;
    if(write_error == OTA_OK)
        write_error = ota_write(data, length);
    return write_error == OTA_OK ? 0 : 1;      /* a nonzero return stops the transfer */
}

static void say_slot(void)
{
    const int slot = ota_runningSlot(), state = ota_runningState();
    uart_puts("ota: running slot ");
    if(slot < 0) { uart_puts("none (no selector)\r\n"); return; }
    uart_putU32((uint32_t)slot);
    uart_puts(state == OTA_STATE_TRIAL ? " (trial)" : state == OTA_STATE_CONFIRMED ? " (confirmed)" : " (state ?)");
    uart_puts("\r\n");
}

static void main_task(void *arg)
{
    nn_ip4_t server;
    norn_http_net_context net;
    norn_http_transport transport;
    norn_http_request req;
    norn_http_response resp;
    norn_http_header headers[1];
    uint32_t t0;
    int result, slot;
    (void)arg;

    say_slot();
    slot = ota_runningSlot();
    if(slot < 0) { uart_puts("ota: build with OTA=1 and upload the selector\r\nRESULT ota FAIL\r\n"); return; }

    if(!nn_ip4Parse(OTA_SERVER_IP, &server)) { uart_puts("ota: bad OTA_SERVER_IP\r\n"); return; }
    if(nw_startNet() != ESP_OK) { uart_puts("ota: Wi-Fi start failed\r\n"); return; }
    if(nw_connect(NW_SECRET_SSID, NW_SECRET_PASSWORD, 20000) != ESP_OK) { uart_puts("ota: Wi-Fi connect failed\r\n"); return; }
    if(nw_dhcp(15000) != ESP_OK) { uart_puts("ota: DHCP failed\r\n"); return; }

#ifdef OTA_NEVER_CONFIRM
    uart_puts("ota: this image never confirms; resetting in 2 s\r\n");
    rtos_delay(RTOS_MS(2000));
    rtos_port_reset();
#else
    if(ota_runningState() == OTA_STATE_TRIAL)
    {
        result = ota_confirm();
        uart_puts(result == OTA_OK ? "ota: confirmed\r\n" : "ota: confirm failed\r\n");
        if(result == OTA_OK  &&  slot == 1)
            uart_puts("RESULT ota PASS\r\n");           /* B runs and is confirmed: the update, or the roll back, held */
    }
    else if(slot == 1)
        uart_puts("RESULT ota PASS\r\n");
#endif

    uart_puts("ota: GET http://" HTTP_HOST "/update.nota\r\n");
    if(nn_tcpOpen(&socket, tcp_rx, sizeof(tcp_rx), tcp_tx, sizeof(tcp_tx)) != NN_OK) { uart_puts("ota: TCP open failed\r\n"); return; }
    if(nn_tcpConnect(&socket, server, OTA_SERVER_PORT, 5000) != NN_OK)
    { uart_puts("ota: no server\r\n"); (void)nn_tcpAbort(&socket); return; }

    net = (norn_http_net_context){ &socket, NULL, NULL };
    (void)norn_http_net_simple(&http, &config, &transport, &net, &socket, &sink);
    config.parser.on_body = on_body;
    config.parser.max_body_bytes = 0;
    config.timeout_ms = 60000;
    if(norn_http_init(&http, &config, &transport) != NORN_HTTP_OK) { uart_puts("ota: http init failed\r\n"); return; }

    write_error = ota_begin();
    if(write_error != OTA_OK) { uart_puts("ota: begin failed\r\n"); return; }
    norn_http_request_init(&req, "GET", "/update.nota", HTTP_HOST, headers, 1);
    req.close = true;
    t0 = rtos_getTimeUs();
    result = norn_http_execute(&http, &req, &resp);
    t0 = (rtos_getTimeUs() - t0) / 1000u;
    (void)nn_tcpClose(&socket);

    if(result != NORN_HTTP_OK  &&  write_error != OTA_OK)
    { uart_puts("ota: write failed, code "); uart_putU32((uint32_t)-write_error); uart_puts("\r\n"); ota_abort(); return; }
    if(result != NORN_HTTP_OK)
    { uart_puts("ota: HTTP error -"); uart_putU32((uint32_t)-result); uart_puts("\r\n"); ota_abort(); return; }
    if(resp.status != 200)
    { uart_puts("ota: no update (status "); uart_putU32(resp.status); uart_puts(")\r\n"); ota_abort(); return; }

    uart_puts("ota: "); uart_putU32(ota_written()); uart_puts(" image bytes written in ");
    uart_putU32(t0); uart_puts(" ms\r\n");
    result = ota_end();
    if(result != OTA_OK) { uart_puts("ota: end failed, code "); uart_putU32((uint32_t)-result); uart_puts("\r\n"); return; }
    uart_puts("ota: slot "); uart_putU32((uint32_t)(slot ^ 1)); uart_puts(" is the trial; resetting\r\n");
    uart_flush();
    rtos_delay(RTOS_MS(200));
    rtos_port_reset();
}

int main(void)
{
    uart_init(UART_BAUD_DEFAULT);
    uart_puts("\r\nnornRTOS ota\r\n");
    rtos_init();
    nn_init();
    nw_init();
    RTOS_TASK(MAIN_TASK, main_task, NULL, 4096);
    rtos_start();
    return 0;
}
