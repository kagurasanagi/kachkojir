#include "pico/stdlib.h"

#include "app_config.hpp"
#include "boot_mode.hpp"

namespace
{
	void init_boot_button(uint gpio)
	{
		gpio_init(gpio);
		gpio_set_dir(gpio, GPIO_IN);
		gpio_pull_up(gpio);
	}

	bool is_boot_button_pressed()
	{
		return gpio_get(SELECT_BUTTON_GPIO) == 0;
	}
}

bool is_host_connected_to_pc()
{
	return false;
}

BootMode detect_boot_mode()
{
	init_boot_button(SELECT_BUTTON_GPIO);
	sleep_ms(10);

	return is_boot_button_pressed() ? BootMode::MassStorage : BootMode::Normal;
}