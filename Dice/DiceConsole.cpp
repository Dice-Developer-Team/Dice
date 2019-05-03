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
#include "CQEVE_ALL.h"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "DiceMsgSend.h"
#include "MsgFormat.h"

using namespace std;
using namespace CQ;

	long long masterQQ = 0;
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

	string transClock(std::pair<int, int> clock) {
		string strClock=to_string(clock.first);
		strClock += ":";
		if(clock.second<10)strClock += "0";
		strClock += to_string(clock.second);
		return strClock;
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
					AddMsgToQueue(GlobalMsg["strSelfName"] + GlobalMsg["strClockOffWork"], masterQQ);
				}
				if (stNow.wHour == ClockToWork.first&&stNow.wMinute == ClockToWork.second&&boolDisabledGlobal) {
					boolDisabledGlobal = false;
					AddMsgToQueue(GlobalMsg["strSelfName"] + GlobalMsg["strClockToWork"], masterQQ);
				}
			}
			Sleep(10000);
		}
	}

	//一键清退
	int clearGroup(string strPara) {
		int intCnt=0;
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
		}
		else if (isdigit(strPara[0])) {
			time_t tNow(NULL);
			time_t tLim= tNow - stoi(strPara) * 24 * 60 * 60;
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, getLoginQQ()).LastMsgTime < tLim) {
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSlefName"], strPara }), eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
			for (auto eachDiscuss : DiscussList) {
				if (eachDiscuss.second < tLim) {
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSlefName"], strPara }), eachDiscuss.first, Group);
					Sleep(10);
					setDiscussLeave(eachDiscuss.first);
					intCnt++;
				}
			}
		}
		else if (strPara == "preserve") {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, masterQQ).QQID != masterQQ&&WhiteGroup.count(eachGroup.first)==0) {
					AddMsgToQueue(GlobalMsg["strPreserve"], eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
		}
		else
			AddMsgToQueue("无法识别筛选参数×",masterQQ);
		return intCnt;
	}
void ConsoleHandler(std::string strMessage) {
	int intMsgCnt = 0;
	std::string strOption;
	while (!isspace(static_cast<unsigned char>(strMessage[intMsgCnt])) && intMsgCnt != strMessage.length() && !isdigit(static_cast<unsigned char>(strMessage[intMsgCnt])))
	{
		strOption += strMessage[intMsgCnt];
		intMsgCnt++;
	}
	if (strOption.empty()) {
		AddMsgToQueue("有什么事么， Master？", masterQQ);
		return;
	}
	if (strOption == "delete") {
		AddMsgToQueue("你不再是" + GlobalMsg["strSelfName"] + "的Master！", masterQQ);
		masterQQ = 0;
	}
	else if (strOption == "state") {
		string strReply= getLoginNick();
		strReply = strReply + "的当前情况" + "\n"
			+ (ClockToWork.first < 24 ? "定时开启" + transClock(ClockToWork) + "\n" : "")
			+ (ClockOffWork.first < 24 ? "定时关闭" + transClock(ClockOffWork) + "\n" : "")
			+ (boolPreserve ? "私用模式" : "公用模式") + "\n"
			+ (boolNoDiscuss ? "禁用讨论组" : "启用讨论组") + "\n"
			+ "全局开关：" + (boolDisabledGlobal ? "禁用" : "启用") + "\n"
			+ "全局.me开关：" + (boolDisabledMeGlobal ? "禁用" : "启用") + "\n"
			+ "全局.jrrp开关：" + (boolDisabledJrrpGlobal ? "禁用" : "启用") + "\n"
			+ "所在群聊数：" + to_string(getGroupList().size()) + "\n"
			+ (DiscussList.size() ? "有记录的讨论组数：" + to_string(DiscussList.size()) + "\n" : "")
			+ "黑名单用户数：" + to_string(BlackQQ.size()) + "\n"
			+ "黑名单群数：" + to_string(BlackGroup.size()) + "\n"
			+ "白名单用户数：" + to_string(WhiteQQ.size()) + "\n"
			+ "白名单群数：" + to_string(WhiteGroup.size());
		AddMsgToQueue(strReply, masterQQ);
	}
	else if (strOption == "clock") {
		string strReply = "本地时间" + to_string(stNow.wHour) + ":" + to_string(stNow.wMinute);
		AddMsgToQueue(strReply, masterQQ);
	}
	else {
		while (isspace(static_cast<unsigned char>(strMessage[intMsgCnt])))intMsgCnt++;
		if (strOption == "addgroup") {
			std::string strPersonalMsg = strMessage.substr(intMsgCnt);
			if (strPersonalMsg.empty()) {
				if (GlobalMsg.count("strAddGroup")) {
					GlobalMsg["strAddGroup"].clear();
					AddMsgToQueue("入群发言已清除√", masterQQ);
				}
				else AddMsgToQueue("当前未设置入群发言×", masterQQ);
			}
			else {
				GlobalMsg["strAddGroup"] = strPersonalMsg;
				AddMsgToQueue("入群发言已准备好了√", masterQQ);
			}
		}
		else if (strOption == "bot") {
			std::string strPersonalMsg = strMessage.substr(intMsgCnt);
			if (strPersonalMsg.empty()) {
				if (GlobalMsg.count("strBotMsg")) {
					GlobalMsg["strBotMsg"].clear();
					AddMsgToQueue("已清除bot附加信息√", masterQQ);
				}
				else AddMsgToQueue("当前未设置bot附加信息×", masterQQ);
			}
			else {
				GlobalMsg["strBotMsg"] = strPersonalMsg;
				AddMsgToQueue("已为bot附加信息√", masterQQ);
			}
		}
		else if (strOption == "on") {
			if (boolDisabledGlobal) {
				boolDisabledGlobal = false;
				AddMsgToQueue("骰娘已结束静默√", masterQQ);
			}
			else {
				AddMsgToQueue("骰娘不在静默中！", masterQQ);
			}
		}
		else if (strOption == "off") {
			if (boolDisabledGlobal) {
				AddMsgToQueue("骰娘已经静默！", masterQQ);
			}
			else {
				boolDisabledGlobal = true;
				AddMsgToQueue("骰娘开始静默√", masterQQ);
			}
		}
		else if (strOption == "meon") {
			if (boolDisabledMeGlobal) {
				boolDisabledMeGlobal = false;
				AddMsgToQueue("骰娘已启用.me√", masterQQ);
			}
			else {
				AddMsgToQueue("骰娘已启用.me！", masterQQ);
			}
		}
		else if (strOption == "meoff") {
			if (boolDisabledMeGlobal) {
				AddMsgToQueue("骰娘已禁用.me！", masterQQ);
			}
			else {
				boolDisabledMeGlobal = true;
				AddMsgToQueue("骰娘已禁用.me√", masterQQ);
			}
		}
		else if (strOption == "jrrpon") {
			if (boolDisabledMeGlobal) {
				boolDisabledMeGlobal = false;
				AddMsgToQueue("骰娘已启用.jrrp√", masterQQ);
			}
			else {
				AddMsgToQueue("骰娘已启用.jrrp！", masterQQ);
			}
		}
		else if (strOption == "jrrpoff") {
			if (boolDisabledMeGlobal) {
				AddMsgToQueue("骰娘已禁用.jrrp！", masterQQ);
			}
			else {
				boolDisabledMeGlobal = true;
				AddMsgToQueue("骰娘已禁用.jrrp√", masterQQ);
			}
		}
		else if (strOption == "discusson") {
			if (boolNoDiscuss) {
				boolNoDiscuss = false;
				AddMsgToQueue("骰娘已关闭讨论组自动退出√", masterQQ);
			}
			else {
				AddMsgToQueue("骰娘已关闭讨论组自动退出！", masterQQ);
			}
		}
		else if (strOption == "discussoff") {
			if (boolNoDiscuss) {
				AddMsgToQueue("骰娘已开启讨论组自动退出！", masterQQ);
			}
			else {
				boolNoDiscuss = true;
				AddMsgToQueue("骰娘已开启讨论组自动退出√", masterQQ);
			}
		}
		else if (strOption == "groupclr") {
			std::string strPara = strMessage.substr(intMsgCnt);
			int intGroupCnt = clearGroup(strPara);
			string strReply = "筛除群聊" + to_string(intGroupCnt) + "个√";
			AddMsgToQueue(strReply, masterQQ);
		}
		else if (strOption == "only") {
			if (boolPreserve) {
				AddMsgToQueue("已成为Master的专属骰娘！", masterQQ);
			}
			else {
				boolPreserve = true;
				AddMsgToQueue("已成为Master的专属骰娘√", masterQQ);
			}
		}
		else if (strOption == "public") {
			if (boolPreserve) {
				boolPreserve = false;
				AddMsgToQueue("已成为公用骰娘√", masterQQ);
			}
			else {
				AddMsgToQueue("已成为公用骰娘！", masterQQ);
			}
		}
		else if (strOption == "clockon") {
			string strHour;
			while (isdigit(strMessage[intMsgCnt])) {
				strHour += strMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strHour.empty()||stoi(strHour)> 23) {
				ClockToWork = { 24,00 };
				AddMsgToQueue("已清除定时启用", masterQQ);
				return;
			}
			int intHour = stoi(strHour);
			while (!isdigit(strMessage[intMsgCnt])) {
				intMsgCnt++;
			}
			string strMinute;
			while (isdigit(strMessage[intMsgCnt])) {
				strMinute += strMessage[intMsgCnt];
				intMsgCnt++;
			}
			if(strMinute.empty()) strMinute="00";
			int intMinute = stoi(strMinute);
			ClockToWork = { intHour,intMinute };
			AddMsgToQueue("已设置定时启用时间"+strHour+":"+strMinute,masterQQ);
		}
		else if (strOption == "clockoff") {
		string strHour;
		while (isdigit(strMessage[intMsgCnt])) {
			strHour += strMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strHour.empty() || stoi(strHour) > 23) {
			ClockOffWork = { 24,00 };
			AddMsgToQueue("已清除定时关闭", masterQQ);
			return;
		}
		int intHour = stoi(strHour);
		while (!isdigit(strMessage[intMsgCnt])) {
			intMsgCnt++;
		}
		string strMinute;
		while (isdigit(strMessage[intMsgCnt])) {
			strMinute += strMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strMinute.empty()) strMinute = "00";
		int intMinute = stoi(strMinute);
		ClockOffWork = { intHour,intMinute };
		AddMsgToQueue("已设置定时关闭时间" + strHour + ":" + strMinute, masterQQ);
		}
		else {
			if (intMsgCnt == strMessage.length()) {
				AddMsgToQueue("有什么事么， Master？", masterQQ);
				return;
			}
			bool boolErase = false;
			std::string strTargetID;
			if (strMessage[intMsgCnt] == '-') {
				boolErase = true;
				intMsgCnt++;
			}
			while (isspace(static_cast<unsigned char>(strMessage[intMsgCnt])))intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strMessage[intMsgCnt]))) {
				strTargetID += strMessage[intMsgCnt];
				intMsgCnt++;
			}
			long long llTargetID;
			if (strTargetID.empty()) {
				llTargetID = stoll(strTargetID);
			}
			if (strOption == "dismiss") {
				WhiteGroup.erase(llTargetID);
				if (getGroupList().count(llTargetID)) {
					setGroupLeave(llTargetID);
					AddMsgToQueue("骰娘已退出该群√", masterQQ);
				}
				else if (llTargetID > 1000000000 && setDiscussLeave(llTargetID) == 0) {
					AddMsgToQueue("骰娘已退出该讨论组√", masterQQ);
					DiscussList.erase(llTargetID);
				}
				else {
					AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ);
				}
			}
			else if (strOption == "boton") {
				if (getGroupList().count(llTargetID)) {
					if (DisabledGroup.count(llTargetID)) {
						DisabledGroup.erase(llTargetID);
						AddMsgToQueue("骰娘已在该群起用√", masterQQ);
					}
					else AddMsgToQueue("骰娘已在该群起用!", masterQQ);
				}
				else {
					AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ);
				}
			}
			else if (strOption == "botoff") {
				if (getGroupList().count(llTargetID)) {
					if (!DisabledGroup.count(llTargetID)) {
						DisabledGroup.insert(llTargetID);
						AddMsgToQueue("骰娘已在该群静默√", masterQQ);
					}
					else AddMsgToQueue("骰娘已在该群静默!", masterQQ);
				}
				else {
					AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ);
				}
			}
			else if (strOption == "whitegroup") {
				if (boolErase) {
					if (WhiteGroup.count(llTargetID)) {
						WhiteGroup.erase(llTargetID);
						AddMsgToQueue("该群已移出白名单√", masterQQ);
					}
					else {
						AddMsgToQueue("该群并不在白名单！", masterQQ);
					}
				}
				else {
					if (WhiteGroup.count(llTargetID)) {
						AddMsgToQueue("该群已加入白名单!", masterQQ);
					}
					else {
						WhiteGroup.insert(llTargetID);
						AddMsgToQueue("该群已加入白名单√", masterQQ);
					}
				}
			}
			else if (strOption == "blackgroup") {
				if (boolErase) {
					if (BlackGroup.count(llTargetID)) {
						BlackGroup.erase(llTargetID);
						AddMsgToQueue("该群已移出黑名单√", masterQQ);
					}
					else {
						AddMsgToQueue("该群并不在黑名单！", masterQQ);
					}
				}
				else {
					if (BlackGroup.count(llTargetID)) {
						AddMsgToQueue("该群已加入黑名单!", masterQQ);
					}
					else {
						BlackGroup.insert(llTargetID);
						AddMsgToQueue("该群已加入黑名单√", masterQQ);
					}
				}
			}
			else if (strOption == "whiteqq") {
				if (boolErase) {
					if (WhiteQQ.count(llTargetID)) {
						WhiteQQ.erase(llTargetID);
						AddMsgToQueue("该用户已移出白名单√", masterQQ);
					}
					else {
						AddMsgToQueue("该用户并不在白名单！", masterQQ);
					}
				}
				else {
					if (WhiteQQ.count(llTargetID)) {
						AddMsgToQueue("该用户已加入白名单!", masterQQ);
					}
					else {
						WhiteQQ.insert(llTargetID);
						AddMsgToQueue("该用户已加入白名单√", masterQQ);
					}
				}
			}
			else if (strOption == "blackqq") {
				if (boolErase) {
					if (BlackQQ.count(llTargetID)) {
						BlackQQ.erase(llTargetID);
						AddMsgToQueue("该用户已移出黑名单√", masterQQ);
					}
					else {
						AddMsgToQueue("该用户并不在黑名单！", masterQQ);
					}
				}
				else {
					if (BlackQQ.count(llTargetID)) {
						AddMsgToQueue("该用户已加入黑名单!", masterQQ);
					}
					else {
						BlackQQ.insert(llTargetID);
						AddMsgToQueue("该用户已加入黑名单√", masterQQ);
					}
				}
			}
			else AddMsgToQueue("有什么事么， Master？", masterQQ);
			return;
		}
	}
}

EVE_Menu(eventClearGroup) {
	int intGroupCnt = clearGroup();
	string strReply = "已清退无权限群聊" + to_string(intGroupCnt) + "个√";
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


