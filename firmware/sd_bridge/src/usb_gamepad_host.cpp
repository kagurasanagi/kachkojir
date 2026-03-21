#include <cstring>

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "pio_usb.h"
#include "tusb.h"

#include "app_config.hpp"
#include "usb_gamepad_host.hpp"

namespace
{
	constexpr uint8_t MAX_REPORT_INFO = 6;

	struct HidInterfaceInfo
	{
		uint8_t report_count;
		tuh_hid_report_info_t report_info[MAX_REPORT_INFO];
	};

	struct GamepadSlot
	{
		UsbGamepadState state;
		bool in_use;
		bool changed;
	};

	GamepadSlot g_slots[USB_HOST_MAX_GAMEPADS];
	HidInterfaceInfo g_hid_info[CFG_TUH_HID];

	volatile bool g_core1_entered = false;
	volatile bool g_host_configured = false;
	volatile bool g_host_started = false;
	volatile uint32_t g_mount_count = 0;
	volatile uint32_t g_umount_count = 0;
	volatile uint32_t g_hid_mount_count = 0;
	volatile uint32_t g_hid_umount_count = 0;
	volatile uint32_t g_report_count = 0;
	volatile uint32_t g_receive_request_fail_count = 0;

	volatile uint32_t g_last_event_id = USB_DEBUG_EVENT_NONE;
	volatile uint32_t g_last_event_data0 = 0;
	volatile uint32_t g_last_event_data1 = 0;
	volatile uint32_t g_last_event_data2 = 0;
	volatile uint32_t g_last_event_seq = 0;
	uint32_t g_last_consumed_event_seq = 0;

	void push_debug_event(uint32_t event_id, uint32_t data0, uint32_t data1, uint32_t data2)
	{
		g_last_event_id = event_id;
		g_last_event_data0 = data0;
		g_last_event_data1 = data1;
		g_last_event_data2 = data2;
		++g_last_event_seq;
	}

	void core1_usb_host_main();

	int find_slot(uint8_t dev_addr, uint8_t instance)
	{
		for (std::size_t i = 0; i < USB_HOST_MAX_GAMEPADS; ++i)
		{
			if (g_slots[i].in_use &&
				g_slots[i].state.dev_addr == dev_addr &&
				g_slots[i].state.instance == instance)
			{
				return static_cast<int>(i);
			}
		}

		return -1;
	}

	int allocate_slot(uint8_t dev_addr, uint8_t instance)
	{
		int existing = find_slot(dev_addr, instance);
		if (existing >= 0)
		{
			return existing;
		}

		for (std::size_t i = 0; i < USB_HOST_MAX_GAMEPADS; ++i)
		{
			if (!g_slots[i].in_use)
			{
				std::memset(&g_slots[i], 0, sizeof(g_slots[i]));
				g_slots[i].in_use = true;
				g_slots[i].state.connected = true;
				g_slots[i].state.mounted = true;
				g_slots[i].state.updated = true;
				g_slots[i].state.dev_addr = dev_addr;
				g_slots[i].state.instance = instance;
				g_slots[i].changed = true;
				return static_cast<int>(i);
			}
		}

		return -1;
	}

	void release_slot(uint8_t dev_addr, uint8_t instance)
	{
		int slot = find_slot(dev_addr, instance);
		if (slot < 0)
		{
			return;
		}

		g_slots[slot].state.connected = false;
		g_slots[slot].state.mounted = false;
		g_slots[slot].state.updated = true;
		g_slots[slot].changed = true;
		g_slots[slot].in_use = false;
	}

	const tuh_hid_report_info_t *find_report_info(uint8_t instance, uint8_t const *report, uint16_t &len)
	{
		if (instance >= CFG_TUH_HID)
		{
			return nullptr;
		}

		HidInterfaceInfo &info = g_hid_info[instance];
		if (info.report_count == 0)
		{
			return nullptr;
		}

		if (info.report_count == 1 && info.report_info[0].report_id == 0)
		{
			return &info.report_info[0];
		}

		uint8_t report_id = report[0];

		for (uint8_t i = 0; i < info.report_count; ++i)
		{
			if (info.report_info[i].report_id == report_id)
			{
				if (len > 0)
				{
					--len;
				}
				return &info.report_info[i];
			}
		}

		return nullptr;
	}

	bool is_gamepad_usage(uint16_t usage_page, uint16_t usage)
	{
		if (usage_page != HID_USAGE_PAGE_DESKTOP)
		{
			return false;
		}

		return usage == HID_USAGE_DESKTOP_GAMEPAD || usage == HID_USAGE_DESKTOP_JOYSTICK;
	}

	void core1_usb_host_main()
	{
		g_core1_entered = true;
		push_debug_event(100, 0, 0, 0);

		sleep_ms(10);

		pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
		pio_cfg.pin_dp = USB_HOST_DP_GPIO;
		pio_cfg.pinout = PIO_USB_PINOUT_DPDM;

		push_debug_event(101, USB_HOST_DP_GPIO, PIO_USB_PINOUT_DPDM, 0);

		tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
		g_host_configured = true;
		push_debug_event(102, 0, 0, 0);

		push_debug_event(103, 0, 0, 0);
		tuh_init(1);
		push_debug_event(104, 0, 0, 0);

		g_host_started = true;
		push_debug_event(USB_DEBUG_EVENT_HOST_STARTED, 0, 0, 0);

		while (true)
		{
			tuh_task();
		}
	}
}

void usb_gamepad_host_init()
{
	g_core1_entered = false;
	g_host_configured = false;
	g_host_started = false;

	gpio_init(USB_HOST_VBUS_EN_GPIO);
	gpio_set_dir(USB_HOST_VBUS_EN_GPIO, GPIO_OUT);
	gpio_put(USB_HOST_VBUS_EN_GPIO, 1);

	std::memset(g_slots, 0, sizeof(g_slots));
	std::memset(g_hid_info, 0, sizeof(g_hid_info));

	g_host_started = false;
	g_mount_count = 0;
	g_umount_count = 0;
	g_hid_mount_count = 0;
	g_hid_umount_count = 0;
	g_report_count = 0;
	g_receive_request_fail_count = 0;
	g_last_event_id = USB_DEBUG_EVENT_NONE;
	g_last_event_data0 = 0;
	g_last_event_data1 = 0;
	g_last_event_data2 = 0;
	g_last_event_seq = 0;
	g_last_consumed_event_seq = 0;

	multicore_reset_core1();
	sleep_ms(USB_HOST_POWER_ON_DELAY_MS);
	multicore_launch_core1(core1_usb_host_main);
}

std::size_t usb_gamepad_host_count()
{
	std::size_t count = 0;

	for (std::size_t i = 0; i < USB_HOST_MAX_GAMEPADS; ++i)
	{
		if (g_slots[i].in_use)
		{
			++count;
		}
	}

	return count;
}

bool usb_gamepad_host_get_state(std::size_t index, UsbGamepadState &out_state)
{
	if (index >= USB_HOST_MAX_GAMEPADS)
	{
		return false;
	}

	if (!g_slots[index].in_use && !g_slots[index].state.updated)
	{
		return false;
	}

	out_state = g_slots[index].state;
	g_slots[index].state.updated = false;
	return true;
}

void usb_gamepad_host_dump_changes()
{
	// main.cpp 側で debug event を出すので、ここは空でもよい
}

UsbHostDebugSnapshot usb_gamepad_host_get_debug_snapshot()
{
	UsbHostDebugSnapshot snapshot{};
	snapshot.core1_entered = g_core1_entered;
	snapshot.host_configured = g_host_configured;
	snapshot.host_started = g_host_started;
	snapshot.mount_count = g_mount_count;
	snapshot.umount_count = g_umount_count;
	snapshot.hid_mount_count = g_hid_mount_count;
	snapshot.hid_umount_count = g_hid_umount_count;
	snapshot.report_count = g_report_count;
	snapshot.receive_request_fail_count = g_receive_request_fail_count;
	snapshot.last_event_id = g_last_event_id;
	snapshot.last_event_data0 = g_last_event_data0;
	snapshot.last_event_data1 = g_last_event_data1;
	snapshot.last_event_data2 = g_last_event_data2;
	return snapshot;
}

bool usb_gamepad_host_consume_debug_event(
	uint32_t &event_id,
	uint32_t &data0,
	uint32_t &data1,
	uint32_t &data2)
{
	uint32_t seq = g_last_event_seq;
	if (seq == g_last_consumed_event_seq)
	{
		return false;
	}

	event_id = g_last_event_id;
	data0 = g_last_event_data0;
	data1 = g_last_event_data1;
	data2 = g_last_event_data2;
	g_last_consumed_event_seq = seq;
	return true;
}

extern "C"
{
	void tuh_mount_cb(uint8_t dev_addr)
	{
		uint16_t vid = 0;
		uint16_t pid = 0;
		tuh_vid_pid_get(dev_addr, &vid, &pid);

		++g_mount_count;
		push_debug_event(
			USB_DEBUG_EVENT_DEVICE_MOUNT,
			dev_addr,
			((uint32_t)vid << 16) | pid,
			0);
	}

	void tuh_umount_cb(uint8_t dev_addr)
	{
		++g_umount_count;
		push_debug_event(USB_DEBUG_EVENT_DEVICE_UMOUNT, dev_addr, 0, 0);
	}

	void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
	{
		++g_hid_mount_count;

		if (instance < CFG_TUH_HID)
		{
			std::memset(&g_hid_info[instance], 0, sizeof(g_hid_info[instance]));
			g_hid_info[instance].report_count = tuh_hid_parse_report_descriptor(
				g_hid_info[instance].report_info,
				MAX_REPORT_INFO,
				desc_report,
				desc_len);
		}

		int slot = allocate_slot(dev_addr, instance);
		if (slot >= 0)
		{
			UsbGamepadState &s = g_slots[slot].state;
			tuh_vid_pid_get(dev_addr, &s.vid, &s.pid);
		}

		push_debug_event(
			USB_DEBUG_EVENT_HID_MOUNT,
			((uint32_t)dev_addr << 16) | instance,
			tuh_hid_interface_protocol(dev_addr, instance),
			desc_len);

		if (!tuh_hid_receive_report(dev_addr, instance))
		{
			++g_receive_request_fail_count;
			push_debug_event(
				USB_DEBUG_EVENT_REPORT_REQUEST_FAIL,
				((uint32_t)dev_addr << 16) | instance,
				1,
				0);
		}
	}

	void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
	{
		++g_hid_umount_count;
		release_slot(dev_addr, instance);

		push_debug_event(
			USB_DEBUG_EVENT_HID_UMOUNT,
			((uint32_t)dev_addr << 16) | instance,
			0,
			0);
	}

	void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
	{
		++g_report_count;

		uint8_t const protocol = tuh_hid_interface_protocol(dev_addr, instance);

		push_debug_event(
			USB_DEBUG_EVENT_REPORT,
			((uint32_t)dev_addr << 16) | instance,
			((uint32_t)protocol << 16) | len,
			report[0]);

		if (protocol != HID_ITF_PROTOCOL_KEYBOARD && protocol != HID_ITF_PROTOCOL_MOUSE)
		{
			uint16_t payload_len = len;
			const tuh_hid_report_info_t *info = find_report_info(instance, report, payload_len);

			if (info && is_gamepad_usage(info->usage_page, info->usage))
			{
				int slot = allocate_slot(dev_addr, instance);
				if (slot >= 0)
				{
					UsbGamepadState &s = g_slots[slot].state;
					s.connected = true;
					s.mounted = true;
					s.updated = true;
					s.dev_addr = dev_addr;
					s.instance = instance;
					s.report_id = info->report_id;
					s.usage_page = info->usage_page;
					s.usage = info->usage;
					s.report_len = payload_len > USB_HOST_MAX_REPORT_BYTES ? USB_HOST_MAX_REPORT_BYTES : payload_len;
					std::memcpy(s.report, info->report_id == 0 ? report : (report + 1), s.report_len);
					g_slots[slot].changed = true;
				}
			}
		}

		if (!tuh_hid_receive_report(dev_addr, instance))
		{
			++g_receive_request_fail_count;
			push_debug_event(
				USB_DEBUG_EVENT_REPORT_REQUEST_FAIL,
				((uint32_t)dev_addr << 16) | instance,
				2,
				0);
		}
	}

	void tud_cdc_rx_cb(uint8_t itf)
	{
		(void)itf;

		char buf[64];
		uint32_t count = tud_cdc_read(buf, sizeof(buf));
		if (count > 0)
		{
			tud_cdc_write(buf, count);
			tud_cdc_write_flush();
		}
	}
}