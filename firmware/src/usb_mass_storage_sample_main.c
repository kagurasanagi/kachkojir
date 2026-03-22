#include "pico/stdlib.h"
#include "tusb.h"

int main(void)
{
	tud_init(0);

	while (true)
	{
		tud_task();
		sleep_ms(1);
	}
}