#include "hw_config.h"

extern "C"
{

	static sd_sdio_if_t sdio_if =
		{
			.CMD_gpio = 3,
			.D0_gpio = 4,
			.SDIO_PIO = pio1,
			.DMA_IRQ_num = DMA_IRQ_1,
			.baud_rate = 125 * 1000 * 1000 / 4,
	};

	static sd_card_t sd_card =
		{
			.type = SD_IF_SDIO,
			.sdio_if_p = &sdio_if,
			.use_card_detect = false};

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