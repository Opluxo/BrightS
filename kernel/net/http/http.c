#include "http.h"
#include "../net.h"
#include "../dns/dns.h"
#include "../../drivers/serial.h"
#include "../printf.h"
#include "../kernel_util.h"

static uint8_t http_state = 0;
int brights_http_init(void)
{
    http_state = 0;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: module initialized\n");
    return 0;
}

int brights_http_get(const char *host, const char *path, 
                     uint8_t *buffer, uint32_t buffer_size, 
                     uint32_t *bytes_received)
{
    if (!host || !path || !buffer || !bytes_received) return -1;
    
    /* Parse host to extract IP (simplified) */
    uint32_t server_ip = 0;
    if (kutil_strcmp(host, "httpbin.org") == 0) {
        server_ip = 0x50630A5A; /* 90.10.99.80 */
    } else if (kutil_strcmp(host, "api.github.com") == 0) {
        server_ip = 0x80BB2A40; /* 64.42.187.128 */
    } else {
        /* Try DNS resolution */
        brights_dns_resolve(host, &server_ip);
    }
    
    if (server_ip == 0) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: could not resolve host\n");
        return -1;
    }
    
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: GET ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, path);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " HTTP/1.1\r\nHost: ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, host);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\n\r\n");
    
    /* Simulate HTTP response */
    const char *response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Hello, World!";
                          
    uint32_t len = kutil_strlen(response);
    if (len > buffer_size) len = buffer_size;
    kutil_memcpy(buffer, response, len);
    *bytes_received = len;
    
    return 0;
}

int brights_http_post(const char *host, const char *path,
                      const uint8_t *data, uint32_t data_len,
                      uint8_t *buffer, uint32_t buffer_size,
                      uint32_t *bytes_received)
{
    if (!host || !path || !data || !buffer || !bytes_received) return -1;
    
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: POST ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, path);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " HTTP/1.1\r\nHost: ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, host);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\nContent-Length: ");
    
    char len_buf[16];
    int len_pos = 0;
    if (data_len == 0) {
        len_buf[len_pos++] = '0';
    } else {
        uint32_t temp = data_len;
        while (temp > 0) {
            len_buf[len_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        // Reverse
        for (int i = 0; i < len_pos/2; i++) {
            char tmp = len_buf[i];
            len_buf[i] = len_buf[len_pos-1-i];
            len_buf[len_pos-1-i] = tmp;
        }
    }
    
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, len_buf);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\n\r\n");
    
    /* Echo back the data */
    uint32_t copy_len = data_len < buffer_size ? data_len : buffer_size;
    kutil_memcpy(buffer, data, copy_len);
    *bytes_received = copy_len;
    
    return 0;
}