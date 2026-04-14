#ifndef BRIGHTS_NET_HTTP_H
#define BRIGHTS_NET_HTTP_H

#include <stdint.h>
#include "../net.h"

#define HTTP_PORT 80
#define HTTP_TIMEOUT_MS 10000
#define HTTP_MAX_HEADER_SIZE 4096
#define HTTP_MAX_BODY_SIZE  (64*1024) /* 64KB */

#define HTTP_METHOD_GET  1
#define HTTP_METHOD_POST 2
#define HTTP_METHOD_PUT  3
#define HTTP_METHOD_DELETE 4

#define HTTP_STATUS_OK           200
#define HTTP_STATUS_CREATED      201
#define HTTP_STATUS_ACCEPTED     202
#define HTTP_STATUS_NO_CONTENT   204
#define HTTP_STATUS_MOVED_PERM   301
#define HTTP_STATUS_FOUND        302
#define HTTP_STATUS_NOT_MODIFIED 304
#define HTTP_STATUS_BAD_REQUEST  400
#define HTTP_STATUS_UNAUTHORIZED 401
#define HTTP_STATUS_FORBIDDEN    403
#define HTTP_STATUS_NOT_FOUND    404
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405
#define HTTP_STATUS_REQUEST_TIMEOUT 408
#define HTTP_STATUS_INTERNAL_ERROR 500
#define HTTP_STATUS_NOT_IMPLEMENTED 501
#define HTTP_STATUS_BAD_GATEWAY  502
#define HTTP_STATUS_SERVICE_UNAVAIL 503

#pragma pack(push, 1)
typedef struct {
    uint8_t method;  /* HTTP method */
    uint8_t version_major;
    uint8_t version_minor;
    uint16_t status_code;
    uint32_t content_length;
    int keep_alive;
} http_response_t;
#pragma pack(pop)

int brights_http_get(const char *host, const char *path, 
                     uint8_t *buffer, uint32_t buffer_size, 
                     uint32_t *bytes_received);
int brights_http_post(const char *host, const char *path,
                      const uint8_t *data, uint32_t data_len,
                      uint8_t *buffer, uint32_t buffer_size,
                      uint32_t *bytes_received);
int brights_http_init(void);

#endif