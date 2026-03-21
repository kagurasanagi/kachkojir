#pragma once

#include <cstdint>

void debug_leds_init();
void debug_led_23(bool on);
void debug_led_24(bool on);
void debug_led_25(bool on);
void debug_leds_show_binary(uint8_t value);