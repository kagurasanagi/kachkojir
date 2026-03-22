#include "hw_config.h"

#include "hardware/gpio.h"

#include "app_config.hpp"
#include "storage_mode.hpp"

extern "C"
{
	static sd_sdio_if_t sdio_if{};
	static spi_t spi_if{};
	static sd_spi_if_t sd_spi_if{};
	static sd_card_t sd_card{};

	static void init_sd_sdio_mode()
	{
		sdio_if = {};
		sd_card = {};

		sdio_if.CMD_gpio = 3;
		sdio_if.D0_gpio = 4;
		sdio_if.set_drive_strength = true;
		sdio_if.CLK_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA;
		sdio_if.CMD_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA;
		sdio_if.D0_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA;
		sdio_if.D1_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA;
		sdio_if.D2_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA;
		sdio_if.D3_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA;
		sdio_if.SDIO_PIO = pio1;
		sdio_if.DMA_IRQ_num = DMA_IRQ_1;

#if PICO_RP2040
		sdio_if.baud_rate = 125 * 1000 * 1000 / 4;
#endif

#if PICO_RP2350
		sdio_if.baud_rate = 150 * 1000 * 1000 / 6;
#endif

		sd_card.type = SD_IF_SDIO;
		sd_card.sdio_if_p = &sdio_if;
		sd_card.use_card_detect = true;
		sd_card.card_detect_gpio = 9;
		sd_card.card_detected_true = 0;
	}

	static void init_sd_spi_mode()
	{
		spi_if = {};
		sd_spi_if = {};
		sd_card = {};

		spi_if.hw_inst = spi1;
		spi_if.miso_gpio = SD_SPI_MISO_GPIO;
		spi_if.mosi_gpio = SD_SPI_MOSI_GPIO;
		spi_if.sck_gpio = SD_SPI_SCK_GPIO;
		spi_if.baud_rate = SD_SPI_BAUD_RATE;

		sd_spi_if.spi = &spi_if;
		sd_spi_if.ss_gpio = SD_SPI_CS_GPIO;

		sd_card.type = SD_IF_SPI;
		sd_card.spi_if_p = &sd_spi_if;
		sd_card.use_card_detect = false;
	}

	void storage_hw_init_for_mode(StorageMode mode)
	{
		if (mode == StorageMode::SdioRawMsc)
		{
			init_sd_sdio_mode();
		}
		else
		{
			init_sd_spi_mode();
		}
	}

	size_t sd_get_num(void)
	{
		return 1;
	}

	sd_card_t *sd_get_by_num(size_t num)
	{
		if (num == 0)
		{
			return &sd_card;
		}

		return nullptr;
	}
}