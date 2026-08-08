#include <zephyr/fs/fs.h>
#include <ff.h>

#include "settings.h"

int8_t timechime_settings_load_timezone_offset_hours()
{
	return 0;
}

void timechime_settings_save_timezone_offset(int8_t hours, int8_t minutes)
{
	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/timezone.txt", FS_O_CREATE | FS_O_WRITE);

	const char data[10];
	snprintf(data, sizeof(data), "%d %d", hours, minutes);

	fs_write(&file, data, strnlen(data, sizeof(data)));
	fs_close(&file);
}

int8_t timechime_settings_load_timezone_offset_minutes()
{
	return 0;
}

void timechime_settings_save_timezone_offset_minutes(int8_t minutes)
{
	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/timezone_minutes.txt", FS_O_CREATE | FS_O_WRITE);
	fs_write(&file, &minutes, sizeof(minutes));
	fs_close(&file);
}
