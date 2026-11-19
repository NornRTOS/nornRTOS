/* REST over Wi-Fi with nornHTTP. Join, obtain an address by DHCP, open one TCP connection
   to the demo server and make four requests on it (keep-alive): GET /hello, POST a JSON
   body to /api, a streamed GET of 64 KiB from /large through the 512-byte receive buffer
   with every byte checked in on_body, and GET /hello again with Connection: close. Prints
   each status, the small bodies, whether the connection stayed reusable, the time of
   each request and the task's stack high-water; then the task exits and the network task
   finishes the TCP shutdown. Reset the board to run it again. */
#include <string.h>
#include "rtos.h"
#include "uart.h"
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
#define HTTP_HOST HTTP_SERVER_IP ":" STRINGIFY(HTTP_SERVER_PORT)
#define LARGE_BYTES 65536ul

/* Static storage keeps buffers off the task stack. The library is buffer-free;
   NORN_HTTP_BUFFERS (the adapter's convenience) declares the parser line, the receive
   buffer and the request buffer with the library's defaults and a 5 s deadline. */
static nn_tcp_t socket;
static uint8_t tcp_rx[1024], tcp_tx[1024];
static uint8_t body[256];
NORN_HTTP_BUFFERS(config, 256, 512, 256);
static norn_http http;
static norn_http_body_buffer output = { body, sizeof(body), 0 };
static bool streaming;                      /* /large: check the bytes instead of keeping them */
static uint32_t stream_pos, stream_bad;

/* One callback for both kinds of response: the small bodies are kept in `body`, the
   large one is checked against its pattern (byte i is i & 0xFF) as it streams through. */
static int on_body(void *context, const uint8_t *data, size_t length)
{
    size_t i;
    if(!streaming) return norn_http_buffer_body(&output, data, length);
    (void)context;
    for(i = 0; i < length; ++i)
        if(data[i] != (uint8_t)(stream_pos + i)) ++stream_bad;
    stream_pos += (uint32_t)length;
    return 0;
}

static void put_body(void)
{
    size_t i;
    for(i = 0; i < output.length; ++i) uart_putc((char)body[i]);
}

/* One request on the open connection. Returns the HTTP status, or 0 on a library error. */
static unsigned request(const char *method, const char *target, const char *json, bool last)
{
    norn_http_request req;
    norn_http_response resp;
    norn_http_header headers[1];
    uint32_t t0;
    int result;

    norn_http_request_init(&req, method, target, HTTP_HOST, headers, 1);
    req.close = last;
    if(json != NULL)
    {
        (void)norn_http_add_header(&req, "Content-Type", "application/json");
        (void)norn_http_set_body(&req, json, strlen(json));
    }
    output.length = 0;
    t0 = rtos_getTimeUs();
    result = norn_http_execute(&http, &req, &resp);
    t0 = (rtos_getTimeUs() - t0) / 1000u;

    uart_puts(method); uart_putc(' '); uart_puts(target); uart_puts(" -> ");
    if(result != NORN_HTTP_OK)
    {
        uart_puts("HTTP error -"); uart_putU32((uint32_t)-result); uart_puts("\r\n");
        return 0;
    }
    uart_putU32(resp.status); uart_puts(", "); uart_putU32((uint32_t)resp.body_received);
    uart_puts(" bytes, reusable "); uart_putU32(resp.reusable); uart_puts(", ");
    uart_putU32(t0); uart_puts(" ms");
    if(!streaming  &&  output.length != 0) { uart_puts(": "); put_body(); }
    uart_puts("\r\n");
    return resp.status;
}

static void main_task(void *arg)
{
    nn_ip4_t server;
    norn_http_net_context net;
    norn_http_transport transport;
    rtos_taskInfo_t info;
    size_t stack_start, stack_connected;
    bool ok;
    (void)arg;

    (void)rtos_getTaskInfo(MAIN_TASK, &info);
    stack_start = info.stackFree;

    if(!nn_ip4Parse(HTTP_SERVER_IP, &server))
    { uart_puts("Invalid HTTP_SERVER_IP\r\n"); return; }
    if(nw_startNet() != ESP_OK)                 /* the radio and one interface on it */
    { uart_puts("Wi-Fi start failed\r\n"); return; }
    uart_puts("Connecting to Wi-Fi...\r\n");
    if(nw_connect(NW_SECRET_SSID, NW_SECRET_PASSWORD, 20000) != ESP_OK)
    { uart_puts("Wi-Fi connect failed\r\n"); return; }
    if(nw_dhcp(15000) != ESP_OK)                /* an address for it, within 15 s */
    { uart_puts("DHCP failed or timed out\r\n"); return; }

    uart_puts("http://" HTTP_HOST "\r\n");
    if(nn_tcpOpen(&socket, tcp_rx, sizeof(tcp_rx), tcp_tx, sizeof(tcp_tx)) != NN_OK)
    { uart_puts("TCP open failed\r\n"); return; }
    if(nn_tcpConnect(&socket, server, HTTP_SERVER_PORT, 5000) != NN_OK)
    {
        uart_puts("TCP connect failed: check server address and port\r\n");
        (void)nn_tcpAbort(&socket);
        return;
    }

    (void)rtos_getTaskInfo(MAIN_TASK, &info);          /* high-water so far: Wi-Fi, DHCP, TCP */
    stack_connected = info.stackFree;

    /* The transport over the connected socket, our callback as the body sink, no body
       limit beyond what the callback keeps (the large response is not stored). */
    net = (norn_http_net_context){ &socket, NULL, NULL };
    (void)norn_http_net_simple(&http, &config, &transport, &net, &socket, &output);
    config.parser.on_body = on_body;
    config.parser.max_body_bytes = 0;
    ok = norn_http_init(&http, &config, &transport) == NORN_HTTP_OK;

    ok = ok  &&  request("GET", "/hello", NULL, false) == 200;
    ok = ok  &&  request("POST", "/api", "{\"item\":42}", false) == 201;
    streaming = true;
    ok = ok  &&  request("GET", "/large", NULL, false) == 200;
    streaming = false;
    uart_puts("  /large: "); uart_putU32(stream_pos); uart_puts(" bytes streamed, ");
    uart_putU32(stream_bad); uart_puts(" bad\r\n");
    ok = ok  &&  stream_pos == LARGE_BYTES  &&  stream_bad == 0;
    ok = ok  &&  request("GET", "/hello", NULL, true) == 200;

    if(ok) (void)nn_tcpClose(&socket);
    else (void)nn_tcpAbort(&socket);

    (void)rtos_getTaskInfo(MAIN_TASK, &info);
    uart_puts("stack free: "); uart_putU32((uint32_t)stack_start); uart_puts(" at start, ");
    uart_putU32((uint32_t)stack_connected); uart_puts(" after connect, ");
    uart_putU32((uint32_t)info.stackFree); uart_puts(" after HTTP, of "); uart_putU32((uint32_t)info.stackSize);
    uart_puts("; sizeof context "); uart_putU32((uint32_t)sizeof(norn_http));
    uart_puts(" request "); uart_putU32((uint32_t)sizeof(norn_http_request));
    uart_puts(" response "); uart_putU32((uint32_t)sizeof(norn_http_response)); uart_puts("\r\n");
    uart_puts(ok ? "wifi_http: OK\r\n" : "wifi_http: FAILED\r\n");
}

int main(void)
{
    uart_init(UART_BAUD_DEFAULT);
    uart_puts("\r\nnornRTOS wifi_http\r\n");
    rtos_init();
    nn_init();
    nw_init();
    RTOS_TASK(MAIN_TASK, main_task, NULL, 4096);
    rtos_start();
    return 0;
}
