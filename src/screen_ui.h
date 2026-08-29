#ifndef TIMECHIME_SCREEN_UI_H
#define TIMECHIME_SCREEN_UI_H

#include <stdbool.h>
#include <stdint.h>

#include "alarm.h"

#define TIMECHIME_SCREEN_UI_MAX_ALARMS 3

// The set of available sprites to display.
typedef enum {
	TIMECHIME_SPRITE_ARROW_UP,
	TIMECHIME_SPRITE_ARROW_DOWN,
	TIMECHIME_SPRITE_PLUS,
	TIMECHIME_SPRITE_BELL,
	TIMECHIME_SPRITE_GEAR,
	TIMECHIME_SPRITE_TRASH,
	TIMECHIME_SPRITE_CONFIRM,
	TIMECHIME_SPRITE_GPSSEARCH,
	NUM_TIMECHIME_SPRITES
} timechime_sprite_t;

// Initialize screen hardware and LVGL and confirm device is available.
bool timechime_screen_init();

// Wait for display to update.
void timechime_screen_wait();

// Clear the screen and add base UI lines.
void timechime_screen_ui_clear();

// Draw the large current time
void timechime_screen_draw_current_time(uint8_t hour, uint8_t minute, bool show_am_pm, bool is_pm);

// Draw search for GPS screen
void timechime_screen_draw_gps_search();

// Draw sprite for a specified button indicator.
void timechime_screen_draw_button_indicator(int button, timechime_sprite_t sprite);

// Draw a specific set of button indicators.
void timechime_screen_draw_button_indicator_set(timechime_sprite_t sprites[4]);

// Draw alarm data in a specific row.
void timechime_screen_draw_alarm(uint8_t row, timechime_alarm_t *alarm, bool selected);

// Returns true while the screen is actively using the SPI bus.
bool timechime_screen_is_busy();

#endif
