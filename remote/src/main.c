#include <zephyr/kernel.h>

#include "screen_ui.h"

int main(void)
{
	timechime_screen_init();
	timechime_screen_ui_clear();

	return 0;
}
