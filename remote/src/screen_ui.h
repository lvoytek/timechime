#ifndef TIMECHIME_SCREEN_UI_H
#define TIMECHIME_SCREEN_UI_H

#include <stdbool.h>

// Initialize screen hardware and LVGL and confirm device is available.
bool timechime_screen_init();

// Wait for display to update.
void timechime_screen_wait();

// Clear the screen and add base UI lines.
void timechime_screen_ui_clear();

#endif
