// server.h

#ifndef SERVER_H
#define SERVER_H

#include "esp_http_server.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// HTML Page Struct
typedef struct {
    const unsigned char *resp;
    size_t              *resp_len;
} WEBKIT_RESPONSE_ARGS;

void server_init();

#endif // SERVER_H