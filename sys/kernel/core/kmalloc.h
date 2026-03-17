#ifndef BRIGHTS_KMALLOC_H
#define BRIGHTS_KMALLOC_H

#include <stddef.h>

void brights_kmalloc_init(void);
void *brights_kmalloc(size_t size);
void brights_kfree(void *ptr);

#endif
