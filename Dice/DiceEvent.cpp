#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include "DiceEvent.h"
#include "Jsonio.h"
#include "MsgFormat.h"
#include "DiceMod.h"
#include "ManagerSystem.h"
#include "BlackListManager.h"
#include "CharacterCard.h"
#include "DiceSession.h"
#include "GetRule.h"
#include "CQAPI.h"
#include "DiceNetwork.h"
#include "DiceCloud.h"

//#pragma warning(disable:28159)
using namespace std;
using namespace CQ;

void FromMsg::FwdMsg(const string& message)
{
	if (mFwdList.count(fromChat) && !isLinkOrder)
	{
		const auto range = mFwdList.equal_range(fromChat);
		string strFwd;
		if (trusted < 5)strFwd += printFrom();
		strFwd += message;
		for (auto it = range.first; it != range.second; ++it)
		{
			AddMsgToQueue(strFwd, it->second.first, it->second.second);
		}
	}
}

int FromMsg::AdminEvent(const string& strOption)
{
	if (strOption == "isban")
	{
		strVar["target"] = readDigit();
		blacklist->isban(this);
		return 1;
	}
	if (strOption == "state")
	{
		ResList res;
		res << "Servant:" + printQQ(console.DiceMaid)
			<< "Master:" + printQQ(console.master())
			<< console.listClock().dot("\t").show()
			<< (console["Private"] ? "˽��ģʽ" : "����ģʽ");
		if (console["LeaveDiscuss"])res << "����������";
		if (console["DisabledGlobal"])res << "ȫ�־�Ĭ��";
		if (console["DisabledMe"])res << "ȫ�ֽ���.me";
		if (console["DisabledJrrp"])res << "ȫ�ֽ���.jrrp";
		if (console["DisabledDraw"])res << "ȫ�ֽ���.draw";
		if (console["DisabledSend"])res << "ȫ�ֽ���.send";
		if (trusted > 3)
			res << "����Ⱥ������" + to_string(getGroupList().size())
				<< "Ⱥ��¼����" + to_string(ChatList.size())
				<< "��������" + to_string(getFriendList().size())
				<< "�û���¼����" + to_string(UserList.size())
				<< (!PList.empty() ? "��ɫ����¼����" + to_string(PList.size()) : "�޽�ɫ����¼")
				<< "�������û�����" + to_string(blacklist->mQQDanger.size())
				<< "������Ⱥ����" + to_string(blacklist->mGroupDanger.size());
		reply(GlobalMsg["strSelfName"] + "�ĵ�ǰ���" + res.show());
		return 1;
	}
	if (trusted < 4)
	{
		reply(GlobalMsg["strNotAdmin"]);
		return -1;
	}
	if (auto it = Console::intDefault.find(strOption);it != Console::intDefault.end())
	{
		int intSet = 0;
		switch (readNum(intSet))
		{
		case 0:
			console.set(it->first, intSet);
			note("�ѽ�" + GlobalMsg["strSelfName"] + "��" + it->first + "����Ϊ" + to_string(intSet), 0b10);
			if (ConsoleSafe.count(it->first) != 0 && intSet != ConsoleSafe[it->first])
			{
				note(GlobalMsg["strConfSetToUnsafe"], 0b10);
			}
			break;
		case -1:
			reply(GlobalMsg["strSelfName"] + "����Ϊ" + to_string(console[strOption.c_str()]));
			break;
		case -2:
			reply("{nick}���ò���������Χ��");
			break;
		}
		return 1;
	}
	if (strOption == "delete")
	{
		note("�Ѿ���������ԱȨ�ޡ�", 0b100);
		getUser(fromQQ).trust(3);
		console.NoticeList.erase({fromQQ, msgtype::Private});
		return 1;
	}
	if (strOption == "on")
	{
		if (console["DisabledGlobal"])
		{
			console.set("DisabledGlobal", 0);
			note("��ȫ�ֿ���" + GlobalMsg["strSelfName"], 3);
		}
		else
		{
			reply(GlobalMsg["strSelfName"] + "���ھ�Ĭ�У�");
		}
		return 1;
	}
	if (strOption == "off")
	{
		if (console["DisabledGlobal"])
		{
			reply(GlobalMsg["strSelfName"] + "�Ѿ���Ĭ��");
		}
		else
		{
			console.set("DisabledGlobal", 1);
			note("��ȫ�ֹر�" + GlobalMsg["strSelfName"], 0b10);
		}
		return 1;
	}
	if (strOption == "dicelist")
	{
		getDiceList();
		strReply = "��ǰ�����б�";
		for (auto it : mDiceList)
		{
			strReply += "\n" + printQQ(it.first);
		}
		reply();
		return 1;
	}
	if (strOption == "only")
	{
		if (console["Private"])
		{
			reply(GlobalMsg["strSelfName"] + "�ѳ�Ϊ˽�����");
		}
		else
		{
			console.set("Private", 1);
			note("�ѽ�" + GlobalMsg["strSelfName"] + "��Ϊ˽�á�", 0b10);
		}
		return 1;
	}
	if (strOption == "public")
	{
		if (console["Private"])
		{
			console.set("Private", 0);
			note("�ѽ�" + GlobalMsg["strSelfName"] + "��Ϊ���á�", 0b10);
		}
		else
		{
			reply(GlobalMsg["strSelfName"] + "�ѳ�Ϊ�������");
		}
		return 1;
	}
	if (strOption == "clock")
	{
		bool isErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			isErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+')
		{
			intMsgCnt++;
		}
		string strType = readPara();
		if (strType.empty() || !Console::mClockEvent.count(strType))
		{
			reply(GlobalMsg["strSelfName"] + "�Ķ�ʱ�б�" + console.listClock().show());
			return 1;
		}
		Console::Clock cc{0, 0};
		switch (readClock(cc))
		{
		case 0:
			if (isErase)
			{
				if (console.rmClock(cc, ClockEvent(Console::mClockEvent[strType])))reply(
					GlobalMsg["strSelfName"] + "�޴˶�ʱ��Ŀ");
				else note("���Ƴ�" + GlobalMsg["strSelfName"] + "��" + printClock(cc) + "�Ķ�ʱ" + strType, 0b10);
			}
			else
			{
				console.setClock(cc, ClockEvent(Console::mClockEvent[strType]));
				note("������" + GlobalMsg["strSelfName"] + "��" + printClock(cc) + "�Ķ�ʱ" + strType, 0b10);
			}
			break;
		case -1:
			reply(GlobalMsg["strParaEmpty"]);
			break;
		case -2:
			reply(GlobalMsg["strParaIllegal"]);
			break;
		default: break;
		}
		return 1;
	}
	if (strOption == "notice")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		else if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		if (chatType cTarget; readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("��ǰ֪ͨ����" + to_string(list.size()) + "����" + list.show());
			return 1;
		}
		else
		{
			if (boolErase)
			{
				console.rmNotice(cTarget);
				note("�ѽ�" + GlobalMsg["strSelfName"] + "��֪ͨ����" + printChat(cTarget) + "�Ƴ�", 0b1);
				return 1;
			}
			readSkipSpace();
			if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-')
			{
				int intAdd = 0;
				int intReduce = 0;
				while (intMsgCnt < strMsg.length())
				{
					bool isReduce = strMsg[intMsgCnt] == '-';
					string strNum = readDigit();
					if (strNum.empty() || strNum.length() > 1)break;
					if (int intNum = stoi(strNum); intNum > 5)continue;
					else
					{
						if (isReduce)intReduce |= (1 << intNum);
						else intAdd |= (1 << intNum);
					}
					readSkipSpace();
				}
				if (intAdd)console.addNotice(cTarget, intAdd);
				if (intReduce)console.redNotice(cTarget, intReduce);
				if (intAdd | intReduce)note(
					"�ѽ�" + GlobalMsg["strSelfName"] + "�Դ���" + printChat(cTarget) + "֪ͨ�������Ϊ" + to_binary(
						console.showNotice(cTarget)), 0b1);
				else reply(GlobalMsg["strParaIllegal"]);
				return 1;
			}
			int intLV;
			switch (readNum(intLV))
			{
			case 0:
				if (intLV < 0 || intLV > 63)
				{
					reply(GlobalMsg["strParaIllegal"]);
					return 1;
				}
				console.setNotice(cTarget, intLV);
				note("�ѽ�" + GlobalMsg["strSelfName"] + "�Դ���" + printChat(cTarget) + "֪ͨ�������Ϊ" + to_string(intLV), 0b1);
				break;
			case -1:
				reply("����" + printChat(cTarget) + "��" + GlobalMsg["strSelfName"] + "����֪ͨ����Ϊ��" + to_binary(
					console.showNotice(cTarget)));
				break;
			case -2:
				reply(GlobalMsg["strParaIllegal"]);
				break;
			}
		}
		return 1;
	}
	if (strOption == "recorder")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		chatType cTarget;
		if (readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("��ǰ֪ͨ����" + to_string(list.size()) + "����" + list.show());
			return 1;
		}
		if (boolErase)
		{
			if (console.showNotice(cTarget) & 0b1)
			{
				note("��ֹͣ����" + printChat(cTarget) + "��", 0b1);
				console.redNotice(cTarget, 0b1);
			}
			else
			{
				reply("�ô��ڲ�������־֪ͨ��");
			}
		}
		else
		{
			if (console.showNotice(cTarget) & 0b1)
			{
				reply("�ô����ѽ�����־֪ͨ��");
			}
			else
			{
				console.addNotice(cTarget, 0b11011);
				reply("�������־����" + printChat(cTarget) + "��");
			}
		}
		return 1;
	}
	if (strOption == "monitor")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		chatType cTarget;
		if (readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("��ǰ֪ͨ����" + to_string(list.size()) + "����" + list.show());
			return 1;
		}
		if (boolErase)
		{
			if (console.showNotice(cTarget) & 0b100000)
			{
				console.redNotice(cTarget, 0b100000);
				note("���Ƴ����Ӵ���" + printChat(cTarget) + "��", 0b1);
			}
			else
			{
				reply("�ô��ڲ������ڼ����б�");
			}
		}
		else
		{
			if (console.showNotice(cTarget) & 0b100000)
			{
				reply("�ô����Ѵ����ڼ����б�");
			}
			else
			{
				console.addNotice(cTarget, 0b100000);
				note("����Ӽ��Ӵ���" + printChat(cTarget) + "��", 0b1);
			}
		}
		return 1;
	}
	if (strOption == "blackfriend")
	{
		ResList res;
		Unpack pack(base64_decode(CQ_getFriendList(getAuthCode()))); //��ȡԭʼ����ת��ΪUnpack
		int Cnt = pack.getInt(); //��ȡ����
		while (Cnt--)
		{
			FriendInfo info(pack.getUnpack()); //��ȡ
			if (blacklist->get_qq_danger(info.QQID))
				res << info.tostring();
		}
		if (res.empty())
		{
			reply("�����б����޺������û���", false);
		}
		else
		{
			reply("�����б��ں������û���" + res.show(), false);
		}
		return 1;
	}
	if (strOption == "whitegroup")
	{
		readSkipSpace();
		bool isErase = false;
		if (strMsg[intMsgCnt] == '-')
		{
			intMsgCnt++;
			isErase = true;
		}
		if (string strGroup; !(strGroup = readDigit()).empty())
		{
			long long llGroup = stoll(strGroup);
			if (isErase)
			{
				if (groupset(llGroup, "���ʹ��") > 0 || groupset(llGroup, "����") > 0)
				{
					chat(llGroup).reset("���ʹ��").reset("����");
					note("���Ƴ�" + printGroup(llGroup) + "��" + GlobalMsg["strSelfName"] + "��ʹ�����");
				}
				else
				{
					reply("��Ⱥδӵ��" + GlobalMsg["strSelfName"] + "��ʹ����ɣ�");
				}
			}
			else
			{
				if (groupset(llGroup, "���ʹ��") > 0)
				{
					reply("��Ⱥ��ӵ��" + GlobalMsg["strSelfName"] + "��ʹ����ɣ�");
				}
				else
				{
					chat(llGroup).set("���ʹ��").reset("δ���");
					if (!chat(llGroup).isset("����") && !chat(llGroup).isset("δ��"))AddMsgToQueue(
						getMsg("strAuthorized"), llGroup, msgtype::Group);
					note("�����" + printGroup(llGroup) + "��" + GlobalMsg["strSelfName"] + "��ʹ�����");
				}
			}
			return 1;
		}
		ResList res;
		for (auto& [id, grp] : ChatList)
		{
			string strGroup;
			if (grp.isset("���ʹ��") || grp.isset("����") || grp.isset("���"))
			{
				strGroup = printChat(grp);
				if (grp.isset("���ʹ��"))strGroup += "-���ʹ��";
				if (grp.isset("����"))strGroup += "-����";
				if (grp.isset("���"))strGroup += "-���";
				res << strGroup;
			}
		}
		reply("��ǰ������Ⱥ" + to_string(res.size()) + "����" + res.show());
		return 1;
	}
	if (strOption == "frq")
	{
		reply("��ǰ��ָ��Ƶ��" + to_string(FrqMonitor::getFrqTotal()));
		return 1;
	}
	bool boolErase = false;
	strVar["note"] = readPara();
	if (strMsg[intMsgCnt] == '-')
	{
		boolErase = true;
		intMsgCnt++;
	}
	if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
	long long llTargetID = readID();
	if (strOption == "dismiss")
	{
		if (ChatList.count(llTargetID))
		{
			note("����" + GlobalMsg["strSelfName"] + "�˳�" + printChat(chat(llTargetID)), 0b10);
			chat(llTargetID).reset("����").leave();
		}
		else
		{
			reply(GlobalMsg["strGroupGetErr"]);
		}
		return 1;
	}
	if (strOption == "boton")
	{
		if (getGroupList().count(llTargetID))
		{
			if (groupset(llTargetID, "ͣ��ָ��") > 0)
			{
				chat(llTargetID).reset("ͣ��ָ��");
				note("����" + GlobalMsg["strSelfName"] + "��" + printGroup(llTargetID) + "����ָ���");
			}
			else reply(GlobalMsg["strSelfName"] + "���ڸ�Ⱥ����ָ��!");
		}
		else
		{
			reply(GlobalMsg["strGroupGetErr"]);
		}
	}
	else if (strOption == "botoff")
	{
		if (groupset(llTargetID, "ͣ��ָ��") < 1)
		{
			chat(llTargetID).set("ͣ��ָ��");
			note("����" + GlobalMsg["strSelfName"] + "��" + printGroup(llTargetID) + "ͣ��ָ���", 0b1);
		}
		else reply(GlobalMsg["strSelfName"] + "���ڸ�Ⱥͣ��ָ��!");
		return 1;
	}
	else if (strOption == "blackgroup")
	{
		if (llTargetID == 0)
		{
			strReply = "��ǰ������Ⱥ�б�";
			for (auto [each, danger] : blacklist->mGroupDanger)
			{
				strReply += "\n" + to_string(each);
			}
			reply();
			return 1;
		}
		strVar["time"] = printSTNow();
		do
		{
			if (boolErase)
			{
				blacklist->rm_black_group(llTargetID, this);
			}
			else
			{
				blacklist->add_black_group(llTargetID, this);
			}
		}
		while ((llTargetID = readID()));
		return 1;
	}
	else if (strOption == "whiteqq")
	{
		if (llTargetID == 0)
		{
			strReply = "��ǰ�������û��б�";
			for (auto& [qq, user] : UserList)
			{
				if (user.nTrust)strReply += "\n" + printQQ(qq) + ":" + to_string(user.nTrust);
			}
			reply();
			return 1;
		}
		do
		{
			if (boolErase)
			{
				if (trustedQQ(llTargetID))
				{
					if (trusted <= trustedQQ(llTargetID))
					{
						reply(GlobalMsg["strUserTrustDenied"]);
					}
					else
					{
						getUser(llTargetID).trust(0);
						note("���ջ�" + GlobalMsg["strSelfName"] + "��" + printQQ(llTargetID) + "�����Ρ�", 0b1);
					}
				}
				else
				{
					reply(printQQ(llTargetID) + "������" + GlobalMsg["strSelfName"] + "�İ�������");
				}
			}
			else
			{
				if (trustedQQ(llTargetID))
				{
					reply(printQQ(llTargetID) + "�Ѽ���" + GlobalMsg["strSelfName"] + "�İ�����!");
				}
				else
				{
					getUser(llTargetID).trust(1);
					note("�����" + GlobalMsg["strSelfName"] + "��" + printQQ(llTargetID) + "�����Ρ�", 0b1);
					strVar["user_nick"] = getName(llTargetID);
					AddMsgToQueue(format(GlobalMsg["strWhiteQQAddNotice"], GlobalMsg, strVar), llTargetID);
				}
			}
		}
		while ((llTargetID = readID()));
		return 1;
	}
	else if (strOption == "blackqq")
	{
		if (llTargetID == 0)
		{
			strReply = "��ǰ�������û��б�";
			for (auto [each, danger] : blacklist->mQQDanger)
			{
				strReply += "\n" + printQQ(each);
			}
			reply();
			return 1;
		}
		strVar["time"] = printSTNow();
		do
		{
			if (boolErase)
			{
				blacklist->rm_black_qq(llTargetID, this);
			}
			else
			{
				blacklist->add_black_qq(llTargetID, this);
			}
		}
		while ((llTargetID = readID()));
		return 1;
	}
	else reply(GlobalMsg["strAdminOptionEmpty"]);
	return 0;
}

int FromMsg::MasterSet()
{
	const std::string strOption = readPara();
	if (strOption.empty())
	{
		reply(GlobalMsg["strAdminOptionEmpty"]);
		return -1;
	}
	if (strOption == "groupclr")
	{
		const std::string strPara = readRest();
		clearGroup(strPara, fromQQ);
		return 1;
	}
	if (strOption == "delete")
	{
		if (console.master() != fromQQ)
		{
			reply(GlobalMsg["strNotMaster"]);
			return 1;
		}
		reply("�㲻����" + GlobalMsg["strSelfName"] + "��Master��");
		console.killMaster();
		return 1;
	}
	if (strOption == "reset")
	{
		if (console.master() != fromQQ)
		{
			reply(GlobalMsg["strNotMaster"]);
			return 1;
		}
		const string strMaster = readDigit();
		if (strMaster.empty() || stoll(strMaster) == console.master())
		{
			reply("Master��Ҫ��ǲ����!");
		}
		else
		{
			console.newMaster(stoll(strMaster));
			note("�ѽ�Masterת�ø�" + printQQ(console.master()));
		}
		return 1;
	}
	if (strOption == "admin")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		long long llAdmin = readID();
		if (llAdmin)
		{
			if (boolErase)
			{
				if (trustedQQ(llAdmin) > 3)
				{
					note("���ջ�" + printQQ(llAdmin) + "��" + GlobalMsg["strSelfName"] + "�Ĺ���Ȩ�ޡ�", 0b100);
					console.rmNotice({llAdmin, msgtype::Private});
					getUser(llAdmin).trust(0);
				}
				else
				{
					reply("���û��޹���Ȩ�ޣ�");
				}
			}
			else
			{
				if (trustedQQ(llAdmin) > 3)
				{
					reply("���û����й���Ȩ�ޣ�");
				}
				else
				{
					getUser(llAdmin).trust(4);
					console.addNotice({llAdmin, msgtype::Private}, 0b1110);
					note("�����" + printQQ(llAdmin) + "��" + GlobalMsg["strSelfName"] + "�Ĺ���Ȩ�ޡ�", 0b100);
				}
			}
			return 1;
		}
		ResList list;
		for (const auto& [qq, user] : UserList)
		{
			if (user.nTrust > 3)list << printQQ(qq);
		}
		reply(GlobalMsg["strSelfName"] + "�Ĺ���Ȩ��ӵ���߹�" + to_string(list.size()) + "λ��" + list.show());
		return 1;
	}
	return AdminEvent(strOption);
}

int FromMsg::DiceReply()
{
	if (strMsg[0] != '.')return 0;
	intMsgCnt++;
	int intT = static_cast<int>(fromType);
	while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
		intMsgCnt++;
	strVar["nick"] = getName(fromQQ, fromGroup);
	strVar["pc"] = getPCName(fromQQ, fromGroup);
	strVar["at"] = intT ? "[CQ:at,qq=" + to_string(fromQQ) + "]" : strVar["nick"];
	isAuth = trusted > 3 || fromType != msgtype::Group || getGroupMemberInfo(fromGroup, fromQQ).permissions > 1;
	strLowerMessage = strMsg;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(),
	               [](unsigned char c) { return tolower(c); });
	//ָ��ƥ��
	if (strLowerMessage.substr(intMsgCnt, 9) == "authorize")
	{
		intMsgCnt += 9;
		readSkipSpace();
		if (intT != GroupT || strMsg[intMsgCnt] == '+')
		{
			long long llTarget = readID();
			if (llTarget)
			{
				pGrp = &chat(llTarget);
			}
			else
			{
				reply(GlobalMsg["strGroupIDEmpty"]);
				return 1;
			}
		}
		if (pGrp->isset("���ʹ��") && !pGrp->isset("δ���"))return 0;
		if (trusted > 0)
		{
			pGrp->set("���ʹ��").reset("δ���");
			note("����Ȩ" + printGroup(pGrp->ID) + "���ʹ��", 1);
			AddMsgToQueue(getMsg("strGroupAuthorized", strVar), pGrp->ID, msgtype::Group);
		}
		else
		{
			if (!console["CheckGroupLicense"] && !console["Private"] && !isCalled)return 0;
			string strInfo = readRest();
			if (strInfo.empty())console.log(printQQ(fromQQ) + "����" + printGroup(pGrp->ID) + "���ʹ��", 0b10, printSTNow());
			else console.log(printQQ(fromQQ) + "����" + printGroup(pGrp->ID) + "���ʹ�ã����ԣ�" + strInfo, 0b100, printSTNow());
			reply(GlobalMsg["strGroupLicenseApply"]);
		}
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		if (!intT)
		{
			string QQNum = readDigit();
			if (QQNum.empty())
			{
				reply(GlobalMsg["strDismissPrivate"]);
				return -1;
			}
			long long llGroup = stoll(QQNum);
			if (!ChatList.count(llGroup))
			{
				reply(GlobalMsg["strGroupNotFound"]);
				return 1;
			}
			Chat& grp = chat(llGroup);
			if (grp.isset("����") || grp.isset("δ��"))
			{
				reply(GlobalMsg["strGroupAway"]);
				return 1;
			}
			if (trustedQQ(fromQQ) > 2 || getGroupMemberInfo(llGroup, fromQQ).permissions > 1)
			{
				grp.leave(GlobalMsg["strDismiss"]);
				reply(GlobalMsg["strGroupExit"]);
				return 1;
			}
			reply(GlobalMsg["strPermissionDeniedErr"]);
			return 1;
		}
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string QQNum = readDigit();
		if (QQNum.empty() || QQNum == to_string(console.DiceMaid) || (QQNum.length() == 4 && stoll(QQNum) ==
			getLoginQQ() % 10000))
		{
			if (!isAuth && trusted < 3)
			{
				if (pGrp->isset("ͣ��ָ��") && GroupInfo(fromGroup).nGroupSize > 200)AddMsgToQueue(
					getMsg("strPermissionDeniedErr", strVar), fromQQ);
				else reply(GlobalMsg["strPermissionDeniedErr"]);
				return -1;
			}
			chat(fromGroup).leave(GlobalMsg["strDismiss"]);
		}
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "warning")
	{
		intMsgCnt += 7;
		string strWarning = readRest();
		AddWarning(strWarning, fromQQ, fromGroup);
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 6) == "master" && console.isMasterMode)
	{
		intMsgCnt += 6;
		if (!console.master())
		{
			console.newMaster(fromQQ);
			strReply = "�������Ķ���ǰ�汾Master�ֲ�������ְ�����·�����:shiki.stringempty.xyz/download/Shiki_Master_Manual.pdf";
			strReply += "\n�û��ֲ�:shiki.stringempty.xyz/download/Shiki_User_Manual.pdf";
			strReply += "\n��Ҫ��ӽ϶�û�е�Ⱥ���صĲ�����Ƽ�����DisabledBlock��֤Ⱥ�ڵľ�Ĭ��";
			strReply += "\nĬ�Ͽ�����Ⱥ�Ƴ������ԡ�ˢ���¼��ļ�������Ҫ�ر����ֶ�������";
			string strOption = readRest();
			if (strOption == "public")
			{
				strReply += "\n��������ģʽ��";
				console.set("BelieveDiceList", 1);
				strReply += "\n�Զ�����BelieveDiceList��Ӧ���������б��warning��";
				console.set("AllowStranger", 1);
				strReply += "\n����ģʽĬ��ͬ��İ���˵ĺ������룻";
				console.set("LeaveBlackQQ", 1);
				console.set("BannedLeave", 1);
				strReply += "\n�ѿ����������Զ���������ʱ��ÿ�ն�ʱ���Զ�������������û��Ĺ�ͬȺ�ģ��������û�ȺȨ�޲������Լ�ʱ�Զ���Ⱥ��";
				console.set("BannedBanInviter", 1);
				console.set("KickedBanInviter", 1);
				strReply += "\n����Ⱥʱ�����������ˣ������ֶ��رգ�";
				console.set("DisabledSend", 0);
				strReply += "\n������send���ܣ�";
			}
			else
			{
				console.set("Private", 1);
				strReply += "\nĬ�Ͽ���˽��ģʽ��";
				strReply += "\nĬ�Ͼܾ�İ���˵ĺ������룬��Ҫͬ���뿪��AllowStranger��";
				strReply += "\nĬ�Ͼܾ�İ���˵�Ⱥ���룬ֻͬ�����Թ���Ա�������������룻";
				strReply += "\n�ѿ����������Զ���������ʱ��ÿ�ն�ʱ���Զ�������������û��Ĺ�ͬȺ�ģ��������û�ȺȨ�޸����Լ�ʱ�Զ���Ⱥ��";
				strReply += "\n.me����Ĭ�ϲ����ã���Ҫ�ֶ�������";
				strReply += "\n�л�������ʹ��.admin public���������ʼ����Ӧ���ã�";
				strReply += "\n����.master delete��ʹ��.master public�����³�ʼ����";
			}
			reply();
		}
		else if (trusted > 4 || console.master() == fromQQ)
		{
			return MasterSet();
		}
		else
		{
			if (isCalled)reply(GlobalMsg["strNotMaster"]);
			return 1;
		}
		return 1;
	}
	if (blacklist->get_qq_danger(fromQQ) || (intT != PrivateT && blacklist->get_group_danger(fromGroup)))
	{
		return 0;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		string Command = readPara();
		string QQNum = readDigit();
		if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && stoll(QQNum) == getLoginQQ() %
			10000))
		{
			if (Command == "on")
			{
				if (console["DisabledGlobal"])reply(GlobalMsg["strGlobalOff"]);
				else if (intT == GroupT && ((console["CheckGroupLicense"] && pGrp->isset("δ���")) || (console[
					"CheckGroupLicense"] == 2 && !pGrp->isset("���ʹ��"))))reply(GlobalMsg["strGroupLicenseDeny"]);
				else if (intT)
				{
					if (isAuth)
					{
						if (groupset(fromGroup, "ͣ��ָ��") > 0)
						{
							chat(fromGroup).reset("ͣ��ָ��");
							reply(GlobalMsg["strBotOn"]);
						}
						else
						{
							reply(GlobalMsg["strBotOnAlready"]);
						}
					}
					else
					{
						if (groupset(fromGroup, "ͣ��ָ��") > 0 && GroupInfo(fromGroup).nGroupSize > 100)AddMsgToQueue(
							getMsg("strPermissionDeniedErr", strVar), fromQQ);
						else reply(GlobalMsg["strPermissionDeniedErr"]);
					}
				}
			}
			else if (Command == "off")
			{
				if (fromType == msgtype(intT))
				{
					if (isAuth)
					{
						if (groupset(fromGroup, "ͣ��ָ��"))
						{
							if (!isCalled && QQNum.empty() && pGrp->isGroup && GroupInfo(fromGroup).nGroupSize > 200)
								AddMsgToQueue(getMsg("strBotOffAlready", strVar), fromQQ);
							else reply(GlobalMsg["strBotOffAlready"]);
						}
						else
						{
							chat(fromGroup).set("ͣ��ָ��");
							reply(GlobalMsg["strBotOff"]);
						}
					}
					else
					{
						if (groupset(fromGroup, "ͣ��ָ��"))AddMsgToQueue(getMsg("strPermissionDeniedErr", strVar), fromQQ);
						else reply(GlobalMsg["strPermissionDeniedErr"]);
					}
				}
			}
			else if (!Command.empty() && !isCalled && pGrp->isset("ͣ��ָ��"))
			{
				return 0;
			}
			else if (intT == GroupT && pGrp->isset("ͣ��ָ��") && GroupInfo(fromGroup).nGroupSize >= 500 && !isCalled)
			{
				AddMsgToQueue(Dice_Full_Ver_For + getMsg("strBotMsg"), fromQQ);
			}
			else
			{
				this_thread::sleep_for(1s);
				reply(Dice_Full_Ver_For + GlobalMsg["strBotMsg"]);
			}
		}
		return 1;
	}
	if (isBotOff)
	{
		if (intT == PrivateT)
		{
			reply(GlobalMsg["strGlobalOff"]);
			return 1;
		}
		return 0;
	}
	/*switch(console.DSens.find(strLowerMessage, fromQQ, fromChat)) {
	case 0:break;
	case 1:
		reply(GlobalMsg["strSensNote"]);
		break;
	case 2:
		reply(GlobalMsg["strSensWarn"]);
		return 1;
	}*/
	if (strLowerMessage.substr(intMsgCnt, 7) == "helpdoc" && trusted > 3)
	{
		intMsgCnt += 7;
		while (strMsg[intMsgCnt] == ' ')
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			reply(GlobalMsg["strHlpNameEmpty"]);
			return true;
		}
		strVar["key"] = readUntilSpace();
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			CustomHelp.erase(strVar["key"]);
			if (auto it = HelpDoc.find(strVar["key"]); it != HelpDoc.end())
				fmt->set_help(it->first, it->second);
			else
				fmt->rm_help(strVar["key"]);
			reply(format(GlobalMsg["strHlpReset"], {strVar["key"]}));
		}
		else
		{
			string strHelpdoc = strMsg.substr(intMsgCnt);
			CustomHelp[strVar["key"]] = strHelpdoc;
			fmt->set_help(strVar["key"], strHelpdoc);
			reply(format(GlobalMsg["strHlpSet"], {strVar["key"]}));
		}
		saveJMap(DiceDir + "\\conf\\CustomHelp.json", CustomHelp);
		return true;
	}
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		const string strOption = readRest();
		if (intT)
		{
			if (!isAuth && (strOption == "on" || strOption == "off"))
			{
				reply(GlobalMsg["strPermissionDeniedErr"]);
				return 1;
			}
			strVar["option"] = "����help";
			if (strOption == "off")
			{
				if (groupset(fromGroup, strVar["option"]) < 1)
				{
					chat(fromGroup).set(strVar["option"]);
					reply(GlobalMsg["strGroupSetOn"]);
				}
				else
				{
					reply(GlobalMsg["strGroupSetOnAlready"]);
				}
				return 1;
			}
			if (strOption == "on")
			{
				if (groupset(fromGroup, strVar["option"]) > 0)
				{
					chat(fromGroup).reset(strVar["option"]);
					reply(GlobalMsg["strGroupSetOff"]);
				}
				else
				{
					reply(GlobalMsg["strGroupSetOffAlready"]);
				}
				return 1;
			}
			if (groupset(fromGroup, strVar["option"]) > 0)
			{
				reply(GlobalMsg["strGroupSetOnAlready"]);
				return 1;
			}
		}
		if (strOption.empty())
		{
			reply(string(Dice_Short_Ver) + "\n" + GlobalMsg["strHlpMsg"]);
		}
		else
		{
			reply(fmt->get_help(strOption));
		}
		return true;
	}
	if (intT == GroupT && ((console["CheckGroupLicense"] && pGrp->isset("δ���")) || (console["CheckGroupLicense"] == 2 &&
		!pGrp->isset("���ʹ��"))))
	{
		return 0;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "welcome")
	{
		if (intT != GroupT)
		{
			reply(GlobalMsg["strWelcomePrivate"]);
			return 1;
		}
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (isAuth)
		{
			string strWelcomeMsg = strMsg.substr(intMsgCnt);
			if (strWelcomeMsg.empty())
			{
				if (chat(fromGroup).strConf.count("��Ⱥ��ӭ"))
				{
					chat(fromGroup).rmText("��Ⱥ��ӭ");
					reply(GlobalMsg["strWelcomeMsgClearNotice"]);
				}
				else
				{
					reply(GlobalMsg["strWelcomeMsgClearErr"]);
				}
			}
			else if (strWelcomeMsg == "show")
			{
				reply(chat(fromGroup).strConf["��Ⱥ��ӭ"]);
			}
			else
			{
				chat(fromGroup).setText("��Ⱥ��ӭ", strWelcomeMsg);
				reply(GlobalMsg["strWelcomeMsgUpdateNotice"]);
			}
		}
		else
		{
			reply(GlobalMsg["strPermissionDeniedErr"]);
		}
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 6) == "setcoc")
	{
		if (!isAuth)
		{
			reply(GlobalMsg["strPermissionDeniedErr"]);
			return 1;
		}
		string strRule = readDigit();
		if (strRule.empty())
		{
			if (intT)chat(fromGroup).rmConf("rc����");
			else getUser(fromQQ).rmIntConf("rc����");
			reply(GlobalMsg["strDefaultCOCClr"]);
			return 1;
		}
		if (strRule.length() > 1)
		{
			reply(GlobalMsg["strDefaultCOCNotFound"]);
			return 1;
		}
		int intRule = stoi(strRule);
		switch (intRule)
		{
		case 0:
			reply(GlobalMsg["strDefaultCOCSet"] + "0 ������\n��1��ɹ�\n����50��96-100��ʧ�ܣ���50��100��ʧ��");
			break;
		case 1:
			reply(GlobalMsg["strDefaultCOCSet"] + "1\n����50��1��ɹ�����50��1-5��ɹ�\n����50��96-100��ʧ�ܣ���50��100��ʧ��");
			break;
		case 2:
			reply(GlobalMsg["strDefaultCOCSet"] + "2\n��1-5��<=�ɹ��ʴ�ɹ�\n��100���96-99��>�ɹ��ʴ�ʧ��");
			break;
		case 3:
			reply(GlobalMsg["strDefaultCOCSet"] + "3\n��1-5��ɹ�\n��96-100��ʧ��");
			break;
		case 4:
			reply(GlobalMsg["strDefaultCOCSet"] + "4\n��1-5��<=ʮ��֮һ��ɹ�\n����50��>=96+ʮ��֮һ��ʧ�ܣ���50��100��ʧ��");
			break;
		case 5:
			reply(GlobalMsg["strDefaultCOCSet"] + "5\n��1-2��<���֮һ��ɹ�\n����50��96-100��ʧ�ܣ���50��99-100��ʧ��");
			break;
		default:
			reply(GlobalMsg["strDefaultCOCNotFound"]);
			return 1;
		}
		if (intT)chat(fromGroup).setConf("rc����", intRule);
		else getUser(fromQQ).setConf("rc����", intRule);
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 6) == "system")
	{
		intMsgCnt += 6;
		if (trusted < 4)
		{
			reply(GlobalMsg["strNotAdmin"]);
			return -1;
		}
		string strOption = readPara();
		if (strOption == "save")
		{
			dataBackUp();
			note("���ֶ��������ݡ�", 0b1);
			return 1;
		}
		if (strOption == "load")
		{
			loadData();
			note("���ֶ��������ݡ�", 0b1);
			return 1;
		}
		if (strOption == "state")
		{
			GetLocalTime(&stNow);
			strReply = "����ʱ�䣺" + printSTime(stNow) + "\n";
			strReply += "�ڴ�ռ�ã�" + to_string(getRamPort()) + "%\n";
			strReply += "CPUռ�ã�" + to_string(getWinCpuUsage()) + "%\n";
			//strReply += "CPUռ�ã�" + to_string(getWinCpuUsage()) + "% / ����ռ�ã�" + to_string(getProcessCpu() / 100.0) + "%\n";
			//strReply += "��������ʱ�䣺" + std::to_string(clock()) + " ����ʱ�䣺" + std::to_string(llStartTime) + "\n";
			strReply += "����ʱ����";
			long long llDuration = (clock() - llStartTime) / 1000;
			if (llDuration < 0)
			{
				strReply += "N/A";
			}
			else if (llDuration < 60 * 2)
			{
				strReply += std::to_string(llDuration) + "��";
			}
			else if (llDuration < 60 * 60 * 2)
			{
				strReply += std::to_string(llDuration / 60) + "��" + std::to_string(llDuration % 60) + "��";
			}
			else if (llDuration < 60 * 60 * 24 * 2)
			{
				strReply += std::to_string(llDuration / 60 / 60) + "Сʱ" + std::to_string(llDuration / 60 % 60) + "��";
			}
			else if (llDuration < 60 * 60 * 24 * 10)
			{
				strReply += std::to_string(llDuration / 60 / 60 / 24) + "��" + std::to_string(llDuration / 60 / 60 % 24)
					+ "Сʱ";
			}
			else
			{
				strReply += std::to_string(llDuration / 60 / 60 / 24) + "��";
			}
			reply();
			return 1;
		}
		if (strOption == "clrimg")
		{
			if (Mirai)
			{
				reply("Mirai��֧�ִ˹���");
				return -1;
			}
			if (trusted < 5)
			{
				reply(GlobalMsg["strNotMaster"]);
				return -1;
			}
			int Cnt = clearImage();
			note("������image�ļ�" + to_string(Cnt) + "��", 0b1);
			return 1;
		}
		if (strOption == "reload")
		{
			if (Mirai)
			{
				reply("Mirai��֧�ִ˹���");
				return -1;
			}
			if (trusted < 5)
			{
				reply(GlobalMsg["strNotMaster"]);
				return -1;
			}

			string strSelfName;
			int pid = _getpid();
			PROCESSENTRY32 pe32;
			pe32.dwSize = sizeof(pe32);
			HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hProcessSnap == INVALID_HANDLE_VALUE)
			{
				note("����ʧ�ܣ����̿��մ���ʧ�ܣ�", 1);
				return false;
			}
			BOOL bResult = Process32First(hProcessSnap, &pe32);
			int ppid(0);
			while (bResult)
			{
				if (pe32.th32ProcessID == pid)
				{
					ppid = pe32.th32ParentProcessID;
					reply("ȷ�Ͻ���" + strModulePath + "\n������id:" + to_string(pe32.th32ProcessID) + "\n������id:" + to_string(
						pe32.th32ParentProcessID));
					strSelfName = pe32.szExeFile;
					break;
				}
				bResult = Process32Next(hProcessSnap, &pe32);
			}
			if (!ppid)
			{
				note("����ʧ�ܣ�δ�ҵ����̣�", 1);
				return -1;
			}
			string command = "taskkill /f /pid " + to_string(ppid) + "\nstart .\\" + strSelfName + " /account " +
				to_string(getLoginQQ());
			//string command = "taskkill /f /pid " + to_string(ppid) + "\ntaskkill /f /pid " + to_string(pid) + "\nstart " + strSelfPath + " /account " + to_string(getLoginQQ()) + "\ntimeout /t 60\ndel %0";
			ofstream fout("reload.bat");
			fout << command << std::endl;
			fout.close();
			note(command, 0);
			this_thread::sleep_for(5s);
			Enabled = false;
			dataBackUp();
			switch (UINT res = -1;res = WinExec(".\\reload.bat", SW_SHOW))
			{
			case 0:
				note("����ʧ�ܣ��ڴ����Դ�Ѻľ���", 1);
				break;
			case ERROR_FILE_NOT_FOUND:
				note("����ʧ�ܣ�ָ�����ļ�δ�ҵ���", 1);
				break;
			case ERROR_PATH_NOT_FOUND:
				note("����ʧ�ܣ�ָ����·��δ�ҵ���", 1);
				break;
			default:
				if (res > 31)note("�����ɹ�" + to_string(res), 0);
				else note("����ʧ�ܣ�δ֪����" + to_string(res), 0);
				break;
			}
			return 1;
		}
		if (strOption == "rexplorer")
		{
			if (trusted < 5)
			{
				reply(GlobalMsg["strNotMaster"]);
				return -1;
			}
			system(R"(taskkill /f /fi "username eq %username%" /im explorer.exe)");
			system(R"(start %SystemRoot%\explorer.exe)");
			note("��������Դ��������\n��ǰ�ڴ�ռ�ã�" + to_string(getRamPort()) + "%");
		}
		else if (strOption == "cmd")
		{
			if (fromQQ != console.master())
			{
				reply(GlobalMsg["strNotMaster"]);
				return -1;
			}
			string strCMD = readRest() + "\ntimeout /t 10";
			system(strCMD.c_str());
			reply("�����������С�");
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "admin")
	{
		intMsgCnt += 5;
		return AdminEvent(readPara());
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "cloud")
	{
		intMsgCnt += 5;
		string strOpt = readPara();
		if (trusted < 4 && fromQQ != console.master())
		{
			reply(GlobalMsg["strNotAdmin"]);
			return 1;
		}
		if (strOpt == "update")
		{
			string strPara = readPara();
			if (strPara.empty())
			{
				Cloud::checkUpdate(this);
			}
			else if (strPara == "dev" || strPara == "release")
			{
				string strAppPath(strModulePath);
				strAppPath = strAppPath.substr(0, strAppPath.find_last_of('\\')) + "\\app\\com.w4123.dice.cpk";

				switch (Cloud::DownloadFile(
					("http://shiki.stringempty.xyz/DiceVer/" + strPara + "?" + to_string(fromTime)).c_str(),
					strAppPath.c_str()))
				{
				case -1:
					reply("����ʧ��:" + strAppPath);
					break;
				case -2:
					reply("�ļ�δ�ҵ�:" + strAppPath);
					break;
				case 0:
					note("���¿�����ɹ���\n��reloadӦ�ø���");
				}
			}
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
		strReply = strVar["nick"];
		COC7D(strReply);
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
		strReply = strVar["nick"];
		COC6D(strReply);
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "group")
	{
		intMsgCnt += 5;
		long long llGroup(fromGroup);
		readSkipSpace();
		if (strLowerMessage.substr(intMsgCnt, 3) == "all")
		{
			if (trusted < 5)
			{
				reply(GlobalMsg["strNotMaster"]);
				return 1;
			}
			intMsgCnt += 3;
			readSkipSpace();
			if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-')
			{
				bool isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				string strOption = strVar["option"] = readRest();
				if (!mChatConf.count(strVar["option"]))
				{
					reply(GlobalMsg["strGroupSetNotExist"]);
					return 1;
				}
				int Cnt = 0;
				if (isSet)
				{
					for (auto& [id, grp] : ChatList)
					{
						if (grp.isset(strOption))continue;
						grp.set(strOption);
						Cnt++;
					}
					strVar["cnt"] = to_string(Cnt);
					note(GlobalMsg["strGroupSetAll"], 0b100);
				}
				else
				{
					for (auto& [id, grp] : ChatList)
					{
						if (!grp.isset(strOption))continue;
						grp.reset(strOption);
						Cnt++;
					}
					strVar["cnt"] = to_string(Cnt);
					note(GlobalMsg["strGroupSetAll"], 0b100);
				}
			}
			return 1;
		}
		if (string strGroup = readDigit(false); !strGroup.empty())
		{
			llGroup = stoll(strGroup);
			if (!ChatList.count(llGroup))
			{
				reply(GlobalMsg["strGroupNotFound"]);
				return 1;
			}
			if (getGroupAuth(llGroup) < 0)
			{
				reply(GlobalMsg["strGroupDenied"]);
				return 1;
			}
		}
		else if (intT != GroupT)return 0;
		Chat& grp = chat(llGroup);
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-')
		{
			bool isSet = strMsg[intMsgCnt] == '+';
			intMsgCnt++;
			strVar["option"] = readRest();
			if (!mChatConf.count(strVar["option"]))
			{
				reply(GlobalMsg["strGroupSetDenied"]);
				return 1;
			}
			if (getGroupAuth(llGroup) >= get<string, short>(mChatConf, strVar["option"], 0))
			{
				if (isSet)
				{
					if (groupset(llGroup, strVar["option"]) < 1)
					{
						chat(llGroup).set(strVar["option"]);
						reply(GlobalMsg["strGroupSetOn"]);
						if (strVar["Option"] == "���ʹ��")
						{
							AddMsgToQueue(getMsg("strGroupAuthorized", strVar), fromQQ, msgtype::Group);
						}
					}
					else
					{
						reply(GlobalMsg["strGroupSetOnAlready"]);
					}
					return 1;
				}
				if (grp.isset(strVar["option"]))
				{
					chat(llGroup).reset(strVar["option"]);
					reply(GlobalMsg["strGroupSetOff"]);
				}
				else
				{
					reply(GlobalMsg["strGroupSetOffAlready"]);
				}
				return 1;
			}
			reply(GlobalMsg["strGroupSetDenied"]);
			return 1;
		}
		GroupInfo grpinfo;
		bool isInGroup = getGroupList().count(llGroup);
		if (isInGroup)grpinfo = GroupInfo(llGroup);
		string Command = readPara();

		if (Command == "state")
		{
			time_t tNow = time(nullptr);
			const int intTMonth = 30 * 24 * 60 * 60;
			set<string> sInact;
			set<string> sBlackQQ;
			if (isInGroup)
				for (const auto& each : getGroupMemberList(llGroup))
				{
					if (!each.LastMsgTime || tNow - each.LastMsgTime > intTMonth)
					{
						sInact.insert(each.GroupNick + "(" + to_string(each.QQID) + ")");
					}
					if (blacklist->get_qq_danger(each.QQID))
					{
						sBlackQQ.insert(each.GroupNick + "(" + to_string(each.QQID) + ")");
					}
				}
			ResList res;
			strVar["group"] = grpinfo.llGroup ? grpinfo.tostring() : printGroup(llGroup);
			res << "��{group}��";
			res << grp.listBoolConf();
			for (const auto& it : grp.intConf)
			{
				res << it.first + "��" + to_string(it.second);
			}
			res << "��¼������" + printDate(grp.tCreated);
			res << "����¼��" + printDate(grp.tUpdated);
			if (grp.inviter)res << "�����ߣ�" + printQQ(grp.inviter);
			if (isInGroup)
			{
				res << string("��Ⱥ��ӭ��") + (grp.isset("��Ⱥ��ӭ") ? "������" : "δ����");
				res << (!sInact.empty() ? "\n30�첻��ԾȺԱ����" + to_string(sInact.size()) : "");
				if (!sBlackQQ.empty())
				{
					if (sBlackQQ.size() > 8)
						res << GlobalMsg["strSelfName"] + "�ĺ�������Ա" + to_string(sBlackQQ.size()) + "��";
					else
					{
						res << GlobalMsg["strSelfName"] + "�ĺ�������Ա:{blackqq}";
						ResList blacks;
						for (const auto& each : sBlackQQ)
						{
							blacks << each;
						}
						strVar["blackqq"] = blacks.show();
					}
				}
				else
				{
					res << "��{self}�ĺ�������Ա";
				}
			}
			reply(GlobalMsg["strSelfName"] + res.show());
			return 1;
		}
		if (!isInGroup && (intT != GroupT || fromGroup != llGroup))
		{
			reply(GlobalMsg["strGroupNotIn"]);
			return 1;
		}
		if (Command == "info")
		{
			reply(grpinfo.tostring(), false);
			return 1;
		}
		if (Command == "diver")
		{
			std::priority_queue<std::pair<time_t, string>> qDiver;
			time_t tNow = time(nullptr);
			const int intTDay = 24 * 60 * 60;
			int intSize = grpinfo.nGroupSize;
			for (auto& each : getGroupMemberList(llGroup))
			{
				time_t intLastMsg = (tNow - each.LastMsgTime) / intTDay;
				if (!each.LastMsgTime || intLastMsg > 30)
				{
					qDiver.emplace(intLastMsg, each.Nick + "(" + to_string(each.QQID) + ")");
				}
			}
			if (qDiver.empty())
			{
				reply("{self}δ����Ǳˮ��Ա���Ա�б����ʧ�ܣ�");
				return 1;
			}
			int intCnt(0);
			ResList res;
			while (!qDiver.empty())
			{
				res << qDiver.top().second + to_string(qDiver.top().first) + "��";
				if (++intCnt > 15 && intCnt > intSize / 80)break;
				qDiver.pop();
			}
			reply("Ǳˮ��Ա�б�:" + res.show());
			return 1;
		}
		if (int intPms = getGroupMemberInfo(llGroup, fromQQ).permissions; Command == "pause")
		{
			if (intPms < 2 && trusted < 4)
			{
				reply(GlobalMsg["strPermissionDeniedErr"]);
				return 1;
			}
			if (setGroupWholeBan(llGroup))reply(GlobalMsg["strGroupWholeBanErr"]);
			else reply(GlobalMsg["strGroupWholeBan"]);
			return 1;
		}
		else
		{
			if (Command == "restart")
			{
				if (intPms < 2 && trusted < 4)
				{
					reply(GlobalMsg["strPermissionDeniedErr"]);
					return 1;
				}
				if (!setGroupWholeBan(llGroup, 0))reply(GlobalMsg["strGroupWholeUnban"]);
				return 1;
			}
			if (Command == "card")
			{
				if (long long llqq = readID())
				{
					if (trusted < 4 && intPms < 2 && llqq != fromQQ)
					{
						reply(GlobalMsg["strPermissionDeniedErr"]);
						return 1;
					}
					if (getGroupMemberInfo(llGroup, console.DiceMaid).permissions < 2)
					{
						reply(GlobalMsg["strSelfPermissionErr"]);
						return 1;
					}
					while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
						intMsgCnt++;
					while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
					strVar["card"] = readRest();
					strVar["target"] = getName(llqq, llGroup);
					if (setGroupCard(llGroup, llqq, strVar["card"]))
					{
						reply(GlobalMsg["strGroupCardSetErr"]);
					}
					else
					{
						reply(GlobalMsg["strGroupCardSet"]);
					}
				}
				else
				{
					reply(GlobalMsg["strQQIDEmpty"]);
				}
				return 1;
			}
			if ((intPms < 2 && (getGroupMemberInfo(llGroup, console.DiceMaid).permissions < 3 || trusted < 5)))
			{
				reply(GlobalMsg["strPermissionDeniedErr"]);
				return 1;
			}
			if (Command == "ban")
			{
				if (trusted < 4)
				{
					reply(GlobalMsg["strNotAdmin"]);
					return -1;
				}
				if (getGroupMemberInfo(llGroup, console.DiceMaid).permissions < 2)
				{
					reply(GlobalMsg["strSelfPermissionErr"]);
					return 1;
				}
				string QQNum = readDigit();
				if (QQNum.empty())
				{
					reply(GlobalMsg["strQQIDEmpty"]);
					return -1;
				}
				long long llMemberQQ = stoll(QQNum);
				GroupMemberInfo Member = getGroupMemberInfo(llGroup, llMemberQQ);
				if (Member.QQID == llMemberQQ)
				{
					strVar["member"] = getName(Member.QQID, llGroup);
					if (Member.permissions > 1)
					{
						reply(GlobalMsg["strSelfPermissionErr"]);
						return 1;
					}
					string strMainDice = readDice();
					if (strMainDice.empty())
					{
						reply(GlobalMsg["strValueErr"]);
						return -1;
					}
					const int intDefaultDice = get(getUser(fromQQ).intConf, string("Ĭ����"), 100);
					RD rdMainDice(strMainDice, intDefaultDice);
					rdMainDice.Roll();
					long long intDuration = rdMainDice.intTotal;
					strVar["res"] = rdMainDice.FormCompleteString();
					if (setGroupBan(llGroup, llMemberQQ, intDuration * 60) == 0)
						if (intDuration <= 0)
							reply(GlobalMsg["strGroupUnban"]);
						else reply(GlobalMsg["strGroupBan"]);
					else reply(GlobalMsg["strGroupBanErr"]);
				}
				else reply("{self}���޴�ȺԱ��");
			}
			else if (Command == "kick")
			{
				if (trusted < 4)
				{
					reply(GlobalMsg["strNotAdmin"]);
					return -1;
				}
				if (getGroupMemberInfo(llGroup, console.DiceMaid).permissions < 2)
				{
					reply(GlobalMsg["strSelfPermissionErr"]);
					return 1;
				}
				long long llMemberQQ = readID();
				if (!llMemberQQ)
				{
					reply(GlobalMsg["strQQIDEmpty"]);
					return -1;
				}
				ResList resKicked, resDenied, resNotFound;
				GroupMemberInfo Member;
				do
				{
					Member = getGroupMemberInfo(llGroup, llMemberQQ);
					if (Member.QQID == llMemberQQ)
					{
						if (Member.permissions > 1)
						{
							resDenied << Member.Nick + "(" + to_string(Member.QQID) + ")";
							continue;
						}
						if (setGroupKick(llGroup, llMemberQQ, false) == 0)
						{
							resKicked << Member.Nick + "(" + to_string(Member.QQID) + ")";
						}
						else resDenied << Member.Nick + "(" + to_string(Member.QQID) + ")";
					}
					else resNotFound << to_string(llMemberQQ);
				}
				while ((llMemberQQ = readID()));
				strReply = GlobalMsg["strSelfName"];
				if (!resKicked.empty())strReply += "���Ƴ�ȺԱ��" + resKicked.show() + "\n";
				if (!resDenied.empty())strReply += "�Ƴ�ʧ�ܣ�" + resDenied.show() + "\n";
				if (!resNotFound.empty())strReply += "�Ҳ�������" + resNotFound.show();
				reply();
				return 1;
			}
			else if (Command == "title")
			{
				if (getGroupMemberInfo(llGroup, console.DiceMaid).permissions < 3)
				{
					reply(GlobalMsg["strSelfPermissionErr"]);
					return 1;
				}
				if (long long llqq = readID())
				{
					while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
						intMsgCnt++;
					while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
					strVar["title"] = readRest();
					if (setGroupSpecialTitle(llGroup, llqq, strVar["title"]))
					{
						reply(GlobalMsg["strGroupTitleSetErr"]);
					}
					else
					{
						strVar["target"] = getName(llqq, llGroup);
						reply(GlobalMsg["strGroupTitleSet"]);
					}
				}
				else
				{
					reply(GlobalMsg["strQQIDEmpty"]);
				}
				return 1;
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "reply")
	{
		intMsgCnt += 5;
		if (trusted < 4)
		{
			reply(GlobalMsg["strNotAdmin"]);
			return -1;
		}
		strVar["key"] = readUntilSpace();
		vector<string>* Deck = nullptr;
		if (strVar["key"].empty())
		{
			reply(GlobalMsg["strParaEmpty"]);
			return -1;
		}
		CardDeck::mReplyDeck[strVar["key"]] = {};
		Deck = &CardDeck::mReplyDeck[strVar["key"]];
		while (intMsgCnt != strMsg.length())
		{
			string item = readItem();
			if (!item.empty())Deck->push_back(item);
		}
		if (Deck->empty())
		{
			reply(GlobalMsg["strReplyDel"], {strVar["key"]});
			CardDeck::mReplyDeck.erase(strVar["key"]);
		}
		else reply(GlobalMsg["strReplySet"], {strVar["key"]});
		saveJMap(DiceDir + "\\conf\\CustomReply.json", CardDeck::mReplyDeck);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "set")
		{
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == ':')
				intMsgCnt++;
			string strDefaultRule = strMsg.substr(intMsgCnt);
			if (strDefaultRule.empty())
			{
				getUser(fromQQ).rmStrConf("Ĭ�Ϲ���");
				reply(GlobalMsg["strRuleReset"]);
			}
			else
			{
				for (auto& n : strDefaultRule)
					n = toupper(static_cast<unsigned char>(n));
				getUser(fromQQ).setConf("Ĭ�Ϲ���", strDefaultRule);
				reply(GlobalMsg["strRuleSet"]);
			}
		}
		else
		{
			string strSearch = strMsg.substr(intMsgCnt);
			for (auto& n : strSearch)
				n = toupper(static_cast<unsigned char>(n));
			string strReturn;
			if (getUser(fromQQ).strConf.count("Ĭ�Ϲ���") && strSearch.find(':') == string::npos && 
				GetRule::get(getUser(fromQQ).strConf["Ĭ�Ϲ���"], strSearch, strReturn))
			{
				reply(strReturn);
			}
			else if (GetRule::analyze(strSearch, strReturn))
			{
				reply(strReturn);
			}
			else
			{
				reply(GlobalMsg["strRuleErr"] + strReturn);
			}
		}
		return 1;
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
			reply(GlobalMsg["strCharacterTooBig"]);
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			reply(GlobalMsg["strCharacterTooBig"]);
			return 1;
		}
		if (intNum == 0)
		{
			reply(GlobalMsg["strCharacterCannotBeZero"]);
			return 1;
		}
		strReply = strVar["nick"];
		COC6(strReply, intNum);
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "deck")
	{
		if (trusted < 4 && console["DisabledDeck"])
		{
			reply(GlobalMsg["strDisabledDeckGlobal"]);
			return 1;
		}
		intMsgCnt += 4;
		readSkipSpace();
		string strPara = readPara();
		vector<string> *DeckPro = nullptr, *DeckTmp = nullptr;
		if (intT != PrivateT && CardDeck::mGroupDeck.count(fromGroup))
		{
			DeckPro = &CardDeck::mGroupDeck[fromGroup];
			DeckTmp = &CardDeck::mGroupDeckTmp[fromGroup];
		}
		else
		{
			if (CardDeck::mPrivateDeck.count(fromQQ))
			{
				DeckPro = &CardDeck::mPrivateDeck[fromQQ];
				DeckTmp = &CardDeck::mPrivateDeckTmp[fromQQ];
			}
			//haichayixiangpanding
		}
		if (strPara == "show")
		{
			if (!DeckTmp)
			{
				reply(GlobalMsg["strDeckTmpNotFound"]);
				return 1;
			}
			if (DeckTmp->empty())
			{
				reply(GlobalMsg["strDeckTmpEmpty"]);
				return 1;
			}
			string strReply = GlobalMsg["strDeckTmpShow"] + "\n";
			for (const auto& it : *DeckTmp)
			{
				it.length() > 10 ? strReply += it + "\n" : strReply += it + "|";
			}
			strReply.erase(strReply.end() - 1);
			reply(strReply);
			return 1;
		}
		if (!intT && !isAuth && !trusted)
		{
			reply(GlobalMsg["strPermissionDeniedErr"]);
			return 1;
		}
		if (strPara == "set")
		{
			strVar["deck_name"] = readAttrName();
			if (strVar["deck_name"].empty())strVar["deck_name"] = readDigit();
			if (strVar["deck_name"].empty())
			{
				reply(GlobalMsg["strDeckNameEmpty"]);
				return 1;
			}
			vector<string> DeckSet = {};
			if ((strVar["deck_name"] == "Ⱥ��Ա" || strVar["deck_name"] == "member") && intT == GroupT)
			{
				vector<GroupMemberInfo> list = getGroupMemberList(fromGroup);
				for (auto& each : list)
				{
					DeckSet.push_back(
						(each.GroupNick.empty() ? each.Nick : each.GroupNick) + "(" + to_string(each.QQID) + ")");
				}
				CardDeck::mGroupDeck[fromGroup] = DeckSet;
				CardDeck::mGroupDeckTmp.erase(fromGroup);
				reply(GlobalMsg["strDeckProSet"], {strVar["deck_name"]});
				return 1;
			}
			switch (CardDeck::findDeck(strVar["deck_name"]))
			{
			case 1:
				if (strVar["deck_name"][0] == '_')
					reply(GlobalMsg["strDeckNotFound"]);
				else
					DeckSet = CardDeck::mPublicDeck[strVar["deck_name"]];
				break;
			case 2: 
				{
					int intSize = stoi(strVar["deck_name"]) + 1;
					if (intSize == 0) 
					{
						reply(GlobalMsg["strNumCannotBeZero"]);
						return 1;
					}
					strVar["deck_name"] = "����1��" + strVar["deck_name"];
					while (--intSize) {
						DeckSet.push_back(to_string(intSize));
					}
					break;
				}
			case 0:
			default:
				reply(GlobalMsg["strDeckNotFound"]);
				return 1;
			}
			if (intT == PrivateT)
			{
				CardDeck::mPrivateDeck[fromQQ] = DeckSet;
			}
			else
			{
				CardDeck::mGroupDeck[fromGroup] = DeckSet;
			}
			reply(GlobalMsg["strDeckProSet"], { strVar["deck_name"] });
			return 1;
		}
		if (strPara == "reset")
		{
			*DeckTmp = vector<string>(*DeckPro);
			reply(GlobalMsg["strDeckTmpReset"]);
			return 1;
		}
		if (strPara == "clr")
		{
			if (intT == PrivateT)
			{
				if (CardDeck::mPrivateDeck.count(fromQQ) == 0)
				{
					reply(GlobalMsg["strDeckProNull"]);
					return 1;
				}
				CardDeck::mPrivateDeck.erase(fromQQ);
				if (DeckTmp)DeckTmp->clear();
				reply(GlobalMsg["strDeckProClr"]);
			}
			else
			{
				if (CardDeck::mGroupDeck.count(fromGroup) == 0)
				{
					reply(GlobalMsg["strDeckProNull"]);
					return 1;
				}
				CardDeck::mGroupDeck.erase(fromGroup);
				if (DeckTmp)DeckTmp->clear();
				reply(GlobalMsg["strDeckProClr"]);
			}
			return 1;
		}
		if (strPara == "new")
		{
			if (intT != PrivateT && groupset(fromGroup, "���ʹ��") == 0)
			{
				reply(GlobalMsg["strWhiteGroupDenied"]);
				return 1;
			}
			if (intT == PrivateT && trustedQQ(fromQQ) == 0)
			{
				reply(GlobalMsg["strWhiteQQDenied"]);
				return 1;
			}
			if (intT == PrivateT)
			{
				CardDeck::mPrivateDeck[fromQQ] = {};
				DeckPro = &CardDeck::mPrivateDeck[fromQQ];
			}
			else
			{
				CardDeck::mGroupDeck[fromGroup] = {};
				DeckPro = &CardDeck::mGroupDeck[fromGroup];
			}
			while (intMsgCnt != strMsg.length())
			{
				string item = readItem();
				if (!item.empty())DeckPro->push_back(item);
			}
			reply(GlobalMsg["strDeckProNew"]);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw")
	{
		if (trusted < 4 && console["DisabledDraw"])
		{
			reply(GlobalMsg["strDisabledDrawGlobal"]);
			return 1;
		}
		strVar["option"] = "����draw";
		if (intT && groupset(fromGroup, strVar["option"]) > 0)
		{
			reply(GlobalMsg["strGroupSetOnAlready"]);
			return 1;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		vector<string> ProDeck;
		vector<string>* TempDeck = nullptr;
		strVar["deck_name"] = readPara();
		if (strVar["deck_name"].empty())
		{
			if (intT != PrivateT && CardDeck::mGroupDeck.count(fromGroup))
			{
				if (CardDeck::mGroupDeckTmp.count(fromGroup) == 0 || CardDeck::mGroupDeckTmp[fromGroup].empty())
					CardDeck::mGroupDeckTmp[fromGroup] = vector<string>(CardDeck::mGroupDeck[fromGroup]);
				TempDeck = &CardDeck::mGroupDeckTmp[fromGroup];
			}
			else if (CardDeck::mPrivateDeck.count(fromQQ))
			{
				if (CardDeck::mPrivateDeckTmp.count(fromQQ) == 0 || CardDeck::mPrivateDeckTmp[fromQQ].empty())
					CardDeck::mPrivateDeckTmp[fromQQ] = vector<string>(CardDeck::mPrivateDeck[fromQQ]);
				TempDeck = &CardDeck::mPrivateDeckTmp[fromQQ];
			}
			else
			{
				reply(GlobalMsg["strDeckNameEmpty"]);
				return 1;
			}
		}
		else
		{
			//int intFoundRes = CardDeck::findDeck(strVar["deck_name"]);
			if (strVar["deck_name"][0] == '_' || CardDeck::findDeck(strVar["deck_name"]) == 0)
			{
				strReply = GlobalMsg["strDeckNotFound"];
				reply(strReply);
				return 1;
			}
			ProDeck = CardDeck::mPublicDeck[strVar["deck_name"]];
			TempDeck = &ProDeck;
		}
		int intCardNum = 1;
		switch (readNum(intCardNum))
		{
		case 0:
			if (intCardNum == 0)
			{
				reply(GlobalMsg["strNumCannotBeZero"]);
				return 1;
			}
			break;
		case -1: break;
		case -2:
			reply(GlobalMsg["strParaIllegal"]);
			console.log("����:" + printQQ(fromQQ) + "��" + GlobalMsg["strSelfName"] + "ʹ���˷Ƿ�ָ�����\n" + strMsg, 1,
			            printSTNow());
			return 1;
		}
		ResList Res;
		while (intCardNum--)
		{
			Res << CardDeck::drawCard(*TempDeck);
			if (TempDeck->empty())break;
		}
		strVar["res"] = Res.dot("|").show();
		reply(GlobalMsg["strDrawCard"], {strVar["pc"], strVar["res"]});
		if (intCardNum > 0)
		{
			reply(GlobalMsg["strDeckEmpty"]);
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init" && intT)
	{
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (gm->session(fromGroup).table_clr("�ȹ�"))
				reply("�ɹ�����ȹ���¼��");
			else
				reply("�б�Ϊ�գ�");
			return 1;
		}
		strVar["table_name"] = "�ȹ�";
		if (gm->session(fromGroup).table_count("�ȹ�"))
			reply(GlobalMsg["strGMTableShow"] + gm->session(fromGroup).table_prior_show("�ȹ�"));
		else reply(GlobalMsg["strGMTableNotExist"]);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		if (console["DisabledJrrp"])
		{
			reply("&strDisabledJrrpGlobal");
			return 1;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
		if (intT == GroupT)
		{
			if (Command == "on")
			{
				if (isAuth)
				{
					if (groupset(fromGroup, "����jrrp") > 0)
					{
						chat(fromGroup).reset("����jrrp");
						reply("�ɹ��ڱ�Ⱥ������JRRP!");
					}
					else
					{
						reply("�ڱ�Ⱥ��JRRPû�б�����!");
					}
				}
				else
				{
					reply(GlobalMsg["strPermissionDeniedErr"]);
				}
				return 1;
			}
			if (Command == "off")
			{
				if (getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
				{
					if (groupset(fromGroup, "����jrrp") < 1)
					{
						chat(fromGroup).set("����jrrp");
						reply("�ɹ��ڱ�Ⱥ�н���JRRP!");
					}
					else
					{
						reply("�ڱ�Ⱥ��JRRPû�б�����!");
					}
				}
				else
				{
					reply(GlobalMsg["strPermissionDeniedErr"]);
				}
				return 1;
			}
			if (groupset(fromGroup, "����jrrp") > 0)
			{
				reply("�ڱ�Ⱥ��JRRP�����ѱ�����");
				return 1;
			}
		}
		else if (intT == DiscussT)
		{
			if (Command == "on")
			{
				if (groupset(fromGroup, "����jrrp") > 0)
				{
					chat(fromGroup).reset("����jrrp");
					reply("�ɹ��ڴ˶�������������JRRP!");
				}
				else
				{
					reply("�ڴ˶���������JRRPû�б�����!");
				}
				return 1;
			}
			if (Command == "off")
			{
				if (groupset(fromGroup, "����jrrp") < 1)
				{
					chat(fromGroup).set("����jrrp");
					reply("�ɹ��ڴ˶��������н���JRRP!");
				}
				else
				{
					reply("�ڴ˶���������JRRPû�б�����!");
				}
				return 1;
			}
			if (groupset(fromGroup, "����jrrp") > 0)
			{
				reply("�ڴ˶���������JRRP�ѱ�����!");
				return 1;
			}
		}
		string data = "QQ=" + to_string(getLoginQQ()) + "&v=20190114" + "&QueryQQ=" + to_string(fromQQ);
		char* frmdata = new char[data.length() + 1];
		strcpy_s(frmdata, data.length() + 1, data.c_str());
		bool res = Network::POST("api.kokona.tech", "/jrrp", 5555, frmdata, strVar["res"]);
		delete[] frmdata;
		if (res)
		{
			reply(GlobalMsg["strJrrp"], {strVar["nick"], strVar["res"]});
		}
		else
		{
			reply(GlobalMsg["strJrrpErr"], {strVar["res"]});
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "link")
	{
		intMsgCnt += 4;
		if (trusted < 3)
		{
			reply(GlobalMsg["strNotAdmin"]);
			return true;
		}
		isLinkOrder = true;
		string strOption = readPara();
		if (strOption == "close")
		{
			if (mLinkedList.count(fromChat))
			{
				chatType ToChat = mLinkedList[fromChat];
				mLinkedList.erase(fromChat);
				auto Range = mFwdList.equal_range(fromChat);
				for (auto it = Range.first; it != Range.second;)
				{
					if (it->second == ToChat)
					{
						it = mFwdList.erase(it);
					}
					else
					{
						++it;
					}
				}
				Range = mFwdList.equal_range(ToChat);
				for (auto it = Range.first; it != Range.second;)
				{
					if (it->second == fromChat)
					{
						it = mFwdList.erase(it);
					}
					else
					{
						++it;
					}
				}
				reply(GlobalMsg["strLinkLoss"]);
				return 1;
			}
			return 1;
		}
		string strType = readPara();
		chatType ToChat;
		string strID = readDigit();
		if (strID.empty())
		{
			reply(GlobalMsg["strLinkNotFound"]);
			return 1;
		}
		ToChat.first = stoll(strID);
		if (strType == "qq")
		{
			ToChat.second = msgtype::Private;
		}
		else if (strType == "group")
		{
			ToChat.second = msgtype::Group;
		}
		else if (strType == "discuss")
		{
			ToChat.second = msgtype::Discuss;
		}
		else
		{
			reply(GlobalMsg["strLinkNotFound"]);
			return 1;
		}
		if (mLinkedList.count(fromChat) && mFwdList.count(mLinkedList[fromChat]))
		{
			mFwdList.erase(mLinkedList[fromChat]);
		}
		if (strOption == "with")
		{
			mLinkedList[fromChat] = ToChat;
			mFwdList.insert({fromChat, ToChat});
			mFwdList.insert({ToChat, fromChat});
		}
		else if (strOption == "from")
		{
			mLinkedList[fromChat] = ToChat;
			mFwdList.insert({ToChat, fromChat});
		}
		else if (strOption == "to")
		{
			mLinkedList[fromChat] = ToChat;
			mFwdList.insert({fromChat, ToChat});
		}
		else return 1;
		if (ChatList.count(ToChat.first) || UserList.count(ToChat.first))reply(GlobalMsg["strLinked"]);
		else reply(GlobalMsg["strLinkWarning"]);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
	{
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;

		string type = readPara();
		string strNum = readDigit();
		if (strNum.length() > 1 && strNum != "10")
		{
			reply(GlobalMsg["strNameNumTooBig"]);
			return 1;
		}
		int intNum = strNum.empty() ? 1 : stoi(strNum);
		if (intNum == 0)
		{
			reply(GlobalMsg["strNameNumCannotBeZero"]);
			return 1;
		}
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("�������_" + type)) ? "�������_" + type : "�������";
		vector<string> TempDeck(CardDeck::mPublicDeck[strDeckName]);
		ResList Res;
		while (intNum--)
		{
			Res << CardDeck::drawCard(TempDeck, true);
		}
		strVar["res"] = Res.dot("��").show();
		reply(GlobalMsg["strNameGenerator"]);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "send")
	{
		intMsgCnt += 4;
		readSkipSpace();
		//�ȿ���Master��������ָ��Ŀ�귢��
		if (trusted > 2)
		{
			chatType ct;
			if (!readChat(ct, true))
			{
				readSkipColon();
				string strFwd(readRest());
				if (strFwd.empty())
				{
					reply(GlobalMsg["strSendMsgEmpty"]);
				}
				else
				{
					AddMsgToQueue(strFwd, ct);
					reply(GlobalMsg["strSendMsg"]);
				}
				return 1;
			}
			readSkipColon();
		}
		else if (!console)
		{
			reply(GlobalMsg["strSendMsgInvalid"]);
			return 1;
		}
		else if (console["DisabledSend"] && trusted < 3)
		{
			reply(GlobalMsg["strDisabledSendGlobal"]);
			return 1;
		}
		string strInfo = readRest();
		if (strInfo.empty())
		{
			reply(GlobalMsg["strSendMsgEmpty"]);
			return 1;
		}
		string strFwd = ((trusted > 4) ? "| " : ("| " + printFrom())) + strInfo;
		console.log(strFwd, 0b100, printSTNow());
		reply(GlobalMsg["strSendMasterMsg"]);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "user")
	{
		intMsgCnt += 4;
		string strOption = readPara();
		if (strOption.empty())return 0;
		if (strOption == "state")
		{
			User& user = getUser(fromQQ);
			strVar["user"] = printQQ(fromQQ);
			ResList rep;
			rep << "���μ���" + to_string(trusted)
				<< "��{nick}�ĵ�һӡ���Լ����" + printDate(user.tCreated)
				<< (!(user.strNick.empty()) ? "����¼{nick}��" + to_string(user.strNick.size()) + "���ƺ�" : "û�м�¼{nick}�ĳƺ�")
				<< ((PList.count(fromQQ)) ? "������{nick}��" + to_string(PList[fromQQ].size()) + "�Ž�ɫ��" : "�޽�ɫ����¼")
				<< user.show();
			reply("{user}" + rep.show());
			return 1;
		}
		if (strOption == "trust")
		{
			if (trusted < 4 && fromQQ != console.master())
			{
				reply(GlobalMsg["strNotAdmin"]);
				return 1;
			}
			string strTarget = readDigit();
			if (strTarget.empty())
			{
				reply(GlobalMsg["strQQIDEmpty"]);
				return 1;
			}
			long long llTarget = stoll(strTarget);
			if (trustedQQ(llTarget) >= trusted && fromQQ != console.master())
			{
				reply(GlobalMsg["strUserTrustDenied"]);
				return 1;
			}
			strVar["user"] = printQQ(llTarget);
			strVar["trust"] = readDigit();
			if (strVar["trust"].empty())
			{
				if (!UserList.count(llTarget))
				{
					reply(GlobalMsg["strUserNotFound"]);
					return 1;
				}
				strVar["trust"] = to_string(trustedQQ(llTarget));
				reply(GlobalMsg["strUserTrustShow"]);
				return 1;
			}
			User& user = getUser(llTarget);
			if (short intTrust = stoi(strVar["trust"]); intTrust < 0 || intTrust > 255 || (intTrust >= trusted && fromQQ
				!= console.master()))
			{
				reply(GlobalMsg["strUserTrustIllegal"]);
				return 1;
			}
			else
			{
				user.trust(intTrust);
			}
			reply(GlobalMsg["strUserTrusted"]);
			return 1;
		}
		if (strOption == "kill")
		{
			if (trusted < 4 && fromQQ != console.master())
			{
				reply(GlobalMsg["strNotAdmin"]);
				return 1;
			}
			long long llTarget = readID();
			if (trustedQQ(llTarget) >= trusted && fromQQ != console.master())
			{
				reply(GlobalMsg["strUserTrustDenied"]);
				return 1;
			}
			strVar["user"] = printQQ(llTarget);
			if (!llTarget || !UserList.count(llTarget))
			{
				reply(GlobalMsg["strUserNotFound"]);
				return 1;
			}
			UserList.erase(llTarget);
			reply("��Ĩ��{user}���û���¼");
			return 1;
		}
		if (strOption == "clr")
		{
			if (trusted < 5)
			{
				reply(GlobalMsg["strNotMaster"]);
				return 1;
			}
			int cnt = clearUser();
			note("��������Ч�û���¼" + to_string(cnt) + "��", 0b10);
			return 1;
		}
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
		if (strNum.length() > 1 && strNum != "10")
		{
			reply(GlobalMsg["strCharacterTooBig"]);
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0)
		{
			reply(GlobalMsg["strCharacterCannotBeZero"]);
			return 1;
		}
		string strReply = strVar["pc"];
		COC7(strReply, intNum);
		reply(strReply);
		return 1;
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
		if (strNum.length() > 1 && strNum != "10")
		{
			reply(GlobalMsg["strCharacterTooBig"]);
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0)
		{
			reply(GlobalMsg["strCharacterCannotBeZero"]);
			return 1;
		}
		string strReply = strVar["pc"];
		DND(strReply, intNum);
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string type = readPara();
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("�������_" + type)) ? "�������_" + type : "�������";
		strVar["new_nick"] = strip(CardDeck::drawCard(CardDeck::mPublicDeck[strDeckName], true));
		getUser(fromQQ).setNick(fromGroup, strVar["new_nick"]);
		const string strReply = format(GlobalMsg["strNameSet"], {strVar["nick"], strVar["new_nick"]});
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(' ', intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(static_cast<unsigned char>(charNumElement)))
			{
				reply(GlobalMsg["strSetInvalid"]);
				return 1;
			}
		if (strDefaultDice.length() > 4)
		{
			reply(GlobalMsg["strSetTooBig"]);
			return 1;
		}
		const int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			getUser(fromQQ).rmIntConf("Ĭ����");
		else
			getUser(fromQQ).setConf("Ĭ����", intDefaultDice);
		const string strSetSuccessReply = "�ѽ�" + strVar["pc"] + "��Ĭ�������͸���ΪD" + strDefaultDice;
		reply(strSetSuccessReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "str" && trusted > 3)
	{
		string strName;
		while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length()
		)
		{
			strName += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		while (strMsg[intMsgCnt] == ' ')intMsgCnt++;
		if (intMsgCnt == strMsg.length() || strMsg.substr(intMsgCnt) == "show")
		{
			AddMsgToQueue(GlobalMsg[strName], fromChat);
			return 1;
		}
		string strMessage = strMsg.substr(intMsgCnt);
		if (strMessage == "reset")
		{
			EditedMsg.erase(strName);
			GlobalMsg[strName] = "";
			note("�����" + strName + "���Զ��壬�����´�������ָ�Ĭ�����á�", 0b1);
		}
		else
		{
			if (strMessage == "NULL")strMessage = "";
			EditedMsg[strName] = strMessage;
			GlobalMsg[strName] = strMessage;
			note("���Զ���" + strName + "���ı�", 0b1);
		}
		saveJMap(DiceDir + "\\conf\\CustomMsg.json", EditedMsg);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		strVar["attr"] = readAttrName();
		string strCurrentValue = readDigit(false);
		short nCurrentVal;
		short* pVal = &nCurrentVal;
		//��ȡ����ԭֵ
		if (strCurrentValue.empty())
		{
			if (PList.count(fromQQ) && !strVar["attr"].empty() && (getPlayer(fromQQ)[fromGroup].stored(strVar["attr"])))
			{
				pVal = &getPlayer(fromQQ)[fromGroup][strVar["attr"]];
			}
			else
			{
				reply(GlobalMsg["strEnValEmpty"]);
				return 1;
			}
		}
		else
		{
			if (strCurrentValue.length() > 3)
			{
				reply(GlobalMsg["strEnValInvalid"]);
				return 1;
			}
			nCurrentVal = stoi(strCurrentValue);
		}
		readSkipSpace();
		//�ɱ�ɳ�ֵ���ʽ
		string strEnChange;
		string strEnFail;
		string strEnSuc = "1D10";
		//�ԼӼ�������ͷȷ���뼼��ֵ������
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strEnChange = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
			//û��'/'ʱĬ�ϳɹ��仯ֵ
			if (strEnChange.find('/') != std::string::npos)
			{
				strEnFail = strEnChange.substr(0, strEnChange.find('/'));
				strEnSuc = strEnChange.substr(strEnChange.find('/') + 1);
			}
			else strEnSuc = strEnChange;
		}
		if (strVar["attr"].empty())strVar["attr"] = GlobalMsg["strEnDefaultName"];
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strVar["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(*pVal) + " ";

		if (intTmpRollRes <= *pVal && intTmpRollRes <= 95)
		{
			if (strEnFail.empty())
			{
				strVar["res"] += GlobalMsg["strFailure"];
				reply(GlobalMsg["strEnRollNotChange"]);
				return 1;
			}
			strVar["res"] += GlobalMsg["strFailure"];
			RD rdEnFail(strEnFail);
			if (rdEnFail.Roll())
			{
				reply(GlobalMsg["strValueErr"]);
				return 1;
			}
			*pVal += rdEnFail.intTotal;
			if (*pVal > 32767)*pVal = 32767;
			if (*pVal < -32767)*pVal = -32767;
			strVar["change"] = rdEnFail.FormCompleteString();
			strVar["final"] = to_string(*pVal);
			reply(GlobalMsg["strEnRollFailure"]);
			return 1;
		}
		strVar["res"] += GlobalMsg["strSuccess"];
		RD rdEnSuc(strEnSuc);
		if (rdEnSuc.Roll())
		{
			reply(GlobalMsg["strValueErr"]);
			return 1;
		}
		*pVal += rdEnSuc.intTotal;
		if (*pVal > 32767)*pVal = 32767;
		if (*pVal < -32767)*pVal = -32767;
		strVar["change"] = rdEnSuc.FormCompleteString();
		strVar["final"] = to_string(*pVal);
		reply(GlobalMsg["strEnRollSuccess"]);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strVar["pc"] + "�ķ����-�ܽ�֢״:\n";
		LongInsane(strAns);
		reply(strAns);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		if (trusted < 4 && console["DisabledMe"])
		{
			reply(GlobalMsg["strDisabledMeGlobal"]);
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intT == 0)
		{
			string strGroupID = readDigit();
			if (strGroupID.empty())
			{
				reply(GlobalMsg["strGroupIDEmpty"]);
				return 1;
			}
			const long long llGroupID = stoll(strGroupID);
			if (groupset(llGroupID, "ͣ��ָ��") && trusted < 4)
			{
				reply(GlobalMsg["strDisabledErr"]);
				return 1;
			}
			if (groupset(llGroupID, "����me") && trusted < 5)
			{
				reply(GlobalMsg["strMEDisabledErr"]);
				return 1;
			}
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strAction = strip(readRest());
			if (strAction.empty())
			{
				reply(GlobalMsg["strActionEmpty"]);
				return 1;
			}
			string strReply = getName(fromQQ, llGroupID) + strAction;
			const int intSendRes = sendGroupMsg(llGroupID, strReply);
			if (intSendRes < 0)
			{
				reply(GlobalMsg["strSendErr"]);
			}
			else
			{
				reply(GlobalMsg["strSendSuccess"]);
			}
			return 1;
		}
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (!isAuth && (strAction == "on" || strAction == "off"))
		{
			reply(GlobalMsg["strPermissionDeniedErr"]);
			return 1;
		}
		if (strAction == "off")
		{
			if (groupset(fromGroup, "����me") < 1)
			{
				chat(fromGroup).set("����me");
				reply(GlobalMsg["strMeOff"]);
			}
			else
			{
				reply(GlobalMsg["strMeOffAlready"]);
			}
			return 1;
		}
		if (strAction == "on")
		{
			if (groupset(fromGroup, "����me") > 0)
			{
				chat(fromGroup).reset("����me");
				reply(GlobalMsg["strMeOn"]);
			}
			else
			{
				reply(GlobalMsg["strMeOnAlready"]);
			}
			return 1;
		}
		if (groupset(fromGroup, "����me"))
		{
			reply(GlobalMsg["strMEDisabledErr"]);
			return 1;
		}
		strAction = strip(readRest());
		if (strAction.empty())
		{
			reply(GlobalMsg["strActionEmpty"]);
			return 1;
		}
		trusted > 4 ? reply(strAction) : reply(strVar["pc"] + strAction);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		strVar["new_nick"] = strip(strMsg.substr(intMsgCnt));
		filter_CQcode(strVar["new_nick"]);
		if (strVar["new_nick"].length() > 50)
		{
			reply(GlobalMsg["strNameTooLongErr"]);
			return 1;
		}
		if (!strVar["new_nick"].empty()) {
			getUser(fromQQ).setNick(fromGroup, strVar["new_nick"]);
			reply(GlobalMsg["strNameSet"], {strVar["nick"], strVar["new_nick"]});
		}
		else
		{
			if (getUser(fromQQ).rmNick(fromGroup))
			{
				reply(GlobalMsg["strNameClr"], {strVar["nick"]});
			}
			else
			{
				reply(GlobalMsg["strNameDelEmpty"]);
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob")
	{
		if (intT == PrivateT)
		{
			reply(GlobalMsg["strObPrivate"]);
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string strOption = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);

		if (!isAuth && (strOption == "on" || strOption == "off"))
		{
			reply(GlobalMsg["strPermissionDeniedErr"]);
			return 1;
		}
		strVar["option"] = "����ob";
		if (strOption == "off")
		{
			if (groupset(fromGroup, strVar["option"]) < 1)
			{
				chat(fromGroup).set(strVar["option"]);
				gm->session(fromGroup).clear_ob();
				reply(GlobalMsg["strObOff"]);
			}
			else
			{
				reply(GlobalMsg["strObOffAlready"]);
			}
			return 1;
		}
		if (strOption == "on")
		{
			if (groupset(fromGroup, strVar["option"]) > 0)
			{
				chat(fromGroup).reset(strVar["option"]);
				reply(GlobalMsg["strObOn"]);
			}
			else
			{
				reply(GlobalMsg["strObOnAlready"]);
			}
			return 1;
		}
		if (groupset(fromGroup, strVar["option"]) > 0)
		{
			reply(GlobalMsg["strObOffAlready"]);
			return 1;
		}
		if (strOption == "list")
		{
			gm->session(fromGroup).ob_list(this);
		}
		else if (strOption == "clr")
		{
			if (intT == DiscussT || getGroupMemberInfo(fromGroup, fromQQ).permissions >= 2)
			{
				gm->session(fromGroup).ob_clr(this);
			}
			else
			{
				reply(GlobalMsg["strPermissionDeniedErr"]);
			}
		}
		else if (strOption == "exit")
		{
			gm->session(fromGroup).ob_exit(this);
		}
		else
		{
			gm->session(fromGroup).ob_enter(this);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "pc")
	{
		intMsgCnt += 2;
		string strOption = readPara();
		Player& pl = getPlayer(fromQQ);
		if (strOption == "tag")
		{
			strVar["char"] = readRest();
			switch (pl.changeCard(strVar["char"], fromGroup))
			{
			case 1:
				reply(GlobalMsg["strPcCardReset"]);
				break;
			case 0:
				reply(GlobalMsg["strPcCardSet"]);
				break;
			case -5:
				reply(GlobalMsg["strPcNameNotExist"]);
				break;
			default:
				reply(GlobalMsg["strUnknownErr"]);
				break;
			}
			return 1;
		}
		if (strOption == "show")
		{
			string strName = readRest();
			strVar["char"] = pl.getCard(strName, fromGroup).Name;
			strVar["type"] = pl.getCard(strName, fromGroup).Type;
			strVar["show"] = pl[strVar["char"]].show(true);
			reply(GlobalMsg["strPcCardShow"]);
			return 1;
		}
		if (strOption == "new")
		{
			strVar["char"] = strip(readRest());
			filter_CQcode(strVar["char"]);
			switch (pl.newCard(strVar["char"], fromGroup)) 
			{
			case 0:
				strVar["type"] = pl[fromGroup].Type;
				strVar["show"] = pl[fromGroup].show(true);
				if (strVar["show"].empty())reply(GlobalMsg["strPcNewEmptyCard"]);
				else reply(GlobalMsg["strPcNewCardShow"]);
				break;
			case -1:
				reply(GlobalMsg["strPcCardFull"]);
				break;
			case -2:
				reply(GlobalMsg["strPcTempInvalid"]);
				break;
			case -4:
				reply(GlobalMsg["strPCNameExist"]);
				break;
			case -6:
				reply(GlobalMsg["strPCNameInvalid"]);
				break;
			default:
				reply(GlobalMsg["strUnknownErr"]);
				break;
			}
			return 1;
		}
		if (strOption == "build")
		{
			strVar["char"] = strip(readRest());
			filter_CQcode(strVar["char"]);
			switch (pl.buildCard(strVar["char"], false, fromGroup)) 
			{
			case 0:
				strVar["show"] = pl[strVar["char"]].show(true);
				reply(GlobalMsg["strPcCardBuild"]);
				break;
			case -1:
				reply(GlobalMsg["strPcCardFull"]);
				break;
			case -2:
				reply(GlobalMsg["strPcTempInvalid"]);
				break;
			case -6:
				reply(GlobalMsg["strPCNameInvalid"]);
				break;
			default:
				reply(GlobalMsg["strUnknownErr"]);
				break;
			}
			return 1;
		}
		if (strOption == "list")
		{
			strVar["show"] = pl.listCard();
			reply(GlobalMsg["strPcCardList"]);
			return 1;
		}
		if (strOption == "nn")
		{
			strVar["new_name"] = strip(readRest());
			filter_CQcode(strVar["char"]);
			if (strVar["new_name"].empty()) 
			{
				reply(GlobalMsg["strPCNameEmpty"]);
				return 1;
			}
			strVar["old_name"] = pl[fromGroup].Name;
			switch (pl.renameCard(strVar["old_name"], strVar["new_name"]))
			{
			case 0:
				reply(GlobalMsg["strPcCardRename"]);
				break;
			case -4:
				reply(GlobalMsg["strPCNameExist"]);
				break;
			case -6:
				reply(GlobalMsg["strPCNameInvalid"]);
				break;
			default:
				reply(GlobalMsg["strUnknownErr"]);
				break;
			}
			return 1;
		}
		if (strOption == "del")
		{
			strVar["char"] = strip(readRest());
			switch (pl.removeCard(strVar["char"]))
			{
			case 0:
				reply(GlobalMsg["strPcCardDel"]);
				break;
			case -5:
				reply(GlobalMsg["strPcNameNotExist"]);
				break;
			case -7:
				reply(GlobalMsg["strPcInitDelErr"]);
				break;
			default:
				reply(GlobalMsg["strUnknownErr"]);
				break;
			}
			return 1;
		}
		if (strOption == "redo")
		{
			strVar["char"] = strip(readRest());
			pl.buildCard(strVar["char"], true, fromGroup);
			strVar["show"] = pl[strVar["char"]].show(true);
			reply(GlobalMsg["strPcCardRedo"]);
			return 1;
		}
		if (strOption == "grp")
		{
			strVar["show"] = pl.listMap();
			reply(GlobalMsg["strPcGroupList"]);
			return 1;
		}
		if (strOption == "cpy")
		{
			string strName = strip(readRest());
			filter_CQcode(strName);
			strVar["char1"] = strName.substr(0, strName.find('='));
			strVar["char2"] = (strVar["char1"].length() < strName.length() - 1)
				                  ? strip(strName.substr(strVar["char1"].length() + 1))
				                  : pl[fromGroup].Name;
			switch (pl.copyCard(strVar["char1"], strVar["char2"], fromGroup))
			{
			case 0:
				reply(GlobalMsg["strPcCardCpy"]);
				break;
			case -1:
				reply(GlobalMsg["strPcCardFull"]);
				break;
			case -3:
				reply(GlobalMsg["strPcNameEmpty"]);
				break;
			case -6:
				reply(GlobalMsg["strPcNameInvalid"]);
				break;
			default:
				reply(GlobalMsg["strUnknownErr"]);
				break;
			}
			return 1;
		}
		if (strOption == "clr")
		{
			PList.erase(fromQQ);
			reply(GlobalMsg["strPcClr"]);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra" || strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		readSkipSpace();
		int intRule = intT
			              ? get(chat(fromGroup).intConf, string("rc����"), 0)
			              : get(getUser(fromQQ).intConf, string("rc����"), 0);
		int intTurnCnt = 1;
		if (strMsg.find('#') != string::npos)
		{
			string strTurnCnt = strMsg.substr(intMsgCnt, strMsg.find('#') - intMsgCnt);
			//#�ܷ�ʶ����Ч
			if (strTurnCnt.empty())intMsgCnt++;
			else if ((strTurnCnt.length() == 1 && isdigit(static_cast<unsigned char>(strTurnCnt[0]))) || strTurnCnt ==
				"10")
			{
				intMsgCnt += strTurnCnt.length() + 1;
				intTurnCnt = stoi(strTurnCnt);
			}
		}
		string strMainDice = "D100";
		string strSkillModify;
		//���ѵȼ�
		string strDifficulty;
		int intDifficulty = 1;
		int intSkillModify = 0;
		//����
		int intSkillMultiple = 1;
		//����
		int intSkillDivisor = 1;
		//�Զ��ɹ�
		bool isAutomatic = false;
		if ((strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') && strLowerMessage[intMsgCnt - 1] != ' ')
		{
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			{
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		readSkipSpace();
		if (strMsg.length() == intMsgCnt)
		{
			strVar["attr"] = GlobalMsg["strEnDefaultName"];
			reply(GlobalMsg["strUnknownPropErr"], {strVar["attr"]});
			return 1;
		}
		strVar["attr"] = strMsg.substr(intMsgCnt);
		if (PList.count(fromQQ) && PList[fromQQ][fromGroup].count(strVar["attr"]))intMsgCnt = strMsg.length();
		else strVar["attr"] = readAttrName();
		if (strVar["attr"].find("�Զ��ɹ�") == 0)
		{
			strDifficulty = strVar["attr"].substr(0, 8);
			strVar["attr"] = strVar["attr"].substr(8);
			isAutomatic = true;
		}
		if (strVar["attr"].find("����") == 0 || strVar["attr"].find("����") == 0)
		{
			strDifficulty += strVar["attr"].substr(0, 4);
			intDifficulty = (strVar["attr"].substr(0, 4) == "����") ? 2 : 5;
			strVar["attr"] = strVar["attr"].substr(4);
		}
		if (strLowerMessage[intMsgCnt] == '*' && isdigit(strLowerMessage[intMsgCnt + 1]))
		{
			intMsgCnt++;
			readNum(intSkillMultiple);
		}
		while ((strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') && isdigit(
			strLowerMessage[intMsgCnt + 1]))
		{
			if (!readNum(intSkillModify))strSkillModify = to_signed_string(intSkillModify);
		}
		if (strLowerMessage[intMsgCnt] == '/' && isdigit(strLowerMessage[intMsgCnt + 1]))
		{
			intMsgCnt++;
			readNum(intSkillDivisor);
			if (intSkillDivisor == 0)
			{
				reply(GlobalMsg["strValueErr"]);
				return 1;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' ||
			strLowerMessage[intMsgCnt] ==
			':')
			intMsgCnt++;
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
		strVar["reason"] = readRest();
		int intSkillVal;
		if (strSkillVal.empty())
		{
			if (PList.count(fromQQ) && PList[fromQQ][fromGroup].count(strVar["attr"]))
			{
				intSkillVal = PList[fromQQ][fromGroup].call(strVar["attr"]);
			}
			else
			{
				if (!PList.count(fromQQ) && SkillNameReplace.count(strVar["attr"]))
				{
					strVar["attr"] = SkillNameReplace[strVar["attr"]];
				}
				if (!PList.count(fromQQ) && SkillDefaultVal.count(strVar["attr"]))
				{
					intSkillVal = SkillDefaultVal[strVar["attr"]];
				}
				else
				{
					reply(GlobalMsg["strUnknownPropErr"], {strVar["attr"]});
					return 1;
				}
			}
		}
		else if (strSkillVal.length() > 3)
		{
			reply(GlobalMsg["strPropErr"]);
			return 1;
		}
		else
		{
			intSkillVal = stoi(strSkillVal);
		}
		int intFianlSkillVal = (intSkillVal * intSkillMultiple + intSkillModify) / intSkillDivisor / intDifficulty;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000)
		{
			reply(GlobalMsg["strSuccessRateErr"]);
			return 1;
		}
		RD rdMainDice(strMainDice);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err)
		{
			reply(GlobalMsg["strZeroDiceErr"]);
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			reply(GlobalMsg["strDiceTooBigErr"]);
			return 1;
		}
		strVar["attr"] = strDifficulty + strVar["attr"] + (
			(intSkillMultiple != 1) ? "��" + to_string(intSkillMultiple) : "") + strSkillModify + ((intSkillDivisor != 1)
				                                                                                      ? "/" + to_string(
					                                                                                      intSkillDivisor)
				                                                                                      : "");
		if (strVar["reason"].empty())
		{
			strReply = format(GlobalMsg["strRollSkill"], {strVar["pc"], strVar["attr"]});
		}
		else strReply = format(GlobalMsg["strRollSkillReason"], {strVar["pc"], strVar["attr"], strVar["reason"]});
		ResList Res;
		string strAns;
		if (intTurnCnt == 1)
		{
			rdMainDice.Roll();
			strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
			int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
			switch (intRes)
			{
			case 0: strAns += GlobalMsg["strRollFumble"];
				break;
			case 1: strAns += isAutomatic ? GlobalMsg["strRollRegularSuccess"] : GlobalMsg["strRollFailure"];
				break;
			case 5: strAns += GlobalMsg["strRollCriticalSuccess"];
				break;
			case 4: if (intDifficulty == 1)
				{
					strAns += GlobalMsg["strRollExtremeSuccess"];
					break;
				}
			case 3: if (intDifficulty == 1)
				{
					strAns += GlobalMsg["strRollHardSuccess"];
					break;
				}
			case 2: strAns += GlobalMsg["strRollRegularSuccess"];
				break;
			}
			strReply += strAns;
		}
		else
		{
			Res.dot("\n");
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
				int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
				switch (intRes)
				{
				case 0: strAns += GlobalMsg["strFumble"];
					break;
				case 1: strAns += isAutomatic ? GlobalMsg["strSuccess"] : GlobalMsg["strFailure"];
					break;
				case 5: strAns += GlobalMsg["strCriticalSuccess"];
					break;
				case 4: if (intDifficulty == 1)
					{
						strAns += GlobalMsg["strExtremeSuccess"];
						break;
					}
				case 3: if (intDifficulty == 1)
					{
						strAns += GlobalMsg["strHardSuccess"];
						break;
					}
				case 2: strAns += GlobalMsg["strSuccess"];
					break;
				}
				Res << strAns;
			}
			strReply += Res.show();
		}
		reply();
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri" && intT)
	{
		intMsgCnt += 2;
		readSkipSpace();
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += readDice();
		}
		else if (isRollDice())
		{
			strinit = readDice();
		}
		readSkipSpace();
		string strname = strMsg.substr(intMsgCnt);
		if (strname.empty())
			strname = strVar["pc"];
		else
			strname = strip(strname);
		RD initdice(strinit, 20);
		const int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			reply(GlobalMsg["strValueErr"]);
			return 1;
		}
		if (intFirstTimeRes == Input_Err)
		{
			reply(GlobalMsg["strInputErr"]);
			return 1;
		}
		if (intFirstTimeRes == ZeroDice_Err)
		{
			reply(GlobalMsg["strZeroDiceErr"]);
			return 1;
		}
		if (intFirstTimeRes == ZeroType_Err)
		{
			reply(GlobalMsg["strZeroTypeErr"]);
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err)
		{
			reply(GlobalMsg["strDiceTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == TypeTooBig_Err)
		{
			reply(GlobalMsg["strTypeTooBigErr"]);
			return 1;
		}
		if (intFirstTimeRes == AddDiceVal_Err)
		{
			reply(GlobalMsg["strAddDiceValErr"]);
			return 1;
		}
		if (intFirstTimeRes != 0)
		{
			reply(GlobalMsg["strUnknownErr"]);
			return 1;
		}
		gm->session(fromGroup).table_add("�ȹ�", initdice.intTotal, strname);
		const string strReply = strname + "���ȹ����㣺" + initdice.FormCompleteString();
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		string SanCost = readUntilSpace();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (SanCost.empty() || SanCost.find('/') == string::npos)
		{
			reply(GlobalMsg["strSanCostInvalid"]);
			return 1;
		}
		string attr = "����";
		int intSan = 0;
		auto* pSan = (short*)&intSan;
		if (readNum(intSan))
		{
			if (PList.count(fromQQ) && getPlayer(fromQQ)[fromGroup].count(attr))
			{
				pSan = &getPlayer(fromQQ)[fromGroup][attr];
			}
			else
			{
				reply(GlobalMsg["strSanEmpty"]);
				return 1;
			}
		}
		string strSanCostSuc = SanCost.substr(0, SanCost.find('/'));
		string strSanCostFail = SanCost.substr(SanCost.find('/') + 1);
		for (const auto& character : strSanCostSuc)
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-')
			{
				reply(GlobalMsg["strSanCostInvalid"]);
				return 1;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find('/') + 1))
		{
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-')
			{
				reply(GlobalMsg["strSanCostInvalid"]);
				return 1;
			}
		}
		RD rdSuc(strSanCostSuc);
		RD rdFail(strSanCostFail);
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{
			reply(GlobalMsg["strSanCostInvalid"]);
			return 1;
		}
		if (*pSan == 0)
		{
			reply(GlobalMsg["strSanInvalid"]);
			return 1;
		}
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		strVar["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(*pSan) + " ";
		//���÷���
		int intRule = intT
			              ? get(chat(fromGroup).intConf, string("rc����"), 0)
			              : get(getUser(fromQQ).intConf, string("rc����"), 0);
		switch (RollSuccessLevel(intTmpRollRes, *pSan, intRule))
		{
		case 5:
		case 4:
		case 3:
		case 2:
			strVar["res"] += GlobalMsg["strSuccess"];
			strVar["change"] = rdSuc.FormCompleteString();
			*pSan = max(0, *pSan - rdSuc.intTotal);
			break;
		case 1:
			strVar["res"] += GlobalMsg["strFailure"];
			strVar["change"] = rdFail.FormCompleteString();
			*pSan = max(0, *pSan - rdFail.intTotal);
			break;
		case 0:
			strVar["res"] += GlobalMsg["strFumble"];
			rdFail.Max();
			strVar["change"] = rdFail.strDice + "���ֵ=" + to_string(rdFail.intTotal);
			*pSan = max(0, *pSan - rdFail.intTotal);
			break;
		}
		strVar["final"] = to_string(*pSan);
		reply(GlobalMsg["strSanRollRes"]);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			reply(GlobalMsg["strStErr"]);
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (!PList.count(fromQQ))
			{
				reply(getMsg("strPcNotExistErr"));
				return 1;
			}
			getPlayer(fromQQ)[fromGroup].clear();
			strVar["char"] = getPlayer(fromQQ)[fromGroup].Name;
			reply(GlobalMsg["strPropCleared"], {strVar["char"]});
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "del")
		{
			if (!PList.count(fromQQ))
			{
				reply(getMsg("strPcNotExistErr"));
				return 1;
			}
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			if (strMsg[intMsgCnt] == '&')
			{
				intMsgCnt++;
			}
			strVar["attr"] = readAttrName();
			if (getPlayer(fromQQ)[fromGroup].erase(strVar["attr"]))
			{
				reply(GlobalMsg["strPropDeleted"], {strVar["pc"], strVar["attr"]});
			}
			else
			{
				reply(GlobalMsg["strPropNotFound"], {strVar["attr"]});
			}
			return 1;
		}
		CharaCard& pc = getPlayer(fromQQ)[fromGroup];
		if (strLowerMessage.substr(intMsgCnt, 4) == "show")
		{
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			strVar["attr"] = readAttrName();
			if (strVar["attr"].empty())
			{
				strVar["char"] = pc.Name;
				strVar["type"] = pc.Type;
				strVar["show"] = pc.show(false);
				reply(GlobalMsg["strPropList"]);
				return 1;
			}
			if (pc.show(strVar["attr"], strVar["val"]) > -1)
			{
				reply(format(GlobalMsg["strProp"], {strVar["pc"], strVar["attr"], strVar["val"]}));
			}
			else
			{
				reply(GlobalMsg["strPropNotFound"], {strVar["attr"]});
			}
			return 1;
		}
		bool boolError = false;
		bool isDetail = false;
		bool isModify = false;
		//ѭ��¼��
		while (intMsgCnt != strLowerMessage.length())
		{
			//��ȡ������
			readSkipSpace();
			if (strMsg[intMsgCnt] == '&')
			{
				intMsgCnt++;
				strVar["attr"] = readToColon();
				if (pc.setExp(strVar["attr"], readExp()))
				{
					reply(GlobalMsg["strPcTextTooLong"]);
					return 1;
				}
				continue;
			}
			string strSkillName = readAttrName();
			if (strSkillName.empty())
			{
				strVar["val"] = readDigit();
				continue;
			}
			if (pc.pTemplet->sInfoList.count(strSkillName))
			{
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
					'=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
				if (pc.setInfo(strSkillName, readUntilTab()))
				{
					reply(GlobalMsg["strPcTextTooLong"]);
					return 1;
				}
				continue;
			}
			if (strSkillName == "note")
			{
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
					'=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
				if (pc.setNote(readRest()))
				{
					reply(GlobalMsg["strPcNoteTooLong"]);
					return 1;
				}
				break;
			}
			//��ȡ����ֵ
			readSkipSpace();
			if ((strLowerMessage[intMsgCnt] == '-' || strLowerMessage[intMsgCnt] == '+'))
			{
				isDetail = true;
				isModify = true;
				short& nVal = pc[strSkillName];
				RD Mod((nVal == 0 ? "" : to_string(nVal)) + readDice());
				if (Mod.Roll())
				{
					reply(GlobalMsg["strValueErr"]);
					return 1;
				}
				strReply += "\n" + strSkillName + "��" + Mod.FormCompleteString();
				if (Mod.intTotal < -32767)
				{
					strReply += "��-32767";
					nVal = -32767;
				}
				else if (Mod.intTotal > 32767)
				{
					strReply += "��32767";
					nVal = 32767;
				}
				else nVal = Mod.intTotal;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
					'|')intMsgCnt++;
				continue;
			}
			string strSkillVal = readDigit();
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 5)
			{
				boolError = true;
				break;
			}
			int intSkillVal = std::clamp(stoi(strSkillVal), -32767, 32767);
			//¼��
			pc.set(strSkillName, intSkillVal);
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
				intMsgCnt++;
		}
		if (boolError)
		{
			reply(GlobalMsg["strPropErr"]);
		}
		else if (isModify)
		{
			reply(format(GlobalMsg["strStModify"], {strVar["pc"]}) + strReply);
		}
		else
		{
			reply(GlobalMsg["strSetPropSuccess"]);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strVar["pc"] + "�ķ����-��ʱ֢״:\n";
		TempInsane(strAns);
		reply(strAns);
		return 1;
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
		if (intT == 0)isHidden = false;
		string strMainDice = readDice();
		readSkipSpace();
		strVar["reason"] = strMsg.substr(intMsgCnt);
		int intTurnCnt = 1;
		const int intDefaultDice = get(getUser(fromQQ).intConf, string("Ĭ����"), 100);
		if (strMainDice.find('#') != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find('#'));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find('#') + 1);
			RD rdTurnCnt(strTurnCnt, intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes != 0)
			{
				if (intRdTurnCntRes == Value_Err)
				{
					reply(GlobalMsg["strValueErr"]);
					return 1;
				}
				if (intRdTurnCntRes == Input_Err)
				{
					reply(GlobalMsg["strInputErr"]);
					return 1;
				}
				if (intRdTurnCntRes == ZeroDice_Err)
				{
					reply(GlobalMsg["strZeroDiceErr"]);
					return 1;
				}
				if (intRdTurnCntRes == ZeroType_Err)
				{
					reply(GlobalMsg["strZeroTypeErr"]);
					return 1;
				}
				if (intRdTurnCntRes == DiceTooBig_Err)
				{
					reply(GlobalMsg["strDiceTooBigErr"]);
					return 1;
				}
				if (intRdTurnCntRes == TypeTooBig_Err)
				{
					reply(GlobalMsg["strTypeTooBigErr"]);
					return 1;
				}
				if (intRdTurnCntRes == AddDiceVal_Err)
				{
					reply(GlobalMsg["strAddDiceValErr"]);
					return 1;
				}
				reply(GlobalMsg["strUnknownErr"]);
				return 1;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				reply(GlobalMsg["strRollTimeExceeded"]);
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				reply(GlobalMsg["strRollTimeErr"]);
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find('d') != string::npos)
			{
				string strTurnNotice = strVar["pc"] + "����������: " + rdTurnCnt.FormShortString() + "��";
				if (!isHidden || intT == PrivateT)
				{
					reply(strTurnNotice);
				}
				else
				{
					strTurnNotice = "��" + printChat(fromChat) + "�� " + strTurnNotice;
					AddMsgToQueue(strTurnNotice, fromQQ, msgtype::Private);
					for (auto qq : gm->session(fromGroup).get_ob())
					{
						if (qq != fromQQ)
						{
							AddMsgToQueue(strTurnNotice, qq, msgtype::Private);
						}
					}
				}
			}
		}
		if (strMainDice.empty())
		{
			reply(GlobalMsg["strEmptyWWDiceErr"]);
			return 1;
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
			                                            ? strMainDice.find('+')
			                                            : strMainDice.find('-'));
		strFirstDice = strFirstDice.substr(0, strFirstDice.find('x') < strFirstDice.find('*')
			                                      ? strFirstDice.find('x')
			                                      : strFirstDice.find('*'));
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
		RD rdMainDice(strMainDice, intDefaultDice);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes != 0)
		{
			if (intFirstTimeRes == Value_Err)
			{
				reply(GlobalMsg["strValueErr"]);
				return 1;
			}
			if (intFirstTimeRes == Input_Err)
			{
				reply(GlobalMsg["strInputErr"]);
				return 1;
			}
			if (intFirstTimeRes == ZeroDice_Err)
			{
				reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			}
			if (intFirstTimeRes == ZeroType_Err)
			{
				reply(GlobalMsg["strZeroTypeErr"]);
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err)
			{
				reply(GlobalMsg["strDiceTooBigErr"]);
				return 1;
			}
			if (intFirstTimeRes == TypeTooBig_Err)
			{
				reply(GlobalMsg["strTypeTooBigErr"]);
				return 1;
			}
			if (intFirstTimeRes == AddDiceVal_Err)
			{
				reply(GlobalMsg["strAddDiceValErr"]);
				return 1;
			}
			reply(GlobalMsg["strUnknownErr"]);
			return 1;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			if (strVar["reason"].empty())strReply = GlobalMsg["strRollMuiltDice"];
			else strReply = GlobalMsg["strRollMuiltDiceReason"];
			vector<int> vintExVal;
			strVar["res"] = "{ ";
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strVar["res"] += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strVar["res"] = ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strVar["res"] += " }";
			if (!vintExVal.empty())
			{
				strVar["res"] += ",��ֵ: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strVar["res"] += to_string(*it);
					if (it != vintExVal.cend() - 1)strVar["res"] += ",";
				}
			}
			if (!isHidden || intT == PrivateT)
			{
				reply();
			}
			else
			{
				strReply = format(strReply, GlobalMsg, strVar);
				strReply = "��" + printChat(fromChat) + "�� " + strReply;
				AddMsgToQueue(strReply, fromQQ, msgtype::Private);
				for (auto qq : gm->session(fromGroup).get_ob())
				{
					if (qq != fromQQ)
					{
						AddMsgToQueue(strReply, qq, msgtype::Private);
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
				strVar["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
				if (strVar["reason"].empty())
					strReply = format(GlobalMsg["strRollDice"], {strVar["pc"], strVar["res"]});
				else strReply = format(GlobalMsg["strRollDiceReason"], {strVar["pc"], strVar["res"], strVar["reason"]});
				if (!isHidden || intT == PrivateT)
				{
					reply();
				}
				else
				{
					strReply = format(strReply, GlobalMsg, strVar);
					strReply = "��" + printChat(fromChat) + "�� " + strReply;
					AddMsgToQueue(strReply, fromQQ, msgtype::Private);
					for (auto qq : gm->session(fromGroup).get_ob())
					{
						if (qq != fromQQ)
						{
							AddMsgToQueue(strReply, qq, msgtype::Private);
						}
					}
				}
			}
		}
		if (isHidden)
		{
			reply(GlobalMsg["strRollHidden"], {strVar["pc"]});
		}
		return 1;
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'h')
	{
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (strMsg[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		if (intT == 0)isHidden = false;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		strVar["reason"] = strMsg.substr(intMsgCnt);
		if (PList.count(fromQQ) && getPlayer(fromQQ)[fromGroup].countExp(strMsg.substr(intMsgCnt)))
		{
			strMainDice = getPlayer(fromQQ)[fromGroup].getExp(strVar["reason"]);
		}
		else
		{
			strMainDice = readDice();
			bool isExp = false;
			for (auto ch : strMainDice)
			{
				if (!isdigit(ch))
				{
					isExp = true;
					break;
				}
			}
			if (isExp)strVar["reason"] = readRest();
			else strMainDice.clear();
		}
		int intTurnCnt = 1;
		const int intDefaultDice = get(getUser(fromQQ).intConf, string("Ĭ����"), 100);
		if (strMainDice.find('#') != string::npos)
		{
			strVar["turn"] = strMainDice.substr(0, strMainDice.find('#'));
			if (strVar["turn"].empty())
				strVar["turn"] = "1";
			strMainDice = strMainDice.substr(strMainDice.find('#') + 1);
			RD rdTurnCnt(strVar["turn"], intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			switch (intRdTurnCntRes)
			{
			case 0: break;
			case Value_Err:
				reply(GlobalMsg["strValueErr"]);
				return 1;
			case Input_Err:
				reply(GlobalMsg["strInputErr"]);
				return 1;
			case ZeroDice_Err:
				reply(GlobalMsg["strZeroDiceErr"]);
				return 1;
			case ZeroType_Err:
				reply(GlobalMsg["strZeroTypeErr"]);
				return 1;
			case DiceTooBig_Err:
				reply(GlobalMsg["strDiceTooBigErr"]);
				return 1;
			case TypeTooBig_Err:
				reply(GlobalMsg["strTypeTooBigErr"]);
				return 1;
			case AddDiceVal_Err:
				reply(GlobalMsg["strAddDiceValErr"]);
				return 1;
			default:
				reply(GlobalMsg["strUnknownErr"]);
				return 1;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				reply(GlobalMsg["strRollTimeExceeded"]);
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0)
			{
				reply(GlobalMsg["strRollTimeErr"]);
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strVar["turn"].find('d') != string::npos)
			{
				strVar["turn"] = rdTurnCnt.FormShortString();
				if (!isHidden)
				{
					reply(GlobalMsg["strRollTurn"], {strVar["pc"], strVar["turn"]});
				}
				else
				{
					strReply = format("��" + printChat(fromChat) + "�� " + GlobalMsg["strRollTurn"], GlobalMsg, strVar);
					AddMsgToQueue(strReply, fromQQ, msgtype::Private);
					for (auto qq : gm->session(fromGroup).get_ob())
					{
						if (qq != fromQQ)
						{
							AddMsgToQueue(strReply, qq, msgtype::Private);
						}
					}
				}
			}
		}
		if (strMainDice.empty() && PList.count(fromQQ) && getPlayer(fromQQ)[fromGroup].countExp(strVar["reason"]))
		{
			strMainDice = getPlayer(fromQQ)[fromGroup].getExp(strVar["reason"]);
		}
		RD rdMainDice(strMainDice, intDefaultDice);

		const int intFirstTimeRes = rdMainDice.Roll();
		switch (intFirstTimeRes)
		{
		case 0: break;
		case Value_Err:
			reply(GlobalMsg["strValueErr"]);
			return 1;
		case Input_Err:
			reply(GlobalMsg["strInputErr"]);
			return 1;
		case ZeroDice_Err:
			reply(GlobalMsg["strZeroDiceErr"]);
			return 1;
		case ZeroType_Err:
			reply(GlobalMsg["strZeroTypeErr"]);
			return 1;
		case DiceTooBig_Err:
			reply(GlobalMsg["strDiceTooBigErr"]);
			return 1;
		case TypeTooBig_Err:
			reply(GlobalMsg["strTypeTooBigErr"]);
			return 1;
		case AddDiceVal_Err:
			reply(GlobalMsg["strAddDiceValErr"]);
			return 1;
		default:
			reply(GlobalMsg["strUnknownErr"]);
			return 1;
		}
		strVar["dice_exp"] = rdMainDice.strDice;
		string strType = (intTurnCnt != 1
			                  ? (strVar["reason"].empty() ? "strRollMultiDice" : "strRollMultiDiceReason")
			                  : (strVar["reason"].empty() ? "strRollDice" : "strRollDiceReason"));
		if (!boolDetail && intTurnCnt != 1)
		{
			strReply = GlobalMsg[strType];
			vector<int> vintExVal;
			strVar["res"] = "{ ";
			while (intTurnCnt--)
			{
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strVar["res"] += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strVar["res"] += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
					rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strVar["res"] += " }";
			if (!vintExVal.empty())
			{
				strVar["res"] += ",��ֵ: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it)
				{
					strVar["res"] += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strVar["res"] += ",";
				}
			}
			if (!isHidden)
			{
				reply();
			}
			else
			{
				strReply = format(strReply, GlobalMsg, strVar);
				strReply = "��" + printChat(fromChat) + "�� " + strReply;
				AddMsgToQueue(strReply, fromQQ, msgtype::Private);
				for (auto qq : gm->session(fromGroup).get_ob())
				{
					if (qq != fromQQ)
					{
						AddMsgToQueue(strReply, qq, msgtype::Private);
					}
				}
			}
		}
		else
		{
			ResList dices;
			if (intTurnCnt > 1)
			{
				while (intTurnCnt--)
				{
					rdMainDice.Roll();
					string strForm = to_string(rdMainDice.intTotal);
					if (boolDetail)
					{
						string strCombined = rdMainDice.FormStringCombined();
						string strSeparate = rdMainDice.FormStringSeparate();
						if (strCombined != strForm)strForm = strCombined + "=" + strForm;
						if (strSeparate != strMainDice && strSeparate != strCombined)strForm = strSeparate + "=" +
							strForm;
					}
					dices << strForm;
				}
				strVar["res"] = dices.dot(", ").line(7).show();
			}
			else strVar["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
			strReply = format(GlobalMsg[strType], {strVar["pc"], strVar["res"], strVar["reason"]});
			if (!isHidden)
			{
				reply();
			}
			else
			{
				strReply = format(strReply, GlobalMsg, strVar);
				strReply = "��" + printChat(fromChat) + "�� " + strReply;
				AddMsgToQueue(strReply, fromQQ, msgtype::Private);
				for (auto qq : gm->session(fromGroup).get_ob())
				{
					if (qq != fromQQ)
					{
						AddMsgToQueue(strReply, qq, msgtype::Private);
					}
				}
			}
		}
		if (isHidden)
		{
			reply(GlobalMsg["strRollHidden"], {strVar["pc"]});
		}
		return 1;
	}
	return 0;
}

int FromMsg::CustomReply()
{
	const string strKey = readRest();
	if (auto deck = CardDeck::mReplyDeck.find(strKey); deck != CardDeck::mReplyDeck.end()
		|| (!isBotOff && (deck = CardDeck::mReplyDeck.find(strMsg)) != CardDeck::mReplyDeck.end()))
	{
		if (strVar.empty())
		{
			strVar["nick"] = getName(fromQQ, fromGroup);
			strVar["pc"] = getPCName(fromQQ, fromGroup);
			strVar["at"] = fromType != msgtype::Private ? "[CQ:at,qq=" + to_string(fromQQ) + "]" : strVar["nick"];
		}
		reply(CardDeck::drawCard(deck->second, true));
		AddFrq(fromQQ, fromTime, fromChat);
		return 1;
	}
	return 0;
}

//�ж��Ƿ���Ӧ
bool FromMsg::DiceFilter()
{
	while (isspace(static_cast<unsigned char>(strMsg[0])))
		strMsg.erase(strMsg.begin());
	init(strMsg);
	bool isOtherCalled = false;
	string strAt = CQ_AT + to_string(getLoginQQ()) + "]";
	while (strMsg.find(CQ_AT) == 0)
	{
		if (strMsg.find(strAt) == 0)
		{
			strMsg = strMsg.substr(strAt.length());
			isCalled = true;
		}
		else if (strMsg.find("[CQ:at,qq=all]") == 0) 
		{
			strMsg = strMsg.substr(14);
			isCalled = true;
		}
		else if (strMsg.find(']') != string::npos)
		{
			strMsg = strMsg.substr(strMsg.find(']') + 1);
			isOtherCalled = true;
		}
		while (isspace(static_cast<unsigned char>(strMsg[0])))
			strMsg.erase(strMsg.begin());
	}
	if (isOtherCalled && !isCalled)return false;
	init2(strMsg);
	if (fromType == msgtype::Private) isCalled = true;
	trusted = trustedQQ(fromQQ);
	isBotOff = (console["DisabledGlobal"] && (trusted < 4 || !isCalled)) || (!(isCalled && console["DisabledListenAt"])
		&& (groupset(fromGroup, "ͣ��ָ��") > 0));
	if (DiceReply())
	{
		AddFrq(fromQQ, fromTime, fromChat);
		if (isAns)getUser(fromQQ).update(fromTime);
		if (fromType != msgtype::Private)chat(fromGroup).update(fromTime);
		return true;
	}
	if (groupset(fromGroup, "���ûظ�") < 1 && CustomReply())return true;
	if (isBotOff)return console["DisabledBlock"];
	return false;
}

int FromMsg::readNum(int& num)
{
	string strNum;
	while (intMsgCnt < strMsg.length() && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])) && strMsg[intMsgCnt]
		!= '-')intMsgCnt++;
	if (strMsg[intMsgCnt] == '-')
	{
		strNum += '-';
		intMsgCnt++;
	}
	if (intMsgCnt >= strMsg.length())return -1;
	while (intMsgCnt < strMsg.length() && isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
	{
		strNum += strMsg[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.length() > 9)return -2;
	num = stoi(strNum);
	return 0;
}

int FromMsg::readChat(chatType& ct, bool isReroll)
{
	const int intFormor = intMsgCnt;
	if (const string strT = readPara(); strT == "me")
	{
		ct = {fromQQ, msgtype::Private};
		return 0;
	}
	else
	{
		if (strT == "this")
		{
			ct = fromChat;
			return 0;
		}
		if (strT == "qq")
		{
			ct.second = msgtype::Private;
		}
		else if (strT == "group")
		{
			ct.second = msgtype::Group;
		}
		else if (strT == "discuss")
		{
			ct.second = msgtype::Discuss;
		}
		else
		{
			if (isReroll)intMsgCnt = intFormor;
			return -1;
		}
	}
	if (const long long llID = readID(); llID)
	{
		ct.first = llID;
		return 0;
	}
	if (isReroll)intMsgCnt = intFormor;
	return -2;
}
