#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <stdbool.h>

#include "alarm.h"
#include "screen_ui.h"
#include "time.h"
#include "nav.h"

// Button values based on input key scancodes.
enum timechime_nav_buttons {
	NAV_BUTTON_0 = 11,
	NAV_BUTTON_1 = 2,
	NAV_BUTTON_2 = 3,
	NAV_BUTTON_3 = 4,
};

typedef enum {
	TIMECHIME_NAV_STATE_SHOW_TIME,
	TIMECHIME_NAV_STATE_ALARM_LIST,
	TIMECHIME_NAV_STATE_ALARM_EDIT,
	NUM_TIMECHIME_NAV_STATES
} timechime_nav_state_t;

static timechime_nav_state_t current_state = TIMECHIME_NAV_STATE_SHOW_TIME;
static bool needs_screen_update_val = true;

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

void timechime_nav_init()
{
	timechime_nav_go_to_state(TIMECHIME_NAV_STATE_SHOW_TIME);
	timechime_screen_init();
}

// ----- Nav state machine -----

uint32_t last_state_change_time = 0;

void timechime_nav_update_state(uint16_t button)
{
	last_state_change_time = k_uptime_get_32();

	switch (current_state) {
	case TIMECHIME_NAV_STATE_SHOW_TIME:
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_ALARM_LIST);
		break;
	case TIMECHIME_NAV_STATE_ALARM_LIST:
		nav_state_update_alarm_list(button);
		break;
	default:
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_SHOW_TIME);
		break;
	}
}

// Initialize a new nav state.
void timechime_nav_go_to_state(timechime_nav_state_t state)
{
	if (state < NUM_TIMECHIME_NAV_STATES) {
		current_state = state;
		needs_screen_update_val = true;
	}
}

// Alarm list screen button mapping.
static enum timechime_nav_alarm_list_buttons {
	NAV_BUTTON_ALARM_LIST_UP = NAV_BUTTON_0,
	NAV_BUTTON_ALARM_LIST_DOWN = NAV_BUTTON_1,
	NAV_BUTTON_ALARM_LIST_EDIT_SOUND = NAV_BUTTON_2,
	NAV_BUTTON_ALARM_LIST_TOGGLE = NAV_BUTTON_3,
};

static volatile uint8_t selected_alarm_index = 0;

// State update in alarm list screen.
void nav_state_update_alarm_list(uint16_t button)
{
	uint8_t alarm_count = timechime_alarm_get_count();

	// Ignore buttons when there are no alarms
	if (alarm_count == 0) {
		return;
	}

	switch (button) {
	case NAV_BUTTON_ALARM_LIST_EDIT_SOUND:
		break;
	case NAV_BUTTON_ALARM_LIST_UP:
		if (selected_alarm_index > 0) {
			selected_alarm_index--;
		} else {
			selected_alarm_index = alarm_count - 1;
		}
		break;
	case NAV_BUTTON_ALARM_LIST_DOWN:
		if (selected_alarm_index + 1 < alarm_count) {
			selected_alarm_index++;
		} else {
			selected_alarm_index = 0;
		}
		break;
	case NAV_BUTTON_ALARM_LIST_TOGGLE:
		break;
	default:
		break;
	}

	needs_screen_update_val = true;
}

// ----- Looped nav screen updates -----

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

// Check if screen needs update and reset flag.
static bool needs_screen_update()
{
	if (needs_screen_update_val) {
		needs_screen_update_val = false;
		return true;
	}

	return false;
}

// Repeated time screen update.
static bool initial_time_update_done = false;

void nav_update_show_time()
{
	if (!initial_time_update_done && needs_screen_update()) {
		timechime_screen_ui_clear();
		timechime_screen_draw_gps_search();
		timechime_screen_wait();
	} else if (timechime_time_updated() || needs_screen_update()) {
		initial_time_update_done = true;

		timechime_screen_ui_clear();
		timechime_screen_draw_current_time(
			timechime_time_get_current_hour(), timechime_time_get_current_minute(),
			timechime_time_using_12hr_format(), timechime_time_current_time_is_pm());

		timechime_screen_wait();
		timechime_alarm_check_and_queue();
	}
}

// Repeated alarm list screen update.
void nav_update_alarm_list()
{
	if (k_uptime_get_32() - last_state_change_time > 60000) {
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_SHOW_TIME);
	} else if (needs_screen_update()) {
		timechime_screen_ui_clear();

		bool selected_alarm_enabled = false;
		uint8_t alarm_count = timechime_alarm_get_count();
		uint8_t alarm_index_mod = selected_alarm_index % TIMECHIME_SCREEN_UI_MAX_ALARMS;
		uint8_t start_index = selected_alarm_index - alarm_index_mod;
		for (uint8_t i = 0; i < TIMECHIME_SCREEN_UI_MAX_ALARMS; i++) {
			uint8_t alarm_index = start_index + i;
			if (alarm_index < alarm_count) {
				timechime_alarm_t *alarm;
				if (timechime_alarm_get(alarm_index, &alarm)) {
					timechime_screen_draw_alarm(
						i, alarm, alarm_index == selected_alarm_index);

					if (alarm_index == selected_alarm_index) {
						selected_alarm_enabled = alarm->enabled;
					}
				}
			}
		}

		timechime_screen_draw_button_indicator_set((timechime_sprite_t[]){
			TIMECHIME_SPRITE_ARROW_UP, TIMECHIME_SPRITE_ARROW_DOWN,
			TIMECHIME_SPRITE_SOUND_SELECT,
			selected_alarm_enabled ? TIMECHIME_SPRITE_TOGGLE_ON
					       : TIMECHIME_SPRITE_TOGGLE_OFF});

		timechime_screen_wait();
	}
}
