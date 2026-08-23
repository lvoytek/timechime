#include "alarm.h"
#include "settings.h"
#include "sound.h"
#include "time.h"

static timechime_alarm_t alarms[TIMECHIME_MAX_ALARMS];
static uint8_t num_alarms = 0;

static bool alarms_need_save = false;

void timechime_alarm_init()
{
	num_alarms = timechime_settings_load_alarms(alarms, TIMECHIME_MAX_ALARMS);
}

void timechime_alarm_update()
{
	if (alarms_need_save) {
		timechime_settings_save_alarms(alarms, num_alarms);
		alarms_need_save = false;
	}
}

bool timechime_alarm_get(uint8_t index, timechime_alarm_t **alarm)
{
	if (index >= num_alarms) {
		return false;
	}

	*alarm = &alarms[index];
	return true;
}

uint8_t timechime_alarm_get_count()
{
	return num_alarms;
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

	alarms_need_save = true;
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

	alarms_need_save = true;
	return true;
}

void timechime_alarm_check_and_queue()
{
	uint8_t current_hour = timechime_time_get_current_hour();
	uint8_t current_minute = timechime_time_get_current_minute();

	for (uint8_t i = 0; i < num_alarms; i++) {
		timechime_alarm_t *alarm = &alarms[i];

		if (alarm->enabled && alarm->hour == current_hour &&
		    alarm->minute == current_minute) {
			char sound_file[65];
			timechime_queue_sound_play(alarm->sound_id);
			break;
		}
	}
}
