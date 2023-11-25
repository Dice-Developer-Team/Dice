#pragma once
/*
 * 骰娘网络
 * Copyright (C) 2019-2023 String.Empty
 */

#include "filesystem.hpp"

class DiceEvent;

namespace Cloud
{
	[[deprecated]] int DownloadFile(const char* url, const char* downloadPath);
	int DownloadFile(const char* url, const std::filesystem::path& downloadPath);
	int checkUpdate(DiceEvent* msg);
}
