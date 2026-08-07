#ifndef TIMECHIME_NAV_H
#define TIMECHIME_NAV_H

#include <stdint.h>

// Initialize navigation state machine and screen.
void timechime_nav_init();

// Update the state machine based on a button press.
void timechime_nav_update_state(uint16_t button);

// Update screen based on the current state.
void timechime_nav_update();

#endif
