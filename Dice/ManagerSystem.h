/*
 * 后台系统
 * Copyright (C) 2019 String.Empty
 */
#pragma once
#include <set>
#include <map>
#include <vector>
#include "DiceFile.hpp"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "CardDeck.h"
using std::string;
using std::to_string;
using std::set;
using std::map;
using std::vector;
constexpr auto CQ_IMAGE = "[CQ:image,file=";

//加载数据
void loadData();
//保存数据
void dataBackUp();
//被引用的图片列表
static set<string> sReferencedImage;
static map<long long, string> WelcomeMsg;

static void scanImage(string s, set<string>& list) {
	int l = 0, r = 0;
	while ((l = s.find('[', r)) != string::npos && (r = s.find(']', l)) != string::npos) {
		if (s.substr(l, 15) != CQ_IMAGE)continue;
		list.insert(s.substr(l + 15, r - l - 15) + ".cqimg");
	}
}
static void scanImage(const vector<string>& v, set<string>& list) {
	for (auto it : v) {
		scanImage(it, sReferencedImage);
	}
}
template<typename TKey,typename TVal>
static void scanImage(const map<TKey, TVal>& m, set<string>& list) {
	for (auto it : m) {
		scanImage(it.second, sReferencedImage);
	}
}

static int clearImage() {
	scanImage(GlobalMsg, sReferencedImage);
	scanImage(HelpDoc, sReferencedImage);
	scanImage(CardDeck::mPublicDeck, sReferencedImage);
	scanImage(CardDeck::mReplyDeck, sReferencedImage);
	scanImage(CardDeck::mGroupDeck, sReferencedImage);
	scanImage(CardDeck::mPrivateDeck, sReferencedImage);
	scanImage(WelcomeMsg, sReferencedImage);
	string strLog = "整理被引用图片" + to_string(sReferencedImage.size()) + "项";
	addRecord(strLog);
	return clrDir("data\\image\\", sReferencedImage);
}

static DWORD getRamPort() {
	typedef BOOL(WINAPI * func)(LPMEMORYSTATUSEX);
	MEMORYSTATUSEX stMem = { 0 };
	func GlobalMemoryStatusEx = (func)GetProcAddress(LoadLibrary(L"Kernel32.dll"), "GlobalMemoryStatusEx");
	stMem.dwLength = sizeof(stMem);
	GlobalMemoryStatusEx(&stMem);
	return stMem.dwMemoryLoad;
}

static __int64 compareFileTime(FILETIME& ft1, FILETIME& ft2) {
	__int64 t1 = ft1.dwHighDateTime;
	t1 = t1 << 32 | ft1.dwLowDateTime;
	__int64 t2 = ft2.dwHighDateTime;
	t2 = t2 << 32 | ft2.dwLowDateTime;
	return t1 - t2;
}

//WIN CPU使用情况
static __int64 getWinCpuUsage() {
	HANDLE hEvent;
	BOOL res;
	FILETIME preidleTime;
	FILETIME prekernelTime;
	FILETIME preuserTime;
	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	res = GetSystemTimes(&idleTime, &kernelTime, &userTime);
	preidleTime = idleTime;
	prekernelTime = kernelTime;
	preuserTime = userTime;

	hEvent = CreateEventA(NULL, FALSE, FALSE, NULL); // 初始值为 nonsignaled ，并且每次触发后自动设置为nonsignaled
	WaitForSingleObject(hEvent, 1000);
	res = GetSystemTimes(&idleTime, &kernelTime, &userTime);

	__int64 idle = compareFileTime(idleTime, preidleTime);
	__int64 kernel = compareFileTime(kernelTime, prekernelTime);
	__int64 user = compareFileTime(userTime, preuserTime);

	__int64 cpu = (kernel + user - idle) * 100 / (kernel + user);
	return cpu;
}

int getProcessCpu()
{
	HANDLE hProcess = GetCurrentProcess();
	//if (INVALID_HANDLE_VALUE == hProcess){return -1;}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	__int64 iCpuNum = si.dwNumberOfProcessors;

	FILETIME ftPreKernelTime, ftPreUserTime;
	FILETIME ftKernelTime, ftUserTime;
	FILETIME ftCreationTime, ftExitTime;
	std::ofstream log("System.log");

	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftPreKernelTime, &ftPreUserTime)){return -1;}
	log << ftPreKernelTime.dwLowDateTime << "\n" << ftPreUserTime.dwLowDateTime << "\n";
	HANDLE hEvent = CreateEventA(NULL, FALSE, FALSE, NULL); // 初始值为 nonsignaled ，并且每次触发后自动设置为nonsignaled
	WaitForSingleObject(hEvent, 1000);
	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime)){return -1;}
	log << ftKernelTime.dwLowDateTime << "\n" << ftUserTime.dwLowDateTime << "\n";
	__int64 ullKernelTime = compareFileTime(ftKernelTime, ftPreKernelTime);
	__int64 ullUserTime = compareFileTime(ftUserTime, ftPreUserTime);
	log << ullKernelTime << "\n" << ullUserTime << "\n" << iCpuNum;
	__int64 dCpu = (ullKernelTime + ullUserTime) / (iCpuNum * 100);
	return (int)dCpu;
}