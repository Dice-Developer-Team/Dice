/*
* Dice! QQ Dice Robot for TRPG
* Copyright (C) 2018 w4123溯洄
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
#include "RD.h"
#include "CQEVE_ALL.h"
#include "CQTools.h"
#include "InitList.h"
#include "GlobalVar.h"
#include "NameStorage.h"
#include "GetRule.h"
#include "DiceMsgSend.h"
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

unique_ptr<NameStorage> Name;
unique_ptr<GetRule> RuleGetter;
std::string strip(std::string origin)
{
	bool flag = true;
	while (flag)
	{
		flag = false;
		if (origin[0]=='!' || origin[0] == '.')
		{
			origin.erase(origin.begin());
			flag = true;
		}
		else if (origin.substr(0,2) == "！"||origin.substr(0,2) == "。")
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
		if (GroupID<1000000000)
		{
			return strip(Name->get(GroupID, QQ).empty() ? (getGroupMemberInfo(GroupID, QQ).GroupNick.empty() ? getStrangerInfo(QQ).nick : getGroupMemberInfo(GroupID, QQ).GroupNick) : Name->get(GroupID, QQ));
		}
		/*讨论组*/
		return strip(Name->get(GroupID, QQ).empty() ? getStrangerInfo(QQ).nick : Name->get(GroupID, QQ));
	}
	/*私聊*/
	return strip(getStrangerInfo(QQ).nick);
}
map<long long, RP> JRRP;
map<long long, int> DefaultDice;
map<long long, string> WelcomeMsg;
set<long long> DisabledGroup;
set<long long> DisabledDiscuss;
set<long long> DisabledJRRPGroup;
set<long long> DisabledJRRPDiscuss;
set<long long> DisabledMEGroup;
set<long long> DisabledMEDiscuss;
set<long long> DisabledHELPGroup;
set<long long> DisabledHELPDiscuss;
set<long long> DisabledOBGroup;
set<long long> DisabledOBDiscuss;
unique_ptr<Initlist> ilInitList;
struct SourceType {
	SourceType(long long a, int b, long long c) :QQ(a), Type(b), GrouporDiscussID(c) {};
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
EVE_Enable(__eventEnable)
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
	ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
	if (ifstreamCharacterProp)
	{
		long long QQ, GrouporDiscussID;
		int Type, Value;
		string SkillName;
		while (ifstreamCharacterProp >> QQ >> Type >> GrouporDiscussID >> SkillName >> Value) {
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
	ifstream ifstreamJRRP(strFileLoc + "JRRP.RDconf");
	if (ifstreamJRRP)
	{
		long long QQ;
		int Val;
		string strDate;
		while (ifstreamJRRP >> QQ >> strDate >> Val)
		{
			JRRP[QQ].Date = strDate;
			JRRP[QQ].RPVal = Val;
		}
	}
	ifstreamJRRP.close();
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
	ilInitList = make_unique<Initlist>(strFileLoc + "INIT.DiceDB");
	RuleGetter = make_unique<GetRule>();
	return 0;
}


EVE_PrivateMsg_EX(__eventPrivateMsg)
{
	if (eve.isSystem())return;
	init(eve.message);
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (isspace(eve.message[intMsgCnt]))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = getName(eve.fromQQ);
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		AddMsgToQueue(strHlpMsg, eve.fromQQ);
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
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;

		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#'&& strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		string strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (isspace(eve.message[intTmpMsgCnt]))
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
				AddMsgToQueue(strValueErr, eve.fromQQ);
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(strInputErr, eve.fromQQ);
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(strZeroDiceErr, eve.fromQQ);
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(strZeroTypeErr, eve.fromQQ);
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(strDiceTooBigErr, eve.fromQQ);
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(strTypeTooBigErr, eve.fromQQ);
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(strAddDiceValErr, eve.fromQQ);
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(strUnknownErr, eve.fromQQ);
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(strRollTimeExceeded, eve.fromQQ);
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(strRollTimeErr, eve.fromQQ);
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				const string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				AddMsgToQueue(strTurnNotice, eve.fromQQ);
			}
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-') ? strMainDice.find('+') : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(i))
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
			AddMsgToQueue(strValueErr, eve.fromQQ);
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromQQ);			
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
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
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
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string San;
		while (isdigit(strLowerMessage[intMsgCnt])) {
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{

			AddMsgToQueue(strSCInvalid, eve.fromQQ);			

			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count("理智")))
		{

			AddMsgToQueue(strSanInvalid, eve.fromQQ);			

			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if(!isdigit(character) && character != 'D' && character!='d' && character != '+' && character != '-')
			{
				AddMsgToQueue(strSCInvalid, eve.fromQQ);				
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(strSCInvalid, eve.fromQQ);				
				return;
			}
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{

			AddMsgToQueue(strSCInvalid, eve.fromQQ);			

			return;
		}
		if (San.length() >= 3)
		{

			AddMsgToQueue(strSanInvalid, eve.fromQQ);			

			return;
		}
		const int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] : stoi(San);
		if (intSan == 0)
		{

			AddMsgToQueue(strSanInvalid, eve.fromQQ);			

			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
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
		AddMsgToQueue(Dice_Full_Ver, eve.fromQQ);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt]))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(eve.message[intMsgCnt]))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strEnValInvalid, eve.fromQQ);				
				return;
			}

		}
		else
		{
			if (strCurrentValue.length() > 3)
			{

				AddMsgToQueue(strEnValInvalid, eve.fromQQ);				

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}

		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			const int intTmpRollD10 = Randint(1, 10);
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
		char cstrDate[100] = {};
		time_t time_tTime = 0;
		time(&time_tTime);
		tm tmTime{};
		localtime_s(&tmTime, &time_tTime);
		strftime(cstrDate, 100, "%F", &tmTime);
		if (JRRP.count(eve.fromQQ) && JRRP[eve.fromQQ].Date == cstrDate)
		{
			const string strReply = strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal);

			AddMsgToQueue(strReply, eve.fromQQ);			

		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int> (GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int> (NormalDistribution(Generator));
			} while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			const string strReply(strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal));

			AddMsgToQueue(strReply, eve.fromQQ);			

		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto &n : strSearch)
			n = toupper(n);
		string strReturn;
		if (RuleGetter->analyze(strSearch, strReturn))
		{
			AddMsgToQueue(strReturn, eve.fromQQ);			
		}
		else
		{
			AddMsgToQueue(strRuleErr + strReturn, eve.fromQQ);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			AddMsgToQueue(strStErr, eve.fromQQ);			
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, PrivateT, 0));
			}
			AddMsgToQueue(strPropCleared, eve.fromQQ);			
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			intMsgCnt += 3;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].erase(strSkillName);
				AddMsgToQueue(strPropDeleted, eve.fromQQ);				
			}
			else
			{
				AddMsgToQueue(strPropNotFound, eve.fromQQ);				
			}
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				AddMsgToQueue(format(strProp,{ strNickName,strSkillName,to_string(CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName]) }), eve.fromQQ);				
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				AddMsgToQueue(format(strProp,{ strNickName,strSkillName,to_string(SkillDefaultVal[strSkillName]) }), eve.fromQQ);				
			}
			else
			{
				AddMsgToQueue(strPropNotFound, eve.fromQQ);				
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(strLowerMessage[intMsgCnt]))
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
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError) {
			AddMsgToQueue(strPropErr, eve.fromQQ);			
		}
		else {

			AddMsgToQueue(strSetPropSuccess, eve.fromQQ);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strGroupID;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strGroupID += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strAction = strLowerMessage.substr(intMsgCnt);

		for (auto i : strGroupID)
		{
			if (!isdigit(i))
			{
				AddMsgToQueue(strGroupIDInvalid, eve.fromQQ);				
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
			AddMsgToQueue(strDisabledErr, eve.fromQQ);			
			return;
		}
		if (DisabledMEGroup.count(llGroupID))
		{
			AddMsgToQueue(strMEDisabledErr, eve.fromQQ);			
			return;
		}
		string strReply = getName(eve.fromQQ,llGroupID) + eve.message.substr(intMsgCnt);
		const int intSendRes = sendGroupMsg(llGroupID, strReply);
		if (intSendRes < 0)
		{
			AddMsgToQueue(strSendErr, eve.fromQQ);			
		}
		else
		{
			AddMsgToQueue("命令执行成功!", eve.fromQQ);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(charNumElement))
			{
				AddMsgToQueue(strSetInvalid, eve.fromQQ);				
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			AddMsgToQueue(strSetTooBig, eve.fromQQ);			
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromQQ);			
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromQQ);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromQQ);			
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromQQ);			
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strUnknownPropErr, eve.fromQQ);				
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(strPropErr, eve.fromQQ);			
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res) + "/" + to_string(intSkillVal) + " ";
		if (intD100Res <= 5)strReply += "大成功";
		else if (intD100Res <= intSkillVal / 5)strReply += "极难成功";
		else if (intD100Res <= intSkillVal / 2)strReply += "困难成功";
		else if (intD100Res <= intSkillVal)strReply += "成功";
		else if (intD100Res <= 95)strReply += "失败";
		else strReply += "大失败";
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strUnknownPropErr, eve.fromQQ);				
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(strPropErr, eve.fromQQ);			
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res) + "/" + to_string(intSkillVal) + " ";
		if (intD100Res == 1)strReply += "大成功";
		else if (intD100Res <= intSkillVal / 5)strReply += "极难成功";
		else if (intD100Res <= intSkillVal / 2)strReply += "困难成功";
		else if (intD100Res <= intSkillVal)strReply += "成功";
		else if (intD100Res <= 95 || (intSkillVal >= 50 && intD100Res != 100))strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromQQ);		
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'd')
	{
		intMsgCnt += 1;
		bool boolDetail = true;
		if (eve.message[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f' || strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (isspace(eve.message[intTmpMsgCnt]))
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
				AddMsgToQueue(strValueErr, eve.fromQQ);				
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(strInputErr, eve.fromQQ);				
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(strZeroDiceErr, eve.fromQQ);				
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(strZeroTypeErr, eve.fromQQ);				
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(strDiceTooBigErr, eve.fromQQ);				
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(strTypeTooBigErr, eve.fromQQ);				
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(strAddDiceValErr, eve.fromQQ);				
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(strUnknownErr, eve.fromQQ);				
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(strRollTimeExceeded, eve.fromQQ);				
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(strRollTimeErr, eve.fromQQ);				
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
			AddMsgToQueue(strValueErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromQQ);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromQQ);			
			return;
		}
		if (!boolDetail&&intTurnCnt != 1)
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
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
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
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				AddMsgToQueue(strAns, eve.fromQQ);				

			}
		}
	}
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			AddMsgToQueue("命令输入错误!", eve.fromQQ);			
		}
	}
}
EVE_GroupMsg_EX(__eventGroupMsg)
{
	if (eve.isSystem() || eve.isAnonymous())return;
	init(eve.message);
	while (isspace(eve.message[0]))
		eve.message.erase(eve.message.begin());
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	if (eve.message.substr(0, 6) == "[CQ:at") {
		if (eve.message.substr(0, strAt.length()) == strAt) {
			eve.message = eve.message.substr(strAt.length());
		}
		else {
			return;
		}
	}
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (isspace(eve.message[intMsgCnt]))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = getName(eve.fromQQ, eve.fromGroup);
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string QQNum = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
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
					AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);					
				}
			}
		}
		else if (Command == "off")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
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
					AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);					
				}
			}
		}
		else
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				AddMsgToQueue(Dice_Full_Ver, eve.fromGroup, false);				
			}
		}
		return;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string QQNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum == to_string(getLoginQQ()))
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				setGroupLeave(eve.fromGroup);
			}
			else
			{
				AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);				
			}
		}
		return;
	}
	if (DisabledGroup.count(eve.fromGroup))
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
				AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);				
			}
			return;
		}
		else if (strAction == "off")
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
				AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);				
			}
			return;
		}
		if (DisabledHELPGroup.count(eve.fromGroup))
		{
			AddMsgToQueue(strHELPDisabledErr, eve.fromGroup, false);			
			return;
		}
		AddMsgToQueue(strHlpMsg, eve.fromGroup, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome") {
		intMsgCnt += 7;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
		{
			string strWelcomeMsg = eve.message.substr(intMsgCnt);
			if (strWelcomeMsg.empty())
			{
				if (WelcomeMsg.count(eve.fromGroup))
				{
					WelcomeMsg.erase(eve.fromGroup);
					AddMsgToQueue(strWelcomeMsgClearNotice, eve.fromGroup, false);					
				}
				else
				{
					AddMsgToQueue(strWelcomeMsgClearErr, eve.fromGroup, false);					
				}
			}
			else {
				WelcomeMsg[eve.fromGroup] = strWelcomeMsg;
				AddMsgToQueue(strWelcomeMsgUpdateNotice, eve.fromGroup, false);				
			}
		}
		else
		{
			AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			AddMsgToQueue(strStErr, eve.fromGroup, false);			
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, GroupT, eve.fromGroup));
			}
			AddMsgToQueue(strPropCleared, eve.fromGroup, false);			
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			intMsgCnt += 3;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].erase(strSkillName);
				AddMsgToQueue(strPropDeleted, eve.fromGroup, false);				
			}
			else
			{
				AddMsgToQueue(strPropNotFound, eve.fromGroup, false);				
			}
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				AddMsgToQueue(format(strProp,{ strNickName,strSkillName,to_string(CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName]) }), eve.fromGroup, false);				
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				AddMsgToQueue(format(strProp,{ strNickName,strSkillName,to_string(SkillDefaultVal[strSkillName]) }), eve.fromGroup, false);				
			}
			else
			{
				AddMsgToQueue(strPropNotFound, eve.fromGroup, false);				
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(strLowerMessage[intMsgCnt]))
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
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError) {
			AddMsgToQueue(strPropErr, eve.fromGroup, false);			
		}
		else {

			AddMsgToQueue(strSetPropSuccess, eve.fromGroup, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		}
		else if (isdigit(strLowerMessage[intMsgCnt]))
			strinit += '+';
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
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
			AddMsgToQueue(strValueErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromGroup, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
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
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;

		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#'&& strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		string strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (isspace(eve.message[intTmpMsgCnt]))
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
				AddMsgToQueue(strValueErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(strInputErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(strZeroDiceErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(strZeroTypeErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(strDiceTooBigErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(strTypeTooBigErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(strAddDiceValErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(strUnknownErr, eve.fromGroup, false);				
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(strRollTimeExceeded, eve.fromGroup, false);				
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(strRollTimeErr, eve.fromGroup, false);				
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
			AddMsgToQueue(strEmptyWWDiceErr, eve.fromGroup, false);			
			return;
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-') ? strMainDice.find('+') : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(i))
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
			AddMsgToQueue(strValueErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromGroup, false);			
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
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
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
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
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
		while (isspace(strLowerMessage[intMsgCnt]))
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
					AddMsgToQueue( "本群中旁观模式没有被禁用!", eve.fromGroup, false);					
				}
			}
			else
			{
				AddMsgToQueue( "你没有权限执行此命令!", eve.fromGroup, false);				
			}
			return;
		}
		else if (Command == "off")
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
					AddMsgToQueue( "本群中旁观模式没有被启用!", eve.fromGroup, false);					
				}
			}
			else
			{
				AddMsgToQueue( "你没有权限执行此命令!", eve.fromGroup, false);				
			}
			return;
		}
		if (DisabledOBGroup.count(eve.fromGroup))
		{
			AddMsgToQueue( "在本群中旁观模式已被禁用!", eve.fromGroup, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string San;
		while (isdigit(strLowerMessage[intMsgCnt])) {
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{
			AddMsgToQueue(strSCInvalid, eve.fromGroup, false);			
			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count("理智")))
		{
			AddMsgToQueue(strSanInvalid, eve.fromGroup, false);			
			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(strSCInvalid, eve.fromQQ);				
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(strSCInvalid, eve.fromQQ);				
				return;
			}
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{
			AddMsgToQueue(strSCInvalid, eve.fromGroup, false);			
			return;
		}
		if (San.length() >= 3)
		{
			AddMsgToQueue(strSanInvalid, eve.fromGroup, false);			
			return;
		}
		const int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] : stoi(San);
		if (intSan == 0)
		{
			AddMsgToQueue(strSanInvalid, eve.fromGroup, false);			
			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt]))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(eve.message[intMsgCnt]))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strEnValInvalid, eve.fromGroup, false);				
				return;
			}

		}
		else
		{
			if (strCurrentValue.length() > 3)
			{

				AddMsgToQueue(strEnValInvalid, eve.fromGroup, false);				

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}

		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			const int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName] = intCurrentVal + intTmpRollD10;
			}
		}
		AddMsgToQueue(strAns, eve.fromGroup, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		intMsgCnt += 4;
		while (isspace(strLowerMessage[intMsgCnt]))
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
					AddMsgToQueue( "在本群中JRRP没有被禁用!", eve.fromGroup, false);					
				}
			}
			else
			{
				AddMsgToQueue( strPermissionDeniedErr, eve.fromGroup, false);				
			}
			return;
		}
		else if (Command == "off")
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
					AddMsgToQueue( "在本群中JRRP没有被启用!", eve.fromGroup, false);					
				}
			}
			else
			{
				AddMsgToQueue( strPermissionDeniedErr, eve.fromGroup, false);				
			}
			return;
		}
		if (DisabledJRRPGroup.count(eve.fromGroup))
		{
			AddMsgToQueue( "在本群中JRRP功能已被禁用", eve.fromGroup, false);			
			return;
		}
		char cstrDate[100] = {};
		time_t time_tTime = 0;
		time(&time_tTime);
		tm tmTime{};
		localtime_s(&tmTime, &time_tTime);
		strftime(cstrDate, 100, "%F", &tmTime);
		if (JRRP.count(eve.fromQQ) && JRRP[eve.fromQQ].Date == cstrDate)
		{
			const string strReply = strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal);
			AddMsgToQueue( strReply, eve.fromGroup, false);			
		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int> (GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int> (NormalDistribution(Generator));
			} while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			const string strReply(strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal));
			AddMsgToQueue( strReply, eve.fromGroup, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
		intMsgCnt += 2;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string name = eve.message.substr(intMsgCnt);
		if (name.length() > 50)
		{
			AddMsgToQueue(strNameTooLongErr, eve.fromGroup, false);
			return;
		}
		if (!name.empty())
		{
			Name->set(eve.fromGroup, eve.fromQQ, name);
			const string strReply = "已将" + strNickName + "的名称更改为" + strip(name);
			AddMsgToQueue( strReply, eve.fromGroup, false);			

		}
		else
		{
			if (Name->del(eve.fromGroup, eve.fromQQ))
			{
				const string strReply = "已将" + strNickName + "的名称删除";
				AddMsgToQueue( strReply, eve.fromGroup, false);				
			}
			else
			{
				const string strReply = strNickName + strNameDelErr;
				AddMsgToQueue( strReply, eve.fromGroup, false);				
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto &n : strSearch)
			n = toupper(n);
		string strReturn;
		if (RuleGetter->analyze(strSearch, strReturn))
		{
			AddMsgToQueue(strReturn, eve.fromGroup, false);			
		}
		else
		{
			AddMsgToQueue(strRuleErr + strReturn, eve.fromGroup, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
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
				AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);				
			}
			return;
		}
		else if (strAction == "off")
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
				AddMsgToQueue(strPermissionDeniedErr, eve.fromGroup, false);				
			}
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			AddMsgToQueue("在本群中.me命令已被禁用!", eve.fromGroup, false);			
			return;
		}
		if (strAction.empty())
		{
			AddMsgToQueue("动作不能为空!", eve.fromGroup, false);			
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			AddMsgToQueue(strMEDisabledErr, eve.fromGroup, false);			
			return;
		}
		const string strReply = strNickName + eve.message.substr(intMsgCnt);
		AddMsgToQueue(strReply, eve.fromGroup, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(charNumElement))
			{
				AddMsgToQueue(strSetInvalid, eve.fromGroup, false);				
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			AddMsgToQueue(strSetTooBig, eve.fromGroup, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromGroup, false);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromGroup, false);			
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromGroup, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromGroup, false);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromGroup, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromGroup, false);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromGroup, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strUnknownPropErr, eve.fromGroup, false);				
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(strPropErr, eve.fromGroup, false);			
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res) + "/" + to_string(intSkillVal) + " ";
		if (intD100Res <= 5)strReply += "大成功";
		else if (intD100Res <= intSkillVal / 5)strReply += "极难成功";
		else if (intD100Res <= intSkillVal / 2)strReply += "困难成功";
		else if (intD100Res <= intSkillVal)strReply += "成功";
		else if (intD100Res <= 95)strReply += "失败";
		else strReply += "大失败";
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strUnknownPropErr, eve.fromGroup, false);				
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(strPropErr, eve.fromGroup, false);			
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res) + "/" + to_string(intSkillVal) + " ";
		if (intD100Res == 1)strReply += "大成功";
		else if (intD100Res <= intSkillVal / 5)strReply += "极难成功";
		else if (intD100Res <= intSkillVal / 2)strReply += "困难成功";
		else if (intD100Res <= intSkillVal)strReply += "成功";
		else if (intD100Res <= 95 || (intSkillVal >= 50 && intD100Res != 100))strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromGroup, false);		
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'h' || strLowerMessage[intMsgCnt] == 'd')
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
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f' || strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (isspace(eve.message[intTmpMsgCnt]))
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
				AddMsgToQueue(strValueErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(strInputErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(strZeroDiceErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(strZeroTypeErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(strDiceTooBigErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(strTypeTooBigErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(strAddDiceValErr, eve.fromGroup, false);				
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(strUnknownErr, eve.fromGroup, false);				
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(strRollTimeExceeded, eve.fromGroup, false);				
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(strRollTimeErr, eve.fromGroup, false);				
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
			AddMsgToQueue(strValueErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromGroup, false);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromGroup, false);			
			return;
		}
		if (!boolDetail&&intTurnCnt != 1)
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
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
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
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
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
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			AddMsgToQueue("命令输入错误!", eve.fromGroup, false);			
		}
	}
}
EVE_DiscussMsg_EX(__eventDiscussMsg)
{
	if (eve.isSystem())return;
	init(eve.message);
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	if (eve.message.substr(0, 6) == "[CQ:at") {
		if (eve.message.substr(0, strAt.length()) == strAt) {
			eve.message = eve.message.substr(strAt.length() + 1);
		}
		else {
			return;
		}
	}
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (isspace(eve.message[intMsgCnt]))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = getName(eve.fromQQ, eve.fromDiscuss);
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string QQNum = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
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
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
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
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				AddMsgToQueue( Dice_Full_Ver, eve.fromDiscuss, false);				
			}
		}
		return;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string QQNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum == to_string(getLoginQQ()))
		{
			setDiscussLeave(eve.fromDiscuss);
		}
		return;
	}
	if (DisabledDiscuss.count(eve.fromDiscuss))
		return;
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		AddMsgToQueue( strHlpMsg, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			AddMsgToQueue(strStErr, eve.fromDiscuss, false);			
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss));
			}
			AddMsgToQueue(strPropCleared, eve.fromDiscuss, false);			
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			intMsgCnt += 3;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].erase(strSkillName);
				AddMsgToQueue(strPropDeleted, eve.fromDiscuss, false);				
			}
			else
			{
				AddMsgToQueue(strPropNotFound, eve.fromDiscuss, false);				
			}
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				AddMsgToQueue(format(strProp,{ strNickName,strSkillName,to_string(CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName]) }), eve.fromDiscuss, false);				
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				AddMsgToQueue(format(strProp,{ strNickName,strSkillName,to_string(SkillDefaultVal[strSkillName]) }), eve.fromDiscuss, false);				
			}
			else
			{
				AddMsgToQueue(strPropNotFound, eve.fromDiscuss, false);				
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(strLowerMessage[intMsgCnt]))
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
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError) {
			AddMsgToQueue(strPropErr, eve.fromDiscuss, false);			
		}
		else {

			AddMsgToQueue(strSetPropSuccess, eve.fromDiscuss, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		}
		else if (isdigit(strLowerMessage[intMsgCnt]))
			strinit += '+';
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
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
			AddMsgToQueue(strValueErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromDiscuss, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
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
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;

		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#'&& strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		string strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (isspace(eve.message[intTmpMsgCnt]))
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
				AddMsgToQueue(strValueErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(strInputErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(strZeroDiceErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(strZeroTypeErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(strDiceTooBigErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(strTypeTooBigErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(strAddDiceValErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(strUnknownErr, eve.fromDiscuss, false);				
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(strRollTimeExceeded, eve.fromDiscuss, false);				
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(strRollTimeErr, eve.fromDiscuss, false);				
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
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-') ? strMainDice.find('+') : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(i))
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
			AddMsgToQueue(strValueErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromDiscuss, false);			
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
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
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
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (DisabledOBDiscuss.count(eve.fromDiscuss))
			{
				DisabledOBDiscuss.erase(eve.fromDiscuss);
				AddMsgToQueue(  "成功在本多人聊天中启用旁观模式!", eve.fromDiscuss, false);				
			}
			else
			{
				AddMsgToQueue(  "在本多人聊天中旁观模式没有被禁用!", eve.fromDiscuss, false);				

			}
			return;
		}
		else if (Command == "off")
		{
			if (!DisabledOBDiscuss.count(eve.fromDiscuss))
			{
				DisabledOBDiscuss.insert(eve.fromDiscuss);
				ObserveDiscuss.clear();
				AddMsgToQueue(  "成功在本多人聊天中禁用旁观模式!", eve.fromDiscuss, false);				
			}
			else
			{
				AddMsgToQueue(  "在本多人聊天中旁观模式没有被启用!", eve.fromDiscuss, false);				
			}
			return;
		}
		if (DisabledOBDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue(  "在本多人聊天中旁观模式已被禁用!", eve.fromDiscuss, false);			
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
			AddMsgToQueue(  strReply, eve.fromDiscuss, false);			
		}
		else if (Command == "clr")
		{
			ObserveDiscuss.erase(eve.fromDiscuss);
			AddMsgToQueue(  "成功删除所有旁观者!", eve.fromDiscuss, false);			
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
					AddMsgToQueue(  strReply, eve.fromDiscuss, false);					
					eve.message_block();
					return;
				}
			}
			const string strReply = strNickName + "没有加入旁观模式!";
			AddMsgToQueue(  strReply, eve.fromDiscuss, false);			
		}
		else
		{
			const auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					const string strReply = strNickName + "已经处于旁观模式!";
					AddMsgToQueue(  strReply, eve.fromDiscuss, false);					
					eve.message_block();
					return;
				}
			}
			ObserveDiscuss.insert(make_pair(eve.fromDiscuss, eve.fromQQ));
			const string strReply = strNickName + "成功加入旁观模式!";
			AddMsgToQueue(  strReply, eve.fromDiscuss, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "的疯狂发作-临时症状:\n";
		TempInsane(strAns);
		AddMsgToQueue(  strAns, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "的疯狂发作-总结症状:\n";
		LongInsane(strAns);
		AddMsgToQueue(  strAns, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string San;
		while (isdigit(strLowerMessage[intMsgCnt])) {
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{

			AddMsgToQueue(strSCInvalid, eve.fromDiscuss, false);			

			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count("理智")))
		{

			AddMsgToQueue(strSanInvalid, eve.fromDiscuss, false);			

			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if(!isdigit(character) && character != 'D' && character!='d' && character != '+' && character != '-')
			{
				AddMsgToQueue(strSCInvalid, eve.fromQQ);				
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(strSCInvalid, eve.fromQQ);				
				return;
			}
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{

			AddMsgToQueue(strSCInvalid, eve.fromDiscuss, false);			

			return;
		}
		if (San.length() >= 3)
		{

			AddMsgToQueue(strSanInvalid, eve.fromDiscuss, false);			

			return;
		}
		const int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] : stoi(San);
		if (intSan == 0)
		{

			AddMsgToQueue(strSanInvalid, eve.fromDiscuss, false);			

			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " 成功\n你的San值减少" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSuc.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdSuc.intTotal);
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
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdFail.intTotal);
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
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdFail.intTotal);
			}
		}
		AddMsgToQueue(strAns, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt]))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(eve.message[intMsgCnt]))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strEnValInvalid, eve.fromDiscuss, false);				
				return;
			}

		}
		else
		{
			if (strCurrentValue.length() > 3)
			{

				AddMsgToQueue(strEnValInvalid, eve.fromDiscuss, false);				

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}
		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			const int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName] = intCurrentVal + intTmpRollD10;
			}
		}
		AddMsgToQueue(strAns, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		intMsgCnt += 4;
		while (isspace(strLowerMessage[intMsgCnt]))
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
		else if (Command == "off")
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
		char cstrDate[100] = {};
		time_t time_tTime = 0;
		time(&time_tTime);
		tm tmTime{};
		localtime_s(&tmTime, &time_tTime);
		strftime(cstrDate, 100, "%F", &tmTime);
		if (JRRP.count(eve.fromQQ) && JRRP[eve.fromQQ].Date == cstrDate)
		{
			const string strReply = strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal);
			AddMsgToQueue(strReply, eve.fromDiscuss, false);			
		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int> (GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int> (NormalDistribution(Generator));
			} while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			const string strReply(strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal));
			AddMsgToQueue(strReply, eve.fromDiscuss, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
		intMsgCnt += 2;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string name = eve.message.substr(intMsgCnt);
		if (name.length() > 50)
		{
			AddMsgToQueue(strNameTooLongErr, eve.fromDiscuss, false);
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
				const string strReply = strNickName + strNameDelErr;
				AddMsgToQueue(strReply, eve.fromDiscuss, false);				
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto &n : strSearch)
			n = toupper(n);
		string strReturn;
		if (RuleGetter->analyze(strSearch, strReturn))
		{
			AddMsgToQueue(strReturn, eve.fromDiscuss, false);			
		}
		else
		{
			AddMsgToQueue(strRuleErr + strReturn, eve.fromDiscuss, false);			
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
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
		else if (strAction == "off")
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
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue("在本多人聊天中.me命令已被禁用!", eve.fromDiscuss, false);			
			return;
		}
		if (strAction.empty())
		{
			AddMsgToQueue("动作不能为空!", eve.fromDiscuss, false);			
			return;
		}
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue(strMEDisabledErr, eve.fromDiscuss, false);			
			return;
		}
		const string strReply = strNickName + eve.message.substr(intMsgCnt);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(charNumElement))
			{
				AddMsgToQueue(strSetInvalid, eve.fromDiscuss, false);				
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			AddMsgToQueue(strSetTooBig, eve.fromDiscuss, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const	int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromDiscuss, false);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromDiscuss, false);			
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromDiscuss, false);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromDiscuss, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromQQ);			
			return;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			AddMsgToQueue(strCharacterTooBig, eve.fromDiscuss, false);			
			return;
		}
		if (intNum == 0)
		{
			AddMsgToQueue(strCharacterCannotBeZero, eve.fromDiscuss, false);			
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strUnknownPropErr, eve.fromDiscuss, false);				
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(strPropErr, eve.fromDiscuss, false);			
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res) + "/" + to_string(intSkillVal) + " ";
		if (intD100Res <= 5)strReply += "大成功";
		else if (intD100Res <= intSkillVal / 5)strReply += "极难成功";
		else if (intD100Res <= intSkillVal / 2)strReply += "困难成功";
		else if (intD100Res <= intSkillVal)strReply += "成功";
		else if (intD100Res <= 95)strReply += "失败";
		else strReply += "大失败";
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		string strReason = eve.message.substr(intMsgCnt);
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				AddMsgToQueue(strUnknownPropErr, eve.fromDiscuss, false);				
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			AddMsgToQueue(strPropErr, eve.fromDiscuss, false);			
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res) + "/" + to_string(intSkillVal) + " ";
		if (intD100Res == 1)strReply += "大成功";
		else if (intD100Res <= intSkillVal / 5)strReply += "极难成功";
		else if (intD100Res <= intSkillVal / 2)strReply += "困难成功";
		else if (intD100Res <= intSkillVal)strReply += "成功";
		else if (intD100Res <= 95 || (intSkillVal >= 50 && intD100Res != 100))strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromDiscuss, false);		
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'h' || strLowerMessage[intMsgCnt] == 'd')
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
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f' || strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (isspace(eve.message[intTmpMsgCnt]))
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
				AddMsgToQueue(strValueErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				AddMsgToQueue(strInputErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				AddMsgToQueue(strZeroDiceErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				AddMsgToQueue(strZeroTypeErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				AddMsgToQueue(strDiceTooBigErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				AddMsgToQueue(strTypeTooBigErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				AddMsgToQueue(strAddDiceValErr, eve.fromDiscuss, false);				
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				AddMsgToQueue(strUnknownErr, eve.fromDiscuss, false);				
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				AddMsgToQueue(strRollTimeExceeded, eve.fromDiscuss, false);				
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				AddMsgToQueue(strRollTimeErr, eve.fromDiscuss, false);				
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
			AddMsgToQueue(strValueErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			AddMsgToQueue(strInputErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			AddMsgToQueue(strZeroDiceErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			AddMsgToQueue(strZeroTypeErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			AddMsgToQueue(strDiceTooBigErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			AddMsgToQueue(strTypeTooBigErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			AddMsgToQueue(strAddDiceValErr, eve.fromDiscuss, false);			
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			AddMsgToQueue(strUnknownErr, eve.fromDiscuss, false);			
			return;
		}
		if (!boolDetail&&intTurnCnt != 1)
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
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
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
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
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
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			AddMsgToQueue("命令输入错误!", eve.fromDiscuss, false);			
		}
	}
}

EVE_System_GroupMemberIncrease(__eventGroupMemberIncrease)
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
			strReply.replace(strReply.find("{sex}"), 5, getStrangerInfo(beingOperateQQ).sex == 0 ? "男" : getStrangerInfo(beingOperateQQ).sex == 1 ? "女" : "未知");
		}
		while (strReply.find("{qq}") != string::npos)
		{
			strReply.replace(strReply.find("{qq}"), 4, to_string(beingOperateQQ));
		}
		AddMsgToQueue(strReply, fromGroup, false);		
	}
	return 0;
}
EVE_Disable(__eventDisable)
{
	Enabled = false;
	ilInitList.reset();
	RuleGetter.reset();
	Name.reset();
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
	ofstream ofstreamJRRP(strFileLoc + "JRRP.RDconf", ios::out | ios::trunc);
	for (auto it = JRRP.begin(); it != JRRP.end(); ++it)
	{
		ofstreamJRRP << it->first << " " << it->second.Date << " " << it->second.RPVal << std::endl;
	}
	ofstreamJRRP.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it) {
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1) {
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " " << it1->first << " " << it1->second << std::endl;
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
	JRRP.clear();
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
EVE_Exit(__eventExit)
{
	if (!Enabled)
		return 0;
	ilInitList.reset();
	RuleGetter.reset();
	Name.reset();
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
	ofstream ofstreamJRRP(strFileLoc + "JRRP.RDconf", ios::out | ios::trunc);
	for (auto it = JRRP.begin(); it != JRRP.end(); ++it)
	{
		ofstreamJRRP << it->first << " " << it->second.Date << " " << it->second.RPVal << std::endl;
	}
	ofstreamJRRP.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it) {
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1) {
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " " << it1->first << " " << it1->second << std::endl;
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