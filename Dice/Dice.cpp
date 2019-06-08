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
#include "RandomGenerator.h"
#include "RD.h"
#include "CQEVE_ALL.h"
#include "InitList.h"
#include "GlobalVar.h"
#include "NameStorage.h"
#include "GetRule.h"
#include "DiceMsgSend.h"
#include "CustomMsg.h"
#include "NameGenerator.h"
#include "MsgFormat.h"
#include "DiceNetwork.h"
#include "CardDeck.h"
#include "DiceConsole.h"
#include "EncodingConvert.h"
#include "DiceEvent.h"

using namespace std;
using namespace CQ;

unique_ptr<NameStorage> Name;


//Master模式
bool boolMasterMode = false;
//替身模式
bool boolStandByMe = false;
//本体与替身帐号
long long IdentityQQ = 0;
long long StandQQ = 0;
map<long long, int> DefaultDice;
map<chatType, int> mDefaultCOC;
map<long long, string> WelcomeMsg;
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

using PropType = map<string, int>;
map<SourceType, PropType> CharacterProp;
multimap<long long, long long> ObserveGroup;
multimap<long long, long long> ObserveDiscuss;
string strFileLoc;

//备份数据
void dataBackUp() {
	//备份MasterQQ
	if (!boolStandByMe) {
		ofstream ofstreamMaster(strFileLoc + "Master.RDconf", ios::out | ios::trunc);
		ofstreamMaster << masterQQ << std::endl << boolMasterMode << std::endl << boolDisabledGlobal << std::endl << boolDisabledMeGlobal << std::endl << boolPreserve << std::endl << boolDisabledJrrpGlobal << std::endl << boolNoDiscuss << std::endl;
		ofstreamMaster << ClockToWork.first << " " << ClockToWork.second << endl
			<< ClockOffWork.first << " " << ClockOffWork.second << endl;
		ofstreamMaster.close();
	}
	//备份个性化语句
	ofstream ofstreamPersonalMsg(strFileLoc + "PersonalMsg.RDconf", ios::out | ios::trunc);
	for (auto it = PersonalMsg.begin(); it != PersonalMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamPersonalMsg << it->first << std::endl << it->second << std::endl;
	}
	//备份CustomMsg
	ofstream ofstreamCustomMsg(strFileLoc + "CustomMsg.json", ios::out | ios::trunc);
	ofstreamCustomMsg << "{\n";
	for (auto it : EditedMsg)
	{
		while (it.second.find("\r\n") != string::npos)it.second.replace(it.second.find("\r\n"), 2, "\\n");
		while (it.second.find('\n') != string::npos)it.second.replace(it.second.find('\n'), 1, "\\n");
		while (it.second.find('\r') != string::npos)it.second.replace(it.second.find('\r'), 1, "\\r");
		while (it.second.find("\t") != string::npos)it.second.replace(it.second.find("\t"), 1, "\\t");
		string strMsg = GBKtoUTF8(it.second);
		ofstreamCustomMsg <<"\"" << it.first << "\":\"" << strMsg << "\",";
	}
	ofstreamCustomMsg << "\n\"Number\":\"" << EditedMsg.size() << "\"\n}" << endl;
	//备份默认规则
	ofstream ofstreamDefaultRule(strFileLoc + "DefaultRule.RDconf", ios::out | ios::trunc);
	for (auto it = DefaultRule.begin(); it != DefaultRule.end(); ++it)
	{
		ofstreamDefaultRule << it->first << std::endl << it->second << std::endl;
	}
	//备份白名单群
	ofstream ofstreamWhiteGroup(strFileLoc + "WhiteGroup.RDconf", ios::out | ios::trunc);
	for (auto it = WhiteGroup.begin(); it != WhiteGroup.end(); ++it)
	{
		ofstreamWhiteGroup << *it << std::endl;
	}
	ofstreamWhiteGroup.close();
	//备份黑名单群
	ofstream ofstreamBlackGroup(strFileLoc + "BlackGroup.RDconf", ios::out | ios::trunc);
	for (auto it : BlackGroup)
	{
		ofstreamBlackGroup << it << std::endl;
	}
	ofstreamBlackGroup.close();
	//备份白名单用户
	ofstream ofstreamWhiteQQ(strFileLoc + "WhiteQQ.RDconf", ios::out | ios::trunc);
	for (auto it : WhiteQQ)
	{
		ofstreamWhiteQQ << it << std::endl;
	}
	ofstreamWhiteQQ.close();
	//备份黑名单用户
	ofstream ofstreamBlackQQ(strFileLoc + "BlackQQ.RDconf", ios::out | ios::trunc);
	for (auto it : BlackQQ)
	{
		ofstreamBlackQQ << it << std::endl;
	}
	ofstreamBlackQQ.close();
	//备份讨论组列表
	ofstream ofstreamDiscussList(strFileLoc + "DiscussList.map", ios::out | ios::trunc);
	for (auto it : DiscussList)
	{
		ofstreamDiscussList << it.first << "\n" << it.second << std::endl;
	}
	ofstreamDiscussList.close();
	//备份聊天列表
	ofstream ofstreamLastMsgList(strFileLoc + "LastMsgList.MYmap", ios::out | ios::trunc);
	for (auto it : mLastMsgList)
	{
		ofstreamLastMsgList << it.first.first << " " << it.first.second << " "<< it.second << std::endl;
	}
	ofstreamLastMsgList.close();
	//备份默认COC房规
	ofstream ofstreamDefaultCOC(strFileLoc + "DefaultCOC.MYmap", ios::out | ios::trunc);
	for (auto it : mDefaultCOC)
	{
		ofstreamDefaultCOC << it.first.first << " " << it.first.second << " " << it.second << std::endl;
	}
	ofstreamDefaultCOC.close();
	//保存卡牌
	CardDeck::saveDeck(strFileLoc + "GroupDeck.json", CardDeck::mGroupDeck);
	CardDeck::saveDeck(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	CardDeck::saveDeck(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	CardDeck::saveDeck(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
}
EVE_Enable(eventEnable)
{
	//Wait until the thread terminates
	while (msgSendThreadRunning)
		Sleep(10);

	thread msgSendThread(SendMsg);
	msgSendThread.detach();
	thread threadConsoleTimer(ConsoleTimer);
	threadConsoleTimer.detach();
	strFileLoc = getAppDirectory();
	/*
	* 名称存储-创建与读取
	*/
	Name = make_unique<NameStorage>(strFileLoc + "Name.dicedb");
	//读取MasterMode
	ifstream ifstreamMaster(strFileLoc + "Master.RDconf");
	if (ifstreamMaster)
	{
		ifstreamMaster >> masterQQ >> boolMasterMode >> boolDisabledGlobal >> boolDisabledMeGlobal >> boolPreserve >> boolDisabledJrrpGlobal >> boolNoDiscuss
			>> ClockToWork.first >> ClockToWork.second >> ClockOffWork.first >> ClockOffWork.second;
	}
	ifstreamMaster.close();
	ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
	if (ifstreamCharacterProp)
	{
		long long QQ, GrouporDiscussID;
		int Type, Value;
		string SkillName;
		while (ifstreamCharacterProp >> QQ >> Type >> GrouporDiscussID >> SkillName >> Value)
		{
			CharacterProp[SourceType(QQ, Type, GrouporDiscussID)][SkillName] = Value;
		}
	}
	ifstreamCharacterProp.close();

	ifstream ifstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf");
	if (ifstreamDisabledGroup)
	{
		long long Group;
		while (ifstreamDisabledGroup >> Group)
		{
			DisabledGroup.insert(Group);
		}
	}
	ifstreamDisabledGroup.close();
	ifstream ifstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf");
	if (ifstreamDisabledDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledDiscuss >> Discuss)
		{
			DisabledDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledDiscuss.close();
	ifstream ifstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf");
	if (ifstreamDisabledJRRPGroup)
	{
		long long Group;
		while (ifstreamDisabledJRRPGroup >> Group)
		{
			DisabledJRRPGroup.insert(Group);
		}
	}
	ifstreamDisabledJRRPGroup.close();
	ifstream ifstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf");
	if (ifstreamDisabledJRRPDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledJRRPDiscuss >> Discuss)
		{
			DisabledJRRPDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledJRRPDiscuss.close();
	ifstream ifstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf");
	if (ifstreamDisabledMEGroup)
	{
		long long Group;
		while (ifstreamDisabledMEGroup >> Group)
		{
			DisabledMEGroup.insert(Group);
		}
	}
	ifstreamDisabledMEGroup.close();
	ifstream ifstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf");
	if (ifstreamDisabledMEDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledMEDiscuss >> Discuss)
		{
			DisabledMEDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledMEDiscuss.close();
	ifstream ifstreamDisabledHELPGroup(strFileLoc + "DisabledHELPGroup.RDconf");
	if (ifstreamDisabledHELPGroup)
	{
		long long Group;
		while (ifstreamDisabledHELPGroup >> Group)
		{
			DisabledHELPGroup.insert(Group);
		}
	}
	ifstreamDisabledHELPGroup.close();
	ifstream ifstreamDisabledHELPDiscuss(strFileLoc + "DisabledHELPDiscuss.RDconf");
	if (ifstreamDisabledHELPDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledHELPDiscuss >> Discuss)
		{
			DisabledHELPDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledHELPDiscuss.close();
	ifstream ifstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf");
	if (ifstreamDisabledOBGroup)
	{
		long long Group;
		while (ifstreamDisabledOBGroup >> Group)
		{
			DisabledOBGroup.insert(Group);
		}
	}
	ifstreamDisabledOBGroup.close();
	ifstream ifstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf");
	if (ifstreamDisabledOBDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledOBDiscuss >> Discuss)
		{
			DisabledOBDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledOBDiscuss.close();
	ifstream ifstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf");
	if (ifstreamObserveGroup)
	{
		long long Group, QQ;
		while (ifstreamObserveGroup >> Group >> QQ)
		{
			ObserveGroup.insert(make_pair(Group, QQ));
		}
	}
	ifstreamObserveGroup.close();

	ifstream ifstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf");
	if (ifstreamObserveDiscuss)
	{
		long long Discuss, QQ;
		while (ifstreamObserveDiscuss >> Discuss >> QQ)
		{
			ObserveDiscuss.insert(make_pair(Discuss, QQ));
		}
	}
	ifstreamObserveDiscuss.close();
	ifstream ifstreamDefault(strFileLoc + "Default.RDconf");
	if (ifstreamDefault)
	{
		long long QQ;
		int DefVal;
		while (ifstreamDefault >> QQ >> DefVal)
		{
			DefaultDice[QQ] = DefVal;
		}
	}
	ifstreamDefault.close();
	ifstream ifstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf");
	if (ifstreamWelcomeMsg)
	{
		long long GroupID;
		string Msg;
		while (ifstreamWelcomeMsg >> GroupID >> Msg)
		{
			while (Msg.find("{space}") != string::npos)Msg.replace(Msg.find("{space}"), 7, " ");
			while (Msg.find("{tab}") != string::npos)Msg.replace(Msg.find("{tab}"), 5, "\t");
			while (Msg.find("{endl}") != string::npos)Msg.replace(Msg.find("{endl}"), 6, "\n");
			while (Msg.find("{enter}") != string::npos)Msg.replace(Msg.find("{enter}"), 7, "\r");
			WelcomeMsg[GroupID] = Msg;
		}
	}
	ifstreamWelcomeMsg.close();
	ifstream ifstreamDefaultRule(strFileLoc + "DefaultRule.RDconf");
	if (ifstreamDefaultRule)
	{
		long long QQ;
		string strRule;
		while (ifstreamWelcomeMsg >> QQ >> strRule){
			DefaultRule[QQ] = strRule;
		}
	}
	ifstreamDefaultRule.close();
	ifstream ifstreamPersonalMsg(strFileLoc + "PersonalMsg.RDconf");
	if (ifstreamPersonalMsg)
	{
		string strType;
		string Msg;
		while (ifstreamPersonalMsg >> strType >> Msg)
		{
			while (Msg.find("{space}") != string::npos)Msg.replace(Msg.find("{space}"), 7, " ");
			while (Msg.find("{tab}") != string::npos)Msg.replace(Msg.find("{tab}"), 5, "\t");
			while (Msg.find("{endl}") != string::npos)Msg.replace(Msg.find("{endl}"), 6, "\n");
			while (Msg.find("{enter}") != string::npos)Msg.replace(Msg.find("{enter}"), 7, "\r");
			PersonalMsg[strType] = Msg;
		}
	}
	ifstreamPersonalMsg.close();
	//读取帮助文档
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
	ifstream ifstreamWhiteGroup(strFileLoc + "WhiteGroup.RDconf");
	if (ifstreamWhiteGroup)
	{
		long long Group;
		while (ifstreamWhiteGroup >> Group)
		{
			WhiteGroup.insert(Group);
		}
	}
	ifstreamWhiteGroup.close();
	ifstream ifstreamBlackGroup(strFileLoc + "BlackGroup.RDconf");
	if (ifstreamBlackGroup)
	{
		long long Group;
		while (ifstreamBlackGroup >> Group)
		{
			BlackGroup.insert(Group);
		}
	}
	ifstreamBlackGroup.close();
	ifstream ifstreamWhiteQQ(strFileLoc + "WhiteQQ.RDconf");
	if (ifstreamWhiteQQ)
	{
		long long Group;
		while (ifstreamWhiteQQ >> Group)
		{
			WhiteQQ.insert(Group);
		}
	}
	ifstreamWhiteQQ.close();
	ifstream ifstreamBlackQQ(strFileLoc + "BlackQQ.RDconf");
	if (ifstreamBlackQQ)
	{
		long long Group;
		while (ifstreamBlackQQ >> Group)
		{
			BlackQQ.insert(Group);
		}
	}
	ifstreamBlackQQ.close();
	//读取讨论组列表
	ifstream ifstreamDiscussList(strFileLoc + "DiscussList.map");
	if (ifstreamDiscussList)
	{
		long long llDiscuss;
		time_t tNow;
		while (ifstreamDiscussList >> llDiscuss >> tNow)
		{
			DiscussList[llDiscuss]=tNow;
		}
	}
	ifstreamDiscussList.close();
	//读取聊天列表
	ifstream ifstreamLastMsgList(strFileLoc + "LastMsgList.MYmap");
	if (ifstreamLastMsgList)
	{
		long long llID;
		int intT;
		chatType ct;
		time_t tLast;
		while (ifstreamLastMsgList >> llID >> intT >> tLast)
		{
			ct = { llID,(msgtype)intT };
			mLastMsgList[ct] = tLast;
		}
	}
	ifstreamLastMsgList.close();
	//读取COC房规
	ifstream ifstreamDefaultCOC(strFileLoc + "DefaultCOC.MYmap");
	if (ifstreamDefaultCOC)
	{
		long long llID;
		int intT;
		chatType ct;
		int intRule;
		while (ifstreamDefaultCOC >> llID >> intT >> intRule)
		{
			ct = { llID,(msgtype)intT };
			mDefaultCOC[ct] = intRule;
		}
	}
	ifstreamDefaultCOC.close();
	ilInitList = make_unique<Initlist>(strFileLoc + "INIT.DiceDB");
	GlobalMsg["strSelfName"] = getLoginNick();
	ifstream ifstreamCustomMsg(strFileLoc + "CustomMsg.json");
	if (ifstreamCustomMsg)
	{
		ReadCustomMsg(ifstreamCustomMsg);
	}
	ifstreamCustomMsg.close();
	//预修改出场回复文本
	for (auto it : GlobalMsg) {
		string strMsg = it.second;
		while (strMsg.find("本机器人") != string::npos) {
			strMsg.replace(strMsg.find("本机器人"), 8, GlobalMsg["strSelfName"]);
		}
		GlobalMsg[it.first] = strMsg;
	}
	//读取卡牌
	CardDeck::loadDeck(strFileLoc + "GroupDeck.json",CardDeck::mGroupDeck);
	CardDeck::loadDeck(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	CardDeck::loadDeck(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	CardDeck::loadDeck(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
	//读取替身模式
	ifstream ifstreamStandByMe(strFileLoc + "StandByMe.RDconf");
	if (ifstreamStandByMe)
	{
		ifstreamStandByMe >> IdentityQQ >> StandQQ;
		if (getLoginQQ() == StandQQ) {
			boolStandByMe = true;
			masterQQ = IdentityQQ;
			string strName,strMsg;
			while (ifstreamStandByMe) {
				getline(ifstreamStandByMe, strName);
				getline(ifstreamStandByMe, strMsg);
				while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
				while (strMsg.find("\\s") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, " ");
				while (strMsg.find("\\t") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, "	");
				GlobalMsg[strName] = strMsg;
			}
		}
	}
	ifstreamStandByMe.close();
	return 0;
}

//处理骰子指令


EVE_PrivateMsg_EX(eventPrivateMsg)
{
	if (eve.isSystem()) {
		if (boolMasterMode&&masterQQ) {
			AddMsgToQueue("来自系统：" + eve.message, masterQQ);
		}
		return;
	}
	FromMsg Msg(eve.message, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_GroupMsg_EX(eventGroupMsg)
{
	if (eve.isAnonymous())return;
	if (eve.isSystem()) {
		if (eve.message.find("被管理员禁言") != string::npos&&eve.message.find(to_string(getLoginQQ())) != string::npos) {
			long long fromQQ;
			int intAuthCnt = 0;
			string strAuthList;
			for (auto member : getGroupMemberList(eve.fromGroup)) {
				if (member.permissions == 3) {
					//相应核心精神，由群主做负责人
					fromQQ = member.QQID;
				}
				else if (member.permissions == 2) {
					//记录管理员
					strAuthList += '\n' + member.Nick + "(" + to_string(member.QQID) + ")";
					intAuthCnt++;
				}
			}
			if (masterQQ&&boolMasterMode) {
				string strMsg = "在群\"" + getGroupList()[eve.fromGroup] + "\"(" + to_string(eve.fromGroup) + ")中," + eve.message + "\n群主" + getStrangerInfo(fromQQ).nick + "(" + to_string(fromQQ) + "),另有管理员" + to_string(intAuthCnt) + "名"+ strAuthList;
				AddMsgToQueue(strMsg, masterQQ);
				BlackGroup.insert(eve.fromGroup);
				if (WhiteGroup.count(eve.fromGroup))WhiteGroup.erase(eve.fromGroup);
				//setGroupLeave(eve.fromGroup);
			}
			string strInfo = "{\"LoginQQ\":\"" + to_string(getLoginQQ()) + "\",\"fromGroup\":" + to_string(eve.fromGroup) + "\",\"Type\":\"banned\",\"fromQQ\":\"" + to_string(fromQQ) + "\"";
		}
		else return;
	}
	FromMsg Msg(eve.message, eve.fromGroup, Group, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_DiscussMsg_EX(eventDiscussMsg)
{
	time_t tNow = time(NULL);
	if (boolNoDiscuss) {
		AddMsgToQueue(GlobalMsg["strNoDiscuss"], eve.fromDiscuss, Discuss);
		Sleep(1000);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	if (eve.isSystem()) {
		if (boolMasterMode&&masterQQ) {
			AddMsgToQueue("在讨论组"+to_string(eve.fromDiscuss)+"中，"+eve.message, masterQQ);
		}
		return;
	}
	DiscussList[eve.fromDiscuss] = tNow;
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
	else if(beingOperateQQ == getLoginQQ()){
		if (BlackGroup.count(fromGroup)) {
			AddMsgToQueue(GlobalMsg["strBlackGroup"], fromGroup, Group);
		}
		else if (boolPreserve&&WhiteGroup.count(fromGroup)==0) 
		{	//避免小群绕过邀请没加上白名单
			if (fromQQ == masterQQ || WhiteQQ.count(fromQQ) || getGroupMemberInfo(fromGroup, masterQQ).QQID == masterQQ) {
				WhiteGroup.insert(fromGroup);
				return 0;
			}
			AddMsgToQueue(GlobalMsg["strPreserve"], fromGroup, Group);
			setGroupLeave(fromGroup);
			return 0;
		}
		//else if(boolStandByMe&&getGroupMemberInfo(fromGroup, IdentityQQ).QQID != IdentityQQ) {
		//	AddMsgToQueue("请不要单独拉替身入群！", fromGroup, Group);
		//	setGroupLeave(fromGroup);
		//	return 0;
		//}
		else if(!GlobalMsg["strAddGroup"].empty()) {
			AddMsgToQueue(GlobalMsg["strAddGroup"], fromGroup, Group);
		}
	}
	return 0;
}

EVE_System_GroupMemberDecrease(eventGroupMemberDecrease) {
	if (beingOperateQQ == getLoginQQ()) {
		if (masterQQ&&boolMasterMode) {
			string strMsg = to_string(fromQQ)+"将"+GlobalMsg["strSelfName"]+"移出了群" + to_string(fromGroup) + "！";
			AddMsgToQueue(strMsg, masterQQ,Private);
			if (WhiteQQ.count(fromQQ)) {
				WhiteQQ.erase(fromQQ);
			}
			BlackQQ.insert(fromQQ);
			if (WhiteGroup.count(fromGroup)) {
				WhiteGroup.erase(fromGroup);
			}
			BlackGroup.insert(fromGroup);
		}
		string strInfo = "{\"LoginQQ\":\"" + to_string(getLoginQQ()) + "\",\"fromGroup\":" + to_string(fromGroup) + "\",\"Type\":\"kicked\",\"fromQQ\":\"" + to_string(fromQQ) + "\"";
	}
	return 0;
}

EVE_Request_AddGroup(eventGroupInvited) {
	if (subType == 2) {
		if (masterQQ&&boolMasterMode) {
			string strMsg = "群添加请求，来自：" + getStrangerInfo(fromQQ).nick +"("+ to_string(fromQQ) + "),群：(" + to_string(fromGroup)+")。";
			if (BlackGroup.count(fromGroup)) {
				strMsg += "\n已拒绝（群在黑名单中）";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else if (BlackQQ.count(fromQQ)) {
				strMsg += "\n已拒绝（用户在黑名单中）";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else if (WhiteGroup.count(fromQQ)) {
				strMsg += "\n已同意（群在白名单中）";
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			else if (WhiteQQ.count(fromQQ)) {
				strMsg += "\n已同意（用户在白名单中）";
				WhiteGroup.insert(fromQQ);
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			else if (boolPreserve) {
				strMsg += "\n已拒绝（当前在私用模式）";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else{
				strMsg += "已同意";
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			AddMsgToQueue(strMsg, masterQQ, Private);
		}
		return 1;
	}
	return 0;
}

EVE_Menu(eventMasterMode) {
	if (boolMasterMode) {
		boolMasterMode = false;
		MessageBoxA(nullptr, "Master模式已关闭√", "Master模式切换",MB_OK | MB_ICONINFORMATION);
	}else {
		boolMasterMode = true;
		MessageBoxA(nullptr, "Master模式已开启√", "Master模式切换", MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

EVE_Disable(eventDisable)
{
	Enabled = false;
	ilInitList.reset();
	Name.reset();
	dataBackUp();
	ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); ++it)
	{
		ofstreamDisabledGroup << *it << std::endl;
	}
	ofstreamDisabledGroup.close();

	ofstream ofstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledDiscuss.begin(); it != DisabledDiscuss.end(); ++it)
	{
		ofstreamDisabledDiscuss << *it << std::endl;
	}
	ofstreamDisabledDiscuss.close();
	ofstream ofstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPGroup.begin(); it != DisabledJRRPGroup.end(); ++it)
	{
		ofstreamDisabledJRRPGroup << *it << std::endl;
	}
	ofstreamDisabledJRRPGroup.close();

	ofstream ofstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPDiscuss.begin(); it != DisabledJRRPDiscuss.end(); ++it)
	{
		ofstreamDisabledJRRPDiscuss << *it << std::endl;
	}
	ofstreamDisabledJRRPDiscuss.close();

	ofstream ofstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEGroup.begin(); it != DisabledMEGroup.end(); ++it)
	{
		ofstreamDisabledMEGroup << *it << std::endl;
	}
	ofstreamDisabledMEGroup.close();

	ofstream ofstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEDiscuss.begin(); it != DisabledMEDiscuss.end(); ++it)
	{
		ofstreamDisabledMEDiscuss << *it << std::endl;
	}
	ofstreamDisabledMEDiscuss.close();

	ofstream ofstreamDisabledHELPGroup(strFileLoc + "DisabledHELPGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPGroup.begin(); it != DisabledHELPGroup.end(); ++it)
	{
		ofstreamDisabledHELPGroup << *it << std::endl;
	}
	ofstreamDisabledHELPGroup.close();

	ofstream ofstreamDisabledHELPDiscuss(strFileLoc + "DisabledHELPDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPDiscuss.begin(); it != DisabledHELPDiscuss.end(); ++it)
	{
		ofstreamDisabledHELPDiscuss << *it << std::endl;
	}
	ofstreamDisabledHELPDiscuss.close();

	ofstream ofstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBGroup.begin(); it != DisabledOBGroup.end(); ++it)
	{
		ofstreamDisabledOBGroup << *it << std::endl;
	}
	ofstreamDisabledOBGroup.close();

	ofstream ofstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBDiscuss.begin(); it != DisabledOBDiscuss.end(); ++it)
	{
		ofstreamDisabledOBDiscuss << *it << std::endl;
	}
	ofstreamDisabledOBDiscuss.close();

	ofstream ofstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveGroup.begin(); it != ObserveGroup.end(); ++it)
	{
		ofstreamObserveGroup << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveGroup.close();

	ofstream ofstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveDiscuss.begin(); it != ObserveDiscuss.end(); ++it)
	{
		ofstreamObserveDiscuss << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveDiscuss.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it)
	{
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
		{
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " "
				<< it1->first << " " << it1->second << std::endl;
		}
	}
	ofstreamCharacterProp.close();
	ofstream ofstreamDefault(strFileLoc + "Default.RDconf", ios::out | ios::trunc);
	for (auto it = DefaultDice.begin(); it != DefaultDice.end(); ++it)
	{
		ofstreamDefault << it->first << " " << it->second << std::endl;
	}
	ofstreamDefault.close();

	ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::out | ios::trunc);
	for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
	}
	ofstreamWelcomeMsg.close();
	DefaultDice.clear();
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
	strFileLoc.clear();
	return 0;
}

EVE_Exit(eventExit)
{
	if (!Enabled)
		return 0;
	ilInitList.reset();
	Name.reset();
	dataBackUp();
	ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); ++it)
	{
		ofstreamDisabledGroup << *it << std::endl;
	}
	ofstreamDisabledGroup.close();

	ofstream ofstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledDiscuss.begin(); it != DisabledDiscuss.end(); ++it)
	{
		ofstreamDisabledDiscuss << *it << std::endl;
	}
	ofstreamDisabledDiscuss.close();
	ofstream ofstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPGroup.begin(); it != DisabledJRRPGroup.end(); ++it)
	{
		ofstreamDisabledJRRPGroup << *it << std::endl;
	}
	ofstreamDisabledJRRPGroup.close();

	ofstream ofstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledJRRPDiscuss.begin(); it != DisabledJRRPDiscuss.end(); ++it)
	{
		ofstreamDisabledJRRPDiscuss << *it << std::endl;
	}
	ofstreamDisabledJRRPDiscuss.close();

	ofstream ofstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEGroup.begin(); it != DisabledMEGroup.end(); ++it)
	{
		ofstreamDisabledMEGroup << *it << std::endl;
	}
	ofstreamDisabledMEGroup.close();

	ofstream ofstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledMEDiscuss.begin(); it != DisabledMEDiscuss.end(); ++it)
	{
		ofstreamDisabledMEDiscuss << *it << std::endl;
	}
	ofstreamDisabledMEDiscuss.close();

	ofstream ofstreamDisabledHELPGroup(strFileLoc + "DisabledHELPGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPGroup.begin(); it != DisabledHELPGroup.end(); ++it)
	{
		ofstreamDisabledHELPGroup << *it << std::endl;
	}
	ofstreamDisabledHELPGroup.close();

	ofstream ofstreamDisabledHELPDiscuss(strFileLoc + "DisabledHELPDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledHELPDiscuss.begin(); it != DisabledHELPDiscuss.end(); ++it)
	{
		ofstreamDisabledHELPDiscuss << *it << std::endl;
	}
	ofstreamDisabledHELPDiscuss.close();

	ofstream ofstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBGroup.begin(); it != DisabledOBGroup.end(); ++it)
	{
		ofstreamDisabledOBGroup << *it << std::endl;
	}
	ofstreamDisabledOBGroup.close();

	ofstream ofstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = DisabledOBDiscuss.begin(); it != DisabledOBDiscuss.end(); ++it)
	{
		ofstreamDisabledOBDiscuss << *it << std::endl;
	}
	ofstreamDisabledOBDiscuss.close();

	ofstream ofstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveGroup.begin(); it != ObserveGroup.end(); ++it)
	{
		ofstreamObserveGroup << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveGroup.close();

	ofstream ofstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf", ios::out | ios::trunc);
	for (auto it = ObserveDiscuss.begin(); it != ObserveDiscuss.end(); ++it)
	{
		ofstreamObserveDiscuss << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveDiscuss.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it)
	{
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
		{
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " "
				<< it1->first << " " << it1->second << std::endl;
		}
	}
	ofstreamCharacterProp.close();
	ofstream ofstreamDefault(strFileLoc + "Default.RDconf", ios::out | ios::trunc);
	for (auto it = DefaultDice.begin(); it != DefaultDice.end(); ++it)
	{
		ofstreamDefault << it->first << " " << it->second << std::endl;
	}
	ofstreamDefault.close();

	ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::out | ios::trunc);
	for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
	}
	ofstreamWelcomeMsg.close();
	return 0;
}

MUST_AppInfo_RETURN(CQAPPID);
