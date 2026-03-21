#pragma once

enum class BootMode
{
	Normal,
	MassStorage
};

BootMode detect_boot_mode();
bool is_host_connected_to_pc();