#ifndef DEVICE_SECRETS_INCLUDED
#define DEVICE_SECRETS_INCLUDED
/* Copy to nw_secrets.h and edit; git ignores nw_secrets.h. These are the first-boot
   defaults only: once saved, the values live in nornStore and are changed from the
   console (`config set wifi.ssid ...`, `config save`), not by rebuilding. */
#define DEVICE_DEFAULT_SSID         "my-network"
#define DEVICE_DEFAULT_PASSWORD     "my-password"
/* IPv4 of the computer running modules/nornHTTP/tools/demo_server.py, as four octets. */
#define DEVICE_DEFAULT_SERVER_IP    192, 168, 1, 10
#define DEVICE_DEFAULT_SERVER_PORT  18080
#define DEVICE_DEFAULT_INTERVAL_S   30
#endif
