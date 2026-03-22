#pragma once

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#include <stdbool.h>
#endif

	void debug_leds_init(void);
	void debug_led_22(bool on);
	void debug_led_23(bool on);
	void debug_led_24(bool on);
	void debug_led_25(bool on);
	void debug_leds_show_binary(uint8_t value);

#ifdef __cplusplus
}
#endif