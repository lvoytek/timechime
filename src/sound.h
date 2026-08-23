#ifndef TIMECHIME_SOUND_H
#define TIMECHIME_SOUND_H

// Set volume on next loop update.
void timechime_sound_queue_set_volume(uint8_t left, uint8_t right);

// Queue a sound file to play on next loop update.
void timechime_queue_sound_play(const char *sound_file);

// Sound loop update.
void timechime_sound_update();

#endif
