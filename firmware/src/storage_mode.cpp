#include "storage_mode.hpp"

namespace
{
	StorageMode g_storage_mode = StorageMode::SpiFatFs;
}

void storage_set_mode(StorageMode mode)
{
	g_storage_mode = mode;
}

StorageMode storage_get_mode()
{
	return g_storage_mode;
}