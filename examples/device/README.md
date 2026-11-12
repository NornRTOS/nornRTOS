# device: every module in one application

A small connected device on an ESP32, built from all six modules of nornRTOS without a
line changed in any of them:

| module | what it does here |
|---|---|
| nornCore | the kernel: five tasks, one per priority |
| nornStore | the configuration in the last two flash sectors: Wi-Fi network and password, the server's address and port, hostname, report interval, a boot counter |
| nornWIFI | joins the network with the stored credentials and keeps the link up; its `wifi` command scans, joins and leaves from the console |
| nornNET | an address by DHCP, TCP to the server |
| nornHTTP | `POST /api` with a JSON report every interval; the test server echoes it with 201 |
| nornSHELL | nosh on UART0: `help uptime ps mem sched`, `config` from nornStore, `wifi` from nornWIFI, and the application's `report` and `net` |

The application is [device.c](device.c); the modules are pulled in by [app.mk](app.mk).
It needs a board: the radio does not run in QEMU.

## 1. First-boot defaults

```sh
cp -n examples/device/nw_secrets.example.h examples/device/nw_secrets.h
```

Edit `examples/device/nw_secrets.h`: the Wi-Fi network and password, the IPv4 address of
the computer that will run the test server (as four octets), the port. These are used
once, on the first boot, to fill the store; after that the values live in the flash and
are changed from the console, not by rebuilding. Git ignores `nw_secrets.h`.

## 2. The test server

On the computer whose address you entered, from the nornRTOS root:

```sh
python3 modules/nornHTTP/tools/demo_server.py          # port 18080 on every interface
```

`POST /api` echoes the JSON body with status 201. The firewall must allow TCP 18080 from
the board's network.

## 3. Build, upload, watch

```sh
make APP=device BOARD=esp32
make APP=device BOARD=esp32 upload PORT=/dev/ttyUSB0
make APP=device BOARD=esp32 monitor PORT=/dev/ttyUSB0
```

```text
nornRTOS device: nornCore, nornStore, nornWIFI, nornNET, nornHTTP, nornSHELL
device: config seq 1, boot 1
device: joining 'my-network'
device: address 192.168.1.37 gateway 192.168.1.1
report: POST http://192.168.1.10:18080/api {"host":"norn-device","boots":1,"uptime_s":4,"load":2,"rssi":-58,"reports":0}
report: HTTP 201 {"host":"norn-device","boots":1,"uptime_s":4,"load":2,"rssi":-58,"reports":0}

nosh> wifi
link     up, rssi -58 dBm
mac      24:6f:28:aa:bb:cc
counters 1 ups, 0 downs, 0 reconnect attempts; rx 412 tx 96 frames, heap free 41208 (min 38904)
nosh> net
address 192.168.1.37 mask 255.255.255.0 gateway 192.168.1.1 (dhcp bound)
reports 1 ok, 0 failed, last HTTP status 201
nosh> config
system.hostname   str  norn-device
system.boots      u32  1
wifi.ssid         str  my-network
wifi.psk          str  my-password
server.ip         ip4  192.168.1.10
server.port       u16  18080
report.interval   u32  30
nosh> config set report.interval 5
report.interval   u32  5 *
nosh> config set system.hostname kitchen
system.hostname   str  kitchen *
nosh> config save
saved, seq 2 in block 1
nosh> report
report requested
nosh> ps
 id name      st prio wait      left  stack      cpu ms  ovr
  0 idle      r  0    -            -          -   12034    0
  1 nosh      R  1    -            -  2048/1412      31    0
  2 device    B  2    notify    4210  4096/2380     220    0
  3 net       B  3    events       -  3072/2104      95    0
  4 wifi-svc  B  4    events       -  3072/1988      40    0
  5 wifi      B  5    events       -  6144/3930    1507    0
```

Nothing needs a reboot. The hostname, the interval and the server take effect at the
next report. A new Wi-Fi network:

```text
nosh> wifi scan
scanning...
 rssi ch  security   ssid
 -52  6  wpa2       kitchen-net
 -71 11  wpa/wpa2   neighbour
nosh> config set wifi.ssid kitchen-net
wifi.ssid         str  kitchen-net *
nosh> config set wifi.psk my new password
wifi.psk          str  my new password *
nosh> config save
saved, seq 3 in block 0
nosh> wifi join
joining 'kitchen-net' (psk)...
started; `wifi` shows the outcome
```

`wifi join` without arguments uses the saved credentials (the application hands the
command a callback that reads the store; nornWIFI itself knows nothing of nornStore), and
`wifi join <ssid> <psk>` tries a network without saving it. The application task follows
the link, so DHCP and the reports resume on the new network by themselves. `config diff`
shows what is not saved yet, `config revert` drops it, `config erase yes` goes back to
the first-boot defaults at the next reset. The boot counter counts resets; `uptime` says
how long since the last one and why.

## What to look at in the code

- `storeDefaults()`: how an application tells a first boot from a later one
  (`STORE_F_FROM_FLASH`) and fills in its defaults.
- `report()`: one HTTP request with nornHTTP's caller-owned buffers over a nornNET socket;
  the JSON is the application's own string building, nornHTTP carries bytes.
- `cmdReport()`: the shell never touches the socket. The command notifies the
  application task, which owns the network and sleeps on `rtos_notifyWait` between
  reports, so `report` and the interval share one wait.
- `storedCredentials()` and `nw_cmd_setCredentials()`: how a module's command gets at
  another module's data without the modules knowing each other.
- `appTask()`: a task that follows the link instead of failing once: DHCP starts when the
  station is associated, reports happen while it is up, and a network change from the
  console is just another link event.
- `app.mk`: which objects run from flash and why nornStore's do not (the cache is off
  while it writes the flash).

## Not in this example

TLS, DNS, a real time clock, retrying a failed report before the next interval, and a
server that does anything with the report but echo it.
