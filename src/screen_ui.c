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

static const struct gpio_dt_spec epaper_cs = GPIO_DT_SPEC_GET(DT_NODELABEL(epaper_cs_gpio), gpios);

static volatile bool screen_busy;

bool timechime_screen_is_busy()
{
	return screen_busy;
}

bool timechime_screen_init()
{
	sprites[TIMECHIME_SPRITE_ARROW_UP] = &sprite_up;
	sprites[TIMECHIME_SPRITE_ARROW_DOWN] = &sprite_down;
	sprites[TIMECHIME_SPRITE_PLUS] = &sprite_plus;
	sprites[TIMECHIME_SPRITE_BELL] = &sprite_bell;
	sprites[TIMECHIME_SPRITE_SOUND_SELECT] = &sprite_sound_select;
	sprites[TIMECHIME_SPRITE_TOGGLE_ON] = &sprite_toggle_on;
	sprites[TIMECHIME_SPRITE_TOGGLE_OFF] = &sprite_toggle_off;
	sprites[TIMECHIME_SPRITE_GEAR] = &sprite_gear;
	sprites[TIMECHIME_SPRITE_TIME] = &sprite_time;
	sprites[TIMECHIME_SPRITE_TRASH] = &sprite_trash;
	sprites[TIMECHIME_SPRITE_CONFIRM] = &sprite_confirm;
	sprites[TIMECHIME_SPRITE_GPSSEARCH] = &sprite_gpssearch;

	gpio_pin_configure_dt(&epaper_cs, GPIO_OUTPUT_INACTIVE);

	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		return false;
	}

	return true;
}

void timechime_screen_ui_clear()
{
	lv_obj_t *base_layer = lv_screen_active();
	lv_obj_clean(base_layer);
	lv_obj_set_style_bg_color(base_layer, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(base_layer, LV_OPA_COVER, 0);
}

void timechime_screen_draw_current_time(uint8_t hour, uint8_t minute, bool show_am_pm, bool is_pm)
{
	lv_obj_t *base_layer = lv_screen_active();

	lv_obj_t *row = lv_obj_create(base_layer);
	lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(row, 0, 0);
	lv_obj_set_style_pad_all(row, 0, 0);
	lv_obj_set_style_pad_column(row, 4, 0);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_align(row, LV_ALIGN_CENTER);

	char time_str[6];

	if (show_am_pm) {
		snprintf(time_str, sizeof(time_str), "%u:%02u", hour, minute);
	} else {
		snprintf(time_str, sizeof(time_str), "%02u:%02u", hour, minute);
	}

	lv_obj_t *time_label = lv_label_create(row);
	lv_label_set_text(time_label, time_str);
	lv_obj_set_style_text_font(time_label, &font_inter_large, 0);

	if (show_am_pm) {
		lv_obj_t *am_pm_label = lv_label_create(row);
		lv_label_set_text(am_pm_label, is_pm ? "PM" : "AM");
		lv_obj_set_style_text_font(am_pm_label, &font_inter, 0);
	}
}

void timechime_screen_draw_gps_search()
{
	lv_obj_t *base_layer = lv_screen_active();

	int32_t img_w = sprites[TIMECHIME_SPRITE_GPSSEARCH]->header.w;
	int32_t img_h = sprites[TIMECHIME_SPRITE_GPSSEARCH]->header.h;

	lv_obj_t *img = lv_image_create(lv_screen_active());
	lv_image_set_src(img, sprites[TIMECHIME_SPRITE_GPSSEARCH]);
	lv_obj_set_pos(img, (lv_display_get_horizontal_resolution(NULL) - img_w) / 2,
		       (lv_display_get_vertical_resolution(NULL) - img_h) / 2);
}

void timechime_screen_draw_button_indicator_outline()
{
	lv_coord_t screen_w = lv_display_get_horizontal_resolution(NULL);
	lv_coord_t screen_h = lv_display_get_vertical_resolution(NULL);
	lv_obj_t *base_layer = lv_screen_active();

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
	timechime_screen_draw_button_indicator_outline();
	for (int i = 0; i < 4; i++) {
		timechime_screen_draw_button_indicator(i, sprites[i]);
	}
}

void timechime_screen_draw_alarm(uint8_t row, timechime_alarm_t *alarm, bool selected)
{
	if (row >= TIMECHIME_SCREEN_UI_MAX_ALARMS || alarm == NULL) {
		return;
	}

	// Create layer for this alarm.
	lv_coord_t screen_w = lv_display_get_horizontal_resolution(NULL);
	lv_coord_t screen_h = lv_display_get_vertical_resolution(NULL);
	lv_coord_t row_h = (screen_h * 3 / 4) / TIMECHIME_SCREEN_UI_MAX_ALARMS;
	lv_coord_t row_w = screen_w - 4;

	lv_obj_t *base_layer = lv_obj_create(lv_screen_active());
	lv_obj_set_size(base_layer, row_w, row_h);
	lv_obj_set_pos(base_layer, 2, row * row_h);
	lv_obj_set_style_border_width(base_layer, 0, 0);
	lv_obj_set_style_pad_ver(base_layer, 0, 0);
	lv_obj_set_style_pad_hor(base_layer, 8, 0);

	// Invert if selected.
	if (selected) {
		lv_obj_set_style_bg_color(base_layer, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(base_layer, LV_OPA_COVER, 0);
		lv_obj_set_style_text_color(base_layer, lv_color_white(), 0);
	} else {
		lv_obj_set_style_bg_opa(base_layer, LV_OPA_TRANSP, 0);
	}

	char time_str[6];
	snprintf(time_str, sizeof(time_str), "%02u:%02u", alarm->hour, alarm->minute);

	lv_obj_t *time_label = lv_label_create(base_layer);
	lv_label_set_text(time_label, time_str);
	lv_obj_set_align(time_label, LV_ALIGN_LEFT_MID);
	lv_obj_set_style_text_font(time_label, &font_inter, 0);

	char sound_id_str[4];
	snprintf(sound_id_str, sizeof(sound_id_str), "%u", alarm->sound_id);

	lv_obj_t *sound_id_label = lv_label_create(base_layer);
	lv_label_set_text(sound_id_label, sound_id_str);
	lv_obj_set_align(sound_id_label, LV_ALIGN_CENTER);
	lv_obj_set_style_text_font(sound_id_label, &font_inter, 0);

	char enabled_str[4];
	snprintf(enabled_str, sizeof(enabled_str), "%s", alarm->enabled ? "ON" : "OFF");

	lv_obj_t *enabled_label = lv_label_create(base_layer);
	lv_label_set_text(enabled_label, enabled_str);
	lv_obj_set_align(enabled_label, LV_ALIGN_RIGHT_MID);
	lv_obj_set_style_text_font(enabled_label, &font_inter, 0);
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
		screen_busy = true;
		gpio_pin_set_dt(&epaper_cs, 1);
		lv_timer_handler();
		gpio_pin_set_dt(&epaper_cs, 0);
		screen_busy = false;
		k_sleep(K_MSEC(10));
	}
}
