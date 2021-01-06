/*
 * ÷»ÄïÍøÂç
 * Copyright (C) 2019 String.Empty
 */
#pragma once

class FromMsg;

namespace Cloud
{
	void heartbeat();
	int checkWarning(const char* warning);
	int DownloadFile(const char* url, const char* downloadPath);
	int checkUpdate(FromMsg* msg);
}
