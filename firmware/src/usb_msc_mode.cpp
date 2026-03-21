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
	constexpr BYTE MSC_PDRV = 0;

	bool card_ready()
	{
		DSTATUS st = disk_initialize(MSC_PDRV);
		return (st & STA_NOINIT) == 0;
	}

	uint32_t card_block_count()
	{
		LBA_t sector_count = 0;

		if (disk_ioctl(MSC_PDRV, GET_SECTOR_COUNT, &sector_count) != RES_OK)
		{
			return 0;
		}

		return static_cast<uint32_t>(sector_count);
	}

	uint16_t card_block_size()
	{
		WORD sector_size = 512;

		if (disk_ioctl(MSC_PDRV, GET_SECTOR_SIZE, &sector_size) != RES_OK)
		{
			return 512;
		}

		return static_cast<uint16_t>(sector_size);
	}

	int32_t card_read(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
	{
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
	disk_initialize(MSC_PDRV);
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
		return card_ready();
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

	int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
	{
		(void)lun;
		(void)buffer;
		(void)bufsize;

		switch (scsi_cmd[0])
		{
		case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
			return 0;

		case SCSI_CMD_START_STOP_UNIT:
			if ((scsi_cmd[4] & 0x03) == 0x02)
			{
				g_should_reboot = true;
			}
			return 0;

		default:
			return -1;
		}
	}

	void tud_mount_cb(void)
	{
		debug_led_25(true);
	}

	void tud_umount_cb(void)
	{
		debug_led_25(false);
		g_should_reboot = true;
	}
}