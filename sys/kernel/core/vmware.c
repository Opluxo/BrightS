#include "vmware.h"
#include "../arch/x86_64/io.h"
#include "../drivers/serial.h"
#include <stdint.h>

static int vmware_available = 0;

int brights_vmware_backdoor_cmd(uint32_t cmd, uint32_t in_eax, uint32_t *out_eax)
{
    if (!vmware_available) {
        return -1;
    }

    uint32_t eax = in_eax;
    uint32_t ebx = VMWARE_MAGIC;
    uint32_t ecx = cmd;
    uint32_t edx = VMWARE_MAGIC;
    
    __asm__ __volatile__(
        "inl %%dx, %0"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(eax), "b"(ebx), "c"(ecx), "d"(edx)
        : "memory"
    );
    
    if (out_eax) {
        *out_eax = eax;
    }
    
    return (ebx == VMWARE_MAGIC) ? 0 : -1;
}

void brights_vmware_backdoor_init(void)
{
    uint32_t version = 0;
    if (brights_vmware_backdoor_cmd(VMWARE_CMD_GET_VERSION, 0, &version) == 0) {
        vmware_available = 1;
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "vmware: backdoor initialized\r\n");
    } else {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "vmware: backdoor not available\r\n");
    }
}

int brights_vmware_get_version(void)
{
    if (!vmware_available) {
        return -1;
    }
    
    uint32_t version = 0;
    if (brights_vmware_backdoor_cmd(VMWARE_CMD_GET_VERSION, 0, &version) == 0) {
        return (int)version;
    }
    return -1;
}

uint32_t brights_vmware_get_time(void)
{
    if (!vmware_available) {
        return 0;
    }
    
    uint32_t time = 0;
    brights_vmware_backdoor_cmd(VMWARE_CMD_GET_TIME, 0, &time);
    return time;
}