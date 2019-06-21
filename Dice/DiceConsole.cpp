/*
 * _______   ________  ________  ________  __
 * |  __ \  |__  __| |  _____| |  _____| | |
 * | | | |   | |   | |    | |_____  | |
 * | | | |   | |   | |    |  _____| |__|
 * | |__| |  __| |__  | |_____  | |_____  __
 * |_______/  |________| |________| |________| |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123溯洄
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <ctime>
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "MsgFormat.h"
#include "NameStorage.h"

using namespace std;
using namespace CQ;

	long long masterQQ = 0;
set<long long> AdminQQ = {};
set<chatType> MonitorList = {};
	//全局静默
	bool boolDisabledGlobal = false;
	//全局禁用.me
	bool boolDisabledMeGlobal = false;
	//全局禁用.jrrp
	bool boolDisabledJrrpGlobal = false;
	//私用模式
	bool boolPreserve = false;
	//禁用讨论组
	bool boolNoDiscuss = false;
	//讨论组消息记录
	std::map<long long, time_t> DiscussList;
//群邀请者
std::map<long long, long long> mGroupInviter;
	//个性化语句
	std::map<std::string, std::string> PersonalMsg;
	//botoff的群
	std::set<long long> DisabledGroup;
	//botoff的讨论组
	std::set<long long> DisabledDiscuss;
	//白名单群：私用模式豁免
	std::set<long long> WhiteGroup;
	//黑名单群：无条件禁用
	std::set<long long> BlackGroup;
	//白名单用户：邀请私用骰娘
	std::set<long long> WhiteQQ;
	//黑名单用户：无条件禁用
	std::set<long long> BlackQQ;
	//当前时间
	SYSTEMTIME stNow = { 0 };
	SYSTEMTIME stTmp = { 0 };
	//上班时间
	std::pair<int, int> ClockToWork = { 24,0 };
	//下班时间
	std::pair<int, int> ClockOffWork = { 24,0 };

	string printClock(std::pair<int, int> clock) {
		string strClock=to_string(clock.first);
		strClock += ":";
		if(clock.second<10)strClock += "0";
		strClock += to_string(clock.second);
		return strClock;
	}

	//打印用户昵称QQ
	string printQQ(long long llqq) {
		return getStrangerInfo(llqq).nick + "(" + to_string(llqq) + ")";
	}
	//打印QQ群号
	string printGroup(long long llgroup) {
		if (getGroupList().count(llgroup))return getGroupList()[llgroup] + "(" + to_string(llgroup) + ")";
		return "群聊(" + to_string(llgroup) + ")";
	}
	//打印聊天窗口
	string printChat(chatType ct) {
		switch (ct.second)
		{
		case Private:
			return printQQ(ct.first);
		case Group:
			return printGroup(ct.first);
		case Discuss:
			return "讨论组(" + to_string(ct.first) + ")";
		default:
			break;
		}
		return "";
	}
	void sendAdmin(std::string strMsg, long long fromQQ) {
		string strName = fromQQ ? getName(fromQQ) : "";
		if(AdminQQ.count(fromQQ)) {
			AddMsgToQueue(strName + strMsg, masterQQ);
			for (auto it : AdminQQ) {
				if (fromQQ == it)AddMsgToQueue(strMsg, it);
				else AddMsgToQueue(strName + strMsg, it);
			}
		}
		else {
			AddMsgToQueue(strMsg, masterQQ);
			for (auto it : AdminQQ) {
				AddMsgToQueue(strName + strMsg, it);
			}
		}
	}

	void NotifyMonitor(std::string strMsg) {
		if (!boolMasterMode)return;
		for (auto it : MonitorList) {
			AddMsgToQueue(strMsg, it.first, it.second);
		}
	}
	//简易计时器
	void ConsoleTimer() {
		while (Enabled) {
			GetLocalTime(&stNow);
			//分钟时点变动
			if (stTmp.wMinute != stNow.wMinute) {
				stTmp = stNow;
				if (stNow.wHour == ClockOffWork.first&&stNow.wMinute == ClockOffWork.second && !boolDisabledGlobal) {
					boolDisabledGlobal = true;
					NotifyMonitor(GlobalMsg["strSelfName"] + GlobalMsg["strClockOffWork"]);
				}
				if (stNow.wHour == ClockToWork.first&&stNow.wMinute == ClockToWork.second&&boolDisabledGlobal) {
					boolDisabledGlobal = false;
					NotifyMonitor(GlobalMsg["strSelfName"] + GlobalMsg["strClockToWork"]);
				}
			}
			Sleep(10000);
		}
	}

	//一键清退
	int clearGroup(string strPara,long long) {
		int intCnt=0;
		string strReply;
		map<long long,string> GroupList=getGroupList();
		if (strPara == "unpower" || strPara.empty()) {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions == 1) {
					AddMsgToQueue(GlobalMsg["strGroupClr"], eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
			strReply = "筛除无群权限群聊" + to_string(intCnt) + "个√";
			sendAdmin(strReply);
		}
		else if (isdigit(strPara[0])) {
			int intDayLim = stoi(strPara);
			string strDayLim = to_string(intDayLim);
			time_t tNow = time(NULL);;
			for (auto eachGroup : GroupList) {
				int intDay = (int)(tNow - getGroupMemberInfo(eachGroup.first, getLoginQQ()).LastMsgTime)/86400;
				if (intDay > intDayLim) {
					strReply += "群(" + to_string(eachGroup.first) + "):" + to_string(intDay) + "天\n";
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSelfName"], to_string(intDay) }), eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					mLastMsgList.erase({ eachGroup.first ,CQ::Group });
					intCnt++;
				}
			}
			for (auto eachDiscuss : DiscussList) {
				int intDay = (int)(tNow - eachDiscuss.second) / 86400;
				if (intDay > intDayLim){
					strReply += "讨论组(" + to_string(eachDiscuss.first) + "):" + to_string(intDay) + "天\n";
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSelfName"], to_string(intDay) }), eachDiscuss.first, Group);
					Sleep(10);
					setDiscussLeave(eachDiscuss.first);
					DiscussList.erase(eachDiscuss.first);
					mLastMsgList.erase({ eachDiscuss.first ,Discuss });
					intCnt++;
				}
			}
			strReply += "筛除潜水" + strDayLim + "天群聊" + to_string(intCnt) + "个√";
			AddMsgToQueue(strReply,masterQQ);
		}
		else if (strPara == "preserve") {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, masterQQ).QQID != masterQQ&&WhiteGroup.count(eachGroup.first)==0) {
					AddMsgToQueue(GlobalMsg["strPreserve"], eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					mLastMsgList.erase({ eachGroup.first ,Group });
					intCnt++;
				}
			}
			strReply = "筛除白名单外群聊" + to_string(intCnt) + "个√";
			sendAdmin(strReply);
		}
		else
			AddMsgToQueue("无法识别筛选参数×",masterQQ);
		return intCnt;
	}

EVE_Menu(eventClearGroupUnpower) {
	int intGroupCnt = clearGroup("unpower");
	string strReply = "已清退无权限群聊" + to_string(intGroupCnt) + "个√";
	MessageBoxA(nullptr, strReply.c_str(), "一键清退", MB_OK | MB_ICONINFORMATION);
	return 0;
}
EVE_Menu(eventClearGroup30) {
	int intGroupCnt = clearGroup("30");
	string strReply = "已清退30天未使用群聊" + to_string(intGroupCnt) + "个√";
	MessageBoxA(nullptr, strReply.c_str(), "一键清退", MB_OK | MB_ICONINFORMATION);
	return 0;
}
EVE_Menu(eventGlobalSwitch) {
	if (boolDisabledGlobal) {
		boolDisabledGlobal = false;
		MessageBoxA(nullptr, "骰娘已结束静默√", "全局开关", MB_OK | MB_ICONINFORMATION);
	}
	else {
		boolDisabledGlobal = true;
		MessageBoxA(nullptr, "骰娘已全局静默√", "全局开关", MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}
EVE_Request_AddFriend(eventAddFriend) {
	string strMsg = "好友添加请求，来自：" + printQQ(fromQQ);
	if (BlackQQ.count(fromQQ)) {
		strMsg += "，已拒绝（用户在黑名单中）";
		setFriendAddRequest(responseFlag, 2, "");
	}
	else if (WhiteQQ.count(fromQQ)) {
		strMsg += "，已同意（用户在白名单中）";
		setFriendAddRequest(responseFlag, 1, "");
		GlobalMsg["strAddFriendWhiteQQ"].empty() ? AddMsgToQueue(GlobalMsg["strAddFriend"], fromQQ)
			: AddMsgToQueue(GlobalMsg["strAddFriendWhiteQQ"], fromQQ);
	}
	else {
		strMsg += "，已同意";
		setFriendAddRequest(responseFlag, 1, "");
		AddMsgToQueue(GlobalMsg["strAddFriend"], fromQQ);
	}
	sendAdmin(strMsg);
	return 1;
}

