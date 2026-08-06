#include <zephyr/kernel.h>

#include "screen_ui.h"

int main(void)
{
	timechime_screen_init();
	timechime_screen_ui_clear();
	timechime_screen_draw_button_indicator_set((timechime_sprite_t[]){
		TIMECHIME_SPRITE_BELL,
		TIMECHIME_SPRITE_ARROW_UP,
		TIMECHIME_SPRITE_ARROW_DOWN,
		TIMECHIME_SPRITE_PLUS,
	});
	timechime_screen_wait();

	return 0;
}
