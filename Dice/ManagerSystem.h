/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * 后台用户管理，系统监控
 * Copyright (C) 2018-2021 w4123溯洄
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
#pragma once 
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
class User :public AnysTable
{
public:
	MetaType getType()const override { return MetaType::Context; }
	long long ID = 0;
	//1-私用信任，2-拉黑豁免，3-加黑退群，4-后台管理，5-Master
	int nTrust = 0;
	time_t tCreated = time(nullptr);

	explicit User(long long id): ID(id){}
	unordered_map<long long, string> strNick{};
	std::mutex ex_user;
	bool has(const string& key)const override;
	AttrVar get(const string& key, const AttrVar& val = {})const override;

	User& create(time_t tt)
	{
		if (tt < tCreated)tCreated = tt;
		return *this;
	}

	User& update(time_t tt) {
		dict["tUpdated"] = (long long)tt;
		return *this;
	}
	time_t updated()const { return get_ll("tUpdated"); }

	User& trust(int n)
	{
		nTrust = n;
		dict["trust"] = n;
		return *this;
	}

	[[nodiscard]] bool empty() const;

	void setConf(const string& key, const AttrVar& val);
	void rmConf(const string& key);
	int getConf(const string& key, int def = 0) {
		if (has(key))return get_int(key);
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

extern unordered_map<long long, ptr<User>> UserList;
extern unordered_map<long long, long long> TinyList;
User& getUser(long long qq); 
AttrVar getUserItem(long long uid, const string& item);
AttrVar getGroupItem(long long uid, const string& item);
AttrVar getSelfItem(string item);
AttrVar getContextItem(const AttrObject& context, string item, bool isTrust = true);
int trustedQQ(long long qq);
int clearUser();
int clearGroup();

string getName(long long QQ, long long GroupID = 0);
AttrVar idx_nick(const AttrObject&);
string filter_CQcode(const string&, long long fromGID = 0);
//forward msg
string forward_filter(const string&, long long fromGID = 0);

extern const map<string, short> mChatConf;

//群聊记录
class Chat :public AnysTable
{
public:
	MetaType getType()const override { return MetaType::Context; }
	long long inviter = 0;
	long long ID = 0;
	mutable string Name = "";
	time_t tCreated = time(nullptr);

	explicit Chat(long long id):ID(id) {}

	map<long long, AnysTable>ChConf;

	bool has(const string& key)const override;
	AttrVar get(const string& key, const AttrVar& val = {})const override;
	//Chat& id(long long grp);
	time_t getLst()const { return (time_t)get_ll("lastMsg"); }
	void rmLst() { reset("lastMsg"); }
	Chat& setLst(time_t t);

	Chat& name(string s)
	{
		Name = std::move(s);
		return *this;
	}
	string print();

	Chat& create(time_t tt)
	{
		if (tt < tCreated)tCreated = tt;
		return *this;
	}

	Chat& update();
	Chat& update(time_t tt);
	time_t updated()const { return get_ll("tUpdated"); }

	Chat& set(const string& item){
		at(item) = true;
		return *this;
	}
	void set(const string& key, const AttrVar& val) {
		if (!key.empty()) {
			if (val.is_null())dict.erase(key);
			else dict[key] = val;
			update();
		}
	}

	Chat& reset(const string& item)
	{
		dict.erase(item);
		update();
		return *this;
	}
	int getConf(const string& key, int def = 0) {
		if (has(key))return get_int(key);
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

	bool is_except()const;


	string listBoolConf()
	{
		ResList res;
		for (const auto& [it,n] : mChatConf) {
			if (has(it))res << it;
		}
		return res.dot("+").show();
	}

	void writeb(std::ofstream& fout);

	void readb(std::ifstream& fin);
};

extern unordered_map<long long, ptr<Chat>> ChatList;
Chat& chat(long long id);
int groupset(long long id, const string& st);

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