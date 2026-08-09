#ifndef TIMECHIME_SETTINGS_H
#define TIMECHIME_SETTINGS_H

#include <stdint.h>

#include "alarm.h"

// Load alarms from storage to provided array and return number loaded.
uint8_t timechime_settings_load_alarms(timechime_alarm_t *alarms, uint8_t max_alarms);

// Save alarms to storage from provided array.
void timechime_settings_save_alarms(timechime_alarm_t *alarms, uint8_t alarm_count);

// Load the timezone offset from storage.
void timechime_settings_load_timezone_offset(int8_t *hours, int8_t *minutes);

// Save the timezone offset to storage.
void timechime_settings_save_timezone_offset(int8_t hours, int8_t minutes);

#endif
