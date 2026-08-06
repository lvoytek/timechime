#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <zephyr/kernel.h>

#include "sprites.h"

#include "screen_ui.h"

static const lv_image_dsc_t *sprites[NUM_TIMECHIME_SPRITES];

// Declare custom fonts
LV_FONT_DECLARE(font_inter);
LV_FONT_DECLARE(font_inter_large);

// MIPI DBI SPI config sets cs_is_gpio=false (no cs-gpios on the MIPI DBI node),
// so the SAM0 SPI driver never asserts CS.
static const struct gpio_dt_spec epaper_cs =
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(arduino_spi), cs_gpios, 0);

bool timechime_screen_init()
{
	sprites[TIMECHIME_SPRITE_ARROW_UP] = &sprite_up;
	sprites[TIMECHIME_SPRITE_ARROW_DOWN] = &sprite_down;
	sprites[TIMECHIME_SPRITE_PLUS] = &sprite_plus;
	sprites[TIMECHIME_SPRITE_BELL] = &sprite_bell;
	sprites[TIMECHIME_SPRITE_GEAR] = &sprite_gear;
	sprites[TIMECHIME_SPRITE_TRASH] = &sprite_trash;
	sprites[TIMECHIME_SPRITE_CONFIRM] = &sprite_confirm;

	gpio_pin_configure_dt(&epaper_cs, GPIO_OUTPUT_ACTIVE);

	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		return false;
	}

	return true;
}

void timechime_screen_ui_clear()
{
	lv_obj_t *base_layer = lv_screen_active();
	lv_obj_set_style_bg_color(base_layer, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(base_layer, LV_OPA_COVER, 0);

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
}

void timechime_screen_draw_current_time(uint8_t hour, uint8_t minute)
{
	lv_obj_t *base_layer = lv_screen_active();

	char time_str[6];
	snprintf(time_str, sizeof(time_str), "%02u:%02u", hour, minute);

	lv_obj_t *time_label = lv_label_create(base_layer);
	lv_label_set_text(time_label, time_str);
	lv_obj_set_align(time_label, LV_ALIGN_CENTER);
	lv_obj_set_style_text_font(time_label, &font_inter_large, 0);
}

void timechime_screen_draw_button_indicator(int button, timechime_sprite_t sprite)
{
	if (button < 0 || button >= 4 || (unsigned int)sprite >= NUM_TIMECHIME_SPRITES) {
		return;
	}

	lv_coord_t screen_w = lv_display_get_horizontal_resolution(NULL);
	lv_coord_t screen_h = lv_display_get_vertical_resolution(NULL);
	lv_coord_t section_w = screen_w / 4;
	lv_coord_t section_h = screen_h / 4;

	int32_t img_w = sprites[sprite]->header.w;
	int32_t img_h = sprites[sprite]->header.h;

	lv_obj_t *img = lv_image_create(lv_screen_active());
	lv_image_set_src(img, sprites[sprite]);
	lv_obj_set_pos(img, button * section_w + (section_w - img_w) / 2,
		       screen_h * 3 / 4 + (section_h - img_h) / 2);
}

void timechime_screen_draw_button_indicator_set(timechime_sprite_t sprites[4])
{
	for (int i = 0; i < 4; i++) {
		timechime_screen_draw_button_indicator(i, sprites[i]);
	}
}

static bool screen_refresh_done;

static void on_refr_finish(lv_event_t *e)
{
	screen_refresh_done = true;
}

void timechime_screen_wait(void)
{
	screen_refresh_done = false;
	lv_display_add_event_cb(lv_display_get_default(), on_refr_finish, LV_EVENT_FLUSH_FINISH,
				NULL);

	while (!screen_refresh_done) {
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
}
