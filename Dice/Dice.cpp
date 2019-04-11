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
/*
TODO:
1. en可变成长检定
2. st多人物卡
3. st人物卡绑定
4. st属性展示，全属性展示以及排序
5. 完善rules规则数据库
6. jrrp优化
7. help优化
8. 全局昵称
*/

using namespace std;
using namespace CQ;
using namespace Console;

unique_ptr<NameStorage> Name;

std::string strip(std::string origin)
{
	bool flag = true;
	while (flag)
	{
		flag = false;
		if (origin[0] == '!' || origin[0] == '.')
		{
			origin.erase(origin.begin());
			flag = true;
		}
		else if (origin.substr(0, 2) == "！" || origin.substr(0, 2) == "。")
		{
			origin.erase(origin.begin());
			origin.erase(origin.begin());
			flag = true;
		}
	}
	return origin;
}

std::string getName(long long QQ, long long GroupID = 0)
{
	if (GroupID)
	{
		/*群*/
		if (GroupID < 1000000000)
		{
			return strip(Name->get(GroupID, QQ).empty()
				             ? (getGroupMemberInfo(GroupID, QQ).GroupNick.empty()
					                ? getStrangerInfo(QQ).nick
					                : getGroupMemberInfo(GroupID, QQ).GroupNick)
				             : Name->get(GroupID, QQ));
		}
		/*讨论组*/
		return strip(Name->get(GroupID, QQ).empty() ? getStrangerInfo(QQ).nick : Name->get(GroupID, QQ));
	}
	/*私聊*/
	return strip(getStrangerInfo(QQ).nick);
}


//Master模式
bool boolMasterMode = false;
map<long long, int> DefaultDice;
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

struct SourceType
{
	SourceType(long long a, int b, long long c) : QQ(a), Type(b), GrouporDiscussID(c)
	{
	}
	long long QQ = 0;
	int Type = 0;
	long long GrouporDiscussID = 0;

	bool operator<(SourceType b) const
	{
		return this->QQ < b.QQ;
	}
};

using PropType = map<string, int>;
map<SourceType, PropType> CharacterProp;
multimap<long long, long long> ObserveGroup;
multimap<long long, long long> ObserveDiscuss;
string strFileLoc;

//备份数据
void dataBackUp() {
	//备份MasterQQ
	ofstream ofstreamMaster(strFileLoc + "Master.RDconf", ios::out | ios::trunc);
	ofstreamMaster << masterQQ << std::endl << boolMasterMode << std::endl << boolDisabledGlobal << std::endl << boolDisabledMeGlobal << std::endl << boolPreserve << std::endl << boolDisabledJrrpGlobal << std::endl << boolNoDiscuss << std::endl;
	ofstreamMaster.close();
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
}
EVE_Enable(eventEnable)
{
	//Wait until the thread terminates
	while (msgSendThreadRunning)
		Sleep(10);

	thread msgSendThread(SendMsg);
	msgSendThread.detach();
	strFileLoc = getAppDirectory();
	/*
	* 名称存储-创建与读取
	*/
	Name = make_unique<NameStorage>(strFileLoc + "Name.dicedb");
	//读取MasterMode
	ifstream ifstreamMaster(strFileLoc + "Master.RDconf");
	if (ifstreamMaster)
	{
		ifstreamMaster >> masterQQ >> boolMasterMode >> boolDisabledGlobal >> boolDisabledMeGlobal >> boolPreserve >> boolDisabledJrrpGlobal >> boolNoDiscuss;
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
	ilInitList = make_unique<Initlist>(strFileLoc + "INIT.DiceDB");
	ifstream ifstreamCustomMsg(strFileLoc + "CustomMsg.json");
	if (ifstreamCustomMsg)
	{
		ReadCustomMsg(ifstreamCustomMsg);
	}
	ifstreamCustomMsg.close();
	return 0;
}


EVE_PrivateMsg_EX(eventPrivateMsg)
{
	if (eve.isSystem())return;
	if (BlackQQ.count(eve.fromQQ)) {
		eve.message_block();
		return;
	}
	init(eve.message);
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = getName(eve.fromQQ);
	string strLowerMessage = eve.message;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), [](unsigned char c) { return tolower(c); });
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		AddMsgToQueue(GlobalMsg["strHlpMsg"], eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "master"&&boolMasterMode) {
		intMsgCnt+=6;
		if (masterQQ==0) {
			masterQQ = eve.fromQQ;
			AddMsgToQueue("试问，你就是本骰娘的Master√", eve.fromQQ);
		}else if (eve.fromQQ == masterQQ){
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			//命令选项
			Process(eve.message.substr(intMsgCnt));
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "str"&&boolMasterMode&&eve.fromQQ == masterQQ) {
		string strName;
		while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strName += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string strMsg = eve.message.substr(intMsgCnt);
		if (GlobalMsg.count(strName)) {
			EditedMsg[strName] = strMsg;
			GlobalMsg[strName] = (strName=="strHlpMsg")? Dice_Short_Ver + "\n" + strMsg : strMsg;
			AddMsgToQueue("已记下" + strName + "的自定义", eve.fromQQ);
		}
		else {
			AddMsgToQueue("是说" + strName + "？这似乎不是会用到的语句×", eve.fromQQ);
		}
	}
	else if (boolDisabledGlobal){
		AddMsgToQueue(GlobalMsg["strGlobalOff"], eve.fromQQ);
		return;
	}
	else if (strLowerMessage[intMsgCnt] == 'w')
	{
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w')
		{
			intMsgCnt++;
			boolDetail = true;
		}
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
					intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
				&&
				strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
					intTmpMsgCnt
				] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		string strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (isspace(static_cast<unsigned char>(eve.message[intTmpMsgCnt])))
			intTmpMsgCnt++;
		string strReason = eve.message.substr(intTmpMsgCnt);


		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromQQ);
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeExceeded"], eve.fromQQ);
				return;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeErr"], eve.fromQQ);
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				const string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				AddMsgToQueue(strTurnNotice, eve.fromQQ);
			}
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
			                                            ? strMainDice.find('+')
			                                            : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(static_cast<unsigned char>(i)))
			{
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, eve.fromQQ);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromQQ);
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			AddMsgToQueue(strAns, eve.fromQQ);
		}
		else
		{
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				AddMsgToQueue(strAns, eve.fromQQ);
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "的疯狂发作-临时症状:\n";
		TempInsane(strAns);

		AddMsgToQueue(strAns, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "的疯狂发作-总结症状:\n";
		LongInsane(strAns);

		AddMsgToQueue(strAns, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string San;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{
			AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ);

			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(
			eve.fromQQ, PrivateT, 0)].count("理智")))
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromQQ);

			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ);
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ);
				return;
			}
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{
			AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ);

			return;
		}
		if (San.length() >= 3)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromQQ);

			return;
		}
		const int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] : stoi(San);
		if (intSan == 0)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromQQ);

			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " 成功\n你的San值减少" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSuc.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] = max(0, intSan - rdSuc.intTotal);
			}
		}
		else if (intTmpRollRes == 100 || (intSan < 50 && intTmpRollRes > 95))
		{
			strAns += " 大失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdFail.Max();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "最大值=" + to_string(rdFail.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdFail.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] = max(0, intSan - rdFail.intTotal);
			}
		}
		else
		{
			strAns += " 失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdFail.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdFail.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] = max(0, intSan - rdFail.intTotal);
			}
		}

		AddMsgToQueue(strAns, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		AddMsgToQueue(Dice_Full_Ver+PersonalMsg["strBotMsg"], eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])) && !isspace(static_cast<unsigned char>(eve.message[intMsgCnt]))
		)
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(
				eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strEnValInvalid"], eve.fromQQ);
				return;
			}
		}
		else
		{
			if (strCurrentValue.length() > 3)
			{
				AddMsgToQueue(GlobalMsg["strEnValInvalid"], eve.fromQQ);

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}

		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			const int intTmpRollD10 = RandomGenerator::Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName] = intCurrentVal + intTmpRollD10;
			}
		}

		AddMsgToQueue(strAns, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		if (boolDisabledJrrpGlobal) {
			AddMsgToQueue(GlobalMsg["strDisabledJrrpGlobal"], eve.fromQQ);
			return;
		}
		string des;
		string data = "QQ=" + to_string(CQ::getLoginQQ()) + "&v=20190114" + "&QueryQQ=" + to_string(eve.fromQQ);
		char *frmdata = new char[data.length() + 1];
		strcpy_s(frmdata, data.length() + 1, data.c_str());
		bool res = Network::POST("api.kokona.tech", "/jrrp", 5555, frmdata, des);
		delete[] frmdata;
		if (res)
		{
			AddMsgToQueue(format(GlobalMsg["strJrrp"], { strNickName, des }), eve.fromQQ);
		}
		else
		{
			AddMsgToQueue(format(GlobalMsg["strJrrpErr"], { des }), eve.fromQQ);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
	{
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		string type;
		while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			type += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}

		auto nameType = NameGenerator::Type::UNKNOWN;
		if (type == "cn")
			nameType = NameGenerator::Type::CN;
		else if (type == "en")
			nameType = NameGenerator::Type::EN;
		else if (type == "jp")
			nameType = NameGenerator::Type::JP;

		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		string strNum;
		while (isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])))
		{
			strNum += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.size() > 2)
		{
			AddMsgToQueue(GlobalMsg["strNameNumTooBig"], eve.fromQQ);
			return;
		}
		auto intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strNameNumTooBig"], eve.fromQQ);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strNameNumCannotBeZero"], eve.fromQQ);
			return;
		}
		vector<string> TempNameStorage;
		while (TempNameStorage.size() != intNum)
		{
			string name = NameGenerator::getRandomName(nameType);
			if (find(TempNameStorage.begin(), TempNameStorage.end(), name) == TempNameStorage.end())
			{
				TempNameStorage.push_back(name);
			}
		}
		string strReply = strNickName + "的随机名称:\n";
		for (auto i = 0; i != TempNameStorage.size(); i++)
		{
			strReply.append(TempNameStorage[i]);
			if (i != TempNameStorage.size() - 1)strReply.append(", ");
		}
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt]))|| eve.message[intMsgCnt] == ':')
				intMsgCnt++;
			string strDefaultRule = eve.message.substr(intMsgCnt);
			if (strDefaultRule.empty()) {
				DefaultRule.erase(eve.fromQQ);
				AddMsgToQueue(GlobalMsg["strRuleReset"], eve.fromQQ);
			}
			else {
				for (auto& n : strDefaultRule)
					n = toupper(static_cast<unsigned char>(n));
				DefaultRule[eve.fromQQ] = strDefaultRule;
				AddMsgToQueue(GlobalMsg["strRuleSet"], eve.fromQQ);
			}
		}
		else {
			string strSearch = eve.message.substr(intMsgCnt);
			string strDefaultRule = DefaultRule[eve.fromQQ];
			for (auto& n : strSearch)
				n = toupper(static_cast<unsigned char>(n));
			string strReturn;
			if (DefaultRule.count(eve.fromQQ) && strSearch.find(':') == string::npos && GetRule::get(strDefaultRule, strSearch, strReturn)) {
				AddMsgToQueue(strReturn, eve.fromQQ);
			}else if (GetRule::analyze(strSearch, strReturn))
			{
				AddMsgToQueue(strReturn, eve.fromQQ);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strRuleErr"] + strReturn, eve.fromQQ);
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			AddMsgToQueue(GlobalMsg["strStErr"], eve.fromQQ);
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, PrivateT, 0));
			}
			AddMsgToQueue(GlobalMsg["strPropCleared"], eve.fromQQ);
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "del")
		{
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
				intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(
				eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].erase(strSkillName);
				AddMsgToQueue(GlobalMsg["strPropDeleted"], eve.fromQQ);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPropNotFound"], eve.fromQQ);
			}
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 4) == "show")
		{
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
				intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(
				eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				AddMsgToQueue(format(GlobalMsg["strProp"], {
					strNickName, strSkillName,
					to_string(CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName])
				}), eve.fromQQ);
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				AddMsgToQueue(format(GlobalMsg["strProp"], {strNickName, strSkillName, to_string(SkillDefaultVal[strSkillName])}),
				              eve.fromQQ);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPropNotFound"], eve.fromQQ);
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
				isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
				!= ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
			] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			{
				strSkillVal += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 3)
			{
				boolError = true;
				break;
			}
			CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName] = stoi(strSkillVal);
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromQQ);
		}
		else
		{
			AddMsgToQueue(GlobalMsg["strSetPropSuccess"], eve.fromQQ);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		if (boolDisabledMeGlobal) {
			AddMsgToQueue(GlobalMsg["strDisabledMeGlobal"], eve.fromQQ);
			return;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strGroupID;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strGroupID += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strAction = strip(eve.message.substr(intMsgCnt));

		for (auto i : strGroupID)
		{
			if (!isdigit(static_cast<unsigned char>(i)))
			{
				AddMsgToQueue(GlobalMsg["strGroupIDInvalid"], eve.fromQQ);
				return;
			}
		}
		if (strGroupID.empty())
		{
			AddMsgToQueue("群号不能为空!", eve.fromQQ);
			return;
		}
		if (strAction.empty())
		{
			AddMsgToQueue("动作不能为空!", eve.fromQQ);
			return;
		}
		const long long llGroupID = stoll(strGroupID);
		if (DisabledGroup.count(llGroupID))
		{
			AddMsgToQueue(GlobalMsg["strDisabledErr"], eve.fromQQ);
			return;
		}
		if (DisabledMEGroup.count(llGroupID))
		{
			AddMsgToQueue(GlobalMsg["strMEDisabledErr"], eve.fromQQ);
			return;
		}
		string strReply = getName(eve.fromQQ, llGroupID) + strAction;
		const int intSendRes = sendGroupMsg(llGroupID, strReply);
		if (intSendRes < 0)
		{
			AddMsgToQueue(GlobalMsg["strSendErr"], eve.fromQQ);
		}
		else
		{
			AddMsgToQueue("命令执行成功!", eve.fromQQ);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(static_cast<unsigned char>(charNumElement)))
			{
				AddMsgToQueue(GlobalMsg["strSetInvalid"], eve.fromQQ);
				return;
			}
		if (strDefaultDice.length() > 3 && strDefaultDice!= "1000")
		{
			AddMsgToQueue(GlobalMsg["strSetTooBig"], eve.fromQQ);
			return;
		}
		const int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		const string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
		AddMsgToQueue(strSetSuccessReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
		string strReply = strNickName;
		COC6D(strReply);
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6")
	{
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromQQ);
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromQQ);
			return;
		}
		string strReply = strNickName;
		DND(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
		string strReply = strNickName;
		COC7D(strReply);
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
	{
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromQQ);
			return;
		}
		string strReply = strNickName;
		COC7(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
	{
		intMsgCnt += 2;
		string strSkillName;
		string strMainDice = "D100";
		string strSkillModify;
		int intSkillModify = 0;
		if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while(isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
			isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':' && strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strSkillModify += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intSkillModify = stoi(strSkillModify);
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(
				eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strUnknownPropErr"], eve.fromQQ);
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromQQ);
			return;
		}
		else
		{
			intSkillVal = stoi(strSkillVal);
		}
		int intFianlSkillVal = intSkillVal + intSkillModify;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
		{
			AddMsgToQueue(GlobalMsg["strSuccessRateErr"], eve.fromQQ);
			return;
		}
		RD rdMainDice(strMainDice, eve.fromQQ);
		const int intFirstTimeRes = rdMainDice.Roll(); 
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromQQ);
			return;
		}
		const int intD100Res = rdMainDice.intTotal;
		string strReply = strNickName + "进行" + strSkillName + strSkillModify + "检定: "+ rdMainDice.FormCompleteString() + "/" +
			to_string(intFianlSkillVal) +" ";
		if (intD100Res <= 5 && intD100Res <= intSkillVal)strReply += GlobalMsg["strRollCriticalSuccess"];
		else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
		else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
		else if (intD100Res <= intFianlSkillVal / 2)strReply += GlobalMsg["strRollHardSuccess"];
		else if (intD100Res <= intFianlSkillVal)strReply += GlobalMsg["strRollRegularSuccess"];
		else if (intD100Res <= 95)strReply += GlobalMsg["strRollFailure"];
		else strReply += GlobalMsg["strRollFumble"];
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		string strSkillName;
		string strMainDice = "D100";
		string strSkillModify;
		int intSkillModify = 0;
		if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
			isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':' && strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strSkillModify += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intSkillModify = stoi(strSkillModify);
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(
				eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strUnknownPropErr"], eve.fromQQ);
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromQQ);
			return;
		}
		else
		{
			intSkillVal = stoi(strSkillVal);
		}

		int intFianlSkillVal = intSkillVal + intSkillModify;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
		{
			AddMsgToQueue(GlobalMsg["strSuccessRateErr"], eve.fromQQ);
			return;
		}
		RD rdMainDice(strMainDice, eve.fromQQ);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromQQ);
			return;
		}
		const int intD100Res = rdMainDice.intTotal;
		string strReply = strNickName + "进行" + strSkillName + strSkillModify + "检定: " + rdMainDice.FormCompleteString() + "/" +
			to_string(intFianlSkillVal) + " ";
		if (intD100Res == 1)strReply += GlobalMsg["strRollCriticalSuccess"];
		else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
		else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
		else if (intD100Res <= intFianlSkillVal / 2)strReply += GlobalMsg["strRollHardSuccess"];
		else if (intD100Res <= intFianlSkillVal)strReply += GlobalMsg["strRollRegularSuccess"];
		else if (intD100Res <= 95 || intFianlSkillVal >= 50 )strReply += GlobalMsg["strRollFailure"];
		else strReply += GlobalMsg["strRollFumble"];
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw") {

	intMsgCnt += 4;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))||(strLowerMessage[intMsgCnt]=='.'))
		intMsgCnt++;

	string strDeckName;
	while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strDeckName += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strCardNum;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strCardNum += eve.message[intMsgCnt];
		intMsgCnt++;
	}
	auto intCardNum = strCardNum.empty() ? 1 : stoi(strCardNum);
	if (intCardNum == 0)
	{
		AddMsgToQueue(GlobalMsg["strNumCannotBeZero"], eve.fromQQ);
		return;
	}
	int intDeckRes = CardDeck::findDeck(strDeckName);
	string strReply;
	if (intDeckRes < 0) {
		strReply = strDeckName + "?" + GlobalMsg["strDeckNotFound"];
		AddMsgToQueue(strReply, eve.fromQQ);
		return;
	}
	vector<string>TempDeck(CardDeck::PublicDeck[intDeckRes]);
	TempDeck.erase(TempDeck.begin());
	strReply = "来看看" + strNickName + "的随机抽取结果:\n" + CardDeck::drawCard(TempDeck);
	while (--intCardNum && TempDeck.size()) {
		strReply += "\n"+CardDeck::drawCard(TempDeck);
		if (strReply.length() > 1000) {
			AddMsgToQueue(strReply, eve.fromQQ);
			strReply.clear();
		}
	}
	AddMsgToQueue(strReply, eve.fromQQ);
	if (intCardNum) {
		AddMsgToQueue(GlobalMsg["strDeckEmpty"], eve.fromQQ);
		return;
	}
}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'd'
	)
	{
		intMsgCnt += 1;
		bool boolDetail = true;
		if (eve.message[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[
					intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f'
				||
				strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
					intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
				&&
				strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
					intTmpMsgCnt
				] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (isspace(static_cast<unsigned char>(eve.message[intTmpMsgCnt])))
				intTmpMsgCnt++;
			strReason = eve.message.substr(intTmpMsgCnt);
		}
		else
			strReason = eve.message.substr(intMsgCnt);

		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromQQ);
				return;
			}
			if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromQQ);
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeExceeded"], eve.fromQQ);
				return;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeErr"], eve.fromQQ);
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				const string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";

				AddMsgToQueue(strTurnNotice, eve.fromQQ);
			}
		}
		RD rdMainDice(strMainDice, eve.fromQQ);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromQQ);
			return;
		}
		if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromQQ);
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			AddMsgToQueue(strAns, eve.fromQQ);
		}
		else
		{
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				AddMsgToQueue(strAns, eve.fromQQ);
			}
		}
	}
}

EVE_GroupMsg_EX(eventGroupMsg)
{
	if (eve.isAnonymous())return;
	if (eve.isSystem()) {
		if (eve.message.find("被管理员禁言") != string::npos&&eve.message.find(to_string(getLoginQQ())) != string::npos) {
			long long fromQQ;
			int intAuthCnt=0;
			for (auto member : getGroupMemberList(eve.fromGroup)) {
				if (member.permissions == 3) {
					//相应核心精神，由群主做负责人
					fromQQ = member.QQID;				}
				else if (member.permissions == 2) {
					//记录管理员数量
					intAuthCnt++;
				}
			}
			if(masterQQ&&boolMasterMode){
				string strMsg = "在群\"" + getGroupList()[eve.fromGroup] + "\"("+ to_string(eve.fromGroup)+ ")中," + eve.message+"\n群主"+getStrangerInfo(fromQQ).nick+"("+to_string(fromQQ)+"),另有管理员"+to_string(intAuthCnt)+"名";
				AddMsgToQueue(strMsg, masterQQ);
				BlackGroup.insert(eve.fromGroup);
				if(WhiteGroup.count(eve.fromGroup))WhiteGroup.erase(eve.fromGroup);
				//setGroupLeave(eve.fromGroup);
			}
			string strInfo = "{\"LoginQQ\":\"" + to_string(getLoginQQ()) + "\",\"fromGroup\":" + to_string(eve.fromGroup) + "\",\"Type\":\"banned\",\"fromQQ\":\"" + to_string(fromQQ)+"\"";
		}
		else return;
	}
	init(eve.message);
	while (isspace(static_cast<unsigned char>(eve.message[0])))
		eve.message.erase(eve.message.begin());
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	bool boolNamed=false;
	if (eve.message.substr(0, 6) == "[CQ:at")
	{
		if (eve.message.substr(0, strAt.length()) == strAt)
		{
			eve.message = eve.message.substr(strAt.length());
			boolNamed = true;
		}
		else
		{
			return;
		}
	}
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	if (BlackQQ.count(eve.fromQQ) || BlackGroup.count(eve.fromGroup)) {
		eve.message_block();
		return;
	}
	int intMsgCnt = 1;
	while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = getName(eve.fromQQ, eve.fromGroup);
	string strLowerMessage = eve.message;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), [](unsigned char c) { return tolower(c); });
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(
			static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
				{
					if (DisabledGroup.count(eve.fromGroup))
					{
						DisabledGroup.erase(eve.fromGroup);
						AddMsgToQueue("成功开启本机器人!", eve.fromGroup, false);
					}
					else
					{
						AddMsgToQueue("本机器人已经处于开启状态!", eve.fromGroup, false);
					}
				}
				else
				{
					AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
				}
			}
		}
		else if (Command == "off")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
				{
					if (!DisabledGroup.count(eve.fromGroup))
					{
						DisabledGroup.insert(eve.fromGroup);
						AddMsgToQueue("成功关闭本机器人!", eve.fromGroup, false);
					}
					else
					{
						AddMsgToQueue("本机器人已经处于关闭状态!", eve.fromGroup, false);
					}
				}
				else
				{
					AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
				}
			}
		}
		else
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				AddMsgToQueue(Dice_Full_Ver + PersonalMsg["strBotMsg"] , eve.fromGroup, false);
			}
		}
		return;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum ==
			to_string(getLoginQQ()))
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				setGroupLeave(eve.fromGroup);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
		}
		return;
	}
	if (boolDisabledGlobal||!boolNamed&&DisabledGroup.count(eve.fromGroup))
		return;
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		const string strAction = strLowerMessage.substr(intMsgCnt);
		if (strAction == "on")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (DisabledHELPGroup.count(eve.fromGroup))
				{
					DisabledHELPGroup.erase(eve.fromGroup);
					AddMsgToQueue("成功在本群中启用.help命令!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("在本群中.help命令没有被禁用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
			return;
		}
		if (strAction == "off")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (!DisabledHELPGroup.count(eve.fromGroup))
				{
					DisabledHELPGroup.insert(eve.fromGroup);
					AddMsgToQueue("成功在本群中禁用.help命令!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("在本群中.help命令没有被启用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
			return;
		}
		if (DisabledHELPGroup.count(eve.fromGroup))
		{
			AddMsgToQueue(GlobalMsg["strHELPDisabledErr"], eve.fromGroup, false);
			return;
		}
		AddMsgToQueue(GlobalMsg["strHlpMsg"], eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome")
	{
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
		{
			string strWelcomeMsg = eve.message.substr(intMsgCnt);
			if (strWelcomeMsg.empty())
			{
				if (WelcomeMsg.count(eve.fromGroup))
				{
					WelcomeMsg.erase(eve.fromGroup);
					AddMsgToQueue(GlobalMsg["strWelcomeMsgClearNotice"], eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue(GlobalMsg["strWelcomeMsgClearErr"], eve.fromGroup, false);
				}
			}
			else
			{
				WelcomeMsg[eve.fromGroup] = strWelcomeMsg;
				AddMsgToQueue(GlobalMsg["strWelcomeMsgUpdateNotice"], eve.fromGroup, false);
			}
		}
		else
		{
			AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "group")
	{
		if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions == 1) {
			AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			return;
		}
		if (getGroupMemberInfo(eve.fromGroup, getLoginQQ()).permissions == 1) {
			AddMsgToQueue(GlobalMsg["strSelfPermissionErr"], eve.fromGroup, false);
			return;
		}
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(
			static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum;
		if (strLowerMessage.substr(intMsgCnt, 10) == "[cq:at,qq=") {
			intMsgCnt += 10;
			while (isdigit(strLowerMessage[intMsgCnt])) {
				QQNum += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intMsgCnt++;
		}
		else while (isdigit(strLowerMessage[intMsgCnt])) {
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (Command == "ban")
		{
			long long llMemberQQ = stoll(QQNum);
			GroupMemberInfo Member = getGroupMemberInfo(eve.fromGroup, llMemberQQ);
			if (Member.QQID == llMemberQQ)
			{
				if (Member.permissions > 1) {
					AddMsgToQueue(GlobalMsg["strSelfPermissionErr"], eve.fromGroup, false);
					return;
				}
				string strMainDice;
				while (isdigit(strLowerMessage[intMsgCnt])|| (strLowerMessage[intMsgCnt])=='d'|| (strLowerMessage[intMsgCnt])=='+'|| (strLowerMessage[intMsgCnt])=='-') {
					strMainDice += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				RD rdMainDice(strMainDice, llMemberQQ);
				rdMainDice.Roll();
				int intDuration = rdMainDice.intTotal;
				if (setGroupBan(eve.fromGroup, llMemberQQ, intDuration * 60)==0)
					if(intDuration<=0)
						AddMsgToQueue("裁定" + getName(Member.QQID, eve.fromGroup) + "解除禁言√", eve.fromGroup, false);
					else AddMsgToQueue("裁定" + getName(Member.QQID, eve.fromGroup) + "禁言时长" + rdMainDice.FormCompleteString() + "分钟√", eve.fromGroup, false);
				else AddMsgToQueue("禁言失败×", eve.fromGroup, false);
			}else AddMsgToQueue("查无此人×", eve.fromGroup, false);
		}
		return;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			AddMsgToQueue(GlobalMsg["strStErr"], eve.fromGroup, false);
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, GroupT, eve.fromGroup));
			}
			AddMsgToQueue(GlobalMsg["strPropCleared"], eve.fromGroup, false);
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "del")
		{
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
				intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(
				eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].erase(strSkillName);
				AddMsgToQueue(GlobalMsg["strPropDeleted"], eve.fromGroup, false);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPropNotFound"], eve.fromGroup, false);
			}
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 4) == "show")
		{
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
				intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(
				eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				AddMsgToQueue(format(GlobalMsg["strProp"], {
					strNickName, strSkillName,
					to_string(CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName])
				}), eve.fromGroup, false);
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				AddMsgToQueue(format(GlobalMsg["strProp"], {strNickName, strSkillName, to_string(SkillDefaultVal[strSkillName])}),
				              eve.fromGroup, false);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPropNotFound"], eve.fromGroup, false);
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
				isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
				!= ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
			] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			{
				strSkillVal += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 3)
			{
				boolError = true;
				break;
			}
			CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName] = stoi(strSkillVal);
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromGroup, false);
		}
		else
		{
			AddMsgToQueue(GlobalMsg["strSetPropSuccess"], eve.fromGroup, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		}
		else if (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			strinit += '+';
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strname = eve.message.substr(intMsgCnt);
		if (strname.empty())
			strname = strNickName;
		else
			strname = strip(strname);
		RD initdice(strinit);
		const int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromGroup, false);
			return;
		}
		ilInitList->insert(eve.fromGroup, initdice.intTotal, strname);
		const string strReply = strname + "的先攻骰点：" + strinit + '=' + to_string(initdice.intTotal);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init")
	{
		intMsgCnt += 4;
		string strReply;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (ilInitList->clear(eve.fromGroup))
				strReply = "成功清除先攻记录！";
			else
				strReply = "列表为空！";
			AddMsgToQueue(strReply, eve.fromGroup, false);
			return;
		}
		ilInitList->show(eve.fromGroup, strReply);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage[intMsgCnt] == 'w')
	{
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w')
		{
			intMsgCnt++;
			boolDetail = true;
		}
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
					intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
				&&
				strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
					intTmpMsgCnt
				] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		string strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (isspace(static_cast<unsigned char>(eve.message[intTmpMsgCnt])))
			intTmpMsgCnt++;
		string strReason = eve.message.substr(intTmpMsgCnt);


		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromGroup, false);
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeExceeded"], eve.fromGroup, false);
				return;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeErr"], eve.fromGroup, false);
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromGroup, false);
				}
				else
				{
					strTurnNotice = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strTurnNotice;
					AddMsgToQueue(strTurnNotice, eve.fromQQ);
					const auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strTurnNotice, it->second);
						}
					}
				}
			}
		}
		if (strMainDice.empty())
		{
			AddMsgToQueue(GlobalMsg["strEmptyWWDiceErr"], eve.fromGroup, false);
			return;
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
			                                            ? strMainDice.find('+')
			                                            : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(static_cast<unsigned char>(i)))
			{
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, eve.fromQQ);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromGroup, false);
			return;
		}
		else
		{
			if (intFirstTimeRes == ZeroType_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromGroup, false);
				return;
			}
			if (intFirstTimeRes == DiceTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromGroup, false);
				return;
			}
			if (intFirstTimeRes == TypeTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromGroup, false);
				return;
			}
			if (intFirstTimeRes == AddDiceVal_Err)
			{
				AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromGroup, false);
				return;
			}
			if (intFirstTimeRes != 0)
			{
				AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromGroup, false);
				return;
			}
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				AddMsgToQueue(strAns, eve.fromGroup, false);
			}
			else
			{
				strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
				AddMsgToQueue(strAns, eve.fromQQ);
				const auto range = ObserveGroup.equal_range(eve.fromGroup);
				for (auto it = range.first; it != range.second; ++it)
				{
					if (it->second != eve.fromQQ)
					{
						AddMsgToQueue(strAns, it->second);
					}
				}
			}
		}
		else
		{
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromGroup, false);
				}
				else
				{
					strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
					AddMsgToQueue(strAns, eve.fromQQ);
					const auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strAns, it->second);
						}
					}
				}
			}
		}
		if (isHidden)
		{
			const string strReply = strNickName + "进行了一次暗骰";
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (DisabledOBGroup.count(eve.fromGroup))
				{
					DisabledOBGroup.erase(eve.fromGroup);
					AddMsgToQueue("成功在本群中启用旁观模式!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("本群中旁观模式没有被禁用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue("你没有权限执行此命令!", eve.fromGroup, false);
			}
			return;
		}
		if (Command == "off")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (!DisabledOBGroup.count(eve.fromGroup))
				{
					DisabledOBGroup.insert(eve.fromGroup);
					ObserveGroup.clear();
					AddMsgToQueue("成功在本群中禁用旁观模式!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("本群中旁观模式没有被启用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue("你没有权限执行此命令!", eve.fromGroup, false);
			}
			return;
		}
		if (DisabledOBGroup.count(eve.fromGroup))
		{
			AddMsgToQueue("在本群中旁观模式已被禁用!", eve.fromGroup, false);
			return;
		}
		if (Command == "list")
		{
			string Msg = "当前的旁观者有:";
			const auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				Msg += "\n" + getName(it->second, eve.fromGroup) + "(" + to_string(it->second) + ")";
			}
			const string strReply = Msg == "当前的旁观者有:" ? "当前暂无旁观者" : Msg;
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
		else if (Command == "clr")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				ObserveGroup.erase(eve.fromGroup);
				AddMsgToQueue("成功删除所有旁观者!", eve.fromGroup, false);
			}
			else
			{
				AddMsgToQueue("你没有权限执行此命令!", eve.fromGroup, false);
			}
		}
		else if (Command == "exit")
		{
			const auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					ObserveGroup.erase(it);
					const string strReply = strNickName + "成功退出旁观模式!";
					AddMsgToQueue(strReply, eve.fromGroup, false);
					eve.message_block();
					return;
				}
			}
			const string strReply = strNickName + "没有加入旁观模式!";
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
		else
		{
			const auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					const string strReply = strNickName + "已经处于旁观模式!";
					AddMsgToQueue(strReply, eve.fromGroup, false);
					eve.message_block();
					return;
				}
			}
			ObserveGroup.insert(make_pair(eve.fromGroup, eve.fromQQ));
			const string strReply = strNickName + "成功加入旁观模式!";
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "的疯狂发作-临时症状:\n";
		TempInsane(strAns);
		AddMsgToQueue(strAns, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "的疯狂发作-总结症状:\n";
		LongInsane(strAns);
		AddMsgToQueue(strAns, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string San;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{
			AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromGroup, false);
			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[
			SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count("理智")))
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromGroup, false);
			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ, false);
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ, false);
				return;
			}
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{
			AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromGroup, false);
			return;
		}
		if (San.length() >= 3)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromGroup, false);
			return;
		}
		const int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] : stoi(San);
		if (intSan == 0)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromGroup, false);
			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " 成功\n你的San值减少" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSuc.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] = max(0, intSan - rdSuc.intTotal);
			}
		}
		else if (intTmpRollRes == 100 || (intSan < 50 && intTmpRollRes > 95))
		{
			strAns += " 大失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdFail.Max();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "最大值=" + to_string(rdFail.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdFail.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] = max(0, intSan - rdFail.intTotal);
			}
		}
		else
		{
			strAns += " 失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdFail.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdFail.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] = max(0, intSan - rdFail.intTotal);
			}
		}
		AddMsgToQueue(strAns, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])) && !isspace(static_cast<unsigned char>(eve.message[intMsgCnt]))
		)
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(
				eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strEnValInvalid"], eve.fromGroup, false);
				return;
			}
		}
		else
		{
			if (strCurrentValue.length() > 3)
			{
				AddMsgToQueue(GlobalMsg["strEnValInvalid"], eve.fromGroup, false);

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}

		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			const int intTmpRollD10 = RandomGenerator::Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName] = intCurrentVal +
					intTmpRollD10;
			}
		}
		AddMsgToQueue(strAns, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		if (boolDisabledJrrpGlobal) {
			AddMsgToQueue(GlobalMsg["strDisabledJrrpGlobal"], eve.fromGroup,false);
			return;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (DisabledJRRPGroup.count(eve.fromGroup))
				{
					DisabledJRRPGroup.erase(eve.fromGroup);
					AddMsgToQueue("成功在本群中启用JRRP!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("在本群中JRRP没有被禁用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
			return;
		}
		if (Command == "off")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (!DisabledJRRPGroup.count(eve.fromGroup))
				{
					DisabledJRRPGroup.insert(eve.fromGroup);
					AddMsgToQueue("成功在本群中禁用JRRP!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("在本群中JRRP没有被启用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
			return;
		}
		if (DisabledJRRPGroup.count(eve.fromGroup))
		{
			AddMsgToQueue("在本群中JRRP功能已被禁用", eve.fromGroup, false);
			return;
		}
		string des;
		string data = "QQ=" + to_string(CQ::getLoginQQ()) + "&v=20190114" + "&QueryQQ=" + to_string(eve.fromQQ);
		char *frmdata = new char[data.length() + 1];
		strcpy_s(frmdata, data.length() + 1, data.c_str());
		bool res = Network::POST("api.kokona.tech", "/jrrp", 5555, frmdata, des);
		delete[] frmdata;
		if (res)
		{
			AddMsgToQueue(format(GlobalMsg["strJrrp"], { strNickName, des }), eve.fromGroup, false);
		}
		else
		{
			AddMsgToQueue(format(GlobalMsg["strJrrpErr"], { des }), eve.fromGroup, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
	{
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		string type;
		while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			type += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}

		auto nameType = NameGenerator::Type::UNKNOWN;
		if (type == "cn")
			nameType = NameGenerator::Type::CN;
		else if (type == "en")
			nameType = NameGenerator::Type::EN;
		else if (type == "jp")
			nameType = NameGenerator::Type::JP;

		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		string strNum;
		while(isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])))
		{
			strNum += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.size() > 2)
		{
			AddMsgToQueue(GlobalMsg["strNameNumTooBig"], eve.fromGroup, false);
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strNameNumTooBig"], eve.fromGroup, false);
			return;
		}
		if(intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strNameNumCannotBeZero"], eve.fromGroup, false);
			return;
		}
		vector<string> TempNameStorage;
		while(TempNameStorage.size() != intNum)
		{
			string name = NameGenerator::getRandomName(nameType);
			if (find(TempNameStorage.begin(), TempNameStorage.end(), name) == TempNameStorage.end())
			{
				TempNameStorage.push_back(name);
			}
		}
		string strReply = strNickName + "的随机名称:\n";
		for (auto i = 0; i != TempNameStorage.size(); i++)
		{
			strReply.append(TempNameStorage[i]);
			if (i != TempNameStorage.size() - 1)strReply.append(", ");
		}
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string type = strLowerMessage.substr(intMsgCnt, 2);
		string name;
		if (type=="cn")
			name = NameGenerator::getChineseName();
		else if (type=="en")
			name = NameGenerator::getEnglishName();
		else if (type=="jp")
			name = NameGenerator::getJapaneseName();
		else
			name = NameGenerator::getRandomName();
		Name->set(eve.fromGroup, eve.fromQQ, name);
		const string strReply = "已将" + strNickName + "的名称更改为" + name;
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string name = eve.message.substr(intMsgCnt);
		if (name.length() > 50)
		{
			AddMsgToQueue(GlobalMsg["strNameTooLongErr"], eve.fromGroup, false);
			return;
		}
		if (!name.empty())
		{
			Name->set(eve.fromGroup, eve.fromQQ, name);
			const string strReply = "已将" + strNickName + "的名称更改为" + strip(name);
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
		else
		{
			if (Name->del(eve.fromGroup, eve.fromQQ))
			{
				const string strReply = "已将" + strNickName + "的名称删除";
				AddMsgToQueue(strReply, eve.fromGroup, false);
			}
			else
			{
				const string strReply = strNickName + GlobalMsg["strNameDelErr"];
				AddMsgToQueue(strReply, eve.fromGroup, false);
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])) || eve.message[intMsgCnt] == ':')
				intMsgCnt++;
			string strDefaultRule = eve.message.substr(intMsgCnt);
			if (strDefaultRule.empty()) {
				DefaultRule.erase(eve.fromQQ);
				AddMsgToQueue(GlobalMsg["strRuleReset"], eve.fromGroup, false);
			}
			else {
				for (auto& n : strDefaultRule)
					n = toupper(static_cast<unsigned char>(n));
				DefaultRule[eve.fromQQ] = strDefaultRule;
				AddMsgToQueue(GlobalMsg["strRuleSet"], eve.fromGroup, false);
			}
		}
		else {
			string strSearch = eve.message.substr(intMsgCnt);
			string strDefaultRule = DefaultRule[eve.fromQQ];
			for (auto& n : strSearch)
				n = toupper(static_cast<unsigned char>(n));
			string strReturn;
			if (DefaultRule.count(eve.fromQQ) && strSearch.find(':') == string::npos && GetRule::get(strDefaultRule, strSearch, strReturn)) {
				AddMsgToQueue(strReturn, eve.fromGroup, false);
			}
			else if (GetRule::analyze(strSearch, strReturn))
			{
				AddMsgToQueue(strReturn, eve.fromGroup, false);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strRuleErr"] + strReturn, eve.fromGroup, false);
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (strAction == "on")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (DisabledMEGroup.count(eve.fromGroup))
				{
					DisabledMEGroup.erase(eve.fromGroup);
					AddMsgToQueue("成功在本群中启用.me命令!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("在本群中.me命令没有被禁用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
			return;
		}
		if (strAction == "off")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (!DisabledMEGroup.count(eve.fromGroup))
				{
					DisabledMEGroup.insert(eve.fromGroup);
					AddMsgToQueue("成功在本群中禁用.me命令!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("在本群中.me命令没有被启用!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
			return;
		}
		if (boolDisabledMeGlobal)
		{
			AddMsgToQueue(GlobalMsg["strDisabledMeGlobal"], eve.fromGroup, false);
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			AddMsgToQueue("在本群中.me命令已被禁用!", eve.fromGroup, false);
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			AddMsgToQueue(GlobalMsg["strMEDisabledErr"], eve.fromGroup, false);
			return;
		}
		strAction = strip(eve.message.substr(intMsgCnt));
		if (strAction.empty())
		{
			AddMsgToQueue("动作不能为空!", eve.fromGroup, false);
			return;
		}
		const string strReply = strNickName + strAction;
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(static_cast<unsigned char>(charNumElement)))
			{
				AddMsgToQueue(GlobalMsg["strSetInvalid"], eve.fromGroup, false);
				return;
			}
		if (strDefaultDice.length() > 3 && strDefaultDice != "1000")
		{
			AddMsgToQueue(GlobalMsg["strSetTooBig"], eve.fromGroup, false);
			return;
		}
		const int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		const string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
		AddMsgToQueue(strSetSuccessReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
		string strReply = strNickName;
		COC6D(strReply);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6")
	{
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ, false);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromGroup, false);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromGroup, false);
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ, false);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromGroup, false);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromGroup, false);
			return;
		}
		string strReply = strNickName;
		DND(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
		string strReply = strNickName;
		COC7D(strReply);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
	{
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ, false);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromGroup, false);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromGroup, false);
			return;
		}
		string strReply = strNickName;
		COC7(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
	{
		intMsgCnt += 2;
		string strSkillName;
		string strMainDice = "D100";
		string strSkillModify;
		int intSkillModify = 0;
		if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
			isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':'&& strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strSkillModify += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intSkillModify = stoi(strSkillModify);
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(
				eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strUnknownPropErr"], eve.fromGroup, false);
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromGroup, false);
			return;
		}
		else
		{
			intSkillVal = stoi(strSkillVal);
		}
		int intFianlSkillVal = intSkillVal + intSkillModify;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
		{
			AddMsgToQueue(GlobalMsg["strSuccessRateErr"], eve.fromGroup, false);
			return;
		}
		RD rdMainDice(strMainDice, eve.fromQQ);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromGroup, false);
			return;
		}
		const int intD100Res = rdMainDice.intTotal;
		string strReply = strNickName + "进行" + strSkillName + strSkillModify + "检定: " + rdMainDice.FormCompleteString() + "/" +
			to_string(intFianlSkillVal) + " ";
		if (intD100Res <= 5 && intD100Res <= intSkillVal)strReply += GlobalMsg["strRollCriticalSuccess"];
		else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
		else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
		else if (intD100Res <= intFianlSkillVal / 2)strReply += GlobalMsg["strRollHardSuccess"];
		else if (intD100Res <= intFianlSkillVal)strReply += GlobalMsg["strRollRegularSuccess"];
		else if (intD100Res <= 95)strReply += GlobalMsg["strRollFailure"];
		else strReply += GlobalMsg["strRollFumble"];
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		string strSkillName;
		string strMainDice = "D100";
		string strSkillModify;
		int intSkillModify = 0;
		if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
			isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':' && strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strSkillModify += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intSkillModify = stoi(strSkillModify);
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(
				eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strUnknownPropErr"], eve.fromGroup, false);
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromGroup, false);
			return;
		}
		else
		{
			intSkillVal = stoi(strSkillVal);
		}
		int intFianlSkillVal = intSkillVal + intSkillModify;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
		{
			AddMsgToQueue(GlobalMsg["strSuccessRateErr"], eve.fromGroup, false);
			return;
		}
		RD rdMainDice(strMainDice, eve.fromQQ);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromGroup, false);
			return;
		}
		const int intD100Res = rdMainDice.intTotal;
		string strReply = strNickName + "进行" + strSkillName + strSkillModify + "检定: " + rdMainDice.FormCompleteString() + "/" +
			to_string(intFianlSkillVal) + " ";
		if (intD100Res == 1)strReply += GlobalMsg["strRollCriticalSuccess"];
		else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
		else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
		else if (intD100Res <= intFianlSkillVal / 2)strReply += GlobalMsg["strRollHardSuccess"];
		else if (intD100Res <= intFianlSkillVal)strReply += GlobalMsg["strRollRegularSuccess"];
		else if (intD100Res <= 95)strReply += GlobalMsg["strRollFailure"];
		else strReply += GlobalMsg["strRollFumble"];
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw") {

	intMsgCnt += 4;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;

	string strDeckName;
	while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strDeckName += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strCardNum;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strCardNum += eve.message[intMsgCnt];
		intMsgCnt++;
	}
	auto intCardNum = strCardNum.empty() ? 1 : stoi(strCardNum);
	if (intCardNum == 0)
	{
		AddMsgToQueue(GlobalMsg["strNumCannotBeZero"], eve.fromGroup, false);
		return;
	}
	int intDeckRes = CardDeck::findDeck(strDeckName);
	string strReply;
	if (intDeckRes < 0) {
		strReply = strDeckName + "?" + GlobalMsg["strDeckNotFound"];
		AddMsgToQueue(strReply, eve.fromGroup, false);
		return;
	}
	vector<string>TempDeck(CardDeck::PublicDeck[intDeckRes]);
	TempDeck.erase(TempDeck.begin());
	strReply = "来看看" + strNickName + "的随机抽取结果:\n" + CardDeck::drawCard(TempDeck);
	while (--intCardNum && TempDeck.size()) {
		strReply += "\n" + CardDeck::drawCard(TempDeck);
		if (strReply.length() > 1000) {
			AddMsgToQueue(strReply, eve.fromGroup, false);
			strReply.clear();
		}
	}
	AddMsgToQueue(strReply, eve.fromGroup, false);
	if (intCardNum) {
		AddMsgToQueue(GlobalMsg["strDeckEmpty"], eve.fromGroup, false);
		return;
	}
}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'h'
		|| strLowerMessage[intMsgCnt] == 'd')
	{
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (eve.message[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[
					intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f'
				||
				strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
					intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
				&&
				strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
					intTmpMsgCnt
				] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (isspace(static_cast<unsigned char>(eve.message[intTmpMsgCnt])))
				intTmpMsgCnt++;
			strReason = eve.message.substr(intTmpMsgCnt);
		}
		else
			strReason = eve.message.substr(intMsgCnt);

		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromGroup, false);
				return;
			}
			if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromGroup, false);
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeExceeded"], eve.fromGroup, false);
				return;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeErr"], eve.fromGroup, false);
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromGroup, false);
				}
				else
				{
					strTurnNotice = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strTurnNotice;
					AddMsgToQueue(strTurnNotice, eve.fromQQ);
					const auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strTurnNotice, it->second);
						}
					}
				}
			}
		}
		RD rdMainDice(strMainDice, eve.fromQQ);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromGroup, false);
			return;
		}
		if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromGroup, false);
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				AddMsgToQueue(strAns, eve.fromGroup, false);
			}
			else
			{
				strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
				AddMsgToQueue(strAns, eve.fromQQ);
				const auto range = ObserveGroup.equal_range(eve.fromGroup);
				for (auto it = range.first; it != range.second; ++it)
				{
					if (it->second != eve.fromQQ)
					{
						AddMsgToQueue(strAns, it->second);
					}
				}
			}
		}
		else
		{
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromGroup, false);
				}
				else
				{
					strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
					AddMsgToQueue(strAns, eve.fromQQ);
					const auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strAns, it->second);
						}
					}
				}
			}
		}
		if (isHidden)
		{
			const string strReply = strNickName + "进行了一次暗骰";
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
	}
}

EVE_DiscussMsg_EX(eventDiscussMsg)
{
	if (boolNoDiscuss) {
		AddMsgToQueue(GlobalMsg["strNoDiscuss"], eve.fromDiscuss, false);
		Sleep(1000);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	if (boolPreserve&&WhiteGroup.count(eve.fromDiscuss)==0) {
		AddMsgToQueue(GlobalMsg["strPreserve"],eve.fromDiscuss,false);
		Sleep(1000);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	if (eve.isSystem())return;
	if (BlackQQ.count(eve.fromQQ)) {
		eve.message_block();
		return;
	}
	init(eve.message);
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	if (eve.message.substr(0, 6) == "[CQ:at")
	{
		if (eve.message.substr(0, strAt.length()) == strAt)
		{
			eve.message = eve.message.substr(strAt.length() + 1);
		}
		else
		{
			return;
		}
	}
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = getName(eve.fromQQ, eve.fromDiscuss);
	string strLowerMessage = eve.message;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), [](unsigned char c) { return tolower(c); });
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(
			static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				if (DisabledDiscuss.count(eve.fromDiscuss))
				{
					DisabledDiscuss.erase(eve.fromDiscuss);
					AddMsgToQueue("成功开启本机器人!", eve.fromDiscuss, false);
				}
				else
				{
					AddMsgToQueue("本机器人已经处于开启状态!", eve.fromDiscuss, false);
				}
			}
		}
		else if (Command == "off")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				if (!DisabledDiscuss.count(eve.fromDiscuss))
				{
					DisabledDiscuss.insert(eve.fromDiscuss);
					AddMsgToQueue("成功关闭本机器人!", eve.fromDiscuss, false);
				}
				else
				{
					AddMsgToQueue("本机器人已经处于关闭状态!", eve.fromDiscuss, false);
				}
			}
		}
		else
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				AddMsgToQueue(Dice_Full_Ver + PersonalMsg["strBotMsg"], eve.fromDiscuss, false);
			}
		}
		return;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum ==
			to_string(getLoginQQ()))
		{
			setDiscussLeave(eve.fromDiscuss);
		}
		return;
	}
	if (boolDisabledGlobal|| DisabledDiscuss.count(eve.fromDiscuss))
		return;
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		if (PersonalMsg.count("strHelpMsg"))
			AddMsgToQueue(PersonalMsg["strHelpMsg"], eve.fromDiscuss, false);
		else AddMsgToQueue(GlobalMsg["strHlpMsg"], eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			AddMsgToQueue(GlobalMsg["strStErr"], eve.fromDiscuss, false);
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss));
			}
			AddMsgToQueue(GlobalMsg["strPropCleared"], eve.fromDiscuss, false);
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "del")
		{
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
				intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(
				eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].erase(strSkillName);
				AddMsgToQueue(GlobalMsg["strPropDeleted"], eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPropNotFound"], eve.fromDiscuss, false);
			}
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 4) == "show")
		{
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
				intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(
				eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				AddMsgToQueue(format(GlobalMsg["strProp"], {
					strNickName, strSkillName,
					to_string(CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName])
				}), eve.fromDiscuss, false);
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				AddMsgToQueue(format(GlobalMsg["strProp"], {strNickName, strSkillName, to_string(SkillDefaultVal[strSkillName])}),
				              eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPropNotFound"], eve.fromDiscuss, false);
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
				isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
				!= ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
			] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			{
				strSkillVal += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 3)
			{
				boolError = true;
				break;
			}
			CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName] = stoi(strSkillVal);
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromDiscuss, false);
		}
		else
		{
			AddMsgToQueue(GlobalMsg["strSetPropSuccess"], eve.fromDiscuss, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		}
		else if (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			strinit += '+';
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strname = eve.message.substr(intMsgCnt);
		if (strname.empty())
			strname = strNickName;
		else
			strname = strip(strname);
		RD initdice(strinit);
		const int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromDiscuss, false);
			return;
		}
		ilInitList->insert(eve.fromDiscuss, initdice.intTotal, strname);
		const string strReply = strname + "的先攻骰点：" + strinit + '=' + to_string(initdice.intTotal);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init")
	{
		intMsgCnt += 4;
		string strReply;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (ilInitList->clear(eve.fromDiscuss))
				strReply = "成功清除先攻记录！";
			else
				strReply = "列表为空！";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
			return;
		}
		ilInitList->show(eve.fromDiscuss, strReply);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage[intMsgCnt] == 'w')
	{
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w')
		{
			intMsgCnt++;
			boolDetail = true;
		}
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
					intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
				&&
				strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
					intTmpMsgCnt
				] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		string strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (isspace(static_cast<unsigned char>(eve.message[intTmpMsgCnt])))
			intTmpMsgCnt++;
		string strReason = eve.message.substr(intTmpMsgCnt);


		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromDiscuss, false);
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeExceeded"], eve.fromDiscuss, false);
				return;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeErr"], eve.fromDiscuss, false);
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromDiscuss, false);
				}
				else
				{
					strTurnNotice = "在多人聊天中 " + strTurnNotice;
					AddMsgToQueue(strTurnNotice, eve.fromQQ);
					const auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strTurnNotice, it->second);
						}
					}
				}
			}
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
			                                            ? strMainDice.find('+')
			                                            : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(static_cast<unsigned char>(i)))
			{
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, eve.fromQQ);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromDiscuss, false);
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				AddMsgToQueue(strAns, eve.fromDiscuss, false);
			}
			else
			{
				strAns = "在多人聊天中 " + strAns;
				AddMsgToQueue(strAns, eve.fromQQ);
				const auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
				for (auto it = range.first; it != range.second; ++it)
				{
					if (it->second != eve.fromQQ)
					{
						AddMsgToQueue(strAns, it->second);
					}
				}
			}
		}
		else
		{
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromDiscuss, false);
				}
				else
				{
					strAns = "在多人聊天中 " + strAns;
					AddMsgToQueue(strAns, eve.fromQQ);
					const auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strAns, it->second);
						}
					}
				}
			}
		}
		if (isHidden)
		{
			const string strReply = strNickName + "进行了一次暗骰";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (DisabledOBDiscuss.count(eve.fromDiscuss))
			{
				DisabledOBDiscuss.erase(eve.fromDiscuss);
				AddMsgToQueue("成功在本多人聊天中启用旁观模式!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("在本多人聊天中旁观模式没有被禁用!", eve.fromDiscuss, false);
			}
			return;
		}
		if (Command == "off")
		{
			if (!DisabledOBDiscuss.count(eve.fromDiscuss))
			{
				DisabledOBDiscuss.insert(eve.fromDiscuss);
				ObserveDiscuss.clear();
				AddMsgToQueue("成功在本多人聊天中禁用旁观模式!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("在本多人聊天中旁观模式没有被启用!", eve.fromDiscuss, false);
			}
			return;
		}
		if (DisabledOBDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue("在本多人聊天中旁观模式已被禁用!", eve.fromDiscuss, false);
			return;
		}
		if (Command == "list")
		{
			string Msg = "当前的旁观者有:";
			const auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				Msg += "\n" + getName(it->second, eve.fromDiscuss) + "(" + to_string(it->second) + ")";
			}
			const string strReply = Msg == "当前的旁观者有:" ? "当前暂无旁观者" : Msg;
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
		else if (Command == "clr")
		{
			ObserveDiscuss.erase(eve.fromDiscuss);
			AddMsgToQueue("成功删除所有旁观者!", eve.fromDiscuss, false);
		}
		else if (Command == "exit")
		{
			const auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					ObserveDiscuss.erase(it);
					const string strReply = strNickName + "成功退出旁观模式!";
					AddMsgToQueue(strReply, eve.fromDiscuss, false);
					eve.message_block();
					return;
				}
			}
			const string strReply = strNickName + "没有加入旁观模式!";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
		else
		{
			const auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					const string strReply = strNickName + "已经处于旁观模式!";
					AddMsgToQueue(strReply, eve.fromDiscuss, false);
					eve.message_block();
					return;
				}
			}
			ObserveDiscuss.insert(make_pair(eve.fromDiscuss, eve.fromQQ));
			const string strReply = strNickName + "成功加入旁观模式!";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "的疯狂发作-临时症状:\n";
		TempInsane(strAns);
		AddMsgToQueue(strAns, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "的疯狂发作-总结症状:\n";
		LongInsane(strAns);
		AddMsgToQueue(strAns, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string San;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{
			AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromDiscuss, false);

			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[
			SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count("理智")))
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromDiscuss, false);

			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ, false);
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ, false);
				return;
			}
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{
			AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromDiscuss, false);

			return;
		}
		if (San.length() >= 3)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromDiscuss, false);

			return;
		}
		const int intSan = San.empty()
			                   ? CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"]
			                   : stoi(San);
		if (intSan == 0)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromDiscuss, false);

			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " 成功\n你的San值减少" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSuc.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdSuc.intTotal
				);
			}
		}
		else if (intTmpRollRes == 100 || (intSan < 50 && intTmpRollRes > 95))

		{
			strAns += " 大失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdFail.Max();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "最大值=" + to_string(rdFail.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdFail.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdFail.intTotal
				);
			}
		}
		else
		{
			strAns += " 失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdFail.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdFail.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdFail.intTotal
				);
			}
		}
		AddMsgToQueue(strAns, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])) && !isspace(static_cast<unsigned char>(eve.message[intMsgCnt]))
		)
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(
				eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strEnValInvalid"], eve.fromDiscuss, false);
				return;
			}
		}
		else
		{
			if (strCurrentValue.length() > 3)
			{
				AddMsgToQueue(GlobalMsg["strEnValInvalid"], eve.fromDiscuss, false);

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}
		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			const int intTmpRollD10 = RandomGenerator::Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName] = intCurrentVal +
					intTmpRollD10;
			}
		}
		AddMsgToQueue(strAns, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		if (boolDisabledJrrpGlobal) {
			AddMsgToQueue(GlobalMsg["strDisabledJrrpGlobal"], eve.fromDiscuss,false);
			return;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (DisabledJRRPDiscuss.count(eve.fromDiscuss))
			{
				DisabledJRRPDiscuss.erase(eve.fromDiscuss);
				AddMsgToQueue("成功在此多人聊天中启用JRRP!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("在此多人聊天中JRRP没有被禁用!", eve.fromDiscuss, false);
			}
			return;
		}
		if (Command == "off")
		{
			if (!DisabledJRRPDiscuss.count(eve.fromDiscuss))
			{
				DisabledJRRPDiscuss.insert(eve.fromDiscuss);
				AddMsgToQueue("成功在此多人聊天中禁用JRRP!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("在此多人聊天中JRRP没有被启用!", eve.fromDiscuss, false);
			}
			return;
		}
		if (DisabledJRRPDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue("在此多人聊天中JRRP已被禁用!", eve.fromDiscuss, false);
			return;
		}
		string des;
		string data =  "QQ=" + to_string(CQ::getLoginQQ()) + "&v=20190114" + "&QueryQQ=" + to_string(eve.fromQQ);
		char *frmdata = new char[data.length() + 1];
		strcpy_s(frmdata, data.length() + 1, data.c_str());
		bool res = Network::POST("api.kokona.tech", "/jrrp", 5555, frmdata, des);
		delete[] frmdata;
		if (res)
		{
			AddMsgToQueue(format(GlobalMsg["strJrrp"], { strNickName, des }), eve.fromDiscuss, false);
		}
		else
		{
			AddMsgToQueue(format(GlobalMsg["strJrrpErr"], { des }), eve.fromDiscuss, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
	{
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		string type;
		while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			type += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}

		auto nameType = NameGenerator::Type::UNKNOWN;
		if (type == "cn")
			nameType = NameGenerator::Type::CN;
		else if (type == "en")
			nameType = NameGenerator::Type::EN;
		else if (type == "jp")
			nameType = NameGenerator::Type::JP;

		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;

		string strNum;
		while (isdigit(static_cast<unsigned char>(eve.message[intMsgCnt])))
		{
			strNum += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.size() > 2)
		{
			AddMsgToQueue(GlobalMsg["strNameNumTooBig"], eve.fromDiscuss, false);
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strNameNumTooBig"], eve.fromDiscuss, false);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strNameNumCannotBeZero"], eve.fromDiscuss, false);
			return;
		}
		vector<string> TempNameStorage;
		while (TempNameStorage.size() != intNum)
		{
			string name = NameGenerator::getRandomName(nameType);
			if (find(TempNameStorage.begin(), TempNameStorage.end(), name) == TempNameStorage.end())
			{
				TempNameStorage.push_back(name);
			}
		}
		string strReply = strNickName + "的随机名称:\n";
		for (auto i = 0; i != TempNameStorage.size(); i++)
		{
			strReply.append(TempNameStorage[i]);
			if (i != TempNameStorage.size() - 1)strReply.append(", ");
		}
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string type = strLowerMessage.substr(intMsgCnt, 2);
		string name;
		if (type == "cn")
			name = NameGenerator::getChineseName();
		else if (type == "en")
			name = NameGenerator::getEnglishName();
		else if (type == "jp")
			name = NameGenerator::getJapaneseName();
		else
			name = NameGenerator::getRandomName();
		Name->set(eve.fromDiscuss, eve.fromQQ, name);
		const string strReply = "已将" + strNickName + "的名称更改为" + name;
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
	intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string name = eve.message.substr(intMsgCnt);
		if (name.length() > 50)
		{
			AddMsgToQueue(GlobalMsg["strNameTooLongErr"], eve.fromDiscuss, false);
			return;
		}
		if (!name.empty())
		{
			Name->set(eve.fromDiscuss, eve.fromQQ, name);
			const string strReply = "已将" + strNickName + "的名称更改为" + strip(name);
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
		else
		{
			if (Name->del(eve.fromDiscuss, eve.fromQQ))
			{
				const string strReply = "已将" + strNickName + "的名称删除";
				AddMsgToQueue(strReply, eve.fromDiscuss, false);
			}
			else
			{
				const string strReply = strNickName + GlobalMsg["strNameDelErr"];
				AddMsgToQueue(strReply, eve.fromDiscuss, false);
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])) || eve.message[intMsgCnt] == ':')
				intMsgCnt++;
			string strDefaultRule = eve.message.substr(intMsgCnt);
			if (strDefaultRule.empty()) {
				DefaultRule.erase(eve.fromQQ);
				AddMsgToQueue(GlobalMsg["strRuleReset"], eve.fromDiscuss, false);
			}
			else {
				for (auto& n : strDefaultRule)
					n = toupper(static_cast<unsigned char>(n));
				DefaultRule[eve.fromQQ] = strDefaultRule;
				AddMsgToQueue(GlobalMsg["strRuleSet"], eve.fromDiscuss, false);
			}
		}
		else {
			string strSearch = eve.message.substr(intMsgCnt);
			string strDefaultRule = DefaultRule[eve.fromQQ];
			for (auto& n : strSearch)
				n = toupper(static_cast<unsigned char>(n));
			string strReturn;
			if (DefaultRule.count(eve.fromQQ) && strSearch.find(':') == string::npos && GetRule::get(strDefaultRule, strSearch, strReturn)) {
				AddMsgToQueue(strReturn, eve.fromDiscuss, false);
			}
			else if (GetRule::analyze(strSearch, strReturn))
			{
				AddMsgToQueue(strReturn, eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strRuleErr"] + strReturn, eve.fromDiscuss, false);
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (strAction == "on")
		{
			if (DisabledMEDiscuss.count(eve.fromDiscuss))
			{
				DisabledMEDiscuss.erase(eve.fromDiscuss);
				AddMsgToQueue("成功在本多人聊天中启用.me命令!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("在本多人聊天中.me命令没有被禁用!", eve.fromDiscuss, false);
			}
			return;
		}
		if (strAction == "off")
		{
			if (!DisabledMEDiscuss.count(eve.fromDiscuss))
			{
				DisabledMEDiscuss.insert(eve.fromDiscuss);
				AddMsgToQueue("成功在本多人聊天中禁用.me命令!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("在本多人聊天中.me命令没有被启用!", eve.fromDiscuss, false);
			}
			return;
		}
		if (boolDisabledMeGlobal)
		{
			AddMsgToQueue(GlobalMsg["strDisabledMeGlobal"], eve.fromDiscuss, false);
			return;
		}
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue("在本多人聊天中.me命令已被禁用!", eve.fromDiscuss, false);
			return;
		}
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue(GlobalMsg["strMEDisabledErr"], eve.fromDiscuss, false);
			return;
		}
		strAction = strip(eve.message.substr(intMsgCnt));
		if (strAction.empty())
		{
			AddMsgToQueue("动作不能为空!", eve.fromDiscuss, false);
			return;
		}
		const string strReply = strNickName + strAction;
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(static_cast<unsigned char>(charNumElement)))
			{
				AddMsgToQueue(GlobalMsg["strSetInvalid"], eve.fromDiscuss, false);
				return;
			}
		if (strDefaultDice.length() > 3 && strDefaultDice !="1000" )
		{
			AddMsgToQueue(GlobalMsg["strSetTooBig"], eve.fromDiscuss, false);
			return;
		}
		const int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		const string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
		AddMsgToQueue(strSetSuccessReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
		string strReply = strNickName;
		COC6D(strReply);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6")
	{
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ, false);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromDiscuss, false);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromDiscuss, false);
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ, false);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromDiscuss, false);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromDiscuss, false);
			return;
		}
		string strReply = strNickName;
		DND(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
		string strReply = strNickName;
		COC7D(strReply);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
	{
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromQQ, false);
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(GlobalMsg["strCharacterTooBig"], eve.fromDiscuss, false);
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(GlobalMsg["strCharacterCannotBeZero"], eve.fromDiscuss, false);
			return;
		}
		string strReply = strNickName;
		COC7(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
	{
		intMsgCnt += 2;
		string strSkillName;
		string strMainDice = "D100";
		string strSkillModify;
		int intSkillModify = 0;
		if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
			isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':' && strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strSkillModify += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intSkillModify = stoi(strSkillModify);
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(
				eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strUnknownPropErr"], eve.fromDiscuss, false);
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromDiscuss, false);
			return;
		}
		else
		{
			intSkillVal = stoi(strSkillVal);
		}
		int intFianlSkillVal = intSkillVal + intSkillModify;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
		{
			AddMsgToQueue(GlobalMsg["strSuccessRateErr"], eve.fromDiscuss, false);
			return;
		}
		RD rdMainDice(strMainDice, eve.fromQQ);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		const int intD100Res = rdMainDice.intTotal;
		string strReply = strNickName + "进行" + strSkillName + strSkillModify + "检定: " + rdMainDice.FormCompleteString() + "/" +
			to_string(intFianlSkillVal) + " ";
		if (intD100Res <= 5 && intD100Res <= intSkillVal)strReply += GlobalMsg["strRollCriticalSuccess"];
		else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
		else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
		else if (intD100Res <= intFianlSkillVal / 2)GlobalMsg["strRollHardSuccess"];
		else if (intD100Res <= intFianlSkillVal)GlobalMsg["strRollRegularSuccess"];
		else if (intD100Res <= 95)strReply += GlobalMsg["strRollFailure"];
		else strReply += GlobalMsg["strRollFumble"];
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		string strSkillName;
		string strMainDice = "D100";
		string strSkillModify;
		int intSkillModify = 0;
		if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') {
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
			isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strSkillModify += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strSkillModify += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			intSkillModify = stoi(strSkillModify);
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(
				eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strUnknownPropErr"], eve.fromDiscuss, false);
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(GlobalMsg["strPropErr"], eve.fromDiscuss, false);
			return;
		}
		else
		{
			intSkillVal = stoi(strSkillVal);
		}
		int intFianlSkillVal = intSkillVal + intSkillModify;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
		{
			AddMsgToQueue(GlobalMsg["strSuccessRateErr"], eve.fromDiscuss, false);
			return;
		}
		RD rdMainDice(strMainDice, eve.fromQQ);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		const int intD100Res = rdMainDice.intTotal;
		string strReply = strNickName + "进行" + strSkillName + strSkillModify + "检定: " + rdMainDice.FormCompleteString() + "/" +
			to_string(intFianlSkillVal) + " ";
		if (intD100Res == 1)strReply += GlobalMsg["strRollCriticalSuccess"];
		else if (intD100Res == 100)strReply += GlobalMsg["strRollFumble"];
		else if (intD100Res <= intFianlSkillVal / 5)strReply += GlobalMsg["strRollExtremeSuccess"];
		else if (intD100Res <= intFianlSkillVal / 2)strReply += GlobalMsg["strRollHardSuccess"];
		else if (intD100Res <= intFianlSkillVal)strReply += GlobalMsg["strRollRegularSuccess"];
		else if (intD100Res <= 95 || intFianlSkillVal >= 50)strReply += GlobalMsg["strRollFailure"];
		else strReply += GlobalMsg["strRollFumble"];
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw") {

	intMsgCnt += 4;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || (strLowerMessage[intMsgCnt] == '.'))
		intMsgCnt++;

	string strDeckName;
	while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strDeckName += strLowerMessage[intMsgCnt];
		intMsgCnt++;
	}
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	string strCardNum;
	while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
	{
		strCardNum += eve.message[intMsgCnt];
		intMsgCnt++;
	}
	auto intCardNum = strCardNum.empty() ? 1 : stoi(strCardNum);
	if (intCardNum == 0)
	{
		AddMsgToQueue(GlobalMsg["strNumCannotBeZero"], eve.fromDiscuss, false);
		return;
	}
	int intDeckRes = CardDeck::findDeck(strDeckName);
	string strReply;
	if (intDeckRes < 0) {
		strReply = strDeckName + "?" + GlobalMsg["strDeckNotFound"];
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
		return;
	}
	vector<string>TempDeck(CardDeck::PublicDeck[intDeckRes]);
	TempDeck.erase(TempDeck.begin());
	strReply = "来看看" + strNickName + "的随机抽取结果:\n" + CardDeck::drawCard(TempDeck);
	while (--intCardNum && TempDeck.size()) {
		strReply += "\n" + CardDeck::drawCard(TempDeck);
		if (strReply.length() > 1000) {
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
			strReply.clear();
		}
	}
	AddMsgToQueue(strReply, eve.fromDiscuss, false);
	if (intCardNum) {
		AddMsgToQueue(GlobalMsg["strDeckEmpty"], eve.fromDiscuss, false);
		return;
	}
}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'h'
		|| strLowerMessage[intMsgCnt] == 'd')
	{
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (eve.message[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (isspace(static_cast<unsigned char>(eve.message[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[
					intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f'
				||
				strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(static_cast<unsigned char>(strLowerMessage[intTmpMsgCnt])) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
					intTmpMsgCnt] != 'k' && strLowerMessage[intTmpMsgCnt] != 'p' && strLowerMessage[intTmpMsgCnt] != 'b'
				&&
				strLowerMessage[intTmpMsgCnt] != 'f' && strLowerMessage[intTmpMsgCnt] != '+' && strLowerMessage[
					intTmpMsgCnt
				] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (isspace(static_cast<unsigned char>(eve.message[intTmpMsgCnt])))
				intTmpMsgCnt++;
			strReason = eve.message.substr(intTmpMsgCnt);
		}
		else
			strReason = eve.message.substr(intMsgCnt);

		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromDiscuss, false);
				return;
			}
			if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromDiscuss, false);
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeExceeded"], eve.fromDiscuss, false);
				return;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(GlobalMsg["strRollTimeErr"], eve.fromDiscuss, false);
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromDiscuss, false);
				}
				else
				{
					strTurnNotice = "在多人聊天中 " + strTurnNotice;
					AddMsgToQueue(strTurnNotice, eve.fromQQ);
					const auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strTurnNotice, it->second);
						}
					}
				}
			}
		}
		RD rdMainDice(strMainDice, eve.fromQQ);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			AddMsgToQueue(GlobalMsg["strValueErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(GlobalMsg["strInputErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroDiceErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(GlobalMsg["strZeroTypeErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strDiceTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(GlobalMsg["strTypeTooBigErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(GlobalMsg["strAddDiceValErr"], eve.fromDiscuss, false);
			return;
		}
		if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(GlobalMsg["strUnknownErr"], eve.fromDiscuss, false);
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				AddMsgToQueue(strAns, eve.fromDiscuss, false);
			}
			else
			{
				strAns = "在多人聊天中 " + strAns;
				AddMsgToQueue(strAns, eve.fromQQ);
				const auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
				for (auto it = range.first; it != range.second; ++it)
				{
					if (it->second != eve.fromQQ)
					{
						AddMsgToQueue(strAns, it->second);
					}
				}
			}
		}
		else
		{
			while (intTurnCnt--)
			{
				// 此处返回值无用
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromDiscuss, false);
				}
				else
				{
					strAns = "在多人聊天中 " + strAns;
					AddMsgToQueue(strAns, eve.fromQQ);
					const auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; ++it)
					{
						if (it->second != eve.fromQQ)
						{
							AddMsgToQueue(strAns, it->second);
						}
					}
				}
			}
		}
		if (isHidden)
		{
			const string strReply = strNickName + "进行了一次暗骰";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
	}
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
		AddMsgToQueue(strReply, fromGroup, false);
	}
	else if(beingOperateQQ == getLoginQQ()){
		if (boolPreserve&&getGroupMemberInfo(fromGroup, masterQQ).QQID != masterQQ&&WhiteGroup.count(fromGroup)==0&&WhiteQQ.count(fromQQ)==0) {
			AddMsgToQueue(GlobalMsg["strPreserve"], fromGroup, false);
			setGroupLeave(fromGroup);
			return 0;
		}
		else if(PersonalMsg.count("strAddGroup")) {
			AddMsgToQueue(PersonalMsg["strAddGroup"], fromGroup, false);
		}
	}
	return 0;
}

EVE_System_GroupMemberDecrease(eventGroupMemberDecrease) {
	if (beingOperateQQ == getLoginQQ()) {
		if (masterQQ&&boolMasterMode) {
			string strMsg = to_string(fromQQ)+"将本骰娘移出了群" + to_string(fromGroup) + "！";
			AddMsgToQueue(strMsg, masterQQ);
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
		setGroupAddRequest(responseFlag,2, 1,"");
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
				WhiteGroup.insert(fromQQ);
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else{
				strMsg += "已同意";
				setGroupAddRequest(responseFlag, 2, 1, "");
			}
			AddMsgToQueue(strMsg, masterQQ);
		}
	}
	return 1;
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
