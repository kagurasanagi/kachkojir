#pragma once

#include <cstddef>
#include <cstdint>

struct UsbGamepadState
{
	bool connected;
	bool mounted;
	bool updated;
	uint8_t dev_addr;
	uint8_t instance;
	uint16_t vid;
	uint16_t pid;
	uint8_t report_id;
	uint16_t usage_page;
	uint16_t usage;
	uint16_t report_len;
	uint8_t report[64];
};

struct UsbHostDebugSnapshot
{
	bool core1_entered;
	bool host_configured;
	bool host_started;
	uint32_t mount_count;
	uint32_t umount_count;
	uint32_t hid_mount_count;
	uint32_t hid_umount_count;
	uint32_t report_count;
	uint32_t receive_request_fail_count;
	uint32_t last_event_id;
	uint32_t last_event_data0;
	uint32_t last_event_data1;
	uint32_t last_event_data2;
};

enum UsbHostDebugEventType : uint32_t
{
	USB_DEBUG_EVENT_NONE = 0,
	USB_DEBUG_EVENT_HOST_STARTED = 1,
	USB_DEBUG_EVENT_DEVICE_MOUNT = 2,
	USB_DEBUG_EVENT_DEVICE_UMOUNT = 3,
	USB_DEBUG_EVENT_HID_MOUNT = 4,
	USB_DEBUG_EVENT_HID_UMOUNT = 5,
	USB_DEBUG_EVENT_REPORT = 6,
	USB_DEBUG_EVENT_REPORT_REQUEST_FAIL = 7,
};

void usb_gamepad_host_init();
void usb_gamepad_host_dump_changes();
bool usb_gamepad_host_get_state(std::size_t index, UsbGamepadState &out_state);
std::size_t usb_gamepad_host_count();

UsbHostDebugSnapshot usb_gamepad_host_get_debug_snapshot();
bool usb_gamepad_host_consume_debug_event(
	uint32_t &event_id,
	uint32_t &data0,
	uint32_t &data1,
	uint32_t &data2);
