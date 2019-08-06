/*
 * Copyright (C) 2019 String.Empty
 * ÏûÏ¢ÆµÂÊ¼àÌý
 */
#include <set>
#include <queue>
#include <mutex>
#include "MsgMonitor.h"

std::map<long long, int> FrqMonitor::mFrequence = {};
std::map<long long, int> FrqMonitor::mWarnLevel = {};

std::queue<FrqMonitor*>EarlyMsgQueue;
std::queue<FrqMonitor*>EarlierMsgQueue;
std::queue<FrqMonitor*>EarliestMsgQueue;

std::set<long long>setFrq;
std::mutex FrqMutex;
void AddFrq(long long QQ, time_t TT, chatType CT)
{
	std::lock_guard<std::mutex> lock_queue(FrqMutex);
	if (setFrq.count(QQ)) return;
	setFrq.insert(QQ);
	FrqMonitor *newFrq = new FrqMonitor(QQ, TT, CT);
	EarlyMsgQueue.push(newFrq);
}
void frqHandler() {
	while (Enabled)
	{
		while (!EarliestMsgQueue.empty()) {
			std::lock_guard<std::mutex> lock_queue(FrqMutex);
			if(EarliestMsgQueue.front()->isEarliest()){
				EarliestMsgQueue.pop();
			}
			else break;
		}
		while (!EarlierMsgQueue.empty()) {
			std::lock_guard<std::mutex> lock_queue(FrqMutex);
			if (EarlierMsgQueue.front()->isEarlier()) {
				EarliestMsgQueue.push(EarlierMsgQueue.front());
				EarlierMsgQueue.pop();
			}
			else break;
		}
		while (!EarlyMsgQueue.empty()) {
			std::lock_guard<std::mutex> lock_queue(FrqMutex);
			if (EarlyMsgQueue.front()->isEarly()) {
				EarlierMsgQueue.push(EarlyMsgQueue.front());
				EarlyMsgQueue.pop();
			}
			else {
				break;
			}
		}
		setFrq.clear();
		Sleep(200);
	}
}