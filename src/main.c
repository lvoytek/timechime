#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#include "nav.h"
#include "time.h"
#include "settings.h"

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = "/SD:",
};

int main(void)
{
	fs_mount(&mp);

	timechime_nav_init();
	timechime_time_load_timezone_offset();

	while (1) {
		timechime_nav_update();
		k_sleep(K_MSEC(100));
	}

	return 0;
}
