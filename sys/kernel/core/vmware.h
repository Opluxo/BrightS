#ifndef BRIGHTS_VMWARE_H
#define BRIGHTS_VMWARE_H

#include <stdint.h>

/* VMware I/O ports */
#define VMWARE_PORT_CMD    0x5658u  /* "VX" */
#define VMWARE_PORT_DATA    0x5659u  /* "VY" */

/* VMware backdoor commands */
#define VMWARE_CMD_GET_VERSION     0x0a
#define VMWARE_CMD_GET_TIME          0x0b
#define VMWARE_CMD_GET_HARDWARE_INFO 0x10

/* VMware backdoor magic */
#define VMWARE_MAGIC 0x564D5868u  /* "VMXh" */

/* VMware backdoor status */
#define VMWARE_STATUS_SUCCESS 0x0000u
#define VMWARE_STATUS_ERROR   0x0001u

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
} __attribute__((packed)) vmware_backdoor_regs_t;

/* VMware backdoor functions */
int brights_vmware_backdoor_cmd(uint32_t cmd, uint32_t in_eax, uint32_t *out_eax);
void brights_vmware_backdoor_init(void);
int brights_vmware_get_version(void);
uint32_t brights_vmware_get_time(void);

#endif