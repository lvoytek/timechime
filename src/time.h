#ifndef TIMECHIME_TIME_H
#define TIMECHIME_TIME_H

#include <stdbool.h>
#include <stdint.h>

uint8_t timechime_time_get_current_hour();
uint8_t timechime_time_get_current_minute();
bool timechime_time_updated();
void timechime_time_load_timezone_offset();
void timechime_time_get_timezone_offset(int8_t *hours, int8_t *minutes);
void timechime_time_set_timezone_offset(int8_t hours, int8_t minutes);

uint8_t timechime_time_convert_to_12_hour(uint8_t hour);
bool timechime_time_is_pm(uint8_t hour);

#endif
