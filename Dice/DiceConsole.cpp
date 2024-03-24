/*
 * _______   ________  ________  ________  __
 * |  __ \  |__  __| |  _____| |  _____| | |
 * | | | |   | |   | |    | |_____  | |
 * | | | |   | |   | |    |  _____| |__|
 * | |__| |  __| |__  | |_____  | |_____  __
 * |_______/  |________| |________| |________| |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123���
 * Copyright (C) 2019-2024 String.Empty
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
#include "DiceMod.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif	
#include "DDAPI.h"
#include "yaml-cpp/yaml.h"
#include "tinyxml2.h"

using namespace std;

const fifo_dict_ci<int>Console::intDefault{
{"DisabledGlobal",0},{"DisabledListenAt",1},
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
//�Զ������¼����[min],�Զ�ͼƬ������[h],�Զ�������ܼ��[h]
{"AutoSaveInterval",5},/*{"AutoClearImage",0},{"AutoFrameRemake",0},*/
//�û���¼��������[Day],Ⱥ��¼��������[Day]
{"InactiveUserLine",360},{"InactiveGroupLine",360},
//����Ⱥ�ڻ�����Ϣ������Ƶ���ڻ�����Ϣ�������Լ�˽����Ϣ
{"ListenGroupEcho",0},{"ListenChannelEcho",1},{"ListenSelfEcho",0},
{"ReferMsgReply",0},
{"EnableWebUI",1},{"WebUIAllowInternetAccess",0},
{"WebUIPort",0},
{"EnablePython",0},
{"DebugMode",0},
{"DefaultCOCRoomRule",0},
};
const std::unordered_map<std::string,string>Console::confComment{
	{"DisabledGlobal","ȫ��ͣ��ָ���trust4����Ч"},
	{"DisabledListenAt","�ر�״̬�£�����atʱҲ��Ӧ"},
	{"DisabledMe","ȫ�ֽ���.me����trust4+����Ч"},
	{"DisabledJrrp","ȫ�ֽ���.jrrp����trust4+����Ч"},
	{"DisabledDeck","ȫ�ֽ���.deck����trust4+����Ч"},
	{"DisabledDraw","ȫ�ֽ���.draw����trust4+����Ч"},
	{"DisabledSend","ȫ�ֽ���.send����trust4+����Ч"},
	{"Private","˽��ģʽ��1-Ⱥ�����ʹ���Զ��˳�;0-�ر�"},
	{"CheckGroupLicense","�������ģʽ��2-�κ�Ⱥ�������ʹ��;1-�¼�Ⱥ�����ʹ��;0-�ر�"},
	{"ListenGroupRequest","������Ⱥ����"},
	{"ListenGroupAdd","������Ⱥ�¼�"},
	{"ListenFriendRequest","������������"},
	{"ListenFriendAdd","������������¼�"},
	{"AllowStranger","2-�����κ���;1-��Ҫ��ʹ�ü�¼;0-�����������û�"},
	{"DisableStrangerChat","�����󲻴���Ǻ���˽����Ϣ"},
	{"AutoClearBlack","�Զ���������"},
	{"LeaveBlackQQ","��������û�ȺȨ��ͬ���Ƿ���Ⱥ"},
	{"ListenGroupKick","����Ⱥ�����¼�"},
	{"ListenGroupBan","����Ⱥ�����¼�"},
	{"ListenSpam","����ָ��ˢ��"},
	{"BannedBanInviter","�����¼�����������"},
	{"KickedBanInviter","�����¼�����������"},
	{"GroupInvalidSize","������������Ⱥ�Զ�Э����Ч"},
	{"GroupClearLimit","������Ⱥ����"},
	{"CloudBlackShare","�ƶ�ͬ��������"},
	{"BelieveDiceList","���������ͣ�ã�"},
	{"CloudVisible","�Ƿ�����Dice��Ϣ�ɱ���ѯ"},
	{"SystemAlarmCPU","���𾯱���CPUռ�ðٷֱ�(1~100��Ч)"},
	{"SystemAlarmRAM","���𾯱����ڴ�ռ�ðٷֱ�(1~100��Ч)"},
	{"SystemAlarmDisk","���𾯱��Ĵ���ռ�ðٷֱ�(1~100��Ч)"},
	{"TimeZoneLag","��Ա���ʱ��ƫ��[h]"},
	{"SendIntervalIdle","���п���ʱ��Ϣ���ͼ��[ms]"},
	{"SendIntervalBusy","���з�æʱ��Ϣ���ͼ��[ms]"},
	{"AutoSaveInterval","�Զ������¼����[min]"},
	{"InactiveUserLine","�û���¼��������[Day]"},
	{"InactiveGroupLine","Ⱥ��¼��������[Day]"},
	{"ListenGroupEcho","����Ⱥ�ڻ�����Ϣ����Ҫ���֧��"},
	{"ListenChannelEcho","����Ƶ���ڻ�����Ϣ����Ҫ���֧��"},
	{"ListenSelfEcho","�����Լ�˽����Ϣ����Ҫ���֧��"},
	{"ReferMsgReply","��Ӧ��Ϣʱ�ظ�����Ϣ����Ҫ���֧��"},
	{"EnableWebUI","�Ƿ�����WebUI��������Ч"},
	{"WebUIAllowInternetAccess","�Ƿ�����Զ�̷���WebUI���˿�����⿪�ţ�������Ч"},
	{"WebUIPort","�̶�WebUI�˿ڣ�������Ч"},
	{"EnablePython","�Ƿ�����Python��������Ч"},
	{"DebugMode","����ģʽ�������н��ܵ�ָ��д���ļ�"},
	{"DefaultCOCRoomRule","δ����rc����ʱ���õļ춨����"},
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
void Console::log(const std::string& msg, const std::string& file) {
	DD::debugLog(msg);
	ofstream fout(DiceDir / "audit" / ("log_" + file + ".txt"),
		ios::out | ios::app);
	fout << printSTNow() << "\t" << printLine(msg) << std::endl;
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
				for (auto cfg : yaml["config"]) {
					intConf[cfg.first.as<string>()] = cfg.second.as<int>();
				}
			}
			if (yaml["clock"]) {
				for (auto task : yaml["clock"]) {
					mWorkClock.insert({ scanClock(task["moment"].as<string>()),
						task["task"].as<string>() });
				}
			}
			if (yaml["git"]) {
				git_user = yaml["git"]["user"].as<string>();
				git_pw = yaml["git"]["password"].as<string>();
			}
		}
		catch (std::exception& e) {
			DD::debugLog("/conf/console.yaml��ȡʧ��!" + string(e.what()));
		}
	}
	else if (tinyxml2::XMLDocument doc; tinyxml2::XML_SUCCESS == doc.LoadFile((DiceDir / "conf" / "console.xml").string().c_str())){
		for (auto node = doc.FirstChildElement()->FirstChildElement(); node; node = node->NextSiblingElement()) {
			if (string tag{ node->Name() }; tag == "master")master = stoll(node->GetText());
			else if (tag == "clock")
				for (auto child = node->FirstChildElement(); child; child = child->NextSiblingElement()) {
					mWorkClock.insert({
						scanClock(child->GetText()), child->Name()
						});
				}
			else if (tag == "conf")
				for (auto child = node->FirstChildElement(); child; child = child->NextSiblingElement()) {
					std::pair<string, int> conf;
					readini(string(child->GetText()), conf);
					if (intDefault.count(conf.first))intConf.insert(conf);
				}
		}
		save();
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
			for (auto& note : jFile) {
				if (chatInfo chat(chatInfo::from_json(note)); chat && note.count("type"))
					NoticeList.emplace(chat, note["type"].get<int>());
			}
			return;
		}
		catch (const std::exception& e) {
			console.log(string("����/conf/NoticeList.json����:") + e.what(), 1);
		}
	}
}

void Console::saveNotice() const
{
	if (NoticeList.empty())filesystem::remove(DiceDir / "conf" / "NoticeList.json");
	ofstream fout(DiceDir / "conf" / "NoticeList.json");
	if (!fout)return;
	fifo_json jList = fifo_json::array();
	for (auto& [chat, lv] : NoticeList) {
		if (lv) {
			fifo_json j = to_json(chat);
			j["type"] = lv;
			jList.push_back(j);
		}
	}
	fout << jList.dump();
}

Console console;

//DiceModManager modules{};
//����Ⱥ�б�
std::set<long long> ExceptGroups;
//�����б�
std::map<long long, long long> mDiceList;

//��������ʱ��
long long llStartTime = time(nullptr);

//��ǰʱ��
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

std::string printDate(tm st) {
	return to_string(st.tm_year + 1900) + "-" + (st.tm_mon + 1 < 10 ? "0" : "") + to_string(st.tm_mon + 1) + "-" + (
		st.tm_mday < 10 ? "0" : "") + to_string(st.tm_mday);
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

std::string printSTime(const tm st)
{
	return to_string(st.tm_year + 1900) + "-" + (st.tm_mon + 1 < 10 ? "0" : "") + to_string(st.tm_mon + 1) + "-" + (
			st.tm_mday < 10 ? "0" : "") + to_string(st.tm_mday) + " " + (st.tm_hour < 10 ? "0" : "") + to_string(st.tm_hour) + ":"
		+ (
			st.tm_min < 10 ? "0" : "") + to_string(st.tm_min) + ":" + (st.tm_sec < 10 ? "0" : "") +
		to_string(st.tm_sec);
}
	//��ӡ�û��ǳ�QQ
	string printUser(long long llqq)
	{
		string nick = DD::getQQNick(llqq);
		if (nick.empty())return getMsg("stranger") + "[" + to_string(llqq) + "]";
		return nick + "(" + to_string(llqq) + ")";
	}
	//��ӡQQȺ��
	string printGroup(long long llgroup)
	{
		if (!llgroup)return "˽��";
		if (ChatList.count(llgroup))return ChatList[llgroup]->print();
		if (string name{ DD::getGroupName(llgroup) };!name.empty())return "[" + name + "](" + to_string(llgroup) + ")";
		return "Ⱥ(" + to_string(llgroup) + ")";
	}
	//��ӡ���촰��
	string printChat(chatInfo ct)
	{
		switch (ct.type)
		{
		case msgtype::Private:
			return printUser(ct.uid);
		case msgtype::Group:
			return printGroup(ct.gid);
		case msgtype::Discuss:
			return "������(" + to_string(ct.gid) + ")";
		case msgtype::Channel: //Warning:Temporary
			return "Ƶ��(" + to_string(ct.gid) + ")";
		default:
			break;
		}
		return "";
	}
//��ȡ�����б�
void getDiceList()
{
}
//��ȡ�����б�
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

