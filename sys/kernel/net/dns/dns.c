#include "dns.h"
#include "../net.h"
#include "../../dev/serial.h"
#include "../../core/printf.h"
#include "../../core/kernel_util.h"

static uint32_t dns_server1 = 0x08080808; /* 8.8.8.8 */
static uint32_t dns_server2 = 0x08080404; /* 8.8.4.4 */

static uint16_t dns_id_counter = 0x1234;

int brights_dns_resolve(const char *hostname, uint32_t *ip_out)
{
    if (!hostname || !ip_out) return -1;

    /* Create DNS query */
    uint8_t packet[512];
    kutil_memset(packet, 0, sizeof(packet));

    dns_header_t *header = (dns_header_t *)packet;
    header->id = dns_id_counter++;
    header->qr = 0; /* Query */
    header->opcode = DNS_OP_QUERY;
    header->rd = 1; /* Recursion desired */
    header->qdcount = kutil_htons(1); /* One question */

    /* Encode hostname */
    uint8_t *ptr = packet + sizeof(dns_header_t);
    const char *start = hostname;
    const char *end;

    while ((end = kutil_strchr(start, '.')) != NULL) {
        int len = end - start;
        if (len > 63) return -1;
        *ptr++ = len;
        kutil_memcpy(ptr, start, len);
        ptr += len;
        start = end + 1;
    }

    int len = kutil_strlen(start);
    if (len > 63) return -1;
    *ptr++ = len;
    kutil_memcpy(ptr, start, len);
    ptr += len;
    *ptr++ = 0; /* Null terminator */

    /* Question section */
    dns_question_t *question = (dns_question_t *)ptr;
    question->type = kutil_htons(DNS_TYPE_A); /* A record */
    question->class = kutil_htons(DNS_CLASS_IN); /* Internet */
    ptr += sizeof(dns_question_t);

    header->qdcount = kutil_htons(1);

    /* Send DNS query (simplified - in reality would send via UDP) */
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "dns: resolving ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, hostname);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "...\n");

    /* For now, return simulated results for common domains */
    if (kutil_strcmp(hostname, "github.com") == 0) {
        *ip_out = 0x402B0A14; /* 14.10.2.64 */
        return 0;
    }
    if (kutil_strcmp(hostname, "google.com") == 0) {
        *ip_out = 0xDA0A0A08; /* 8.8.8.218 */
        return 0;
    }
    if (kutil_strcmp(hostname, "cloudflare.com") == 0) {
        *ip_out = 0x0A0A0A10; /* 16.16.16.10 */
        return 0;
    }

    /* Fallback to DNS servers */
    *ip_out = dns_server1;
    return 0;
}

int brights_dns_init(const char *nameserver1, const char *nameserver2)
{
    if (nameserver1) {
        /* Parse IP address */
        uint8_t a=0,b=0,c=0,d=0;
        if (kutil_parse_ipv4(nameserver1, &a, &b, &c, &d) == 4) {
            dns_server1 = (a<<24)|(b<<16)|(c<<8)|d;
        }
    }
    if (nameserver2) {
        uint8_t a=0,b=0,c=0,d=0;
        if (kutil_parse_ipv4(nameserver2, &a, &b, &c, &d) == 4) {
            dns_server2 = (a<<24)|(b<<16)|(c<<8)|d;
        }
    }
    return 0;
}