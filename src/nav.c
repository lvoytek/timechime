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
	TIMECHIME_NAV_STATE_NEXT_ALARM,
	TIMECHIME_NAV_STATE_ALARM_LIST,
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
		nav_state_update_show_time(button);
		break;
	case TIMECHIME_NAV_STATE_NEXT_ALARM:
		nav_state_update_next_alarm(button);
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

// Show time screen button mapping.
static enum timechime_nav_show_time_buttons {
	NAV_BUTTON_SHOW_TIME_ALARM_LIST = NAV_BUTTON_0,
	NAV_BUTTON_SHOW_TIME_NEXT_ALARM = NAV_BUTTON_3,
};

// State update in show time screen.
void nav_state_update_show_time(uint16_t button)
{
	switch (button) {
	case NAV_BUTTON_SHOW_TIME_ALARM_LIST:
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_ALARM_LIST);
		break;
	case NAV_BUTTON_SHOW_TIME_NEXT_ALARM:
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_NEXT_ALARM);
		break;
	default:
		break;
	}
}

// Next alarm screen button mapping.
static enum timechime_nav_next_alarm_buttons {
	NAV_BUTTON_NEXT_ALARM_ALARM_LIST = NAV_BUTTON_0,
	NAV_BUTTON_NEXT_ALARM_SHOW_TIME = NAV_BUTTON_3,
};

// State update in next alarm screen.
void nav_state_update_next_alarm(uint16_t button)
{
	switch (button) {
	case NAV_BUTTON_NEXT_ALARM_ALARM_LIST:
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_ALARM_LIST);
		break;
	case NAV_BUTTON_NEXT_ALARM_SHOW_TIME:
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_SHOW_TIME);
		break;
	default:
		break;
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
		timechime_alarm_toggle_enabled(selected_alarm_index);
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
	case TIMECHIME_NAV_STATE_NEXT_ALARM:
		nav_update_next_alarm();
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

		timechime_screen_draw_button_indicator_set(
			(timechime_sprite_t[]){TIMECHIME_SPRITE_GEAR, TIMECHIME_SPRITE_NONE,
					       TIMECHIME_SPRITE_NONE, TIMECHIME_SPRITE_BELL});

		timechime_screen_wait();
	} else if (timechime_time_updated() || needs_screen_update()) {
		initial_time_update_done = true;

		timechime_screen_ui_clear();
		timechime_screen_draw_current_time(
			timechime_time_get_current_hour(), timechime_time_get_current_minute(),
			timechime_time_using_12hr_format(), timechime_time_current_time_is_pm());

		timechime_screen_draw_button_indicator_set(
			(timechime_sprite_t[]){TIMECHIME_SPRITE_GEAR, TIMECHIME_SPRITE_NONE,
					       TIMECHIME_SPRITE_NONE, TIMECHIME_SPRITE_BELL});

		timechime_screen_wait();
		timechime_alarm_check_and_queue();
	}
}

// Repeated next alarm screen update.
static uint8_t last_next_alarm_index = TIMECHIME_MAX_ALARMS;

void nav_update_next_alarm()
{
	if (!initial_time_update_done && needs_screen_update()) {
		timechime_screen_ui_clear();
		timechime_screen_draw_gps_search();

		timechime_screen_draw_button_indicator_set(
			(timechime_sprite_t[]){TIMECHIME_SPRITE_GEAR, TIMECHIME_SPRITE_NONE,
					       TIMECHIME_SPRITE_NONE, TIMECHIME_SPRITE_TIME});

		timechime_screen_wait();
	}

	bool next_alarm_updated = false;
	bool needs_update = needs_screen_update();

	if (needs_update || timechime_time_updated()) {
		initial_time_update_done = true;

		uint8_t next_alarm_index = timechime_alarm_get_next(
			timechime_time_get_current_hour(), timechime_time_get_current_minute());

		if (last_next_alarm_index != next_alarm_index) {
			next_alarm_updated = true;
			last_next_alarm_index = next_alarm_index;
		}

		timechime_alarm_check_and_queue();
	}

	// Do not refresh screen unless update needed or next alarm changed.
	if (needs_update || next_alarm_updated) {
		timechime_alarm_t *next_alarm;
		if (timechime_alarm_get(last_next_alarm_index, &next_alarm)) {
			timechime_screen_ui_clear();

			if (timechime_time_using_12hr_format()) {
				uint8_t hour = timechime_time_convert_to_12_hour(next_alarm->hour);
				bool is_pm = next_alarm->hour >= 12;

				timechime_screen_draw_next_alarm(hour, next_alarm->minute, true,
								 is_pm);
			} else {
				timechime_screen_draw_next_alarm(next_alarm->hour,
								 next_alarm->minute, false, false);
			}

			timechime_screen_draw_button_indicator_set((timechime_sprite_t[]){
				TIMECHIME_SPRITE_GEAR, TIMECHIME_SPRITE_NONE, TIMECHIME_SPRITE_NONE,
				TIMECHIME_SPRITE_TIME});
			timechime_screen_wait();
		}
	}
}

// Repeated alarm list screen update.
void nav_update_alarm_list()
{
	uint32_t current_time = k_uptime_get_32();
	// Go back to default screen after 1 minute of inactivity.
	if (current_time - last_state_change_time > 60000) {
		timechime_nav_go_to_state(TIMECHIME_NAV_STATE_SHOW_TIME);
	}

	// Refresh screen after inputs + 1 second of inactivity.
	else if (current_time - last_state_change_time > 1000 && needs_screen_update()) {
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
