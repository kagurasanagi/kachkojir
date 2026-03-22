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










set(USB_MASS_STORAGE_SAMPLE_DIR
	${CMAKE_CURRENT_LIST_DIR}/../external/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/examples/usb_mass_storage
)

add_executable(usb_mass_storage_sample_port
	src/usb_mass_storage_sample_main.c
	src/usb_mass_storage_sample_msc_disk.c
	src/usb_mass_storage_sample_usb_descriptors.c
	src/usb_mass_storage_sample_hw_config.c
)

target_include_directories(usb_mass_storage_sample_port PRIVATE
	${USB_MASS_STORAGE_SAMPLE_DIR}
	${USB_MASS_STORAGE_SAMPLE_DIR}/include
)

target_compile_definitions(usb_mass_storage_sample_port PUBLIC
	USE_PRINTF
	USE_DBG_PRINTF
	CFG_TUSB_MCU=OPT_MCU_RP2040
	BOARD_TUD_RHPORT=0
	BOARD_TUD_MAX_SPEED=OPT_MODE_FULL_SPEED
)

target_link_libraries(usb_mass_storage_sample_port PRIVATE
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
	pico_stdlib
	pico_pio_usb
	tinyusb_additions
	tinyusb_board
	tinyusb_device
)

pico_enable_stdio_usb(usb_mass_storage_sample_port 1)
pico_enable_stdio_uart(usb_mass_storage_sample_port 0)

pico_add_extra_outputs(usb_mass_storage_sample_port)