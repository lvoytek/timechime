#include "alarm.h"

#define TIMECHIME_MAX_ALARMS 25
static timechime_alarm_t alarms[TIMECHIME_MAX_ALARMS];
static uint8_t num_alarms = 0;

bool timechime_alarm_get(uint8_t index, timechime_alarm_t **alarm)
{
	if (index >= num_alarms) {
		return false;
	}

	*alarm = &alarms[index];
	return true;
}

bool timechime_alarm_new(uint8_t hour, uint8_t minute, uint8_t sound_id, bool enabled)
{
	if (num_alarms >= TIMECHIME_MAX_ALARMS) {
		return false;
	}

	alarms[num_alarms].hour = hour;
	alarms[num_alarms].minute = minute;
	alarms[num_alarms].sound_id = sound_id;
	alarms[num_alarms].enabled = enabled;

	num_alarms++;
	return true;
}

bool timechime_alarm_delete(uint8_t index)
{
	if (index >= num_alarms) {
		return false;
	}

	for (uint8_t i = index; i < num_alarms - 1; i++) {
		alarms[i] = alarms[i + 1];
	}

	num_alarms--;
	return true;
}
