#include "daemon.h"
#include "libc.h"

/*
 * DHCP Client Daemon (dhcpd)
 */

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67
#define DHCP_TIMEOUT_MS 10000

typedef struct {
    uint8_t op;      /* Message type */
    uint8_t htype;   /* Hardware type */
    uint8_t hlen;    /* Hardware address length */
    uint8_t hops;    /* Hops */
    uint32_t xid;    /* Transaction ID */
    uint16_t secs;   /* Seconds elapsed */
    uint16_t flags;  /* Flags */
    uint32_t ciaddr; /* Client IP address */
    uint32_t yiaddr; /* Your IP address */
    uint32_t siaddr; /* Server IP address */
    uint32_t giaddr; /* Gateway IP address */
    uint8_t chaddr[16]; /* Client hardware address */
    uint8_t sname[64];  /* Server name */
    uint8_t file[128];  /* Boot file name */
    uint32_t cookie;    /* Magic cookie */
    uint8_t options[312]; /* Options */
} dhcp_packet_t;

static uint32_t client_ip = 0;
static uint32_t server_ip = 0;
static uint32_t gateway_ip = 0;
static uint32_t dns_ip = 0;

/*
 * Initialize DHCP client
 */
static int dhcpd_init(void)
{
    printf("dhcpd: initializing DHCP client\n");

    /* Create DHCP socket */
    return 0;
}

/*
 * Send DHCP discover message
 */
static int dhcp_send_discover(int sockfd, uint32_t xid)
{
    dhcp_packet_t packet;
    memset(&packet, 0, sizeof(packet));

    packet.op = 1;        /* BOOTREQUEST */
    packet.htype = 1;     /* Ethernet */
    packet.hlen = 6;      /* MAC address length */
    packet.hops = 0;
    packet.xid = xid;
    packet.secs = 0;
    packet.flags = 0x8000; /* Broadcast */
    packet.ciaddr = 0;
    packet.yiaddr = 0;
    packet.siaddr = 0;
    packet.giaddr = 0;

    /* Client MAC address (simplified) */
    packet.chaddr[0] = 0x52;
    packet.chaddr[1] = 0x54;
    packet.chaddr[2] = 0x00;
    packet.chaddr[3] = 0x12;
    packet.chaddr[4] = 0x34;
    packet.chaddr[5] = 0x56;

    packet.cookie = 0x63825363; /* DHCP magic cookie */

    /* DHCP options */
    uint8_t *opts = packet.options;
    *opts++ = 53; /* DHCP Message Type */
    *opts++ = 1;
    *opts++ = 1;  /* DHCPDISCOVER */

    *opts++ = 55; /* Parameter Request List */
    *opts++ = 4;  /* Length */
    *opts++ = 1;  /* Subnet Mask */
    *opts++ = 3;  /* Router */
    *opts++ = 6;  /* DNS */
    *opts++ = 15; /* Domain Name */

    *opts++ = 255; /* End */

    /* Send to broadcast address */
    return sys_send(sockfd, &packet, sizeof(packet));
}

/*
 * Process DHCP offer
 */
static int dhcp_process_offer(dhcp_packet_t *packet)
{
    if (packet->yiaddr == 0) return -1;

    client_ip = packet->yiaddr;
    server_ip = packet->siaddr;

    printf("dhcpd: offered IP %d.%d.%d.%d\n",
           (client_ip >> 24) & 0xFF, (client_ip >> 16) & 0xFF,
           (client_ip >> 8) & 0xFF, client_ip & 0xFF);

    return 0;
}

/*
 * Send DHCP request
 */
static int dhcp_send_request(int sockfd, uint32_t xid)
{
    dhcp_packet_t packet;
    memset(&packet, 0, sizeof(packet));

    packet.op = 1;        /* BOOTREQUEST */
    packet.htype = 1;     /* Ethernet */
    packet.hlen = 6;      /* MAC address length */
    packet.xid = xid;
    packet.ciaddr = 0;
    packet.yiaddr = 0;
    packet.siaddr = server_ip;
    packet.giaddr = 0;

    /* Client MAC address */
    packet.chaddr[0] = 0x52;
    packet.chaddr[1] = 0x54;
    packet.chaddr[2] = 0x00;
    packet.chaddr[3] = 0x12;
    packet.chaddr[4] = 0x34;
    packet.chaddr[5] = 0x56;

    packet.cookie = 0x63825363;

    /* DHCP options */
    uint8_t *opts = packet.options;
    *opts++ = 53; /* DHCP Message Type */
    *opts++ = 1;
    *opts++ = 3;  /* DHCPREQUEST */

    *opts++ = 50; /* Requested IP Address */
    *opts++ = 4;
    memcpy(opts, &client_ip, 4);
    opts += 4;

    *opts++ = 54; /* DHCP Server Identifier */
    *opts++ = 4;
    memcpy(opts, &server_ip, 4);
    opts += 4;

    *opts++ = 255; /* End */

    return sys_send(sockfd, &packet, sizeof(packet));
}

/*
 * Process DHCP ACK
 */
static int dhcp_process_ack(dhcp_packet_t *packet)
{
    if (packet->yiaddr != client_ip) return -1;

    /* Parse options for gateway and DNS */
    uint8_t *opts = packet->options;
    while (*opts != 255 && opts < packet->options + sizeof(packet->options)) {
        uint8_t code = *opts++;
        uint8_t len = *opts++;

        switch (code) {
        case 1: /* Subnet Mask */
            if (len == 4) {
                uint32_t subnet;
                memcpy(&subnet, opts, 4);
                printf("dhcpd: subnet mask %d.%d.%d.%d\n",
                       (subnet >> 24) & 0xFF, (subnet >> 16) & 0xFF,
                       (subnet >> 8) & 0xFF, subnet & 0xFF);
            }
            break;
        case 3: /* Router */
            if (len == 4) {
                memcpy(&gateway_ip, opts, 4);
                printf("dhcpd: gateway %d.%d.%d.%d\n",
                       (gateway_ip >> 24) & 0xFF, (gateway_ip >> 16) & 0xFF,
                       (gateway_ip >> 8) & 0xFF, gateway_ip & 0xFF);
            }
            break;
        case 6: /* DNS */
            if (len == 4) {
                memcpy(&dns_ip, opts, 4);
                printf("dhcpd: DNS %d.%d.%d.%d\n",
                       (dns_ip >> 24) & 0xFF, (dns_ip >> 16) & 0xFF,
                       (dns_ip >> 8) & 0xFF, dns_ip & 0xFF);
            }
            break;
        }

        opts += len;
    }

    /* Configure network interface */
    if (sys_ifconfig(1) == 0) { /* IFCONFIG_SET_IP */
        printf("dhcpd: network configured successfully\n");
        return 0;
    }

    return -1;
}

/*
 * DHCP client main loop
 */
static void dhcpd_main(void)
{
    int sockfd = sys_socket(2, 2, 17); /* AF_INET, SOCK_DGRAM, UDP */
    if (sockfd < 0) {
        printf("dhcpd: failed to create socket\n");
        return;
    }

    /* Bind to client port */
    if (sys_bind(sockfd, 0, DHCP_CLIENT_PORT) < 0) {
        printf("dhcpd: failed to bind to port %d\n", DHCP_CLIENT_PORT);
        sys_close_socket(sockfd);
        return;
    }

    printf("dhcpd: starting DHCP client\n");

    uint32_t xid = sys_clock_ms(); /* Use timestamp as transaction ID */
    int state = 0; /* 0: discover, 1: request, 2: bound */

    int64_t last_send = 0;

    while (1) {
        int64_t now = sys_clock_ms();

        /* Send DHCP messages */
        if (state == 0 && (last_send == 0 || now - last_send > 5000)) {
            if (dhcp_send_discover(sockfd, xid) > 0) {
                printf("dhcpd: sent DHCPDISCOVER\n");
                last_send = now;
            }
        } else if (state == 1 && now - last_send > 2000) {
            if (dhcp_send_request(sockfd, xid) > 0) {
                printf("dhcpd: sent DHCPREQUEST\n");
                last_send = now;
            }
        }

        /* Receive DHCP responses */
        dhcp_packet_t response;
        int64_t n = sys_recv(sockfd, &response, sizeof(response));
        if (n > 0 && response.xid == xid) {
            if (response.op == 2) { /* BOOTREPLY */
                uint8_t msg_type = 0;
                uint8_t *opts = response.options;
                while (*opts != 255 && opts < response.options + sizeof(response.options)) {
                    if (*opts == 53 && *(opts + 1) == 1) {
                        msg_type = *(opts + 2);
                        break;
                    }
                    uint8_t len = *(opts + 1);
                    opts += 2 + len;
                }

                if (msg_type == 2 && state == 0) { /* DHCPOFFER */
                    if (dhcp_process_offer(&response) == 0) {
                        state = 1;
                        last_send = 0; /* Send request immediately */
                    }
                } else if (msg_type == 5 && state == 1) { /* DHCPACK */
                    if (dhcp_process_ack(&response) == 0) {
                        state = 2;
                        printf("dhcpd: DHCP lease acquired\n");
                    }
                }
            }
        }

        /* Check lease timeout (simplified) */
        if (state == 2 && now - last_send > 300000) { /* 5 minutes */
            printf("dhcpd: lease expired, restarting\n");
            state = 0;
            last_send = 0;
            client_ip = 0;
        }

        sys_sleep_ms(1000);
    }

    sys_close_socket(sockfd);
}

/*
 * Cleanup DHCP client
 */
static void dhcpd_cleanup(void)
{
    printf("dhcpd: shutdown\n");
}

/*
 * DHCP daemon info
 */
const daemon_info_t dhcpd_info = {
    .name = "dhcpd",
    .init_func = dhcpd_init,
    .main_loop = dhcpd_main,
    .cleanup_func = dhcpd_cleanup,
    .auto_restart = 1,
    .restart_delay_ms = 10000
};