# HTTP over Wi-Fi with nornHTTP

A minimal ESP32 example: connect to Wi-Fi, obtain an IP address using DHCP,
make **one `GET /hello` request**, and print the status code and response over
UART. The application task then exits. The network task stays alive to complete
TCP shutdown. Reset the board to make another request.

The example uses nornWIFI → nornNET/TCP → nornHTTP. It builds on the same
Wi-Fi configuration as `examples/wifi`, without the ping tests and statistics.
It requires a physical ESP32 board; Wi-Fi cannot run in the project's QEMU.

## 1. Configure Wi-Fi and the server address

Run these commands from the nornRTOS repository root:

```sh
cp -n examples/wifi_http/nw_secrets.example.h examples/wifi_http/nw_secrets.h
```

Edit `examples/wifi_http/nw_secrets.h`:

```c
#define NW_SECRET_SSID      "my-network"
#define NW_SECRET_PASSWORD  "my-password"
#define HTTP_SERVER_IP      "192.168.1.10"
#define HTTP_SERVER_PORT    18080
```

`HTTP_SERVER_IP` is **the server computer's IPv4 address**, without `http://`,
a port, or a path. The ESP32 must be able to reach that computer from its Wi-Fi
network. Do not use `127.0.0.1` or `0.0.0.0` as the application's server address.
Write the port as a plain integer. Git ignores the `nw_secrets.h` file.

## 2. Start the demo server

The server is part of nornHTTP and uses only the Python standard library. Run it on
the computer whose address is `HTTP_SERVER_IP`, from the nornRTOS repository root:

```sh
python3 modules/nornHTTP/tools/demo_server.py          # port 18080 on every interface
```

`GET /hello` returns HTTP 200 and `{"message":"Hello from the test server!"}`. The
same server also serves `/chunked`, `/range`, `/large` and `POST /api`, which the
Linux integration test and the coming REST version of this example use. Check it
from the computer with `curl http://127.0.0.1:18080/hello`. The firewall must allow
TCP port 18080 from the ESP32's network. Stop the server with Ctrl+C.

## 3. Build, upload, and read the response

```sh
make APP=wifi_http BOARD=esp32
make APP=wifi_http BOARD=esp32 upload PORT=/dev/ttyUSB0
make APP=wifi_http BOARD=esp32 monitor PORT=/dev/ttyUSB0
```

Change the serial port as needed. UART runs at 115200 baud. If startup has
already completed when you open the monitor, press the board's reset button.

Expected application output (the modules may also print their own logs):

```text
nornRTOS wifi_http
Connecting to Wi-Fi...
GET http://192.168.1.10:18080/hello
HTTP status: 200
{"message":"Hello from the test server!"}

wifi_http: OK
```

If something fails, the example reports the failing step: Wi-Fi, DHCP, TCP,
or HTTP. For TCP errors, check that the server is running, that the IP address
and port are correct, and that neither the firewall nor the access point's
client isolation blocks the connection. HTTP error codes are defined in
`modules/nornHTTP/include/norn_http.h`.

## The HTTP code at a glance

`get_hello()` in [wifi_http.c](wifi_http.c) contains the complete HTTP operation.
Once the TCP connection is established, the steps are:

1. Attach the open socket using `norn_http_net_transport_init`.
2. Provide caller-owned buffers and the `norn_http_buffer_body` callback.
3. Initialize the client and execute the request:

```c
norn_http_request_init(&request, "GET", "/hello", HTTP_HOST, NULL, 0);
request.close = true;
result = norn_http_execute(&http, &request, &response);
```

nornHTTP builds the HTTP headers and parses the response. The example prints
the body as bytes without assuming a terminating NUL. **The JSON text is not
parsed**: nornHTTP handles HTTP, not JSON serialization or parsing.

The buffers are deliberately small: 256 bytes for a header line, 256 for request
headers, 512 for receiving data, and at most 256 for the response body. TCP uses
two separate 1024-byte rings. For larger responses, replace the body callback
with one that processes incoming data incrementally, without storing the entire
response.

The HTTP operation has a total timeout of 5 seconds; Wi-Fi, DHCP, and TCP
connection setup have separate timeouts. The HTTP code does not use the heap.
The Wi-Fi module requires the nornRTOS heap and event groups, just as in the
original Wi-Fi example. This is plain HTTP on a local test network, without
TLS or DNS.

## Verification

- A complete ESP32 image was built using the project's Xtensa toolchain.
- The demo server was verified with curl and nornHTTP's POSIX client:
  HTTP 200, the JSON body, and a closed connection.
- The example was not flashed or run over physical Wi-Fi during this session:
  no ESP32 board was connected. A successful Linux request does not replace
  that hardware test.

No source files in nornCore, nornNET, or nornWIFI need to change. `app.mk`
includes the modules and places the HTTP code in flash.
