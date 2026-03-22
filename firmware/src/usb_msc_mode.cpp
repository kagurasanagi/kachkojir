#include <cstring>

#include "pico/stdlib.h"
#include "tusb.h"

extern "C"
{
#include "diskio.h"
}

#include "usb_msc_mode.hpp"
#include "debug_leds.hpp"

namespace
{
	volatile bool g_should_reboot = false;
	bool g_ejected = false;
	constexpr BYTE MSC_PDRV = 0;

	bool card_ready()
	{
		DSTATUS st = disk_initialize(MSC_PDRV);
		return ((st & STA_NOINIT) == 0) && ((st & STA_NODISK) == 0);
	}

	uint32_t card_block_count()
	{
		LBA_t sector_count = 0;

		if (!card_ready())
		{
			return 0;
		}

		if (disk_ioctl(MSC_PDRV, GET_SECTOR_COUNT, &sector_count) != RES_OK)
		{
			return 0;
		}

		return static_cast<uint32_t>(sector_count);
	}

	uint16_t card_block_size()
	{
		WORD sector_size = 512;

		if (!card_ready())
		{
			return 512;
		}

		if (disk_ioctl(MSC_PDRV, GET_SECTOR_SIZE, &sector_size) != RES_OK)
		{
			return 512;
		}

		return static_cast<uint16_t>(sector_size);
	}

	int32_t card_read(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
	{
		if (g_ejected || !card_ready())
		{
			return -1;
		}

		uint16_t block_size = card_block_size();

		if (offset != 0 || (bufsize % block_size) != 0)
		{
			return -1;
		}

		UINT block_count = bufsize / block_size;

		DRESULT rc = disk_read(
			MSC_PDRV,
			static_cast<BYTE *>(buffer),
			static_cast<LBA_t>(lba),
			block_count);

		return (rc == RES_OK) ? static_cast<int32_t>(bufsize) : -1;
	}

	int32_t card_write(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
	{
		if (g_ejected || !card_ready())
		{
			return -1;
		}

		uint16_t block_size = card_block_size();

		if (offset != 0 || (bufsize % block_size) != 0)
		{
			return -1;
		}

		UINT block_count = bufsize / block_size;

		DRESULT rc = disk_write(
			MSC_PDRV,
			buffer,
			static_cast<LBA_t>(lba),
			block_count);

		return (rc == RES_OK) ? static_cast<int32_t>(bufsize) : -1;
	}
}

void usb_msc_mode_init()
{
	g_should_reboot = false;
	g_ejected = false;
	// ここで disk_initialize() しない
}

bool usb_msc_mode_should_reboot()
{
	return g_should_reboot;
}

extern "C"
{
	void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
	{
		(void)lun;

		const char vid[8] = {'K', 'a', 'c', 'h', 'k', 'o', 'j', 'i'};
		const char pid[16] = {'r', ' ', 'S', 'D', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e', ' ', ' ', ' ', ' '};
		const char rev[4] = {'0', '0', '0', '1'};

		memcpy(vendor_id, vid, 8);
		memcpy(product_id, pid, 16);
		memcpy(product_rev, rev, 4);
	}

	bool tud_msc_test_unit_ready_cb(uint8_t lun)
	{
		(void)lun;
		return !g_ejected && card_ready();
	}

	void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
	{
		(void)lun;
		*block_count = card_block_count();
		*block_size = card_block_size();
	}

	int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
	{
		(void)lun;
		return card_read(lba, offset, buffer, bufsize);
	}

	bool tud_msc_is_writable_cb(uint8_t lun)
	{
		(void)lun;

		if (g_ejected)
		{
			return false;
		}

		DSTATUS st = disk_status(MSC_PDRV);
		return (st & STA_PROTECT) == 0;
	}

	int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
	{
		(void)lun;
		return card_write(lba, offset, buffer, bufsize);
	}

	void tud_msc_write10_complete_cb(uint8_t lun)
	{
		(void)lun;
		disk_ioctl(MSC_PDRV, CTRL_SYNC, nullptr);
	}

	bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
	{
		(void)lun;
		(void)power_condition;

		if (load_eject)
		{
			if (start)
			{
				g_ejected = false;
			}
			else
			{
				if (disk_ioctl(MSC_PDRV, CTRL_SYNC, nullptr) != RES_OK)
				{
					return false;
				}

				g_ejected = true;
				g_should_reboot = true;
			}
		}

		return true;
	}

	int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
	{
		(void)lun;
		(void)scsi_cmd;
		(void)buffer;
		(void)bufsize;
		return -1;
	}

	void tud_mount_cb(void)
	{
	}

	void tud_umount_cb(void)
	{
		g_should_reboot = true;
	}
}