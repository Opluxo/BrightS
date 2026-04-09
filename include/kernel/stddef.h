#ifndef BRIGHTS_STDDEF_H
#define BRIGHTS_STDDEF_H

/*
 * BrightS Kernel stddef.h - Basic type definitions and macros
 * Kernel-integrated implementation without external library dependencies
 */

/* Basic types with kernel-specific definitions for LP64/x86_64 */

/* size_t - type for object sizes */
typedef unsigned long long size_t;

/* ptrdiff_t - type for pointer differences */
typedef long long ptrdiff_t;

/* wchar_t - wide character type */
typedef unsigned short wchar_t;

/* max_align_t - type with maximum alignment requirement */
typedef union {
    long long _ll;
    long double _ld;
    void *_p;
} max_align_t;

/*
 * NULL macro - null pointer constant
 * Defined as 0 for kernel environment compatibility
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/*
 * offsetof - offset of a member within a struct
 * Kernel-integrated implementation using GCC built-ins when available
 */
#ifdef __GNUC__
#define offsetof(type, member) __builtin_offsetof(type, member)
#else
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif

/*
 * Kernel-specific extensions for memory alignment
 */

/* BRIGHTS_ALIGNOF - get alignment requirement of a type */
#ifdef __GNUC__
#define BRIGHTS_ALIGNOF(type) __alignof__(type)
#else
#define BRIGHTS_ALIGNOF(type) ((size_t)&(((struct { char c; type t; } *)0)->t))
#endif

/* BRIGHTS_ALIGNED - declare aligned variable */
#define BRIGHTS_ALIGNED(alignment) __attribute__((aligned(alignment)))

/* BRIGHTS_PACKED - declare packed structure */
#define BRIGHTS_PACKED __attribute__((packed))

/*
 * Kernel memory utilities
 */

/* BRIGHTS_CONTAINER_OF - get containing structure from member pointer */
#define BRIGHTS_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* BRIGHTS_ARRAY_SIZE - get number of elements in array */
#define BRIGHTS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * Architecture-specific type aliases for better code readability
 */
typedef unsigned long br_uintptr_t;  /* unsigned integer same size as pointer */
typedef long br_intptr_t;            /* signed integer same size as pointer */
typedef unsigned long long br_phys_addr_t; /* physical address type */

/*
 * Kernel-specific NULL checks
 */
#define BRIGHTS_IS_NULL(ptr) ((ptr) == NULL)
#define BRIGHTS_IS_NOT_NULL(ptr) ((ptr) != NULL)

/*
 * Size validation macros
 */
#define BRIGHTS_SIZE_MAX ((size_t)-1)
#define BRIGHTS_SIZE_MIN ((size_t)0)

/*
 * Kernel-specific size calculations
 */
#define BRIGHTS_SIZEOF_ALIGNED(type) \
    ((sizeof(type) + BRIGHTS_ALIGNOF(type) - 1) & ~(BRIGHTS_ALIGNOF(type) - 1))

#ifndef BRIGHTS_PAGE_SIZE
#define BRIGHTS_PAGE_SIZE 4096UL
#endif
#define BRIGHTS_PAGE_MASK (BRIGHTS_PAGE_SIZE - 1)

#define BRIGHTS_PAGE_ALIGN(addr) \
    (((br_uintptr_t)(addr) + BRIGHTS_PAGE_MASK) & ~BRIGHTS_PAGE_MASK)

#define BRIGHTS_IS_PAGE_ALIGNED(addr) \
    (((br_uintptr_t)(addr) & BRIGHTS_PAGE_MASK) == 0)

/*
 * Kernel string/array bounds checking
 */
#define BRIGHTS_BOUNDS_CHECK(idx, max) \
    ((size_t)(idx) < (size_t)(max))

#endif /* BRIGHTS_STDDEF_H */