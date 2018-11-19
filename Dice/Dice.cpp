/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018 w4123���
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
#include "GetRule.h"
#include "DiceMsgSend.h"
#include "CustomMsg.h"
#include"dbManager.h"
#include"nameGetter.h"
#include"l5rDiceManager.h"
/*
TODO:
1. en�ɱ�ɳ��춨
2. st�����￨
3. st���￨��
4. st����չʾ��ȫ����չʾ�Լ�����
5. ����rules�������ݿ�
6. jrrp�Ż�
7. help�Ż�
8. ȫ���ǳ�
*/

using namespace std;
using namespace CQ;

//unique_ptr<NameStorage> Name;
unique_ptr<GetRule> RuleGetter;
unique_ptr<dbManager> dbCtrl=nullptr;
unique_ptr<nameGetter> nameCtrl = nullptr;
unique_ptr<l5rDiceManager> l5rDiceCtrl = nullptr;
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

struct SourceType
{
	SourceType(long long a, int b, long long c) : QQ(a), Type(b), GrouporDiscussID(c)
	{
	};
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
	* ���ƴ洢-�������ȡ
	*/
	//Name = make_unique<NameStorage>(strFileLoc + "Name.dicedb");
	dbCtrl = make_unique<dbManager>(strFileLoc);
	nameCtrl = make_unique<nameGetter>();
	l5rDiceCtrl = make_unique<l5rDiceManager>();
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
	ifstream ifstreamCustomMsg(strFileLoc + "CustomMsg.json");
	if (ifstreamCustomMsg)
	{
		ReadCustomMsg(ifstreamCustomMsg);
	}
	ifstreamCustomMsg.close();
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
	const string strNickName = nameCtrl->getNickName(eve.fromQQ);
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		AddMsgToQueue(GlobalMsg["strHlpMsg"], eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "lfr")
	{
		intMsgCnt += 3;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string reply= l5rDiceCtrl->dice(strNickName, strLowerMessage.substr(intMsgCnt), eve.fromQQ);
		AddMsgToQueue(reply, eve.fromQQ);
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
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
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
				const string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
				AddMsgToQueue(strTurnNotice, eve.fromQQ);
			}
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
			                                            ? strMainDice.find('+')
			                                            : strMainDice.find('-'));
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
			string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
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
				strAns += ",��ֵ: ";
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
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "������: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "����" + strReason + " ");
				AddMsgToQueue(strAns, eve.fromQQ);
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "�ķ����-��ʱ֢״:\n";
		TempInsane(strAns);

		AddMsgToQueue(strAns, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "�ķ����-�ܽ�֢״:\n";
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
		while (isdigit(strLowerMessage[intMsgCnt]))
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
			eve.fromQQ, PrivateT, 0)].count("����")))
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromQQ);

			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ);
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
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
		const int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["����"] : stoi(San);
		if (intSan == 0)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromQQ);

			return;
		}
		string strAns = strNickName + "��Sancheck:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " �ɹ�\n���Sanֵ����" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdSuc.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["����"] = max(0, intSan - rdSuc.intTotal);
			}
		}
		else if (intTmpRollRes == 100 || (intSan < 50 && intTmpRollRes > 95))
		{
			strAns += " ��ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdFail.Max();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "���ֵ=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["����"] = max(0, intSan - rdFail.intTotal);
			}
		}
		else
		{
			strAns += " ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["����"] = max(0, intSan - rdFail.intTotal);
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
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt])
		)
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

		string strAns = strNickName + "��" + strSkillName + "��ǿ��ɳ��춨:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " ʧ��!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "û�б仯!";
		}
		else
		{
			strAns += " �ɹ�!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "����1D10=";
			const int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "��,��ǰΪ" + to_string(intCurrentVal + intTmpRollD10) + "��";
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
			const string strReply = strNickName + "�������Ʒֵ��:" + to_string(JRRP[eve.fromQQ].RPVal);

			AddMsgToQueue(strReply, eve.fromQQ);
		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int>(GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int>(NormalDistribution(Generator));
			}
			while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			const string strReply(strNickName + "�������Ʒֵ��:" + to_string(JRRP[eve.fromQQ].RPVal));

			AddMsgToQueue(strReply, eve.fromQQ);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto& n : strSearch)
			n = toupper(n);
		string strReturn;
		if (RuleGetter->analyze(strSearch, strReturn))
		{
			AddMsgToQueue(strReturn, eve.fromQQ);
		}
		else
		{
			AddMsgToQueue(GlobalMsg["strRuleErr"] + strReturn, eve.fromQQ);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
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
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[
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
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[
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
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
				isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
				!= ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
			] == ':')intMsgCnt++;
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
				AddMsgToQueue(GlobalMsg["strGroupIDInvalid"], eve.fromQQ);
				return;
			}
		}
		if (strGroupID.empty())
		{
			AddMsgToQueue("Ⱥ�Ų���Ϊ��!", eve.fromQQ);
			return;
		}
		if (strAction.empty())
		{
			AddMsgToQueue("��������Ϊ��!", eve.fromQQ);
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
		string strReply = nameCtrl->getNickName(eve.fromQQ, llGroupID) + eve.message.substr(intMsgCnt);
		const int intSendRes = sendGroupMsg(llGroupID, strReply);
		if (intSendRes < 0)
		{
			AddMsgToQueue(GlobalMsg["strSendErr"], eve.fromQQ);
		}
		else
		{
			AddMsgToQueue("����ִ�гɹ�!", eve.fromQQ);
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
				AddMsgToQueue(GlobalMsg["strSetInvalid"], eve.fromQQ);
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			AddMsgToQueue(GlobalMsg["strSetTooBig"], eve.fromQQ);
			return;
		}
		const int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		const string strSetSuccessReply = "�ѽ�" + strNickName + "��Ĭ�������͸���ΪD" + strDefaultDice;
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
			isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
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
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "����" + strSkillName + "�춨: D100=" + to_string(intD100Res) + "/" +
			to_string(intSkillVal) + " ";
		if (intD100Res <= 5)strReply += "��ɹ�";
		else if (intD100Res <= intSkillVal / 5)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal / 2)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal)strReply += "�ɹ�";
		else if (intD100Res <= 95)strReply += "ʧ��";
		else strReply += "��ʧ��";
		if (!strReason.empty())
		{
			strReply = "����" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromQQ);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
			isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
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
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "����" + strSkillName + "�춨: D100=" + to_string(intD100Res) + "/" +
			to_string(intSkillVal) + " ";
		if (intD100Res == 1)strReply += "��ɹ�";
		else if (intD100Res <= intSkillVal / 5)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal / 2)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal)strReply += "�ɹ�";
		else if (intD100Res <= 95 || (intSkillVal >= 50 && intD100Res != 100))strReply += "ʧ��";
		else strReply += "��ʧ��";
		if (!strReason.empty())
		{
			strReply = "����" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromQQ);
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
		while (isspace(eve.message[intMsgCnt]))
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
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
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
				const string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";

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
			string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
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
				strAns += ",��ֵ: ";
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
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "������: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "����" + strReason + " ");
				AddMsgToQueue(strAns, eve.fromQQ);
			}
		}
	}
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			AddMsgToQueue("�����������!", eve.fromQQ);
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
	if (eve.message.substr(0, 6) == "[CQ:at")
	{
		if (eve.message.substr(0, strAt.length()) == strAt)
		{
			eve.message = eve.message.substr(strAt.length());
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
	while (isspace(eve.message[intMsgCnt]))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = nameCtrl->getNickName(eve.fromQQ, eve.fromGroup);
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(
			strLowerMessage[intMsgCnt]))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
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
						AddMsgToQueue("�ɹ�������������!", eve.fromGroup, false);
					}
					else
					{
						AddMsgToQueue("���������Ѿ����ڿ���״̬!", eve.fromGroup, false);
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
						AddMsgToQueue("�ɹ��رձ�������!", eve.fromGroup, false);
					}
					else
					{
						AddMsgToQueue("���������Ѿ����ڹر�״̬!", eve.fromGroup, false);
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
				AddMsgToQueue(Dice_Full_Ver, eve.fromGroup, false);
			}
		}
		return;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ������.help����!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("�ڱ�Ⱥ��.help����û�б�����!", eve.fromGroup, false);
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ�н���.help����!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("�ڱ�Ⱥ��.help����û�б�����!", eve.fromGroup, false);
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
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
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
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[
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
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[
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
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
				isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
				!= ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
			] == ':')intMsgCnt++;
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
			strname = nameCtrl->strip(strname);
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
		const string strReply = strname + "���ȹ����㣺" + strinit + '=' + to_string(initdice.intTotal);
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
				strReply = "�ɹ�����ȹ���¼��";
			else
				strReply = "�б�Ϊ�գ�";
			AddMsgToQueue(strReply, eve.fromGroup, false);
			return;
		}
		ilInitList->show(eve.fromGroup, strReply);
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "lfr")
	{
		intMsgCnt += 3;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string reply= l5rDiceCtrl->dice(strNickName, strLowerMessage.substr(intMsgCnt), eve.fromQQ, eve.fromGroup);
		AddMsgToQueue(reply, eve.fromGroup,false);
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
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
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
				string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromGroup, false);
				}
				else
				{
					strTurnNotice = "��Ⱥ\"" + getGroupList()[eve.fromGroup] + "\"�� " + strTurnNotice;
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
			string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
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
				strAns += ",��ֵ: ";
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
				strAns = "��Ⱥ\"" + getGroupList()[eve.fromGroup] + "\"�� " + strAns;
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
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "������: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "����" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromGroup, false);
				}
				else
				{
					strAns = "��Ⱥ\"" + getGroupList()[eve.fromGroup] + "\"�� " + strAns;
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
			const string strReply = strNickName + "������һ�ΰ���";
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ�������Թ�ģʽ!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("��Ⱥ���Թ�ģʽû�б�����!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue("��û��Ȩ��ִ�д�����!", eve.fromGroup, false);
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ�н����Թ�ģʽ!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("��Ⱥ���Թ�ģʽû�б�����!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue("��û��Ȩ��ִ�д�����!", eve.fromGroup, false);
			}
			return;
		}
		if (DisabledOBGroup.count(eve.fromGroup))
		{
			AddMsgToQueue("�ڱ�Ⱥ���Թ�ģʽ�ѱ�����!", eve.fromGroup, false);
			return;
		}
		if (Command == "list")
		{
			string Msg = "��ǰ���Թ�����:";
			const auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				Msg += "\n" + nameCtrl->getNickName(it->second, eve.fromGroup) + "(" + to_string(it->second) + ")";
			}
			const string strReply = Msg == "��ǰ���Թ�����:" ? "��ǰ�����Թ���" : Msg;
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
		else if (Command == "clr")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				ObserveGroup.erase(eve.fromGroup);
				AddMsgToQueue("�ɹ�ɾ�������Թ���!", eve.fromGroup, false);
			}
			else
			{
				AddMsgToQueue("��û��Ȩ��ִ�д�����!", eve.fromGroup, false);
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
					const string strReply = strNickName + "�ɹ��˳��Թ�ģʽ!";
					AddMsgToQueue(strReply, eve.fromGroup, false);
					eve.message_block();
					return;
				}
			}
			const string strReply = strNickName + "û�м����Թ�ģʽ!";
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
		else
		{
			const auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					const string strReply = strNickName + "�Ѿ������Թ�ģʽ!";
					AddMsgToQueue(strReply, eve.fromGroup, false);
					eve.message_block();
					return;
				}
			}
			ObserveGroup.insert(make_pair(eve.fromGroup, eve.fromQQ));
			const string strReply = strNickName + "�ɹ������Թ�ģʽ!";
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "�ķ����-��ʱ֢״:\n";
		TempInsane(strAns);
		AddMsgToQueue(strAns, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "�ķ����-�ܽ�֢״:\n";
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
		while (isdigit(strLowerMessage[intMsgCnt]))
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
			SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count("����")))
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromGroup, false);
			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ, false);
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
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
		const int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["����"] : stoi(San);
		if (intSan == 0)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromGroup, false);
			return;
		}
		string strAns = strNickName + "��Sancheck:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " �ɹ�\n���Sanֵ����" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdSuc.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["����"] = max(0, intSan - rdSuc.intTotal);
			}
		}
		else if (intTmpRollRes == 100 || (intSan < 50 && intTmpRollRes > 95))
		{
			strAns += " ��ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdFail.Max();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "���ֵ=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["����"] = max(0, intSan - rdFail.intTotal);
			}
		}
		else
		{
			strAns += " ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["����"] = max(0, intSan - rdFail.intTotal);
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
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt])
		)
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

		string strAns = strNickName + "��" + strSkillName + "��ǿ��ɳ��춨:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " ʧ��!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "û�б仯!";
		}
		else
		{
			strAns += " �ɹ�!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "����1D10=";
			const int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "��,��ǰΪ" + to_string(intCurrentVal + intTmpRollD10) + "��";
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ������JRRP!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("�ڱ�Ⱥ��JRRPû�б�����!", eve.fromGroup, false);
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ�н���JRRP!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("�ڱ�Ⱥ��JRRPû�б�����!", eve.fromGroup, false);
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
			AddMsgToQueue("�ڱ�Ⱥ��JRRP�����ѱ�����", eve.fromGroup, false);
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
			const string strReply = strNickName + "�������Ʒֵ��:" + to_string(JRRP[eve.fromQQ].RPVal);
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int>(GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int>(NormalDistribution(Generator));
			}
			while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			const string strReply(strNickName + "�������Ʒֵ��:" + to_string(JRRP[eve.fromQQ].RPVal));
			AddMsgToQueue(strReply, eve.fromGroup, false);
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
			AddMsgToQueue(GlobalMsg["strNameTooLongErr"], eve.fromGroup, false);
			return;
		}
		if (!name.empty())
		{
			nameCtrl->setNickName(name,eve.fromQQ, eve.fromGroup);
			const string strReply = "�ѽ�" + strNickName + "�����Ƹ���Ϊ" + nameCtrl->strip(name);
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
		else
		{
			if (nameCtrl->setNickName(name, eve.fromQQ, eve.fromGroup))
			{
				const string strReply = "�ѽ�" + strNickName + "������ɾ��";
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
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto& n : strSearch)
			n = toupper(n);
		string strReturn;
		if (RuleGetter->analyze(strSearch, strReturn))
		{
			AddMsgToQueue(strReturn, eve.fromGroup, false);
		}
		else
		{
			AddMsgToQueue(GlobalMsg["strRuleErr"] + strReturn, eve.fromGroup, false);
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ������.me����!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("�ڱ�Ⱥ��.me����û�б�����!", eve.fromGroup, false);
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
					AddMsgToQueue("�ɹ��ڱ�Ⱥ�н���.me����!", eve.fromGroup, false);
				}
				else
				{
					AddMsgToQueue("�ڱ�Ⱥ��.me����û�б�����!", eve.fromGroup, false);
				}
			}
			else
			{
				AddMsgToQueue(GlobalMsg["strPermissionDeniedErr"], eve.fromGroup, false);
			}
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			AddMsgToQueue("�ڱ�Ⱥ��.me�����ѱ�����!", eve.fromGroup, false);
			return;
		}
		if (strAction.empty())
		{
			AddMsgToQueue("��������Ϊ��!", eve.fromGroup, false);
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			AddMsgToQueue(GlobalMsg["strMEDisabledErr"], eve.fromGroup, false);
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
				AddMsgToQueue(GlobalMsg["strSetInvalid"], eve.fromGroup, false);
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			AddMsgToQueue(GlobalMsg["strSetTooBig"], eve.fromGroup, false);
			return;
		}
		const int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		const string strSetSuccessReply = "�ѽ�" + strNickName + "��Ĭ�������͸���ΪD" + strDefaultDice;
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
			isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
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
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "����" + strSkillName + "�춨: D100=" + to_string(intD100Res) + "/" +
			to_string(intSkillVal) + " ";
		if (intD100Res <= 5)strReply += "��ɹ�";
		else if (intD100Res <= intSkillVal / 5)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal / 2)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal)strReply += "�ɹ�";
		else if (intD100Res <= 95)strReply += "ʧ��";
		else strReply += "��ʧ��";
		if (!strReason.empty())
		{
			strReply = "����" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromGroup, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
			isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
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
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "����" + strSkillName + "�춨: D100=" + to_string(intD100Res) + "/" +
			to_string(intSkillVal) + " ";
		if (intD100Res == 1)strReply += "��ɹ�";
		else if (intD100Res <= intSkillVal / 5)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal / 2)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal)strReply += "�ɹ�";
		else if (intD100Res <= 95 || (intSkillVal >= 50 && intD100Res != 100))strReply += "ʧ��";
		else strReply += "��ʧ��";
		if (!strReason.empty())
		{
			strReply = "����" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromGroup, false);
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
		while (isspace(eve.message[intMsgCnt]))
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
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
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
				string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromGroup, false);
				}
				else
				{
					strTurnNotice = "��Ⱥ\"" + getGroupList()[eve.fromGroup] + "\"�� " + strTurnNotice;
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
			string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
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
				strAns += ",��ֵ: ";
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
				strAns = "��Ⱥ\"" + getGroupList()[eve.fromGroup] + "\"�� " + strAns;
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
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "������: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "����" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromGroup, false);
				}
				else
				{
					strAns = "��Ⱥ\"" + getGroupList()[eve.fromGroup] + "\"�� " + strAns;
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
			const string strReply = strNickName + "������һ�ΰ���";
			AddMsgToQueue(strReply, eve.fromGroup, false);
		}
	}
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			AddMsgToQueue("�����������!", eve.fromGroup, false);
		}
	}
}

EVE_DiscussMsg_EX(__eventDiscussMsg)
{
	if (eve.isSystem())return;
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
	while (isspace(eve.message[intMsgCnt]))
		intMsgCnt++;
	eve.message_block();
	const string strNickName = nameCtrl->getNickName(eve.fromQQ, eve.fromDiscuss);
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(
			strLowerMessage[intMsgCnt]))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(strLowerMessage[intMsgCnt]))
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
					AddMsgToQueue("�ɹ�������������!", eve.fromDiscuss, false);
				}
				else
				{
					AddMsgToQueue("���������Ѿ����ڿ���״̬!", eve.fromDiscuss, false);
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
					AddMsgToQueue("�ɹ��رձ�������!", eve.fromDiscuss, false);
				}
				else
				{
					AddMsgToQueue("���������Ѿ����ڹر�״̬!", eve.fromDiscuss, false);
				}
			}
		}
		else
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(
				getLoginQQ() % 10000)))
			{
				AddMsgToQueue(Dice_Full_Ver, eve.fromDiscuss, false);
			}
		}
		return;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
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
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum ==
			to_string(getLoginQQ()))
		{
			setDiscussLeave(eve.fromDiscuss);
		}
		return;
	}
	if (DisabledDiscuss.count(eve.fromDiscuss))
		return;
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		AddMsgToQueue(GlobalMsg["strHlpMsg"], eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
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
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[
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
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[
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
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
				isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
				!= ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
			] == ':')intMsgCnt++;
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
			strname = nameCtrl->strip(strname);
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
		const string strReply = strname + "���ȹ����㣺" + strinit + '=' + to_string(initdice.intTotal);
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
				strReply = "�ɹ�����ȹ���¼��";
			else
				strReply = "�б�Ϊ�գ�";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
			return;
		}
		ilInitList->show(eve.fromDiscuss, strReply);
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "lfr")
	{
		intMsgCnt += 3;
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string reply=l5rDiceCtrl->dice(strNickName, strLowerMessage.substr(intMsgCnt), eve.fromQQ, eve.fromDiscuss);
		AddMsgToQueue(reply, eve.fromDiscuss,false);
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
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' ';
		     intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
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
				string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromDiscuss, false);
				}
				else
				{
					strTurnNotice = "�ڶ��������� " + strTurnNotice;
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
			string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
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
				strAns += ",��ֵ: ";
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
				strAns = "�ڶ��������� " + strAns;
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
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "������: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "����" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromDiscuss, false);
				}
				else
				{
					strAns = "�ڶ��������� " + strAns;
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
			const string strReply = strNickName + "������һ�ΰ���";
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
				AddMsgToQueue("�ɹ��ڱ����������������Թ�ģʽ!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("�ڱ������������Թ�ģʽû�б�����!", eve.fromDiscuss, false);
			}
			return;
		}
		if (Command == "off")
		{
			if (!DisabledOBDiscuss.count(eve.fromDiscuss))
			{
				DisabledOBDiscuss.insert(eve.fromDiscuss);
				ObserveDiscuss.clear();
				AddMsgToQueue("�ɹ��ڱ����������н����Թ�ģʽ!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("�ڱ������������Թ�ģʽû�б�����!", eve.fromDiscuss, false);
			}
			return;
		}
		if (DisabledOBDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue("�ڱ������������Թ�ģʽ�ѱ�����!", eve.fromDiscuss, false);
			return;
		}
		if (Command == "list")
		{
			string Msg = "��ǰ���Թ�����:";
			const auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				Msg += "\n" + nameCtrl->getNickName(it->second, eve.fromDiscuss) + "(" + to_string(it->second) + ")";
			}
			const string strReply = Msg == "��ǰ���Թ�����:" ? "��ǰ�����Թ���" : Msg;
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
		else if (Command == "clr")
		{
			ObserveDiscuss.erase(eve.fromDiscuss);
			AddMsgToQueue("�ɹ�ɾ�������Թ���!", eve.fromDiscuss, false);
		}
		else if (Command == "exit")
		{
			const auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					ObserveDiscuss.erase(it);
					const string strReply = strNickName + "�ɹ��˳��Թ�ģʽ!";
					AddMsgToQueue(strReply, eve.fromDiscuss, false);
					eve.message_block();
					return;
				}
			}
			const string strReply = strNickName + "û�м����Թ�ģʽ!";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
		else
		{
			const auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					const string strReply = strNickName + "�Ѿ������Թ�ģʽ!";
					AddMsgToQueue(strReply, eve.fromDiscuss, false);
					eve.message_block();
					return;
				}
			}
			ObserveDiscuss.insert(make_pair(eve.fromDiscuss, eve.fromQQ));
			const string strReply = strNickName + "�ɹ������Թ�ģʽ!";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "�ķ����-��ʱ֢״:\n";
		TempInsane(strAns);
		AddMsgToQueue(strAns, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "�ķ����-�ܽ�֢״:\n";
		LongInsane(strAns);
		AddMsgToQueue(strAns, eve.fromDiscuss, false);
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
		while (isdigit(strLowerMessage[intMsgCnt]))
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
			SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count("����")))
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromDiscuss, false);

			return;
		}
		for (const auto& character : SanCost.substr(0, SanCost.find("/")))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
			{
				AddMsgToQueue(GlobalMsg["strSCInvalid"], eve.fromQQ, false);
				return;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find("/") + 1))
		{
			if (!isdigit(character) && character != 'D' && character != 'd' && character != '+' && character != '-')
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
			                   ? CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["����"]
			                   : stoi(San);
		if (intSan == 0)
		{
			AddMsgToQueue(GlobalMsg["strSanInvalid"], eve.fromDiscuss, false);

			return;
		}
		string strAns = strNickName + "��Sancheck:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " �ɹ�\n���Sanֵ����" + SanCost.substr(0, SanCost.find("/"));
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSuc.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdSuc.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["����"] = max(0, intSan - rdSuc.intTotal
				);
			}
		}
		else if (intTmpRollRes == 100 || (intSan < 50 && intTmpRollRes > 95))

		{
			strAns += " ��ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			// ReSharper disable once CppExpressionWithoutSideEffects
			rdFail.Max();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "���ֵ=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["����"] = max(0, intSan - rdFail.intTotal
				);
			}
		}
		else
		{
			strAns += " ʧ��\n���Sanֵ����" + SanCost.substr(SanCost.find("/") + 1);
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdFail.intTotal);
			strAns += +"��,��ǰʣ��" + to_string(max(0, intSan - rdFail.intTotal)) + "��";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["����"] = max(0, intSan - rdFail.intTotal
				);
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
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt])
		)
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
		string strAns = strNickName + "��" + strSkillName + "��ǿ��ɳ��춨:\n1D100=";
		const int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " ʧ��!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "û�б仯!";
		}
		else
		{
			strAns += " �ɹ�!\n���" + (strSkillName.empty() ? "���Ի���ֵ" : strSkillName) + "����1D10=";
			const int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "��,��ǰΪ" + to_string(intCurrentVal + intTmpRollD10) + "��";
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
		intMsgCnt += 4;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (DisabledJRRPDiscuss.count(eve.fromDiscuss))
			{
				DisabledJRRPDiscuss.erase(eve.fromDiscuss);
				AddMsgToQueue("�ɹ��ڴ˶�������������JRRP!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("�ڴ˶���������JRRPû�б�����!", eve.fromDiscuss, false);
			}
			return;
		}
		if (Command == "off")
		{
			if (!DisabledJRRPDiscuss.count(eve.fromDiscuss))
			{
				DisabledJRRPDiscuss.insert(eve.fromDiscuss);
				AddMsgToQueue("�ɹ��ڴ˶��������н���JRRP!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("�ڴ˶���������JRRPû�б�����!", eve.fromDiscuss, false);
			}
			return;
		}
		if (DisabledJRRPDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue("�ڴ˶���������JRRP�ѱ�����!", eve.fromDiscuss, false);
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
			const string strReply = strNickName + "�������Ʒֵ��:" + to_string(JRRP[eve.fromQQ].RPVal);
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int>(GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int>(NormalDistribution(Generator));
			}
			while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			const string strReply(strNickName + "�������Ʒֵ��:" + to_string(JRRP[eve.fromQQ].RPVal));
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
			AddMsgToQueue(GlobalMsg["strNameTooLongErr"], eve.fromDiscuss, false);
			return;
		}
		if (!name.empty())
		{
			nameCtrl->setNickName(name, eve.fromQQ, eve.fromDiscuss);
			const string strReply = "�ѽ�" + strNickName + "�����Ƹ���Ϊ" + nameCtrl->strip(name);
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
		else
		{
			if (nameCtrl->setNickName(name, eve.fromQQ, eve.fromDiscuss))
			{
				const string strReply = "�ѽ�" + strNickName + "������ɾ��";
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
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto& n : strSearch)
			n = toupper(n);
		string strReturn;
		if (RuleGetter->analyze(strSearch, strReturn))
		{
			AddMsgToQueue(strReturn, eve.fromDiscuss, false);
		}
		else
		{
			AddMsgToQueue(GlobalMsg["strRuleErr"] + strReturn, eve.fromDiscuss, false);
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
				AddMsgToQueue("�ɹ��ڱ���������������.me����!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("�ڱ�����������.me����û�б�����!", eve.fromDiscuss, false);
			}
			return;
		}
		if (strAction == "off")
		{
			if (!DisabledMEDiscuss.count(eve.fromDiscuss))
			{
				DisabledMEDiscuss.insert(eve.fromDiscuss);
				AddMsgToQueue("�ɹ��ڱ����������н���.me����!", eve.fromDiscuss, false);
			}
			else
			{
				AddMsgToQueue("�ڱ�����������.me����û�б�����!", eve.fromDiscuss, false);
			}
			return;
		}
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue("�ڱ�����������.me�����ѱ�����!", eve.fromDiscuss, false);
			return;
		}
		if (strAction.empty())
		{
			AddMsgToQueue("��������Ϊ��!", eve.fromDiscuss, false);
			return;
		}
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			AddMsgToQueue(GlobalMsg["strMEDisabledErr"], eve.fromDiscuss, false);
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
				AddMsgToQueue(GlobalMsg["strSetInvalid"], eve.fromDiscuss, false);
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			AddMsgToQueue(GlobalMsg["strSetTooBig"], eve.fromDiscuss, false);
			return;
		}
		const int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		const string strSetSuccessReply = "�ѽ�" + strNickName + "��Ĭ�������͸���ΪD" + strDefaultDice;
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
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
			isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
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
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "����" + strSkillName + "�춨: D100=" + to_string(intD100Res) + "/" +
			to_string(intSkillVal) + " ";
		if (intD100Res <= 5)strReply += "��ɹ�";
		else if (intD100Res <= intSkillVal / 5)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal / 2)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal)strReply += "�ɹ�";
		else if (intD100Res <= 95)strReply += "ʧ��";
		else strReply += "��ʧ��";
		if (!strReason.empty())
		{
			strReply = "����" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !
			isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
			':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] ==
			':')intMsgCnt++;
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
		const int intD100Res = Randint(1, 100);
		string strReply = strNickName + "����" + strSkillName + "�춨: D100=" + to_string(intD100Res) + "/" +
			to_string(intSkillVal) + " ";
		if (intD100Res == 1)strReply += "��ɹ�";
		else if (intD100Res <= intSkillVal / 5)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal / 2)strReply += "���ѳɹ�";
		else if (intD100Res <= intSkillVal)strReply += "�ɹ�";
		else if (intD100Res <= 95 || (intSkillVal >= 50 && intD100Res != 100))strReply += "ʧ��";
		else strReply += "��ʧ��";
		if (!strReason.empty())
		{
			strReply = "����" + strReason + " " + strReply;
		}
		AddMsgToQueue(strReply, eve.fromDiscuss, false);
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
		while (isspace(eve.message[intMsgCnt]))
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
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' && strLowerMessage[
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
				string strTurnNotice = strNickName + "����������: " + rdTurnCnt.FormShortString() + "��";
				if (!isHidden)
				{
					AddMsgToQueue(strTurnNotice, eve.fromDiscuss, false);
				}
				else
				{
					strTurnNotice = "�ڶ��������� " + strTurnNotice;
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
			string strAns = strNickName + "������: " + to_string(intTurnCnt) + "��" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "����" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
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
				strAns += ",��ֵ: ";
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
				strAns = "�ڶ��������� " + strAns;
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
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				string strAns = strNickName + "������: " + (boolDetail
					                                         ? rdMainDice.FormCompleteString()
					                                         : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "����" + strReason + " ");
				if (!isHidden)
				{
					AddMsgToQueue(strAns, eve.fromDiscuss, false);
				}
				else
				{
					strAns = "�ڶ��������� " + strAns;
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
			const string strReply = strNickName + "������һ�ΰ���";
			AddMsgToQueue(strReply, eve.fromDiscuss, false);
		}
	}
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			AddMsgToQueue("�����������!", eve.fromDiscuss, false);
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
			strReply.replace(strReply.find("{sex}"), 5,
			                 getStrangerInfo(beingOperateQQ).sex == 0
				                 ? "��"
				                 : getStrangerInfo(beingOperateQQ).sex == 1
				                 ? "Ů"
				                 : "δ֪");
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
//	Name.reset();
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
//	Name.reset();
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
