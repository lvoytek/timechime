#include <zephyr/kernel.h>

#include "nav.h"
#include "time.h"

int main(void)
{
	timechime_nav_init();

	while (1) {
		timechime_nav_update();
		k_sleep(K_MSEC(100));
	}

	return 0;
}
