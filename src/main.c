#include <zephyr/kernel.h>

#include "screen_ui.h"
#include "nav.h"
#include "time.h"

int main(void)
{
	timechime_nav_begin();
	timechime_screen_init();
	timechime_screen_ui_clear();
	timechime_screen_draw_button_indicator_set((timechime_sprite_t[]){
		TIMECHIME_SPRITE_BELL,
		TIMECHIME_SPRITE_ARROW_UP,
		TIMECHIME_SPRITE_ARROW_DOWN,
		TIMECHIME_SPRITE_PLUS,
	});

	while (1) {
		if (timechime_time_updated()) {
			timechime_screen_ui_clear();
			timechime_screen_draw_button_indicator_set((timechime_sprite_t[]){
				TIMECHIME_SPRITE_BELL,
				TIMECHIME_SPRITE_ARROW_UP,
				TIMECHIME_SPRITE_ARROW_DOWN,
				TIMECHIME_SPRITE_PLUS,
			});
			timechime_screen_draw_current_time(timechime_time_get_current_hour(),
							   timechime_time_get_current_minute());
			timechime_screen_wait();
		}
		k_sleep(K_MSEC(100));
	}

	return 0;
}
