#ifndef TIMECHIME_SOUND_H
#define TIMECHIME_SOUND_H

#include <stdbool.h>
#include <stdint.h>

// Initialize sound system and confirm device is available.
bool timechime_sound_init();

// Set volume on next loop update.
void timechime_sound_queue_set_volume(uint8_t left, uint8_t right);

// Queue a sound file to play on next loop update.
void timechime_queue_sound_play(uint8_t sound_file_index);

// Sound loop update.
void timechime_sound_update();

#endif
