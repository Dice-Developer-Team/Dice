/*
 * Copyright (C) 2019 String.Empty
 * 消息频率监听
 */
#pragma once
#include <map>
#include <ctime>
#include "DiceConsole.h"
class FrqMonitor {
public:
	static const long long earlyTime = 30;
	static const long long earlierTime = 60;
	static const long long earliestTime = 300;
	//频率记录
	static std::map<long long, int>mFrequence;
	static std::map<long long, int>mWarnLevel;
	long long fromQQ = 0;
	time_t fromTime = 0;
	FrqMonitor(long long QQ,time_t TT,chatType CT): fromQQ(QQ),fromTime(TT){
		if (mFrequence.count(fromQQ)) {
			mFrequence[fromQQ] += 10;
			if (mFrequence[fromQQ] > 50 && mWarnLevel[fromQQ] < 50) {
				mWarnLevel[fromQQ] = mFrequence[fromQQ];
				const std::string strMsg = "提醒：\n于" + printChat(CT) + "监测到" + printQQ(fromQQ) + "指令频度达到" + std::to_string(mFrequence[fromQQ] / 10);
				AddMsgToQueue(GlobalMsg["strSpamFirstWarning"], CT.first, CT.second);
				sendAdmin(strMsg);
			}
			else if (mFrequence[fromQQ] > 100 && mWarnLevel[fromQQ] < 100) {
				mWarnLevel[fromQQ] = mFrequence[fromQQ]; 
				const std::string strMsg = "警告：\n于" + printChat(CT) + "监测到" + printQQ(fromQQ) + "指令频度达到" + std::to_string(mFrequence[fromQQ] / 10);
				AddMsgToQueue(GlobalMsg["strSpamFinalWarning"], CT.first, CT.second);
				NotifyMonitor(strMsg);
			}
			else if (mFrequence[fromQQ] > 200 && mWarnLevel[fromQQ] < 200) {
				mWarnLevel[fromQQ] = mFrequence[fromQQ];
				std::string strNow = printSTime(stNow);
				std::string strNote = strNow + " 于" + printChat(CT) + "监测到" + printQQ(fromQQ) + "对" + GlobalMsg["strSelfName"] + "30s发送指令频度达" + std::to_string(mFrequence[fromQQ] / 10);
				while (strNote.find('\"') != std::string::npos)strNote.replace(strNote.find('\"'), 1, "\'"); 
				std::string strWarning = "!warning{\n\"fromGroup\":0,\n\"type\":\"spam\",\n\"fromQQ\":" + std::to_string(fromQQ) + ",\n\"time\":\"" + strNow + "\",\n\"DiceMaid\":" + std::to_string(CQ::getLoginQQ()) + ",\n\"masterQQ\":" + std::to_string(masterQQ) + ",\n\"note\":\"" + strNote + "\"\n}";
				if (AdminQQ.count(fromQQ) || mDiceList.count(fromQQ)) {
					NotifyMonitor(strNote);
				}
				else {
					NotifyMonitor(strWarning);
					addBlackQQ(fromQQ, strNote, strWarning);
				}
			}
		}
		else {
			mFrequence[fromQQ] = 10;
			mWarnLevel[fromQQ] = 0;
		}
	}
	~FrqMonitor() {
		if (mWarnLevel[fromQQ])mWarnLevel[fromQQ] -= 10;
	}
	bool isEarliest() {
		if (time(NULL) - fromTime > earliestTime) {
			mFrequence[fromQQ]--;
			delete this;
			return true;
		}
		else return false;
	}
	bool isEarlier() {
		if (time(NULL) - fromTime > earlierTime) {
			mFrequence[fromQQ] -= 4;
			return true;
		}
		else return false;
	}
	bool isEarly() {
		if (time(NULL) - fromTime > earlyTime) {
			mFrequence[fromQQ] -= 5;
			return true;
		}
		else return false;
	}
};
void AddFrq(const long long QQ, time_t TT, chatType CT);
void frqHandler();