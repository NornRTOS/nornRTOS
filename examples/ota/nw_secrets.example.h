#ifndef OTA_SECRETS_INCLUDED
#define OTA_SECRETS_INCLUDED
/* Copy to nw_secrets.h (ignored by git) and fill in. The network as in examples/wifi, and
   the host that serves the image: tools/ota_serve_once.py PORT FILE. */
#define NW_SECRET_SSID      "your-network"
#define NW_SECRET_PASSWORD  "your-password"
#define OTA_SERVER_IP       "192.168.1.10"
#define OTA_SERVER_PORT     18081
#endif
