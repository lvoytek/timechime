#include <zephyr/fs/fs.h>
#include <zephyr/sys/util.h>
#include <ff.h>
#include <vs1053.h>

#include <stdio.h>
#include <string.h>

#include "pot.h"
#include "screen_ui.h"
#include "sound.h"

#define TIMECHIME_SOUND_DIR "/SD:/sounds"

const static struct device *dev = DEVICE_DT_GET(DT_NODELABEL(vs1053));

static char sound_files[255][65];
uint8_t num_sound_files = 0;
uint8_t current_sound_file;
static bool play_sound = false;

static uint8_t volume_left = 0x00;
static uint8_t volume_right = 0x00;
static bool update_volume = false;

static struct fs_file_t upload_file;
static char upload_path[65];
static bool upload_active = false;

bool timechime_sound_init()
{
	for (uint8_t i = 0; i < 255; i++) {
		sound_files[i][0] = '\0';
	}

	num_sound_files = timechime_settings_load_sound_files(sound_files);

	if (!device_is_ready(dev)) {
		return false;
	}

	vs1053_soft_reset(dev);

	return true;
}

void timechime_sound_add(const char *file_path)
{
	if (num_sound_files >= 255) {
		return;
	}

	strncpy(sound_files[num_sound_files], file_path, 64);
	sound_files[num_sound_files][64] = '\0';
	num_sound_files++;
	timechime_settings_save_sound_files(sound_files, num_sound_files);
}

void timechime_sound_remove(uint8_t sound_file_index)
{
	if (sound_file_index >= num_sound_files) {
		return;
	}

	for (uint8_t i = sound_file_index; i < num_sound_files - 1; i++) {
		strncpy(sound_files[i], sound_files[i + 1], 64);
		sound_files[i][64] = '\0';
	}

	sound_files[num_sound_files - 1][0] = '\0';
	num_sound_files--;
	timechime_settings_save_sound_files(sound_files, num_sound_files);
}

uint8_t timechime_sound_get_count()
{
	return num_sound_files;
}

// Formats supported by the VS1053 decoder, as FAT16 three character extensions.
static bool sound_extension_is_supported(const char *extension)
{
	static const char *const extensions[] = {"mp3", "aac", "m4a", "mp4", "ogg",
						 "oga", "wma", "mid", "fla", "wav"};
	char lowered[4];

	for (size_t i = 0; i < 3; i++) {
		char c = extension[i];

		lowered[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
	}
	lowered[3] = '\0';

	for (size_t i = 0; i < ARRAY_SIZE(extensions); i++) {
		if (strcmp(lowered, extensions[i]) == 0) {
			return true;
		}
	}

	return false;
}

static bool sound_name_is_valid(const char *file_name)
{
	size_t len = strlen(file_name);
	const char *dot = strchr(file_name, '.');

	if (len == 0 || len > TIMECHIME_SOUND_NAME_MAX_LEN || dot == NULL) {
		return false;
	}

	size_t base_len = (size_t)(dot - file_name);

	if (base_len < 1 || base_len > 8 || strlen(dot + 1) != 3) {
		return false;
	}

	// Restrict to a safe character set so the name cannot escape the sound directory.
	for (size_t i = 0; i < len; i++) {
		char c = file_name[i];

		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
		      c == '_' || c == '-')) {
			if (i != base_len) {
				return false;
			}
		}
	}

	return sound_extension_is_supported(dot + 1);
}

bool timechime_sound_upload_begin(const char *file_name)
{
	timechime_sound_upload_abort();

	if (num_sound_files >= 255 || !sound_name_is_valid(file_name)) {
		return false;
	}

	fs_mkdir(TIMECHIME_SOUND_DIR);
	snprintf(upload_path, sizeof(upload_path), TIMECHIME_SOUND_DIR "/%s", file_name);

	fs_file_t_init(&upload_file);
	if (fs_open(&upload_file, upload_path, FS_O_CREATE | FS_O_WRITE) != 0) {
		upload_path[0] = '\0';
		return false;
	}

	fs_truncate(&upload_file, 0);
	upload_active = true;

	return true;
}

bool timechime_sound_upload_write(const uint8_t *data, size_t len)
{
	if (!upload_active) {
		return false;
	}

	if (len == 0) {
		return true;
	}

	return fs_write(&upload_file, data, len) == (ssize_t)len;
}

bool timechime_sound_upload_finish(uint8_t *sound_file_index)
{
	if (!upload_active) {
		return false;
	}

	fs_close(&upload_file);
	upload_active = false;

	// Keep alarm index on sound re-upload.
	for (uint8_t i = 0; i < num_sound_files; i++) {
		if (strcmp(sound_files[i], upload_path) == 0) {
			*sound_file_index = i;
			return true;
		}
	}

	if (num_sound_files >= 255) {
		return false;
	}

	*sound_file_index = num_sound_files;
	timechime_sound_add(upload_path);

	return true;
}

void timechime_sound_upload_abort()
{
	if (!upload_active) {
		return;
	}

	fs_close(&upload_file);
	upload_active = false;
	fs_unlink(upload_path);
	upload_path[0] = '\0';
}

void timechime_sound_queue_set_volume(uint8_t left, uint8_t right)
{
	volume_left = left;
	volume_right = right;
	update_volume = true;
}

void timechime_queue_sound_play(uint8_t sound_file_index)
{
	if (sound_file_index >= num_sound_files) {
		return;
	}

	current_sound_file = sound_file_index;
	play_sound = true;
}

void timechime_sound_play()
{
	if (device_is_ready(dev)) {
		struct fs_file_t f;
		uint8_t buf[VS1053_SDI_CHUNK_SIZE];
		ssize_t n;

		fs_file_t_init(&f);
		if (fs_open(&f, sound_files[current_sound_file], FS_O_READ) == 0) {
			while ((n = fs_read(&f, buf, sizeof(buf))) > 0) {
				vs1053_sdi_write(dev, buf, n);
			}
			fs_close(&f);
		}
	}
}

// Translate pot value to VS1053 volume.
static uint8_t sound_volume_from_pot()
{
	uint8_t level = timechime_pot_get_value();

	return (uint8_t)(0xFE - ((uint16_t)level * 0xFE) / UINT8_MAX);
}

void timechime_sound_update()
{
	if (timechime_screen_is_busy()) {
		return;
	}

	if (play_sound) {
		uint8_t attenuation = sound_volume_from_pot();

		timechime_sound_queue_set_volume(attenuation, attenuation);
	}

	if (update_volume && device_is_ready(dev)) {
		vs1053_set_volume(dev, volume_left, volume_right);
		update_volume = false;
	}

	if (play_sound) {
		timechime_sound_play();
		play_sound = false;
	}
}

bool timechime_sound_get_name(uint8_t sound_file_index, size_t max_len, char *name)
{
	if (sound_file_index >= num_sound_files || name == NULL || max_len == 0) {
		return false;
	}

	// Get file base name, removing the directory and extension.
	const char *file_path = sound_files[sound_file_index];
	const char *base_name = strrchr(file_path, '/');
	if (base_name == NULL) {
		base_name = file_path;
	} else {
		base_name++;
	}

	const char *dot = strrchr(base_name, '.');
	size_t base_len = (dot != NULL) ? (size_t)(dot - base_name) : strlen(base_name);

	if (base_len > max_len - 1) {
		base_len = max_len - 1;
	}

	memcpy(name, base_name, base_len);
	name[base_len] = '\0';
	return true;
}
