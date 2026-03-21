#pragma once

#include <cstdint>

void usb_msc_mode_init();
bool usb_msc_mode_should_reboot();

extern "C"
{
	int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize);
	int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize);
	void tud_msc_write10_complete_cb(uint8_t lun);
	bool tud_msc_test_unit_ready_cb(uint8_t lun);
	void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size);
	int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize);
	void tud_umount_cb(void);
}