#ifndef BSP_RTC_H
#define BSP_RTC_H

#include <stdbool.h>
#include <stdint.h>

void bsp_rtc_init(void);
bool bsp_rtc_get_minutes_of_day(uint16_t *minutes_of_day);
void bsp_rtc_mock_set_minutes_of_day(uint16_t minutes_of_day);
void bsp_rtc_mock_set_valid(bool valid);

#endif
