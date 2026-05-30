#ifndef BRIGHTS_NET_H
#define BRIGHTS_NET_H

#include <stdint.h>

/* Network constants */
#define BRIGHTS_NET_MAX_IF 4
#define BRIGHTS_NET_MAX_DRIVERS 4
#define BRIGHTS_NET_BUF_SIZE 1518

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
