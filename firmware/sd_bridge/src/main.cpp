#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "pico/time.h"

extern "C"
{
#include "ff.h"
}

#include "app_config.hpp"

namespace
{

	FATFS g_fs;
	bool g_sd_ready = false;

	void init_button(uint gpio)
	{
		gpio_init(gpio);
		gpio_set_dir(gpio, GPIO_IN);
		gpio_pull_up(gpio);
	}

	bool is_button_pressed(uint gpio)
	{
		return gpio_get(gpio) == BUTTON_ACTIVE_LEVEL;
	}

	bool mount_sd_once()
	{
		std::memset(&g_fs, 0, sizeof(g_fs));

		std::printf("before f_mount\r\n");
		FRESULT fr = f_mount(&g_fs, "", 1);
		std::printf("after f_mount: %d\r\n", fr);

		if (fr != FR_OK)
		{
			return false;
		}

		return true;
	}

	bool write_text(FIL &fil, const char *text)
	{
		UINT written = 0;
		FRESULT fr = f_write(&fil, text, static_cast<UINT>(std::strlen(text)), &written);

		if (fr != FR_OK)
		{
			std::printf("f_write failed: %d\r\n", fr);
			return false;
		}

		if (written != std::strlen(text))
		{
			std::printf("short write: %u / %u\r\n",
						written,
						static_cast<unsigned>(std::strlen(text)));
			return false;
		}

		return true;
	}

	bool generate_file_list_txt()
	{
		if (!g_sd_ready)
		{
			std::printf("SD not ready\r\n");
			return false;
		}

		std::printf("generate_file_list_txt: begin\r\n");

		FIL fil;
		FRESULT fr = f_open(&fil, "LIST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
		std::printf("after f_open: %d\r\n", fr);
		if (fr != FR_OK)
		{
			return false;
		}

		auto fail_close = [&](const char *label, FRESULT code) -> bool
		{
			std::printf("%s: %d\r\n", label, code);
			f_close(&fil);
			return false;
		};

		if (!write_text(fil, "LIST.TXT\r\n"))
		{
			return fail_close("header write failed", FR_INT_ERR);
		}

		if (!write_text(fil, "==============================\r\n"))
		{
			return fail_close("separator write failed", FR_INT_ERR);
		}

		DIR dir;
		FILINFO fno;

		fr = f_opendir(&dir, "");
		std::printf("after f_opendir: %d\r\n", fr);
		if (fr != FR_OK)
		{
			return fail_close("f_opendir failed", fr);
		}

		for (;;)
		{
			fr = f_readdir(&dir, &fno);
			if (fr != FR_OK)
			{
				f_closedir(&dir);
				return fail_close("f_readdir failed", fr);
			}

			if (fno.fname[0] == '\0')
			{
				break;
			}

			char line[384];

			if (fno.fattrib & AM_DIR)
			{
				std::snprintf(line, sizeof(line), "[DIR ] %s\r\n", fno.fname);
			}
			else
			{
				std::snprintf(
					line,
					sizeof(line),
					"[FILE] %10lu %s\r\n",
					static_cast<unsigned long>(fno.fsize),
					fno.fname);
			}

			if (!write_text(fil, line))
			{
				f_closedir(&dir);
				return fail_close("item write failed", FR_INT_ERR);
			}
		}

		fr = f_closedir(&dir);
		if (fr != FR_OK)
		{
			return fail_close("f_closedir failed", fr);
		}

		fr = f_sync(&fil);
		std::printf("after f_sync: %d\r\n", fr);
		if (fr != FR_OK)
		{
			return fail_close("f_sync failed", fr);
		}

		fr = f_close(&fil);
		std::printf("after f_close: %d\r\n", fr);
		if (fr != FR_OK)
		{
			return false;
		}

		return true;
	}

}

int main()
{
	stdio_init_all();

	init_button(SELECT_BUTTON_GPIO);

	sleep_ms(1500);
	std::printf("kachkojir normal mode start\r\n");

	g_sd_ready = mount_sd_once();

	if (g_sd_ready)
	{
		std::printf("SD mounted successfully\r\n");
	}
	else
	{
		std::printf("SD mount failed\r\n");
	}

	std::printf("Press SELECT button to generate LIST.TXT\r\n");

	absolute_time_t last_trigger = get_absolute_time();

	while (true)
	{
		if (is_button_pressed(SELECT_BUTTON_GPIO))
		{
			absolute_time_t now = get_absolute_time();

			if (absolute_time_diff_us(last_trigger, now) >= static_cast<int64_t>(SELECT_REPEAT_GUARD_MS) * 1000)
			{
				sleep_ms(SELECT_DEBOUNCE_MS);

				if (is_button_pressed(SELECT_BUTTON_GPIO))
				{
					std::printf("SELECT pressed\r\n");

					bool ok = generate_file_list_txt();

					if (ok)
					{
						std::printf("LIST.TXT generated\r\n");
					}
					else
					{
						std::printf("LIST.TXT generation failed\r\n");
					}

					last_trigger = get_absolute_time();

					while (is_button_pressed(SELECT_BUTTON_GPIO))
					{
						sleep_ms(10);
					}
				}
			}
		}

		sleep_ms(10);
	}
}