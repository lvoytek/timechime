#include <zephyr/fs/fs.h>
#include <ff.h>

#include "settings.h"

void timechime_settings_load_timezone_offset(int8_t *hours, int8_t *minutes)
{
	*hours = 0;
	*minutes = 0;

	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/timezone.txt", FS_O_READ);

	const char data[10];

	fs_read(&file, data, sizeof(data));
	fs_close(&file);

	sscanf(data, "%hhd %hhd", hours, minutes);
}

void timechime_settings_save_timezone_offset(int8_t hours, int8_t minutes)
{
	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/timezone.txt", FS_O_CREATE | FS_O_WRITE);

	char data[10];
	snprintf(data, sizeof(data), "%hhd %hhd", hours, minutes);

	fs_write(&file, data, strnlen(data, sizeof(data)));
	fs_close(&file);
}
