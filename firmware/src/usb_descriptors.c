#include <string.h>
#include "tusb.h"

extern void debug_led_25(bool on);

static bool g_usb_msc_mode = false;

void usb_descriptors_set_msc_mode(bool enabled)
{
	g_usb_msc_mode = enabled;
}

bool usb_descriptors_is_msc_mode(void)
{
	return g_usb_msc_mode;
}

#define USB_VID 0xCafe
#define USB_BCD 0x0200

// Windows が CDC/MSC を別物として認識できるよう固定で分ける
#define USB_PID_CDC 0x4001
#define USB_PID_MSC 0x4010

static tusb_desc_device_t const desc_device_cdc =
	{
		.bLength = sizeof(tusb_desc_device_t),
		.bDescriptorType = TUSB_DESC_DEVICE,
		.bcdUSB = USB_BCD,
		.bDeviceClass = TUSB_CLASS_MISC,
		.bDeviceSubClass = MISC_SUBCLASS_COMMON,
		.bDeviceProtocol = MISC_PROTOCOL_IAD,
		.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
		.idVendor = USB_VID,
		.idProduct = USB_PID_CDC,
		.bcdDevice = 0x0100,
		.iManufacturer = 0x01,
		.iProduct = 0x02,
		.iSerialNumber = 0x03,
		.bNumConfigurations = 0x01};

static tusb_desc_device_t const desc_device_msc =
	{
		.bLength = sizeof(tusb_desc_device_t),
		.bDescriptorType = TUSB_DESC_DEVICE,
		.bcdUSB = USB_BCD,
		.bDeviceClass = 0x00,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
		.idVendor = USB_VID,
		.idProduct = USB_PID_MSC,
		.bcdDevice = 0x0101,
		.iManufacturer = 0x01,
		.iProduct = 0x02,
		.iSerialNumber = 0x03,
		.bNumConfigurations = 0x01};

uint8_t const *tud_descriptor_device_cb(void)
{
	debug_led_25(true);
	return (uint8_t const *)(g_usb_msc_mode ? &desc_device_msc : &desc_device_cdc);
}

enum
{
	ITF_NUM_CDC = 0,
	ITF_NUM_CDC_DATA,
	ITF_NUM_CDC_TOTAL
};

enum
{
	ITF_NUM_MSC = 0,
	ITF_NUM_MSC_TOTAL
};

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82

#define EPNUM_MSC_OUT 0x01
#define EPNUM_MSC_IN 0x81

#define CONFIG_TOTAL_LEN_CDC (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)
#define CONFIG_TOTAL_LEN_MSC (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

uint8_t const desc_fs_configuration_cdc[] =
	{
		TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_CDC_TOTAL, 0, CONFIG_TOTAL_LEN_CDC, 0x00, 100),
		TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

uint8_t const desc_fs_configuration_msc[] =
	{
		TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_MSC_TOTAL, 0, CONFIG_TOTAL_LEN_MSC, 0x00, 100),
		TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 4, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
	(void)index;
	debug_led_25(true);
	return g_usb_msc_mode ? desc_fs_configuration_msc : desc_fs_configuration_cdc;
}

char const *string_desc_arr_cdc[] =
	{
		(const char[]){0x09, 0x04},
		"Kachkojir",
		"Kachkojir Dual USB",
		"000000000001",
		"Kachkojir CDC",
};

char const *string_desc_arr_msc[] =
	{
		(const char[]){0x09, 0x04},
		"Kachkojir",
		"Kachkojir SD MSC",
		"000000000002",
		"Kachkojir MSC",
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	(void)langid;
	debug_led_25(true);

	char const **table = g_usb_msc_mode ? string_desc_arr_msc : string_desc_arr_cdc;
	uint8_t count = g_usb_msc_mode
						? (uint8_t)(sizeof(string_desc_arr_msc) / sizeof(string_desc_arr_msc[0]))
						: (uint8_t)(sizeof(string_desc_arr_cdc) / sizeof(string_desc_arr_cdc[0]));

	uint8_t chr_count;

	if (index == 0)
	{
		memcpy(&_desc_str[1], table[0], 2);
		chr_count = 1;
	}
	else
	{
		if (!(index < count))
		{
			return NULL;
		}

		const char *str = table[index];
		chr_count = (uint8_t)strlen(str);

		if (chr_count > 31)
		{
			chr_count = 31;
		}

		for (uint8_t i = 0; i < chr_count; i++)
		{
			_desc_str[1 + i] = str[i];
		}
	}

	_desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
	return _desc_str;
}