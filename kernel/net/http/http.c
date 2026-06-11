#include "http.h"
#include "../net.h"
#include "../dns/dns.h"
#include "../../../drivers/serial.h"
#include "../../core/sleep.h"
#include "../../core/kernel_util.h"

#define HTTP_TIMEOUT_MS 10000
#define HTTP_CONNECT_TIMEOUT 3000
#define HTTP_PORT 80

static int http_wait_state(int sockfd, uint32_t target_state, uint64_t timeout_ms)
{
  for (uint64_t ms = 0; ms < timeout_ms; ms += 50) {
    brights_net_poll_all();
    if (sockfd >= 0 && sockfd < BRIGHTS_NET_MAX_SOCKETS &&
        sockets[sockfd].in_use && sockets[sockfd].tcp_state == target_state)
      return 0;
    brights_sleep_ms(50);
  }
  return -1;
}

int brights_http_init(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: module initialized\n");
  return 0;
}

int brights_http_get(const char *host, const char *path,
                     uint8_t *buffer, uint32_t buffer_size,
                     uint32_t *bytes_received)
{
  if (!host || !path || !buffer || !bytes_received) return -1;

  /* Resolve hostname */
  uint32_t server_ip = 0;
  if (brights_dns_resolve(host, &server_ip) < 0 || server_ip == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: DNS resolution failed\n");
    return -1;
  }

  /* TCP connect */
  int sock = brights_tcp_connect(server_ip, HTTP_PORT);
  if (sock < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: TCP connect failed\n");
    return -1;
  }

  /* Wait for ESTABLISHED */
  if (http_wait_state(sock, 2, HTTP_CONNECT_TIMEOUT) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: connection timeout\n");
    brights_close(sock);
    return -1;
  }

  /* Build HTTP GET request */
  char request[512];
  uint32_t ri = 0;
  const char *parts[] = { "GET ", path, " HTTP/1.1\r\nHost: ", host, "\r\nConnection: close\r\n\r\n" };
  for (int pi = 0; pi < 5; pi++) {
    const char *s = parts[pi];
    while (*s && ri < sizeof(request) - 1) request[ri++] = *s++;
  }
  request[ri] = 0;

  /* Send request (may need multiple sends for long URLs, but 512 is enough) */
  if (brights_send(sock, request, ri) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: send failed\n");
    brights_close(sock);
    return -1;
  }

  /* Receive response */
  uint32_t total = 0;
  for (uint64_t ms = 0; ms < HTTP_TIMEOUT_MS && total < buffer_size; ms += 50) {
    brights_net_poll_all();
    int n = brights_recv(sock, buffer + total, buffer_size - total);
    if (n > 0) total += (uint32_t)n;
    if (n == 0 && total > 0) {
      /* Brief pause to see if more data arrives */
      brights_sleep_ms(100);
      brights_net_poll_all();
      n = brights_recv(sock, buffer + total, buffer_size - total);
      if (n <= 0) break;
      total += (uint32_t)n;
    }
    brights_sleep_ms(50);
  }

  brights_close(sock);

  if (total == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: no response\n");
    return -1;
  }

  *bytes_received = total;
  return 0;
}

int brights_http_post(const char *host, const char *path,
                      const uint8_t *data, uint32_t data_len,
                      uint8_t *buffer, uint32_t buffer_size,
                      uint32_t *bytes_received)
{
  if (!host || !path || !data || !buffer || !bytes_received) return -1;

  uint32_t server_ip = 0;
  if (brights_dns_resolve(host, &server_ip) < 0 || server_ip == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: DNS resolution failed\n");
    return -1;
  }

  int sock = brights_tcp_connect(server_ip, HTTP_PORT);
  if (sock < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: TCP connect failed\n");
    return -1;
  }

  if (http_wait_state(sock, 2, HTTP_CONNECT_TIMEOUT) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: connection timeout\n");
    brights_close(sock);
    return -1;
  }

  /* Build POST request with Content-Length */
  char header[512];
  uint32_t hi = 0;
  const char *pre = "POST ";
  while (*pre && hi < sizeof(header) - 1) header[hi++] = *pre++;
  {
    const char *s = path;
    while (*s && hi < sizeof(header) - 1) header[hi++] = *s++;
  }
  const char *mid = " HTTP/1.1\r\nHost: ";
  while (*mid && hi < sizeof(header) - 1) header[hi++] = *mid++;
  {
    const char *s = host;
    while (*s && hi < sizeof(header) - 1) header[hi++] = *s++;
  }
  const char *cl_hdr = "\r\nContent-Length: ";
  while (*cl_hdr && hi < sizeof(header) - 1) header[hi++] = *cl_hdr++;
  /* Append data_len as decimal */
  {
    char tmp[16];
    int ti = 0;
    uint32_t n = data_len;
    do { tmp[ti++] = '0' + (n % 10); n /= 10; } while (n > 0);
    for (int tj = ti - 1; tj >= 0 && hi < sizeof(header) - 1; tj--)
      header[hi++] = tmp[tj];
  }
  const char *suf = "\r\nConnection: close\r\n\r\n";
  while (*suf && hi < sizeof(header) - 1) header[hi++] = *suf++;
  header[hi] = 0;

  /* Send header */
  if (brights_send(sock, header, hi) < 0) {
    brights_close(sock);
    return -1;
  }
  /* Send body */
  if (data_len > 0) {
    brights_send(sock, data, data_len);
  }

  /* Receive response */
  uint32_t total = 0;
  for (uint64_t ms = 0; ms < HTTP_TIMEOUT_MS && total < buffer_size; ms += 50) {
    brights_net_poll_all();
    int n = brights_recv(sock, buffer + total, buffer_size - total);
    if (n > 0) total += (uint32_t)n;
    if (n == 0 && total > 0) {
      brights_sleep_ms(100);
      brights_net_poll_all();
      n = brights_recv(sock, buffer + total, buffer_size - total);
      if (n <= 0) break;
      total += (uint32_t)n;
    }
    brights_sleep_ms(50);
  }

  brights_close(sock);

  if (total == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "http: no response\n");
    return -1;
  }

  *bytes_received = total;
  return 0;
}
