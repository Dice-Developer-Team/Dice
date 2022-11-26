/*
 * Copyright (C) 2019 String.Empty
 * 消息频率监听
 */
#include <set>
#include <queue>
#include <mutex>
#include "MsgMonitor.h"
#include "DiceSchedule.h"
#include "DDAPI.h"

std::atomic<unsigned int> FrqMonitor::sumFrqTotal = 0;
std::map<long long, int> FrqMonitor::mFrequence = {};
std::map<long long, int> FrqMonitor::mCntOrder = {};
std::map<long long, int> FrqMonitor::mWarnLevel = {};

std::queue<FrqMonitor*> EarlyMsgQueue;
std::queue<FrqMonitor*> EarlierMsgQueue;
std::queue<FrqMonitor*> EarliestMsgQueue;

std::set<long long> setFrq;
std::mutex FrqMutex;

void AddFrq(DiceEvent& msg)
{
	std::lock_guard<std::mutex> lock_queue(FrqMutex);
	if (!msg.fromChat.uid || setFrq.count(msg.fromChat.uid)) return;
	setFrq.insert(msg.fromChat.uid);
	auto* newFrq = new FrqMonitor(msg);
	EarlyMsgQueue.push(newFrq);
	FrqMonitor::sumFrqTotal++;
	today->inc("frq");
	today->set(msg.fromChat.uid, "frq", today->get(msg.fromChat.uid).get_int("frq") + 1);
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


FrqMonitor::FrqMonitor(DiceEvent& msg) : fromUID(msg.fromChat.uid), fromTime(msg.get_ll("time")) {
	if (mFrequence.count(fromUID)) {
		mFrequence[fromUID] += 10;
		mCntOrder[fromUID] += 1;
		if ((!console["ListenSpam"] || trustedQQ(fromUID) > 1) && fromUID != console.DiceMaid)return;
		if (mFrequence[fromUID] > 60 && mWarnLevel[fromUID] < 60 && fromUID) {
			mWarnLevel[fromUID] = mFrequence[fromUID];
			const std::string strMsg = "提醒：\n" + (msg.fromChat.gid ? printChat(msg.fromChat) : "私聊窗口") +
				"监测到" + printUser(fromUID) + "高频发送指令达" + to_string(mCntOrder[fromUID])
				+ (mCntOrder[fromUID] > 18 ? "/5min"
				   : (mCntOrder[fromUID] > 8 ? "/min" : "/30s"))
				+ "\n最近指令: " + msg.get_str("fromMsg");
			if (fromUID != console.DiceMaid)AddMsgToQueue(getMsg("strSpamFirstWarning",msg), msg.fromChat);
			console.log(strMsg, 1, printSTNow());
		}
		else if (mFrequence[fromUID] > 120 && mWarnLevel[fromUID] < 120 && fromUID) {
			mWarnLevel[fromUID] = mFrequence[fromUID];
			const std::string strMsg = "警告：\n" + (msg.fromChat.gid ? printChat(msg.fromChat) : "私聊窗口") +
				printUser(fromUID) + "高频发送指令达" + to_string(mCntOrder[fromUID])
				+ (mCntOrder[fromUID] > 36 ? "/5min"
				   : (mCntOrder[fromUID] > 15 ? "/min" : "/30s"))
				+ "\n最近指令: " + msg.get_str("fromMsg");
			if (fromUID != console.DiceMaid)AddMsgToQueue(getMsg("strSpamFinalWarning", msg), msg.fromChat);
			console.log(strMsg, 0b10, printSTNow());
		}
		else if (mFrequence[fromUID] > 200 && mWarnLevel[fromUID] < 200) {
			mWarnLevel[fromUID] = mFrequence[fromUID];
			std::string strNow = printSTNow();
			std::string strFrq = to_string(mCntOrder[fromUID])
				+ (mCntOrder[fromUID] > 60 ? "/5min"
				   : (mCntOrder[fromUID] > 25 ? "/min" : "/30s"));
			if (!fromUID) {
				console.log("警告：" + getMsg("strSelfName") + "高频处理虚拟指令达" + strFrq
							+ "\n最近指令: " + msg.get_str("fromMsg"), 0b1000, strNow);
				return;
			}
			std::string strNote = (msg.fromChat.gid ? printChat(msg.fromChat) : "私聊窗口") + "监测到" +
				printUser(fromUID) + "对" + printUser(console.DiceMaid) + "高频发送指令达" + strFrq
				+ "\n最近指令: " + msg.get_str("fromMsg");
			if (fromUID == console.DiceMaid) {
				console.set("ListenSelfEcho", 0);
				console.set("ListenGroupEcho", 0);
				console.log(strNote + "\n已强制停止接收回音", 0b1000, strNow);
			}
			else if (DD::getDiceSisters().count(fromUID)) {
				console.log(strNote, 0b1000, strNow);
			}
			else {
				DDBlackMarkFactory mark{ fromUID, 0 };
				mark.sign().type("spam").time(strNow).note(strNow + " " + strNote);
				blacklist->create(mark.product());
			}
		}
	}
	else {
		mFrequence[fromUID] = 10;
		mWarnLevel[fromUID] = 0;
	}
}

int FrqMonitor::getFrqTotal()
{
	return EarlyMsgQueue.size() + EarlierMsgQueue.size() / 2 + EarliestMsgQueue.size() / 10;
}

/*
EVE_Status_EX(statusFrq)
{
	if (!Enabled)
	{
		eve.data = "准备中";
		eve.color_gray();
		return;
	}
	//平滑到分钟的频度
	const int intFrq = FrqMonitor::getFrqTotal();
	//long long llDuration = time(nullptr) - llStartTime;
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
		if (intFrq < 10)
		{
			eve.color_green();
		}
		else if (intFrq < 20) 
		{
			eve.color_orange();
		}
		else
		{
			eve.color_red();
		}
	}
}
*/