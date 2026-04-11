#include "command.h"
#include "libc.h"

/*
 * ping help
 */
static void cmd_ping_help(void)
{
    printf("Usage: ping HOST\n");
    printf("Send ICMP echo requests to HOST.\n\n");
    printf("HOST can be an IP address or hostname.\n");
}

/*
 * ping command handler
 */
int cmd_ping_handler(int argc, char **argv)
{
    if (argc < 2) {
        printf("ping: missing host\n");
        printf("Usage: ping host\n");
        return 1;
    }

    const char *host = argv[1];

    /* Parse IP address */
    uint32_t ip = sys_ip_parse(host);
    if (ip == 0) {
        printf("ping: cannot resolve '%s'\n", host);
        return 1;
    }

    printf("PING %s (%u.%u.%u.%u)\n",
           host,
           (ip >> 24) & 0xFF,
           (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF,
           ip & 0xFF);

    /* Send a few pings */
    for (int i = 0; i < 4; ++i) {
        int64_t start = sys_clock_ms();
        int result = sys_icmp_echo(ip);
        int64_t end = sys_clock_ms();

        if (result == 0) {
            printf("64 bytes from %u.%u.%u.%u: icmp_seq=%d ttl=64 time=%lld ms\n",
                   (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF,
                   i + 1, end - start);
        } else {
            printf("Request timeout for icmp_seq %d\n", i + 1);
        }

        sys_sleep_ms(1000);
    }

    return 0;
}