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

void AddFrq(long long QQ, time_t TT, chatType CT)
{
	std::lock_guard<std::mutex> lock_queue(FrqMutex);
	if (setFrq.count(QQ)) return;
	setFrq.insert(QQ);
	auto* newFrq = new FrqMonitor(QQ, TT, CT);
	EarlyMsgQueue.push(newFrq);
	if (QQ) {
		FrqMonitor::sumFrqTotal++;
		today->inc("frq");
		today->inc(QQ, "frq");
	}
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


FrqMonitor::FrqMonitor(long long QQ, time_t TT, chatType CT) : fromQQ(QQ), fromTime(TT) {
	if (mFrequence.count(fromQQ)) {
		mFrequence[fromQQ] += 10;
		mCntOrder[fromQQ] += 1;
		if ((!console["ListenSpam"] || trustedQQ(fromQQ) > 1) && QQ!=console.DiceMaid)return;
		if (mFrequence[fromQQ] > 60 && mWarnLevel[fromQQ] < 60 && QQ) {
			mWarnLevel[fromQQ] = mFrequence[fromQQ];
			const std::string strMsg = "提醒：\n" + (CT.second != msgtype::Private ? printChat(CT) : "私聊窗口") +
				"监测到" + printQQ(fromQQ) + "高频发送指令达" + to_string(mCntOrder[fromQQ])
				+ (mCntOrder[fromQQ] > 18 ? "/5min"
				   : (mCntOrder[fromQQ] > 8 ? "/min" : "/30s"));
			if(QQ!=console.DiceMaid)AddMsgToQueue(getMsg("strSpamFirstWarning"), CT);
			console.log(strMsg, 1, printSTNow());
		}
		else if (mFrequence[fromQQ] > 120 && mWarnLevel[fromQQ] < 120 && QQ) {
			mWarnLevel[fromQQ] = mFrequence[fromQQ];
			const std::string strMsg = "警告：\n" + (CT.second != msgtype::Private ? printChat(CT) : "私聊窗口") +
				printQQ(fromQQ) + "高频发送指令达" + to_string(mCntOrder[fromQQ])
				+ (mCntOrder[fromQQ] > 36 ? "/5min"
				   : (mCntOrder[fromQQ] > 15 ? "/min" : "/30s"));
			if (QQ!=console.DiceMaid)AddMsgToQueue(getMsg("strSpamFinalWarning"), CT);
			console.log(strMsg, 0b10, printSTNow());
		}
		else if (mFrequence[fromQQ] > 200 && mWarnLevel[fromQQ] < 200) {
			mWarnLevel[fromQQ] = mFrequence[fromQQ];
			std::string strNow = printSTNow();
			std::string strFrq = to_string(mCntOrder[fromQQ])
				+ (mCntOrder[fromQQ] > 60 ? "/5min"
				   : (mCntOrder[fromQQ] > 25 ? "/min" : "/30s"));
			if (!QQ) {
				console.log("警告：" + GlobalMsg["strSelfName"] + "高频处理虚拟指令达" + strFrq, 0b1000, strNow);
				return;
			}
			std::string strNote = (CT.second != msgtype::Private ? printChat(CT) : "私聊窗口") + "监测到" +
				printQQ(fromQQ) + "对" + printQQ(console.DiceMaid) + "高频发送指令达" + strFrq;
			if (QQ==console.DiceMaid) {
				console.set("ListenSelfEcho", 0);
				console.set("ListenGroupEcho", 0);
				console.log(strNote + "\n已强制停止接收回音", 0b1000, strNow);
			}
			else if (DD::getDiceSisters().count(fromQQ)) {
				console.log(strNote, 0b1000, strNow);
			}
			else {
				DDBlackMarkFactory mark{ fromQQ, 0 };
				mark.sign().type("spam").time(strNow).note(strNow + " " + strNote);
				blacklist->create(mark.product());
			}
		}
	}
	else {
		mFrequence[fromQQ] = 10;
		mWarnLevel[fromQQ] = 0;
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