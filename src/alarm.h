#ifndef TIMECHIME_ALARM_H
#define TIMECHIME_ALARM_H

#include <stdint.h>
#include <stdbool.h>

#define TIMECHIME_MAX_ALARMS 25

typedef struct {
	uint8_t hour;
	uint8_t minute;
	uint8_t sound_id;
	bool enabled;
} timechime_alarm_t;

// Initialize the alarm system and load alarms from storage.
void timechime_alarm_init();

// Update alarms called repeatedly.
void timechime_alarm_update();

// Get the alarm at the specified index by reference.
bool timechime_alarm_get(uint8_t index, timechime_alarm_t **alarm);

// Get the number of alarms currently stored.
uint8_t timechime_alarm_get_count();

// Create a new alarm if there is space.
bool timechime_alarm_new(uint8_t hour, uint8_t minute, uint8_t sound_id, bool enabled);

// Delete the alarm at the specified index.
bool timechime_alarm_delete(uint8_t index);

// Check if alarm should be played and queue if so.
void timechime_alarm_check_and_queue();

#endif
