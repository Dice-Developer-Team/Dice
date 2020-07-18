/*
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#ifndef Dice_Console
#define Dice_Console
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <set>
#include <Windows.h>
#include <thread>
#include <chrono>
#include "STLExtern.hpp"
#include "DiceXMLTree.h"
#include "DiceFile.hpp"
#include "MsgFormat.h"
#include "DiceMsgSend.h"
#include "CQEVE_ALL.h"
using namespace std::literals::chrono_literals;
using std::string;
using std::to_string;

enum class ClockEvent { off, on, save, clear };

class Console
{
public:
	bool isMasterMode = false;
	long long masterQQ = 0;
	long long DiceMaid = 0;
	friend void ConsoleTimer();
	friend class FromMsg;
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
	//֪ͨ�б� 1-�ճ��/2-�����¼�/4-������Ϣ/8-��������/16-�û�����/32-����㲥
	int log(const std::string& msg, int lv, const std::string& strTime = "");
	operator bool() const { return isMasterMode && masterQQ; }
	[[nodiscard]] long long master() const { return masterQQ; }
	void newMaster(long long);

	void killMaster()
	{
		rmNotice({masterQQ, CQ::msgtype::Private});
		masterQQ = 0;
		save();
	}

	int operator[](const char* key) const
	{
		auto it = intConf.find(key);
		if (it != intConf.end() || (it = intDefault.find(key)) != intDefault.end())return it->second;
		return 0;
	}

	int setClock(Clock c, ClockEvent e);
	int rmClock(Clock c, ClockEvent e);
	[[nodiscard]] ResList listClock() const;
	[[nodiscard]] ResList listNotice() const;
	[[nodiscard]] int showNotice(chatType ct) const;
	void setPath(std::string path) { strPath = std::move(path); }

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

	bool load()
	{
		string s;
		//DSens.build({ {"nn�Ϲ�",2 } });
		if (!rdbuf(strPath, s))return false;
		DDOM xml(s);
		if (xml.count("mode"))isMasterMode = stoi(xml["mode"].strValue);
		if (xml.count("master"))masterQQ = stoll(xml["master"].strValue);
		if (xml.count("clock"))
			for (auto& child : xml["clock"].vChild)
			{
				if (mClockEvent.count(child.tag))mWorkClock.insert({
					scanClock(child.strValue), static_cast<ClockEvent>(mClockEvent[child.tag])
				});
			}
		if (xml.count("conf"))
			for (auto& child : xml["conf"].vChild)
			{
				std::pair<string, int> conf;
				readini(child.strValue, conf);
				if (intDefault.count(conf.first))intConf.insert(conf);
			}
		loadNotice();
		return true;
	}

	void save()
	{
		DDOM xml("console", "");
		xml.push(DDOM("mode", to_string(isMasterMode)));
		xml.push(DDOM("master", to_string(masterQQ)));
		if (!mWorkClock.empty())
		{
			DDOM clocks("clock", "");
			for (auto& [clock, type] : mWorkClock)
			{
				clocks.push(DDOM(mClockEvent[static_cast<int>(type)], printClock(clock)));
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
		std::ofstream fout(strPath);
		fout << xml.dump();
	}

	void loadNotice();
	void saveNotice() const;
private:
	string strPath;
	std::map<std::string, int, less_ci> intConf;
	std::multimap<Clock, ClockEvent> mWorkClock{};
	std::map<chatType, int> NoticeList{};
};

extern std::map<std::string, int, less_ci> ConsoleSafe;

extern Console console;
//extern DiceModManager modules;
//�����б�
extern std::map<long long, long long> mDiceList;
//��ȡ�����б�
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

//֪ͨ
//һ������
extern int clearGroup(std::string strPara = "unpower", long long fromQQ = 0);
//���ӵ����촰��
extern std::map<chatType, chatType> mLinkedList;
//����ת���б�
extern std::multimap<chatType, chatType> mFwdList;
//��������ʱ��
extern long long llStartTime;
//��ǰʱ��
extern SYSTEMTIME stNow;
std::string printClock(std::pair<int, int> clock);
std::string printSTime(SYSTEMTIME st);
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
	ThreadFactory()
	{
	}

	int rear = 0;
	std::thread vTh[4];

	void operator()(void (*func)())
	{
		std::thread th(func);
		th.detach();
		vTh[rear] = std::move(th);
		rear++;
	}
};

extern ThreadFactory threads;
#endif /*Dice_Console*/
