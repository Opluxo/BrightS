#ifndef BRIGHTS_VM_H
#define BRIGHTS_VM_H

#include <stdint.h>

void brights_vm_init(void);
void brights_vm_bootstrap(uint64_t base, uint64_t size);

#endif
