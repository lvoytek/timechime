#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>

#include "screen_ui.h"

#include "nav.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static void button_input_cb(struct input_event *evt, void *user_data)
{
	if (evt->sync == 0) {
		return;
	}

	gpio_pin_toggle_dt(&led);

	/*if (evt->value) {
		timechime_screen_ui_clear();
		timechime_screen_draw_current_time(0, evt->code);
	}*/
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

void timechime_nav_begin()
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}
