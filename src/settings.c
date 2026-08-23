#include <zephyr/fs/fs.h>
#include <ff.h>

#include "settings.h"

uint8_t timechime_settings_load_alarms(timechime_alarm_t *alarms, uint8_t max_alarms)
{
	if (max_alarms > TIMECHIME_MAX_ALARMS) {
		max_alarms = TIMECHIME_MAX_ALARMS;
	}

	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/alarms.txt", FS_O_READ);

	char data[TIMECHIME_MAX_ALARMS * 17];

	int bytes_read = fs_read(&file, data, sizeof(data) - 1);
	data[bytes_read > 0 ? bytes_read : 0] = '\0';
	fs_close(&file);

	uint8_t alarm_count = 0;
	char *line = data;
	char *next_line = strchr(line, '\n');

	while (line != NULL && next_line != NULL && alarm_count < max_alarms) {
		*next_line = '\0';

		timechime_alarm_t *alarm = &alarms[alarm_count];
		if (sscanf(line, "%hhu %hhu %hhu %hhu", &alarm->hour, &alarm->minute,
			   &alarm->enabled, &alarm->sound_id) != 4) {
			break;
		}

		alarm_count++;
		line = next_line + 1;
		next_line = strchr(line, '\n');
	}

	return alarm_count;
}

void timechime_settings_save_alarms(timechime_alarm_t *alarms, uint8_t alarm_count)
{
	if (alarm_count > TIMECHIME_MAX_ALARMS) {
		alarm_count = TIMECHIME_MAX_ALARMS;
	}

	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/alarms.txt", FS_O_CREATE | FS_O_WRITE);
	fs_truncate(&file, 0);

	char data[TIMECHIME_MAX_ALARMS * 17];
	size_t offset = 0;
	for (uint8_t i = 0; i < alarm_count && offset < sizeof(data); i++) {
		timechime_alarm_t *alarm = &alarms[i];
		offset += snprintf(data + offset, sizeof(data) - offset, "%hhu %hhu %hhu %hhu\n",
				   alarm->hour, alarm->minute, alarm->enabled, alarm->sound_id);
	}

	fs_write(&file, data, offset);
	fs_close(&file);
}

void timechime_settings_load_timezone_offset(int8_t *hours, int8_t *minutes)
{
	*hours = 0;
	*minutes = 0;

	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/timezone.txt", FS_O_READ);

	char data[10];

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

uint8_t timechime_settings_load_sound_files(char sound_files[255][65])
{
	uint8_t count = 0;

	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/sounds.txt", FS_O_READ);

	char data[255 * 65];
	int bytes_read = fs_read(&file, data, sizeof(data) - 1);
	data[bytes_read > 0 ? bytes_read : 0] = '\0';
	fs_close(&file);

	char *line = data;
	char *next_line = strchr(line, '\n');

	while (line != NULL && next_line != NULL) {
		*next_line = '\0';

		snprintf(sound_files[count], sizeof(sound_files[count]), "%s", line);
		count++;

		line = next_line + 1;
		next_line = strchr(line, '\n');
	}

	return count;
}

void timechime_settings_save_sound_files(char sound_files[255][65], uint8_t sound_file_count)
{
	struct fs_file_t file;
	fs_file_t_init(&file);
	fs_open(&file, "/SD:/sounds.txt", FS_O_CREATE | FS_O_WRITE);
	fs_truncate(&file, 0);

	char data[255 * 65];
	size_t offset = 0;
	for (uint8_t i = 0; i < sound_file_count && offset < sizeof(data); i++) {
		offset += snprintf(data + offset, sizeof(data) - offset, "%s\n", sound_files[i]);
	}

	fs_write(&file, data, offset);
	fs_close(&file);
}
