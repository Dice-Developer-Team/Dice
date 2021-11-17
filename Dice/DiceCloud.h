#pragma once

/*
 * ÷»ÄïÍøÂç
 * Copyright (C) 2019-2021 String.Empty
 */

#include "filesystem.hpp"

class FromMsg;

namespace Cloud
{
	void heartbeat();
	[[deprecated]] int DownloadFile(const char* url, const char* downloadPath);
	int DownloadFile(const char* url, const std::filesystem::path& downloadPath);
	int checkUpdate(FromMsg* msg);
}
