#pragma once

/*
 * 后台系统
 * Copyright (C) 2019-2020 String.Empty
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
constexpr auto CQ_IMAGE = "[CQ:image,file=";
constexpr auto CQ_AT = "[CQ:at,qq=";
constexpr time_t NEWYEAR = 1609430400;

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
	short nTrust = 0;
	time_t tCreated = time(nullptr);
	time_t tUpdated = 0;

	User()
	{
	}

	map<string, int> intConf{};
	map<string, string> strConf{};
	map<long long, string> strNick{};
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

	User& update(time_t tt)
	{
		tUpdated = tt;
		return *this;
	}

	User& trust(short n)
	{
		nTrust = n;
		return *this;
	}

	[[nodiscard]] bool empty() const
	{
		return (!nTrust) && (!tUpdated) && intConf.empty() && strConf.empty() && strNick.empty();
	}

	[[nodiscard]] string show() const
	{
		ResList res;
		//res << "昵称记录数:" + to_string(strNick.size());
		for (auto& [key, val] : strConf)
		{
			res << key + ":" + val;
		}
		for (auto& [key, val] : intConf)
		{
			res << key + ":" + to_string(val);
		}
		return res.show();
	}

	void setConf(const string& key, int val)
	{
		if (key.empty())return;
		std::lock_guard<std::mutex> lock_queue(ex_user);
		intConf[key] = val;
	}

	void setConf(const string& key, const string& val)
	{
		std::lock_guard<std::mutex> lock_queue(ex_user);
		strConf[key] = val;
	}

	void rmIntConf(const string& key)
	{
		std::lock_guard<std::mutex> lock_queue(ex_user);
		intConf.erase(key);
	}

	void rmStrConf(const string& key)
	{
		std::lock_guard<std::mutex> lock_queue(ex_user);
		strConf.erase(key);
	}

	bool getNick(string& nick, long long group = 0) const
	{
		if (auto it = strNick.find(group); it != strNick.end() || (it = strNick.find(0)) != strNick.end())
		{
			nick = it->second;
			return true;
		}
		return false;
	}

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
		return false;
	}

	void writeb(std::ofstream& fout)
	{
		std::lock_guard<std::mutex> lock_queue(ex_user);
		fwrite(fout, ID);
		fwrite(fout, intConf);
		fwrite(fout, strConf);
		fwrite(fout, strNick);
	}

	void readb(std::ifstream& fin)
	{
		std::lock_guard<std::mutex> lock_queue(ex_user);
		ID = fread<long long>(fin);
		intConf = fread<string, int>(fin);
		strConf = fread<string, string>(fin);
		strNick = fread<long long, string>(fin);
	}
};

ifstream& operator>>(ifstream& fin, User& user);
ofstream& operator<<(ofstream& fout, const User& user);
extern unordered_map<long long, User> UserList;
User& getUser(long long qq);
short trustedQQ(long long qq);
int clearUser();

string getName(long long QQ, long long GroupID = 0);
void filter_CQcode(string&, long long fromGroup = 0);

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
	time_t tUpdated = 0;
	time_t tLastMsg = 0;

	Chat()
	{
	}

	set<string> boolConf{};
	map<string, int> intConf{};
	map<string, string> strConf{};

	Chat& id(long long grp);

	Chat& group()
	{
		isGroup = true;
		return *this;
	}

	Chat& discuss()
	{
		isGroup = false;
		return *this;
	}

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

	Chat& update(time_t tt)
	{
		tUpdated = tt;
		return *this;
	}

	Chat& lastmsg(time_t tt)
	{
		tLastMsg = tt;
		return *this;
	}

	Chat& set(const string& item)
	{
		if (mChatConf.count(item))boolConf.insert(item);
		return *this;
	}

	Chat& reset(const string& item)
	{
		boolConf.erase(item);
		return *this;
	}

	void leave(const string& msg = "");

	[[nodiscard]] bool isset(const string& key) const
	{
		return boolConf.count(key) || intConf.count(key) || strConf.count(key);
	}

	bool is_except()const;

	void setConf(const string& key, int val)
	{
		intConf[key] = val;
	}

	void rmConf(const string& key)
	{
		intConf.erase(key);
	}

	string listBoolConf()
	{
		ResList res;
		for (const auto& it : boolConf)
		{
			res << it;
		}
		return res.dot("+").show();
	}

	void setText(const string& key, string val)
	{
		strConf[key] = std::move(val);
	}

	void rmText(const string& key)
	{
		strConf.erase(key);
	}

	void writeb(std::ofstream& fout)
	{
		fwrite(fout, ID);
		if (!Name.empty())
		{
			fwrite(fout, static_cast<short>(0));
			fwrite(fout, Name);
		}
		if (!boolConf.empty())
		{
			fwrite(fout, static_cast<short>(1));
			fwrite(fout, boolConf);
		}
		if (!intConf.empty())
		{
			fwrite(fout, static_cast<short>(2));
			fwrite(fout, intConf);
		}
		if (!strConf.empty())
		{
			fwrite(fout, static_cast<short>(3));
			fwrite(fout, strConf);
		}
		fwrite(fout, static_cast<short>(-1));
	}

	void readb(std::ifstream& fin)
	{
		ID = fread<long long>(fin);
		short tag = fread<short>(fin);
		while (tag != -1)
		{
			switch (tag)
			{
			case 0:
				Name = fread<string>(fin);
				break;
			case 1:
				boolConf = fread<string, true>(fin);
				break;
			case 2:
				intConf = fread<string, int>(fin);
				break;
			case 3:
				strConf = fread<string, string>(fin);
				break;
			default:
				return;
			}
			tag = fread<short>(fin);
		}
		//strConf = fread<string, string>(fin);
	}
};

inline unordered_map<long long, Chat> ChatList;
Chat& chat(long long id);
int groupset(long long id, const string& st);
string printChat(Chat& grp);
ifstream& operator>>(ifstream& fin, Chat& grp);
ofstream& operator<<(ofstream& fout, const Chat& grp);

extern unordered_set<std::string>sReferencedImage;

void scanImage(const string& s, unordered_set<string>& list);

void scanImage(const vector<string>& v, unordered_set<string>& list);

template <typename TVal, typename sort>
void scanImage(const map<string, TVal, sort>& m, unordered_set<string>& list)
{
	for (const auto& it : m)
	{
		scanImage(it.first, sReferencedImage);
		scanImage(it.second, sReferencedImage);
	}
}

template <typename TVal>
void scanImage(const map<string, TVal>& m, unordered_set<string>& list)
{
	for (const auto& it : m)
	{
		scanImage(it.first, sReferencedImage);
		scanImage(it.second, sReferencedImage);
	}
}

template <typename TKey, typename TVal>
void scanImage(const map<TKey, TVal>& m, unordered_set<string>& list)
{
	for (const auto& it : m) 
	{
		scanImage(it.second, sReferencedImage);
	}
}

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