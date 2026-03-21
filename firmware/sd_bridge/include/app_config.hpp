#pragma once

constexpr unsigned int START_BUTTON_GPIO = 14;
constexpr unsigned int SELECT_BUTTON_GPIO = 15;

constexpr bool BUTTON_ACTIVE_LEVEL = false;

constexpr unsigned int SELECT_DEBOUNCE_MS = 30;
constexpr unsigned int SELECT_REPEAT_GUARD_MS = 300;

constexpr unsigned int USB_HOST_DP_GPIO = 16;
constexpr unsigned int USB_HOST_DM_GPIO = 17;
constexpr unsigned int USB_HOST_VBUS_EN_GPIO = 18;

constexpr unsigned int USB_HOST_POWER_ON_DELAY_MS = 100;
constexpr unsigned int USB_HOST_MAX_GAMEPADS = 2;
constexpr unsigned int USB_HOST_MAX_REPORT_BYTES = 64;