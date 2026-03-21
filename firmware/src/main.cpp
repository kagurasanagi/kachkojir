#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"

extern "C"
{
#include "ff.h"
}

#include "tusb.h"

#include "app_config.hpp"
#include "usb_gamepad_host.hpp"

namespace
{
	FATFS g_fs;
	bool g_sd_ready = false;

	void cdc_printf(const char *fmt, ...)
	{
		if (!tud_inited())
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

	void init_button(uint gpio)
	{
		gpio_init(gpio);
		gpio_set_dir(gpio, GPIO_IN);
		gpio_pull_up(gpio);
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
		{
			// uint8_t dev_addr = (uint8_t)(data0 >> 16);
			// uint8_t instance = (uint8_t)(data0 & 0xffff);
			// uint16_t protocol = (uint16_t)(data1 >> 16);
			// uint16_t len = (uint16_t)(data1 & 0xffff);
			// uint8_t first_byte = (uint8_t)data2;
			// cdc_printf(
			// 	"[USBH] report: addr=%u instance=%u protocol=%u len=%u first=%02x\r\n",
			// 	dev_addr,
			// 	instance,
			// 	protocol,
			// 	len,
			// 	first_byte);
			break;
		}

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
			cdc_printf("[USBH] core1 configuring pio usb: dp_gpio=%lu pinout=%lu\r\n",
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
			cdc_printf("[USBH] unknown event: id=%lu %lu %lu %lu\r\n",
					   (unsigned long)event_id,
					   (unsigned long)data0,
					   (unsigned long)data1,
					   (unsigned long)data2);
			break;
		}
	}

	void dump_gamepad_states()
	{
		constexpr uint16_t LOGICAL_REPORT_BYTES = 18;

		static uint8_t prev_report[USB_HOST_MAX_GAMEPADS][USB_HOST_MAX_REPORT_BYTES];
		static uint16_t prev_len[USB_HOST_MAX_GAMEPADS] = {};
		static bool prev_valid[USB_HOST_MAX_GAMEPADS] = {};

		auto axis_to_percent = [](uint8_t value) -> int
		{
			int centered = (int)value - 128;
			return (centered * 100) / 127;
		};

		auto hat_to_string = [](uint8_t hat) -> const char *
		{
			switch (hat & 0x0f)
			{
			case 0x0:
				return "up";
			case 0x1:
				return "up-right";
			case 0x2:
				return "right";
			case 0x3:
				return "down-right";
			case 0x4:
				return "down";
			case 0x5:
				return "down-left";
			case 0x6:
				return "left";
			case 0x7:
				return "up-left";
			default:
				return "center";
			}
		};

		auto append_button_names = [](char *dst, size_t dst_size, uint8_t b0, uint8_t b1) -> void
		{
			struct ButtonBit
			{
				uint8_t mask;
				const char *name;
			};

			static const ButtonBit byte0_map[] =
				{
					{0x01, "X"},
					{0x02, "A"},
					{0x04, "B"},
					{0x08, "Y"},
					{0x10, "L1"},
					{0x20, "R1"},
					{0x40, "L2"},
					{0x80, "R2"},
				};

			static const ButtonBit byte1_map[] =
				{
					{0x01, "SELECT"},
					{0x02, "START"},
					{0x04, "L3"},
					{0x08, "R3"},
					{0x10, "HOME"},
					{0x20, "CAPTURE"},
				};

			int pos = 0;
			bool first = true;

			for (size_t i = 0; i < sizeof(byte0_map) / sizeof(byte0_map[0]); ++i)
			{
				if ((b0 & byte0_map[i].mask) == 0)
				{
					continue;
				}

				pos += std::snprintf(
					dst + pos,
					dst_size - (size_t)pos,
					"%s%s",
					first ? "" : ",",
					byte0_map[i].name);
				first = false;

				if (pos < 0 || pos >= (int)dst_size)
				{
					return;
				}
			}

			for (size_t i = 0; i < sizeof(byte1_map) / sizeof(byte1_map[0]); ++i)
			{
				if ((b1 & byte1_map[i].mask) == 0)
				{
					continue;
				}

				pos += std::snprintf(
					dst + pos,
					dst_size - (size_t)pos,
					"%s%s",
					first ? "" : ",",
					byte1_map[i].name);
				first = false;

				if (pos < 0 || pos >= (int)dst_size)
				{
					return;
				}
			}

			if (first)
			{
				std::snprintf(dst, dst_size, "none");
			}
		};

		for (std::size_t i = 0; i < USB_HOST_MAX_GAMEPADS; ++i)
		{
			UsbGamepadState state{};
			if (!usb_gamepad_host_get_state(i, state))
			{
				continue;
			}

			if (!state.connected)
			{
				cdc_printf("[PAD] slot=%u disconnected\r\n", (unsigned)i);
				prev_valid[i] = false;
				prev_len[i] = 0;
				continue;
			}

			uint16_t logical_len = state.report_len;
			if (logical_len > LOGICAL_REPORT_BYTES)
			{
				logical_len = LOGICAL_REPORT_BYTES;
			}

			bool changed = true;

			if (prev_valid[i] && prev_len[i] == logical_len)
			{
				if (std::memcmp(prev_report[i], state.report, logical_len) == 0)
				{
					changed = false;
				}
			}

			if (!changed)
			{
				continue;
			}

			if (logical_len <= USB_HOST_MAX_REPORT_BYTES)
			{
				std::memcpy(prev_report[i], state.report, logical_len);
				prev_len[i] = logical_len;
				prev_valid[i] = true;
			}

			char raw_hex[LOGICAL_REPORT_BYTES * 3 + 1];
			int raw_pos = 0;

			for (uint16_t j = 0; j < logical_len && raw_pos >= 0 && raw_pos < (int)sizeof(raw_hex); ++j)
			{
				raw_pos += std::snprintf(
					raw_hex + raw_pos,
					sizeof(raw_hex) - (size_t)raw_pos,
					"%02x%s",
					state.report[j],
					(j + 1 < logical_len) ? " " : "");
			}

			if (raw_pos < 0 || raw_pos >= (int)sizeof(raw_hex))
			{
				raw_hex[sizeof(raw_hex) - 1] = '\0';
			}

			uint8_t b0 = logical_len > 0 ? state.report[0] : 0;
			uint8_t b1 = logical_len > 1 ? state.report[1] : 0;
			uint8_t hat = logical_len > 2 ? state.report[2] : 0x0f;

			uint8_t lx = logical_len > 3 ? state.report[3] : 128;
			uint8_t ly = logical_len > 4 ? state.report[4] : 128;
			uint8_t rx = logical_len > 5 ? state.report[5] : 128;
			uint8_t ry = logical_len > 6 ? state.report[6] : 128;

			char buttons[128];
			append_button_names(buttons, sizeof(buttons), b0, b1);

			cdc_printf(
				"[PAD] slot=%u addr=%u inst=%u vid=%04x pid=%04x buttons=[%s] dpad=%s "
				"lstick=(%3d,%3d) rstick=(%3d,%3d) "
				"len=%u data=%s\r\n",
				(unsigned)i,
				state.dev_addr,
				state.instance,
				state.vid,
				state.pid,
				buttons,
				hat_to_string(hat),
				axis_to_percent(lx),
				axis_to_percent(ly),
				axis_to_percent(rx),
				axis_to_percent(ry),
				logical_len,
				raw_hex);
		}
	}
}
int main()
{
	set_sys_clock_khz(120000, true);

	init_button(SELECT_BUTTON_GPIO);
	init_button(START_BUTTON_GPIO);

	usb_gamepad_host_init();
	sleep_ms(50);
	tud_init(0);

	g_sd_ready = mount_sd_once();

	while (true)
	{
		tud_task();

		static bool boot_logged = false;
		if (!boot_logged)
		{
			cdc_printf("kachkojir dual mode start\r\n");
			cdc_printf("SD mount: %s\r\n", g_sd_ready ? "OK" : "NG");
			boot_logged = true;
		}

		uint32_t event_id = 0;
		uint32_t data0 = 0;
		uint32_t data1 = 0;
		uint32_t data2 = 0;

		while (usb_gamepad_host_consume_debug_event(event_id, data0, data1, data2))
		{
			dump_host_event(event_id, data0, data1, data2);
		}

		dump_gamepad_states();

		// static absolute_time_t last_alive = get_absolute_time();
		// absolute_time_t now = get_absolute_time();

		// if (absolute_time_diff_us(last_alive, now) >= 1000 * 1000)
		// {
		// 	UsbHostDebugSnapshot s = usb_gamepad_host_get_debug_snapshot();

		// 	cdc_printf(
		// 		"alive core1=%u cfg=%u host=%u mount=%lu umount=%lu hid_mount=%lu hid_umount=%lu report=%lu req_fail=%lu\r\n",
		// 		s.core1_entered ? 1u : 0u,
		// 		s.host_configured ? 1u : 0u,
		// 		s.host_started ? 1u : 0u,
		// 		(unsigned long)s.mount_count,
		// 		(unsigned long)s.umount_count,
		// 		(unsigned long)s.hid_mount_count,
		// 		(unsigned long)s.hid_umount_count,
		// 		(unsigned long)s.report_count,
		// 		(unsigned long)s.receive_request_fail_count);

		// 	last_alive = now;
		// }

		sleep_ms(1);
	}
}