#include <array>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"

extern "C"
{
#include "diskio.h"
#include "ff.h"
#include "f_util.h"
#include "hw_config.h"
}

namespace
{
	constexpr BYTE kPdrv = 0;
	constexpr LBA_t kTestLba = 0;
	constexpr UINT kSectorCount = 1;
	constexpr size_t kSectorSize = FF_MAX_SS;
	constexpr int kReadRepeatCount = 3;

	void wait_for_stdio()
	{
		stdio_init_all();

		for (int i = 0; i < 100; ++i)
		{
			sleep_ms(100);
		}
	}

	void print_card_config()
	{
		std::printf("sd_get_num() = %u\r\n", static_cast<unsigned>(sd_get_num()));

		sd_card_t *card = sd_get_by_num(kPdrv);
		if (!card)
		{
			std::printf("sd_get_by_num(%u) returned null\r\n", static_cast<unsigned>(kPdrv));
			return;
		}

		std::printf("card type = %d\r\n", static_cast<int>(card->type));

		if (card->type == SD_IF_SDIO && card->sdio_if_p)
		{
			std::printf(
				"SDIO cfg: CMD=%u D0=%u PIO=%s DMA_IRQ=%u baud=%" PRIu32 "\r\n",
				card->sdio_if_p->CMD_gpio,
				card->sdio_if_p->D0_gpio,
				(card->sdio_if_p->SDIO_PIO == pio0) ? "pio0" : (card->sdio_if_p->SDIO_PIO == pio1) ? "pio1"
																								   : "unknown",
				static_cast<unsigned>(card->sdio_if_p->DMA_IRQ_num),
				static_cast<uint32_t>(card->sdio_if_p->baud_rate));
		}
	}

	void dump_hex(const uint8_t *data, size_t size)
	{
		for (size_t base = 0; base < size; base += 16)
		{
			std::printf("%04zx :", base);

			for (size_t i = 0; i < 16; ++i)
			{
				if (base + i < size)
				{
					std::printf(" %02x", data[base + i]);
				}
				else
				{
					std::printf("   ");
				}
			}

			std::printf("  |");

			for (size_t i = 0; i < 16 && (base + i) < size; ++i)
			{
				uint8_t c = data[base + i];
				std::printf("%c", (c >= 0x20 && c <= 0x7e) ? static_cast<char>(c) : '.');
			}

			std::printf("|\r\n");
		}
	}

	uint32_t checksum32(const uint8_t *data, size_t size)
	{
		uint32_t acc = 0x811c9dc5u;

		for (size_t i = 0; i < size; ++i)
		{
			acc ^= data[i];
			acc *= 16777619u;
		}

		return acc;
	}

	bool read_sector_once(BYTE pdrv, LBA_t lba, uint8_t *buffer, size_t buffer_size)
	{
		if (buffer_size < kSectorSize)
		{
			std::printf("buffer too small: %u < %u\r\n",
						static_cast<unsigned>(buffer_size),
						static_cast<unsigned>(kSectorSize));
			return false;
		}

		std::memset(buffer, 0, buffer_size);

		DRESULT dr = disk_read(pdrv, buffer, lba, kSectorCount);
		if (dr != RES_OK)
		{
			std::printf("disk_read(pdrv=%u, lba=%" PRIu32 ") failed: %d\r\n",
						static_cast<unsigned>(pdrv),
						static_cast<uint32_t>(lba),
						static_cast<int>(dr));
			return false;
		}

		return true;
	}

	void print_disk_status(BYTE pdrv)
	{
		DSTATUS st = disk_status(pdrv);
		std::printf("disk_status(%u) = 0x%02x\r\n",
					static_cast<unsigned>(pdrv),
					static_cast<unsigned>(st));
	}

	bool initialize_disk(BYTE pdrv)
	{
		print_disk_status(pdrv);

		std::printf("before disk_initialize(%u)\r\n", static_cast<unsigned>(pdrv));
		DSTATUS st = disk_initialize(pdrv);
		std::printf("after disk_initialize(%u): 0x%02x\r\n",
					static_cast<unsigned>(pdrv),
					static_cast<unsigned>(st));

		if (st & STA_NOINIT)
		{
			std::printf("disk still reports STA_NOINIT\r\n");
			return false;
		}

		print_disk_status(pdrv);
		return true;
	}

	void probe_block_zero()
	{
		alignas(4) std::array<uint8_t, kSectorSize> first{};
		alignas(4) std::array<uint8_t, kSectorSize> current{};

		bool have_first = false;

		for (int attempt = 0; attempt < kReadRepeatCount; ++attempt)
		{
			std::printf("---- read attempt %d ----\r\n", attempt + 1);

			if (!read_sector_once(kPdrv, kTestLba, current.data(), current.size()))
			{
				std::printf("raw read failed on attempt %d\r\n", attempt + 1);
				return;
			}

			uint32_t sum = checksum32(current.data(), current.size());
			std::printf("block %" PRIu32 " checksum32 = 0x%08" PRIx32 "\r\n",
						static_cast<uint32_t>(kTestLba),
						sum);

			dump_hex(current.data(), 64);

			if (!have_first)
			{
				first = current;
				have_first = true;
				continue;
			}

			if (std::memcmp(first.data(), current.data(), current.size()) == 0)
			{
				std::printf("compare vs attempt 1: IDENTICAL\r\n");
			}
			else
			{
				std::printf("compare vs attempt 1: DIFFERENT\r\n");

				for (size_t i = 0; i < current.size(); ++i)
				{
					if (first[i] != current[i])
					{
						std::printf("first diff at offset 0x%04zx: %02x -> %02x\r\n",
									i,
									first[i],
									current[i]);
						break;
					}
				}
			}
		}
	}

	void optional_mount_check()
	{
		FATFS fs;
		std::memset(&fs, 0, sizeof(fs));

		std::printf("before optional f_mount\r\n");
		FRESULT fr = f_mount(&fs, "", 1);
		std::printf("after optional f_mount: %d (%s)\r\n", fr, FRESULT_str(fr));

		if (fr == FR_OK)
		{
			f_unmount("");
			std::printf("optional f_mount succeeded\r\n");
		}
	}
}

int main()
{
	wait_for_stdio();

	std::printf("\r\n");
	std::printf("kachkojir sdio raw-read probe start\r\n");

	print_card_config();

	if (!initialize_disk(kPdrv))
	{
		std::printf("disk_initialize failed\r\n");
		while (true)
		{
			sleep_ms(1000);
		}
	}

	probe_block_zero();

	// raw read が安定してから mount 側を確認したいときだけ有効化
	// optional_mount_check();

	std::printf("probe done\r\n");

	while (true)
	{
		sleep_ms(1000);
	}
}