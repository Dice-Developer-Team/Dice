/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <thread>
#include <chrono>
#include <mutex>

#include "APPINFO.h"
#include "jsonio.h"
#include "RandomGenerator.h"
#include "RD.h"
#include "CQEVE_ALL.h"
#include "InitList.h"
#include "GlobalVar.h"
#include "NameStorage.h"
#include "DiceMsgSend.h"
#include "MsgFormat.h"
#include "DiceCloud.h"
#include "CardDeck.h"
#include "DiceConsole.h"
#include "EncodingConvert.h"
#include "DiceEvent.h"

using namespace std;
using namespace CQ;


//Master模式
bool boolMasterMode = false;
//替身模式
bool boolStandByMe = false;
//本体与替身帐号
long long IdentityQQ = 0;
long long StandQQ = 0;
map<long long, int> DefaultDice;
map<chatType, int> mDefaultCOC;
map<long long, string> DefaultRule;
set<long long> DisabledJRRPGroup;
set<long long> DisabledJRRPDiscuss;
set<long long> DisabledMEGroup;
set<long long> DisabledMEDiscuss;
set<long long> DisabledHELPGroup;
set<long long> DisabledHELPDiscuss;
set<long long> DisabledOBGroup;
set<long long> DisabledOBDiscuss;

unique_ptr<Initlist> ilInitList;

map<chatType, time_t> mLastMsgList;
map<chatType, chatType> mLinkedList;
multimap<chatType, chatType> mFwdList;

multimap<long long, long long> ObserveGroup;
multimap<long long, long long> ObserveDiscuss;
string strFileLoc;

//加载数据
void loadData() {
	mkDir("DiceData");
	string strLog;
	loadDir(loadINI<CardTemp>, string("DiceData\\CardTemp\\"), mCardTemplet, strLog);
	loadJMap(strFileLoc + "PublicDeck.json", CardDeck::mPublicDeck);
	loadJMap(strFileLoc + "ExternDeck.json", CardDeck::mPublicDeck);
	loadJMap(strFileLoc + "ReplyDeck.json", CardDeck::mReplyDeck);
	loadDir(loadJMap, string("DiceData\\PublicDeck\\"), CardDeck::mPublicDeck, strLog); 
	if (!strLog.empty()) {
		strLog += "扩展配置读取完毕√";
		addRecord(strLog);
	}
}
//备份数据
void dataBackUp() {
	mkDir("DiceData\\conf");
	mkDir("DiceData\\user");
	//备份MasterQQ
	if (!boolStandByMe) {
		ofstream ofstreamMaster(strFileLoc + "Master.RDconf", ios::out | ios::trunc);
		ofstreamMaster << masterQQ << std::endl << boolMasterMode << std::endl << boolConsole["DisabledGlobal"] << std::endl << boolConsole["DisabledMe"] << std::endl << boolConsole["Private"] << std::endl << boolConsole["DisabledJrrp"] << std::endl << boolConsole["LeaveDiscuss"] << std::endl;
		ofstreamMaster << ClockToWork.first << " " << ClockToWork.second << endl
			<< ClockOffWork.first << " " << ClockOffWork.second << endl;
		ofstreamMaster.close();
	}
	//备份管理员列表
	saveFile(strFileLoc + "AdminQQ.RDconf", AdminQQ);
	saveFile("DiceData\\conf\\RecorderList.RDconf", RecorderList);
	saveFile(strFileLoc + "MonitorList.RDconf", MonitorList);
	saveJMap(strFileLoc + "boolConsole.json", boolConsole);
	//备份默认规则
	saveFile(strFileLoc + "DefaultRule.RDconf", DefaultRule);
	//备份黑白名单
	saveFile(strFileLoc + "WhiteGroup.RDconf", WhiteGroup);
	saveFile(strFileLoc + "BlackGroup.RDconf", BlackGroup);
	saveFile(strFileLoc + "WhiteQQ.RDconf", WhiteQQ);
	saveFile(strFileLoc + "BlackQQ.RDconf", BlackQQ);
	//saveBlackMark(strFileLoc + "BlackMarks.json");
	saveFile(strFileLoc + "LastMsgList.MYmap", mLastMsgList);
	saveFile(strFileLoc + "Default.RDconf", DefaultDice);
	//保存卡牌
	saveJMap(strFileLoc + "GroupDeck.json", CardDeck::mGroupDeck);
	saveJMap(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	saveJMap(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	saveJMap(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
	//保存群内设置
	saveFile(strFileLoc + "GroupInviter.RDconf", mGroupInviter);
	saveFile(strFileLoc + "DisabledGroup.RDconf", DisabledGroup);
	saveFile(strFileLoc + "DisabledDiscuss.RDconf", DisabledDiscuss);
	saveFile(strFileLoc + "DisabledJRRPGroup.RDconf", DisabledJRRPGroup);
	saveFile(strFileLoc + "DisabledJRRPDiscuss.RDconf", DisabledJRRPDiscuss);
	saveFile(strFileLoc + "DisabledMEGroup.RDconf", DisabledMEGroup);
	saveFile(strFileLoc + "DisabledMEDiscuss.RDconf", DisabledMEDiscuss);
	saveFile(strFileLoc + "DisabledHELPGroup.RDconf", DisabledHELPGroup);
	saveFile(strFileLoc + "DisabledHELPDiscuss.RDconf", DisabledHELPDiscuss);
	saveFile(strFileLoc + "DisabledOBGroup.RDconf", DisabledOBGroup);
	saveFile(strFileLoc + "DisabledOBDiscuss.RDconf", DisabledOBDiscuss);
	saveFile(strFileLoc + "ObserveGroup.RDconf", ObserveGroup);
	saveFile(strFileLoc + "ObserveDiscuss.RDconf", ObserveDiscuss);
	saveFile(strFileLoc + "WelcomeMsg.RDconf", WelcomeMsg);
	saveFile(strFileLoc + "DefaultCOC.MYmap", mDefaultCOC);
	saveFile("DiceData\\user\\PlayerCards.RDconf", PList);
	Name->save();
	ilInitList->save();
}
EVE_Enable(eventEnable)
{
	llStartTime = clock();
	//Wait until the thread terminates
	while (msgSendThreadRunning)
		Sleep(10);

	thread msgSendThread(SendMsg);
	msgSendThread.detach();
	thread threadConsoleTimer(ConsoleTimer);
	threadConsoleTimer.detach();
	thread threadWarning(warningHandler);
	threadWarning.detach();
	thread threadFrq(frqHandler);
	threadFrq.detach();
	strFileLoc = getAppDirectory();
	DiceMaid = getLoginQQ();
	/*
	* 名称存储-创建与读取
	*/
	Name = make_unique<NameStorage>(strFileLoc + "Name.dicedb");
	//读取MasterMode
	ifstream ifstreamMaster(strFileLoc + "Master.RDconf");
	if (ifstreamMaster)
	{
		ifstreamMaster >> masterQQ >> boolMasterMode >> boolConsole["DisabledGlobal"] >> boolConsole["DisabledMe"] >> boolConsole["Private"] >> boolConsole["DisabledJrrp"] >> boolConsole["LeaveDiscuss"]
			>> ClockToWork.first >> ClockToWork.second >> ClockOffWork.first >> ClockOffWork.second;
	}
	ifstreamMaster.close();
	//读取管理员列表
	loadFile(strFileLoc + "AdminQQ.RDconf", AdminQQ);
	if(AdminQQ.upper_bound(0)!= AdminQQ.begin()) {
		AdminQQ.erase(AdminQQ.begin(), AdminQQ.upper_bound(0));
	}
	loadFile("DiceData\\conf\\RecorderList.RDconf", RecorderList);
	AdminQQ.insert(masterQQ);
	//读取监控窗口列表
	loadFile(strFileLoc + "MonitorList.RDconf", MonitorList);
	if (MonitorList.size() == 0) {
		for (auto it : AdminQQ) {
			MonitorList.insert({ it ,Private });
		}
		MonitorList.insert({ 863062599 ,Group });
		MonitorList.insert({ 192499947 ,Group });
		MonitorList.insert({ 754494359 ,Group });
	}
	//获取boolConsole
	loadJMap(strFileLoc + "boolConsole.json", boolConsole);

	loadFile(strFileLoc + "WhiteGroup.RDconf", WhiteGroup);
	loadFile(strFileLoc + "WhiteQQ.RDconf", WhiteQQ);
	loadFile(strFileLoc + "BlackGroup.RDconf", BlackGroup);
	loadFile(strFileLoc + "BlackQQ.RDconf", BlackQQ);
	loadBlackMark(strFileLoc + "BlackMarks.json");
	loadFile(strFileLoc + "DisabledGroup.RDconf", DisabledGroup);
	loadFile(strFileLoc + "DisabledDiscuss.RDconf", DisabledDiscuss);
	loadFile(strFileLoc + "DisabledJRRPGroup.RDconf", DisabledJRRPGroup);
	loadFile(strFileLoc + "DisabledJRRPDiscuss.RDconf", DisabledJRRPDiscuss);
	loadFile(strFileLoc + "DisabledMEGroup.RDconf", DisabledMEGroup);
	loadFile(strFileLoc + "DisabledMEDiscuss.RDconf", DisabledMEDiscuss);
	loadFile(strFileLoc + "DisabledHELPGroup.RDconf", DisabledHELPGroup);
	loadFile(strFileLoc + "DisabledHELPDiscuss.RDconf", DisabledHELPDiscuss);
	loadFile(strFileLoc + "DisabledOBGroup.RDconf", DisabledOBGroup);
	loadFile(strFileLoc + "DisabledOBDiscuss.RDconf", DisabledOBDiscuss);
	loadFile(strFileLoc + "ObserveGroup.RDconf", ObserveGroup);
	loadFile(strFileLoc + "ObserveDiscuss.RDconf", ObserveDiscuss);
	loadFile(strFileLoc + "Default.RDconf", DefaultDice);
	loadFile(strFileLoc + "DefaultRule.RDconf", DefaultRule);
	loadFile(strFileLoc + "WelcomeMsg.RDconf", WelcomeMsg);
	//读取帮助文档
	HelpDoc["master"] = printQQ(masterQQ);
	ifstream ifstreamHelpDoc(strFileLoc + "HelpDoc.txt");
	if (ifstreamHelpDoc)
	{
		string strName, strMsg ,strDebug;
		while (ifstreamHelpDoc) {
			getline(ifstreamHelpDoc, strName);
			getline(ifstreamHelpDoc, strMsg);
			while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
			while (strMsg.find("\\s") != string::npos)strMsg.replace(strMsg.find("\\s"), 2, " ");
			while (strMsg.find("\\t") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, "	");
			EditedHelpDoc[strName] = strMsg;
			HelpDoc[strName] = strMsg;
		}
	}
	ifstreamHelpDoc.close();
	//读取聊天列表
	loadFile(strFileLoc + "LastMsgList.MYmap", mLastMsgList);
	//读取邀请者列表
	loadFile(strFileLoc + "GroupInviter.RDconf", mGroupInviter);
	//读取COC房规
	loadFile(strFileLoc + "DefaultCOC.MYmap", mDefaultCOC);
	ilInitList = make_unique<Initlist>(strFileLoc + "INIT.DiceDB");
	GlobalMsg["strSelfName"] = getLoginNick();
	if (loadJMap("DiceData\\conf\\CustomMsg.json", EditedMsg) < 0)loadJMap(strFileLoc + "CustomMsg.json", EditedMsg);
	//预修改出场回复文本
	for (auto it : EditedMsg) {
		GlobalMsg[it.first] = it.second;
	}
	loadData();
	if (loadFile("DiceData\\user\\PlayerCards.RDconf", PList) < 1) {
		ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
		if (ifstreamCharacterProp)
		{
			long long QQ, GrouporDiscussID;
			int Type, Value;
			string SkillName;
			while (ifstreamCharacterProp >> QQ >> Type >> GrouporDiscussID >> SkillName >> Value)
			{
				if (SkillName == "智力/灵感")SkillName = "智力";
				getPlayer(QQ)[0].set(SkillName, Value);
			}
		}
		ifstreamCharacterProp.close();
	}
	//读取卡牌
	loadJMap(strFileLoc + "GroupDeck.json",CardDeck::mGroupDeck);
	loadJMap(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	loadJMap(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	loadJMap(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
	//读取替身模式
	ifstream ifstreamStandByMe(strFileLoc + "StandByMe.RDconf");
	if (ifstreamStandByMe)
	{
		ifstreamStandByMe >> IdentityQQ >> StandQQ;
		if (getLoginQQ() == StandQQ) {
			boolStandByMe = true;
			masterQQ = IdentityQQ;
			string strName,strMsg;
			while (ifstreamStandByMe >> strName) {
				getline(ifstreamStandByMe, strMsg);
				while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
				while (strMsg.find("\\s") != string::npos)strMsg.replace(strMsg.find("\\s"), 2, " ");
				while (strMsg.find("\\t") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, "	");
				GlobalMsg[strName] = strMsg;
			}
		}
	}
	ifstreamStandByMe.close();
	//骰娘网络
	getDiceList();
	if (masterQQ)Cloud::update();
	addRecord(GlobalMsg["strSelfName"] + "初始化完成，用时" + to_string((clock() - llStartTime) / 1000) + "秒");
	llStartTime = clock();
	return 0;
}

//处理骰子指令


EVE_PrivateMsg_EX(eventPrivateMsg)
{
	FromMsg Msg(eve.message, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_GroupMsg_EX(eventGroupMsg)
{
	if (eve.isAnonymous())return;
	if (eve.isSystem())return;
	FromMsg Msg(eve.message, eve.fromGroup, Group, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_DiscussMsg_EX(eventDiscussMsg)
{
	time_t tNow = time(NULL);
	if (boolConsole["LeaveDiscuss"]) {
		sendDiscussMsg(eve.fromDiscuss, format(GlobalMsg["strLeaveDiscuss"], GlobalMsg));
		Sleep(1000);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	if (BlackQQ.count(eve.fromQQ) && boolConsole["AutoClearBlack"]) {
		string strMsg = "发现黑名单用户" + printQQ(eve.fromQQ) + "，自动执行退群";
		sendDiscussMsg(eve.fromDiscuss, strMsg);
		sendAdmin(printChat({ eve.fromDiscuss,Discuss }) + strMsg);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	FromMsg Msg(eve.message, eve.fromDiscuss, Discuss, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_System_GroupMemberIncrease(eventGroupMemberIncrease)
{
	if (beingOperateQQ != getLoginQQ() && WelcomeMsg.count(fromGroup))
	{
		string strReply = WelcomeMsg[fromGroup];
		while (strReply.find("{@}") != string::npos)
		{
			strReply.replace(strReply.find("{@}"), 3, "[CQ:at,qq=" + to_string(beingOperateQQ) + "]");
		}
		while (strReply.find("{nick}") != string::npos)
		{
			strReply.replace(strReply.find("{nick}"), 6, getStrangerInfo(beingOperateQQ).nick);
		}
		while (strReply.find("{age}") != string::npos)
		{
			strReply.replace(strReply.find("{age}"), 5, to_string(getStrangerInfo(beingOperateQQ).age));
		}
		while (strReply.find("{sex}") != string::npos)
		{
			strReply.replace(strReply.find("{sex}"), 5,
			                 getStrangerInfo(beingOperateQQ).sex == 0
				                 ? "男"
				                 : getStrangerInfo(beingOperateQQ).sex == 1
				                 ? "女"
				                 : "未知");
		}
		while (strReply.find("{qq}") != string::npos)
		{
			strReply.replace(strReply.find("{qq}"), 4, to_string(beingOperateQQ));
		}
		AddMsgToQueue(strReply, fromGroup, Group);
	}
	if (beingOperateQQ != getLoginQQ() && BlackQQ.count(beingOperateQQ)) {
		string strNote = printGroup(fromGroup) + "发现黑名单用户" + printQQ(beingOperateQQ) + "入群";
		if (mBlackQQMark.count(beingOperateQQ) && mBlackQQMark[beingOperateQQ].isVal("DiceMaid", getLoginQQ()))AddMsgToQueue(mBlackQQMark[beingOperateQQ].getWarning(), fromGroup, Group);
		if (WhiteGroup.count(fromGroup))strNote += "（群在白名单中）";
		else if (MonitorList.count({ fromGroup,Group }));
		else if (getGroupMemberInfo(fromGroup, getLoginQQ()).permissions > 1)strNote += "（群内有权限）";
		else if (boolConsole["LeaveBlackQQ"]) {
			sendGroupMsg(fromGroup, "发现黑名单用户" + printQQ(beingOperateQQ) + "入群,将预防性退群");
			strNote += "（已退群）";
			Sleep(100);
			setGroupLeave(fromGroup);
		}
		sendAdmin(strNote);
	}
	else if(beingOperateQQ == getLoginQQ()){
		if (!boolConsole["ListenGroupAdd"])return 0;
		string strMsg = "新加入" + printGroup(fromGroup);
		if (BlackGroup.count(fromGroup)) {
			if (mBlackGroupMark.count(fromGroup))sendGroupMsg(fromGroup, mBlackGroupMark[fromGroup].getWarning());
			else sendGroupMsg(fromGroup, GlobalMsg["strBlackGroup"]);
			strMsg += "为黑名单群，已退群";
			sendAdmin(strMsg);
			setGroupLeave(fromGroup);
			return 1;
		}
		if (WhiteGroup.count(fromGroup))strMsg += "（已在白名单中）";
		if (mGroupInviter.count(fromGroup)) {
			strMsg += ",邀请者" + printQQ(mGroupInviter[fromGroup]);
		}
		long long ownerQQ = 0;
		set<long long>sBlackList;
		std::vector<GroupMemberInfo>list = getGroupMemberList(fromGroup);
		if (list.empty()) {
			strMsg += "，群员名单未加载；";
		}
		else {
			for (auto each : list) {
				if (each.permissions > 1) {
					if (BlackQQ.count(each.QQID)) {
						if (mBlackQQMark.count(each.QQID))sendGroupMsg(fromGroup, mBlackQQMark[each.QQID].getWarning());
						else sendGroupMsg(fromGroup, "发现黑名单管理员" + printQQ(each.QQID) + "将预防性退群");
						strMsg += ",发现黑名单管理员" + printQQ(each.QQID) + "，已退群";
						sendAdmin(strMsg);
						setGroupLeave(fromGroup);
						return 1;
					}
					if (each.permissions == 3) {
						ownerQQ = each.QQID;
						strMsg += "，群主" + printQQ(each.QQID) + "；";
					}
				}
				else if (BlackQQ.count(each.QQID)) {
					sBlackList.insert(each.QQID);
					if (mBlackQQMark.count(each.QQID) && mBlackQQMark[each.QQID].isVal("DiceMaid", DiceMaid)) {
						AddMsgToQueue(mBlackQQMark[each.QQID].getWarning(), fromGroup, Group);
					}
				}
			}
			if (!mGroupInviter.count(fromGroup) && getGroupMemberList(fromGroup).size() == 2 && ownerQQ) {
				mGroupInviter[fromGroup] = ownerQQ;
				strMsg += "邀请者" + printQQ(ownerQQ);
			}
		}
		if (!sBlackList.empty()) {
			string strNote = "发现黑名单群员";
			for (auto it : sBlackList) {
				strNote += "\n" + printQQ(it);
			}
			AddMsgToQueue(strNote + "\n已通知管理员", fromGroup, Group);
			strMsg += strNote;
		}
		if (boolConsole["Private"] && WhiteGroup.count(fromGroup) == 0)
		{	//避免小群绕过邀请没加上白名单
			if (WhiteQQ.count(fromQQ) || WhiteQQ.count(ownerQQ) || getGroupMemberInfo(fromGroup, masterQQ).QQID == masterQQ) {
				WhiteGroup.insert(fromGroup);
				strMsg += "已自动添加群白名单";
			}
			else {
				sendGroupMsg(fromGroup, GlobalMsg["strPreserve"]);
				strMsg += "无白名单，已退群";
				addRecord(strMsg);
				setGroupLeave(fromGroup);
				return 1;
			}
		}
		if (sBlackList.size())sendAdmin(strMsg);
		else addRecord(strMsg);
		if(!GlobalMsg["strAddGroup"].empty()) {
			AddMsgToQueue(GlobalMsg["strAddGroup"], fromGroup, Group);
		}
	}
	return 0;
}

EVE_System_GroupMemberDecrease(eventGroupMemberDecrease) {
	if (beingOperateQQ == getLoginQQ() && boolMasterMode) {
		string strNow = printSTime(stNow);
		mLastMsgList.erase({ fromGroup ,Group });
		if (AdminQQ.count(fromQQ))return 1;
		string strNote = strNow + " " + printQQ(fromQQ) + "将" + GlobalMsg["strSelfName"] + "移出了群" + to_string(fromGroup);
		addRecord(strNote);
		if (!boolConsole["ListenGroupKick"])return 0;
		BlackMark mark;
		mark.llMap = { {"fromGroup",fromGroup},{"fromQQ",fromQQ},{"DiceMaid",getLoginQQ()},{"masterQQ", masterQQ} };
		mark.strMap = { {"type","kick"},{"time",strNow}};
		mark.set("note", strNote);
		Cloud::upWarning(mark.getData());
		if (mGroupInviter.count(fromGroup) && AdminQQ.count(mGroupInviter[fromGroup]) == 0) {
			long long llInviter = mGroupInviter[fromGroup];
			strNote += ";入群邀请者：" + printQQ(llInviter);
			mark.llMap["inviterQQ"] = llInviter;
			mark.set("note", strNote);
			mark.getWarning();
			if (boolConsole["KickedBanInviter"])addBlackQQ(BlackMark(mark, "inviterQQ"));
		}
		if (WhiteGroup.count(fromGroup)) {
			WhiteGroup.erase(fromGroup);
		}
		BlackGroup.insert(fromGroup);
		NotifyMonitor(mark.getWarning());
		addBlackQQ(BlackMark(mark, "fromQQ"));
	}
	else if (mDiceList.count(beingOperateQQ) && subType == 2 && boolConsole["ListenGroupKick"]) {
		string strNow = printSTime(stNow);
		string strNote = strNow + " " + printQQ(fromQQ) + "将" + printQQ(beingOperateQQ) + "移出了群" + to_string(fromGroup);
		while (strNote.find('\"') != string::npos)strNote.replace(strNote.find('\"'), 1, "\'"); 
		string strWarning = "!warning{\n\"fromGroup\":" + to_string(fromGroup) + ",\n\"type\":\"kick\",\n\"fromQQ\":" + to_string(fromQQ) + ",\n\"time\":\"" + strNow + "\",\n\"DiceMaid\":" + to_string(beingOperateQQ) + ",\n\"masterQQ\":" + to_string(masterQQ) + ",\n\"note\":\"" + strNote + "\"\n}";
		BlackMark mark;
		mark.llMap = { {"fromGroup",fromGroup},{"fromQQ",fromQQ},{"DiceMaid",beingOperateQQ},{"masterQQ", mDiceList[beingOperateQQ]} };
		mark.strMap = { {"type","kick"},{"time",strNow},{"note",strNote} };
		Cloud::upWarning(mark.getData());
		NotifyMonitor(strWarning);
		addBlackGroup(mark);;
		addBlackQQ(BlackMark(mark, "fromQQ"));
	}
	return 0;
}

EVE_System_GroupBan(eventGroupBan) {
	if (beingOperateQQ != DiceMaid && !mDiceList.count(beingOperateQQ) || !boolConsole["ListenGroupBan"])return 0;
	if (subType == 1) {
		if (beingOperateQQ == DiceMaid) {
			sendAdmin(GlobalMsg["strSelfName"] + "在" + printGroup(fromGroup) + "中被解除禁言");
			return 1;
		}
	}
	else {
		string strNow = printSTime(stNow);
		long long llOwner = 0;
		string strNote = strNow + "在" + printGroup(fromGroup) + "中," + (beingOperateQQ == DiceMaid ? GlobalMsg["strSelfName"] : printQQ(beingOperateQQ)) + "被" + printQQ(fromQQ) + "禁言" + to_string(duration) + "秒";
		//AdminQQ豁免
		if (AdminQQ.count(fromQQ)) {
			sendAdmin(strNote);
			return 1;
		}
		//统计群内管理
		int intAuthCnt = 0;
		string strAuthList;
		for (auto member : getGroupMemberList(fromGroup)) {
			if (member.permissions == 3) {
				llOwner = member.QQID;
			}
			else if (member.permissions == 2) {
				strAuthList += '\n' + member.Nick + "(" + to_string(member.QQID) + ")";
				intAuthCnt++;
			}
		}
		strAuthList = "；群主" + printQQ(llOwner) + ",另有管理员" + to_string(intAuthCnt) + "名" + strAuthList;
		BlackMark mark;
		mark.llMap = { {"fromGroup",fromGroup},{"DiceMaid",beingOperateQQ} };
		mark.strMap = { {"type","ban"},{"time",strNow} };
		if (!mDiceList.count(fromQQ))mark.set("fromQQ", fromQQ);
		Cloud::upWarning(mark.getData());
		mark.set("masterQQ", beingOperateQQ == DiceMaid ? masterQQ : mDiceList[beingOperateQQ]);
		mark.set("note", strNote);
		if (mGroupInviter.count(fromGroup) && !AdminQQ.count(mGroupInviter[fromGroup])) {
			long long llInviter = mGroupInviter[fromGroup];
			strNote += ";入群邀请者：" + printQQ(llInviter);
			mark.set("inviterQQ", llInviter);
			mark.set("note", strNote);
			if (boolConsole["BannedBanInviter"])addBlackQQ(BlackMark(mark, "inviterQQ"));
		}
		NotifyMonitor(strNote + strAuthList);
		NotifyMonitor(mark.getWarning());
		if (boolConsole["BannedLeave"]) {
			CQ::setGroupLeave(fromGroup);
			mLastMsgList.erase({ fromGroup ,Group });
		}
		addBlackGroup(mark);
		return 1;
	}
	return 0;
}

EVE_Request_AddGroup(eventGroupInvited) {
	if (!boolConsole["ListenGroupRequest"])return 0;
	if (subType == 2) {
		if (masterQQ&&boolMasterMode) {
			string strMsg = "群添加请求，来自：" + getStrangerInfo(fromQQ).nick +"("+ to_string(fromQQ) + "),群：(" + to_string(fromGroup)+")。";
			if (BlackGroup.count(fromGroup)) {
				strMsg += "\n已拒绝（群在黑名单中）";
				setGroupAddRequest(responseFlag, 2, 2, "");
				sendAdmin(strMsg);
				return 1;
			}
			else if (BlackQQ.count(fromQQ)) {
				strMsg += "\n已拒绝（用户在黑名单中）";
				setGroupAddRequest(responseFlag, 2, 2, "");
				sendAdmin(strMsg);
				return 1;
			}
			else if (WhiteGroup.count(fromGroup)) {
				strMsg += "\n已同意（群在白名单中）";
				mGroupInviter[fromGroup] = fromQQ;
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			else if (WhiteQQ.count(fromQQ)) {
				strMsg += "\n已同意（用户在白名单中）";
				WhiteGroup.insert(fromGroup);
				mGroupInviter[fromGroup] = fromQQ;
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			else if (boolConsole["Private"]) {
				strMsg += "\n已拒绝（当前在私用模式）";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else{
				strMsg += "已同意";
				mGroupInviter[fromGroup] = fromQQ;
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			addRecord(strMsg);
		}
		return 1;
	}
	return 0;
}

EVE_Menu(eventMasterMode) {
	if (boolMasterMode) {
		boolMasterMode = false;
		masterQQ = 0;
		AdminQQ.clear();
		MonitorList.clear();
		MessageBoxA(nullptr, "Master模式已关闭√\nmaster已清除", "Master模式切换",MB_OK | MB_ICONINFORMATION);
	}else {
		boolMasterMode = true;
		MessageBoxA(nullptr, "Master模式已开启√", "Master模式切换", MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

EVE_Disable(eventDisable)
{
	Enabled = false;
	dataBackUp();
	ilInitList.reset();
	Name.reset();
	EditedMsg.clear(); 
	WhiteGroup.clear();
	BlackGroup.clear();
	WhiteQQ.clear();
	BlackQQ.clear();
	mDefaultCOC.clear();
	DefaultDice.clear();
	DefaultRule.clear();
	DisabledGroup.clear();
	DisabledDiscuss.clear();
	DisabledJRRPGroup.clear();
	DisabledJRRPDiscuss.clear();
	DisabledMEGroup.clear();
	DisabledMEDiscuss.clear();
	DisabledOBGroup.clear();
	DisabledOBDiscuss.clear();
	ObserveGroup.clear();
	ObserveDiscuss.clear();
	return 0;
}

EVE_Exit(eventExit)
{
	if (!Enabled)
		return 0;
	dataBackUp();
	return 0;
}

MUST_AppInfo_RETURN(CQAPPID);
