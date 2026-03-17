#include "tss.h"

static brights_tss_t tss;

void brights_tss_init(uint64_t rsp0)
{
  tss.rsp0 = rsp0;
  tss.io_map_base = sizeof(brights_tss_t);
}

brights_tss_t *brights_tss_ptr(void)
{
  return &tss;
}
