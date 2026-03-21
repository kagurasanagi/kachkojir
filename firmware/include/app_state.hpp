#pragma once

enum class SdOwner
{
	Pico = 0,
	Pc
};

extern volatile SdOwner g_sd_owner;