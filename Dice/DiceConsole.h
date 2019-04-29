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

	//Master的QQ，无主时为0
	extern long long masterQQ;
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
	//一键清退
	extern int clearGroup(std::string strPara = "unpower");
	//命令处理
	extern void Process(std::string message);
#endif /*Dice_Console*/


