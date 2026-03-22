#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"

extern "C"
{
#include "ff.h"
}

#include "tusb.h"

#include "app_config.hpp"
#include "boot_mode.hpp"
#include "storage_mode.hpp"
#include "usb_msc_mode.hpp"
#include "usb_gamepad_host.hpp"
#include "debug_leds.hpp"

extern "C"
{
	void usb_descriptors_set_msc_mode(bool enabled);
}

namespace
{
	FATFS g_fs;
	bool g_sd_ready = false;

	void cdc_printf(const char *fmt, ...)
	{
		if (!tud_inited() || !tud_cdc_connected())
		{
			return;
		}

		char buf[256];

		va_list args;
		va_start(args, fmt);
		int len = vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		if (len <= 0)
		{
			return;
		}

		if (len > (int)sizeof(buf))
		{
			len = sizeof(buf);
		}

		tud_cdc_write(buf, (uint32_t)len);
		tud_cdc_write_flush();
	}

	bool mount_sd_once()
	{
		std::memset(&g_fs, 0, sizeof(g_fs));
		FRESULT fr = f_mount(&g_fs, "", 1);
		return fr == FR_OK;
	}

	void dump_host_event(uint32_t event_id, uint32_t data0, uint32_t data1, uint32_t data2)
	{
		switch (event_id)
		{
		case USB_DEBUG_EVENT_HOST_STARTED:
			cdc_printf("[USBH] host started\r\n");
			break;

		case USB_DEBUG_EVENT_DEVICE_MOUNT:
		{
			uint8_t dev_addr = (uint8_t)data0;
			uint16_t vid = (uint16_t)(data1 >> 16);
			uint16_t pid = (uint16_t)(data1 & 0xffff);
			cdc_printf("[USBH] device mount: addr=%u vid=%04x pid=%04x\r\n", dev_addr, vid, pid);
			break;
		}

		case USB_DEBUG_EVENT_DEVICE_UMOUNT:
			cdc_printf("[USBH] device umount: addr=%u\r\n", (unsigned)data0);
			break;

		case USB_DEBUG_EVENT_HID_MOUNT:
		{
			uint8_t dev_addr = (uint8_t)(data0 >> 16);
			uint8_t instance = (uint8_t)(data0 & 0xffff);
			uint32_t protocol = data1;
			uint32_t desc_len = data2;
			cdc_printf(
				"[USBH] hid mount: addr=%u instance=%u protocol=%lu desc_len=%lu\r\n",
				dev_addr,
				instance,
				(unsigned long)protocol,
				(unsigned long)desc_len);
			break;
		}

		case USB_DEBUG_EVENT_HID_UMOUNT:
		{
			uint8_t dev_addr = (uint8_t)(data0 >> 16);
			uint8_t instance = (uint8_t)(data0 & 0xffff);
			cdc_printf("[USBH] hid umount: addr=%u instance=%u\r\n", dev_addr, instance);
			break;
		}

		case USB_DEBUG_EVENT_REPORT:
			break;

		case USB_DEBUG_EVENT_REPORT_REQUEST_FAIL:
		{
			uint8_t dev_addr = (uint8_t)(data0 >> 16);
			uint8_t instance = (uint8_t)(data0 & 0xffff);
			cdc_printf(
				"[USBH] report request failed: addr=%u instance=%u phase=%lu\r\n",
				dev_addr,
				instance,
				(unsigned long)data1);
			break;
		}

		case 100:
			cdc_printf("[USBH] core1 entered\r\n");
			break;

		case 101:
			cdc_printf(
				"[USBH] core1 configuring pio usb: dp_gpio=%lu pinout=%lu\r\n",
				(unsigned long)data0,
				(unsigned long)data1);
			break;

		case 102:
			cdc_printf("[USBH] tuh_configure done\r\n");
			break;

		case 103:
			cdc_printf("[USBH] calling tuh_init(1)\r\n");
			break;

		case 104:
			cdc_printf("[USBH] tuh_init(1) returned\r\n");
			break;

		default:
			cdc_printf(
				"[USBH] unknown event: id=%lu %lu %lu %lu\r\n",
				(unsigned long)event_id,
				(unsigned long)data0,
				(unsigned long)data1,
				(unsigned long)data2);
			break;
		}
	}
}

int main()
{
	debug_leds_init();

	BootMode mode = detect_boot_mode();

	if (mode == BootMode::MassStorage)
	{
		storage_set_mode(StorageMode::SdioRawMsc);
		storage_hw_init_for_mode(StorageMode::SdioRawMsc);
		usb_descriptors_set_msc_mode(true);

		tud_init(0);
		usb_msc_mode_init();

		while (true)
		{
			tud_task();

			if (usb_msc_mode_should_reboot())
			{
				if (tud_inited())
				{
					tud_disconnect();
					sleep_ms(USB_REENUMERATION_GUARD_MS);
				}

				watchdog_reboot(0, 0, MSC_REBOOT_DELAY_MS);

				while (true)
				{
					tight_loop_contents();
				}
			}

			sleep_ms(1);
		}
	}

	storage_set_mode(StorageMode::SpiFatFs);
	storage_hw_init_for_mode(StorageMode::SpiFatFs);
	usb_descriptors_set_msc_mode(false);

	tud_init(0);

	g_sd_ready = mount_sd_once();

	usb_gamepad_host_init();
	sleep_ms(50);

	while (true)
	{
		tud_task();

		static absolute_time_t last_log = 0;
		static absolute_time_t last_usb_snapshot = 0;
		absolute_time_t now = get_absolute_time();

		if (tud_cdc_connected())
		{
			if (last_log == 0 || absolute_time_diff_us(last_log, now) >= 1000 * 1000)
			{
				cdc_printf("normal mode alive\r\n");
				last_log = now;
			}

			if (last_usb_snapshot == 0 || absolute_time_diff_us(last_usb_snapshot, now) >= 1000 * 1000)
			{
				UsbHostDebugSnapshot s = usb_gamepad_host_get_debug_snapshot();

				cdc_printf(
					"[USBH] snapshot core1=%u cfg=%u host=%u mount=%lu umount=%lu hid_mount=%lu hid_umount=%lu report=%lu req_fail=%lu last=%lu\r\n",
					s.core1_entered ? 1u : 0u,
					s.host_configured ? 1u : 0u,
					s.host_started ? 1u : 0u,
					(unsigned long)s.mount_count,
					(unsigned long)s.umount_count,
					(unsigned long)s.hid_mount_count,
					(unsigned long)s.hid_umount_count,
					(unsigned long)s.report_count,
					(unsigned long)s.receive_request_fail_count,
					(unsigned long)s.last_event_id);

				last_usb_snapshot = now;
			}
		}

		uint32_t event_id = 0;
		uint32_t data0 = 0;
		uint32_t data1 = 0;
		uint32_t data2 = 0;

		while (usb_gamepad_host_consume_debug_event(event_id, data0, data1, data2))
		{
			dump_host_event(event_id, data0, data1, data2);
		}

		sleep_ms(1);
	}
}