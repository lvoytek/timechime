#ifndef TIMECHIME_SCREEN_UI_H
#define TIMECHIME_SCREEN_UI_H

#include <stdbool.h>

// The set of available sprites to display.
typedef enum {
	TIMECHIME_SPRITE_ARROW_UP,
	TIMECHIME_SPRITE_ARROW_DOWN,
	TIMECHIME_SPRITE_PLUS,
	TIMECHIME_SPRITE_BELL,
	TIMECHIME_SPRITE_GEAR,
	NUM_TIMECHIME_SPRITES
} timechime_sprite_t;

// Initialize screen hardware and LVGL and confirm device is available.
bool timechime_screen_init();

// Wait for display to update.
void timechime_screen_wait();

// Clear the screen and add base UI lines.
void timechime_screen_ui_clear();

// Draw sprite for a specified button indicator.
void timechime_screen_draw_button_indicator(int button, timechime_sprite_t sprite);

#endif
