#include "dhcp.h"
#include "../net.h"
#include "../../../drivers/serial.h"
#include "../../core/sleep.h"
#include "../../core/kernel_util.h"

#define DHCP_BROADCAST 0xFFFFFFFF

#define DHCP_OP_BOOTREQUEST 1
#define DHCP_OP_BOOTREPLY   2

#define DHCP_MAGIC0 0x63
#define DHCP_MAGIC1 0x82
#define DHCP_MAGIC2 0x53
#define DHCP_MAGIC3 0x63

#define DHCP_OPT_PAD          0
#define DHCP_OPT_SUBNET_MASK  1
#define DHCP_OPT_ROUTER       3
#define DHCP_OPT_DNS          6
#define DHCP_OPT_DOMAIN      15
#define DHCP_OPT_REQUESTED_IP 50
#define DHCP_OPT_LEASE_TIME  51
#define DHCP_OPT_MSG_TYPE    53
#define DHCP_OPT_SERVER_ID   54
#define DHCP_OPT_PARAM_LIST  55
#define DHCP_OPT_END        255

#define DHCP_RETRY 3
#define DHCP_TIMEOUT_MS 4000

extern brights_netif_t netifs[BRIGHTS_NET_MAX_IF];

static uint32_t dhcp_xid;

int brights_dhcp_client_init(void)
{
  dhcp_xid = 0x12345678;
  return 0;
}

static void opt_write8(uint8_t **p, uint8_t v)
{
  *(*p)++ = v;
}

static void opt_write32(uint8_t **p, uint32_t v)
{
  *(*p)++ = (uint8_t)(v);
  *(*p)++ = (uint8_t)(v >> 8);
  *(*p)++ = (uint8_t)(v >> 16);
  *(*p)++ = (uint8_t)(v >> 24);
}

static uint32_t opt_read32(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void build_discover(dhcp_packet_t *pkt, const uint8_t *mac)
{
  kutil_memset(pkt, 0, sizeof(*pkt));
  pkt->op = DHCP_OP_BOOTREQUEST;
  pkt->htype = DHCP_HTYPE_ETHERNET;
  pkt->hlen = DHCP_HLEN_ETHERNET;
  pkt->xid = dhcp_xid;
  pkt->flags = 0x8000;
  kutil_memcpy(pkt->chaddr, mac, 6);

  uint8_t *opt = pkt->options;
  opt_write8(&opt, DHCP_MAGIC0);
  opt_write8(&opt, DHCP_MAGIC1);
  opt_write8(&opt, DHCP_MAGIC2);
  opt_write8(&opt, DHCP_MAGIC3);
  opt_write8(&opt, DHCP_OPT_MSG_TYPE); opt_write8(&opt, 1); opt_write8(&opt, DHCP_DISCOVER);
  opt_write8(&opt, DHCP_OPT_PARAM_LIST); opt_write8(&opt, 4);
  opt_write8(&opt, DHCP_OPT_SUBNET_MASK);
  opt_write8(&opt, DHCP_OPT_ROUTER);
  opt_write8(&opt, DHCP_OPT_DNS);
  opt_write8(&opt, DHCP_OPT_LEASE_TIME);
  opt_write8(&opt, DHCP_OPT_END);
}

static void build_request(dhcp_packet_t *pkt, uint32_t ciaddr,
                          uint32_t requested_ip, uint32_t server_id,
                          const uint8_t *mac)
{
  kutil_memset(pkt, 0, sizeof(*pkt));
  pkt->op = DHCP_OP_BOOTREQUEST;
  pkt->htype = DHCP_HTYPE_ETHERNET;
  pkt->hlen = DHCP_HLEN_ETHERNET;
  pkt->xid = dhcp_xid;
  pkt->flags = 0x8000;
  pkt->ciaddr = ciaddr;
  kutil_memcpy(pkt->chaddr, mac, 6);

  uint8_t *opt = pkt->options;
  opt_write8(&opt, DHCP_MAGIC0);
  opt_write8(&opt, DHCP_MAGIC1);
  opt_write8(&opt, DHCP_MAGIC2);
  opt_write8(&opt, DHCP_MAGIC3);
  opt_write8(&opt, DHCP_OPT_MSG_TYPE); opt_write8(&opt, 1); opt_write8(&opt, DHCP_REQUEST);
  if (requested_ip) {
    opt_write8(&opt, DHCP_OPT_REQUESTED_IP); opt_write8(&opt, 4);
    opt_write32(&opt, requested_ip);
  }
  if (server_id) {
    opt_write8(&opt, DHCP_OPT_SERVER_ID); opt_write8(&opt, 4);
    opt_write32(&opt, server_id);
  }
  opt_write8(&opt, DHCP_OPT_END);
}

static int parse_reply(const dhcp_packet_t *pkt, uint32_t len,
                       uint8_t *msg_type, uint32_t *yiaddr,
                       uint32_t *server_id, uint32_t *subnet_mask,
                       uint32_t *router, uint32_t *dns1, uint32_t *dns2)
{
  if (len < sizeof(dhcp_packet_t) - 312) return -1;
  if (pkt->op != DHCP_OP_BOOTREPLY) return -1;
  if (pkt->xid != dhcp_xid) return -1;

  *yiaddr = pkt->yiaddr;
  *msg_type = 0;
  *server_id = 0;
  *subnet_mask = 0;
  *router = 0;
  *dns1 = 0;
  *dns2 = 0;

  const uint8_t *opt = pkt->options;
  if (opt[0] != DHCP_MAGIC0 || opt[1] != DHCP_MAGIC1 ||
      opt[2] != DHCP_MAGIC2 || opt[3] != DHCP_MAGIC3)
    return -1;
  opt += 4;

  while (*opt != DHCP_OPT_END) {
    if (*opt == DHCP_OPT_PAD) { ++opt; continue; }
    uint8_t code = *opt;
    uint8_t olen = opt[1];
    const uint8_t *val = opt + 2;
    opt += 2 + olen;
    if (opt > (const uint8_t *)pkt + len) break;

    switch (code) {
      case DHCP_OPT_MSG_TYPE:
        if (olen >= 1) *msg_type = val[0];
        break;
      case DHCP_OPT_SERVER_ID:
        if (olen >= 4) *server_id = opt_read32(val);
        break;
      case DHCP_OPT_SUBNET_MASK:
        if (olen >= 4) *subnet_mask = opt_read32(val);
        break;
      case DHCP_OPT_ROUTER:
        if (olen >= 4) *router = opt_read32(val);
        break;
      case DHCP_OPT_DNS:
        if (olen >= 4) *dns1 = opt_read32(val);
        if (olen >= 8) *dns2 = opt_read32(val + 4);
        break;
    }
  }
  return 0;
}

int brights_dhcp_request_lease(const char *interface, uint32_t *ip_out,
                               uint32_t *netmask_out, uint32_t *gateway_out,
                               uint32_t *dns1_out, uint32_t *dns2_out)
{
  if (!interface || !ip_out) return -1;

  brights_netif_t *iface = 0;
  for (int i = 0; i < BRIGHTS_NET_MAX_IF; i++) {
    if (netifs[i].up && kutil_strcmp(netifs[i].name, interface) == 0) {
      iface = &netifs[i];
      break;
    }
  }
  if (!iface) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: interface not found\n");
    return -1;
  }

  /* Create socket bound to DHCP client port 68 for receiving replies */
  int sock = brights_socket(2, 2, 0);
  if (sock < 0) { brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: socket failed\n"); return -1; }
  brights_bind(sock, 0, DHCP_CLIENT_PORT);

  dhcp_packet_t pkt;
  uint8_t reply_buf[sizeof(dhcp_packet_t)];
  uint32_t server_id = 0;
  uint32_t offered_ip = 0;
  uint32_t subnet_mask = 0, router = 0, dns1 = 0, dns2 = 0;
  uint8_t msg_type;
  int ret = -1;

  /* Phase 1: DISCOVER → OFFER */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: DISCOVER...\n");
  dhcp_xid = (dhcp_xid + 1) & 0x7FFFFFFF;
  build_discover(&pkt, iface->mac);

  for (int attempt = 0; attempt < DHCP_RETRY; attempt++) {
    brights_udp_send(DHCP_BROADCAST, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &pkt, sizeof(pkt));

    int got_offer = 0;
    for (uint64_t ms = 0; ms < DHCP_TIMEOUT_MS; ms += 50) {
      brights_net_poll_all();
      int n = brights_recv(sock, reply_buf, sizeof(reply_buf));
      if (n > 0) {
        if (parse_reply((dhcp_packet_t *)reply_buf, (uint32_t)n,
                        &msg_type, &offered_ip, &server_id,
                        &subnet_mask, &router, &dns1, &dns2) == 0 &&
            msg_type == DHCP_OFFER && offered_ip != 0) {
          brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: got OFFER\n");
          got_offer = 1;
          break;
        }
      }
      brights_sleep_ms(50);
    }
    if (got_offer) { ret = 0; break; }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: DISCOVER timeout, retrying...\n");
  }

  if (ret < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: no OFFER received\n");
    brights_close(sock);
    return -1;
  }

  /* Phase 2: REQUEST → ACK */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: REQUEST...\n");
  build_request(&pkt, 0, offered_ip, server_id, iface->mac);
  ret = -1;

  for (int attempt = 0; attempt < DHCP_RETRY; attempt++) {
    brights_udp_send(DHCP_BROADCAST, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &pkt, sizeof(pkt));

    int got_ack = 0;
    for (uint64_t ms = 0; ms < DHCP_TIMEOUT_MS; ms += 50) {
      brights_net_poll_all();
      int n = brights_recv(sock, reply_buf, sizeof(reply_buf));
      if (n > 0) {
        uint32_t ack_yiaddr = 0, ack_sid = 0;
        uint32_t ack_mask = 0, ack_rtr = 0, ack_d1 = 0, ack_d2 = 0;
        if (parse_reply((dhcp_packet_t *)reply_buf, (uint32_t)n,
                        &msg_type, &ack_yiaddr, &ack_sid,
                        &ack_mask, &ack_rtr, &ack_d1, &ack_d2) == 0 &&
            msg_type == DHCP_ACK) {
          offered_ip = ack_yiaddr;
          if (ack_mask) subnet_mask = ack_mask;
          if (ack_rtr)  router = ack_rtr;
          if (ack_d1)   dns1 = ack_d1;
          if (ack_d2)   dns2 = ack_d2;
          brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: got ACK\n");
          got_ack = 1;
          break;
        }
      }
      brights_sleep_ms(50);
    }
    if (got_ack) { ret = 0; break; }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: REQUEST timeout, retrying...\n");
  }

  brights_close(sock);

  if (ret < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: no ACK received\n");
    return -1;
  }

  /* Apply lease */
  iface->ip_addr = offered_ip;
  if (subnet_mask) iface->netmask = subnet_mask;
  if (router)      iface->gateway = router;

  *ip_out = offered_ip;
  *netmask_out = subnet_mask;
  *gateway_out = router;
  *dns1_out = dns1 ? dns1 : 0x08080808;
  *dns2_out = dns2 ? dns2 : 0x08080404;

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: lease acquired\n");
  return 0;
}

void brights_dhcp_release_lease(uint32_t ip_addr)
{
  (void)ip_addr;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dhcp: releasing lease\n");
}
