#ifndef BRIGHTS_RTC_H
#define BRIGHTS_RTC_H

#include <stdint.h>

typedef struct {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} brights_rtc_time_t;

int brights_rtc_read(brights_rtc_time_t *t);

#endif
