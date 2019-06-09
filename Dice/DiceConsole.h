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
#include <Windows.h>
#include "GlobalVar.h"
#include "DiceMsgSend.h"
#include "CQEVE_ALL.h"

	//Master模式
	extern bool boolMasterMode;
	//Master的QQ，无主时为0
	extern long long masterQQ;
	//管理员列表
	extern std::set<long long> AdminQQ;
	//监控窗口列表
	extern std::set<std::pair<long long, CQ::msgtype>> MonitorList;
	//全部群静默
	extern bool boolDisabledGlobal;
	//全局禁用.ME
	extern bool boolDisabledMeGlobal;
	//全局禁用.jrrp
	extern bool boolDisabledJrrpGlobal;
	//独占模式：被拉进讨论组或Master不在的群则秒退
	extern bool boolPreserve;
	//自动退出一切讨论组
	extern bool boolNoDiscuss;
	//讨论组消息记录
	extern std::map<long long, time_t> DiscussList;
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
	//通知管理员 
	void sendAdmin(std::string strMsg, long long fromQQ = 0);
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
	//当前时间
	extern SYSTEMTIME stNow;
	//上班时间
	extern std::pair<int, int> ClockToWork;
	//下班时间
	extern std::pair<int, int> ClockOffWork;
	std::string printClock(std::pair<int, int> clock);
	extern void ConsoleTimer();
#endif /*Dice_Console*/


