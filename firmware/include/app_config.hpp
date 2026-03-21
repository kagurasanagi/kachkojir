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

// boot / mode select
constexpr unsigned int BOOT_MODE_PROBE_MS = 800;
constexpr unsigned int BOOT_MODE_POLL_MS = 10;
constexpr uint8_t GAMEPAD_SELECT_MASK = 0x01;

// msc mode
constexpr unsigned int MSC_REBOOT_DELAY_MS = 100;

// sd spi mode pins (通常モード用)
// 必要に応じて基板配線に合わせて変更
constexpr unsigned int SD_SPI_SCK_GPIO = 10;
constexpr unsigned int SD_SPI_MOSI_GPIO = 11;
constexpr unsigned int SD_SPI_MISO_GPIO = 12;
constexpr unsigned int SD_SPI_CS_GPIO = 13;
constexpr unsigned int SD_SPI_BAUD_RATE = 12 * 1000 * 1000;