#pragma once

/*
 * ÷»ÄïÍøÂç
 * Copyright (C) 2019 String.Empty
 */

#include <filesystem>

class FromMsg;

namespace Cloud
{
	void heartbeat();
	int checkWarning(const char* warning);
	[[deprecated]] int DownloadFile(const char* url, const char* downloadPath);
	int DownloadFile(const char* url, const std::filesystem::path& downloadPath);
	int checkUpdate(FromMsg* msg);
}
