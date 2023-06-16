#pragma once

/*
 * Copyright (C) 2019-2021 String.Empty
 */

#ifndef Dice_Console
#define Dice_Console
#include <ctime>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <array>
#include <CivetServer.h>
#include "STLExtern.hpp"
#include "DiceXMLTree.h"
#include "DiceFile.hpp"
#include "MsgFormat.h"
#include "DiceMsgSend.h"
#include "RandomGenerator.h"
using namespace std::literals::chrono_literals;
using std::string;
using std::to_string;

extern std::filesystem::path dirExe;
extern std::filesystem::path DiceDir;
extern std::unique_ptr<CivetServer> ManagerServer;

//enum class ClockEvent { off, on, save, clear };

using Clock = std::pair<unsigned short, unsigned short>;
class Console {
	long long master = 0;
public:
	long long DiceMaid = 0;
	bool is_self(long long qq)const { return master == qq || DiceMaid == qq; }
	string git_user;
	string git_pw;
	string authkey_pub{ RandomGenerator::genKey(8,RandomGenerator::Code::Alnum) };
	string authkey_pri{ RandomGenerator::genKey(8,RandomGenerator::Code::Alnum) };

	friend void ConsoleTimer();
	friend class DiceEvent;
	friend void MsgNote(AttrObject&, string, int);
	//DiceSens DSens;
	static const enumap<string> mClockEvent;

	static std::string printClock(Clock clock)
	{
		return to_string(clock.first) + ":" + (clock.second < 10 ? "0" : "") + to_string(clock.second);
	}

	static Clock scanClock(string s)
	{
		size_t pos = 0;
		string sHour, sMin;
		while (!isdigit(static_cast<unsigned char>(s[pos])) && pos < s.length())pos++;
		while (isdigit(static_cast<unsigned char>(s[pos])))sHour += s[pos++];
		if (sHour.empty())return {0, 0};
		while (!isdigit(static_cast<unsigned char>(s[pos])) && pos < s.length())pos++;
		while (isdigit(static_cast<unsigned char>(s[pos])))sMin += s[pos++];
		if (sMin.empty())return {stoi(sHour), 0};
		return {stoi(sHour), stoi(sMin)};
	}

	static const fifo_dict_ci<int> intDefault;
	static const std::unordered_map<std::string, string> confComment;
	//通知列表 1-日常活动/2-提醒事件/4-接收消息/8-警告内容/16-用户推送/32-骰娘广播
	int log(const std::string& msg, int lv, const std::string& strTime = "");
	void log(const std::string& msg, const std::string& file);
	operator long long() const { return master; }
	void newMaster(long long);

	void killMaster()
	{
		rmNotice({ master });
		master = 0;
		save();
	}

	int operator[](const string& key) const	{
		if (auto it = intConf.find(key); it != intConf.end() || (it = intDefault.find(key)) != intDefault.end())return it->second;
		return 0;
	}
	int operator[](const char* key) const {
		if (auto it = intConf.find(key); it != intConf.end() || (it = intDefault.find(key)) != intDefault.end())return it->second;
		return 0;
	}

	int setClock(Clock c, const string&);
	int rmClock(Clock c, const string&);
	[[nodiscard]] ResList listClock() const;
	[[nodiscard]] ResList listNotice() const;
	[[nodiscard]] int showNotice(chatInfo ct) const;

	void set(const std::string& key, int val)
	{
		intConf[key] = val;
		save();
	}

	void addNotice(chatInfo ct, int lv);
	void redNotice(chatInfo ct, int lv);
	void setNotice(chatInfo ct, int lv);
	void rmNotice(chatInfo ct);
	void reset();

	bool load();
	void save();

	void loadNotice();
	void saveNotice() const;
private:
	fifo_dict_ci<int> intConf;
	std::multimap<Clock, string> mWorkClock{};
	fifo_map<chatInfo, int> NoticeList{};
};
extern Console console;

extern std::set<long long> ExceptGroups;
void getExceptGroup();
	//骰娘列表
	extern std::map<long long, long long> mDiceList;
	//获取骰娘列表
	void getDiceList();

	struct fromMsg
	{
		std::string strMsg;
		long long fromUID = 0;
		long long fromGID = 0;
		fromMsg() = default;

		fromMsg(std::string msg, long long QQ, long long Group) : strMsg(std::move(msg)), fromUID(QQ), fromGID(Group)
		{
		};
	};

	//程序启动时间
	extern long long llStartTime;
	//当前时间
	extern tm stNow;
	std::string printClock(std::pair<int, int> clock);
	std::string printSTime(tm st);
	std::string printSTNow(); 
	std::string printDate(tm st);
	std::string printDate();
	std::string printDate(time_t tt);
	std::string printUser(long long);
	std::string printGroup(long long);
	std::string printChat(chatInfo);
void ConsoleTimer();

class ThreadFactory
{
public:
	ThreadFactory() {}

	int rear = 0;
	std::array<std::thread, 6> vTh;

	void operator()(void (*func)()) {
		std::thread th{ [func]() {try { func(); } catch (...) { return; }} };
		vTh[rear] = std::move(th);
		//vTh[rear].detach();
		rear++;
	}
	void exit();
	~ThreadFactory() {
		exit();
	}
};

extern ThreadFactory threads;
#endif /*Dice_Console*/
