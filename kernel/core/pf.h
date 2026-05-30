#ifndef BRIGHTS_PF_H
#define BRIGHTS_PF_H
#include <stdint.h>
#define PF_ERR_PRESENT   (1ULL << 0)
#define PF_ERR_WRITE     (1ULL << 1)
#define PF_ERR_USER      (1ULL << 2)
#define PF_ERR_RSVD      (1ULL << 3)
#define PF_ERR_INST      (1ULL << 4)
int brights_page_fault_handler(uint64_t fault_addr, uint64_t error_code);
#endif
