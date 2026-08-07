#ifndef TIMECHIME_TIME_H
#define TIMECHIME_TIME_H

#include <stdbool.h>
#include <stdint.h>

uint8_t timechime_time_get_current_hour();
uint8_t timechime_time_get_current_minute();
bool timechime_time_updated();

#endif
