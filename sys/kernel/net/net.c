#include "net.h"
#include "wifi.h"
#include "../dev/serial.h"
#include "../core/printf.h"
#include "../core/kernel_util.h"
#include "dhcp/dhcp.h"
#include "dns/dns.h"
#include "http/http.h"
#include <stdint.h>

/* ===== Constants ===== */
#define BRIGHTS_NET_MAX_IF 4
#define BRIGHTS_NET_MAX_SOCKETS 32
#define BRIGHTS_NET_BUF_SIZE 1518
#define BRIGHTS_ARP_CACHE_SIZE 16

/* Ethernet types */
#define ETH_TYPE_IP   0x0800
#define ETH_TYPE_ARP  0x0806

/* IP protocols */
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

/* ===== Ethernet header ===== */
typedef struct {
  uint8_t dst[6];
  uint8_t src[6];
  uint16_t type;
} __attribute__((packed)) eth_hdr_t;

/* ===== ARP ===== */
typedef struct {
  uint16_t hw_type;
  uint16_t proto_type;
  uint8_t hw_size;
  uint8_t proto_size;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint32_t sender_ip;
  uint8_t target_mac[6];
  uint32_t target_ip;
} __attribute__((packed)) arp_hdr_t;

#define ARP_REQUEST 1
#define ARP_REPLY   2

/* ===== IP header ===== */
typedef struct {
  uint8_t ver_ihl;
  uint8_t tos;
  uint16_t total_len;
  uint16_t ident;
  uint16_t flags_frag;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t checksum;
  uint32_t src_ip;
  uint32_t dst_ip;
} __attribute__((packed)) ip_hdr_t;

/* ===== ICMP ===== */
typedef struct {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t ident;
  uint16_t seq;
} __attribute__((packed)) icmp_hdr_t;

#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

/* ===== UDP header ===== */
typedef struct {
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t length;
  uint16_t checksum;
} __attribute__((packed)) udp_hdr_t;

/* ===== TCP header ===== */
typedef struct {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq;
  uint32_t ack;
  uint8_t data_off;
  uint8_t flags;
  uint16_t window;
  uint16_t checksum;
  uint16_t urgent;
} __attribute__((packed)) tcp_hdr_t;

#define TCP_SYN 0x02
#define TCP_ACK 0x10
#define TCP_FIN 0x01
#define TCP_RST 0x04
#define TCP_PSH 0x08

/* ===== Network interfaces ===== */
brights_netif_t netifs[BRIGHTS_NET_MAX_IF];
static int netif_count = 0;

/* ===== ARP cache ===== */
typedef struct {
  uint32_t ip;
  uint8_t mac[6];
  int valid;
} arp_entry_t;

static arp_entry_t arp_cache[BRIGHTS_ARP_CACHE_SIZE];

/* ===== Sockets ===== */
typedef struct {
  int in_use;
  int domain;
  int type;
  uint32_t local_ip;
  uint16_t local_port;
  uint32_t remote_ip;
  uint16_t remote_port;
  uint8_t recv_buf[4096];
  uint32_t recv_len;
  uint32_t tcp_state; /* 0=closed, 1=syn_sent, 2=established, 3=fin_wait */
  uint32_t tcp_seq;
  uint32_t tcp_ack;
} brights_socket_t;

static brights_socket_t sockets[BRIGHTS_NET_MAX_SOCKETS];

/* ===== Forward declarations ===== */
static void arp_handle(uint8_t *frame, uint32_t len);
static void ip_handle(uint8_t *frame, uint32_t len);
void icmp_handle(uint8_t *data, uint32_t len, uint32_t src_ip);
void udp_handle(uint8_t *data, uint32_t len, uint32_t src_ip);
void tcp_handle(uint8_t *data, uint32_t len, uint32_t src_ip);

/* ===== Helper functions ===== */

static uint16_t ip_checksum(const void *data, uint32_t len)
{
  const uint16_t *buf = (const uint16_t *)data;
  uint32_t sum = 0;

  while (len > 1) {
    sum += *buf++;
    len -= 2;
  }
  if (len) sum += *(const uint8_t *)buf;

  while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
  return (uint16_t)(~sum);
}

static uint16_t alloc_port(void)
{
  static uint32_t next_port = 49152;
  uint16_t p = next_port++;
  if (next_port > 65535) next_port = 49152;
  return p;
}

/* ===== ARP ===== */

static int arp_cache_find(uint32_t ip)
{
  for (int i = 0; i < BRIGHTS_ARP_CACHE_SIZE; ++i) {
    if (arp_cache[i].valid && arp_cache[i].ip == ip) return i;
  }
  return -1;
}

static int arp_cache_add(uint32_t ip, const uint8_t *mac)
{
  for (int i = 0; i < BRIGHTS_ARP_CACHE_SIZE; ++i) {
    if (!arp_cache[i].valid) {
      arp_cache[i].ip = ip;
      kutil_memcpy(arp_cache[i].mac, mac, 6);
      arp_cache[i].valid = 1;
      return 0;
    }
  }
  return -1; /* Cache full */
}

int brights_arp_resolve(uint32_t target_ip, uint8_t *mac_out)
{
  int idx = arp_cache_find(target_ip);
  if (idx >= 0) {
    kutil_memcpy(mac_out, arp_cache[idx].mac, 6);
    return 0;
  }
  return -1;
}

int brights_arp_request(uint32_t target_ip)
{
  if (netif_count == 0) return -1;

  brights_netif_t *iface = &netifs[0];
  uint8_t frame[BRIGHTS_NET_BUF_SIZE];
  kutil_memset(frame, 0, sizeof(frame));

  /* Ethernet header */
  eth_hdr_t *eth = (eth_hdr_t *)frame;
  kutil_memset(eth->dst, 0xFF, 6); /* Broadcast */
  kutil_memcpy(eth->src, iface->mac, 6);
  eth->type = 0x0608; /* ETH_TYPE_ARP (little-endian) */

  /* ARP payload */
  arp_hdr_t *arp = (arp_hdr_t *)(frame + sizeof(eth_hdr_t));
  arp->hw_type = 0x0100; /* Ethernet (little-endian) */
  arp->proto_type = 0x0008; /* IPv4 (little-endian) */
  arp->hw_size = 6;
  arp->proto_size = 4;
  arp->opcode = 0x0100; /* ARP_REQUEST (little-endian) */
  kutil_memcpy(arp->sender_mac, iface->mac, 6);
  arp->sender_ip = iface->ip_addr;
  kutil_memset(arp->target_mac, 0, 6);
  arp->target_ip = target_ip;

  uint32_t frame_len = sizeof(eth_hdr_t) + sizeof(arp_hdr_t);
  brights_net_send(frame, frame_len);
  return 0;
}

static void arp_handle(uint8_t *frame, uint32_t len)
{
  if (len < sizeof(eth_hdr_t) + sizeof(arp_hdr_t)) return;

  arp_hdr_t *arp = (arp_hdr_t *)(frame + sizeof(eth_hdr_t));
  uint16_t opcode = arp->opcode;

  if (netif_count == 0) return;
  brights_netif_t *iface = &netifs[0];

  /* Cache sender info */
  arp_cache_add(arp->sender_ip, arp->sender_mac);

  if (opcode == 0x0100) { /* ARP_REQUEST (little-endian) */
    if (arp->target_ip == iface->ip_addr) {
      /* Send ARP reply */
      uint8_t reply[BRIGHTS_NET_BUF_SIZE];
      kutil_memset(reply, 0, sizeof(reply));

      eth_hdr_t *eth = (eth_hdr_t *)reply;
      kutil_memcpy(eth->dst, arp->sender_mac, 6);
      kutil_memcpy(eth->src, iface->mac, 6);
      eth->type = 0x0608;

      arp_hdr_t *rarp = (arp_hdr_t *)(reply + sizeof(eth_hdr_t));
      rarp->hw_type = 0x0100;
      rarp->proto_type = 0x0008;
      rarp->hw_size = 6;
      rarp->proto_size = 4;
      rarp->opcode = 0x0200; /* ARP_REPLY (little-endian) */
      kutil_memcpy(rarp->sender_mac, iface->mac, 6);
      rarp->sender_ip = iface->ip_addr;
      kutil_memcpy(rarp->target_mac, arp->sender_mac, 6);
      rarp->target_ip = arp->sender_ip;

      uint32_t reply_len = sizeof(eth_hdr_t) + sizeof(arp_hdr_t);
      brights_net_send(reply, reply_len);
    }
  }
}

/* ===== IP ===== */

int brights_ip_send(uint32_t dst_ip, uint8_t protocol, const void *data, uint32_t len)
{
  if (netif_count == 0) return -1;

  brights_netif_t *iface = &netifs[0];
  uint8_t frame[BRIGHTS_NET_BUF_SIZE];
  kutil_memset(frame, 0, sizeof(frame));

  uint32_t ip_hdr_len = sizeof(ip_hdr_t);
  uint32_t total_len = ip_hdr_len + len;

  /* Build IP header */
  ip_hdr_t *ip = (ip_hdr_t *)(frame + sizeof(eth_hdr_t));
  ip->ver_ihl = 0x45; /* IPv4, 5 words */
  ip->tos = 0;
  ip->total_len = (uint16_t)((total_len >> 8) | (total_len << 8)); /* Big-endian */
  ip->ident = 0;
  ip->flags_frag = 0;
  ip->ttl = 64;
  ip->protocol = protocol;
  ip->checksum = 0;
  ip->src_ip = iface->ip_addr;
  ip->dst_ip = dst_ip;
  ip->checksum = ip_checksum(ip, ip_hdr_len);

  /* Copy payload */
  if (data && len > 0) {
    kutil_memcpy(frame + sizeof(eth_hdr_t) + ip_hdr_len, data, len);
  }

  /* Find destination MAC */
  uint8_t dst_mac[6];
  if (brights_arp_resolve(dst_ip, dst_mac) < 0) {
    /* Send ARP request first, then drop this packet */
    brights_arp_request(dst_ip);
    return -1;
  }

  /* Build Ethernet header */
  eth_hdr_t *eth = (eth_hdr_t *)frame;
  kutil_memcpy(eth->dst, dst_mac, 6);
  kutil_memcpy(eth->src, iface->mac, 6);
  eth->type = 0x0008; /* ETH_TYPE_IP (big-endian) */

  uint32_t frame_len = sizeof(eth_hdr_t) + total_len;
  brights_net_send(frame, frame_len);
  return 0;
}

static void ip_handle(uint8_t *frame, uint32_t len)
{
  if (len < sizeof(eth_hdr_t) + sizeof(ip_hdr_t)) return;

  ip_hdr_t *ip = (ip_hdr_t *)(frame + sizeof(eth_hdr_t));
  uint32_t ip_hdr_len = (ip->ver_ihl & 0x0F) * 4;
  uint32_t payload_len = ((ip->total_len >> 8) | (ip->total_len << 8)) - ip_hdr_len;
  uint8_t *payload = frame + sizeof(eth_hdr_t) + ip_hdr_len;

  if (netif_count == 0) return;
  brights_netif_t *iface = &netifs[0];

  /* Check if packet is for us */
  if (ip->dst_ip != iface->ip_addr && ip->dst_ip != 0xFFFFFFFF) return;

  switch (ip->protocol) {
    case IP_PROTO_ICMP:
      icmp_handle(payload, payload_len, ip->src_ip);
      break;
    case IP_PROTO_UDP:
      udp_handle(payload, payload_len, ip->src_ip);
      break;
    case IP_PROTO_TCP:
      tcp_handle(payload, payload_len, ip->src_ip);
      break;
  }
}

/* ===== ICMP ===== */

int brights_icmp_echo_request(uint32_t dst_ip)
{
  icmp_hdr_t icmp;
  icmp.type = ICMP_ECHO_REQUEST;
  icmp.code = 0;
  icmp.checksum = 0;
  icmp.ident = 0x1234;
  icmp.seq = 0x0001;
  icmp.checksum = ip_checksum(&icmp, sizeof(icmp));

  return brights_ip_send(dst_ip, IP_PROTO_ICMP, &icmp, sizeof(icmp));
}

void icmp_handle(uint8_t *data, uint32_t len, uint32_t src_ip)
{
  if (len < sizeof(icmp_hdr_t)) return;

  icmp_hdr_t *icmp = (icmp_hdr_t *)data;

  if (icmp->type == ICMP_ECHO_REQUEST) {
    /* Send echo reply */
    icmp->type = ICMP_ECHO_REPLY;
    icmp->checksum = 0;
    icmp->checksum = ip_checksum(icmp, len);

    brights_ip_send(src_ip, IP_PROTO_ICMP, icmp, len);
  }
}

/* ===== UDP ===== */

int brights_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const void *data, uint32_t len)
{
  udp_hdr_t udp;
  udp.src_port = (uint16_t)((src_port >> 8) | (src_port << 8)); /* Big-endian */
  udp.dst_port = (uint16_t)((dst_port >> 8) | (dst_port << 8));
  udp.length = (uint16_t)(((sizeof(udp_hdr_t) + len) >> 8) | ((sizeof(udp_hdr_t) + len) << 8));
  udp.checksum = 0;

  return brights_ip_send(dst_ip, IP_PROTO_UDP, &udp, sizeof(udp));
}

void udp_handle(uint8_t *data, uint32_t len, uint32_t src_ip)
{
  if (len < sizeof(udp_hdr_t)) return;

  udp_hdr_t *udp = (udp_hdr_t *)data;
  uint16_t dst_port = (uint16_t)((udp->dst_port >> 8) | (udp->dst_port << 8));

  /* Find socket listening on this port */
  for (int i = 0; i < BRIGHTS_NET_MAX_SOCKETS; ++i) {
    if (sockets[i].in_use && sockets[i].type == 2 && /* SOCK_DGRAM */
        sockets[i].local_port == dst_port) {
      /* Copy payload to socket buffer */
      uint32_t payload_len = len - sizeof(udp_hdr_t);
      if (payload_len > sizeof(sockets[i].recv_buf) - sockets[i].recv_len) {
        payload_len = sizeof(sockets[i].recv_buf) - sockets[i].recv_len;
      }
      if (payload_len > 0) {
        kutil_memcpy(sockets[i].recv_buf + sockets[i].recv_len,
                     data + sizeof(udp_hdr_t), payload_len);
        sockets[i].recv_len += payload_len;
        sockets[i].remote_ip = src_ip;
        sockets[i].remote_port = (uint16_t)((udp->src_port >> 8) | (udp->src_port << 8));
      }
      return;
    }
  }
}

/* ===== TCP ===== */

int brights_tcp_connect(uint32_t dst_ip, uint16_t dst_port)
{
  /* Find a free socket */
  int sockfd = -1;
  for (int i = 0; i < BRIGHTS_NET_MAX_SOCKETS; ++i) {
    if (!sockets[i].in_use) {
      sockfd = i;
      sockets[i].in_use = 1;
      break;
    }
  }
  if (sockfd < 0) return -1;

  brights_socket_t *sock = &sockets[sockfd];
  sock->domain = 1; /* AF_INET */
  sock->type = 1; /* SOCK_STREAM */
  sock->remote_ip = dst_ip;
  sock->remote_port = dst_port;
  sock->local_port = alloc_port();
  sock->tcp_state = 1; /* SYN_SENT */
  sock->tcp_seq = 1000; /* Initial sequence number */
  sock->tcp_ack = 0;
  sock->recv_len = 0;

  if (netif_count > 0) {
    sock->local_ip = netifs[0].ip_addr;
  }

  /* Send SYN */
  tcp_hdr_t tcp;
  kutil_memset(&tcp, 0, sizeof(tcp));
  tcp.src_port = (uint16_t)((sock->local_port >> 8) | (sock->local_port << 8));
  tcp.dst_port = (uint16_t)((dst_port >> 8) | (dst_port << 8));
  tcp.seq = (uint32_t)((sock->tcp_seq >> 24) | ((sock->tcp_seq >> 8) & 0xFF00) |
                       ((sock->tcp_seq << 8) & 0xFF0000) | (sock->tcp_seq << 24));
  tcp.data_off = 0x50; /* 5 words, no options */
  tcp.flags = TCP_SYN;
  tcp.window = 0xFFFF;

  brights_ip_send(dst_ip, IP_PROTO_TCP, &tcp, sizeof(tcp));
  return sockfd;
}

void tcp_handle(uint8_t *data, uint32_t len, uint32_t src_ip)
{
  if (len < sizeof(tcp_hdr_t)) return;

  tcp_hdr_t *tcp = (tcp_hdr_t *)data;
  uint16_t dst_port = (uint16_t)((tcp->dst_port >> 8) | (tcp->dst_port << 8));
  uint16_t src_port = (uint16_t)((tcp->src_port >> 8) | (tcp->src_port << 8));
  uint32_t seq = (uint32_t)((tcp->seq >> 24) | ((tcp->seq >> 8) & 0xFF00) |
                            ((tcp->seq << 8) & 0xFF0000) | (tcp->seq << 24));

  /* Find matching socket */
  for (int i = 0; i < BRIGHTS_NET_MAX_SOCKETS; ++i) {
    if (!sockets[i].in_use) continue;
    if (sockets[i].remote_ip != src_ip) continue;
    if (sockets[i].remote_port != src_port) continue;
    if (sockets[i].local_port != dst_port) continue;

    brights_socket_t *sock = &sockets[i];

    if (tcp->flags & TCP_SYN) {
      /* SYN received - send SYN+ACK */
      sock->tcp_state = 2; /* ESTABLISHED */
      sock->tcp_ack = seq + 1;

      tcp_hdr_t reply;
      kutil_memset(&reply, 0, sizeof(reply));
      reply.src_port = tcp->dst_port;
      reply.dst_port = tcp->src_port;
      reply.seq = (uint32_t)((sock->tcp_seq >> 24) | ((sock->tcp_seq >> 8) & 0xFF00) |
                             ((sock->tcp_seq << 8) & 0xFF0000) | (sock->tcp_seq << 24));
      reply.ack = (uint32_t)((sock->tcp_ack >> 24) | ((sock->tcp_ack >> 8) & 0xFF00) |
                             ((sock->tcp_ack << 8) & 0xFF0000) | (sock->tcp_ack << 24));
      reply.data_off = 0x50;
      reply.flags = TCP_SYN | TCP_ACK;
      reply.window = 0xFFFF;

      brights_ip_send(src_ip, IP_PROTO_TCP, &reply, sizeof(reply));
    } else if (tcp->flags & TCP_ACK) {
      /* ACK received */
      if (sock->tcp_state == 1) {
        /* SYN-ACK received - connection established */
        sock->tcp_state = 2;
        sock->tcp_ack = seq;
      }
    } else if (tcp->flags & TCP_FIN) {
      /* FIN received */
      sock->tcp_state = 3;
      sock->tcp_ack = seq + 1;

      /* Send ACK */
      tcp_hdr_t reply;
      kutil_memset(&reply, 0, sizeof(reply));
      reply.src_port = tcp->dst_port;
      reply.dst_port = tcp->src_port;
      reply.seq = (uint32_t)((sock->tcp_seq >> 24) | ((sock->tcp_seq >> 8) & 0xFF00) |
                             ((sock->tcp_seq << 8) & 0xFF0000) | (sock->tcp_seq << 24));
      reply.ack = (uint32_t)((sock->tcp_ack >> 24) | ((sock->tcp_ack >> 8) & 0xFF00) |
                             ((sock->tcp_ack << 8) & 0xFF0000) | (sock->tcp_ack << 24));
      reply.data_off = 0x50;
      reply.flags = TCP_ACK;
      reply.window = 0xFFFF;

      brights_ip_send(src_ip, IP_PROTO_TCP, &reply, sizeof(reply));
    } else if (tcp->flags & TCP_PSH || (tcp->flags & TCP_ACK && len > sizeof(tcp_hdr_t))) {
      /* Data received */
      uint32_t payload_len = len - ((tcp->data_off >> 4) * 4);
      if (payload_len > 0) {
        uint32_t avail = sizeof(sock->recv_buf) - sock->recv_len;
        if (payload_len > avail) payload_len = avail;
        if (payload_len > 0) {
          kutil_memcpy(sock->recv_buf + sock->recv_len,
                       data + ((tcp->data_off >> 4) * 4), payload_len);
          sock->recv_len += payload_len;
          sock->tcp_ack = seq + payload_len;

          /* Send ACK */
          tcp_hdr_t reply;
          kutil_memset(&reply, 0, sizeof(reply));
          reply.src_port = tcp->dst_port;
          reply.dst_port = tcp->src_port;
          reply.seq = (uint32_t)((sock->tcp_seq >> 24) | ((sock->tcp_seq >> 8) & 0xFF00) |
                                 ((sock->tcp_seq << 8) & 0xFF0000) | (sock->tcp_seq << 24));
          reply.ack = (uint32_t)((sock->tcp_ack >> 24) | ((sock->tcp_ack >> 8) & 0xFF00) |
                                 ((sock->tcp_ack << 8) & 0xFF0000) | (sock->tcp_ack << 24));
          reply.data_off = 0x50;
          reply.flags = TCP_ACK;
          reply.window = 0xFFFF;

          brights_ip_send(src_ip, IP_PROTO_TCP, &reply, sizeof(reply));
        }
      }
    }
    return;
  }
}

/* ===== Socket API ===== */

int brights_socket(int domain, int type, int protocol)
{
  (void)protocol;

  for (int i = 0; i < BRIGHTS_NET_MAX_SOCKETS; ++i) {
    if (!sockets[i].in_use) {
      sockets[i].in_use = 1;
      sockets[i].domain = domain;
      sockets[i].type = type;
      sockets[i].local_port = 0;
      sockets[i].remote_ip = 0;
      sockets[i].remote_port = 0;
      sockets[i].tcp_state = 0;
      sockets[i].recv_len = 0;
      if (netif_count > 0) {
        sockets[i].local_ip = netifs[0].ip_addr;
      }
      return i;
    }
  }
  return -1;
}

int brights_bind(int sockfd, uint32_t addr, uint16_t port)
{
  if (sockfd < 0 || sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;

  sockets[sockfd].local_ip = addr;
  sockets[sockfd].local_port = port;
  return 0;
}

int brights_listen(int sockfd, int backlog)
{
  (void)backlog;
  if (sockfd < 0 || sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;

  sockets[sockfd].tcp_state = 0; /* LISTEN state (simplified) */
  return 0;
}

int brights_accept(int sockfd, uint32_t *addr, uint16_t *port)
{
  (void)addr; (void)port;
  if (sockfd < 0 || sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;

  /* Return the same socket for now (simplified) */
  if (sockets[sockfd].tcp_state == 2) {
    if (addr) *addr = sockets[sockfd].remote_ip;
    if (port) *port = sockets[sockfd].remote_port;
    return sockfd;
  }
  return -1;
}

int brights_connect(int sockfd, uint32_t addr, uint16_t port)
{
  if (sockfd < 0 || sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;

  sockets[sockfd].remote_ip = addr;
  sockets[sockfd].remote_port = port;

  if (sockets[sockfd].type == 1) { /* SOCK_STREAM */
    return brights_tcp_connect(addr, port);
  }
  return 0;
}

int brights_send(int sockfd, const void *buf, uint32_t len)
{
  if (sockfd < 0 || sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;

  brights_socket_t *sock = &sockets[sockfd];

  if (sock->type == 2) { /* SOCK_DGRAM */
    return brights_udp_send(sock->remote_ip, sock->local_port,
                            sock->remote_port, buf, len);
  } else if (sock->type == 1) { /* SOCK_STREAM */
    if (sock->tcp_state != 2) return -1;

    tcp_hdr_t tcp;
    kutil_memset(&tcp, 0, sizeof(tcp));
    tcp.src_port = (uint16_t)((sock->local_port >> 8) | (sock->local_port << 8));
    tcp.dst_port = (uint16_t)((sock->remote_port >> 8) | (sock->remote_port << 8));
    tcp.seq = (uint32_t)((sock->tcp_seq >> 24) | ((sock->tcp_seq >> 8) & 0xFF00) |
                         ((sock->tcp_seq << 8) & 0xFF0000) | (sock->tcp_seq << 24));
    tcp.ack = (uint32_t)((sock->tcp_ack >> 24) | ((sock->tcp_ack >> 8) & 0xFF00) |
                         ((sock->tcp_ack << 8) & 0xFF0000) | (sock->tcp_ack << 24));
    tcp.data_off = 0x50;
    tcp.flags = TCP_ACK | TCP_PSH;
    tcp.window = 0xFFFF;

    int ret = brights_ip_send(sock->remote_ip, IP_PROTO_TCP, &tcp, sizeof(tcp));
    if (ret == 0) {
      sock->tcp_seq += len;
    }
    return ret == 0 ? (int)len : -1;
  }
  return -1;
}

int brights_recv(int sockfd, void *buf, uint32_t len)
{
  if (sockfd < 0 || sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;

  brights_socket_t *sock = &sockets[sockfd];
  if (sock->recv_len == 0) return 0;

  if (len > sock->recv_len) len = sock->recv_len;
  kutil_memcpy(buf, sock->recv_buf, len);

  /* Shift remaining data */
  if (len < sock->recv_len) {
    for (uint32_t i = 0; i < sock->recv_len - len; ++i) {
      sock->recv_buf[i] = sock->recv_buf[i + len];
    }
  }
  sock->recv_len -= len;
  return (int)len;
}

int brights_close(int sockfd)
{
  if (sockfd < 0 || sockfd >= BRIGHTS_NET_MAX_SOCKETS) return -1;
  if (!sockets[sockfd].in_use) return -1;

  brights_socket_t *sock = &sockets[sockfd];

  if (sock->type == 1 && sock->tcp_state == 2) {
    /* Send FIN */
    tcp_hdr_t tcp;
    kutil_memset(&tcp, 0, sizeof(tcp));
    tcp.src_port = (uint16_t)((sock->local_port >> 8) | (sock->local_port << 8));
    tcp.dst_port = (uint16_t)((sock->remote_port >> 8) | (sock->remote_port << 8));
    tcp.seq = (uint32_t)((sock->tcp_seq >> 24) | ((sock->tcp_seq >> 8) & 0xFF00) |
                         ((sock->tcp_seq << 8) & 0xFF0000) | (sock->tcp_seq << 24));
    tcp.ack = (uint32_t)((sock->tcp_ack >> 24) | ((sock->tcp_ack >> 8) & 0xFF00) |
                         ((sock->tcp_ack << 8) & 0xFF0000) | (sock->tcp_ack << 24));
    tcp.data_off = 0x50;
    tcp.flags = TCP_FIN | TCP_ACK;
    tcp.window = 0xFFFF;

    brights_ip_send(sock->remote_ip, IP_PROTO_TCP, &tcp, sizeof(tcp));
  }

  sock->in_use = 0;
  return 0;
}

/* ===== Network receive (called by driver) ===== */

void brights_net_recv(const uint8_t *frame, uint32_t len)
{
  if (len < sizeof(eth_hdr_t)) return;

  eth_hdr_t *eth = (eth_hdr_t *)frame;
  uint16_t type = (uint16_t)((eth->type >> 8) | (eth->type << 8)); /* Big-endian */

  switch (type) {
    case ETH_TYPE_ARP:
      arp_handle((uint8_t *)frame, len);
      break;
    case ETH_TYPE_IP:
      ip_handle((uint8_t *)frame, len);
      break;
  }
}

/* ===== Network send (calls driver) ===== */

int brights_net_send(const uint8_t *frame, uint32_t len)
{
  /* TODO: Call actual network driver (e.g., e1000, virtio-net) */
  /* For now, just print that we're sending */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "net: sending frame len=");

  /* Print length */
  char buf[16];
  int i = 0;
  uint32_t v = len;
  if (v == 0) { buf[i++] = '0'; }
  else { while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; } }
  buf[i] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  (void)frame;
  return 0;
}

/* ===== Interface management ===== */

int brights_netif_add(const char *name, const uint8_t *mac)
{
  if (netif_count >= BRIGHTS_NET_MAX_IF) return -1;

  brights_netif_t *iface = &netifs[netif_count];
  int i = 0;
  while (name[i] && i < 15) { iface->name[i] = name[i]; ++i; }
  iface->name[i] = 0;

  kutil_memcpy(iface->mac, mac, 6);
  iface->ip_addr = 0;
  iface->netmask = 0;
  iface->gateway = 0;
  iface->up = 0;

  ++netif_count;
  return 0;
}

int brights_netif_set_ip(const char *name, uint32_t ip, uint32_t netmask, uint32_t gateway)
{
  for (int i = 0; i < netif_count; ++i) {
    if (netifs[i].name[0] == name[0]) {
      netifs[i].ip_addr = ip;
      netifs[i].netmask = netmask;
      netifs[i].gateway = gateway;
      return 0;
    }
  }
  return -1;
}

int brights_netif_up(const char *name)
{
  for (int i = 0; i < netif_count; ++i) {
    if (netifs[i].name[0] == name[0]) {
      netifs[i].up = 1;
      return 0;
    }
  }
  return -1;
}

int brights_netif_down(const char *name)
{
  for (int i = 0; i < netif_count; ++i) {
    if (netifs[i].name[0] == name[0]) {
      netifs[i].up = 0;
      return 0;
    }
  }
  return -1;
}

/* ===== Init ===== */

void brights_net_init(void)
{
    netif_count = 0;
    kutil_memset(arp_cache, 0, sizeof(arp_cache));
    kutil_memset(sockets, 0, sizeof(sockets));
    
    /* Initialize network subsystems */
    brights_dhcp_client_init();
    brights_dns_init("8.8.8.8", "8.8.4.4");
    brights_http_init();
}

/* ===== IP address utilities ===== */

uint32_t brights_ip_parse(const char *str)
{
  uint32_t a = 0, b = 0, c = 0, d = 0;
  int part = 0;
  uint32_t val = 0;

  while (*str) {
    if (*str >= '0' && *str <= '9') {
      val = val * 10 + (uint32_t)(*str - '0');
    } else if (*str == '.') {
      if (part == 0) a = val;
      else if (part == 1) b = val;
      else if (part == 2) c = val;
      else return 0;
      ++part;
      val = 0;
    } else {
      return 0;
    }
    ++str;
  }
  d = val;
  if (part != 3) return 0;

  return (a << 24) | (b << 16) | (c << 8) | d;
}

void brights_ip_to_str(uint32_t ip, char *buf)
{
  uint8_t a = (uint8_t)((ip >> 24) & 0xFF);
  uint8_t b = (uint8_t)((ip >> 16) & 0xFF);
  uint8_t c = (uint8_t)((ip >> 8) & 0xFF);
  uint8_t d = (uint8_t)(ip & 0xFF);

  int p = 0;

  /* Write a */
  if (a >= 100) { buf[p++] = '0' + (a / 100); buf[p++] = '0' + ((a / 10) % 10); buf[p++] = '0' + (a % 10); }
  else if (a >= 10) { buf[p++] = '0' + (a / 10); buf[p++] = '0' + (a % 10); }
  else { buf[p++] = '0' + a; }

  buf[p++] = '.';

  /* Write b */
  if (b >= 100) { buf[p++] = '0' + (b / 100); buf[p++] = '0' + ((b / 10) % 10); buf[p++] = '0' + (b % 10); }
  else if (b >= 10) { buf[p++] = '0' + (b / 10); buf[p++] = '0' + (b % 10); }
  else { buf[p++] = '0' + b; }

  buf[p++] = '.';

  /* Write c */
  if (c >= 100) { buf[p++] = '0' + (c / 100); buf[p++] = '0' + ((c / 10) % 10); buf[p++] = '0' + (c % 10); }
  else if (c >= 10) { buf[p++] = '0' + (c / 10); buf[p++] = '0' + (c % 10); }
  else { buf[p++] = '0' + c; }

  buf[p++] = '.';

  /* Write d */
  if (d >= 100) { buf[p++] = '0' + (d / 100); buf[p++] = '0' + ((d / 10) % 10); buf[p++] = '0' + (d % 10); }
  else if (d >= 10) { buf[p++] = '0' + (d / 10); buf[p++] = '0' + (d % 10); }
  else { buf[p++] = '0' + d; }

  buf[p] = 0;
}
