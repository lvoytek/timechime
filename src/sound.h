#ifndef TIMECHIME_SOUND_H
#define TIMECHIME_SOUND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Limit filename length for FAT16
#define TIMECHIME_SOUND_NAME_MAX_LEN 12

// Initialize sound system and confirm device is available.
bool timechime_sound_init();

// Set volume on next loop update.
void timechime_sound_queue_set_volume(uint8_t left, uint8_t right);

// Queue a sound file to play on next loop update.
void timechime_queue_sound_play(uint8_t sound_file_index);

// Sound loop update.
void timechime_sound_update();

// Add a new sound file to the system.
void timechime_sound_add(const char *file_path);

// Remove a sound file from the system.
void timechime_sound_remove(uint8_t sound_file_index);

// Number of sound files currently registered.
uint8_t timechime_sound_get_count();

// Start receiving a new sound file.
bool timechime_sound_upload_begin(const char *file_name);

// Append received bytes to the sound file currently being uploaded.
bool timechime_sound_upload_write(const uint8_t *data, size_t len);

// Finish the upload and register file.
bool timechime_sound_upload_finish(uint8_t *sound_file_index);

// Cancel in-progress upload and discard the file.
void timechime_sound_upload_abort();

#endif
