/*
 * _______   ________  ________  ________  __
 * |  __ \  |__  __| |  _____| |  _____| | |
 * | | | |   | |   | |    | |_____  | |
 * | | | |   | |   | |    |  _____| |__|
 * | |__| |  __| |__  | |_____  | |_____  __
 * |_______/  |________| |________| |________| |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2020 w4123溯洄
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
{"BannedBanInviter",0},{"KickedBanInviter",0},
{"GroupClearLimit",20},
{"CloudBlackShare",1},{"BelieveDiceList",0},{"CloudVisible",1},
{"SystemAlarmCPU",90},{"SystemAlarmRAM",90},{"SystemAlarmDisk",90},
{"SendIntervalIdle",500},{"SendIntervalBusy",100},
//自动保存事件间隔[min],自动图片清理间隔[h]
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
			strClock += " 定时开启";
			break;
		case ClockEvent::off:
			strClock += " 定时关闭";
			break;
		case ClockEvent::save:
			strClock += " 定时保存";
			break;
		case ClockEvent::clear:
			strClock += " 定时清群";
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
				chat(ct.first).set("许可使用").set("免清").set("免黑");
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
//除外群列表
std::set<long long> ExceptGroups;
//骰娘列表
std::map<long long, long long> mDiceList;

//程序启动时间
long long llStartTime = clock();

//当前时间
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

std::string printSTime(const SYSTEMTIME st)
{
	return to_string(st.wYear) + "-" + (st.wMonth < 10 ? "0" : "") + to_string(st.wMonth) + "-" + (
			st.wDay < 10 ? "0" : "") + to_string(st.wDay) + " " + (st.wHour < 10 ? "0" : "") + to_string(st.wHour) + ":"
		+ (
			st.wMinute < 10 ? "0" : "") + to_string(st.wMinute) + ":" + (st.wSecond < 10 ? "0" : "") +
		to_string(st.wSecond);
}
	//打印用户昵称QQ
	string printQQ(long long llqq)
	{
		string nick = getStrangerInfo(llqq).nick;
		if (nick.empty())nick = getFriendList()[llqq].nick;
		if(nick.empty())return "用户(" + to_string(llqq) + ")";
		return nick + "(" + to_string(llqq) + ")";
	}
	//打印QQ群号
	string printGroup(long long llgroup)
	{
		if (!llgroup)return"私聊";
		if (ChatList.count(llgroup))return printChat(ChatList[llgroup]);
		if (getGroupList().count(llgroup))return "[" + getGroupList()[llgroup] + "](" + to_string(llgroup) + ")";
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
	std::string list;
	if (Network::GET("shiki.stringempty.xyz", "/DiceList/", 80, list))
		readJson(list, mDiceList);
}
//获取骰娘列表
void getExceptGroup() {
	std::string list;
	if (Network::GET("shiki.stringempty.xyz", "/DiceCloud/except_group.json", 80, list))
		readJson(list, ExceptGroups);
}


bool operator==(const SYSTEMTIME& st, const Console::Clock clock)
{
	return st.wHour == clock.first && st.wHour == clock.second;
}

bool operator<(const Console::Clock clock, const SYSTEMTIME& st)
{
	return st.wHour == clock.first && st.wHour == clock.second;
}

//简易计时器
	void ConsoleTimer()
	{
		Console::Clock clockNow{stNow.wHour,stNow.wMinute};
		while (Enabled) 
		{
			GetLocalTime(&stNow);
			//分钟时点变动
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
						console.log(GlobalMsg["strSelfName"] + "定时保存完成√", 1, printSTime(stTmp));
						break;
					case ClockEvent::clear:
						if (clearGroup("black"))
							console.log(GlobalMsg["strSelfName"] + "定时清群完成√", 1, printSTNow());
						break;
					default: break;
					}
				}
			}
			this_thread::sleep_for(100ms);
		}
	}

//一键清退
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
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清"))continue;
			if (grp.isGroup && getGroupMemberInfo(id, console.DiceMaid).permissions == 1)
			{
				res << printGroup(id);
				grp.leave(getMsg("strLeaveNoPower"));
				intCnt++;
				this_thread::sleep_for(3s);
			}
		}
		strReply = GlobalMsg["strSelfName"] + "筛除无群权限群聊" + to_string(intCnt) + "个:" + res.show();
		console.log(strReply, 0b10, printSTNow());
	}
	else if (isdigit(static_cast<unsigned char>(strPara[0])))
	{
		const int intDayLim = stoi(strPara);
		const string strDayLim = to_string(intDayLim);
		const time_t tNow = time(nullptr);
		for (auto& [id, grp] : ChatList)
		{
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清"))continue;
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
				res << printGroup(id) + ":" + to_string(intDay) + "天\n";
				grp.leave(getMsg("strLeaveUnused", strVar));
				intCnt++;
				this_thread::sleep_for(2s);
			}
		}
		strReply += GlobalMsg["strSelfName"] + "已筛除潜水" + strDayLim + "天群聊" + to_string(intCnt) + "个√" + res.show();
		console.log(strReply, 0b10, printSTNow());
	}
	else if (strPara == "black")
	{
		try
		{
			for (auto& [id, grp_name] : getGroupList())
			{
				Chat& grp = chat(id).group().name(grp_name);
				if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("免清") || grp.isset("免黑"))continue
					;
				if (blacklist->get_group_danger(id))
				{
					res << printGroup(id) + "：" + "黑名单群";
					grp.leave(getMsg("strBlackGroup"));
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
							res << printChat(grp) + "：" + printQQ(eachQQ.QQID) + "对方群权限较高";
							grp.leave("发现黑名单管理员" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群");
							intCnt++;
							break;
						}
						if (console["LeaveBlackQQ"])
						{
							res << printChat(grp) + "：" + printQQ(eachQQ.QQID);
							grp.leave("发现黑名单成员" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "将预防性退群");
							intCnt++;
							break;
						}
					}
				}
			}
		}
		catch (...)
		{
			console.log(strReply, 0b10, "提醒：" + GlobalMsg["strSelfName"] + "清查黑名单群聊时出错！");
		}
		if (intCnt)
		{
			strReply = GlobalMsg["strSelfName"] + "已按黑名单清查群聊" + to_string(intCnt) + "个：" + res.show();
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
			if (grp.isset("忽略") || grp.isset("已退") || grp.isset("未进") || grp.isset("使用许可") || grp.isset("免清"))continue;
			if (grp.isGroup && getGroupMemberInfo(id, console.master()).permissions)
			{
				grp.set("使用许可");
				continue;
			}
			res << printChat(grp);
			grp.leave(getMsg("strPreserve"));
			intCnt++;
			this_thread::sleep_for(3s);
		}
		strReply = GlobalMsg["strSelfName"] + "筛除无许可群聊" + to_string(intCnt) + "个：" + res.show();
		console.log(strReply, 1, printSTNow());
	}
	else
		AddMsgToQueue("无法识别筛选参数×", fromQQ);
	return intCnt;
}


EVE_Menu(eventClearGroupUnpower)
{
	const int intGroupCnt = clearGroup("unpower");
	const string strReply = "已清退无权限群聊" + to_string(intGroupCnt) + "个√";
	MessageBoxA(nullptr, strReply.c_str(), "一键清退", MB_OK | MB_ICONINFORMATION);
	return 0;
}

EVE_Menu(eventClearGroup30)
{
	const int intGroupCnt = clearGroup("30");
	const string strReply = "已清退30天未使用群聊" + to_string(intGroupCnt) + "个√";
	MessageBoxA(nullptr, strReply.c_str(), "一键清退", MB_OK | MB_ICONINFORMATION);
	return 0;
}

EVE_Menu(eventGlobalSwitch)
{
	if (console["DisabledGlobal"])
	{
		console.set("DisabledGlobal", 0);
		MessageBoxA(nullptr, "骰娘已结束静默√", "全局开关", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		console.set("DisabledGlobal", 1);
		MessageBoxA(nullptr, "骰娘已全局静默√", "全局开关", MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}

EVE_Request_AddFriend(eventAddFriend)
{
	if (!console["ListenFriendRequest"])return 0;
	string strMsg = "好友添加请求，来自 " + printQQ(fromQQ)+ ":";
	if (msg && msg[0] != '\0') strMsg += msg;
	this_thread::sleep_for(3s);
	if (blacklist->get_qq_danger(fromQQ))
	{
		strMsg += "\n已拒绝（用户在黑名单中）";
		setFriendAddRequest(responseFlag, 2, "");
		console.log(strMsg, 0b10, printSTNow());
	}
	else if (trustedQQ(fromQQ))
	{
		strMsg += "\n已同意（受信任用户）";
		setFriendAddRequest(responseFlag, 1, "");
		AddMsgToQueue(getMsg("strAddFriendWhiteQQ"), fromQQ);
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 2 && !UserList.count(fromQQ))
	{
		strMsg += "\n已拒绝（无用户记录）";
		setFriendAddRequest(responseFlag, 2, getMsg("strFriendDenyNotUser").c_str());
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 1)
	{
		strMsg += "\n已拒绝（非信任用户）";
		setFriendAddRequest(responseFlag, 2, getMsg("strFriendDenyNoTrust").c_str());
		console.log(strMsg, 1, printSTNow());
	}
	else
	{
		strMsg += "\n已同意";
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
