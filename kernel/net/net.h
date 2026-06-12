#ifndef BRIGHTS_NET_H
#define BRIGHTS_NET_H

#include <stdint.h>

/* ===== Byte-order helpers ===== */
static inline uint16_t htons(uint16_t v)
{
  return (uint16_t)((v >> 8) | (v << 8));
}
static inline uint16_t ntohs(uint16_t v) { return htons(v); }

static inline uint32_t htonl(uint32_t v)
{
  return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) |
         ((v << 8) & 0xFF0000) | (v << 24);
}
static inline uint32_t ntohl(uint32_t v) { return htonl(v); }

/* Network constants */
#define BRIGHTS_NET_MAX_IF 4
#define BRIGHTS_NET_MAX_DRIVERS 4
#define BRIGHTS_NET_BUF_SIZE 1518

/* Socket option levels */
#define SOL_SOCKET  1
#define IPPROTO_IP  2

/* SOL_SOCKET option names */
#define SO_REUSEADDR 1
#define SO_KEEPALIVE 4
#define SO_LINGER    8
#define SO_BROADCAST 13
#define SO_SNDBUF    32
#define SO_RCVBUF    33

/* IPPROTO_IP option names */
#define IP_TOS      1
#define IP_TTL      2
#define IP_HDRINCL  3

/* IP header constants */
#define IP_VERSION_IHL  0x45
#define IP_DEFAULT_TTL  64

/* TCP header constants */
#define TCP_DATA_OFFSET_5WORDS 0x50
#define TCP_MAX_WINDOW         0xFFFF

/* TCP states */
#define TCP_STATE_CLOSED      0
#define TCP_STATE_LISTEN      1
#define TCP_STATE_SYN_SENT    2
#define TCP_STATE_SYN_RCVD    3
#define TCP_STATE_ESTABLISHED 4
#define TCP_STATE_FIN_WAIT_1  5
#define TCP_STATE_FIN_WAIT_2  6
#define TCP_STATE_CLOSING     7
#define TCP_STATE_TIME_WAIT   8
#define TCP_STATE_CLOSE_WAIT  9
#define TCP_STATE_LAST_ACK    10

/* Network driver operations */
typedef struct {
  int (*init)(void);
  int (*send)(const uint8_t *frame, uint32_t len);
  int (*recv)(uint8_t *frame, uint32_t *len);
  int (*poll)(void);
} brights_net_driver_ops_t;

/* Network driver */
typedef struct {
  const char *name;
  int initialized;
  brights_net_driver_ops_t ops;
} brights_net_driver_t;

/* Register a network driver */
int brights_net_register_driver(brights_net_driver_t *driver);

/* Get network driver for interface */
brights_net_driver_t *brights_net_get_driver(int if_idx);

/* Network interface extended */
typedef struct {
  char name[16];
  uint8_t mac[6];
  uint32_t ip_addr;
  uint32_t netmask;
  uint32_t gateway;
  int up;
  int driver_idx;
} brights_netif_t;

/* External network interfaces array */
extern brights_netif_t netifs[BRIGHTS_NET_MAX_IF];

/* Socket API internals (for syscall use) */
#define BRIGHTS_NET_MAX_SOCKETS 32

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
  uint32_t tcp_state;
  uint32_t tcp_seq;
  uint32_t tcp_ack;
  uint64_t last_send_tick;   /* For retransmission timeout */
  uint32_t rto_ms;           /* Retransmission timeout in ms */
  uint32_t retransmit_count; /* Number of retransmissions */
} brights_socket_t;

extern brights_socket_t sockets[BRIGHTS_NET_MAX_SOCKETS];

/* Initialize network subsystem */
void brights_net_init(void);

/* Add a network interface */
int brights_netif_add(const char *name, const uint8_t *mac);

/* Set interface IP address */
int brights_netif_set_ip(const char *name, uint32_t ip, uint32_t netmask, uint32_t gateway);

/* Bring interface up/down */
int brights_netif_up(const char *name);
int brights_netif_down(const char *name);

/* Poll all drivers for incoming frames */
void brights_net_poll_all(void);

/* Send raw Ethernet frame */
int brights_net_send(const uint8_t *frame, uint32_t len);

/* Receive raw Ethernet frame (called by driver) */
void brights_net_recv(const uint8_t *frame, uint32_t len);

/* IP address helpers */
uint32_t brights_ip_parse(const char *str);
void brights_ip_to_str(uint32_t ip, char *buf);

/* ARP */
int brights_arp_request(uint32_t target_ip);
int brights_arp_resolve(uint32_t target_ip, uint8_t *mac_out);

/* IP */
int brights_ip_send(uint32_t dst_ip, uint8_t protocol, const void *data, uint32_t len);

/* ICMP */
int brights_icmp_echo_request(uint32_t dst_ip);

/* UDP */
int brights_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const void *data, uint32_t len);

/* TCP */
int brights_tcp_connect(uint32_t dst_ip, uint16_t dst_port);

/* Socket API */
int brights_socket(int domain, int type, int protocol);
int brights_bind(int sockfd, uint32_t addr, uint16_t port);
int brights_listen(int sockfd, int backlog);
int brights_accept(int sockfd, uint32_t *addr, uint16_t *port);
int brights_connect(int sockfd, uint32_t addr, uint16_t port);
int brights_send(int sockfd, const void *buf, uint32_t len);
int brights_recv(int sockfd, void *buf, uint32_t len);
int brights_close(int sockfd);

#endif
