#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <zephyr/kernel.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* MIPI DBI SPI config sets cs_is_gpio=false (no cs-gpios on the MIPI DBI node),
 * so the SAM0 SPI driver never asserts CS.  Hold it low permanently; the bus
 * is dedicated to this display so there is no contention. */
static const struct gpio_dt_spec epaper_cs =
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(arduino_spi), cs_gpios, 0);

int main(void)
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&epaper_cs, GPIO_OUTPUT_ACTIVE);

	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		return 1;
	}

	/* White background */
	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

	/* Filled black 100×100 square centred on the display */
	lv_obj_t *square = lv_obj_create(lv_screen_active());
	lv_obj_set_size(square, 100, 100);
	lv_obj_align(square, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_radius(square, 0, 0);
	lv_obj_set_style_bg_color(square, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(square, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(square, 0, 0);

	/* lvgl_flush_cb_mono handles blanking_off after the last chunk. */
	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}

	return 0;
}
