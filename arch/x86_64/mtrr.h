#ifndef BRIGHTS_MTRR_H
#define BRIGHTS_MTRR_H

#include <stdint.h>

/* MTRR memory types */
#define MTRR_UC  0x00  /* Uncacheable */
#define MTRR_WC  0x01  /* Write Combining */
#define MTRR_WT  0x04  /* Write Through */
#define MTRR_WP  0x05  /* Write Protected */
#define MTRR_WB  0x06  /* Write Back */

/* Initialize MTRR subsystem */
void brights_mtrr_init(void);

/* Set MTRR for a physical range */
/* Returns MTRR index on success, -1 on failure */
int brights_mtrr_set(uint64_t phys_base, uint64_t size, uint8_t type);

/* Clear a specific MTRR */
void brights_mtrr_clear(int index);

/* Get number of MTRRs available */
int brights_mtrr_count(void);

/* Initialize PAT (Page Attribute Table) for custom page caching */
void brights_pat_init(void);

/* PAT indices (custom configuration) */
/* Index 0: WB (default) */
/* Index 1: WT */
/* Index 2: UC- */
/* Index 3: UC */
/* Index 4: WC (for framebuffer) */
/* Index 5: WP */
#define PAT_WB  0
#define PAT_WT  1
#define PAT_UC_MINUS 2
#define PAT_UC  3
#define PAT_WC  4
#define PAT_WP  5

#endif
