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
 * Copyright (C) 2019-2022 String.Empty
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
#include "yaml-cpp/yaml.h"

using namespace std;

const fifo_dict_ci<int>Console::intDefault{
{"DisabledGlobal",0},{"DisabledBlock",0},{"DisabledListenAt",1},
{"DisabledMe",1},{"DisabledJrrp",0},{"DisabledDeck",1},{"DisabledDraw",0},{"DisabledSend",0},
{"Private",0},{"CheckGroupLicense",0},
{"ListenGroupRequest",1},{"ListenGroupAdd",1},
{"ListenFriendRequest",1},{"ListenFriendAdd",1},{"AllowStranger",1},
{"DisableStrangerChat",1},
{"AutoClearBlack",1},{"LeaveBlackQQ",0},
{"ListenGroupKick",1},{"ListenGroupBan",1},{"ListenSpam",1},
{"BannedBanInviter",0},{"KickedBanInviter",0},
{"GroupInvalidSize",500},{"GroupClearLimit",20},
{"CloudBlackShare",1},{"BelieveDiceList",0},{"CloudVisible",1},
{"SystemAlarmCPU",90},{"SystemAlarmRAM",90},{"SystemAlarmDisk",90},
{"TimeZoneLag",0},
{"SendIntervalIdle",500},{"SendIntervalBusy",100},
//自动保存事件间隔[min],自动图片清理间隔[h],自动重启框架间隔[h]
{"AutoSaveInterval",5},/*{"AutoClearImage",0},{"AutoFrameRemake",0},*/
//用户记录清理期限[Day],群记录清理期限[Day]
{"InactiveUserLine",360},{"InactiveGroupLine",360},
//接收群内回音消息，接受频道内回音消息，接受自己私聊消息
{"ListenGroupEcho",0},{"ListenChannelEcho",1},{"ListenSelfEcho",0},
{"ReferMsgReply",0},
{"EnableWebUI",1},{"WebUIAllowInternetAccess",0},
{"WebUIPort",0},
{"DebugMode",0},
{"DefaultCOCRoomRule",0},
};
const std::unordered_map<std::string,string>Console::confComment{
	{"DisabledGlobal","全局停用指令，对trust4不起效"},
	{"DisabledBlock","框架多插件时，关闭状态开启拦截，阻止其他插件处理消息"},
	{"DisabledListenAt","关闭状态下，自身被at时也响应"},
	{"DisabledMe","全局禁用.me，对trust4+不起效"},
	{"DisabledJrrp","全局禁用.jrrp，对trust4+不起效"},
	{"DisabledDeck","全局禁用.deck，对trust4+不起效"},
	{"DisabledDraw","全局禁用.draw，对trust4+不起效"},
	{"DisabledSend","全局禁用.send，对trust4+不起效"},
	{"Private","私用模式：1-群无许可使用自动退出;0-关闭"},
	{"CheckGroupLicense","公用审核模式：2-任何群必须许可使用;1-新加群须许可使用;0-关闭"},
	{"ListenGroupRequest","监听加群邀请"},
	{"ListenGroupAdd","监听入群事件"},
	{"ListenFriendRequest","监听好友申请"},
	{"ListenFriendAdd","监听好友添加事件"},
	{"AllowStranger","2-允许任何人;1-需要有使用记录;0-仅允许信任用户"},
	{"DisableStrangerChat","开启后不处理非好友私聊消息"},
	{"AutoClearBlack","自动清查黑名单"},
	{"LeaveBlackQQ","与黑名单用户群权限同级是否退群"},
	{"ListenGroupKick","监听群被踢事件"},
	{"ListenGroupBan","监听群禁言事件"},
	{"ListenSpam","监听指令刷屏"},
	{"BannedBanInviter","被踢事件连带邀请人"},
	{"KickedBanInviter","禁言事件连带邀请人"},
	{"GroupInvalidSize","超过该人数的群自动协议无效"},
	{"GroupClearLimit","单次清群上限"},
	{"CloudBlackShare","云端同步黑名单"},
	{"BelieveDiceList","信任骰娘（已停用）"},
	{"CloudVisible","是否允许Dice信息可被查询"},
	{"SystemAlarmCPU","引起警报的CPU占用百分比(1~100生效)"},
	{"SystemAlarmRAM","引起警报的内存占用百分比(1~100生效)"},
	{"SystemAlarmDisk","引起警报的磁盘占用百分比(1~100生效)"},
	{"TimeZoneLag","相对本机时区偏移[h]"},
	{"SendIntervalIdle","队列空闲时消息发送间隔[ms]"},
	{"SendIntervalBusy","队列繁忙时消息发送间隔[ms]"},
	{"AutoSaveInterval","自动保存事件间隔[min]"},
	{"InactiveUserLine","用户记录清理期限[Day]"},
	{"InactiveGroupLine","群记录清理期限[Day]"},
	{"ListenGroupEcho","接收群内回音消息，需要框架支持"},
	{"ListenChannelEcho","接受频道内回音消息，需要框架支持"},
	{"ListenSelfEcho","接受自己私聊消息，需要框架支持"},
	{"ReferMsgReply","响应消息时回复该消息，需要框架支持"},
	{"EnableWebUI","是否启用WebUI，重启生效"},
	{"WebUIAllowInternetAccess","是否允许远程访问WebUI（端口须对外开放）重启生效"},
	{"WebUIPort","固定WebUI端口，重启生效"},
	{"DebugMode","调试模式：将所有接受的指令写入文件"},
	{"DefaultCOCRoomRule","未设置rc房规时调用的检定规则"},
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

int Console::showNotice(chatInfo ct) const
{
	if (const auto it = NoticeList.find(ct); it != NoticeList.end())return it->second;
	return 0;
}

void Console::addNotice(chatInfo ct, int lv)
{
	NoticeList[ct] |= lv;
	saveNotice();
}

void Console::redNotice(chatInfo ct, int lv) {
	if (NoticeList[ct] == lv)NoticeList.erase(ct);
	else NoticeList[ct] &= (~lv);
	saveNotice();
}

void Console::setNotice(chatInfo ct, int lv)
{
	NoticeList[ct] = lv;
	saveNotice();
}

void Console::rmNotice(chatInfo ct)
{
	NoticeList.erase(ct);
	saveNotice();
}

int Console::log(const std::string& strMsg, int note_lv, const string& strTime)
{
	ofstream fout(DiceDir / "audit" / ("log" + to_string(DiceMaid) + "_" + printDate() + ".txt"),
	              ios::out | ios::app);
	fout << strTime << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
	fout.close();
	int Cnt = 0;
	const string note = strTime.empty() ? strMsg : (strTime + " " + strMsg);
	if (note_lv){
		for (auto& [ct, level] : NoticeList)
		{
			if (!(level & note_lv))continue;
			AddMsgToQueue(note, ct);
			Cnt++; 
			if(strTime.empty())this_thread::sleep_for(chrono::milliseconds(console["SendIntervalIdle"]));
		}
	}
	if (!Cnt)DD::debugMsg(note);
	else DD::debugLog(note);
	return Cnt;
} 

void Console::newMaster(long long uid) {
	master = uid;
	if (trustedQQ(uid) < 5)getUser(uid).trust(5);
	if (!intConf["Private"]) {
		if (!intConf.count("BelieveDiceList"))intConf["BelieveDiceList"] = 1;
		if (!intConf.count("LeaveBlackQQ"))intConf["LeaveBlackQQ"] = 1;
		if (!intConf.count("BannedBanInviter"))intConf["BannedBanInviter"] = 1;
		if (!intConf.count("KickedBanInviter"))intConf["KickedBanInviter"] = 1;
		if (!intConf.count("AllowStranger"))intConf["AllowStranger"] = 2;
	}
	setNotice({ uid, 0,0 }, 0b111111);
	save(); 
	AddMsgToQueue(getMsg("strNewMaster"), uid);
	AddMsgToQueue(intConf["Private"] ? getMsg("strNewMasterPrivate") : getMsg("strNewMasterPublic"), uid);
}

void Console::reset()
{
	intConf.clear();
	mWorkClock.clear();
	NoticeList.clear();
}

bool Console::load() {
	if (std::filesystem::exists(DiceDir / "conf" / "console.yaml")) {
		try {
			YAML::Node yaml{ YAML::LoadFile(getNativePathString(DiceDir / "conf" / "console.yaml")) };
			if (yaml["master"])master = yaml["master"].as<long long>();
			if (yaml["config"]) {
				for (auto& cfg : yaml["config"]) {
					intConf[cfg.first.as<string>()] = cfg.second.as<int>();
				}
			}
			if (yaml["clock"]) {
				for (auto& task : yaml["clock"]) {
					mWorkClock.insert({ scanClock(task["moment"].as<string>()),
						task["task"].as<string>() });
				}
			}
			if (yaml["git"]) {
				git_user = yaml["git"]["user"].as<string>();
				git_pw = yaml["git"]["password"].as<string>();
			}
		} catch (std::exception& e) {
			DD::debugLog("/conf/console.yaml读取失败!" + string(e.what()));
		}
	}
	else if (string s; !rdbuf(DiceDir / "conf" / "console.xml", s))return false;
	else {
		DDOM xml(s);
		for (auto& node : xml.vChild) {
			if (node.tag == "master")master = stoll(node.strValue);
			else if (node.tag == "clock")
				for (auto& child : node.vChild) {
					mWorkClock.insert({
						scanClock(child.strValue), child.tag
						});
				}
			else if (node.tag == "conf")
				for (auto& child : node.vChild) {
					std::pair<string, int> conf;
					readini(child.strValue, conf);
					if (intDefault.count(conf.first))intConf.insert(conf);
				}
		}
	}
	loadNotice();
	return true;
}
void Console::save() {
	std::error_code ec;
	std::filesystem::create_directories(DiceDir / "conf", ec);
	YAML::Node yaml;
	yaml["master"] = master;
	if (!intConf.empty()){
		YAML::Node cfg;
		for (auto& [item, val] : intConf){
			cfg[item] = val;
		}
		yaml["config"] = cfg;
	}
	if (!mWorkClock.empty()) {
		YAML::Node clocks;
		for (auto& [clock, task] : mWorkClock) {
			YAML::Node item;
			item["moment"] = printClock(clock);
			item["task"] = task;
			clocks.push_back(item);
		}
		yaml["clock"] = clocks;
	}
	if (!git_user.empty()) {
		YAML::Node git;
		git["user"] = git_user;
		git["password"] = git_pw;
		yaml["git"] = git;
	}
	if (std::ofstream fout{ DiceDir / "conf" / "console.yaml" }) fout << yaml;
}
void Console::loadNotice()
{
	fifo_json jFile = freadJson(DiceDir / "conf" / "NoticeList.json");
	if (!jFile.empty()) {
		try {
			for (auto note : jFile) {
				if (chatInfo chat(chatInfo::from_json(note)); chat && note.count("type"))
					NoticeList.emplace(chat, note["type"].get<int>());
			}
			return;
		}
		catch (const std::exception& e) {
			console.log(string("解析/conf/NoticeList.json出错:") + e.what(), 1);
		}
	}
	if (NoticeList.empty() && loadFile(DiceDir / "conf" / "NoticeList.txt", NoticeList) < 1){
		console.setNotice({ 0,192499947 }, 0b100000);
		console.setNotice({ 0,928626681 }, 0b100000);
	}
}

void Console::saveNotice() const
{
	if (NoticeList.empty())filesystem::remove(DiceDir / "conf" / "NoticeList.json");
	ofstream fout(DiceDir / "conf" / "NoticeList.json");
	if (!fout)return;
	fifo_json jList = fifo_json::array();
	for (auto& [chat, lv] : NoticeList) {
		fifo_json j = to_json(chat);
		j["type"] = lv;
		jList.push_back(j);
	}
	fout << jList.dump();
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

std::string printSTNow(){
	time_t tt = time(nullptr) + time_t(console["TimeZoneLag"]) * 3600;
#ifdef _MSC_VER
	localtime_s(&stNow, &tt);
#else
	localtime_r(&tt, &stNow);
#endif
	return printSTime(stNow);
}

std::string printDate(){
	time_t tt = time(nullptr) + time_t(console["TimeZoneLag"]) * 3600;
#ifdef _MSC_VER
	localtime_s(&stNow, &tt);
#else
	localtime_r(&tt, &stNow);
#endif
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
	string printUser(long long llqq)
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
	string printChat(chatInfo ct)
	{
		switch (ct.type)
		{
		case msgtype::Private:
			return printUser(ct.uid);
		case msgtype::Group:
			return printGroup(ct.gid);
		case msgtype::Discuss:
			return "讨论组(" + to_string(ct.gid) + ")";
		case msgtype::Channel: //Warning:Temporary
			return "频道(" + to_string(ct.gid) + ")";
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
	if (Network::GET("http://shiki.stringempty.xyz/DiceCloud/except_group.json", list))
		readJson(list, ExceptGroups);
}


bool operator==(const tm& st, const Clock clock)
{
	return st.tm_hour == clock.first && st.tm_hour == clock.second;
}

bool operator<(const Clock clock, const tm& st)
{
	return st.tm_hour < clock.first || st.tm_min < clock.second;
}

	void ThreadFactory::exit() {
		rear = 0;
		for (auto& th : vTh) {
			if (th.joinable())th.join();
		}
		vTh = {};
	}

