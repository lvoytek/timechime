#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>

#include "settings.h"

#include "time.h"

static volatile uint8_t current_hour = 0;
static volatile uint8_t current_minute = 0;
static volatile bool time_updated = false;

static int8_t timezone_offset_hours = 0;
static int8_t timezone_offset_minutes = 0;

#define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		uint8_t hour = data->utc.hour;
		uint8_t minute = data->utc.minute;

		// Apply timezone offset.
		int8_t offset_minutes = minute + timezone_offset_minutes;
		int8_t offset_hours = hour + timezone_offset_hours;

		// Handle minute overflow and underflow.
		if (offset_minutes >= 60) {
			offset_minutes -= 60;
			offset_hours++;
		} else if (offset_minutes < 0) {
			offset_minutes += 60;
			offset_hours--;
		}

		// Handle hour overflow and underflow.
		if (offset_hours >= 24) {
			offset_hours -= 24;
		} else if (offset_hours < 0) {
			offset_hours += 24;
		}

		hour = (uint8_t)offset_hours;
		minute = (uint8_t)offset_minutes;

		if (hour != current_hour || minute != current_minute) {
			current_hour = hour;
			current_minute = minute;
			time_updated = true;
		}
	}
}

GNSS_DATA_CALLBACK_DEFINE(GNSS_MODEM, gnss_data_cb);

uint8_t timechime_time_get_current_hour()
{
	return current_hour;
}

uint8_t timechime_time_get_current_minute()
{
	return current_minute;
}

bool timechime_time_updated()
{
	if (!time_updated) {
		return false;
	}

	time_updated = false;
	return true;
}

void timechime_time_load_timezone_offset()
{
	timechime_settings_load_timezone_offset(&timezone_offset_hours, &timezone_offset_minutes);
}

void timechime_time_get_timezone_offset(int8_t *hours, int8_t *minutes)
{
	*hours = timezone_offset_hours;
	*minutes = timezone_offset_minutes;
}

void timechime_time_set_timezone_offset(int8_t hours, int8_t minutes)
{
	timezone_offset_hours = hours;
	timezone_offset_minutes = minutes;
	timechime_settings_save_timezone_offset(hours, minutes);
}
