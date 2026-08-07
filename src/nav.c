#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <stdbool.h>

#include "screen_ui.h"
#include "time.h"
#include "nav.h"

// Button values based on input key scancodes.
enum timechime_nav_buttons {
	TIMECHIME_NAV_BUTTON_0 = 11,
	TIMECHIME_NAV_BUTTON_1 = 2,
	TIMECHIME_NAV_BUTTON_2 = 3,
	TIMECHIME_NAV_BUTTON_3 = 4,
};

typedef enum {
	TIMECHIME_NAV_STATE_SHOW_TIME,
	TIMECHIME_NAV_STATE_ALARM_LIST,
	TIMECHIME_NAV_STATE_ALARM_EDIT,
	NUM_TIMECHIME_NAV_STATES
} timechime_nav_state_t;

timechime_nav_state_t current_state = TIMECHIME_NAV_STATE_SHOW_TIME;
bool needs_screen_update_val = true;

static void button_input_cb(struct input_event *evt, void *user_data)
{
	if (evt->sync == 0) {
		return;
	}

	if (evt->value) {
		timechime_nav_update_state(evt->code);
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

void timechime_nav_update_state(uint16_t button)
{
	switch (current_state) {
	case TIMECHIME_NAV_STATE_SHOW_TIME:
		current_state = TIMECHIME_NAV_STATE_ALARM_LIST;
		break;
	default:
		current_state = TIMECHIME_NAV_STATE_SHOW_TIME;
		break;
	}

	needs_screen_update_val = true;
}

void timechime_nav_init()
{
	current_state = TIMECHIME_NAV_STATE_SHOW_TIME;
	needs_screen_update_val = true;
	timechime_screen_init();
}

void timechime_nav_update()
{
	switch (current_state) {
	case TIMECHIME_NAV_STATE_SHOW_TIME:
		nav_update_show_time();
		break;
	case TIMECHIME_NAV_STATE_ALARM_LIST:
		nav_update_alarm_list();
		break;
	case TIMECHIME_NAV_STATE_ALARM_EDIT:
		break;
	default:
		break;
	}
}

bool needs_screen_update()
{
	if (needs_screen_update_val) {
		needs_screen_update_val = false;
		return true;
	}

	return false;
}

void nav_update_show_time()
{
	if (timechime_time_updated() || needs_screen_update()) {
		timechime_screen_ui_clear();
		timechime_screen_draw_current_time(timechime_time_get_current_hour(),
						   timechime_time_get_current_minute());
		timechime_screen_wait();
	}
}

void nav_update_alarm_list()
{
	if (needs_screen_update()) {
		timechime_screen_ui_clear();
		timechime_screen_draw_button_indicator_set(
			(timechime_sprite_t[]){TIMECHIME_SPRITE_GEAR, TIMECHIME_SPRITE_ARROW_UP,
					       TIMECHIME_SPRITE_ARROW_DOWN, TIMECHIME_SPRITE_PLUS});
		timechime_screen_wait();
	}
}
