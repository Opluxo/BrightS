#ifndef BRIGHTS_NET_DNS_H
#define BRIGHTS_NET_DNS_H

#include <stdint.h>
#include "../net.h"

#define DNS_PORT 53
#define DNS_MAX_UDP_SIZE 512
#define DNS_MAX_NAME 255
#define DNS_MAX_LABEL 63

#define DNS_OP_QUERY 0
#define DNS_OP_IQUERY 1
#define DNS_OP_STATUS  2

#define DNS_RCODE_NOERROR  0
#define DNS_RCODE_FORMERR  1
#define DNS_RCODE_SERVFAIL 2
#define DNS_RCODE_NXDOMAIN 3
#define DNS_RCODE_NOTIMP   4
#define DNS_RCODE_REFUSED  5

#define DNS_TYPE_A       1   /* Host address */
#define DNS_TYPE_NS      2   /* Authoritative name server */
#define DNS_TYPE_CNAME   5   /* Canonical name */
#define DNS_TYPE_SOA     6   /* Start of authority zone */
#define DNS_TYPE_PTR     12  /* Domain name pointer */
#define DNS_TYPE_MX      15  /* Mail exchange */
#define DNS_TYPE_TXT     16  /* Text string */
#define DNS_TYPE_AAAA    28  /* IPv6 address */

#define DNS_CLASS_IN     1   /* the Internet */
#define DNS_CLASS_CS     2   /* the CSNET class */
#define DNS_CLASS_CH     3   /* the CHAOS class */
#define DNS_CLASS_HS     4   /* Hesiod */
#define DNS_CLASS_NONE   254
#define DNS_CLASS_ANY    255

#pragma pack(push, 1)
typedef struct {
    uint16_t id;          /* Identification number */
    
    uint8_t rd :1;        /* Recursion desired */
    uint8_t tc :1;        /* Truncated message */
    uint8_t aa :1;        /* Authoritative answer */
    uint8_t opcode :4;    /* Purpose of message */
    uint8_t qr :1;        /* Query/Response flag */
    
    uint8_t rcode :4;     /* Response code */
    uint8_t cd :1;        /* Checking disabled */
    uint8_t ad :1;        /* Authenticated data */
    uint8_t z :1;         /* Reserved */
    uint8_t ra :1;        /* Recursion available */
    
    uint16_t qdcount;     /* Number of question entries */
    uint16_t ancount;     /* Number of answer RRs */
    uint16_t nscount;     /* Number of authority RRs */
    uint16_t arcount;     /* Number of resource RRs */
} dns_header_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint16_t type;        /* Type of query */
    uint16_t class;       /* Class of query */
} dns_question_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint16_t type;        /* Type of resource record */
    uint16_t class;       /* Class of resource record */
    uint32_t ttl;         /* Time to live */
    uint16_t rdlength;    /* Length of resource data */
} dns_rr_t;
#pragma pack(pop)

int brights_dns_resolve(const char *hostname, uint32_t *ip_out);
int brights_dns_init(const char *nameserver1, const char *nameserver2);

#endif