#include "pico/stdlib.h"

#include "debug_leds.hpp"

namespace
{
	constexpr uint LED22_GPIO = 22;
	constexpr uint LED23_GPIO = 23;
	constexpr uint LED24_GPIO = 24;
	constexpr uint LED25_GPIO = 25;
}

extern "C"
{
	void debug_leds_init(void)
	{
		gpio_init(LED22_GPIO);
		gpio_init(LED23_GPIO);
		gpio_init(LED24_GPIO);
		gpio_init(LED25_GPIO);

		gpio_set_dir(LED22_GPIO, GPIO_OUT);
		gpio_set_dir(LED23_GPIO, GPIO_OUT);
		gpio_set_dir(LED24_GPIO, GPIO_OUT);
		gpio_set_dir(LED25_GPIO, GPIO_OUT);

		gpio_put(LED22_GPIO, 0);
		gpio_put(LED23_GPIO, 0);
		gpio_put(LED24_GPIO, 0);
		gpio_put(LED25_GPIO, 0);
	}

	void debug_led_22(bool on)
	{
		gpio_put(LED22_GPIO, on ? 1 : 0);
	}

	void debug_led_23(bool on)
	{
		gpio_put(LED23_GPIO, on ? 1 : 0);
	}

	void debug_led_24(bool on)
	{
		gpio_put(LED24_GPIO, on ? 1 : 0);
	}

	void debug_led_25(bool on)
	{
		gpio_put(LED25_GPIO, on ? 1 : 0);
	}

	void debug_leds_show_binary(uint8_t value)
	{
		gpio_put(LED23_GPIO, (value & 0x01) ? 1 : 0);
		gpio_put(LED24_GPIO, (value & 0x02) ? 1 : 0);
		gpio_put(LED25_GPIO, (value & 0x04) ? 1 : 0);
	}
}