#include "DDAPI.h"
#include "DiceEvent.h"
#include "Jsonio.h"
#include "MsgFormat.h"
#include "DiceCensor.h"
#include "DiceMod.h"
#include "ManagerSystem.h"
#include "BlackListManager.h"
#include "CharacterCard.h"
#include "DiceSession.h"
#include "GetRule.h"
#include "DiceNetwork.h"
#include "DiceCloud.h"
#include "DiceGUI.h"
#include "DiceStatic.hpp"
#include <memory>
#include <ctime>
using namespace std;

FromMsg::FromMsg(const AttrVars& var, const chatInfo& ct) :DiceJobDetail(var, ct) {
	strMsg = vars["fromMsg"].to_str();
	if (fromChat.gid) {
		pGrp = &chat(fromChat.gid);
		fromSession = fromChat.gid;
	}
	else {
		fromSession = ~fromChat.uid;
	}

}
bool FromMsg::isPrivate()const {
	return !fromChat.gid;
}
bool FromMsg::isFromMaster()const {
	return fromChat.uid == console.master();
}
bool FromMsg::isChannel()const {
	return fromChat.chid;
}

FromMsg& FromMsg::initVar(const std::initializer_list<const std::string>& replace_str) {
	int index = 0;
	for (const auto& s : replace_str) {
		vars[to_string(index++)] = s;
	}
	return *this;
}
void FromMsg::formatReply() {
	if (!vars.count("nick") || vars["nick"].str_empty())vars["nick"] = getName(fromChat.uid, fromChat.gid);
	if (!vars.count("pc") || vars["pc"].str_empty())getPCName(vars);
	if (!vars.count("at") || vars["at"].str_empty())vars["at"] = !isPrivate() ? "[CQ:at,id=" + to_string(fromChat.uid) + "]" : vars["nick"];
	if (msgMatch.ready())strReply = convert_realw2a(msgMatch.format(convert_a2realw(strReply.c_str())).c_str());
	strReply = format(strReply, GlobalMsg, vars);
}

void FromMsg::reply(const std::string& msgReply, bool isFormat) {
	strReply = msgReply;
	reply(isFormat);
}

void FromMsg::reply(const char* msgReply, bool isFormat) {
	strReply = msgReply;
	reply(isFormat);
}

void FromMsg::reply(const std::string& msgReply, const std::initializer_list<const std::string>& replace_str) {
	initVar(replace_str);
	strReply = msgReply;
	reply();
}

void FromMsg::reply(bool isFormat) {
	if (isVirtual && fromChat.uid == console.DiceMaid && isPrivate())return;
	isAns = true;
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	if (isFormat)
		formatReply();
	if (console["ReferMsgReply"] && vars["msgid"])strReply = "[CQ:reply,id=" + vars["msgid"].to_str() + "]" + strReply;
	AddMsgToQueue(strReply, fromChat);
	if (LogList.count(fromSession) && gm->session(fromSession).is_logging()) {
		filter_CQcode(strReply, fromChat.gid);
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		logout << GBKtoUTF8(getMsg("strSelfName")) + "(" + to_string(console.DiceMaid) + ") " + printTTime(fromTime) << endl
			<< GBKtoUTF8(strReply) << endl << endl;
	}
}

void FromMsg::fwdMsg()
{
	if (LinkList.count(fromSession) && LinkList[fromSession].second && isFromMaster() && strLowerMessage.find(".link") != 0)
	{
		string strFwd;
		if (trusted < 5)strFwd += printFrom();
		strFwd += strMsg;
		if (long long aim = LinkList[fromSession].first;aim < 0) {
			AddMsgToQueue(strFwd, ~aim);
		}
		else if (ChatList.count(aim)) {
			AddMsgToQueue(strFwd, { 0,aim });
		}
	}
	if (LogList.count(fromSession) && strLowerMessage.find(".log") != 0) {
		string msg = strMsg;
		filter_CQcode(msg, fromChat.gid);
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		if (!vars["nick"])vars["nick"] = getName(fromChat.uid, fromChat.gid);
		if (!vars["pc"])getPCName(vars);
		logout << GBKtoUTF8(vars["pc"].to_str()) + "(" + to_string(fromChat.uid) + ") " + printTTime(fromTime) << endl
			<< GBKtoUTF8(msg) << endl << endl;
	}
}

void FromMsg::note(std::string strMsg, int note_lv)
{
	strMsg = format(strMsg, GlobalMsg, vars);
	ofstream fout(DiceDir / "audit" / ("log" + to_string(console.DiceMaid) + "_" + printDate() + ".txt"),
		ios::out | ios::app);
	fout << printSTNow() << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
	fout.close();
	reply(strMsg);
	const string note = getName(fromChat.uid) + strMsg;
	for (const auto& [ct, level] : console.NoticeList)
	{
		if (!(level & note_lv) || ct.uid == fromChat.uid || ct.gid == fromChat.gid)continue;
		AddMsgToQueue(note, ct);
	}
}
int FromMsg::AdminEvent(const string& strOption)
{
	if (strOption == "isban")
	{
		vars["target"] = readDigit();
		blacklist->isban(this);
		return 1;
	}
	if (strOption == "state")
	{
		ResList res;
		res << "Servant:" + printUser(console.DiceMaid)
			<< "Master:" + printUser(console.master())
			<< (console["Private"] ? "˽��ģʽ" : "����ģʽ");
		if (console["LeaveDiscuss"])res << "����������";
		if (console["DisabledGlobal"])res << "ȫ�־�Ĭ��";
		if (console["DisabledMe"])res << "ȫ�ֽ���.me";
		if (console["DisabledJrrp"])res << "ȫ�ֽ���.jrrp";
		if (console["DisabledDraw"])res << "ȫ�ֽ���.draw";
		if (console["DisabledSend"])res << "ȫ�ֽ���.send";
		if (trusted > 3)
			res << "����Ⱥ������" + to_string(DD::getGroupIDList().size())
			<< "Ⱥ��¼����" + to_string(ChatList.size())
			<< "��������" + to_string(DD::getFriendQQList().size())
			<< "�û���¼����" + to_string(UserList.size())
			<< "�����û�����" + to_string(today->cnt())
			<< (!PList.empty() ? "��ɫ����¼����" + to_string(PList.size()) : "")
			<< "�������û�����" + to_string(blacklist->mQQDanger.size())
			<< "������Ⱥ����" + to_string(blacklist->mGroupDanger.size())
			<< (censor.size() ? "���дʿ��ģ��" + to_string(censor.size()) : "")
			<< console.listClock().dot("\t").show();
		reply(getMsg("strSelfName") + "�ĵ�ǰ���" + res.show());
		return 1;
	}
	if (trusted < 4)
	{
		reply(getMsg("strNotAdmin"));
		return -1;
	}
	if (auto it = Console::intDefault.find(strOption);it != Console::intDefault.end())
	{
		int intSet = 0;
		switch (readNum(intSet))
		{
		case 0:
			console.set(it->first, intSet);
			note("�ѽ�" + getMsg("strSelfName") + "��" + it->first + "����Ϊ" + to_string(intSet), 0b10);
			break;
		case -1:
			reply(getMsg("strSelfName") + "����Ϊ" + to_string(console[strOption.c_str()]));
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
		getUser(fromChat.uid).trust(3);
		console.NoticeList.erase({ fromChat.uid, 0,0 });
		return 1;
	}
	if (strOption == "on")
	{
		if (console["DisabledGlobal"])
		{
			console.set("DisabledGlobal", 0);
			note("��ȫ�ֿ���" + getMsg("strSelfName"), 3);
		}
		else
		{
			reply(getMsg("strSelfName") + "���ھ�Ĭ�У�");
		}
		return 1;
	}
	if (strOption == "off")
	{
		if (console["DisabledGlobal"])
		{
			reply(getMsg("strSelfName") + "�Ѿ���Ĭ��");
		}
		else
		{
			console.set("DisabledGlobal", 1);
			note("��ȫ�ֹر�" + getMsg("strSelfName"), 0b10);
		}
		return 1;
	}
	if (strOption == "dicelist")
	{
		getDiceList();
		strReply = "��ǰ�����б�";
		for (auto& [diceQQ, masterQQ] : mDiceList)
		{
			strReply += "\n" + printUser(diceQQ);
		}
		reply();
		return 1;
	}
	if (strOption == "censor") {
		readSkipSpace();
		if (strMsg[intMsgCnt] == '+') {
			intMsgCnt++;
			vars["danger_level"] = readToColon();
			Censor::Level danger_level = censor.get_level(vars["danger_level"].to_str());
			readSkipColon();
			ResList res;
			while (intMsgCnt != strMsg.length()) {
				string item = readItem();
				if (!item.empty()) {
					censor.add_word(item, danger_level);
					res << item;
				}
			}
			if (res.empty()) {
				reply("{nick}δ�����������дʣ�");
			}
			else {
				note("{nick}�����{danger_level}�����д�" + to_string(res.size()) + "��:" + res.show(), 1);
			}
		}
		else if (strMsg[intMsgCnt] == '-') {
			intMsgCnt++;
			ResList res,resErr;
			while (intMsgCnt != strMsg.length()) {
				string item = readItem();
				if (!item.empty()) {
					if (censor.rm_word(item))
						res << item;
					else
						resErr << item;
				}
			}
			if (res.empty()) {
				reply("{nick}δ������Ƴ����дʣ�");
			}
			else {
				note("{nick}���Ƴ����д�" + to_string(res.size()) + "��:" + res.show(), 1);
			}
			if (!resErr.empty())
				reply("{nick}�Ƴ����������д�" + to_string(resErr.size()) + "��:" + resErr.show());
		}
		else
			reply(fmt->get_help("censor"));
		return 1;
	}
	if (strOption == "only")
	{
		if (console["Private"])
		{
			reply(getMsg("strSelfName") + "�ѳ�Ϊ˽�����");
		}
		else
		{
			console.set("Private", 1);
			note("�ѽ�" + getMsg("strSelfName") + "��Ϊ˽�á�", 0b10);
		}
		return 1;
	}
	if (strOption == "public")
	{
		if (console["Private"])
		{
			console.set("Private", 0);
			note("�ѽ�" + getMsg("strSelfName") + "��Ϊ���á�", 0b10);
		}
		else
		{
			reply(getMsg("strSelfName") + "�ѳ�Ϊ�������");
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
		if (strType.empty())
		{
			reply(getMsg("strSelfName") + "�Ķ�ʱ�б�" + console.listClock().show());
			return 1;
		}
		Console::Clock cc{0, 0};
		switch (readClock(cc))
		{
		case 0:
			if (isErase)
			{
				if (console.rmClock(cc, strType))reply(
					getMsg("strSelfName") + "�޴˶�ʱ��Ŀ");
				else note("���Ƴ�" + getMsg("strSelfName") + "��" + printClock(cc) + "�Ķ�ʱ" + strType, 0b10);
			}
			else
			{
				console.setClock(cc, strType);
				note("������" + getMsg("strSelfName") + "��" + printClock(cc) + "�Ķ�ʱ" + strType, 0b10);
			}
			break;
		case -1:
			reply(getMsg("strParaEmpty"));
			break;
		case -2:
			reply(getMsg("strParaIllegal"));
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
		if (chatInfo cTarget; readChat(cTarget))
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
				note("�ѽ�" + getMsg("strSelfName") + "��֪ͨ����" + printChat(cTarget) + "�Ƴ�", 0b1);
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
					if (int intNum = stoi(strNum); intNum > 9)continue;
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
					"�ѽ�" + getMsg("strSelfName") + "�Դ���" + printChat(cTarget) + "֪ͨ�������Ϊ" + to_binary(
						console.showNotice(cTarget)), 0b1);
				else reply(getMsg("strParaIllegal"));
				return 1;
			}
			int intLV;
			switch (readNum(intLV))
			{
			case 0:
				if (intLV < 0 || intLV > 63)
				{
					reply(getMsg("strParaIllegal"));
					return 1;
				}
				console.setNotice(cTarget, intLV);
				note("�ѽ�" + getMsg("strSelfName") + "�Դ���" + printChat(cTarget) + "֪ͨ�������Ϊ" + to_string(intLV), 0b1);
				break;
			case -1:
				reply("����" + printChat(cTarget) + "��" + getMsg("strSelfName") + "����֪ͨ����Ϊ��" + to_binary(
					console.showNotice(cTarget)));
				break;
			case -2:
				reply(getMsg("strParaIllegal"));
				break;
			}
		}
		return 1;
	}
	if (strOption == "ext")
	{
		try
		{
			string action = readPara();
			if (action == "install")
			{
				string package = readRest();
				ExtensionManagerInstance->installPackage(GBKtoUTF8(package));
				reply("�ѳɹ���װ" + package);
			}
			else if (action == "query")
			{
				string package = readRest();
				reply(ExtensionManagerInstance->queryPackage(GBKtoUTF8(package)));
			}
			else if (action == "update")
			{
				ExtensionManagerInstance->refreshIndex();
				reply("�ѳɹ�ˢ����������棬" + to_string(ExtensionManagerInstance->getIndexCount()) + "����չ���ã�"
					+ to_string(ExtensionManagerInstance->getUpgradableCount()) + "��������");
			}
			else if (action == "list")
			{
				string re = "������չ:\n";
				auto index = ExtensionManagerInstance->getIndex();
				for (const auto& i : index)
				{
					re += UTF8toGBK(i.second.name) + " ";
				}
				reply(re);
			}
			else if (action == "search")
			{
				string package = readRest();
				string re = "�������:\n";
				auto index = ExtensionManagerInstance->getIndex();
				for (const auto& i : index)
				{
					string GBKname = UTF8toGBK(i.second.name);
					if(GBKname.find(package) != string::npos)
					{
						re += GBKname + " ";
					}
				}
				reply(re);
			}
			else if (action == "listinstalled")
			{
				string re = "�Ѱ�װ��չ:\n";
				auto index = ExtensionManagerInstance->getInstalledIndex();
				for (const auto& i : index)
				{
					re += UTF8toGBK(i.second.first.name) + " ";
				}
				reply(re);
			}
			else if (action == "remove")
			{
				string package = readRest();
				ExtensionManagerInstance->removePackage(GBKtoUTF8(package));
				reply("�ѳɹ�ж��" + package);
			}
			else if (action == "queryinstalled")
			{
				string package = readRest();
				reply(ExtensionManagerInstance->queryInstalledPackage(GBKtoUTF8(package)).first);
			}
			else if (action == "searchinstalled")
			{
				string package = readRest();
				string re = "�������:\n";
				auto index = ExtensionManagerInstance->getInstalledIndex();
				for (const auto& i : index)
				{
					string GBKname = UTF8toGBK(i.second.first.name);
					if (GBKname.find(package) != string::npos)
					{
						re += GBKname + " ";
					}
				}
				reply(re);
			}
			else if (action == "upgrade")
			{
				string package = readRest();
				if (package.empty())
				{
					int cnt = ExtensionManagerInstance->upgradeAllPackages();
					reply("�ɹ�����" + std::to_string(cnt) + "����չ");
				}
				else
				{
					if (ExtensionManagerInstance->upgradePackage(GBKtoUTF8(package)))
					{
						reply(package + "�ѳɹ�������");
					}
					else
					{
						reply(package + "��������");
					}
				}
			}
			else
			{
				reply("Unknown command");
			}
		}
		catch (const std::exception& e)
		{
			reply(UTF8toGBK(e.what(), true));
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
		chatInfo cTarget;
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
		chatInfo cTarget;
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
		for(long long uid: DD::getFriendQQList()){
			if (blacklist->get_qq_danger(uid))
				res << printUser(uid);
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
					note("���Ƴ�" + printGroup(llGroup) + "��" + getMsg("strSelfName") + "��ʹ�����");
				}
				else
				{
					reply("��Ⱥδӵ��" + getMsg("strSelfName") + "��ʹ����ɣ�");
				}
			}
			else
			{
				if (groupset(llGroup, "���ʹ��") > 0)
				{
					reply("��Ⱥ��ӵ��" + getMsg("strSelfName") + "��ʹ����ɣ�");
				}
				else
				{
					chat(llGroup).set("���ʹ��").reset("δ���");
					if (!chat(llGroup).isset("����") && !chat(llGroup).isset("δ��"))AddMsgToQueue(
						getMsg("strAuthorized"), { 0,llGroup });
					note("�����" + printGroup(llGroup) + "��" + getMsg("strSelfName") + "��ʹ�����");
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
	else
	{
		bool boolErase = false;
		vars["note"] = readPara();
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
				note("����" + getMsg("strSelfName") + "�˳�" + printChat(chat(llTargetID)), 0b10);
				chat(llTargetID).reset("����").leave();
			}
			else
			{
				reply(getMsg("strGroupGetErr"));
			}
			return 1;
		}
		else if (strOption == "boton")
		{
			if (ChatList.count(llTargetID))
			{
				if (groupset(llTargetID, "ͣ��ָ��") > 0)
				{
					chat(llTargetID).reset("ͣ��ָ��");
					note("����" + getMsg("strSelfName") + "��" + printGroup(llTargetID) + "����ָ���");
				}
				else reply(getMsg("strSelfName") + "���ڸ�Ⱥ����ָ��!");
			}
			else
			{
				reply(getMsg("strGroupGetErr"));
			}
		}
		else if (strOption == "botoff")
		{
			if (groupset(llTargetID, "ͣ��ָ��") < 1)
			{
				chat(llTargetID).set("ͣ��ָ��");
				note("����" + getMsg("strSelfName") + "��" + printGroup(llTargetID) + "ͣ��ָ���", 0b1);
			}
			else reply(getMsg("strSelfName") + "���ڸ�Ⱥͣ��ָ��!");
			return 1;
		}
		else if (strOption == "blackgroup")
		{
			if (llTargetID == 0)
			{
				ResList res;
				for (auto [each, danger] : blacklist->mGroupDanger) {
					res << printGroup(each) + ":" + to_string(danger);
				}
				reply(res.show(), false);
				return 1;
			}
			vars["time"] = printSTNow();
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
				for (auto& [uid, user] : UserList)
				{
					if (user.nTrust)strReply += "\n" + printUser(uid) + ":" + to_string(user.nTrust);
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
							reply(getMsg("strUserTrustDenied"));
						}
						else
						{
							getUser(llTargetID).trust(0);
							note("���ջ�" + getMsg("strSelfName") + "��" + printUser(llTargetID) + "�����Ρ�", 0b1);
						}
					}
					else
					{
						reply(printUser(llTargetID) + "������" + getMsg("strSelfName") + "�İ�������");
					}
				}
				else
				{
					if (trustedQQ(llTargetID))
					{
						reply(printUser(llTargetID) + "�Ѽ���" + getMsg("strSelfName") + "�İ�����!");
					}
					else
					{
						getUser(llTargetID).trust(1);
						note("�����" + getMsg("strSelfName") + "��" + printUser(llTargetID) + "�����Ρ�", 0b1);
						vars["user_nick"] = getName(llTargetID);
						AddMsgToQueue(format(getMsg("strWhiteQQAddNotice"), GlobalMsg, vars), llTargetID);
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
				ResList res;
				for (auto [each, danger] : blacklist->mQQDanger)
				{
					res << printUser(each) + ":" + to_string(danger);
				}
				reply(res.show(), false);
				return 1;
			}
			vars["time"] = printSTNow();
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
		else reply(getMsg("strAdminOptionEmpty"));
		return 0;
	}
}

int FromMsg::MasterSet()
{
	const std::string strOption = readPara();
	if (strOption.empty())
	{
		reply(getMsg("strAdminOptionEmpty"));
		return -1;
	}
	if (strOption == "groupclr")
	{
		vars["clear_mode"] = readRest();
		cmd_key = "clrgroup";
		sch.push_job(*this);
		return 1;
	}
	if (strOption == "delete")
	{
		if (console.master() != fromChat.uid)
		{
			reply(getMsg("strNotMaster"));
			return 1;
		}
		reply("�㲻����" + getMsg("strSelfName") + "��Master��");
		console.killMaster();
		return 1;
	}
	else if (strOption == "reset")
	{
		if (console.master() != fromChat.uid)
		{
			reply(getMsg("strNotMaster"));
			return 1;
		}
		const string strMaster = readDigit();
		if (strMaster.empty() || stoll(strMaster) == console.master())
		{
			reply("Master��Ҫ��ǲ{strSelfCall}!");
		}
		else
		{
			console.newMaster(stoll(strMaster));
			note("�ѽ�Masterת�ø�" + printUser(console.master()));
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
					note("���ջ�" + printUser(llAdmin) + "��" + getMsg("strSelfName") + "�Ĺ���Ȩ�ޡ�", 0b100);
					console.rmNotice({ llAdmin,0,0 });
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
					console.addNotice({ llAdmin,0,0 }, 0b1110);
					note("�����" + printUser(llAdmin) + "��" + getMsg("strSelfName") + "�Ĺ���Ȩ�ޡ�", 0b100);
				}
			}
			return 1;
		}
		ResList list;
		for (const auto& [uid, user] : UserList)
		{
			if (user.nTrust > 3)list << printUser(uid);
		}
		reply(getMsg("strSelfName") + "�Ĺ���Ȩ��ӵ���߹�" + to_string(list.size()) + "λ��" + list.show());
		return 1;
	}
	return AdminEvent(strOption);
}

int FromMsg::BasicOrder()
{
	if (strMsg[0] != '.')return 0;
	intMsgCnt++;
	while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
		intMsgCnt++;
	//Warning:Temporary
	isAuth = trusted > 3
		|| isChannel() || isPrivate() || DD::isGroupAdmin(fromChat.gid, fromChat.uid, true) || pGrp->inviter == fromChat.uid;
	//ָ��ƥ��
	if (console["DebugMode"])console.log("listen:" + strMsg, 0, printSTNow());
	if (strLowerMessage.substr(intMsgCnt, 9) == "authorize")
	{
		intMsgCnt += 9;
		readSkipSpace();
		if (isPrivate() || strMsg[intMsgCnt] == '+')
		{
			long long llTarget = readID();
			if (llTarget)
			{
				pGrp = &chat(llTarget);
			}
			else
			{
				reply(getMsg("strGroupIDEmpty"));
				return 1;
			}
		}
		if (pGrp->isset("���ʹ��") && !pGrp->isset("δ���") && !pGrp->isset("Э����Ч"))return 0;
		if (trusted > 0)
		{
			pGrp->set("���ʹ��").reset("δ���").reset("Э����Ч");
			note("����Ȩ" + printGroup(pGrp->ID) + "���ʹ��", 1);
			AddMsgToQueue(getMsg("strGroupAuthorized", vars), { 0,pGrp->ID });
		}
		else
		{
			if (!console["CheckGroupLicense"] && !console["Private"] && !isCalled)return 0;
			string strInfo = readRest();
			if (strInfo.empty())console.log(printUser(fromChat.uid) + "����" + printGroup(pGrp->ID) + "���ʹ��", 0b10, printSTNow());
			else console.log(printUser(fromChat.uid) + "����" + printGroup(pGrp->ID) + "���ʹ�ã����ԣ�" + strInfo, 0b100, printSTNow());
			reply(getMsg("strGroupLicenseApply"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		if (isPrivate())
		{
			string QQNum = readDigit();
			if (QQNum.empty())
			{
				reply(getMsg("strDismissPrivate"));
				return -1;
			}
			long long llGroup = stoll(QQNum);
			if (!ChatList.count(llGroup))
			{
				reply(getMsg("strGroupNotFound"));
				return 1;
			}
			Chat& grp = chat(llGroup);
			if (grp.isset("����") || grp.isset("δ��"))
			{
				reply(getMsg("strGroupAway"));
			}
			if (trustedQQ(fromChat.uid) > 2) {
				grp.leave(getMsg("strAdminDismiss", vars));
				reply(getMsg("strGroupExit"));
			}
			else if(DD::isGroupAdmin(llGroup, fromChat.uid, true) || (grp.inviter == fromChat.uid))
			{
				reply(getMsg("strDismiss"));
			}
			else
			{
				reply(getMsg("strPermissionDeniedErr"));
			}
			return 1;
		}
		string QQNum = readDigit();
		if (QQNum.empty() || QQNum == to_string(console.DiceMaid)
			|| (QQNum.length() == 4 && stoll(QQNum) == console.DiceMaid % 10000)){
			if (trusted > 2)
			{
				pGrp->leave(getMsg("strAdminDismiss", vars));
				return 1;
			}
			if (pGrp->isset("Э����Ч"))return 0;
			if (isAuth)
			{
				pGrp->leave(getMsg("strDismiss"));
			}
			else
			{
				if (!isCalled && (pGrp->isset("ͣ��ָ��") || DD::getGroupSize(fromChat.gid).currSize > 200))AddMsgToQueue(getMsg("strPermissionDeniedErr", vars), fromChat.uid);
				else reply(getMsg("strPermissionDeniedErr"));
			}
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "warning")
	{
		intMsgCnt += 7;
		string strWarning = readRest();
		AddWarning(strWarning, fromChat.uid, fromChat.gid);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "master" && console.isMasterMode)
	{
		intMsgCnt += 6;
		if (!console.master())
		{
			string strOption = readRest();
			if (strOption == "public"){
				console.set("BelieveDiceList", 1);
				console.set("LeaveBlackQQ", 1);
				console.set("BannedBanInviter", 1);
				console.set("KickedBanInviter", 1);
			}
			else{
				console.set("Private", 1);
			}
			console.newMaster(fromChat.uid);
		}
		else if (trusted > 4 || console.master() == fromChat.uid)
		{
			return MasterSet();
		}
		else
		{
			if (isCalled)reply(getMsg("strNotMaster"));
			return 1;
		}
		return 1;
	}
	else if (!isPrivate() && pGrp->isset("Э����Ч")){
		return 1;
	}
	if (blacklist->get_qq_danger(fromChat.uid) || (!isPrivate() && blacklist->get_group_danger(fromChat.gid)))
	{
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		string Command = readPara();
		string QQNum = readDigit();
		if (QQNum.empty() || QQNum == to_string(console.DiceMaid)
			|| (QQNum.length() == 4 && stoll(QQNum) == console.DiceMaid % 10000))
		{
			if (Command == "on")
			{
				if (console["DisabledGlobal"])reply(getMsg("strGlobalOff"));
				else if (!isPrivate() && ((console["CheckGroupLicense"] && pGrp->isset("δ���")) || (console["CheckGroupLicense"] == 2 && !pGrp->isset("���ʹ��"))))reply(getMsg("strGroupLicenseDeny"));
				else if (!isPrivate())
				{
					if (isAuth || trusted >2)
					{
						if (groupset(fromChat.gid, "ͣ��ָ��") > 0)
						{
							chat(fromChat.gid).reset("ͣ��ָ��");
							reply(getMsg("strBotOn"));
						}
						else
						{
							reply(getMsg("strBotOnAlready"));
						}
					}
					else
					{
						if (groupset(fromChat.gid, "ͣ��ָ��") > 0 && DD::getGroupSize(fromChat.gid).currSize > 200)AddMsgToQueue(
							getMsg("strPermissionDeniedErr", vars), fromChat.uid);
						else reply(getMsg("strPermissionDeniedErr"));
					}
				}
			}
			else if (Command == "off")
			{
				if (isAuth || trusted > 2)
				{
					if (groupset(fromChat.gid, "ͣ��ָ��"))
					{
						if (!isCalled && QQNum.empty() && pGrp->isGroup && DD::getGroupSize(fromChat.gid).currSize > 200)AddMsgToQueue(getMsg("strBotOffAlready", vars), fromChat.uid);
						else reply(getMsg("strBotOffAlready"));
					}
					else
					{
						chat(fromChat.gid).set("ͣ��ָ��");
						reply(getMsg("strBotOff"));
					}
				}
				else
				{
					if (groupset(fromChat.gid, "ͣ��ָ��"))AddMsgToQueue(getMsg("strPermissionDeniedErr", vars), fromChat.uid);
					else reply(getMsg("strPermissionDeniedErr"));
				}
			}
			else if (!Command.empty() && !isCalled && pGrp->isset("ͣ��ָ��"))
			{
				return 0;
			}
			else if (!isPrivate() && pGrp->isset("ͣ��ָ��") && DD::getGroupSize(fromChat.gid).currSize > 500 && !isCalled)
			{
				AddMsgToQueue(getMsg("strBotHeader") + Dice_Full_Ver_On + getMsg("strBotMsg"), fromChat.uid);
			}
			else
			{
				this_thread::sleep_for(1s);
				reply(getMsg("strBotHeader") + Dice_Full_Ver_On + getMsg("strBotMsg"));
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "chan") {
		if (isPrivate())return 0;
		intMsgCnt += 4;
		string strPara{ readPara() };
		string QQNum = readDigit();
		if (!QQNum.empty() && QQNum != to_string(console.DiceMaid)
			&& (QQNum.length() != 4 || stoll(QQNum) != console.DiceMaid % 10000))return 0;
		if (strPara == "on") {
			pGrp->ChConf[fromChat.chid]["order"] = 1;
			reply(getMsg("strBotChannelOn"));
		}
		else if (strPara == "off") {
			pGrp->ChConf[fromChat.chid]["order"] = -1;
			reply(getMsg("strBotChannelOff"));
		}
	}
	if (isDisabled || (!isCalled || !console["DisabledListenAt"]) && (groupset(fromChat.gid, "ͣ��ָ��") > 0))
	{
		if (isPrivate())
		{
			reply(getMsg("strGlobalOff"));
			return 1;
		}
		return 0;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "helpdoc" && trusted > 3)
	{
		intMsgCnt += 7;
		while (strMsg[intMsgCnt] == ' ')
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			reply(getMsg("strHlpNameEmpty"));
			return true;
		}
		string key{ readUntilSpace() };
		vars["key"] = key;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			CustomHelp.erase(key);
			if (auto it = HelpDoc.find(key); it != HelpDoc.end())
				fmt->set_help(it->first, it->second);
			else
				fmt->rm_help(key);
			reply(getMsg("strHlpReset"));
		}
		else
		{
			string strHelpdoc = strMsg.substr(intMsgCnt);
			CustomHelp[key] = strHelpdoc;
			fmt->set_help(key, strHelpdoc);
			reply(getMsg("strHlpSet"));
		}
		saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
		return true;
	}
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		vars["help_word"] = readRest();
		if (!isPrivate())
		{
			if (!isAuth && (vars["help_word"] == "on" || vars["help_word"] == "off"))
			{
				reply(getMsg("strPermissionDeniedErr"));
				return 1;
			}
			vars["option"] = "����help";
			if (vars["help_word"] == "off")
			{
				if (groupset(fromChat.gid, vars["option"].to_str()) < 1)
				{
					chat(fromChat.gid).set(vars["option"].to_str());
					reply(getMsg("strGroupSetOn"));
				}
				else
				{
					reply(getMsg("strGroupSetOnAlready"));
				}
				return 1;
			}
			if (vars["help_word"] == "on")
			{
				if (groupset(fromChat.gid, vars["option"].to_str()) > 0)
				{
					chat(fromChat.gid).reset(vars["option"].to_str());
					reply(getMsg("strGroupSetOff"));
				}
				else
				{
					reply(getMsg("strGroupSetOffAlready"));
				}
				return 1;
			}
			if (groupset(fromChat.gid, vars["option"].to_str()) > 0)
			{
				reply(getMsg("strGroupSetOnAlready"));
				return 1;
			}
		}
		std::thread th(&DiceModManager::_help, fmt.get(), shared_from_this());
		th.detach();
		return true;
	}
	return 0;
}

int FromMsg::InnerOrder() {
	if (strMsg[0] != '.')return 0;
	if (WordCensor()) {
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 8) == "setreply") {
		return 0;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome") {
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("welcome"));
			return 1;
		}
		if (isPrivate()) {
			reply(getMsg("strWelcomePrivate"));
			return 1;
		}
		if (isAuth) {
			string strWelcomeMsg = strMsg.substr(intMsgCnt);
			if (strWelcomeMsg == "clr") {
				if (chat(fromChat.gid).confs.count("��Ⱥ��ӭ")) {
					chat(fromChat.gid).reset("��Ⱥ��ӭ");
					reply(getMsg("strWelcomeMsgClearNotice"));
				}
				else {
					reply(getMsg("strWelcomeMsgClearErr"));
				}
			}
			else if (strWelcomeMsg == "show") {
				string strWelcome{ chat(fromChat.gid).confs["��Ⱥ��ӭ"].to_str() };
				if (strWelcome.empty())reply(getMsg("strWelcomeMsgEmpty"));
				else reply(strWelcome, false);	//ת����ע�����
			}
			else if (readPara() == "set") {
				chat(fromChat.gid).set("��Ⱥ��ӭ", strip(readRest()));
				reply(getMsg("strWelcomeMsgUpdateNotice"));
			}
			else {
				chat(fromChat.gid).set("��Ⱥ��ӭ", strWelcomeMsg);
				reply(getMsg("strWelcomeMsgUpdateNotice"));
			}
		}
		else {
			reply(getMsg("strPermissionDeniedErr"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "groups") {
		if (trusted < 4) {
			reply(getMsg("strNotAdmin"));
			return 1;
		}
		intMsgCnt += 6;
		string strOption = readPara();
		if (strOption == "list") {
			vars["list_mode"] = readPara();
			cmd_key = "lsgroup";
			sch.push_job(*this);
		}
		else if (strOption == "clr") {
			if (trusted < 5) {
				reply(getMsg("strNotMaster"));
				return 1;
			}
			int cnt = clearGroup();
			note("���������Ⱥ��¼" + to_string(cnt) + "��", 0b10);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "setcoc") {
		if (!isAuth) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		string strRule = readDigit();
		if (strRule.empty()) {
			if (isPrivate())chat(fromChat.gid).reset("rc����");
			else getUser(fromChat.uid).rmConf("rc����");
			reply(getMsg("strDefaultCOCClr"));
			return 1;
		}
		if (strRule.length() > 1) {
			reply(getMsg("strDefaultCOCNotFound"));
			return 1;
		}
		int intRule = stoi(strRule);
		switch (intRule) {
		case 0:
			reply(getMsg("strDefaultCOCSet") + "0 ������\n��1��ɹ�\n����50��96-100��ʧ�ܣ���50��100��ʧ��");
			break;
		case 1:
			reply(getMsg("strDefaultCOCSet") + "1\n����50��1��ɹ�����50��1-5��ɹ�\n����50��96-100��ʧ�ܣ���50��100��ʧ��");
			break;
		case 2:
			reply(getMsg("strDefaultCOCSet") + "2\n��1-5��<=�ɹ��ʴ�ɹ�\n��100���96-99��>�ɹ��ʴ�ʧ��");
			break;
		case 3:
			reply(getMsg("strDefaultCOCSet") + "3\n��1-5��ɹ�\n��96-100��ʧ��");
			break;
		case 4:
			reply(getMsg("strDefaultCOCSet") + "4\n��1-5��<=ʮ��֮һ��ɹ�\n����50��>=96+ʮ��֮һ��ʧ�ܣ���50��100��ʧ��");
			break;
		case 5:
			reply(getMsg("strDefaultCOCSet") + "5\n��1-2��<���֮һ��ɹ�\n����50��96-100��ʧ�ܣ���50��99-100��ʧ��");
			break;
		case 6:
			reply(getMsg("strDefaultCOCSet") + "6\n��ɫ������\n��1�����λʮλ��ͬ��<=�ɹ��ʴ�ɹ�\n��100�����λʮλ��ͬ��>�ɹ��ʴ�ʧ��");
			break;
		default:
			reply(getMsg("strDefaultCOCNotFound"));
			return 1;
		}
		if (isPrivate())chat(fromChat.gid).set("rc����", intRule);
		else getUser(fromChat.uid).setConf("rc����", intRule);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "system") {
		intMsgCnt += 6;
		if (console && trusted < 4) {
			reply(getMsg("strNotAdmin"));
			return -1;
		}
		string strOption = readPara();
#ifdef _WIN32
		if (strOption == "gui") {
			reply("Dice! GUI�ѱ����ã��뿼��ʹ��Dice! WebUI https://forum.kokona.tech/d/721-dice-webui-shi-yong-shuo-ming");
			thread th(GUIMain);
			th.detach();
			return 1;
		}
#endif
		if (strOption == "save") {
			dataBackUp();
			note("���ֶ�����{self}�����ݡ�", 0b1);
			return 1;
		}
		if (strOption == "load") {
			loadData();
			note("���ֶ�����{self}�����á�", 0b1);
			return 1;
		}
		if (strOption == "state")
		{
			time_t tt = time(nullptr);
#ifdef _MSC_VER
			localtime_s(&stNow, &tt);
#else
			localtime_r(&tt, &stNow);
#endif
#ifdef _WIN32
			double mbFreeBytes = 0, mbTotalBytes = 0;
			long long milDisk(getDiskUsage(mbFreeBytes, mbTotalBytes));
#endif
			ResList res;
			res << "����ʱ��:" + printSTime(stNow)
#ifdef _WIN32
				<< "�ڴ�ռ��:" + to_string(getRamPort()) + "%"
				<< "CPUռ��:" + toString(getWinCpuUsage() / 10.0) + "%"
				<< "Ӳ��ռ��:" + toString(milDisk / 10.0) + "%(����:" + toString(mbFreeBytes) + "GB/ " + toString(mbTotalBytes) + "GB)"
#endif
				<< "����ʱ��:" + printDuringTime(time(nullptr) - llStartTime)
				<< "����ָ����:" + to_string(today->get("frq"))
				<< "������ָ����:" + to_string(FrqMonitor::sumFrqTotal);
			reply(res.show());
			return 1;
		}
		if (strOption == "clrimg") {
			reply("�ǿ�Q��ܲ���Ҫ�˹���");
			return -1;
		}
		else if (strOption == "reload") {
			if (trusted < 5 && fromChat.uid != console.master()) {
				reply(getMsg("strNotMaster"));
				return -1;
			}
			cmd_key = "reload";
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "remake") {

			if (trusted < 5 && fromChat.uid != console.master()) {
				reply(getMsg("strNotMaster"));
				return -1;
			}
			cmd_key = "remake";
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "die") {
			if (trusted < 5 && fromChat.uid != console.master()) {
				reply(getMsg("strNotMaster"));
				return -1;
			}
			cmd_key = "die";
			sch.push_job(*this);
			return 1;
		}
		if (strOption == "rexplorer")
		{
#ifdef _WIN32
			if (trusted < 5 && fromChat.uid != console.master())
			{
				reply(getMsg("strNotMaster"));
				return -1;
			}
			system(R"(taskkill /f /fi "username eq %username%" /im explorer.exe)");
			system(R"(start %SystemRoot%\explorer.exe)");
			this_thread::sleep_for(3s);
			note("��������Դ��������\n��ǰ�ڴ�ռ�ã�" + to_string(getRamPort()) + "%");
#endif
		}
		else if (strOption == "cmd")
		{
#ifdef _WIN32
			if (fromChat.uid != console.master())
			{
				reply(getMsg("strNotMaster"));
				return -1;
			}
			string strCMD = readRest() + "\ntimeout /t 10";
			system(strCMD.c_str());
			reply("�����������С�");
			return 1;
#endif
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "admin") {
		intMsgCnt += 5;
		return AdminEvent(readPara());
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "cloud") {
		intMsgCnt += 5;
		string strOpt = readPara();
		if (trusted < 4 && fromChat.uid != console.master()) {
			reply(getMsg("strNotAdmin"));
			return 1;
		}
		if (strOpt == "update") {
			vars["ver"] = readPara();
			if (vars["ver"].str_empty()) {
				Cloud::checkUpdate(this);
			}
			else if (vars["ver"] == "dev" || vars["ver"] == "release") {
				cmd_key = "update";
				sch.push_job(*this);
			}
			return 1;
		}
		else if (strOpt == "black") {
			cmd_key = "cloudblack";
			sch.push_job(*this);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd") {
		vars["res"] = COC7D();
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d") {
		vars["res"] = COC6D();
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "group") {
		intMsgCnt += 5;
		long long llGroup(fromChat.gid);
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("group"));
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "all") {
			if (trusted < 5) {
				reply(getMsg("strNotMaster"));
				return 1;
			}
			intMsgCnt += 3;
			readSkipSpace();
			if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-') {
				bool isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				string strOption{ (vars["option"] = readRest()).to_str() };
				if (!mChatConf.count(vars["option"].to_str())) {
					reply(getMsg("strGroupSetNotExist"));
					return 1;
				}
				int Cnt = 0;
				if (isSet) {
					for (auto& [id, grp] : ChatList) {
						if (grp.isset(strOption))continue;
						grp.set(strOption);
						Cnt++;
					}
					vars["cnt"] = to_string(Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
				else {
					for (auto& [id, grp] : ChatList) {
						if (!grp.isset(strOption))continue;
						grp.reset(strOption);
						Cnt++;
					}
					vars["cnt"] = to_string(Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
			}
			return 1;
		}
		if (!(vars["group_id"] = readDigit(false)).str_empty()) {
			llGroup = stoll(vars["group_id"].to_str());
			if (!ChatList.count(llGroup)) {
				reply(getMsg("strGroupNotFound"));
				return 1;
			}
			if (getGroupAuth(llGroup) < 0) {
				reply(getMsg("strGroupDenied"));
				return 1;
			}
		}
		else if (isPrivate())return 0;
		else vars["group_id"] = to_string(fromChat.gid);
		Chat& grp = chat(llGroup);
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if(strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-'){
			int cntSet{ 0 };
			bool isSet{ strMsg[intMsgCnt] == '+' };
			do{
				isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				const string option{ (vars["option"] = readPara()).to_str() };
				readSkipSpace();
				if (!mChatConf.count(vars["option"].to_str())) {
					reply(getMsg("strGroupSetInvalid"));
					continue;
				}
				if (getGroupAuth(llGroup) >= get<string, short>(mChatConf, option, 0)) {
					if (isSet) {
						if (groupset(llGroup, vars["option"].to_str()) < 1) {
							chat(llGroup).set(vars["option"].to_str());
							++cntSet;
							if (vars["Option"] == "���ʹ��") {
								AddMsgToQueue(getMsg("strGroupAuthorized", vars), { 0, fromChat.uid });
							}
						}
						else {
							reply(getMsg("strGroupSetOnAlready"));
						}
					}
					else if (grp.isset(vars["option"].to_str())) {
						++cntSet;
						chat(llGroup).reset(vars["option"].to_str());
					}
					else {
						reply(getMsg("strGroupSetOffAlready"));
					}
				}
				else {
					reply(getMsg("strGroupSetDenied"));
				}
			} while (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-');
			if (cntSet == 1) {
				isSet ? reply(getMsg("strGroupSetOn")) : reply(getMsg("strGroupSetOff"));
			}
			else if(cntSet > 1) {
				vars["opt_list"] = grp.listBoolConf();
				reply(getMsg("strGroupMultiSet"));
			}
			return 1;
		}
		bool isInGroup{ fromChat.gid == llGroup || DD::isGroupMember(llGroup,console.DiceMaid,true) };
		string Command = readPara();
		vars["group"] = DD::printGroupInfo(llGroup);
		if (Command.empty()) {
			reply(fmt->get_help("group"));
			return 1;
		}
		else if (Command == "state") {
			ResList res;
			res << "��{group}��";
			res << grp.listBoolConf();
			res << "��¼������" + printDate(grp.tCreated);
			res << "����¼��" + printDate(grp.tUpdated);
			if (grp.inviter)res << "�����ߣ�" + printUser(grp.inviter);
			res << string("��Ⱥ��ӭ��") + (grp.isset("��Ⱥ��ӭ") ? "������" : "��");
			reply(getMsg("strSelfName") + res.show());
			return 1;
		}
		if (!grp.isGroup || (fromChat.gid == llGroup && isPrivate())) {
			reply(getMsg("strGroupNot"));
			return 1;
		}
		else if (Command == "info") {
			reply(DD::printGroupInfo(llGroup), false);
			return 1;
		}
		else if (!isInGroup) {
			reply(getMsg("strGroupNotIn"));
			return 1;
		}
		else if (Command == "survey") {
			int cntDiver(0);
			long long dayMax(0);
			set<string> sBlackQQ;
			int cntUser(0);
			size_t cntDice(0);
			time_t tNow = time(nullptr);
			const int intTDay = 24 * 60 * 60;
			int cntSize(0);
			set<long long> list{ DD::getGroupMemberList(llGroup) };
			if (list.empty()) {
				reply("{self}���س�Ա�б�ʧ�ܣ�");
				return 1;
			}
			for (auto each : list) {
				if (each == console.DiceMaid)continue;
				if (long long lst{ DD::getGroupLastMsg(llGroup,each) }; lst > 0) {
					long long dayDive((tNow - lst) / intTDay);
					if (dayDive > dayMax)dayMax = dayDive;
					if (dayDive > 30)++cntDiver;
				}
				if (blacklist->get_qq_danger(each) > 1) {
					sBlackQQ.insert(printUser(each));
				}
				if (UserList.count(each))++cntUser;
				if (DD::isDiceMaid(each))++cntDice;
				++cntSize;
			}
			ResList res;
			res << "��{group}��"
				<< "{self}�û�ռ��: " + to_string(cntUser * 100 / (cntSize)) + "%"
				<< (cntDice ? "ͬϵ����: " + to_string(cntDice) : "")
				<< (cntDiver ? "30��ǱˮȺԱ: " + to_string(cntDiver) : "");
			if (!sBlackQQ.empty()) {
				if (sBlackQQ.size() > 8)
					res << getMsg("strSelfName") + "�ĺ�������Ա" + to_string(sBlackQQ.size()) + "��";
				else {
					res << getMsg("strSelfName") + "�ĺ�������Ա:{blackqq}";
					ResList blacks;
					for (const auto& each : sBlackQQ) {
						blacks << each;
					}
					vars["blackqq"] = blacks.show();
				}
			}
			reply(res.show());
			return 1;
		}
		else if (Command == "diver") {
			bool bForKick = false;
			if (strLowerMessage.substr(intMsgCnt, 5) == "4kick") {
				bForKick = true;
				intMsgCnt += 5;
			}
			std::priority_queue<std::pair<time_t, string>> qDiver;
			time_t tNow = time(nullptr);
			const int intTDay = 24 * 60 * 60;
			int cntSize(0);
			for (auto each : DD::getGroupMemberList(llGroup)) {
				long long lst{ DD::getGroupLastMsg(llGroup,each) };
				time_t intLastMsg = (tNow - lst) / intTDay;
				if (lst > 0 || intLastMsg > 30) {
					qDiver.emplace(intLastMsg, (bForKick ? to_string(each)
												: printUser(each)));
				}
				++cntSize;
			}
			if (!cntSize) {
				reply("{self}���س�Ա�б�ʧ�ܣ�");
				return 1;
			}
			else if (qDiver.empty()) {
				reply("{self}δ����ǱˮȺ��Ա��");
				return 1;
			}
			int intCnt(0);
			ResList res;
			while (!qDiver.empty()) {
				res << (bForKick ? qDiver.top().second
						: (qDiver.top().second + to_string(qDiver.top().first) + "��"));
				if (++intCnt > 15 && intCnt > cntSize / 80)break;
				qDiver.pop();
			}
			bForKick ? reply("(.group " + to_string(llGroup) + " kick " + res.show(1))
				: reply("Ǳˮ��Ա�б�:" + res.show(1));
			return 1;
		}
		if (bool isAdmin = DD::isGroupAdmin(llGroup, fromChat.uid, true); Command == "pause") {
			if (!isAdmin && trusted < 4) {
				reply(getMsg("strPermissionDeniedErr"));
				return 1;
			}
			int secDuring(-1);
			string strDuring{ readDigit() };
			if (!strDuring.empty())secDuring = stoi(strDuring);
			DD::setGroupWholeBan(llGroup, secDuring);
			reply(getMsg("strGroupWholeBan"));
			return 1;
		}
		else if (Command == "restart") {
			if (!isAdmin && trusted < 4) {
				reply(getMsg("strPermissionDeniedErr"));
				return 1;
			}
			DD::setGroupWholeBan(llGroup, 0);
			reply(getMsg("strGroupWholeUnban"));
			return 1;
		}
		else if (Command == "card") {
			if (long long llqq = readID()) {
				if (trusted < 4 && !isAdmin && llqq != fromChat.uid) {
					reply(getMsg("strPermissionDeniedErr"));
					return 1;
				}
				if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
					reply(getMsg("strSelfPermissionErr"));
					return 1;
				}
				while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
					intMsgCnt++;
				while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
				vars["card"] = readRest();
				vars["target"] = getName(llqq, llGroup);
				DD::setGroupCard(llGroup, llqq, vars["card"].to_str());
				reply(getMsg("strGroupCardSet"));
			}
			else {
				reply(getMsg("struidEmpty"));
			}
			return 1;
		}
		else if ((!isAdmin && (!DD::isGroupOwner(llGroup, console.DiceMaid,true) || trusted < 5))) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		else if (Command == "ban") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
				reply(getMsg("strSelfPermissionErr"));
				return 1;
			}
			string QQNum = readDigit();
			if (QQNum.empty()) {
				reply(getMsg("struidEmpty"));
				return -1;
			}
			long long llMemberQQ = stoll(QQNum);
			vars["member"] = getName(llMemberQQ, llGroup);
			string strMainDice = readDice();
			if (strMainDice.empty()) {
				reply(getMsg("strValueErr"));
				return -1;
			}
			const int intDefaultDice = getUser(fromChat.uid).getConf("Ĭ����", 100);
			RD rdMainDice(strMainDice, intDefaultDice);
			rdMainDice.Roll();
			int intDuration{ rdMainDice.intTotal };
			vars["res"] = rdMainDice.FormShortString();
			DD::setGroupBan(llGroup, llMemberQQ, intDuration * 60);
			if (intDuration <= 0)
				reply(getMsg("strGroupUnban"));
			else reply(getMsg("strGroupBan"));
		}
		else if (Command == "kick") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
				reply(getMsg("strSelfPermissionErr"));
				return 1;
			}
			long long llMemberQQ = readID();
			if (!llMemberQQ) {
				reply(getMsg("struidEmpty"));
				return -1;
			}
			ResList resKicked, resDenied, resNotFound;
			do {
				if (int auth{ DD::getGroupAuth(llGroup, llMemberQQ,0) }) {
					if (auth > 1) {
						resDenied << printUser(llMemberQQ);
						continue;
					}
					DD::setGroupKick(llGroup, llMemberQQ);
					resKicked << printUser(llMemberQQ);
				}
				else resNotFound << printUser(llMemberQQ);
			} while ((llMemberQQ = readID()));
			strReply = getMsg("strSelfName");
			if (!resKicked.empty())strReply += "���Ƴ�ȺԱ��" + resKicked.show() + "\n";
			if (!resDenied.empty())strReply += "�Ƴ�ʧ�ܣ�" + resDenied.show() + "\n";
			if (!resNotFound.empty())strReply += "�Ҳ�������" + resNotFound.show();
			reply();
			return 1;
		}
		else if (Command == "title") {
			if (!DD::isGroupOwner(llGroup, console.DiceMaid,true)) {
				reply(getMsg("strSelfPermissionErr"));
				return 1;
			}
			if (long long llqq = readID()) {
				while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
					intMsgCnt++;
				while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
				vars["title"] = readRest();
				DD::setGroupTitle(llGroup, llqq, vars["title"].to_str());
				vars["target"] = getName(llqq, llGroup);
				reply(getMsg("strGroupTitleSet"));
			}
			else {
				reply(getMsg("struidEmpty"));
			}
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "reply") {
		intMsgCnt += 5;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("reply"));
			return 1;
		}
		unsigned int intMsgTmpCnt{ intMsgCnt };
		string action{ readPara() };
		if (action == "on" && fromChat.gid) {
			const string option{ (vars["option"] = "���ûظ�").to_str() };
			if (!chat(fromChat.gid).isset(option)) {
				reply(getMsg("strGroupSetOffAlready"));
			}
			else if (trusted > 0 || isAuth) {
				chat(fromChat.gid).reset(option);
				reply(getMsg("strReplyOn"));
			}
			else {
				reply(getMsg("strWhiteQQDenied"));
			}
			return 1;
		}
		else if (action == "off" && fromChat.gid) {
			const string option{ (vars["option"] = "���ûظ�").to_str() };
			if (chat(fromChat.gid).isset(option)) {
				reply(getMsg("strGroupSetOnAlready"));
			}
			else if (trusted > 0 || isAuth) {
				chat(fromChat.gid).set(option);
				reply(getMsg("strReplyOff"));
			}
			else {
				reply(getMsg("strWhiteQQDenied"));
			}
			return 1;
		}
		else if (action == "show"){
			if (trusted < 2) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			vars["key"] = readRest();
			fmt->show_reply(shared_from_this());
			return 1;
		}
		else if (action == "set") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			DiceMsgReply trigger;
			string key;
			string attr{ readToColon() };
			while (!attr.empty()) {
				if (intMsgCnt < strMsg.length() && (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':'))intMsgCnt++;
				if (attr == "Type") {	//Type=Order|Reply
					string type{ readUntilTab() };
					if(DiceMsgReply::sType.count(type))trigger.type = (DiceMsgReply::Type)DiceMsgReply::sType[type];
				}
				else if (DiceMsgReply::sMode.count(attr)) {	//Mode=Key
					trigger.mode = (DiceMsgReply::Mode)DiceMsgReply::sMode[attr];
					key = readUntilTab();
					if (trigger.mode == DiceMsgReply::Mode::Regex) {
						try
						{
							std::wregex re(convert_a2realw(key.c_str()), std::regex::ECMAScript);
						}
						catch (const std::regex_error& e)
						{
							vars["err"] = e.what();
							reply(getMsg("strRegexInvalid"));
							return -1;
						}
					}
				}
				else if (DiceMsgReply::sEcho.count(attr)) {	//Echo=Reply
					trigger.echo = (DiceMsgReply::Echo)DiceMsgReply::sEcho[attr];
					if (trigger.echo == DiceMsgReply::Echo::Deck) {
						while (intMsgCnt < strMsg.length()) {
							string item = readItem();
							if (!item.empty())trigger.deck.push_back(item);
						}
					}
					else trigger.text = readRest();
					break;
				}
				attr = readToColon();
			}
			if (key.empty()) {
				reply(getMsg("strReplyKeyEmpty"));
			}
			else {
				vars["key"] = key;
				fmt->set_reply(key, trigger);
				reply(getMsg("strReplySet"));
			}
			return 1;
		}
		else if (action == "list") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			vars["res"] = fmt->list_reply();
			reply(getMsg("strReplyList"));
			return 1;
		}
		else if (action == "del") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			vars["key"] = readRest();
			if (fmt->del_reply(vars["key"].to_str())) {
				reply(getMsg("strReplyDel"));
			}
			else {
				reply(getMsg("strReplyKeyNotFound"));
			}
			return 1;
		}
		intMsgCnt = intMsgTmpCnt;
		DiceMsgReply rep;
		if (strLowerMessage.substr(intMsgCnt, 2) == "re") {
			intMsgCnt += 2;
			rep.mode = DiceMsgReply::Mode::Regex;
		}
		if (trusted < 4) {
			reply(getMsg("strNotAdmin"));
			return -1;
		}
		const string& key{ (vars["key"] = readUntilSpace()).text };
		if (key.empty()) {
			reply(fmt->get_help("reply"));
			return -1;
		}

		if(rep.mode == DiceMsgReply::Mode::Regex)
		{
			try
			{
				std::wregex re(convert_a2realw(key.c_str()), std::regex::ECMAScript);
			}
			catch (const std::regex_error& e)
			{
				vars["err"] = e.what();
				reply(getMsg("strRegexInvalid"));
				return -1;
			}
		}
		readItems(rep.deck);
		if (rep.deck.empty()) {
			fmt->del_reply(key);
			reply(getMsg("strReplyDel"));
		}
		else {
			fmt->set_reply(key, rep);
			reply(getMsg("strReplySet"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules") {
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("rules"));
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == ':')
				intMsgCnt++;
			string strDefaultRule = strMsg.substr(intMsgCnt);
			if (strDefaultRule.empty()) {
				getUser(fromChat.uid).rmConf("Ĭ�Ϲ���");
				reply(getMsg("strRuleReset"));
			}
			else {
				for (auto& n : strDefaultRule)
					n = toupper(static_cast<unsigned char>(n));
				getUser(fromChat.uid).setConf("Ĭ�Ϲ���", strDefaultRule);
				reply(getMsg("strRuleSet"));
			}
		}
		else {
			string strSearch = strMsg.substr(intMsgCnt);
			for (auto& n : strSearch)
				n = toupper(static_cast<unsigned char>(n));
			string strReturn;
			if (getUser(fromChat.uid).confs.count("Ĭ�Ϲ���") && strSearch.find(':') == string::npos &&
				GetRule::get(getUser(fromChat.uid).confs["Ĭ�Ϲ���"].to_str(), strSearch, strReturn)) {
				reply(strReturn);
			}
			else if (GetRule::analyze(strSearch, strReturn)) {
				reply(strReturn);
			}
			else {
				reply(getMsg("strRuleErr") + strReturn);
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6") {
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2) {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10) {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		if (intNum == 0) {
			reply(getMsg("strCharacterCannotBeZero"));
			return 1;
		}
		vars["res"] = COC6(intNum);
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "deck") {
		if (trusted < 4 && console["DisabledDeck"]) {
			reply(getMsg("strDisabledDeckGlobal"));
			return 1;
		}
		intMsgCnt += 4;
		string strRoom = readDigit(false);
		long long llRoom = strRoom.empty() ? fromSession : stoll(strRoom);
		if (llRoom == 0)llRoom = fromSession;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("deck"));
			return 1;
		}
		string strPara = readPara();
		if (strPara == "show") {
			if (gm->has_session(llRoom))
				gm->session(llRoom).deck_show(this);
			else reply(getMsg("strDeckListEmpty"));
		}
		else if ((!isAuth || llRoom != fromSession) && !trusted) {
			reply(getMsg("strWhiteQQDenied"));
		}
		else if (strPara == "set") {
			gm->session(llRoom).deck_set(this);
		}
		else if (strPara == "reset") {
			gm->session(llRoom).deck_reset(this);
		}
		else if (strPara == "del") {
			gm->session(llRoom).deck_del(this);
		}
		else if (strPara == "clr") {
			gm->session(llRoom).deck_clr(this);
		}
		else if (strPara == "new") {
			gm->session(llRoom).deck_new(this);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw") {
		if (trusted < 4 && console["DisabledDraw"]) {
			reply(getMsg("strDisabledDrawGlobal"));
			return 1;
		}
		vars["option"] = "����draw";
		if (isPrivate() && groupset(fromChat.gid, vars["option"].to_str()) > 0) {
			reply(getMsg("strGroupSetOnAlready"));
			return 1;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		vector<string> ProDeck;
		vector<string>* TempDeck = nullptr;
		string& key{ (vars["deck_name"] = readAttrName()).text };
		if (key.empty()) {
			reply(fmt->get_help("draw"));
			return 1;
		}
		else {
			if (gm->has_session(fromSession) && gm->session(fromSession).has_deck(key)) {
				gm->session(fromSession)._draw(this);
				return 1;
			}
			else if (CardDeck::findDeck(key) == 0) {
				strReply = getMsg("strDeckNotFound");
				reply(strReply);
				return 1;
			}
			ProDeck = CardDeck::mPublicDeck[key];
			TempDeck = &ProDeck;
		}
		int intCardNum = 1;
		switch (readNum(intCardNum)) {
		case 0:
			if (intCardNum == 0) {
				reply(getMsg("strNumCannotBeZero"));
				return 1;
			}
			break;
		case -1: break;
		case -2:
			reply(getMsg("strParaIllegal"));
			console.log("����:" + printUser(fromChat.uid) + "��" + getMsg("strSelfName") + "ʹ���˷Ƿ�ָ�����\n" + strMsg, 1,
						printSTNow());
			return 1;
		}
		ResList Res;
		while (intCardNum--) {
			Res << CardDeck::drawCard(*TempDeck);
			if (TempDeck->empty())break;
		}
		vars["res"] = Res.dot("|").show();
		vars["cnt"] = to_string(Res.size());
    reply(getMsg("strDrawCard"));
		if (intCardNum > 0) {
			reply(getMsg("strDeckEmpty"));
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init") {
		intMsgCnt += 4;
		vars["table_name"] = "�ȹ�";
		string strCmd = readPara();
		if (strCmd.empty()|| isPrivate()) {
			reply(fmt->get_help("init"));
		}
		else if (!gm->has_session(fromSession) || !gm->session(fromSession).table_count("�ȹ�")) {
			reply(getMsg("strGMTableNotExist"));
		}
		else if (strCmd == "show" || strCmd == "list") {
			vars["res"] = gm->session(fromSession).table_prior_show("�ȹ�");
			reply(getMsg("strGMTableShow"));
		}
		else if (strCmd == "del") {
			vars["table_item"] = readRest();
			if (vars["table_item"].str_empty())
				reply(getMsg("strGMTableItemEmpty"));
			else if (gm->session(fromSession).table_del("�ȹ�", vars["table_item"].to_str()))
				reply(getMsg("strGMTableItemDel"));
			else
				reply(getMsg("strGMTableItemNotFound"));
		}
		else if (strCmd == "clr") {
			gm->session(fromSession).table_clr("�ȹ�");
			reply(getMsg("strGMTableClr"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp") {
		if (console["DisabledJrrp"]) {
			reply("&strDisabledJrrpGlobal");
			return 1;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
		if (isPrivate()) {
			if (Command == "on") {
				if (isAuth) {
					if (groupset(fromChat.gid, "����jrrp") > 0) {
						chat(fromChat.gid).reset("����jrrp");
						reply("�ɹ��ڱ�Ⱥ������JRRP!");
					}
					else {
						reply("�ڱ�Ⱥ��JRRPû�б�����!");
					}
				}
				else {
					reply(getMsg("strPermissionDeniedErr"));
				}
				return 1;
			}
			if (Command == "off") {
				if (isAuth) {
					if (groupset(fromChat.gid, "����jrrp") < 1) {
						chat(fromChat.gid).set("����jrrp");
						reply("�ɹ��ڱ�Ⱥ�н���JRRP!");
					}
					else {
						reply("�ڱ�Ⱥ��JRRPû�б�����!");
					}
				}
				else {
					reply(getMsg("strPermissionDeniedErr"));
				}
				return 1;
			}
			if (groupset(fromChat.gid, "����jrrp") > 0) {
				reply("�ڱ�Ⱥ��JRRP�����ѱ�����");
				return 1;
			}
		}
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
		vars["res"] = to_string(today->getJrrp(fromChat.uid));
		reply(getMsg("strJrrp"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "link") {
		intMsgCnt += 4;
		if (trusted < 3) {
			reply(getMsg("strNotAdmin"));
			return true;
		}
		vars["option"] = readPara();
		if (vars["option"] == "close") {
			gm->session(fromSession).link_close(this);
		}
		else if (vars["option"] == "start") {
			gm->session(fromSession).link_start(this);
		}
		else if (vars["option"] == "with" || vars["option"] == "from" || vars["option"] == "to") {
			gm->session(fromSession).link_new(this);
		}
		else {
			reply(fmt->get_help("link"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name") {
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;

		string type = readPara();
		string strNum = readDigit();
		if (strNum.length() > 1 && strNum != "10") {
			reply(getMsg("strNameNumTooBig"));
			return 1;
		}
		int intNum = strNum.empty() ? 1 : stoi(strNum);
		if (intNum == 0) {
			reply(getMsg("strNameNumCannotBeZero"));
			return 1;
		}
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("�������_" + type)) ? "�������_" + type : "�������";
		vector<string> TempDeck(CardDeck::mPublicDeck[strDeckName]);
		ResList Res;
		while (intNum--) {
			Res << CardDeck::drawCard(TempDeck, true);
		}
		vars["res"] = Res.dot("��").show();
		reply(getMsg("strNameGenerator"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "send") {
		intMsgCnt += 4;
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("send"));
			return 1;
		}
		//�ȿ���Master��������ָ��Ŀ�귢��
		if (trusted > 2) {
			chatInfo ct;
			if (!readChat(ct, true)) {
				readSkipColon();
				string strFwd(readRest());
				if (strFwd.empty()) {
					reply(getMsg("strSendMsgEmpty"));
				}
				else {
					AddMsgToQueue(strFwd, ct);
					reply(getMsg("strSendMsg"));
				}
				return 1;
			}
			else if (strLowerMessage.substr(intMsgCnt, 6) == "notice" && trusted > 3) {
				intMsgCnt += 6;
				int intLv = 0;
				string strNum{ readDigit(false) };
				while (!strNum.empty()){
					if (strNum.length() > 1)break;
					if (int intNum = stoi(strNum); intNum > 9)continue;
					else 					{
						intLv |= (1 << intNum);
					}
					if (strLowerMessage[intMsgCnt] == '+')++intMsgCnt;
					strNum = readDigit(false);
				}
				string strNotice(readRest());
				if (intLv && !strNotice.empty()){
					console.log(strNotice, intLv);
					reply(getMsg("strSendMsg"));
				}
				else reply(getMsg("strParaEmpty"));
				return 1;
			}
			readSkipColon();
		}
		else if (!console) {
			reply(getMsg("strSendMsgInvalid"));
			return 1;
		}
		else if (console["DisabledSend"] && trusted < 3) {
			reply(getMsg("strDisabledSendGlobal"));
			return 1;
		}
		string strInfo = readRest();
		if (strInfo.empty()) {
			reply(getMsg("strSendMsgEmpty"));
			return 1;
		}
		string strFwd = ((trusted > 4) ? "| " : ("| " + printFrom())) + strInfo;
		console.log(strFwd, 0b100, printSTNow());
		reply(getMsg("strSendMasterMsg"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "user") {
		intMsgCnt += 4;
		string strOption = readPara();
		if (strOption.empty())return 0;
		if (strOption == "state") {
			User& user = getUser(fromChat.uid);
			vars["user"] = printUser(fromChat.uid);
			ResList rep;
			rep << "���μ���" + to_string(trusted)
				<< "��{nick}�ĵ�һӡ���Լ����" + printDate(user.tCreated)
				<< (!(user.strNick.empty()) ? "����¼{nick}��" + to_string(user.strNick.size()) + "���ƺ�" : "û�м�¼{nick}�ĳƺ�")
				<< ((PList.count(fromChat.uid)) ? "������{nick}��" + to_string(PList[fromChat.uid].size()) + "�Ž�ɫ��" : "�޽�ɫ����¼")
				<< user.show();
			reply("{user}" + rep.show());
			return 1;
		}
		if (strOption == "trust") {
			if (trusted < 4 && fromChat.uid != console.master()) {
				reply(getMsg("strNotAdmin"));
				return 1;
			}
			string strTarget = readDigit();
			if (strTarget.empty()) {
				reply(getMsg("struidEmpty"));
				return 1;
			}
			long long llTarget = stoll(strTarget);
			if (trustedQQ(llTarget) >= trusted && !console.is_self(fromChat.uid) && fromChat.uid != llTarget) {
				reply(getMsg("strUserTrustDenied"));
				return 1;
			}
			vars["user"] = printUser(llTarget);
			vars["trust"] = readDigit();
			if (vars["trust"].str_empty()) {
				if (!UserList.count(llTarget)) {
					reply(getMsg("strUserNotFound"));
					return 1;
				}
				vars["trust"] = trustedQQ(llTarget);
				reply(getMsg("strUserTrustShow"));
				return 1;
			}
			User& user = getUser(llTarget);
			if (int intTrust = vars["trust"].to_int(); intTrust < 0 || intTrust > 255 || (intTrust >= trusted && fromChat.uid
																						   != console.master())) {
				reply(getMsg("strUserTrustIllegal"));
				return 1;
			}
			else {
				user.trust(intTrust);
			}
			reply(getMsg("strUserTrusted"));
			return 1;
		}
		if (strOption == "diss") {
			if (trusted < 4 && fromChat.uid != console.master()) {
				reply(getMsg("strNotAdmin"));
				return 1;
			}
			vars["note"] = readPara();
			long long llTargetID(readID());
			if (!llTargetID) {
				reply(getMsg("struidEmpty"));
			}
			else if (trustedQQ(llTargetID) >= trusted) {
				reply(getMsg("strUserTrustDenied"));
			}
			else {
				blacklist->add_black_qq(llTargetID, this);
				UserList.erase(llTargetID);
				PList.erase(llTargetID);
			}
			return 1;
		}
		if (strOption == "kill") {
			if (trusted < 4 && fromChat.uid != console.master()) {
				reply(getMsg("strNotAdmin"));
				return 1;
			}
			long long llTarget = readID();
			if (trustedQQ(llTarget) >= trusted && fromChat.uid != console.master()) {
				reply(getMsg("strUserTrustDenied"));
				return 1;
			}
			vars["user"] = printUser(llTarget);
			if (!llTarget || !UserList.count(llTarget)) {
				reply(getMsg("strUserNotFound"));
				return 1;
			}
			UserList.erase(llTarget);
			reply("��Ĩ��{user}���û���¼");
			return 1;
		}
		if (strOption == "clr") {
			if (trusted < 5) {
				reply(getMsg("strNotMaster"));
				return 1;
			}
			int cnt = clearUser();
			note("��������Ч������û���¼" + to_string(cnt) + "��", 0b10);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc") {
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 1 && strNum != "10") {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0) {
			reply(getMsg("strCharacterCannotBeZero"));
			return 1;
		}
		vars["res"] = COC7(intNum);
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 1 && strNum != "10") {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0) {
			reply(getMsg("strCharacterCannotBeZero"));
			return 1;
		}
		vars["res"] = DND(intNum);
		reply(getMsg("strDNDBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "log") {
		intMsgCnt += 3;
		string strPara = readPara();
		if (strPara.empty()) {
			reply(fmt->get_help("log"));
		}
		else if (DiceSession& game = gm->session(fromSession); strPara == "new") {
			game.log_new(this);
		}
		else if (strPara == "on") {
			game.log_on(this);
		}
		else if (strPara == "off") {
			game.log_off(this);
		}
		else if (strPara == "end") {
			game.log_end(this);
		}
		else {
			reply(fmt->get_help("log"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string type = readPara();
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("�������_" + type)) ? "�������_" + type : "�������";
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
		vars["new_nick"] = strip(CardDeck::drawCard(CardDeck::mPublicDeck[strDeckName], true));
		getUser(fromChat.uid).setNick(fromChat.gid, vars["new_nick"].to_str());
		reply(getMsg("strNameSet"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string& strDice{ (vars["default"] = readDigit()).text };
		while (strDice[0] == '0')
			strDice.erase(strDice.begin());
		if (strDice.empty())
			strDice = "100";
		for (auto charNumElement : strDice)
			if (!isdigit(static_cast<unsigned char>(charNumElement))) {
				reply(getMsg("strSetInvalid"));
				return 1;
			}
		if (strDice.length() > 4) {
			reply(getMsg("strSetTooBig"));
			return 1;
		}
		const int intDefaultDice = stoi(strDice);
		if (PList.count(fromChat.uid)) {
			PList[fromChat.uid][fromChat.gid]["__DefaultDice"] = intDefaultDice;
			reply(getMsg("strSetDefaultDice"));
			return 1;
		}
		if (intDefaultDice == 100)
			getUser(fromChat.uid).rmConf("Ĭ����");
		else
			getUser(fromChat.uid).setConf("Ĭ����", intDefaultDice);
		reply(getMsg("strSetDefaultDice"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "str" && trusted > 3) {
		string strName;
		while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length()
			   ) {
			strName += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		while (strMsg[intMsgCnt] == ' ')intMsgCnt++;
		if (intMsgCnt == strMsg.length() || strMsg.substr(intMsgCnt) == "show") {
			std::shared_lock lock(GlobalMsgMutex);
			const auto it = GlobalMsg.find(strName);
			if (it != GlobalMsg.end())AddMsgToQueue(it->second, fromChat);
			return 1;
		}
		string strMessage = strMsg.substr(intMsgCnt);
		if (strMessage == "reset") {
			{
				std::unique_lock lock(GlobalMsgMutex);
				EditedMsg.erase(strName);
				GlobalMsg[strName] = "";
			}
			note("�����" + strName + "���Զ��壬�����´�������ָ�Ĭ�����á�", 0b1);
		}
		else {
			{
				std::unique_lock lock(GlobalMsgMutex);
				if (strMessage == "NULL")strMessage = "";
				EditedMsg[strName] = strMessage;
				GlobalMsg[strName] = strMessage;
			}
			note("���Զ���" + strName + "���ı�", 0b1);
		}
		saveJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ak") {
	intMsgCnt += 2;
	readSkipSpace();
	if (intMsgCnt == strMsg.length()) {
		reply(fmt->get_help("ak"));
		return 1;
	}
	Session& room{ gm->session(fromSession) };
	char sign{ strMsg[intMsgCnt] };
	string action{ readPara() };
	if (sign == '#' || action == "new") {
		if (sign == '#')++intMsgCnt;
		room.get_deck().erase("__Ank");
		if (string strTitle{ strMsg.substr(intMsgCnt,strMsg.find('+') - intMsgCnt) }; !strTitle.empty()) {
			intMsgCnt += strTitle.length();
			room.setConf("AkFork", vars["fork"] = strTitle);
		}
		if (intMsgCnt == strMsg.length()) {
			reply(getMsg("strAkForkNew"));
		}
		else {
			sign = strMsg[intMsgCnt];
		}
	}
	if (sign == '+' || action == "add") {
		if (sign == '+')++intMsgCnt;
		std::vector<string>& deck{ room.get_deck("__Ank").meta };
		if (!readItems(deck)) {
			reply(getMsg("strAkAddEmpty"));
			return 1;
		}
		vars["fork"] = room.conf["AkFork"];
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		vars["li"] = list.linebreak().show();
		reply(getMsg("strAkAdd"));
		room.save();
	}
	else if (sign == '-' || action == "del") {
		if (sign == '-')++intMsgCnt;
		std::vector<string>& deck{ room.get_deck("__Ank").meta };
		int nNo{ 0 };
		if (readNum(nNo) || nNo <= 0 || nNo > deck.size()) {
			reply(getMsg("strAkNumErr"));
			return 1;
		}
		deck.erase(deck.begin() + nNo - 1);
		vars["fork"] = room.conf["AkFork"];
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		vars["li"] = list.linebreak().show();
		reply(getMsg("strAkDel"));
		room.save();
	}
	else if (sign == '=' || action == "get") {
		if (DeckInfo& deck{ room.get_deck("__Ank") }; !deck.meta.empty()) {
			vars["fork"] = room.conf["AkFork"];
			room.rmConf("AkFork");
			size_t res{ (size_t)RandomGenerator::Randint(0,deck.meta.size() - 1) };
			vars["get"] = to_string(res + 1) + ". " + deck.meta[res];
			ResList list;
			list.order();
			for (auto& val : deck.meta) {
				list << val;
			}
			vars["li"] = list.linebreak().show();
			room.get_deck().erase("__Ank");
			reply(getMsg("strAkGet"));
		}
		else {
			reply(getMsg("strAkOptEmptyErr"));
		}
		room.save();
	}
	else if (action == "show") {
		std::vector<string>& deck{ room.get_deck("__Ank").meta };
		vars["fork"] = room.conf["AkFork"];
		ResList list;
		list.order();
		for (auto& val : deck) {
			list << val;
		}
		vars["li"] = list.linebreak().show();
		reply(getMsg("strAkShow"));
	}
	else if (action == "clr") {
		room.get_deck().erase("__Ank");
		vars["fork"] = room.conf["AkFork"];
		room.rmConf("AkFork");
		reply(getMsg("strAkClr"));
		room.save();
	}
	if(strReply.empty())reply(fmt->get_help("ak"));
	return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en") {
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	if (strMsg.length() == intMsgCnt) {
		reply(fmt->get_help("en"));
		return 1;
	}
	string& strAttr{ (vars["attr"] = readAttrName()).text };
	string strCurrentValue{ readDigit(false) };
	CharaCard* pc{ PList.count(fromChat.uid) ? &getPlayer(fromChat.uid)[fromChat.gid] : nullptr };
	int intVal{ 0 };
		//��ȡ����ԭֵ
		if (strCurrentValue.empty()) {
			if (pc && !strAttr.empty() && (pc->stored(strAttr))) {
				intVal = getPlayer(fromChat.uid)[fromChat.gid][strAttr].to_int();
			}
			else {
				reply(getMsg("strEnValEmpty"));
				return 1;
			}
		}
		else {
			if (strCurrentValue.length() > 3) {
				reply(getMsg("strEnValInvalid"));
				return 1;
			}
			intVal = stoi(strCurrentValue);
		}
		readSkipSpace();
		//�ɱ�ɳ�ֵ���ʽ
		string strEnChange;
		string strEnFail;
		string strEnSuc = "1D10";
		//�ԼӼ�������ͷȷ���뼼��ֵ������
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strEnChange = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
			//û��'/'ʱĬ�ϳɹ��仯ֵ
			if (strEnChange.find('/') != std::string::npos) {
				strEnFail = strEnChange.substr(0, strEnChange.find('/'));
				strEnSuc = strEnChange.substr(strEnChange.find('/') + 1);
			}
			else strEnSuc = strEnChange;
		}
		if (strAttr.empty())strAttr = getMsg("strEnDefaultName");
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		//�ɳ��춨����������ͳ�ƣ�������춨ͳ��
		if (pc)pc->cntRollStat(intTmpRollRes, 100);
		string& res{ (vars["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intVal) + " ").text };
		if (intTmpRollRes <= intVal && intTmpRollRes <= 95) {
			if (strEnFail.empty()) {
				res += getMsg("strFailure");
				reply(getMsg("strEnRollNotChange"));
				return 1;
			}
			res += getMsg("strFailure");
			RD rdEnFail(strEnFail);
			if (rdEnFail.Roll()) {
				reply(getMsg("strValueErr"));
				return 1;
			}
			intVal = intVal + rdEnFail.intTotal;
			vars["change"] = rdEnFail.FormCompleteString();
			vars["final"] = to_string(intVal);
			reply(getMsg("strEnRollFailure"));
		}
		else {
			res += getMsg("strSuccess");
			RD rdEnSuc(strEnSuc);
			if (rdEnSuc.Roll()) {
				reply(getMsg("strValueErr"));
				return 1;
			}
			intVal = intVal + rdEnSuc.intTotal;
			vars["change"] = rdEnSuc.FormCompleteString();
			vars["final"] = to_string(intVal);
			reply(getMsg("strEnRollSuccess"));
		}
		if (pc)pc->set(strAttr, intVal);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li") {
		vars["res"] = LongInsane();
		reply(getMsg("strLongInsane"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me") {
		if (trusted < 4 && console["DisabledMe"]) {
			reply(getMsg("strDisabledMeGlobal"));
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (isPrivate()) {
			string strGroupID = readDigit();
			if (strGroupID.empty()) {
				reply(getMsg("strGroupIDEmpty"));
				return 1;
			}
			const long long llGroupID = stoll(strGroupID);
			if (groupset(llGroupID, "ͣ��ָ��") && trusted < 4) {
				reply(getMsg("strDisabledErr"));
				return 1;
			}
			if (groupset(llGroupID, "����me") && trusted < 5) {
				reply(getMsg("strMEDisabledErr"));
				return 1;
			}
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strAction = strip(readRest());
			if (strAction.empty()) {
				reply(getMsg("strActionEmpty"));
				return 1;
			}
			string strReply = (trusted > 4 ? getName(fromChat.uid, llGroupID) : "") + strAction;
			DD::sendGroupMsg(llGroupID, strReply);
			reply(getMsg("strSendSuccess"));
			return 1;
		}
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (!isAuth && (strAction == "on" || strAction == "off")) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		if (strAction == "off") {
			if (groupset(fromChat.gid, "����me") < 1) {
				chat(fromChat.gid).set("����me");
				reply(getMsg("strMeOff"));
			}
			else {
				reply(getMsg("strMeOffAlready"));
			}
			return 1;
		}
		if (strAction == "on") {
			if (groupset(fromChat.gid, "����me") > 0) {
				chat(fromChat.gid).reset("����me");
				reply(getMsg("strMeOn"));
			}
			else {
				reply(getMsg("strMeOnAlready"));
			}
			return 1;
		}
		if (groupset(fromChat.gid, "����me")) {
			reply(getMsg("strMEDisabledErr"));
			return 1;
		}
		strAction = strip(readRest());
		if (strAction.empty()) {
			reply(getMsg("strActionEmpty"));
			return 1;
		}
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
	    getPCName(vars);
		trusted > 4 ? reply(strAction) : reply(vars["pc"].to_str() + strAction);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn") {
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
		string& strNN{ (vars["new_nick"] = strip(strMsg.substr(intMsgCnt))).text };
		filter_CQcode(strNN);
		if (strNN.length() > 50) {
			reply(getMsg("strNameTooLongErr"));
			return 1;
		}
		if (!strNN.empty()) {
			getUser(fromChat.uid).setNick(fromChat.gid, strNN);
			reply(getMsg("strNameSet"));
		}
		else {
			if (getUser(fromChat.uid).rmNick(fromChat.gid)) {
				reply(getMsg("strNameClr"));
			}
			else {
				reply(getMsg("strNameDelEmpty"));
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob") {
		if (isPrivate()) {
			reply(fmt->get_help("ob"));
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string strOption = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);

		if (!isAuth && (strOption == "on" || strOption == "off")) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		vars["option"] = "����ob";
		if (strOption == "off") {
			if (groupset(fromChat.gid, vars["option"].to_str()) < 1) {
				chat(fromChat.gid).set(vars["option"].to_str());
				gm->session(fromSession).clear_ob();
				reply(getMsg("strObOff"));
			}
			else {
				reply(getMsg("strObOffAlready"));
			}
			return 1;
		}
		if (strOption == "on") {
			if (groupset(fromChat.gid, vars["option"].to_str()) > 0) {
				chat(fromChat.gid).reset(vars["option"].to_str());
				reply(getMsg("strObOn"));
			}
			else {
				reply(getMsg("strObOnAlready"));
			}
			return 1;
		}
		if (groupset(fromChat.gid, vars["option"].to_str()) > 0) {
			reply(getMsg("strObOffAlready"));
			return 1;
		}
		if (strOption == "list") {
			gm->session(fromSession).ob_list(this);
		}
		else if (strOption == "clr") {
			if (isAuth) {
				gm->session(fromSession).ob_clr(this);
			}
			else {
				reply(getMsg("strPermissionDeniedErr"));
			}
		}
		else if (strOption == "exit") {
			gm->session(fromSession).ob_exit(this);
		}
		else {
			gm->session(fromSession).ob_enter(this);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "pc") {
		intMsgCnt += 2;
		string strOption = readPara();
		if (strOption.empty()) {
			reply(fmt->get_help("pc"));
			return 1;
		}
		Player& pl = getPlayer(fromChat.uid);
		if (strOption == "tag") {
			vars["char"] = readRest();
			switch (pl.changeCard(vars["char"].to_str(), fromChat.gid)) {
			case 1:
				reply(getMsg("strPcCardReset"));
				break;
			case 0:
				reply(getMsg("strPcCardSet"));
				break;
			case -5:
				reply(getMsg("strPcNameNotExist"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "show") {
			string strName = readRest();
			CharaCard& pc{ pl.getCard(strName, fromChat.gid) };
			vars["char"] = pc.getName();
			vars["type"] = pc.Attr["__Type"].to_str();
			vars["show"] = pc.show(true);
			reply(getMsg("strPcCardShow"));
			return 1;
		}
		if (strOption == "new") {
			string& strPC{ (vars["char"] = strip(readRest())).text };
			filter_CQcode(strPC);
			switch (pl.newCard(strPC, fromChat.gid)) {
			case 0:
				vars["type"] = pl[fromChat.gid].Attr["__Type"].to_str();
				vars["show"] = pl[fromChat.gid].show(true);
				if (vars["show"].str_empty())reply(getMsg("strPcNewEmptyCard"));
				else reply(getMsg("strPcNewCardShow"));
				break;
			case -1:
				reply(getMsg("strPcCardFull"));
				break;
			case -4:
				reply(getMsg("strPcNameExist"));
				break;
			case -6:
				reply(getMsg("strPcNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "build") {
			string& strPC{ (vars["char"] = strip(readRest())).text };
			filter_CQcode(strPC);
			switch (pl.buildCard(strPC, false, fromChat.gid)) {
			case 0:
				vars["show"] = pl[strPC].show(true);
				reply(getMsg("strPcCardBuild"));
				break;
			case -1:
				reply(getMsg("strPcCardFull"));
				break;
			case -2:
				reply(getMsg("strPcTempInvalid"));
				break;
			case -6:
				reply(getMsg("strPCNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "list") {
			vars["show"] = pl.listCard();
			reply(getMsg("strPcCardList"));
			return 1;
		}
		if (strOption == "nn") {
			string& strPC{ (vars["new_name"] = strip(readRest())).text };
			filter_CQcode(strPC);
			vars["old_name"] = pl[fromChat.gid].getName();
			switch (pl.renameCard(vars["old_name"].to_str(), strPC)) {
			case 0:
				reply(getMsg("strPcCardRename"));
				break;
			case -3:
				reply(getMsg("strPCNameEmpty"));
				break;
			case -4:
				reply(getMsg("strPCNameExist"));
				break;
			case -6:
				reply(getMsg("strPCNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "del") {
			vars["char"] = strip(readRest());
			switch (pl.removeCard(vars["char"].to_str())) {
			case 0:
				reply(getMsg("strPcCardDel"));
				break;
			case -5:
				reply(getMsg("strPcNameNotExist"));
				break;
			case -7:
				reply(getMsg("strPcInitDelErr"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "redo") {
			vars["char"] = strip(readRest());
			pl.buildCard(vars["char"].text, true, fromChat.gid);
			vars["show"] = pl[vars["char"].to_str()].show(true);
			reply(getMsg("strPcCardRedo"));
			return 1;
		}
		if (strOption == "grp") {
			vars["show"] = pl.listMap();
			reply(getMsg("strPcGroupList"));
			return 1;
		}
		if (strOption == "cpy") {
			string strName = strip(readRest());
			filter_CQcode(strName);
			string& strPC1{ (vars["char1"] = strName.substr(0, strName.find('='))).text };
			vars["char2"] = (strPC1.length() + 1 < strName.length())
				? strip(strName.substr(strPC1.length() + 1))
				: pl[fromChat.gid].getName();
			switch (pl.copyCard(strPC1, vars["char2"].to_str(), fromChat.gid)) {
			case 0:
				reply(getMsg("strPcCardCpy"));
				break;
			case -1:
				reply(getMsg("strPcCardFull"));
				break;
			case -3:
				reply(getMsg("strPcNameEmpty"));
				break;
			case -6:
				reply(getMsg("strPcNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "stat") {
			CharaCard& pc{ pl[fromChat.gid] };
			bool isEmpty{ true };
			ResList res;
			int intFace{ pc.count("__DefaultDice")
				? pc.call(string("__DefaultDice"))
				: getUser(fromChat.uid).getConf("Ĭ����",100) };
			string strFace{ to_string(intFace) };
			string keyStatCnt{ "__StatD" + strFace + "Cnt" };	//��������
			if (intFace <= 100 && pc.count(keyStatCnt)) {
				int cntRoll{ pc[keyStatCnt].to_int() };
				if (cntRoll > 0) {
					isEmpty = false;
					res << "D" + strFace + "ͳ�ƴ���: " + to_string(cntRoll);
					int sumRes{ pc["__StatD" + strFace + "Sum"].to_int() };		//������
					int sumResSqr{ pc["__StatD" + strFace + "SqrSum"].to_int() };	//����ƽ����
					DiceEst stat{ intFace,cntRoll,sumRes,sumResSqr };
					if (stat.estMean > 0)
						res << "��ֵ: " + toString(stat.estMean, 2, true) + " [" + toString(stat.expMean) + "]";
					if (stat.pNormDist) {
						if (stat.pNormDist < 0.5)res << "��ֵ����" + toString(100 - stat.pNormDist * 100, 2) + "%���û�";
						else res << "��ֵ����" + toString(stat.pNormDist * 100, 2) + "%���û�";
					}
					if (stat.estStd > 0) {
						res << "��׼��: " + toString(stat.estStd, 2) + " [" + toString(stat.expStd) + "]";
					}
					/*if (stat.pZtest > 0) {
						res << "Z���顰ƫ�ġ�ˮƽ: " + toString(stat.pZtest * 100, 2) + "%";
					}*/
				}
			}
			string keyRcCnt{ "__StatRcCnt" };	//rc/sc�춨����
			if (pc.count(keyRcCnt)) {
				int cntRc{ pc["__StatRcCnt"].to_int() };
				if (cntRc > 0) {
					isEmpty = false;
					res << "�춨ͳ�ƴ���: " + to_string(cntRc);
					int sumRcSuc{ pc["__StatRcSumSuc"].to_int() };//ʵ�ʳɹ���
					int sumRcRate{ pc["__StatRcSumRate"].to_int() };//�ܳɹ���
					res << "�춨[����]�ɹ���: " + toString((double)sumRcSuc / cntRc * 100) + "%" + "(" + to_string(sumRcSuc) + ") [" + toString((double)sumRcRate / cntRc) + "%]";
					if (pc.count("__StatRcCnt5") || pc.count("__StatRcCnt96"))
						res << "5- | 96+ ������: " + toString((double)pc["__StatRcCnt5"].to_int() / cntRc * 100) + "%" + "(" + pc["__StatRcCnt5"].to_str() + ") | " + toString((double)pc["__StatRcCnt96"].to_int() / cntRc * 100) + "%" + "(" + pc["__StatRcCnt96"].to_str() + ")";
					if(pc.count("__StatRcCnt1")|| pc.count("__StatRcCnt100"))
						res << "1 | 100 ���ִ���: " + to_string(pc["__StatRcCnt1"].to_int()) + " | " + to_string(pc["__StatRcCnt100"].to_int());
				}
			}
			if (isEmpty) {
				reply(getMsg("strPcStatEmpty"));
			}
			else {
				vars["stat"] = res.show();
				reply(getMsg("strPcStatShow"));
			}
			return 1;
		}
		if (strOption == "clr") {
			PList.erase(fromChat.uid);
			reply(getMsg("strPcClr"));
			return 1;
		}
		if (strOption == "type") {
			vars["new_type"] = strip(readRest());
			if (vars["new_type"].str_empty()) {
				vars["attr"] = "ģ����";
				vars["val"] = pl[fromChat.gid].Attr["__Type"].to_str();
				reply(getMsg("strProp"));
			}
			else {
				pl[fromChat.gid].setType(vars["new_type"].to_str());
				reply(getMsg("strSetPropSuccess"));
			}
			return 1;
		}
		if (strOption == "temp") {
			CardTemp& temp{ *pl[fromChat.gid].pTemplet};
			reply(temp.show());
			return 1;
		}
		reply(fmt->get_help("pc"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra" || strLowerMessage.substr(intMsgCnt, 2) == "rc") {
		intMsgCnt += 2;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("rc"));
			return 1;
		}
		int intRule = isPrivate()
			? chat(fromChat.gid).getConf("rc����")
			: getUser(fromChat.uid).getConf("rc����");
		int intTurnCnt = 1;
		readSkipSpace();
		if (strMsg.find('#') != string::npos) {
			string strTurnCnt = strMsg.substr(intMsgCnt, strMsg.find('#') - intMsgCnt);
			//#�ܷ�ʶ����Ч
			if (strTurnCnt.empty())intMsgCnt++;
			else if ((strTurnCnt.length() == 1 && isdigit(static_cast<unsigned char>(strTurnCnt[0]))) || strTurnCnt ==
					 "10") {
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
		//D100���н�ɫ��ʱ����ͳ��
		bool isStatic = PList.count(fromChat.uid);
		CharaCard* pc{ isStatic ? &PList[fromChat.uid][fromChat.gid] : nullptr };
		if ((strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') && strLowerMessage[intMsgCnt - 1] != ' ') {
			isStatic = false;
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			vars["attr"] = getMsg("strEnDefaultName");
			reply(getMsg("strUnknownPropErr"));
			return 1;
		}
		string attr{ strMsg.substr(intMsgCnt) };
		vars["attr"] = attr;
		if (attr.find("�Զ��ɹ�") == 0) {
			strDifficulty = attr.substr(0, 8);
			attr = attr.substr(8);
			isAutomatic = true;
		}
		if (attr.find("����") == 0 || attr.find("����") == 0) {
			strDifficulty += attr.substr(0, 4);
			intDifficulty = (attr.substr(0, 4) == "����") ? 2 : 5;
			attr = attr.substr(4);
		}
		if (pc && pc->count(attr))intMsgCnt = strMsg.length();
		else {
			vars["attr"] = attr = readAttrName();
		}
		if (strLowerMessage[intMsgCnt] == '*' && isdigit(strLowerMessage[intMsgCnt + 1])) {
			intMsgCnt++;
			readNum(intSkillMultiple);
		}
		while ((strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') && isdigit(
			strLowerMessage[intMsgCnt + 1])) {
			if (!readNum(intSkillModify))strSkillModify = to_signed_string(intSkillModify);
		}
		if (strLowerMessage[intMsgCnt] == '/' && isdigit(strLowerMessage[intMsgCnt + 1])) {
			intMsgCnt++;
			readNum(intSkillDivisor);
			if (intSkillDivisor == 0) {
				reply(getMsg("strValueErr"));
				return 1;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' ||
			   strLowerMessage[intMsgCnt] ==
			   ':')
			intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			intMsgCnt++;
		}
		vars["reason"] = readRest();
		int intSkillVal;
		if (strSkillVal.empty()) {
			if (pc && pc->count(attr)) {
				intSkillVal = PList[fromChat.uid][fromChat.gid].call(attr);
			}
			else {
				if (!pc && SkillNameReplace.count(attr)) {
					vars["attr"] = SkillNameReplace[attr];
				}
				if (!pc && SkillDefaultVal.count(attr)) {
					intSkillVal = SkillDefaultVal[attr];
				}
				else {
					reply(getMsg("strUnknownPropErr"));
					return 1;
				}
			}
		}
		else if (strSkillVal.length() > 3) {
			reply(getMsg("strPropErr"));
			return 1;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		//���ճɹ��ʼ���춨ͳ��
		int intFianlSkillVal = (intSkillVal * intSkillMultiple + intSkillModify) / intSkillDivisor / intDifficulty;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000) {
			reply(getMsg("strSuccessRateErr"));
			return 1;
		}
		RD rdMainDice(strMainDice);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err) {
			reply(getMsg("strZeroDiceErr"));
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err) {
			reply(getMsg("strDiceTooBigErr"));
			return 1;
		}
		vars["attr"] = strDifficulty + attr + (
			(intSkillMultiple != 1) ? "��" + to_string(intSkillMultiple) : "") + strSkillModify + ((intSkillDivisor != 1)
																								  ? "/" + to_string(
																									  intSkillDivisor)
																								  : "");
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
	    getPCName(vars);
		if (vars["reason"].str_empty()) {
			strReply = getMsg("strRollSkill", vars);
		}
		else strReply = getMsg("strRollSkillReason", vars);
		ResList Res;
		string strAns;
		if (intTurnCnt == 1) {
			rdMainDice.Roll();
			if (isStatic) {
				pc->cntRollStat(rdMainDice.intTotal, 100);
				pc->cntRcStat(rdMainDice.intTotal, intFianlSkillVal);
			}
			strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
			int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
			switch (intRes) {
			case 0: strAns += getMsg("strRollFumble");
				break;
			case 1: strAns += isAutomatic ? getMsg("strRollRegularSuccess") : getMsg("strRollFailure");
				break;
			case 5: strAns += getMsg("strRollCriticalSuccess");
				break;
			case 4: if (intDifficulty == 1) {
				strAns += getMsg("strRollExtremeSuccess");
				break;
			}
			case 3: if (intDifficulty == 1) {
				strAns += getMsg("strRollHardSuccess");
				break;
			}
			case 2: strAns += getMsg("strRollRegularSuccess");
				break;
			}
			strReply += strAns;
		}
		else {
			Res.dot("\n");
			while (intTurnCnt--) {
				rdMainDice.Roll();
				if (isStatic) {
					pc->cntRollStat(rdMainDice.intTotal, 100);
					pc->cntRcStat(rdMainDice.intTotal, intFianlSkillVal);
				}
				strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
				int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
				switch (intRes) {
				case 0: strAns += getMsg("strFumble");
					break;
				case 1: strAns += isAutomatic ? getMsg("strSuccess") : getMsg("strFailure");
					break;
				case 5: strAns += getMsg("strCriticalSuccess");
					break;
				case 4: if (intDifficulty == 1) {
					strAns += getMsg("strExtremeSuccess");
					break;
				}
				case 3: if (intDifficulty == 1) {
					strAns += getMsg("strHardSuccess");
					break;
				}
				case 2: strAns += getMsg("strSuccess");
					break;
				}
				Res << strAns;
			}
			strReply += Res.show();
		}
		reply();
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri") {
		if (isPrivate()) {
			reply(fmt->get_help("ri"));
			return 1;
		}
		intMsgCnt += 2;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strinit += readDice();
		}
		else if (isRollDice()) {
			strinit = readDice();
		}
		readSkipSpace();
		vars["char"] = strip(strMsg.substr(intMsgCnt));
		if (vars["char"].str_empty()) {

			if (!vars.count("pc") || vars["pc"].str_empty()) {
				vars["nick"] = getName(fromChat.uid, fromChat.gid);
				getPCName(vars);
			}
			vars["char"] = vars["pc"].to_str();
		}
		RD initdice(strinit, 20);
		const int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err) {
			reply(getMsg("strValueErr"));
			return 1;
		}
		if (intFirstTimeRes == Input_Err) {
			reply(getMsg("strInputErr"));
			return 1;
		}
		if (intFirstTimeRes == ZeroDice_Err) {
			reply(getMsg("strZeroDiceErr"));
			return 1;
		}
		if (intFirstTimeRes == ZeroType_Err) {
			reply(getMsg("strZeroTypeErr"));
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err) {
			reply(getMsg("strDiceTooBigErr"));
			return 1;
		}
		if (intFirstTimeRes == TypeTooBig_Err) {
			reply(getMsg("strTypeTooBigErr"));
			return 1;
		}
		if (intFirstTimeRes == AddDiceVal_Err) {
			reply(getMsg("strAddDiceValErr"));
			return 1;
		}
		if (intFirstTimeRes != 0) {
			reply(getMsg("strUnknownErr"));
			return 1;
		}
		gm->session(fromSession).table_add("�ȹ�", initdice.intTotal, vars["char"].to_str());
		vars["res"] = initdice.FormCompleteString();
		reply(getMsg("strRollInit"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc") {
		intMsgCnt += 2;
		string SanCost = readUntilSpace();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (SanCost.empty()) {
			reply(fmt->get_help("sc"));
			return 1;
		}
		if (SanCost.find('/') == string::npos) {
			reply(getMsg("strSanCostInvalid"));
			return 1;
		}
		string attr = "����";
		int intSan = 0;
		CharaCard* pc{ PList.count(fromChat.uid) ? &getPlayer(fromChat.uid)[fromChat.gid] : nullptr };
		if (readNum(intSan)) {
			if (pc && pc->count(attr)) {
				intSan = pc->call(attr);
			}
			else {
				reply(getMsg("strSanEmpty"));
				return 1;
			}
		}
		string strSanCostSuc = SanCost.substr(0, SanCost.find('/'));
		string strSanCostFail = SanCost.substr(SanCost.find('/') + 1);
		for (const auto& character : strSanCostSuc) {
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-') {
				reply(getMsg("strSanCostInvalid"));
				return 1;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find('/') + 1)) {
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-') {
				reply(getMsg("strSanCostInvalid"));
				return 1;
			}
		}
		RD rdSuc(strSanCostSuc);
		RD rdFail(strSanCostFail);
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0) {
			reply(getMsg("strSanCostInvalid"));
			return 1;
		}
		if (intSan <= 0) {
			reply(getMsg("strSanInvalid"));
			return 1;
		}
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		//���Ǽ춨����ͳ��
		if (pc) {
			pc->cntRollStat(intTmpRollRes, 100);
			pc->cntRcStat(intTmpRollRes, intSan);
		}
		string& strRes{ (vars["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intSan) + " ").text };
		//���÷���
		int intRule = fromChat.gid
			? chat(fromChat.gid).getConf("rc����")
			: getUser(fromChat.uid).getConf("rc����");
		switch (RollSuccessLevel(intTmpRollRes, intSan, intRule)) {
		case 5:
		case 4:
		case 3:
		case 2:
			strRes += getMsg("strSuccess");
			vars["change"] = rdSuc.FormCompleteString();
			intSan = max(0, intSan - rdSuc.intTotal);
			break;
		case 1:
			strRes += getMsg("strFailure");
			vars["change"] = rdFail.FormCompleteString();
			intSan = max(0, intSan - rdFail.intTotal);
			break;
		case 0:
			strRes += getMsg("strFumble");
			rdFail.Max();
			vars["change"] = rdFail.strDice + "���ֵ=" + to_string(rdFail.intTotal);
			intSan = max(0, intSan - rdFail.intTotal);
			break;
		}
		vars["final"] = to_string(intSan);
		if (pc)pc->set(attr, intSan);
		reply(getMsg("strSanRollRes"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st") {
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length()) {
			reply(fmt->get_help("st"));
			return 1;
		}
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
		getPCName(vars);
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr") {
			if (!PList.count(fromChat.uid)) {
				reply(getMsg("strPcNotExistErr"));
				return 1;
			}
			getPlayer(fromChat.uid)[fromChat.gid].clear();
			vars["char"] = getPlayer(fromChat.uid)[fromChat.gid].getName();
			reply(getMsg("strPropCleared"));
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			if (!PList.count(fromChat.uid)) {
				reply(getMsg("strPcNotExistErr"));
				return 1;
			}
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			if (strMsg[intMsgCnt] == '&') {
				intMsgCnt++;
			}
			vars["attr"] = readAttrName();
			if (getPlayer(fromChat.uid)[fromChat.gid].erase(vars["attr"].text)) {
				reply(getMsg("strPropDeleted"));
			}
			else {
				reply(getMsg("strPropNotFound"));
			}
			return 1;
		}
		CharaCard& pc = getPlayer(fromChat.uid)[fromChat.gid];
		if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			vars["attr"] = readAttrName();
			if (vars["attr"].str_empty()) {
				vars["char"] = pc.getName();
				vars["type"] = pc.Attr["__Type"].to_str();
				vars["show"] = pc.show(false);
				reply(getMsg("strPropList"));
				return 1;
			}
			if (string val; pc.show(vars["attr"].to_str(), val) > -1) {
				vars["val"] = val;
				reply(getMsg("strProp"));
			}
			else {
				reply(getMsg("strPropNotFound"));
			}
			return 1;
		}
		bool boolError = false;
		bool isDetail = false;
		bool isModify = false;
		//ѭ��¼��
		int cntInput{ 0 };
		while (intMsgCnt != strLowerMessage.length()) {
			readSkipSpace();
			//�ж�¼����ʽ
			if (strMsg[intMsgCnt] == '&') {
				vars["attr"] = readToColon();
				if (vars["attr"].str_empty()) {
					continue;
				}
				if (pc.set(vars["attr"].to_str(), readExp())) {
					reply(getMsg("strPcTextTooLong"));
					return 1;
				}
				++cntInput;
				continue;
			}
			//��ȡ������
			string strSkillName = readAttrName();
			if (strSkillName.empty()) {
				readSkipSpace();
				while (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '+' ||
			           strMsg[intMsgCnt] == '-' || strMsg[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/')
				{
					intMsgCnt++;
				}
				readDigit(false);
				continue;
			}
			strSkillName = pc.standard(strSkillName);
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
				'=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			//�ж���¼��Ϊ�ı�
			if (bool isSqr{ strMsg.substr(intMsgCnt, 2) == "��" }; pc.pTemplet->sInfoList.count(strSkillName) || isSqr) {
				string strVal;
				if (auto pos{ strMsg.find("��",intMsgCnt) }; pos != string::npos) {
					strVal = strMsg.substr(intMsgCnt + 2, pos - intMsgCnt - 2);
					intMsgCnt = pos + 2;
				}
				else {
					strVal = readUntilTab();
				}
				if (pc.set(strSkillName, strVal)) {
					reply(getMsg("strPcTextTooLong"));
					return 1;
				}
				++cntInput;
				continue;
			}
			if (strSkillName == "note") {
				if (pc.setNote(readRest())) {
					reply(getMsg("strPcNoteTooLong"));
					return 1;
				}
				++cntInput;
				break;
			}
			readSkipSpace();
			//�ж���ֵ�޸�
			if ((strLowerMessage[intMsgCnt] == '-' || strLowerMessage[intMsgCnt] == '+')) {
				isDetail = true;
				isModify = true;
				AttrVar& nVal{ pc[strSkillName] };
				RD Mod((nVal.to_int() == 0 ? "" : nVal.to_str()) + readDice());
				if (Mod.Roll()) {
					reply(getMsg("strValueErr"));
					return 1;
				}
				strReply += "\n" + strSkillName + "��" + Mod.FormCompleteString();
				if (Mod.intTotal < -32767) {
					strReply += "��-32767";
					nVal = -32767;
				}
				else if (Mod.intTotal > 32767) {
					strReply += "��32767";
					nVal = 32767;
				}
				else nVal = Mod.intTotal;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
					   '|')intMsgCnt++;
				++cntInput;
				continue;
			}
			string strSkillVal = readDigit();
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 5) {
				boolError = true;
				break;
			}
			int intSkillVal = std::clamp(stoi(strSkillVal), -32767, 32767);
			//¼�봿��ֵ
			pc.set(strSkillName, intSkillVal);
			++cntInput;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
				intMsgCnt++;
		}
		if (boolError) {
			reply(getMsg("strPropErr"));
		}
		else if (isModify) {
			vars["change"] = strReply;
			reply(getMsg("strStModify"));
		}
		else if(cntInput){
			vars["cnt"] = to_string(cntInput);
			reply(getMsg("strSetPropSuccess"));
		}
		else {
			reply(fmt->get_help("st"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti") {
		vars["res"] = TempInsane();
		reply(getMsg("strTempInsane"));
		return 1;
	}
	else if (strLowerMessage[intMsgCnt] == 'w') {
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w') {
			intMsgCnt++;
			boolDetail = true;
		}
		readSkipSpace();
		const unsigned int len{ (unsigned int)strMsg.length() };
		if (intMsgCnt == len) {
			reply(fmt->get_help("ww"));
			return 1;
		}
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
		getPCName(vars);
		CharaCard* pc{ PList.count(fromChat.uid) ? &getPlayer(fromChat.uid)[fromChat.gid] : nullptr };
		string strMainDice;
		string& strReason{ (vars["reason"] = "").text };
		string strAttr;
		if (pc) {	//���ý�ɫ�����Ի���ʽ
			while (intMsgCnt < len && !isspace(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
				if (isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))
					|| strMsg[intMsgCnt] == 'a'
					|| strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-'
					|| strMsg[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/') {
					strMainDice += strMsg[intMsgCnt++];
				}
				else if (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':') {
					intMsgCnt++;
				}
				else {
					strAttr = readAttrName();
					strMainDice += pc->getExp(strAttr);
					if (!pc->count("&" + strAttr) && (*pc)[strAttr].type == AttrVar::AttrType::Integer)strMainDice += 'a';
				}
			}
		}
		else {
			strMainDice = readDice(); 	//ww�ı��ʽ�����Ǵ�����
		}
		strReason = readRest();
		int intTurnCnt = 1;
		const int intDefaultDice = getUser(fromChat.uid).getConf("Ĭ����", 100);
		//����.ww[����]#[���ʽ]
		if (size_t pos{ strMainDice.find('#') }; pos != string::npos) {
			string strTurnCnt = strMainDice.substr(0, pos);
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(pos + 1);
			RD rdTurnCnt(strTurnCnt, intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes != 0) {
				if (intRdTurnCntRes == Value_Err) {
					reply(getMsg("strValueErr"));
					return 1;
				}
				if (intRdTurnCntRes == Input_Err) {
					reply(getMsg("strInputErr"));
					return 1;
				}
				if (intRdTurnCntRes == ZeroDice_Err) {
					reply(getMsg("strZeroDiceErr"));
					return 1;
				}
				if (intRdTurnCntRes == ZeroType_Err) {
					reply(getMsg("strZeroTypeErr"));
					return 1;
				}
				if (intRdTurnCntRes == DiceTooBig_Err) {
					reply(getMsg("strDiceTooBigErr"));
					return 1;
				}
				if (intRdTurnCntRes == TypeTooBig_Err) {
					reply(getMsg("strTypeTooBigErr"));
					return 1;
				}
				if (intRdTurnCntRes == AddDiceVal_Err) {
					reply(getMsg("strAddDiceValErr"));
					return 1;
				}
				reply(getMsg("strUnknownErr"));
				return 1;
			}
			if (rdTurnCnt.intTotal > 10) {
				reply(getMsg("strRollTimeExceeded"));
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0) {
				reply(getMsg("strRollTimeErr"));
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find('d') != string::npos) {
				string strTurnNotice = vars["pc"].to_str() + "����������: " + rdTurnCnt.FormShortString() + "��";
				reply(strTurnNotice);
			}
		}
		if (strMainDice.empty()) {
			reply(fmt->get_help("ww"));
			return 1;
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
												 ? strMainDice.find('+')
												 : strMainDice.find('-'));
		strFirstDice = strFirstDice.substr(0, strFirstDice.find('x') < strFirstDice.find('*')
										   ? strFirstDice.find('x')
										   : strFirstDice.find('*'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice) {
			if (!isdigit(static_cast<unsigned char>(i))) {
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, intDefaultDice);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes != 0) {
			if (intFirstTimeRes == Value_Err) {
				reply(getMsg("strValueErr"));
				return 1;
			}
			if (intFirstTimeRes == Input_Err) {
				reply(getMsg("strInputErr"));
				return 1;
			}
			if (intFirstTimeRes == ZeroDice_Err) {
				reply(getMsg("strZeroDiceErr"));
				return 1;
			}
			if (intFirstTimeRes == ZeroType_Err) {
				reply(getMsg("strZeroTypeErr"));
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err) {
				reply(getMsg("strDiceTooBigErr"));
				return 1;
			}
			if (intFirstTimeRes == TypeTooBig_Err) {
				reply(getMsg("strTypeTooBigErr"));
				return 1;
			}
			if (intFirstTimeRes == AddDiceVal_Err) {
				reply(getMsg("strAddDiceValErr"));
				return 1;
			}
			reply(getMsg("strUnknownErr"));
			return 1;
		}
		if (!boolDetail && intTurnCnt != 1) {
			if (strReason.empty())strReply = getMsg("strRollMuiltDice");
			else strReply = getMsg("strRollMuiltDiceReason");
			vector<int> vintExVal;
			string& strRes{ (vars["res"] = "{ ").text };
			while (intTurnCnt--) {
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strRes += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					vars["res"] = ",";
			}
			strRes += " }";
			if (!vintExVal.empty()) {
				strRes += ",��ֵ: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it) {
					strRes += to_string(*it);
					if (it != vintExVal.cend() - 1)strRes += ",";
				}
			}
			reply();
		}
		else {
			while (intTurnCnt--) {
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				vars["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
				if (strReason.empty())
					strReply = getMsg("strRollDice");
				else strReply = getMsg("strRollDiceReason");
        reply();
			}
		}
		return 1;
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'h') {
		vars["nick"] = getName(fromChat.uid, fromChat.gid);
		getPCName(vars);
		intMsgCnt += 1;
		bool boolDetail = true;
		if (strMsg[intMsgCnt] == 's') {
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h') {
			intMsgCnt += 1;
		}
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		CharaCard* pc{ PList.count(fromChat.uid) ? &getPlayer(fromChat.uid)[fromChat.gid] : nullptr };
		string& strReason{ (vars["reason"] = strMsg.substr(intMsgCnt)).text };
		if (strReason.empty()) {
			string key{ "__DefaultDiceExp" };
			if (pc && pc->countExp(key)) {
				strMainDice = pc->getExp(key);
			}
		}
		else if (pc && pc->countExp(strReason)) {
			strMainDice = pc->getExp(strReason);
		}
		else {
			strMainDice = readDice();
			bool isExp = false;
			for (auto ch : strMainDice) {
				if (!isdigit(ch)) {
					isExp = true;
					break;
				}
			}
			if (isExp)vars["reason"] = readRest();
			else strMainDice.clear();
		}
		int intTurnCnt = 1;
		const int intDefaultDice = (pc && pc->count("__DefaultDice"))
			? (*pc)["__DefaultDice"].to_int()
			: getUser(fromChat.uid).getConf("Ĭ����", 100);
		if (strMainDice.find('#') != string::npos) {
			string& turn{ (vars["turn"] = strMainDice.substr(0, strMainDice.find('#'))).text };
			if (turn.empty())
				turn = "1";
			strMainDice = strMainDice.substr(strMainDice.find('#') + 1);
			RD rdTurnCnt(turn, intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			switch (intRdTurnCntRes) {
			case 0: break;
			case Value_Err:
				reply(getMsg("strValueErr"));
				return 1;
			case Input_Err:
				reply(getMsg("strInputErr"));
				return 1;
			case ZeroDice_Err:
				reply(getMsg("strZeroDiceErr"));
				return 1;
			case ZeroType_Err:
				reply(getMsg("strZeroTypeErr"));
				return 1;
			case DiceTooBig_Err:
				reply(getMsg("strDiceTooBigErr"));
				return 1;
			case TypeTooBig_Err:
				reply(getMsg("strTypeTooBigErr"));
				return 1;
			case AddDiceVal_Err:
				reply(getMsg("strAddDiceValErr"));
				return 1;
			default:
				reply(getMsg("strUnknownErr"));
				return 1;
			}
			if (rdTurnCnt.intTotal > 10) {
				reply(getMsg("strRollTimeExceeded"));
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0) {
				reply(getMsg("strRollTimeErr"));
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (turn.find('d') != string::npos) {
				turn = rdTurnCnt.FormShortString();
        reply(getMsg("strRollTurn"));
      }
		}
		if (strMainDice.empty() && pc && pc->countExp(strReason)) {
			strMainDice = pc->getExp(strReason);
		}
		RD rdMainDice(strMainDice, intDefaultDice);
		const int intFirstTimeRes = rdMainDice.Roll();
		switch (intFirstTimeRes) {
		case 0: break;
		case Value_Err:
			reply(getMsg("strValueErr"));
			return 1;
		case Input_Err:
			reply(getMsg("strInputErr"));
			return 1;
		case ZeroDice_Err:
			reply(getMsg("strZeroDiceErr"));
			return 1;
		case ZeroType_Err:
			reply(getMsg("strZeroTypeErr"));
			return 1;
		case DiceTooBig_Err:
			reply(getMsg("strDiceTooBigErr"));
			return 1;
		case TypeTooBig_Err:
			reply(getMsg("strTypeTooBigErr"));
			return 1;
		case AddDiceVal_Err:
			reply(getMsg("strAddDiceValErr"));
			return 1;
		default:
			reply(getMsg("strUnknownErr"));
			return 1;
		}
		vars["dice_exp"] = rdMainDice.strDice;
		//��ͳ����Ĭ����һ�µ�����
		bool isStatic{ intDefaultDice <= 100 && pc && rdMainDice.strDice == ("D" + to_string(intDefaultDice)) };
		string strType = (intTurnCnt != 1
						  ? (vars["reason"].str_empty() ? "strRollMultiDice" : "strRollMultiDiceReason")
						  : (vars["reason"].str_empty() ? "strRollDice" : "strRollDiceReason"));
		if (!boolDetail && intTurnCnt != 1) {
			strReply = getMsg(strType);
			vector<int> vintExVal;
			string& res{ (vars["res"] = "{ ").text };
			while (intTurnCnt--) {
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
				res += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					res += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
																						rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			res += " }";
			if (!vintExVal.empty()) {
				res += ",��ֵ: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it) {
					res += to_string(*it);
					if (it != vintExVal.cend() - 1)
						res += ",";
				}
			}
			reply();
		}
		else {
			ResList dices;
			if (intTurnCnt > 1) {
				while (intTurnCnt--) {
					rdMainDice.Roll();
					if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
					string strForm = to_string(rdMainDice.intTotal);
					if (boolDetail) {
						string strCombined = rdMainDice.FormStringCombined();
						string strSeparate = rdMainDice.FormStringSeparate();
						if (strCombined != strForm)strForm = strCombined + "=" + strForm;
						if (strSeparate != strMainDice && strSeparate != strCombined)strForm = strSeparate + "=" +
							strForm;
					}
					dices << strForm;
				}
				vars["res"] = dices.dot(", ").line(7).show();
			}
			else {
				if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
				vars["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
			}
			strReply = getMsg(strType);
      reply();
		}
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
	bool isSummoned{ false };
	string strAt{ CQ_AT + to_string(console.DiceMaid) + "]" };
	size_t r{ 0 };
	while ((r = strMsg.find(']')) != string::npos && strMsg.find(CQ_AT) == 0 || strMsg.find(CQ_QQAT) == 0)
	{
		if (string strTarget{ strMsg.substr(10,r - 10) }; strTarget == "all") {
			isCalled = true;
		}
		else if (strTarget == to_string(console.DiceMaid))
		{
			isCalled = true;
		}
		else if (User& self{ getUser(console.DiceMaid) }; self.confs.count("tinyID") && self.confs["tinyID"] == strTarget)
		{
			isCalled = true;
		}
		else {
			isOtherCalled = true;
		}
		strMsg = strMsg.substr(r + 1);
		while (isspace(static_cast<unsigned char>(strMsg[0])))
			strMsg.erase(strMsg.begin());
	}
	string strSummon{ getMsg("strSummonWord") };
	if (!strSummon.empty() && strMsg.find(strSummon) == 0) {
		isSummoned = true;
		if(isChannel())strMsg = strMsg.substr(strSummon.length());
	}
	init2(strMsg);
	strLowerMessage = strMsg;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(),
				   [](unsigned char c) { return tolower(c); });
	trusted = trustedQQ(fromChat.uid);
	fwdMsg();
	if (isOtherCalled && !isCalled)return false;
	if (isPrivate()) isCalled = true;
	isDisabled = ((console["DisabledGlobal"] && trusted < 4) || groupset(fromChat.gid, "Э����Ч") > 0);
	if (BasicOrder())
	{
		if (isAns) {
			if (!isVirtual) {
				AddFrq(fromChat, fromTime, strMsg);
				getUser(fromChat.uid).update(fromTime);
				if (isPrivate())chat(fromChat.gid).update(fromTime);
			}
		}
		return 1;
	}
	if (!isPrivate() && ((console["CheckGroupLicense"] > 0 && pGrp->isset("δ���"))
											  || (console["CheckGroupLicense"] == 2 && !pGrp->isset("���ʹ��"))
											  || blacklist->get_group_danger(fromChat.gid))) {
		isDisabled = true;
	}
	if (blacklist->get_qq_danger(fromChat.uid))isDisabled = true;
	if (isDisabled)return console["DisabledBlock"];
	if (int chon{ isChannel() && pGrp ? pGrp->getChConf(fromChat.chid,"order",0):0 }; isCalled || chon > 0 ||
		!(chon || pGrp->isset("ͣ��ָ��"))) {
		if (fmt->listen_order(this) || InnerOrder()) {
			if (!isVirtual) {
				AddFrq(fromChat, fromTime,  strMsg);
				getUser(fromChat.uid).update(fromTime);
				if (!isPrivate())chat(fromChat.gid).update(fromTime);
			}
			return true;
		}
	}
	if (fmt->listen_reply(this))return true;
	if (isSummoned && (strMsg.empty() || strMsg == strSummon))reply(getMsg("strSummonEmpty"));
	return false;
}
bool FromMsg::WordCensor() {
	//����С��4���û��������дʼ��
	if (trusted < 4) {
		vector<string>sens_words;
		switch (int danger = censor.search(strMsg, sens_words) - 1) {
		case 3:
			if (trusted < danger++) {
				console.log("����:" + printUser(fromChat.uid) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 0b1000,
							printTTime(fromTime));
				reply(getMsg("strCensorDanger"));
				return 1;
			}
		case 2:
			if (trusted < danger++) {
				console.log("����:" + printUser(fromChat.uid) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 0b10,
							printTTime(fromTime));
				reply(getMsg("strCensorWarning"));
				break;
			}
		case 1:
			if (trusted < danger++) {
				console.log("����:" + printUser(fromChat.uid) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 0b10,
							printTTime(fromTime));
				reply(getMsg("strCensorCaution"));
				break;
			}
		case 0:
			console.log("����:" + printUser(fromChat.uid) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 1,
						printTTime(fromTime));
			break;
		default:
			break;
		}
	}
	return false;
}

void FromMsg::virtualCall() {
	isVirtual = true;
	isCalled = true;
	DiceFilter();
}

int FromMsg::getGroupAuth(long long group) {
	if (trusted > 0)return trusted;
	if (ChatList.count(group)) {
		return DD::isGroupAdmin(group, fromChat.uid, true) ? 0 : -1;
	}
	return -2;
}
void FromMsg::readSkipColon() {
	readSkipSpace();
	while (intMsgCnt < strMsg.length() && (strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '='))intMsgCnt++;
}

int FromMsg::readNum(int& num)
{
	string strNum;
	while (intMsgCnt < strMsg.length() && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])) && strMsg[intMsgCnt] != '-')intMsgCnt++;
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
	if (strNum.empty() || strNum == "-")return -3;
	num = stoi(strNum);
	return 0;
}

int FromMsg::readChat(chatInfo& ct, bool isReroll)
{
	const int intFormor = intMsgCnt;
	if (const string strT = readPara(); strT == "me")
	{
		ct = { fromChat.uid, 0,0 };
		return 0;
	}
	else if (strT == "this")
	{
		ct = fromChat;
		return 0;
	}
	else if (const long long llID{ readID() }; !llID) {
		if (isReroll)intMsgCnt = intFormor;
		return -2;
	}
	else if (strT == "qq")
	{
		ct.type = msgtype::Private;
		ct.uid = llID;
		return 0;
	}
	else if (strT == "group")
	{
		ct.type = msgtype::Group;
		ct.gid = llID;
		return 0;
	}
	if (isReroll)intMsgCnt = intFormor;
	return -1;
}

string FromMsg::readItem()
{
	size_t len{ strMsg.length() };
	if (intMsgCnt >= len)return{};
	while (intMsgCnt < len &&
		(isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == '|'))intMsgCnt++;
	size_t intBegin{ intMsgCnt };
	size_t intEnd{ strMsg.find('|',intMsgCnt) };
	if (intEnd > len) {
		intMsgCnt = intEnd = len;
	}
	else intMsgCnt = intEnd + 1;
	intEnd = strMsg.find_last_not_of(" \t\r\n", intEnd - 1) + 1;
	//while (isspace(static_cast<unsigned char>(strMsg[intEnd - 1])))--intEnd;
	return strMsg.substr(intBegin, intEnd - intBegin);
}
int FromMsg::readItems(vector<string>& vItem) {
	int cnt{ 0 };
	string strItem;
	while (intMsgCnt < strMsg.length() && !(strItem = readItem()).empty()) {
		vItem.push_back(strItem);
		++cnt;
	}
	return cnt;
}

std::string FromMsg::printFrom()
{
	std::string strFwd;
	if (!isPrivate())strFwd += isChannel()
		? ("[Ƶ��:" + to_string(fromChat.gid) + "]")
		: ("[Ⱥ:" + to_string(fromChat.gid) + "]");
	strFwd += getName(fromChat.uid, fromChat.gid) + "(" + to_string(fromChat.uid) + "):";
	return strFwd;
}
