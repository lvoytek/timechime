#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>

#include "time.h"

static volatile uint8_t current_hour = 0;
static volatile uint8_t current_minute = 0;
static volatile bool time_updated = false;

#define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		uint8_t hour = data->utc.hour;
		uint8_t minute = data->utc.minute;
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
