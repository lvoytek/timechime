#ifndef TIMECHIME_SCREEN_UI_H
#define TIMECHIME_SCREEN_UI_H

#include <stdbool.h>

// The set of available sprites to display.
typedef enum {
	TIMECHIME_SPRITE_ARROW_UP,
	TIMECHIME_SPRITE_ARROW_DOWN,
	NUM_TIMECHIME_SPRITES
} timechime_sprite_t;

// Initialize screen hardware and LVGL and confirm device is available.
bool timechime_screen_init();

// Wait for display to update.
void timechime_screen_wait();

// Clear the screen and add base UI lines.
void timechime_screen_ui_clear();

#endif
