/*
 * Copyright (C) 2019 String.Empty
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
#include "GlobalVar.h"
#include "DiceMsgSend.h"
#include "CQEVE_ALL.h"
#include "BlackMark.hpp"

	//Master模式
	extern bool boolMasterMode;
	//Master的QQ，无主时为0
	extern long long masterQQ;
	extern long long DiceMaid;
	//管理员列表
	extern std::set<long long> AdminQQ;
	//日志窗口列表
	extern std::set<chatType> RecorderList;
	//监控窗口列表
	extern std::set<std::pair<long long, CQ::msgtype>> MonitorList;
	//各类全局开关
	extern std::map<std::string, bool>boolConsole;
	//骰娘列表
	extern std::map<long long, long long> mDiceList;
	//个性化语句
	extern std::map<std::string, std::string> PersonalMsg;
	//botoff的群
	extern std::set<long long> DisabledGroup;
	//botoff的讨论组
	extern std::set<long long> DisabledDiscuss;
	//白名单群：私用模式豁免
	extern std::set<long long> WhiteGroup;
	//黑名单群：无条件禁用
	extern std::set<long long> BlackGroup;
	//白名单用户：邀请私用骰娘
	extern std::set<long long> WhiteQQ;
	//黑名单用户：无条件禁用
	extern std::set<long long> BlackQQ;
	extern std::map<long long, BlackMark>mBlackQQMark;
	extern std::map<long long, BlackMark>mBlackGroupMark;
//加载数据
void loadData();
//保存数据
void dataBackUp(); 
void loadBlackMark(std::string strPath);
void saveBlackMark(std::string strPath);
	//获取骰娘列表
	void getDiceList();
	//通知管理员 
	void sendAdmin(std::string strMsg, long long fromQQ = 0);
	//添加日志
	void addRecord(std::string strMsg);
	//通知监控窗口 
	void NotifyMonitor(std::string strMsg);
	//一键清退
	extern int clearGroup(std::string strPara = "unpower", long long fromQQ = 0);
	//最近消息记录
	extern std::map<chatType, time_t> mLastMsgList;
	//连接的聊天窗口
	extern std::map<chatType, chatType> mLinkedList;
	//单向转发列表
	extern std::multimap<chatType, chatType> mFwdList;
	//群邀请者
	extern std::map<long long,long long> mGroupInviter;
	//程序启动时间
	extern long long llStartTime;
	//当前时间
	extern SYSTEMTIME stNow;
	//上班时间
	extern std::pair<int, int> ClockToWork;
	//下班时间
	extern std::pair<int, int> ClockOffWork;
	std::string printClock(std::pair<int, int> clock);
	std::string printSTime(SYSTEMTIME st);
	std::string printQQ(long long);
	std::string printGroup(long long);
	std::string printChat(chatType);
//拉黑用户后检查群
void checkBlackQQ(BlackMark &mark);
//拉黑用户
bool addBlackQQ(BlackMark mark);
bool addBlackGroup(BlackMark &mark);
void rmBlackQQ(long long llQQ, long long operateQQ = 0);
void rmBlackGroup(long long llQQ, long long operateQQ = 0);
extern std::set<std::string> strWarningList;
void AddWarning(const std::string& msg, long long DiceQQ, long long fromGroup);
void warningHandler();
	extern void ConsoleTimer();
#endif /*Dice_Console*/


