#ifndef _KERNEL_STDDEF_H
#define _KERNEL_STDDEF_H

#include <stdint.h>
#include <stddef.h>

#define NULL ((void*)0)
#define offsetof(type, member) __builtin_offsetof(type, member)

#endif
