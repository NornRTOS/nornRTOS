/* Join Wi-Fi, obtain DHCP, GET /hello once and print the response. */
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

/* Static storage keeps buffers off the task stack. No HTTP heap allocation. */
static uint8_t stack[4096];
static nn_iface_t interface;
static nn_dhcp_t dhcp;
static rtos_sem_t dhcp_ready = RTOS_SEM_INIT(0);
static nn_tcp_t socket;
static uint8_t tcp_rx[1024], tcp_tx[1024];
static char line[256], request_bytes[256];
static uint8_t receive[512], body[256];
static norn_http http;

static void on_dhcp(nn_dhcp_t *client, bool bound, void *arg)
{
    (void)client; (void)arg;
    if(bound) rtos_semGive(&dhcp_ready);
}

static uint32_t now_ms(void *arg)
{
    (void)arg;
    return nn_os_now();
}

/* The complete HTTP operation: attach transport, choose storage, execute GET. */
static bool get_hello(void)
{
    norn_http_net_context net = { &socket, now_ms, NULL };
    norn_http_transport transport;
    norn_http_body_buffer output = { body, sizeof(body), 0 };
    norn_http_config config = {0};
    norn_http_request request;
    norn_http_response response;
    int result;
    size_t i;

    if(norn_http_net_transport_init(&transport, &net) != 0) return false;
    norn_http_parser_config_init(&config.parser, line, sizeof(line));
    config.parser.on_body = norn_http_buffer_body;
    config.parser.callback_context = &output;
    config.parser.max_body_bytes = sizeof(body);
    config.receive_buffer = receive;
    config.receive_capacity = sizeof(receive);
    config.request_buffer = request_bytes;
    config.request_capacity = sizeof(request_bytes);
    config.timeout_ms = 5000;
    if(norn_http_init(&http, &config, &transport) != NORN_HTTP_OK) return false;

    norn_http_request_init(&request, "GET", "/hello", HTTP_HOST, NULL, 0);
    request.close = true; /* This example makes just one request. */
    result = norn_http_execute(&http, &request, &response);
    if(result != NORN_HTTP_OK)
    {
        uart_puts("HTTP error -"); uart_putU32((uint32_t)-result); uart_puts("\r\n");
        return false;
    }
    uart_puts("HTTP status: "); uart_putU32(response.status); uart_puts("\r\n");
    /* Body is a byte span, not a NUL-terminated string or parsed JSON. */
    for(i = 0; i < output.length; ++i) uart_putc((char)body[i]);
    uart_puts("\r\n");
    return response.status == 200;
}

static void main_task(void *arg)
{
    nn_ip4_t server;
    bool ok;
    (void)arg;

    if(!nn_ip4Parse(HTTP_SERVER_IP, &server))
    { uart_puts("Invalid HTTP_SERVER_IP\r\n"); return; }
    if(nw_start() != ESP_OK)
    { uart_puts("Wi-Fi start failed\r\n"); return; }
    if(nn_ifaceAdd(nw_netdev(), &interface, nn_ip4(0,0,0,0),
                   nn_ip4(0,0,0,0), nn_ip4(0,0,0,0)) != NN_OK)
    { uart_puts("Network interface failed\r\n"); return; }
    uart_puts("Connecting to Wi-Fi...\r\n");
    if(nw_connect(NW_SECRET_SSID, NW_SECRET_PASSWORD, 20000) != ESP_OK)
    { uart_puts("Wi-Fi connect failed\r\n"); return; }
    if(nn_dhcpStart(&dhcp, &interface, on_dhcp, NULL) != NN_OK)
    { uart_puts("DHCP start failed\r\n"); return; }
    if(!rtos_semTake(&dhcp_ready, RTOS_MS(15000)))
    { uart_puts("DHCP timed out\r\n"); return; }

    uart_puts("GET http://" HTTP_HOST "/hello\r\n");
    if(nn_tcpOpen(&socket, tcp_rx, sizeof(tcp_rx), tcp_tx, sizeof(tcp_tx)) != NN_OK)
    { uart_puts("TCP open failed\r\n"); return; }
    if(nn_tcpConnect(&socket, server, HTTP_SERVER_PORT, 5000) != NN_OK)
    {
        uart_puts("TCP connect failed: check server address and port\r\n");
        (void)nn_tcpAbort(&socket);
        return;
    }
    ok = get_hello();
    if(ok) (void)nn_tcpClose(&socket);
    else (void)nn_tcpAbort(&socket);
    uart_puts(ok ? "wifi_http: OK\r\n" : "wifi_http: FAILED\r\n");
    /* Task returns; the network task remains alive to complete TCP shutdown. */
}

void rtos_assertFailed(const char *file, uint16_t line_number)
{
    (void)rtos_port_enterCritical();
    uart_puts("ASSERT: "); uart_putsP(file); uart_putc(':'); uart_putU32(line_number);
    uart_puts("\r\n"); rtos_port_halt();
}

void rtos_panicHook(const rtos_panicInfo_t *info)
{
    uart_puts("PANIC reason "); uart_putU32(info->reason);
    uart_puts(" task "); uart_putU32(info->task); uart_puts("\r\n"); uart_flush();
}

int main(void)
{
    uart_init(115200);
    uart_puts("\r\nnornRTOS wifi_http\r\n");
    rtos_init();
    nn_init();
    nw_init(NW_HEAP_SPARE_START, NW_HEAP_SPARE_SIZE);
    rtos_createTask(MAIN_TASK, main_task, NULL, stack, sizeof(stack));
    rtos_start();
    return 0;
}
