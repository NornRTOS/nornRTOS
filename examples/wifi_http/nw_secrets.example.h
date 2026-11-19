#ifndef WIFI_HTTP_SECRETS_INCLUDED
#define WIFI_HTTP_SECRETS_INCLUDED
/* Copy to nw_secrets.h and edit. Never commit real Wi-Fi credentials. */
#define NW_SECRET_SSID      "my-network"
#define NW_SECRET_PASSWORD  "my-password"
/* IPv4 of the computer running the test server, reachable from the ESP32.
 * No scheme, port or path in HTTP_SERVER_IP. Port must be a numeric literal. */
#define HTTP_SERVER_IP      "192.168.1.10"
#define HTTP_SERVER_PORT    18080
#endif
