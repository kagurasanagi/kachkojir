set(SD_LIB_ROOT ${CMAKE_CURRENT_LIST_DIR}/../external/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)
set(PICO_PIO_USB_DIR ${CMAKE_CURRENT_LIST_DIR}/../external/Pico-PIO-USB)

add_subdirectory(${SD_LIB_ROOT}/src ${CMAKE_CURRENT_BINARY_DIR}/noosfat_build)

if(NOT EXISTS ${PICO_PIO_USB_DIR}/src/pio_usb.h)
	message(FATAL_ERROR "Pico-PIO-USB is missing. Please add external/Pico-PIO-USB")
endif()

add_subdirectory(${PICO_PIO_USB_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pico_pio_usb_build)

add_executable(kachkojir
	src/main.cpp
	src/hw_config.cpp
	src/usb_gamepad_host.cpp
	src/usb_descriptors.c
	src/usb_descriptors_normal.c
	src/usb_descriptors_msc_sample.c
	src/boot_mode.cpp
	src/storage_mode.cpp
	src/usb_msc_mode.cpp
	src/debug_leds.cpp
	${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
	${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_include_directories(kachkojir PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_definitions(kachkojir PRIVATE
	USE_PRINTF
	USE_DBG_PRINTF
	PIO_USB_USE_TINYUSB
	PICO_STDIO_USB=0
	PICO_STDIO_UART=0
)

target_link_libraries(kachkojir PRIVATE
	pico_stdlib
	pico_multicore
	hardware_clocks
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
	pico_pio_usb
	tinyusb_device
	tinyusb_host
)

pico_enable_stdio_usb(kachkojir 0)
pico_enable_stdio_uart(kachkojir 0)

pico_add_extra_outputs(kachkojir)



