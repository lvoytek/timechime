#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#include "alarm.h"
#include "nav.h"
#include "sound.h"
#include "time.h"
#include "usb.h"

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = "/SD:",
};

int main(void)
{
	fs_mount(&mp);

	timechime_usb_init();
	timechime_nav_init();
	timechime_alarm_init();
	timechime_time_load_timezone_offset();

	timechime_sound_queue_set_volume(0x00, 0x00);

	while (1) {
		timechime_nav_update();
		timechime_alarm_update();
		timechime_sound_update();
		k_sleep(K_MSEC(100));
	}

	return 0;
}
