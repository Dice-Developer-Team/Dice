/*
 * _______   ________  ________  ________  __
 * |  __ \  |__  __| |  _____| |  _____| | |
 * | | | |   | |   | |    | |_____  | |
 * | | | |   | |   | |    |  _____| |__|
 * | |__| |  __| |__  | |_____  | |_____  __
 * |_______/  |________| |________| |________| |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2021 String.Empty
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
#include "DiceSchedule.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif	
#include "DDAPI.h"

using namespace std;

const std::map<std::string, int, less_ci>Console::intDefault{
{"DisabledGlobal",0},{"DisabledBlock",0},{"DisabledListenAt",1},
{"DisabledMe",1},{"DisabledJrrp",0},{"DisabledDeck",1},{"DisabledDraw",0},{"DisabledSend",0},
{"Private",0},{"CheckGroupLicense",0},{"LeaveDiscuss",0},
{"ListenGroupRequest",1},{"ListenGroupAdd",1},
{"ListenFriendRequest",1},{"ListenFriendAdd",1},{"AllowStranger",1},
{"AutoClearBlack",1},{"LeaveBlackQQ",0},
{"ListenGroupKick",1},{"ListenGroupBan",1},{"ListenSpam",1},
{"BannedBanInviter",0},{"KickedBanInviter",0},
{"GroupInvalidSize",500},{"GroupClearLimit",20},
{"CloudBlackShare",1},{"BelieveDiceList",0},{"CloudVisible",1},
{"SystemAlarmCPU",90},{"SystemAlarmRAM",90},{"SystemAlarmDisk",90},
{"SendIntervalIdle",500},{"SendIntervalBusy",100},
//自动保存事件间隔[min],自动图片清理间隔[h],自动重启框架间隔[h]
{"AutoSaveInterval",5},{"AutoClearImage",0},{"AutoFrameRemake",0},
//接收群内自己的消息，接受自己私聊消息
{"ListenGroupEcho",0},{"ListenSelfEcho",0},
};
const enumap<string> Console::mClockEvent{ "off", "on", "save", "clear" };

int Console::setClock(Clock c, const string& e)
{
	if (c.first > 23 || c.second > 59)return -1;
	mWorkClock.emplace(c, e);
	save();
	return 0;
}

int Console::rmClock(Clock c, const string& e)
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
		string strClock = printClock(clock) + " " + eve;
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
	ofstream fout(DiceDir / "audit" / "log" / (to_string(DiceMaid) + "_" + printDate() + ".txt"),
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
			if(strTime.empty())this_thread::sleep_for(chrono::milliseconds(console["SendIntervalIdle"]));
		}
		if (!Cnt)DD::sendPrivateMsg(DiceMaid, note);
	}
	else DD::debugLog(note);
	return Cnt;
} 
void Console::newMaster(long long qq)
{
	masterQQ = qq; 
	if (trustedQQ(qq) < 5)getUser(qq).trust(5);
	setNotice({qq, msgtype::Private}, 0b111111);
	save(); 
	AddMsgToQueue(getMsg("strNewMaster"), qq);
	AddMsgToQueue(intConf["Private"] ? getMsg("strNewMasterPrivate") : getMsg("strNewMasterPublic"), qq);
}

void Console::reset()
{
	intConf.clear();
	mWorkClock.clear();
	NoticeList.clear();
}

bool Console::load() 	{
	string s;
	//DSens.build({ {"nn老公",2 } });
	if (!rdbuf(fpPath, s))return false;
	DDOM xml(s);
	if (xml.count("mode"))isMasterMode = stoi(xml["mode"].strValue);
	if (xml.count("master"))masterQQ = stoll(xml["master"].strValue);
	if (xml.count("clock"))
		for (auto& child : xml["clock"].vChild) {
			mWorkClock.insert({
				scanClock(child.strValue), child.tag
							  });
		}
	if (xml.count("conf"))
		for (auto& child : xml["conf"].vChild) 			{
			std::pair<string, int> conf;
			readini(child.strValue, conf);
			if (intDefault.count(conf.first))intConf.insert(conf);
		}
	loadNotice();
	return true;
}
void Console::loadNotice()
{
	if (loadFile(DiceDir / "conf" / "NoticeList.txt", NoticeList) < 1)
	{
		std::set<chatType> sChat;
		if (loadFile(DiceDir / "com.w4123.dice" / "MonitorList.RDconf", sChat) > 0)
			for (const auto& it : sChat)
			{
				console.setNotice(it, 0b100000);
			}
		sChat.clear();
		if (loadFile(DiceDir / "conf" / "RecorderList.RDconf", sChat) > 0)
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
				chat(ct.first).set("许可使用").set("免清").set("免黑");
			}
		}
	}
}

void Console::saveNotice() const
{
	saveFile(DiceDir / "conf" / "NoticeList.txt", NoticeList);
}

Console console;

//DiceModManager modules{};
//除外群列表
std::set<long long> ExceptGroups;
//骰娘列表
std::map<long long, long long> mDiceList;

//程序启动时间
long long llStartTime = time(nullptr);

//当前时间
tm stNow{};

std::string printSTNow()
{
	time_t tt = time(nullptr);
#ifdef _MSC_VER
	localtime_s(&stNow, &tt);
#else
	localtime_r(&tt, &stNow);
#endif
	return printSTime(stNow);
}

std::string printDate()
{
	return to_string(stNow.tm_year + 1900) + "-" + (stNow.tm_mon + 1 < 10 ? "0" : "") + to_string(stNow.tm_mon + 1) + "-" + (
		stNow.tm_mday < 10 ? "0" : "") + to_string(stNow.tm_mday);
}

std::string printDate(time_t tt)
{
	tm t{};
	if(!tt) return R"(????-??-??)"; 
#ifdef _MSC_VER
	auto ret = localtime_s(&t, &tt);
	if(ret) return R"(????-??-??)";
#else
	auto ret = localtime_r(&tt, &t);
	if(!ret) return R"(????-??-??)";
#endif
	return to_string(t.tm_year + 1900) + "-" + to_string(t.tm_mon + 1) + "-" + to_string(t.tm_mday);
}

//上班时间
std::pair<int, int> ClockToWork = {24, 0};
//下班时间
std::pair<int, int> ClockOffWork = {24, 0};

string printClock(std::pair<int, int> clock)
{
	string strClock = to_string(clock.first);
	strClock += ":";
	if (clock.second < 10)strClock += "0";
	strClock += to_string(clock.second);
	return strClock;
}

std::string printSTime(const tm st)
{
	return to_string(st.tm_year + 1900) + "-" + (st.tm_mon + 1 < 10 ? "0" : "") + to_string(st.tm_mon + 1) + "-" + (
			st.tm_mday < 10 ? "0" : "") + to_string(st.tm_mday) + " " + (st.tm_hour < 10 ? "0" : "") + to_string(st.tm_hour) + ":"
		+ (
			st.tm_min < 10 ? "0" : "") + to_string(st.tm_min) + ":" + (st.tm_sec < 10 ? "0" : "") +
		to_string(st.tm_sec);
}
	//打印用户昵称QQ
	string printQQ(long long llqq)
	{
		string nick = DD::getQQNick(llqq);
		if (nick.empty())return getMsg("stranger") + "[" + to_string(llqq) + "]";
		return nick + "(" + to_string(llqq) + ")";
	}
	//打印QQ群号
	string printGroup(long long llgroup)
	{
		if (!llgroup)return "私聊";
		if (ChatList.count(llgroup))return printChat(ChatList[llgroup]);
		if (string name{ DD::getGroupName(llgroup) };!name.empty())return "[" + name + "](" + to_string(llgroup) + ")";
		return "群(" + to_string(llgroup) + ")";
	}
	//打印聊天窗口
	string printChat(chatType ct)
	{
		switch (ct.second)
		{
		case msgtype::Private:
			return printQQ(ct.first);
		case msgtype::Group:
			return printGroup(ct.first);
		case msgtype::Discuss:
			return "讨论组(" + to_string(ct.first) + ")";
		default:
			break;
		}
		return "";
	}
//获取骰娘列表
void getDiceList()
{
}
//获取骰娘列表
void getExceptGroup() {
	std::string list;
	if (Network::GET("shiki.stringempty.xyz", "/DiceCloud/except_group.json", 80, list))
		readJson(list, ExceptGroups);
}


bool operator==(const tm& st, const Console::Clock clock)
{
	return st.tm_hour == clock.first && st.tm_hour == clock.second;
}

bool operator<(const Console::Clock clock, const tm& st)
{
	return st.tm_hour == clock.first && st.tm_hour == clock.second;
}

	void ThreadFactory::exit() {
		rear = 0;
		for (auto& th : vTh) {
			if (th.joinable())th.join();
		}
		vTh = {};
	}

