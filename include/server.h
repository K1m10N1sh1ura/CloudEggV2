// server.h

#ifndef SERVER_H
#define SERVER_H

#include "esp_http_server.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

void server_init();

#endif // SERVER_H