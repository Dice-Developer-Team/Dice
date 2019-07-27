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
#include <queue>
#include <mutex>
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "MsgFormat.h"
#include "NameStorage.h"
#include "DiceNetwork.h"
#include "jsonio.h"

using namespace std;
using namespace CQ;

	long long masterQQ = 0;
set<long long> AdminQQ = {};
set<chatType> MonitorList = {};
std::map<std::string, bool>boolConsole = { {"DisabledGlobal",false},
{"DisabledMe",false},{"DisabledJrrp",false},{"DisabledDeck",true},{"DisabledDraw",false},{"DisabledSend",true},
{"Private",false},{"LeaveDiscuss",false},
{"LeaveBlackQQ",true},{"AllowStranger",true},
{"BannedBanOwner",true},{"BannedLeave",false},{"BannedBanInviter",true},
{"KickedBanInviter",true} };
//骰娘列表
std::map<long long, long long> mDiceList;
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
std::string printSTime(SYSTEMTIME st){
	return to_string(st.wYear) + "/" + to_string(st.wMonth) + "/" + to_string(st.wDay) + " " + to_string(st.wHour) + ":" + to_string(st.wMinute) + ":" + to_string(st.wSecond);
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
//获取骰娘列表
void getDiceList() {
	std::string list;
	if (!Network::GET("shiki.stringempty.xyz", "/DiceList", 80, list))
	{
		sendAdmin(("获取骰娘列表时遇到错误: \n" + list).c_str(), 0);
		return;
	}
	readJson(list, mDiceList);
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
			masterQQ == fromQQ ? AddMsgToQueue(strMsg, masterQQ) : AddMsgToQueue(strName + strMsg, masterQQ);
			for (auto it : AdminQQ) {
				AddMsgToQueue(strName + strMsg, it);
			}
		}
	}

void NotifyMonitor(std::string strMsg) {
		if (!boolMasterMode)return;
		for (auto it : MonitorList) {
			AddMsgToQueue(strMsg, it.first, it.second);
			Sleep(1000);
		}
	}
//拉黑用户后搜查群
void checkBlackQQ(long long llQQ, std::string strWarning) {
	map<long long, string> GroupList = getGroupList();
	string strNotice;
	int intCnt = 0;
	for (auto eachGroup : GroupList) {
		if (getGroupMemberInfo(eachGroup.first, llQQ).QQID == llQQ) {
			strNotice += "\n" + printGroup(eachGroup.first);
			if (getGroupMemberInfo(eachGroup.first, llQQ).permissions < getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
				strNotice += "对方群权限较低";
			}
			else if (getGroupMemberInfo(eachGroup.first, llQQ).permissions > getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
				AddMsgToQueue(strWarning, eachGroup.first, Group);
				AddMsgToQueue("发现新增黑名单成员" + printQQ(llQQ) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群", eachGroup.first, Group);
				strNotice += "对方群权限较高，已退群";
				Sleep(1000);
				setGroupLeave(eachGroup.first);
			}
			else if (WhiteGroup.count(eachGroup.first) || MonitorList.count({ eachGroup.first ,Group })) {
				strNotice += "群在白名单中";
			}
			else if (boolConsole["LeaveBlackQQ"]) {
				AddMsgToQueue(strWarning, eachGroup.first, Group); 
				AddMsgToQueue("发现新增黑名单成员" + printQQ(llQQ) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群", eachGroup.first, Group);
				strNotice += "已退群";
				Sleep(1000);
				setGroupLeave(eachGroup.first);
			}
			else
				AddMsgToQueue(strWarning, eachGroup.first, Group);
			intCnt++;
		}
	}
	if (intCnt) {
		strNotice = "已清查与" + printQQ(llQQ) + "共同群聊" + to_string(intCnt) + "个：" + strNotice;
		sendAdmin(strNotice);
	}
}
//拉黑用户
void addBlackQQ(long long llQQ, std::string strReason, std::string strNotice) {
	if (llQQ == masterQQ || AdminQQ.count(llQQ) || llQQ == getLoginQQ())return;
	if (WhiteQQ.count(llQQ))WhiteQQ.erase(llQQ);
	if (BlackQQ.count(llQQ) == 0) {
		BlackQQ.insert(llQQ);
		strReason.empty() ? AddMsgToQueue(GlobalMsg["strBlackQQAddNotice"], llQQ)
			: AddMsgToQueue(format(GlobalMsg["strBlackQQAddNoticeReason"], { strReason }), llQQ);
	}
	checkBlackQQ(llQQ, strNotice);
}
struct fromMsg {
	string strMsg;
	long long fromQQ = 0;
	long long fromGroup = 0;
	fromMsg() = default;
	fromMsg(string strMsg, long long QQ, long long Group) :strMsg(strMsg), fromQQ(QQ), fromGroup(Group) {};
};
// 消息发送队列
std::queue<fromMsg> warningQueue;
// 消息发送队列锁
mutex warningMutex;
void AddWarning(const string& msg, long long DiceQQ, long long fromGroup)
{
	lock_guard<std::mutex> lock_queue(warningMutex);
	warningQueue.emplace(msg, DiceQQ, fromGroup);
}
void warningHandler() {
	while (Enabled)
	{
		fromMsg warning;
		{
			lock_guard<std::mutex> lock_queue(warningMutex);
			if (!warningQueue.empty())
			{
				warning = warningQueue.front();
				warningQueue.pop();
			}
		}
		if (!warning.strMsg.empty()) {
			long long blackQQ, blackGroup;
			string type, time, note;
			nlohmann::json jInfo;
			try {
				jInfo = nlohmann::json::parse(GBKtoUTF8(warning.strMsg));
				jInfo.count("fromQQ") ? blackQQ = jInfo["fromQQ"] : blackQQ = 0;
				jInfo.count("fromGroup") ? blackGroup = jInfo["fromGroup"] : blackGroup = 0;
				jInfo.count("type") ? type = readJKey<string>(jInfo["type"]) : type = "Unknown";
				//jInfo.count("time") ? time = readJKey<string>(jInfo["time"]) : time = "Unknown";
				jInfo.count("note") ? note = readJKey<string>(jInfo["note"]) : note = "";
			}
			catch (...) {
				continue;
			}
			if (type != "ban" && type != "kick" || (!blackGroup || BlackGroup.count(blackGroup)) && (!blackQQ || BlackQQ.count(blackQQ))) {
				continue;
			}
			string strWarning = "!warning" + warning.strMsg;
			if (warning.fromQQ != masterQQ || !AdminQQ.count(warning.fromQQ))sendAdmin("来自" + printQQ(warning.fromQQ) + strWarning);
			if (blackGroup) {
				if (!BlackGroup.count(blackGroup)) {
					BlackGroup.insert(blackGroup);
					sendAdmin("已通知" + GlobalMsg["strSelfName"] + "将" + printGroup(blackGroup) + "加入群黑名单√", warning.fromQQ);
				}
				if (getGroupList().count(blackGroup)) {
					if (blackGroup != warning.fromGroup)AddMsgToQueue(strWarning, blackGroup, Group);
					setGroupLeave(blackGroup);
				}
			}
			if (blackQQ) {
				if (!BlackQQ.count(blackQQ))sendAdmin("已通知" + GlobalMsg["strSelfName"] + "将" + printQQ(blackQQ) + "加入用户黑名单", warning.fromQQ);
				addBlackQQ(blackQQ, note, strWarning);
			}
		}
		else
		{
			this_thread::sleep_for(chrono::milliseconds(20));
		}
	}
}
//简易计时器
	void ConsoleTimer() {
		while (Enabled) {
			GetLocalTime(&stNow);
			//分钟时点变动
			if (stTmp.wMinute != stNow.wMinute) {
				stTmp = stNow;
				if (stNow.wHour == ClockOffWork.first&&stNow.wMinute == ClockOffWork.second && !boolConsole["DisabledGlobal"]) {
					boolConsole["DisabledGlobal"] = true;
					NotifyMonitor(GlobalMsg["strSelfName"] + GlobalMsg["strClockOffWork"]);
				}
				if (stNow.wHour == ClockToWork.first&&stNow.wMinute == ClockToWork.second&&boolConsole["DisabledGlobal"]) {
					boolConsole["DisabledGlobal"] = false;
					NotifyMonitor(GlobalMsg["strSelfName"] + GlobalMsg["strClockToWork"]);
				}
				if (stNow.wHour == 5 && stNow.wMinute == 0) {
					getDiceList();
					clearGroup("black");
				}
			}
			Sleep(100);
		}
	}

	//一键清退
	int clearGroup(string strPara,long long fromQQ) {
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
			strReply = GlobalMsg["strSelfName"] + "筛除无群权限群聊" + to_string(intCnt) + "个√";
			sendAdmin(strReply);
		}
		else if (isdigit(static_cast<unsigned char>(strPara[0]))) {
			int intDayLim = stoi(strPara);
			string strDayLim = to_string(intDayLim);
			time_t tNow = time(NULL);;
			for (auto eachChat : mLastMsgList) {
				if (eachChat.first.second == Private)continue;
				int intDay = (int)(tNow - eachChat.second) / 86400;
				if (intDay > intDayLim) {
					strReply += printChat(eachChat.first) + ":" + to_string(intDay) + "天\n";
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSelfName"], to_string(intDay) }), eachChat.first.first, eachChat.first.second);
					Sleep(100);
					if (eachChat.first.second == Group) {
						setGroupLeave(eachChat.first.first);
						if (GroupList.count(eachChat.first.first))GroupList.erase(eachChat.first.first);
					}
					else setDiscussLeave(eachChat.first.first);
					mLastMsgList.erase(eachChat.first);
					intCnt++;
				}
			}
			for (auto eachGroup : GroupList) {
				int intDay = (int)(tNow - getGroupMemberInfo(eachGroup.first, getLoginQQ()).LastMsgTime)/86400;
				if (intDay > intDayLim) {
					strReply += printGroup(eachGroup.first) + ":" + to_string(intDay) + "天\n";
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSelfName"], to_string(intDay) }), eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					mLastMsgList.erase({ eachGroup.first ,CQ::Group });
					intCnt++;
				}
			}
			strReply += GlobalMsg["strSelfName"] + "已筛除潜水" + strDayLim + "天群聊" + to_string(intCnt) + "个√";
			AddMsgToQueue(strReply,masterQQ);
		}
		else if (strPara == "black") {
			for (auto eachGroup : GroupList) {
				if (BlackGroup.count(eachGroup.first)) {
					AddMsgToQueue(GlobalMsg["strBlackGroup"], eachGroup.first, Group);
					strReply += "\n" + printGroup(eachGroup.first) + "：" + "黑名单群";
					Sleep(100);
					setGroupLeave(eachGroup.first);
				}
				vector<GroupMemberInfo> MemberList = getGroupMemberList(eachGroup.first);
				for (auto eachQQ : MemberList) {
					if (BlackQQ.count(eachQQ.QQID)) {
						if (getGroupMemberInfo(eachGroup.first, eachQQ.QQID).permissions < getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
							continue;
						}
						else if (getGroupMemberInfo(eachGroup.first, eachQQ.QQID).permissions > getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
							AddMsgToQueue("发现黑名单成员" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群", eachGroup.first, Group);
							strReply += "\n" + printGroup(eachGroup.first) + "：" + printQQ(eachQQ.QQID) + "对方群权限较高";
							Sleep(100);
							setGroupLeave(eachGroup.first);
							intCnt++;
							break;
						}
						else if (WhiteGroup.count(eachGroup.first) || MonitorList.count({ eachGroup.first ,Group })) {
							continue;
						}
						else if (boolConsole["LeaveBlackQQ"]) {
							AddMsgToQueue("发现黑名单成员" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群", eachGroup.first, Group);
							strReply += "\n" + printGroup(eachGroup.first) + "：" + printQQ(eachQQ.QQID);
							Sleep(100);
							setGroupLeave(eachGroup.first);
							intCnt++;
							break;
						}
					}
				}
			}
			if (intCnt) {
				strReply = GlobalMsg["strSelfName"] + "已按黑名单清查群聊" + to_string(intCnt) + "个：" + strReply;
				sendAdmin(strReply);
			}
			else if (fromQQ) {
				sendAdmin(GlobalMsg["strSelfName"] + "未发现涉黑群聊");
			}
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
			strReply = GlobalMsg["strSelfName"] + "筛除白名单外群聊" + to_string(intCnt) + "个√";
			sendAdmin(strReply);
		}
		else
			AddMsgToQueue("无法识别筛选参数×", fromQQ);
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
	if (boolConsole["DisabledGlobal"]) {
		boolConsole["DisabledGlobal"] = false;
		MessageBoxA(nullptr, "骰娘已结束静默√", "全局开关", MB_OK | MB_ICONINFORMATION);
	}
	else {
		boolConsole["DisabledGlobal"] = true;
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
	else if (boolConsole["Private"]&& !boolConsole["AllowStranger"]) {
		strMsg += "，已拒绝（当前在私用模式）";
		setFriendAddRequest(responseFlag, 2, "");
	}
	else {
		strMsg += "，已同意";
		setFriendAddRequest(responseFlag, 1, "");
		AddMsgToQueue(GlobalMsg["strAddFriend"], fromQQ);
	}
	sendAdmin(strMsg);
	return 1;
}

