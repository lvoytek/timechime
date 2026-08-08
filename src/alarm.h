#ifndef TIMECHIME_ALARM_H
#define TIMECHIME_ALARM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t hour;
	uint8_t minute;
	uint8_t sound_id;
	bool enabled;
} timechime_alarm_t;

// Get the alarm at the specified index by reference.
bool timechime_alarm_get(uint8_t index, timechime_alarm_t **alarm);

// Create a new alarm if there is space.
bool timechime_alarm_new(uint8_t hour, uint8_t minute, uint8_t sound_id, bool enabled);

// Delete the alarm at the specified index.
bool timechime_alarm_delete(uint8_t index);

#endif
