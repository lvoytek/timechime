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

	lv_obj_t *base_layer = lv_screen_active();

	lv_coord_t screen_w = lv_display_get_horizontal_resolution(NULL);
	lv_coord_t screen_h = lv_display_get_vertical_resolution(NULL);

	// Horizontal line 3/4 of the way down
	lv_obj_t *h_line = lv_obj_create(base_layer);
	lv_obj_set_style_bg_color(h_line, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(h_line, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(h_line, 0, 0);
	lv_obj_set_size(h_line, screen_w, 2);
	lv_obj_set_pos(h_line, 0, screen_h * 3 / 4);

	// Three vertical lines 1/4 apart, connecting horizontal line to bottom
	lv_coord_t v_line_top = screen_h * 3 / 4;
	lv_coord_t v_line_height = screen_h - v_line_top;
	for (int i = 1; i <= 3; i++) {
		lv_obj_t *v_line = lv_obj_create(base_layer);
		lv_obj_set_style_bg_color(v_line, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(v_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(v_line, 0, 0);
		lv_obj_set_size(v_line, 2, v_line_height);
		lv_obj_set_pos(v_line, screen_w * i / 4, v_line_top);
	}

	timechime_screen_wait();
}

void timechime_screen_wait()
{
	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
}
