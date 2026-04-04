#include "libc.h"

/* Simple ping command for BrightS */

int main(int argc, char **argv)
{
  if (argc < 2) {
    printf("usage: ping <ip_address>\n");
    return 1;
  }

  /* Parse IP address */
  uint32_t dst_ip = brights_ip_parse(argv[1]);
  if (dst_ip == 0) {
    printf("ping: invalid address '%s'\n", argv[1]);
    return 1;
  }

  char ip_str[20];
  brights_ip_to_str(dst_ip, ip_str);
  printf("PING %s\n", ip_str);

  /* Send ICMP echo request */
  int ret = sys_icmp_echo(dst_ip);
  if (ret < 0) {
    printf("ping: failed to send\n");
    return 1;
  }

  printf("ping sent ok\n");
  return 0;
}
