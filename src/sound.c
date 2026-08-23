#include <zephyr/fs/fs.h>
#include <ff.h>
#include <vs1053.h>

#include "screen_ui.h"
#include "sound.h"

const static struct device *dev = DEVICE_DT_GET(DT_NODELABEL(vs1053));

static char sound_files[255][65];
uint8_t num_sound_files = 0;
uint8_t current_sound_file;
static bool play_sound = false;

static uint8_t volume_left = 0x00;
static uint8_t volume_right = 0x00;
static bool update_volume = false;

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

void timechime_sound_update()
{
	if (timechime_screen_is_busy()) {
		return;
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
