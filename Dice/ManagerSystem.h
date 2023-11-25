#pragma once

/*
 * 后台系统
 * Copyright (C) 2019-2023 String.Empty
 * 控制清理用户/群聊记录，清理图片，监控系统
 */

#include <set>
#include <map>
#include <utility>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "DiceFile.hpp"
#include "DiceConsole.h"
#include "MsgFormat.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using std::string;
using std::to_string;
using std::set;
using std::map;
using std::vector;
using std::unordered_map;
constexpr auto CQ_IMAGE = "[CQ:image,";
constexpr auto CQ_AT = "[CQ:at,id=";
constexpr auto CQ_QQAT = "[CQ:at,qq=";
constexpr auto CQ_FACE = "[CQ:face,id=";
constexpr auto CQ_POKE = "[CQ:poke,id=";
constexpr auto CQ_FILE = "[CQ:file,";

//加载数据
void loadData();
//保存数据
void dataBackUp();

//用户记录
class User
{
public:
	long long ID = 0;
	//1-私用信任，2-拉黑豁免，3-加黑退群，4-后台管理，5-Master
	int nTrust = 0;
	time_t tCreated = time(nullptr);

	User(){}

	AttrObject confs;
	unordered_map<long long, string> strNick{};
	std::mutex ex_user;

	User& id(long long qq)
	{
		ID = qq;
		return *this;
	}

	User& create(time_t tt)
	{
		if (tt < tCreated)tCreated = tt;
		return *this;
	}

	User& update(time_t tt) {
		confs.set("tUpdated", (long long)tt);
		return *this;
	}
	time_t updated()const { return confs.get_ll("tUpdated"); }

	User& trust(int n)
	{
		nTrust = n;
		confs["trust"] = n;
		return *this;
	}

	[[nodiscard]] bool empty() const;

	[[nodiscard]] bool isset(const string& key) const{
		return confs.has(key);
	}
	void setConf(const string& key, const AttrVar& val);
	void rmConf(const string& key);
	int getConf(const string& key, int def = 0) {
		if (confs.has(key))return confs.get_int(key);
		return def;
	}

	bool getNick(string& nick, long long group = 0) const;
	void setNick(long long group, string val)
	{
		std::lock_guard<std::mutex> lock_queue(ex_user);
		strNick[group] = std::move(val);
	}

	bool rmNick(long long group)
	{
		std::lock_guard<std::mutex> lock_queue(ex_user);
		if (auto it = strNick.find(group); it != strNick.end() || (it = strNick.find(0)) != strNick.end())
		{
			strNick.erase(it);
			return true;
		}
		else if (strNick.size() == 1) {
			strNick.clear();
			return true;
		}
		return false;
	}
	void clrNick(){
		std::lock_guard<std::mutex> lock_queue(ex_user);
		strNick.clear();
	}

	void writeb(std::ofstream& fout);

	void old_readb(std::ifstream& fin);
	void readb(std::ifstream& fin);
};

ifstream& operator>>(ifstream& fin, User& user);
extern unordered_map<long long, User> UserList;
extern unordered_map<long long, long long> TinyList;
User& getUser(long long qq); 
AttrVar getUserItem(long long uid, const string& item);
AttrVar getGroupItem(long long uid, const string& item);
AttrVar getSelfItem(string item);
AttrVar getContextItem(AttrObject context, string item, bool isTrust = true);
int trustedQQ(long long qq);
int clearUser();
int clearGroup();

string getName(long long QQ, long long GroupID = 0);
AttrVar idx_nick(AttrObject&);
string filter_CQcode(const string&, long long fromGID = 0);
//forward msg
string forward_filter(const string&, long long fromGID = 0);

extern const map<string, short> mChatConf;

//群聊记录
class Chat
{
public:
	bool isGroup = true;
	long long inviter = 0;
	long long ID = 0;
	string Name = "";
	time_t tCreated = time(nullptr);

	Chat() {}

	AttrObject confs;
	map<long long, AttrObject>ChConf;

	Chat& id(long long grp);

	Chat& group()
	{
		isGroup = true;
		return *this;
	}

	Chat& channel()
	{
		isGroup = false;
		return *this;
	}
	time_t getLst()const { return (time_t)confs.get_ll("lastMsg"); }
	void rmLst()const { confs.reset("lastMsg"); }
	Chat& setLst(time_t t) { confs.set("lastMsg", (long long)t); return *this; }

	Chat& name(string s)
	{
		Name = std::move(s);
		return *this;
	}

	Chat& create(time_t tt)
	{
		if (tt < tCreated)tCreated = tt;
		return *this;
	}

	Chat& update(){
		confs.set("tUpdated", (long long)time(nullptr));
		return *this;
	}
	Chat& update(time_t tt)	{
		confs.set("tUpdated", (long long)tt);
		return *this;
	}
	time_t updated()const { return confs.get_ll("tUpdated"); }

	Chat& set(const string& item){
		confs.set(item);
		return *this;
	}
	void set(const string& item, const AttrVar& val) {
		confs.set(item, val);
		update();
	}

	Chat& reset(const string& item)
	{
		confs.reset(item);
		update();
		return *this;
	}
	int getConf(const string& key, int def = 0) {
		if (confs.has(key))return confs.get_int(key);
		return def;
	}
	int getChConf(long long chid, const string& key, int def = 0) {
		if (ChConf.count(chid) && ChConf[chid].has(key))return ChConf[chid].get_int(key);
		return def;
	}
	void setChConf(long long chid, const string& key, const AttrVar& val) {
		ChConf[chid].set(key, val);
		update();
	}

	void leave(const string& msg = "");

	[[nodiscard]] bool isset(const string& key) const
	{
		return confs.is(key);
	}

	bool is_except()const;


	string listBoolConf()
	{
		ResList res;
		for (const auto& [it,n] : mChatConf) {
			if (confs.has(it))res << it;
		}
		return res.dot("+").show();
	}

	void writeb(std::ofstream& fout);

	void readb(std::ifstream& fin);
};

extern unordered_map<long long, Chat> ChatList;
Chat& chat(long long id);
int groupset(long long id, const string& st);
string printChat(Chat& grp);
ifstream& operator>>(ifstream& fin, Chat& grp);
ofstream& operator<<(ofstream& fout, const Chat& grp);

#ifdef _WIN32
DWORD getRamPort();

/*static DWORD getRamPort() {
	typedef BOOL(WINAPI * func)(LPMEMORYSTATUSEX);
	MEMORYSTATUSEX stMem = { 0 };
	func GlobalMemoryStatusEx = (func)GetProcAddress(LoadLibrary(L"Kernel32.dll"), "GlobalMemoryStatusEx");
	stMem.dwLength = sizeof(stMem);
	GlobalMemoryStatusEx(&stMem);
	return stMem.dwMemoryLoad;
}*/

__int64 compareFileTime(const FILETIME& ft1, const FILETIME& ft2);

//WIN CPU使用情况
long long getWinCpuUsage();

long long getProcessCpu();

long long getDiskUsage(double&, double&);
#endif