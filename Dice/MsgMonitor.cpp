/*
 * Copyright (C) 2019 String.Empty
 * 消息频率监听
 */
#include <set>
#include <queue>
#include <mutex>
#include "MsgMonitor.h"

std::map<long long, int> FrqMonitor::mFrequence = {};
std::map<long long, int> FrqMonitor::mWarnLevel = {};

std::queue<FrqMonitor*> EarlyMsgQueue;
std::queue<FrqMonitor*> EarlierMsgQueue;
std::queue<FrqMonitor*> EarliestMsgQueue;

std::set<long long> setFrq;
std::mutex FrqMutex;

void AddFrq(long long QQ, time_t TT, chatType CT)
{
	std::lock_guard<std::mutex> lock_queue(FrqMutex);
	if (setFrq.count(QQ)) return;
	setFrq.insert(QQ);
	FrqMonitor* newFrq = new FrqMonitor(QQ, TT, CT);
	EarlyMsgQueue.push(newFrq);
}

void frqHandler()
{
	while (Enabled)
	{
		while (!EarliestMsgQueue.empty())
		{
			std::lock_guard<std::mutex> lock_queue(FrqMutex);
			if (EarliestMsgQueue.front()->isEarliest())
			{
				EarliestMsgQueue.pop();
			}
			else break;
		}
		while (!EarlierMsgQueue.empty())
		{
			std::lock_guard<std::mutex> lock_queue(FrqMutex);
			if (EarlierMsgQueue.front()->isEarlier())
			{
				EarliestMsgQueue.push(EarlierMsgQueue.front());
				EarlierMsgQueue.pop();
			}
			else break;
		}
		while (!EarlyMsgQueue.empty())
		{
			std::lock_guard<std::mutex> lock_queue(FrqMutex);
			if (EarlyMsgQueue.front()->isEarly())
			{
				EarlierMsgQueue.push(EarlyMsgQueue.front());
				EarlyMsgQueue.pop();
			}
			else
			{
				break;
			}
		}
		setFrq.clear();
		std::this_thread::sleep_for(100ms);
	}
}

int FrqMonitor::getFrqTotal()
{
	return EarlyMsgQueue.size() + EarlierMsgQueue.size() / 2 + EarliestMsgQueue.size() / 10;
}

/*EVE_Status_EX(statusUptime) {
	//初始化以来的秒数
	long long llDuration = clock() / 1000;
	//long long llDuration = (clock() - llStartTime) / 1000;
	if (llDuration < 0) {
		eve.data = "N";
		eve.dataf = "/A";
	}
	else if (llDuration < 60 * 5) {
		eve.data = std::to_string(llDuration);
		eve.dataf = "s";
	}
	else if (llDuration < 60 * 60 * 5) {
		eve.data = std::to_string(llDuration / 60);
		eve.dataf = "min";
	}
	else if (llDuration < 60 * 60 * 24 * 5) {
		eve.data = std::to_string(llDuration / 60 / 60);
		eve.dataf = "h";
	} 
	else{
		eve.data = std::to_string(llDuration / 60 / 60 / 24);
		eve.dataf = "day";
	}
	eve.color_green();
}*/
EVE_Status_EX(statusFrq)
{
	if (!Enabled)
	{
		eve.data = "准备中";
		eve.color_gray();
		return;
	}
	//平滑到分钟的频度
	int intFrq = FrqMonitor::getFrqTotal();
	//long long llDuration = (clock() - llStartTime) / 1000;
	if (intFrq < 0)
	{
		eve.data = "N";
		eve.dataf = "/A";
		eve.color_crimson();
	}
	else
	{
		eve.data = std::to_string(intFrq);
		eve.dataf = "/min";
		if (intFrq < 60)
		{
			eve.color_green();
		}
		else if (intFrq < 120)
		{
			eve.color_orange();
		}
		else
		{
			eve.color_red();
		}
	}
}
