/*
 * _______   ________  ________  ________  __
 * |  __ \  |__  __| |  _____| |  _____| | |
 * | | | |   | |   | |    | |_____  | |
 * | | | |   | |   | |    |  _____| |__|
 * | |__| |  __| |__  | |_____  | |_____  __
 * |_______/  |________| |________| |________| |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2020 w4123���
 * Copyright (C) 2019-2020 String.Empty
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
#include <ctime>
#include <queue>
#include <mutex>
#include <chrono>
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "ManagerSystem.h"
#include "DiceNetwork.h"
#include "DiceCloud.h"
#include "Jsonio.h"
#include "BlackListManager.h"

using namespace std;
using namespace CQ;

const std::map<std::string, int, less_ci>Console::intDefault{
{"DisabledGlobal",0},{"DisabledBlock",0},{"DisabledListenAt",1},
{"DisabledMe",1},{"DisabledJrrp",0},{"DisabledDeck",1},{"DisabledDraw",0},{"DisabledSend",0},
{"Private",0},{"CheckGroupLicense",0},{"LeaveDiscuss",0},
{"ListenGroupRequest",1},{"ListenGroupAdd",1},
{"ListenFriendRequest",1},{"ListenFriendAdd",1},{"AllowStranger",1},
{"AutoClearBlack",1},{"LeaveBlackQQ",0},
{"ListenGroupKick",1},{"ListenGroupBan",1},{"ListenSpam",1},
{"BannedLeave",0},{"BannedBanInviter",0},
{"KickedBanInviter",0},
{"GroupClearLimit",20},
{"CloudBlackShare",1},{"BelieveDiceList",0},{"CloudVisible",1},
{"SystemAlarmCPU",90},{"SystemAlarmRAM",90},{"SystemAlarmDisk",90},
{"SendIntervalIdle",500},{"SendIntervalBusy",100},
//�Զ������¼����[min],�Զ�ͼƬ������[h]
{"AutoSaveInterval",10},{"AutoClearImage",0}
};
const enumap<string> Console::mClockEvent{"off", "on", "save", "clear"};

int Console::setClock(Clock c, ClockEvent e)
{
	if (c.first > 23 || c.second > 59)return -1;
	if (static_cast<int>(e) > 3)return -2;
	mWorkClock.emplace(c, e);
	save();
	return 0;
}

int Console::rmClock(Clock c, ClockEvent e)
{
	if (const auto it = match(mWorkClock, c, e); it != mWorkClock.end())
	{
		mWorkClock.erase(it);
		save();
		return 0;
	}
	return -1;
}

ResList Console::listClock() const
{
	ResList list;
	for (const auto& [clock, eve] : mWorkClock)
	{
		string strClock = printClock(clock);
		switch (eve)
		{
		case ClockEvent::on:
			strClock += " ��ʱ����";
			break;
		case ClockEvent::off:
			strClock += " ��ʱ�ر�";
			break;
		case ClockEvent::save:
			strClock += " ��ʱ����";
			break;
		case ClockEvent::clear:
			strClock += " ��ʱ��Ⱥ";
			break;
		default: break;
		}
		list << strClock;
	}
	return list;
}

ResList Console::listNotice() const
{
	ResList list;
	for (const auto& [ct,lv] : NoticeList)
	{
		list << printChat(ct) + " " + to_binary(lv);
	}
	return list;
}

int Console::showNotice(chatType ct) const
{
	if (const auto it = NoticeList.find(ct); it != NoticeList.end())return it->second;
	return 0;
}

void Console::addNotice(chatType ct, int lv)
{
	NoticeList[ct] |= lv;
	saveNotice();
}

void Console::redNotice(chatType ct, int lv)
{
	NoticeList[ct] &= (~lv);
	saveNotice();
}

void Console::setNotice(chatType ct, int lv)
{
	NoticeList[ct] = lv;
	saveNotice();
}

void Console::rmNotice(chatType ct)
{
	NoticeList.erase(ct);
	saveNotice();
}

int Console::log(const std::string& strMsg, int note_lv, const string& strTime)
{
	ofstream fout(string(DiceDir + "\\audit\\log") + to_string(DiceMaid) + "_" + printDate() + ".txt",
	              ios::out | ios::app);
	fout << strTime << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
	fout.close();
	int Cnt = 0;
	const string note = strTime.empty() ? strMsg : (strTime + " " + strMsg);
	if (note_lv)
	{
		for (auto& [ct, level] : NoticeList)
		{
			if (!(level & note_lv))continue;
			AddMsgToQueue(note, ct);
			Cnt++;
		}
		if (!Cnt)sendPrivateMsg(DiceMaid, note);
	}
	return Cnt;
} 
void Console::newMaster(long long qq)
{
	masterQQ = qq; 
	getUser(qq).trust(5); 
	setNotice({qq, CQ::msgtype::Private}, 0b111111);
	save(); 
	AddMsgToQueue(getMsg("strNewMaster"), qq); 
}

void Console::reset()
{
	intConf.clear();
	mWorkClock.clear();
	NoticeList.clear();
}

void Console::loadNotice()
{
	if (loadFile(DiceDir + "\\conf\\NoticeList.txt", NoticeList) < 1)
	{
		std::set<chatType> sChat;
		if (loadFile(static_cast<string>(getAppDirectory()) + "MonitorList.RDconf", sChat) > 0)
			for (const auto& it : sChat)
			{
				console.setNotice(it, 0b100000);
			}
		sChat.clear();
		if (loadFile(DiceDir + "\\conf\\RecorderList.RDconf", sChat) > 0)
			for (const auto& it : sChat)
			{
				console.setNotice(it, 0b11011);
			}
		console.setNotice({863062599, msgtype::Group}, 0b100000);
		console.setNotice({192499947, msgtype::Group}, 0b100000);
		console.setNotice({754494359, msgtype::Group}, 0b100000);
		for (auto& [ct, lv] : NoticeList)
		{
			if (ct.second != msgtype::Private)
			{
				chat(ct.first).set("���ʹ��").set("����").set("���");
			}
		}
	}
}

void Console::saveNotice() const
{
	saveFile(DiceDir + "\\conf\\NoticeList.txt", NoticeList);
}

Console console;

//DiceModManager modules{};
//����Ⱥ�б�
std::set<long long> ExceptGroups;
//�����б�
std::map<long long, long long> mDiceList;

//��������ʱ��
long long llStartTime = clock();

//��ǰʱ��
SYSTEMTIME stNow{};
SYSTEMTIME stTmp{};

std::string printSTNow()
{
	GetLocalTime(&stNow);
	return printSTime(stNow);
}

std::string printDate()
{
	return to_string(stNow.wYear) + "-" + (stNow.wMonth < 10 ? "0" : "") + to_string(stNow.wMonth) + "-" + (
		stNow.wDay < 10 ? "0" : "") + to_string(stNow.wDay);
}

std::string printDate(time_t tt)
{
	tm t{};
	if (!tt || localtime_s(&t, &tt))return R"(????-??-??)";
	return to_string(t.tm_year + 1900) + "-" + to_string(t.tm_mon + 1) + "-" + to_string(t.tm_mday);
}

//�ϰ�ʱ��
std::pair<int, int> ClockToWork = {24, 0};
//�°�ʱ��
std::pair<int, int> ClockOffWork = {24, 0};

string printClock(std::pair<int, int> clock)
{
	string strClock = to_string(clock.first);
	strClock += ":";
	if (clock.second < 10)strClock += "0";
	strClock += to_string(clock.second);
	return strClock;
}

std::string printSTime(const SYSTEMTIME st)
{
	return to_string(st.wYear) + "-" + (st.wMonth < 10 ? "0" : "") + to_string(st.wMonth) + "-" + (
			st.wDay < 10 ? "0" : "") + to_string(st.wDay) + " " + (st.wHour < 10 ? "0" : "") + to_string(st.wHour) + ":"
		+ (
			st.wMinute < 10 ? "0" : "") + to_string(st.wMinute) + ":" + (st.wSecond < 10 ? "0" : "") +
		to_string(st.wSecond);
}
	//��ӡ�û��ǳ�QQ
	string printQQ(long long llqq)
	{
		string nick = getStrangerInfo(llqq).nick;
		if (nick.empty())nick = getFriendList()[llqq].nick;
		if(nick.empty())return "�û�(" + to_string(llqq) + ")";
		return nick + "(" + to_string(llqq) + ")";
	}
	//��ӡQQȺ��
	string printGroup(long long llgroup)
	{
		if (!llgroup)return"˽��";
		if (ChatList.count(llgroup))return printChat(ChatList[llgroup]);
		if (getGroupList().count(llgroup))return "[" + getGroupList()[llgroup] + "](" + to_string(llgroup) + ")";
		return "Ⱥ(" + to_string(llgroup) + ")";
	}
	//��ӡ���촰��
	string printChat(chatType ct)
	{
		switch (ct.second)
		{
		case msgtype::Private:
			return printQQ(ct.first);
		case msgtype::Group:
			return printGroup(ct.first);
		case msgtype::Discuss:
			return "������(" + to_string(ct.first) + ")";
		default:
			break;
		}
		return "";
	}
//��ȡ�����б�
void getDiceList()
{
	std::string list;
	if (Network::GET("shiki.stringempty.xyz", "/DiceList/", 80, list))
		readJson(list, mDiceList);
}
//��ȡ�����б�
void getExceptGroup() {
	std::string list;
	if (Network::GET("shiki.stringempty.xyz", "/DiceCloud/except_group.json", 80, list))
	{
		try
		{
			json::parse(list, nullptr, false).get_to(ExceptGroups);
		}
		catch (...)
		{
			console.log("����ExceptGroupsʱ��������", 1, printSTNow());
		}
	}
	else
	{
		console.log("��ȡExceptGroupsʱ��������", 1, printSTNow());
	}
}


bool operator==(const SYSTEMTIME& st, const Console::Clock clock)
{
	return st.wHour == clock.first && st.wHour == clock.second;
}

bool operator<(const Console::Clock clock, const SYSTEMTIME& st)
{
	return st.wHour == clock.first && st.wHour == clock.second;
}

//���׼�ʱ��
	void ConsoleTimer()
	{
		Console::Clock clockNow{stNow.wHour,stNow.wMinute};
		while (Enabled) 
		{
			GetLocalTime(&stNow);
			//����ʱ��䶯
			if (stTmp.wMinute != stNow.wMinute)
			{
				stTmp = stNow;
				clockNow = {stNow.wHour, stNow.wMinute};
				for (const auto& [clock,eve_type] : multi_range(console.mWorkClock, clockNow))
				{
					switch (eve_type)
					{
					case ClockEvent::on:
						if (console["DisabledGlobal"])
						{
							console.set("DisabledGlobal", 0);
							console.log(getMsg("strClockToWork"), 0b10000, "");
						}
						break;
					case ClockEvent::off:
						if (!console["DisabledGlobal"])
						{
							console.set("DisabledGlobal", 1);
							console.log(getMsg("strClockOffWork"), 0b10000, "");
						}
						break;
					case ClockEvent::save:
						dataBackUp();
						console.log(GlobalMsg["strSelfName"] + "��ʱ������ɡ�", 1, printSTime(stTmp));
						break;
					case ClockEvent::clear:
						if (clearGroup("black"))
							console.log(GlobalMsg["strSelfName"] + "��ʱ��Ⱥ��ɡ�", 1, printSTNow());
						break;
					default: break;
					}
				}
			}
			this_thread::sleep_for(100ms);
		}
	}

//һ������
int clearGroup(string strPara, long long fromQQ)
{
	int intCnt = 0;
	string strReply;
	ResList res;
	std::map<string, string> strVar;
	if (strPara == "unpower" || strPara.empty())
	{
		for (auto& [id, grp] : ChatList)
		{
			if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("����"))continue;
			if (grp.isGroup && getGroupMemberInfo(id, console.DiceMaid).permissions == 1)
			{
				res << printGroup(id);
				grp.leave(getMsg("strLeaveNoPower"));
				intCnt++;
				this_thread::sleep_for(3s);
			}
		}
		strReply = GlobalMsg["strSelfName"] + "ɸ����ȺȨ��Ⱥ��" + to_string(intCnt) + "��:" + res.show();
		console.log(strReply, 0b10, printSTNow());
	}
	else if (isdigit(static_cast<unsigned char>(strPara[0])))
	{
		const int intDayLim = stoi(strPara);
		const string strDayLim = to_string(intDayLim);
		const time_t tNow = time(nullptr);
		for (auto& [id, grp] : ChatList)
		{
			if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("����"))continue;
			time_t tLast = grp.tLastMsg;
			if (grp.isGroup)
			{
				const int tLMT = getGroupMemberInfo(id, console.DiceMaid).LastMsgTime;
				if (tLMT > 0)
					tLast = tLMT;
			}
			if (!tLast)continue;
			const int intDay = static_cast<int>(tNow - tLast) / 86400;
			if (intDay > intDayLim)
			{
				strVar["day"] = to_string(intDay);
				res << printGroup(id) + ":" + to_string(intDay) + "��\n";
				grp.leave(getMsg("strLeaveUnused", strVar));
				intCnt++;
				this_thread::sleep_for(2s);
			}
		}
		strReply += GlobalMsg["strSelfName"] + "��ɸ��Ǳˮ" + strDayLim + "��Ⱥ��" + to_string(intCnt) + "����" + res.show();
		console.log(strReply, 0b10, printSTNow());
	}
	else if (strPara == "black")
	{
		try
		{
			for (auto& [id, grp_name] : getGroupList())
			{
				Chat& grp = chat(id).group().name(grp_name);
				if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("����") || grp.isset("���"))continue
					;
				if (blacklist->get_group_danger(id))
				{
					res << printGroup(id) + "��" + "������Ⱥ";
					if (console["LeaveBlackGroup"])grp.leave(getMsg("strBlackGroup"));
				}
				vector<GroupMemberInfo> MemberList = getGroupMemberList(id);
				for (const auto& eachQQ : MemberList)
				{
					if (blacklist->get_qq_danger(eachQQ.QQID) > 1)
					{
						if (eachQQ.permissions < getGroupMemberInfo(id, getLoginQQ()).permissions)
						{
							continue;
						}
						if (eachQQ.permissions > getGroupMemberInfo(id, getLoginQQ()).permissions)
						{
							res << printChat(grp) + "��" + printQQ(eachQQ.QQID) + "�Է�ȺȨ�޽ϸ�";
							grp.leave("���ֺ���������Ա" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
						if (console["LeaveBlackQQ"])
						{
							res << printChat(grp) + "��" + printQQ(eachQQ.QQID);
							grp.leave("���ֺ�������Ա" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
					}
				}
			}
		}
		catch (...)
		{
			console.log(strReply, 0b10, "���ѣ�" + GlobalMsg["strSelfName"] + "��������Ⱥ��ʱ����");
		}
		if (intCnt)
		{
			strReply = GlobalMsg["strSelfName"] + "�Ѱ����������Ⱥ��" + to_string(intCnt) + "����" + res.show();
			console.log(strReply, 0b10, printSTNow());
		}
		else if (fromQQ)
		{
			console.log(strReply, 1, printSTNow());
		}
	}
	else if (strPara == "preserve")
	{
		for (auto& [id,grp] : ChatList)
		{
			if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("ʹ�����") || grp.isset("����"))continue;
			if (grp.isGroup && getGroupMemberInfo(id, console.master()).permissions)
			{
				grp.set("ʹ�����");
				continue;
			}
			res << printChat(grp);
			grp.leave(getMsg("strPreserve"));
			intCnt++;
			this_thread::sleep_for(3s);
		}
		strReply = GlobalMsg["strSelfName"] + "ɸ�������Ⱥ��" + to_string(intCnt) + "����" + res.show();
		console.log(strReply, 1, printSTNow());
	}
	else
		AddMsgToQueue("�޷�ʶ��ɸѡ������", fromQQ);
	return intCnt;
}


EVE_Menu(eventClearGroupUnpower)
{
	const int intGroupCnt = clearGroup("unpower");
	const string strReply = "��������Ȩ��Ⱥ��" + to_string(intGroupCnt) + "����";
	MessageBoxA(nullptr, strReply.c_str(), "һ������", MB_OK | MB_ICONINFORMATION);
	return 0;
}

EVE_Menu(eventClearGroup30)
{
	const int intGroupCnt = clearGroup("30");
	const string strReply = "������30��δʹ��Ⱥ��" + to_string(intGroupCnt) + "����";
	MessageBoxA(nullptr, strReply.c_str(), "һ������", MB_OK | MB_ICONINFORMATION);
	return 0;
}

EVE_Menu(eventGlobalSwitch)
{
	if (console["DisabledGlobal"])
	{
		console.set("DisabledGlobal", 0);
		MessageBoxA(nullptr, "�����ѽ�����Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		console.set("DisabledGlobal", 1);
		MessageBoxA(nullptr, "������ȫ�־�Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}

EVE_Request_AddFriend(eventAddFriend)
{
	if (!console["ListenFriendRequest"])return 0;
	string strMsg = "��������������ԣ�" + printQQ(fromQQ);
	this_thread::sleep_for(3s);
	if (blacklist->get_qq_danger(fromQQ))
	{
		strMsg += "���Ѿܾ����û��ں������У�";
		setFriendAddRequest(responseFlag, 2, "");
		console.log(strMsg, 0b10, printSTNow());
	}
	else if (trustedQQ(fromQQ))
	{
		strMsg += "����ͬ�⣨�������û���";
		setFriendAddRequest(responseFlag, 1, "");
		AddMsgToQueue(getMsg("strAddFriendWhiteQQ"), fromQQ);
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 2 && !UserList.count(fromQQ))
	{
		strMsg += "���Ѿܾ������û���¼��";
		setFriendAddRequest(responseFlag, 2, "");
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 1)
	{
		strMsg += "���Ѿܾ����������û���";
		setFriendAddRequest(responseFlag, 2, "");
		console.log(strMsg, 1, printSTNow());
	}
	else
	{
		strMsg += "����ͬ��";
		setFriendAddRequest(responseFlag, 1, "");
		AddMsgToQueue(getMsg("strAddFriend"), fromQQ);
		console.log(strMsg, 1, printSTNow());
	}
	return 1;
}

EVE_Friend_Add(eventFriendAdd)
{
	if (!console["ListenFriendAdd"])return 0;
	this_thread::sleep_for(3s);
	GlobalMsg["strAddFriendWhiteQQ"].empty()
		? AddMsgToQueue(getMsg("strAddFriend"), fromQQ)
		: AddMsgToQueue(getMsg("strAddFriendWhiteQQ"), fromQQ);
	return 0;
}
