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
#include "STLExtern.hpp"
#include "DiceXMLTree.h"
#include "DiceFile.hpp"
#include "MsgFormat.h"
#include "DiceMsgSend.h"
using namespace std::literals::chrono_literals;
using std::string;
using std::to_string;

extern std::filesystem::path dirExe;
extern std::filesystem::path DiceDir;

//enum class ClockEvent { off, on, save, clear };

class Console
{
public:
	bool isMasterMode = false;
	long long masterQQ = 0;
	long long DiceMaid = 0;
	bool is_self(long long qq)const { return masterQQ == qq || DiceMaid == qq; }
	friend void ConsoleTimer();
	friend class FromMsg;
	friend class DiceJob;
	//DiceSens DSens;
	using Clock = std::pair<unsigned short, unsigned short>;
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

	static const std::map<std::string, int, less_ci> intDefault;
	//通知列表 1-日常活动/2-提醒事件/4-接收消息/8-警告内容/16-用户推送/32-骰娘广播
	int log(const std::string& msg, int lv, const std::string& strTime = "");
	operator bool() const { return isMasterMode && masterQQ; }
	[[nodiscard]] long long master() const { return masterQQ; }
	void newMaster(long long);

	void killMaster()
	{
		rmNotice({masterQQ, msgtype::Private});
		masterQQ = 0;
		save();
	}

	int operator[](const char* key) const
	{
		auto it = intConf.find(key);
		if (it != intConf.end() || (it = intDefault.find(key)) != intDefault.end())return it->second;
		return 0;
	}

	int setClock(Clock c, const string&);
	int rmClock(Clock c, const string&);
	[[nodiscard]] ResList listClock() const;
	[[nodiscard]] ResList listNotice() const;
	[[nodiscard]] int showNotice(chatType ct) const;
	void setPath(const std::filesystem::path& path) { fpPath = path; }

	void set(const std::string& key, int val)
	{
		intConf[key] = val;
		save();
	}

	void addNotice(chatType ct, int lv);
	void redNotice(chatType ct, int lv);
	void setNotice(chatType ct, int lv);
	void rmNotice(chatType ct);
	void reset();

	bool load();
	void save() 
	{
		std::error_code ec;
		std::filesystem::create_directories(DiceDir / "conf", ec);
		DDOM xml("console","");
		xml.push(DDOM("mode", to_string(isMasterMode)));
		xml.push(DDOM("master", to_string(masterQQ)));
		if (!mWorkClock.empty())
		{
			DDOM clocks("clock", "");
			for (auto& [clock, type] : mWorkClock)
			{
				clocks.push(DDOM(type, printClock(clock)));
			}
			xml.push(clocks);
		}
		if (!intConf.empty())
		{
			DDOM conf("conf", "");
			for (auto& [item, val] : intConf)
			{
				conf.push(DDOM("n", item + "=" + to_string(val)));
			}
			xml.push(conf);
		}
		std::ofstream fout(fpPath);
		if (fout) fout << xml.dump();
	}

	void loadNotice();
	void saveNotice() const;
private:
	std::filesystem::path fpPath;
	std::map<std::string, int, less_ci> intConf;
	std::multimap<Clock, string> mWorkClock{};
	std::map<chatType, int> NoticeList{};
};
	extern Console console;
	//extern DiceModManager modules;

extern std::set<long long> ExceptGroups;
void getExceptGroup();
	//骰娘列表
	extern std::map<long long, long long> mDiceList;
	//获取骰娘列表
	void getDiceList();

	struct fromMsg
	{
		std::string strMsg;
		long long fromQQ = 0;
		long long fromGroup = 0;
		fromMsg() = default;

		fromMsg(std::string msg, long long QQ, long long Group) : strMsg(std::move(msg)), fromQQ(QQ), fromGroup(Group)
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
	std::string printDate();
	std::string printDate(time_t tt);
	std::string printQQ(long long);
	std::string printGroup(long long);
	std::string printChat(chatType);
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
