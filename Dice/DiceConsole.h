/*
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#ifndef Dice_Console
#define Dice_Console
#include <string>
#include <vector>
#include <map>
#include <set>
#include <time.h>
#include <Windows.h>
#include <thread>
#include <chrono>
#include "STLExtern.hpp"
#include "DiceXMLTree.h"
#include "DiceFile.hpp"
#include "MsgFormat.h"
#include "GlobalVar.h"
#include "DiceMsgSend.h"
#include "CQEVE_ALL.h"
#include "BlackMark.hpp"
using namespace std::literals::chrono_literals;
using std::string;
using std::to_string;
enum class ClockEvent { off, on, save, clear };
class Console {
public:
	bool isMasterMode = false;
	long long masterQQ = 0;
	long long DiceMaid = 0;
	Console(string path) :strPath(path) {}
	friend void ConsoleTimer();
	friend class FromMsg;
	using Clock = std::pair<unsigned short, unsigned short>;
	static const enumap<string> mClockEvent;
	static std::string printClock(Clock clock) {
		return to_string(clock.first) + ":" + (clock.second < 10 ? "0" : "") + to_string(clock.second);
	}
	static Clock scanClock(string s) {
		size_t pos = 0;
		string sHour,sMin;
		while (!isdigit(static_cast<unsigned char>(s[pos])) && pos < s.length())pos++;
		while (isdigit(static_cast<unsigned char>(s[pos])))sHour += s[pos++];
		if (sHour.empty())return { 0,0 };
		while (!isdigit(static_cast<unsigned char>(s[pos])) && pos < s.length())pos++;
		while (isdigit(static_cast<unsigned char>(s[pos])))sMin += s[pos++];
		if (sMin.empty())return { stoi(sHour),0 };
		return { stoi(sHour),stoi(sMin) };
	}
	static const std::map<std::string, int>intDefault;
	//通知列表 1-日常活动/2-提醒事件/4-接收消息/8-警告内容/16-用户推送/32-骰娘广播
	int log(std::string msg, int lv, std::string strTime = "");
	operator bool()const{ return isMasterMode && masterQQ;}
	long long master()const { return masterQQ; }
	void newMaster(long long);
	void killMaster() { rmNotice({ masterQQ,CQ::Private }); masterQQ = 0; save(); }
	int operator[](const char* key)const {
		auto it = intConf.find(key);
		if (it != intConf.end() || (it = intDefault.find(key)) != intDefault.end())return it->second;
		else return 0;
	}
	int setClock(Clock c, ClockEvent e);
	int rmClock(Clock c, ClockEvent e);
	ResList listClock()const;
	ResList listNotice()const;
	int showNotice(chatType ct)const;
	void set(std::string key, int val) {intConf[key] = val;	save();}
	void addNotice(chatType ct, int lv);
	void redNotice(chatType ct, int lv);
	void setNotice(chatType ct, int lv);
	void rmNotice(chatType ct);
	void reset();
	bool load() {
		string s;
		if(!rdbuf(strPath,s))return false;
		DDOM xml(s);
		if (xml.count("mode"))isMasterMode = stoi(xml["mode"].strValue);
		if (xml.count("master"))masterQQ = stoll(xml["master"].strValue);
		if (xml.count("clock"))
			for (auto& child : xml["clock"].vChild) {
				if(mClockEvent.count(child.tag))mWorkClock.insert({ scanClock(child.strValue),(ClockEvent)mClockEvent[child.tag] });
			}
		if (xml.count("conf"))
			for (auto& child : xml["conf"].vChild) {
				std::pair<string, int>conf;
				readini(child.strValue, conf);
				log(conf.first, 0);
				if(intDefault.count(conf.first))intConf.insert(conf);
			}
		loadNotice();
		return true;
	}
	void save() {
		DDOM xml("console","");
		xml.push(DDOM("mode", to_string(isMasterMode)));
		xml.push(DDOM("master", to_string(masterQQ)));
		if (!mWorkClock.empty()) {
			DDOM clocks("clock", "");
			for (auto &[clock, type]: mWorkClock) {
				clocks.push(DDOM(mClockEvent[(int)type], printClock(clock)));
			}
			xml.push(clocks);
		}
		if (!intConf.empty()) {
			DDOM conf("conf", "");
			for (auto& [item, val] : intConf) {
				conf.push(DDOM("n", item + "=" + to_string(val)));
			}
			xml.push(conf);
		}
		std::ofstream fout(strPath);
		fout << xml.dump();
	}
	void loadNotice();
	void saveNotice();
private:
	string strPath;
	std::map<std::string, int>intConf;
	std::multimap<Clock, ClockEvent>mWorkClock{};
	std::map<chatType, int> NoticeList{};
};
extern Console console;
	//Master的QQ，无主时为0
	static long long DiceMaid;
	//骰娘列表
	extern std::map<long long, long long> mDiceList;
	//个性化语句
	extern std::map<std::string, std::string> PersonalMsg;
	//黑名单群：无条件禁用
	extern std::set<long long> BlackGroup;
	//黑名单用户：无条件禁用
	extern std::set<long long> BlackQQ;
	extern std::map<long long, BlackMark>mBlackQQMark;
	extern std::map<long long, BlackMark>mBlackGroupMark;
void loadBlackMark(std::string strPath);
void saveBlackMark(std::string strPath);
	//获取骰娘列表
	void getDiceList();
	struct fromMsg {
		std::string strMsg;
		long long fromQQ = 0;
		long long fromGroup = 0;
		fromMsg() = default;
		fromMsg(std::string msg, long long QQ, long long Group) :strMsg(msg), fromQQ(QQ), fromGroup(Group) {};
	};
	//通知
	//一键清退
	extern int clearGroup(std::string strPara = "unpower", long long fromQQ = 0);
	//连接的聊天窗口
	extern std::map<chatType, chatType> mLinkedList;
	//单向转发列表
	extern std::multimap<chatType, chatType> mFwdList;
	//程序启动时间
	extern long long llStartTime;
	//当前时间
	extern SYSTEMTIME stNow;
	std::string printClock(std::pair<int, int> clock);
	std::string printSTime(SYSTEMTIME st);
	std::string printSTNow(); 
	std::string printDate();
	std::string printDate(time_t tt);
	std::string printQQ(long long);
	std::string printGroup(long long);
	std::string printChat(chatType);
//拉黑用户后检查群
void checkBlackQQ(BlackMark &mark);
//拉黑用户
bool addBlackQQ(BlackMark mark);
bool addBlackGroup(const BlackMark &mark);
bool rmBlackQQ(long long llQQ, long long operateQQ = 0);
bool rmBlackGroup(long long llQQ, long long operateQQ = 0);
void AddWarning(const std::string& msg, long long DiceQQ, long long fromGroup);
void warningHandler();
extern void ConsoleTimer();
class ThreadFactory {
public:
	ThreadFactory(){}
	int rear = 0;
	std::thread vTh[4];
	void operator()(void(*func)()) {
		std::thread th(func);
		th.detach();
		vTh[rear] = std::move(th);
		rear++;
	}
};
static ThreadFactory threads;
#endif /*Dice_Console*/


