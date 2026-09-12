#include <zephyr/drivers/adc.h>
#include <zephyr/sys/util.h>

#include "pot.h"

static const struct adc_dt_spec pot_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
static bool pot_ready = false;

void timechime_pot_init()
{
	if (!adc_is_ready_dt(&pot_channel)) {
		return;
	}

	if (adc_channel_setup_dt(&pot_channel) != 0) {
		return;
	}

	pot_ready = true;
}

uint8_t timechime_pot_get_value()
{
	uint16_t sample = 0;
	struct adc_sequence sequence = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
	};

	if (!pot_ready) {
		return 0;
	}

	if (adc_sequence_init_dt(&pot_channel, &sequence) != 0) {
		return 0;
	}

	if (adc_read_dt(&pot_channel, &sequence) != 0) {
		return 0;
	}

	uint16_t max = BIT(pot_channel.resolution) - 1;

	if (sample > max) {
		sample = max;
	}

	return (uint8_t)(((uint32_t)sample * UINT8_MAX) / max);
}
