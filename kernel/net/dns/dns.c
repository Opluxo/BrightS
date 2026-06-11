#include "dns.h"
#include "../net.h"
#include "../../../drivers/serial.h"
#include "../../core/sleep.h"
#include "../../core/kernel_util.h"

#define DNS_CLIENT_PORT 54321
#define DNS_TIMEOUT_MS 5000
#define DNS_RETRY 2

static uint32_t dns_server1 = 0x08080808;
static uint32_t dns_server2 = 0x08080404;

static uint16_t dns_id_counter = 0x1234;

/* Single socket for both send and recv on the same port */
static int dns_exchange(uint8_t *query, uint32_t qlen, uint8_t *reply,
                        uint32_t rcap, uint32_t server)
{
  int sock = brights_socket(2, 2, 0);
  if (sock < 0) return -1;
  brights_bind(sock, 0, DNS_CLIENT_PORT);
  brights_connect(sock, server, DNS_PORT);
  if (brights_send(sock, query, qlen) < 0) {
    brights_close(sock);
    return -1;
  }

  int got = 0;
  for (uint64_t ms = 0; ms < DNS_TIMEOUT_MS; ms += 50) {
    brights_net_poll_all();
    int n = brights_recv(sock, reply, rcap);
    if (n > 0) { got = n; break; }
    brights_sleep_ms(50);
  }

  brights_close(sock);
  return got;
}

static int dns_skip_name(const uint8_t *pkt, uint32_t pkt_len, uint32_t off)
{
  while (off < pkt_len) {
    uint8_t len = pkt[off];
    if (len == 0) return off + 1;
    if ((len & 0xC0) == 0xC0) return off + 2;
    off += 1 + len;
  }
  return -1;
}

int brights_dns_resolve(const char *hostname, uint32_t *ip_out)
{
  if (!hostname || !ip_out) return -1;

  uint8_t packet[DNS_MAX_UDP_SIZE];
  kutil_memset(packet, 0, sizeof(packet));

  /* Build DNS query header (raw bytes to avoid bitfield ambiguity) */
  uint16_t id = dns_id_counter++;
  packet[0] = (uint8_t)(id >> 8);
  packet[1] = (uint8_t)(id);
  packet[2] = 0x01; /* QR=0, OPCODE=0, AA=0, TC=0, RD=1 */
  packet[3] = 0x00; /* RA=0, Z=0, AD=0, CD=0, RCODE=0 */
  packet[4] = 0x00; packet[5] = 0x01; /* QDCOUNT = 1 */
  packet[6] = 0x00; packet[7] = 0x00; /* ANCOUNT = 0 */
  packet[8] = 0x00; packet[9] = 0x00; /* NSCOUNT = 0 */
  packet[10] = 0x00; packet[11] = 0x00; /* ARCOUNT = 0 */

  /* Encode hostname as length-prefixed labels */
  uint32_t off = 12;
  const char *p = hostname;
  while (*p) {
    const char *dot = kutil_strchr(p, '.');
    int lab_len = dot ? (int)(dot - p) : (int)kutil_strlen(p);
    if (lab_len <= 0 || lab_len > DNS_MAX_LABEL) return -1;
    packet[off++] = (uint8_t)lab_len;
    kutil_memcpy(packet + off, p, lab_len);
    off += lab_len;
    p = dot ? dot + 1 : p + lab_len;
  }
  packet[off++] = 0; /* root label */

  /* Question type and class */
  packet[off++] = 0x00; packet[off++] = 0x01; /* QTYPE = A */
  packet[off++] = 0x00; packet[off++] = 0x01; /* QCLASS = IN */

  uint32_t query_len = off;

  /* Try primary DNS server */
  uint32_t servers[2] = { dns_server1, dns_server2 };
  for (int si = 0; si < 2; si++) {
    if (servers[si] == 0) continue;
    for (int attempt = 0; attempt < DNS_RETRY; attempt++) {
      uint8_t reply[DNS_MAX_UDP_SIZE];
      int rlen = dns_exchange(packet, query_len, reply, sizeof(reply), servers[si]);
      if (rlen < 12) continue;

      /* Validate response ID matches */
      uint16_t rid = (uint16_t)(reply[0] << 8) | reply[1];
      if (rid != id) continue;

      /* Check QR=1 (response) and RCODE=0 (no error) */
      if (!(reply[2] & 0x80)) continue; /* QR must be 1 */
      if ((reply[3] & 0x0F) != 0) continue; /* RCODE must be 0 */

      uint16_t qdcount = (uint16_t)(reply[4] << 8) | reply[5];
      uint16_t ancount = (uint16_t)(reply[6] << 8) | reply[7];

      /* Skip question section */
      uint32_t pos = 12;
      for (uint16_t q = 0; q < qdcount; q++) {
        int next = dns_skip_name(reply, rlen, pos);
        if (next < 0) break;
        pos = next + 4; /* skip QTYPE + QCLASS */
      }

      /* Parse answer section */
      for (uint16_t a = 0; a < ancount; a++) {
        int next = dns_skip_name(reply, rlen, pos);
        if (next < 0) break;
        if (next + 10 > rlen) break; /* TYPE + CLASS + TTL + RDLENGTH */

        uint16_t rtype = (uint16_t)(reply[next] << 8) | reply[next + 1];
        uint16_t rdlength = (uint16_t)(reply[next + 8] << 8) | reply[next + 9];
        uint32_t rdata_off = next + 10;

        if (rtype == DNS_TYPE_A && rdlength == 4 && rdata_off + 4 <= (uint32_t)rlen) {
          *ip_out = (uint32_t)reply[rdata_off] |
                    ((uint32_t)reply[rdata_off + 1] << 8) |
                    ((uint32_t)reply[rdata_off + 2] << 16) |
                    ((uint32_t)reply[rdata_off + 3] << 24);
          return 0;
        }
        pos = rdata_off + rdlength;
      }
    }
  }

  return -1;
}

int brights_dns_init(const char *nameserver1, const char *nameserver2)
{
  if (nameserver1) {
    uint8_t a = 0, b = 0, c = 0, d = 0;
    if (kutil_parse_ipv4(nameserver1, &a, &b, &c, &d) == 4)
      dns_server1 = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
  }
  if (nameserver2) {
    uint8_t a = 0, b = 0, c = 0, d = 0;
    if (kutil_parse_ipv4(nameserver2, &a, &b, &c, &d) == 4)
      dns_server2 = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d;
  }
  return 0;
}
