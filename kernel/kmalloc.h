#ifndef BRIGHTS_KMALLOC_H
#define BRIGHTS_KMALLOC_H

#include "kernel/stddef.h"

void brights_kmalloc_init(void);
void *brights_kmalloc(size_t size);
void brights_kfree(void *ptr);
size_t brights_kmalloc_used(void);
size_t brights_kmalloc_capacity(void);

#endif
