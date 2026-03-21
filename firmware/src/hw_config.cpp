#include "hw_config.h"

extern "C"
{
	static sd_sdio_if_t sdio_if{};
	static sd_card_t sd_card{};

	static void init_hw_config()
	{
		sdio_if.CMD_gpio = 3;
		sdio_if.D0_gpio = 4;
		sdio_if.SDIO_PIO = pio1;
		sdio_if.DMA_IRQ_num = DMA_IRQ_1;
		sdio_if.baud_rate = 120 * 1000 * 1000 / 4;

		sd_card.type = SD_IF_SDIO;
		sd_card.sdio_if_p = &sdio_if;
		sd_card.use_card_detect = false;
	}

	struct hw_config_initializer_t
	{
		hw_config_initializer_t()
		{
			init_hw_config();
		}
	};

	static hw_config_initializer_t hw_config_initializer;

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