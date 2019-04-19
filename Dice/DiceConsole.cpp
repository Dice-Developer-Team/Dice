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

using namespace std;
using namespace CQ;
namespace Console
{
	long long masterQQ = 0;
	bool boolDisabledGlobal = false;
	bool boolDisabledMeGlobal = false;
	bool boolDisabledJrrpGlobal = false;
	bool boolPreserve = false;
	bool boolNoDiscuss = false;
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
	//一键清退
	int clearGroup(string strPara) {
		int intCnt=0;
		map<long long,string> GroupList=getGroupList();
		if (strPara == "unpower" || strPara.empty()) {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions == 1) {
					AddMsgToQueue(GlobalMsg["strGroupClr"], eachGroup.first, MsgType::Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
		}
		else if (strPara == "preserve") {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, masterQQ).QQID != masterQQ&&WhiteGroup.count(eachGroup.first)==0) {
					AddMsgToQueue(GlobalMsg["strPreserve"], eachGroup.first, MsgType::Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
		}
		else
			AddMsgToQueue("无法识别筛选参数×",masterQQ, MsgType::Private);
		return intCnt;
	}
	void Process(std::string strMessage) {
		int intMsgCnt = 0;
		std::string strOption;
		while (!isspace(static_cast<unsigned char>(strMessage[intMsgCnt])) && intMsgCnt != strMessage.length() && !isdigit(static_cast<unsigned char>(strMessage[intMsgCnt])))
		{
			strOption += strMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strOption.empty()) {
			AddMsgToQueue("有什么事么， Master？", masterQQ, MsgType::Private);
			return;
		}
		if (strOption == "delete") {
			AddMsgToQueue("你不再是本骰娘的Master！", masterQQ, MsgType::Private);
			masterQQ = 0;
		}
		else {
			while (isspace(static_cast<unsigned char>(strMessage[intMsgCnt])))intMsgCnt++;
			if (strOption == "addgroup") {
				std::string strPersonalMsg = strMessage.substr(intMsgCnt);
				if (strPersonalMsg.empty()) {
					if (GlobalMsg.count("strAddGroup")) {
						GlobalMsg["strAddGroup"].clear();
						AddMsgToQueue("入群发言已清除√", masterQQ, MsgType::Private);
					}
					else AddMsgToQueue("当前未设置入群发言×", masterQQ, MsgType::Private);
				}
				else {
					GlobalMsg["strAddGroup"] = strPersonalMsg;
					AddMsgToQueue("入群发言已准备好了√", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "bot") {
				std::string strPersonalMsg = strMessage.substr(intMsgCnt);
				if (strPersonalMsg.empty()) {
					if (GlobalMsg.count("strBotMsg")) {
						GlobalMsg["strBotMsg"].clear();
						AddMsgToQueue("已清除bot附加信息√", masterQQ, MsgType::Private);
					}
					else AddMsgToQueue("当前未设置bot附加信息×", masterQQ, MsgType::Private);
				}
				else {
					GlobalMsg["strBotMsg"] = strPersonalMsg;
					AddMsgToQueue("已为bot附加信息√", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "on") {
				if (boolDisabledGlobal) {
					boolDisabledGlobal = false;
					AddMsgToQueue("骰娘已结束静默√", masterQQ, MsgType::Private);
				}
				else {
					AddMsgToQueue("骰娘不在静默中！", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "off") {
				if (boolDisabledGlobal) {
					AddMsgToQueue("骰娘已经静默！", masterQQ, MsgType::Private);
				}
				else {
					boolDisabledGlobal = true;
					AddMsgToQueue("骰娘开始静默√", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "meon") {
				if (boolDisabledMeGlobal) {
					boolDisabledMeGlobal = false;
					AddMsgToQueue("骰娘已启用.me√", masterQQ, MsgType::Private);
				}
				else {
					AddMsgToQueue("骰娘已启用.me！", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "meoff") {
				if (boolDisabledMeGlobal) {
					AddMsgToQueue("骰娘已禁用.me！", masterQQ, MsgType::Private);
				}
				else {
					boolDisabledMeGlobal = true;
					AddMsgToQueue("骰娘已禁用.me√", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "jrrpon") {
				if (boolDisabledMeGlobal) {
					boolDisabledMeGlobal = false;
					AddMsgToQueue("骰娘已启用.jrrp√", masterQQ, MsgType::Private);
				}
				else {
					AddMsgToQueue("骰娘已启用.jrrp！", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "jrrpoff") {
				if (boolDisabledMeGlobal) {
					AddMsgToQueue("骰娘已禁用.jrrp！", masterQQ, MsgType::Private);
				}
				else {
					boolDisabledMeGlobal = true;
					AddMsgToQueue("骰娘已禁用.jrrp√", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "discusson") {
				if (boolNoDiscuss) {
					boolNoDiscuss = false;
					AddMsgToQueue("骰娘已关闭讨论组自动退出√", masterQQ, MsgType::Private);
				}
				else {
					AddMsgToQueue("骰娘已关闭讨论组自动退出！", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "discussoff") {
				if (boolNoDiscuss) {
					AddMsgToQueue("骰娘已开启讨论组自动退出！", masterQQ, MsgType::Private);
				}
				else {
					boolNoDiscuss = true;
					AddMsgToQueue("骰娘已开启讨论组自动退出√", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "groupclr") {
				std::string strPara = strMessage.substr(intMsgCnt);
				int intGroupCnt=clearGroup(strPara);
				string strReply = "筛除群聊" + to_string(intGroupCnt) + "个√";
				AddMsgToQueue(strReply, masterQQ, MsgType::Private);
			}
			else if (strOption == "only") {
				if (boolPreserve) {
					AddMsgToQueue("已成为Master的专属骰娘！", masterQQ, MsgType::Private);
				}
				else {
					boolPreserve = true;
					AddMsgToQueue("已成为Master的专属骰娘√", masterQQ, MsgType::Private);
				}
			}
			else if (strOption == "public") {
				if (boolPreserve) {
					boolPreserve = false;
					AddMsgToQueue("已成为公用骰娘√", masterQQ, MsgType::Private);
				}
				else {
					AddMsgToQueue("已成为公用骰娘！", masterQQ, MsgType::Private);
				}
			}
			else {
				bool boolErase=false;
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
				long long llTargetID = stoll(strTargetID);
				if (strOption == "dismiss") {
					WhiteGroup.erase(llTargetID);
					if (getGroupList().count(llTargetID)&& llTargetID<1000000000) {
						setGroupLeave(llTargetID);
						AddMsgToQueue("骰娘已退出该群√", masterQQ, MsgType::Private);
					}
					else if(llTargetID > 1000000000&& setDiscussLeave(llTargetID)==0) {
						AddMsgToQueue("骰娘已退出该讨论组√", masterQQ, MsgType::Private);
					}
					else {
						AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ, MsgType::Private);
					}
				}
				else if (strOption == "boton") {
					if (getGroupList().count(llTargetID)) {
						if (DisabledGroup.count(llTargetID)) {
							DisabledGroup.erase(llTargetID);
							AddMsgToQueue("骰娘已在该群起用√", masterQQ, MsgType::Private);
						}
						else AddMsgToQueue("骰娘已在该群起用!", masterQQ, MsgType::Private);
					}
					else {
						AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ, MsgType::Private);
					}
				}
				else if (strOption == "botoff") {
					if (getGroupList().count(llTargetID)) {
						if (!DisabledGroup.count(llTargetID)) {
							DisabledGroup.insert(llTargetID);
							AddMsgToQueue("骰娘已在该群静默√", masterQQ, MsgType::Private);
						}
						else AddMsgToQueue("骰娘已在该群静默!", masterQQ, MsgType::Private);
					}
					else {
						AddMsgToQueue(GlobalMsg["strGroupGetErr"], masterQQ, MsgType::Private);
					}
				}
				else if (strOption == "whitegroup") {
					if (boolErase) {
						if (WhiteGroup.count(llTargetID)) {
							WhiteGroup.erase(llTargetID);
							AddMsgToQueue("该群已移出白名单√", masterQQ, MsgType::Private);
						}
						else {
							WhiteGroup.insert(llTargetID);
							AddMsgToQueue("该群并不在白名单！", masterQQ, MsgType::Private);
						}
					}
					else {
						if (WhiteGroup.count(llTargetID)) {
							AddMsgToQueue("该群已加入白名单!", masterQQ, MsgType::Private);
						}
						else {
							WhiteGroup.insert(llTargetID);
							AddMsgToQueue("该群已加入白名单√", masterQQ, MsgType::Private);
						}
					}
				}
				else if (strOption == "blackgroup") {
					if (boolErase) {
						if (BlackGroup.count(llTargetID)) {
							BlackGroup.erase(llTargetID);
							AddMsgToQueue("该群已移出黑名单√", masterQQ, MsgType::Private);
						}
						else {
							BlackGroup.insert(llTargetID);
							AddMsgToQueue("该群并不在黑名单！", masterQQ, MsgType::Private);
						}
					}
					else {
						if (BlackGroup.count(llTargetID)) {
							AddMsgToQueue("该群已加入黑名单!", masterQQ, MsgType::Private);
						}
						else {
							BlackGroup.insert(llTargetID);
							AddMsgToQueue("该群已加入黑名单√", masterQQ, MsgType::Private);
						}
					}
				}
				else if (strOption == "whiteqq") {
					if (boolErase) {
						if (WhiteQQ.count(llTargetID)) {
							WhiteQQ.erase(llTargetID);
							AddMsgToQueue("该用户已移出白名单√", masterQQ, MsgType::Private);
						}
						else {
							WhiteQQ.insert(llTargetID);
							AddMsgToQueue("该用户并不在白名单！", masterQQ, MsgType::Private);
						}
					}
					else {
						if (WhiteQQ.count(llTargetID)) {
							AddMsgToQueue("该用户已加入白名单!", masterQQ, MsgType::Private);
						}
						else {
							WhiteQQ.insert(llTargetID);
							AddMsgToQueue("该用户已加入白名单√", masterQQ, MsgType::Private);
						}
					}
				}
				else if (strOption == "blackqq") {
					if (boolErase) {
						if (BlackQQ.count(llTargetID)) {
							BlackQQ.erase(llTargetID);
							AddMsgToQueue("该用户已移出黑名单√", masterQQ, MsgType::Private);
						}
						else {
							BlackQQ.insert(llTargetID);
							AddMsgToQueue("该用户并不在黑名单！", masterQQ, MsgType::Private);
						}
					}
					else {
						if (BlackQQ.count(llTargetID)) {
							AddMsgToQueue("该用户已加入黑名单!", masterQQ, MsgType::Private);
						}
						else {
							BlackQQ.insert(llTargetID);
							AddMsgToQueue("该用户已加入黑名单√", masterQQ, MsgType::Private);
						}
					}
				}
			}
		}
	}

	EVE_Menu(eventClearGroup) {
		int intGroupCnt = clearGroup();
		string strReply= "已清退无权限群聊" + to_string(intGroupCnt) + "个√";
		MessageBoxA(nullptr,strReply.c_str(),"一键清退", MB_OK | MB_ICONINFORMATION);
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
}


