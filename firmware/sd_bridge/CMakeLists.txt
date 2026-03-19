set(SD_LIB_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../external/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)

add_subdirectory(${SD_LIB_ROOT}/src ${CMAKE_CURRENT_BINARY_DIR}/noosfat_build)

add_executable(kachkojir
	src/main.cpp
	src/hw_config.cpp
)

target_include_directories(kachkojir PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(kachkojir PRIVATE
	pico_stdlib
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

target_compile_definitions(kachkojir PRIVATE
	USE_PRINTF
	USE_DBG_PRINTF
)

pico_enable_stdio_usb(kachkojir 1)
pico_enable_stdio_uart(kachkojir 0)

pico_add_extra_outputs(kachkojir)