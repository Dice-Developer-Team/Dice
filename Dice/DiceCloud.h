/*
 * ÷»ÄïÍøÂç
 * Copyright (C) 2019 String.Empty
 */
#pragma once
#include "DiceNetwork.h"

class FromMsg;

namespace Cloud
{
	void update();
	void upWarning(const char* warning);
	int checkWarning(const char* warning);
	int DownloadFile(const char* url, const char* downloadPath);
	int checkUpdate(FromMsg* msg);
}
