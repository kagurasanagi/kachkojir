#pragma once

enum class StorageMode
{
	SpiFatFs,
	SdioRawMsc
};

void storage_set_mode(StorageMode mode);
StorageMode storage_get_mode();

// 追加
extern "C"
{
	void storage_hw_init_for_mode(StorageMode mode);
}