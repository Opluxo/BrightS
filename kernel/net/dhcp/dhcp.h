#ifndef BRIGHTS_NET_DHCP_H
#define BRIGHTS_NET_DHCP_H

#include <stdint.h>
#include "../net.h"

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67

#define DHCP_OP_REQUEST 1
#define DHCP_OP_REPLY   2

#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6

#define DHCP_MSG_SIZE 236

#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_DECLINE  4
#define DHCP_ACK      5
#define DHCP_NAK      6
#define DHCP_RELEASE  7
#define DHCP_INFORM   8

#define DHCP_OPTION_SUBNET_MASK        1
#define DHCP_OPTION_ROUTERS            3
#define DHCP_OPTION_DNS_SERVERS        6
#define DHCP_OPTION_DOMAIN_NAME        15
#define DHCP_OPTION_REQUESTED_IP       50
#define DHCP_OPTION_LEASE_TIME         51
#define DHCP_OPTION_OVERLOAD           52
#define DHCP_OPTION_TFTP_SERVERNAME    66
#define DHCP_OPTION_BOOTFILE_NAME      67
#define DHCP_OPTION_END                255

#pragma pack(push, 1)
typedef struct {
    uint8_t op;           /* Message op code/message type */
    uint8_t htype;        /* Hardware address type */
    uint8_t hlen;         /* Hardware address length */
    uint8_t hops;         /* Gateway hops */
    uint32_t xid;         /* Transaction ID */
    uint16_t secs;        /* Seconds since client began addressing */
    uint16_t flags;       /* Flags */
    uint32_t ciaddr;      /* Client IP address */
    uint32_t yiaddr;      /* 'Your' IP address */
    uint32_t siaddr;      /* Server IP address */
    uint32_t giaddr;      /* Gateway IP address */
    uint8_t chaddr[16];   /* Client hardware address */
    uint8_t sname[64];    /* Server host name */
    uint8_t file[128];    /* Boot file name */
    uint8_t options[312]; /* Options field */
} dhcp_packet_t;
#pragma pack(pop)

int brights_dhcp_client_init(void);
int brights_dhcp_request_lease(const char *interface, uint32_t *ip_out, 
                              uint32_t *netmask_out, uint32_t *gateway_out,
                              uint32_t *dns1_out, uint32_t *dns2_out);
void brights_dhcp_release_lease(uint32_t ip_addr);

#endif