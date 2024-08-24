/*
 * Copyright (C) 2019 String.Empty
 * 消息频率监听
 */
#pragma once
#include <map>
#include <ctime>
#include <atomic>
#include "DiceConsole.h"
#include "ManagerSystem.h"
#include "GlobalVar.h"
#include "BlackListManager.h"
#include "DiceEvent.h"

class DiceJobDetail;
class FrqMonitor
{
public:
	static const long long earlyTime = 30;
	static const long long earlierTime = 60;
	static const long long earliestTime = 300;
	//频率记录
	static std::unordered_map<long long, int>mFrequence;
	static std::unordered_map<long long, int>mWarnLevel;
	static std::unordered_map<long long, int>mCntOrder;
	static std::atomic<unsigned int> sumFrqTotal;
	static int getFrqTotal();
	long long fromUID = 0;
	time_t fromTime = 0;

	FrqMonitor(DiceEvent&);

	~FrqMonitor()
	{
		if (mWarnLevel[fromUID])mWarnLevel[fromUID] -= 10;
	}

	bool isEarliest()
	{
		if (time(nullptr) - fromTime > earliestTime)
		{
			mFrequence[fromUID] -= 2;
			mCntOrder[fromUID] -= 1;
			delete this;
			return true;
		}
		return false;
	}

	bool isEarlier()
	{
		if (time(nullptr) - fromTime > earlierTime)
		{
			mFrequence[fromUID] -= 4;
			return true;
		}
		return false;
	}

	bool isEarly()
	{
		if (time(nullptr) - fromTime > earlyTime)
		{
			mFrequence[fromUID] -= 4;
			return true;
		}
		return false;
	}
};

void AddFrq(DiceEvent& msg);
void frqHandler();
