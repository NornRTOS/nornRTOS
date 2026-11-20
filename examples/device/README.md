# device: every module in one application

A small connected device on an ESP32, built from all six modules of nornRTOS without a
line changed in any of them:

| module | what it does here |
|---|---|
| nornCore | the kernel: five tasks, one per priority; the console speaks in its log classes (`hal/log.h`), level from the store |
| nornStore | the configuration in the last two flash sectors: Wi-Fi network and password, the server's address and port, hostname, report interval, log level, a boot counter; a change of the log level acts at once (`store_onChange`) |
| nornWIFI | joins the network with the stored credentials and keeps the link up; its `wifi` command scans, joins and leaves from the console |
| nornNET | an address by DHCP, TCP to the server; its `net` command shows and steers the stack from the console |
| nornHTTP | `POST /api` with a JSON report every interval; the test server echoes it with 201 |
| nornSHELL | nosh on UART0: `help uptime ps mem sched log`, `config` from nornStore, `wifi` from nornWIFI, `net` from nornNET, and the application's `report` |

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
I (12) device: config seq 1, boot 1
I (14) device: joining 'my-network'
I (1875) wifi: connected, bssid 24:6f:28:11:22:33 channel 6
I (3102) device: address 192.168.1.37 gateway 192.168.1.1
I (3388) report: HTTP 201, 1 reports

nosh> wifi
link     up, rssi -58 dBm
mac      24:6f:28:aa:bb:cc
counters 1 ups, 0 downs, 0 reconnect attempts; rx 412 tx 96 frames, heap free 41208 (min 38904)
nosh> net
wifi0    ethernet up link mtu 1500 mac 24:6f:28:aa:bb:cc rx 412 tx 96
  addr 192.168.1.37 mask 255.255.255.0 gw 192.168.1.1  dhcp bound from 192.168.1.1, lease 86400 s, 31 s ago
dns      192.168.1.1  buffers 16/16 free (min 11, 0 failed)
nosh> net sockets
tcp  TIME_WAIT   192.168.1.37:49153 <> 192.168.1.10:18080 rx 96 tx 231
udp              0.0.0.0:68 rx 2 tx 2
nosh> net ping 192.168.1.10
reply from 192.168.1.10: seq 1 time 4 ms
reply from 192.168.1.10: seq 2 time 3 ms
reply from 192.168.1.10: seq 3 time 3 ms
3 sent, 3 received
nosh> report status
reports 1 ok, 0 failed, last HTTP status 201
nosh> config
system.hostname   str  norn-device
system.boots      u32  1
system.loglvl     u8   3
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

The stack is steered from the console too, for the current run: `net dhcp release`
gives the lease up and clears the address, `net dhcp start` asks for one again, `net dhcp
renew` confirms the lease now; `net addr 192.168.1.50 255.255.255.0 192.168.1.1` sets a
static address instead (releasing DHCP first); `net dns 1.1.1.1` and `net resolve
example.com` try the resolver; `net stats` and `net arp` show what the stack has seen.
None of that is saved: what should survive a reset goes through `config`, and the
`net` command does not know that nornStore exists. `help` lists everything.

The console's lines carry nornCore's log classes: the letter (E W I D, P for a panic) and
the time in ms. `system.loglvl` says how much is written, and it acts the moment it is
set, so a device that reports every few seconds can be made quiet or talkative from the
console:

```text
nosh> config set system.loglvl 2
system.loglvl     u8   2 *                      ← from now on errors and warnings only
nosh> config save
saved, seq 4 in block 1
nosh> config set system.loglvl 4
system.loglvl     u8   4 *
D (91234) report: POST http://192.168.1.10:18080/api {"host":"norn-device","boots":1,"uptime_s":91,"load":2,"rssi":-58,"reports":3}
I (91412) report: HTTP 201, 4 reports
D (91413) report: reply {"host":"norn-device","boots":1,"uptime_s":91,"load":2,"rssi":-58,"reports":3}
nosh> config set system.loglvl 9
config: set: value                              ← refused by the validate hook
nosh> log
log level debug (4), compiled up to debug
```

`log info` sets the level for this run only. The DEBUG lines are in this image because
app.mk raises `LOG_CFG_MAX`; an application built with the default cap has no DEBUG text
in its flash at all. The radio's own lines (`I (1875) wifi: ...`) follow the same level.

## What to look at in the code

- `storeDefaults()`: how an application tells a first boot from a later one
  (`STORE_F_FROM_FLASH`) and fills in its defaults.
- `applyLogLevel()`, `onStoreChange()` and `deviceValidate()` (named in
  `store_config.h`): a stored value that is mirrored in RAM, applied at boot and on every
  change, and checked before the store accepts it. The `LOG_x` lines in `report()` and
  `appTask()` are what the level governs.
- `report()`: one HTTP request with nornHTTP's caller-owned buffers over a nornNET socket;
  the JSON is the application's own string building, nornHTTP carries bytes.
- `cmdReport()`: the shell never touches the socket. The command notifies the
  application task, which owns the network and sleeps on `rtos_notifyWait` between
  reports, so `report` and the interval share one wait; `report status` reads the
  counters.
- `nosh_add(&nn_cmd_net)`: one line gives the console the whole `net` command. nornNET
  compiles it only because nosh.mk is in `MODULES`; an application without the shell
  gets a stack that is byte for byte the same.
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
