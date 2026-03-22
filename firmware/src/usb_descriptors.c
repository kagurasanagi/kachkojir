#include <stdbool.h>
#include <stdint.h>
#include "tusb.h"

static bool g_usb_msc_mode = false;

void usb_descriptors_set_msc_mode(bool enabled)
{
	g_usb_msc_mode = enabled;
}

bool usb_descriptors_is_msc_mode(void)
{
	return g_usb_msc_mode;
}

uint8_t const *usb_descriptors_normal_device_cb(void);
uint8_t const *usb_descriptors_normal_configuration_cb(uint8_t index);
uint16_t const *usb_descriptors_normal_string_cb(uint8_t index, uint16_t langid);

uint8_t const *usb_descriptors_msc_sample_device_cb(void);
uint8_t const *usb_descriptors_msc_sample_configuration_cb(uint8_t index);
uint16_t const *usb_descriptors_msc_sample_string_cb(uint8_t index, uint16_t langid);

uint8_t const *tud_descriptor_device_cb(void)
{
	return g_usb_msc_mode
			   ? usb_descriptors_msc_sample_device_cb()
			   : usb_descriptors_normal_device_cb();
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
	return g_usb_msc_mode
			   ? usb_descriptors_msc_sample_configuration_cb(index)
			   : usb_descriptors_normal_configuration_cb(index);
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	return g_usb_msc_mode
			   ? usb_descriptors_msc_sample_string_cb(index, langid)
			   : usb_descriptors_normal_string_cb(index, langid);
}