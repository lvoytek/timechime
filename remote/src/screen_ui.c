#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <zephyr/kernel.h>

#include "screen_ui.h"

// MIPI DBI SPI config sets cs_is_gpio=false (no cs-gpios on the MIPI DBI node),
// so the SAM0 SPI driver never asserts CS.
static const struct gpio_dt_spec epaper_cs =
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(arduino_spi), cs_gpios, 0);

bool timechime_screen_init()
{
	gpio_pin_configure_dt(&epaper_cs, GPIO_OUTPUT_ACTIVE);

	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		return false;
	}

	return true;
}

void timechime_screen_ui_clear()
{
	// Set white background
	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

	timechime_screen_wait();
}

void timechime_screen_wait()
{
	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
}
