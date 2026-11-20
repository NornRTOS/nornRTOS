# REST over Wi-Fi with nornHTTP

An ESP32 example: join Wi-Fi, obtain an address by DHCP, open one TCP connection to
the demo server and make four requests on it:

1. `GET /hello`, a small JSON body, printed;
2. `POST /api` with a JSON body, which the server echoes with status 201;
3. `GET /large`, 64 KiB streamed through the 512-byte receive buffer, every byte
   checked against its pattern in the body callback and nothing stored;
4. `GET /hello` again with `Connection: close`.

The first three leave the connection reusable (keep-alive over nornNET); the last
closes it. The example prints each status, the small bodies, the `reusable` flag, the
time of each request and the task's stack high-water, then the task exits and the
network task finishes the TCP shutdown. Reset the board to run it again.

The example uses nornWIFI, nornNET TCP and nornHTTP, on the Wi-Fi configuration of
`examples/wifi`. It needs a physical ESP32: Wi-Fi cannot run in the project's QEMU.

## 1. Configure Wi-Fi and the server address

From the nornRTOS repository root:

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

`HTTP_SERVER_IP` is the server computer's IPv4 address, without `http://`, a port or
a path, reachable from the ESP32's network. Not `127.0.0.1` or `0.0.0.0`. The port is
a plain integer. Git ignores `nw_secrets.h`.

## 2. Start the demo server

The server is part of nornHTTP and uses only the Python standard library. Run it on
the computer whose address is `HTTP_SERVER_IP`, from the nornRTOS root:

```sh
python3 modules/nornHTTP/tools/demo_server.py          # port 18080 on every interface
```

It serves `/hello`, `/api` (POST, echoes the body with 201), `/large` (64 KiB, byte i
is i & 0xFF), `/chunked` and `/range`; the Linux integration test of nornHTTP uses the
same handler. Check it with `curl http://127.0.0.1:18080/hello`. The firewall must
allow TCP port 18080 from the ESP32's network.

## 3. Build, upload and read the output

```sh
make APP=wifi_http BOARD=esp32
make APP=wifi_http BOARD=esp32 upload PORT=/dev/ttyUSB0
make APP=wifi_http BOARD=esp32 monitor PORT=/dev/ttyUSB0
```

UART at 115200 baud. Expected output (the modules print their own log lines too):

```text
nornRTOS wifi_http
Connecting to Wi-Fi...
http://192.168.1.10:18080
GET /hello -> 200, 41 bytes, reusable 1, 52 ms: {"message":"Hello from the test server!"}
POST /api -> 201, 11 bytes, reusable 1, 99 ms: {"item":42}
GET /large -> 200, 65536 bytes, reusable 1, 586 ms
  /large: 65536 bytes streamed, 0 bad
GET /hello -> 200, 41 bytes, reusable 0, 9 ms: {"message":"Hello from the test server!"}
stack free: 3952 at start, 2880 after connect, 2688 after HTTP, of 4096; sizeof context 184 request 64 response 56
wifi_http: OK
```

If something fails the example says which step: Wi-Fi, DHCP, TCP or HTTP. For TCP
errors check that the server runs, that the address and port are right, and that
neither the firewall nor the access point's client isolation blocks the connection.
HTTP error codes are `norn_http_result` in `modules/nornHTTP/include/norn_http.h`.

## The HTTP code at a glance

`request()` in [wifi_http.c](wifi_http.c) is one request on the open connection:

```c
norn_http_request_init(&req, method, target, HTTP_HOST, headers, 1);
req.close = last;                                   /* Connection: close on the last one */
norn_http_add_header(&req, "Content-Type", "application/json");   /* the POST */
norn_http_set_body(&req, json, strlen(json));
result = norn_http_execute(&http, &req, &resp);     /* resp.status, resp.reusable */
```

The client is set up once: `norn_http_net_simple` binds the transport to the connected
nornNET socket and declares the buffers (`NORN_HTTP_BUFFERS`: 256-byte header line,
512-byte receive buffer, 256-byte request buffer, 5 s deadline), then the example puts
its own body callback in place and calls `norn_http_init`. That callback keeps the
small bodies in a 256-byte buffer and checks the large one byte by byte as it streams
through, so the response size does not decide the RAM. nornHTTP handles HTTP, not
JSON: the bodies are byte spans, printed as they are.

What it cost on the board (2026-11-19) is in `nornInternal/nornHTTP/measurements.md`:
about 6.1 KB of flash for the library and the adapter, no static RAM of its own, and
190 to 240 bytes of stack beyond what the Wi-Fi join and the TCP connect had already
used.

## Verification

Run on a NodeMCU-32S on 2026-11-19 against the demo server on the development
machine, three runs, all four requests OK, 65536 bytes streamed with 0 bad, keep-alive
held across the first three requests. No source in nornCore, nornNET or nornWIFI
changes for this example; `app.mk` adds the module and places its code in flash.
