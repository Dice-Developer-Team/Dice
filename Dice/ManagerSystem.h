/*
 * 后台系统
 * Copyright (C) 2019-2020 String.Empty
 * 控制清理用户/群聊记录，清理图片，监控系统
 */
#pragma once
#include <set>
#include <map>
#include <vector>
#include <mutex>
#include "DiceFile.hpp"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "CardDeck.h"
#include "MsgFormat.h"
using std::string;
using std::to_string;
using std::set;
using std::map;
using std::vector;
constexpr auto CQ_IMAGE = "[CQ:image,file=";
constexpr time_t NEWYEAR = 1580486400;

//加载数据
void loadData();
//保存数据
void dataBackUp();
//用户记录
class User {
public:
	long long ID = 0;
	//1-私用信任，2-拉黑豁免，3-加黑退群，4-后台管理，5-Master
	short nTrust = 0;
	time_t tCreated = time(NULL);
	time_t tUpdated = 0;
	User() {}
	map<string, int> intConf{};
	map<string, string> strConf{};
	map<long long, string> strNick{};
	std::mutex ex_user;
	User& id(long long qq) {
		ID = qq;
		return *this;
	}
	User& create(time_t tt) {
		if (tt < tCreated)tCreated = tt;
		return *this;
	}
	User& update(time_t tt) {
		tUpdated = tt;
		return *this;
	}
	User& trust(short n) {
		nTrust = n;
		return *this;
	}
	bool empty()const {
		return (!nTrust) && (!tUpdated) && intConf.empty() && strConf.empty() && strNick.empty();
	}
	string show()const {
		ResList res;
		//res << "昵称记录数:" + to_string(strNick.size());
		for (auto& [key, val] : strConf) {
			res << key + ":" + val;
		}
		for (auto& [key, val] : intConf) {
			res << key + ":" + to_string(val);
		}
		return res.show();
	}
	void setConf(string key, int val) {
		std::lock_guard<std::mutex> lock_queue(ex_user);
		intConf[key] = val;
	}
	void setConf(string key, string val) {
		std::lock_guard<std::mutex> lock_queue(ex_user);
		strConf[key] = val;
	}
	void rmIntConf(string key) {
		std::lock_guard<std::mutex> lock_queue(ex_user);
		intConf.erase(key);
	}
	void rmStrConf(string key) {
		std::lock_guard<std::mutex> lock_queue(ex_user);
		strConf.erase(key);
	}
	bool getNick(string& nick,long long group = 0)const {
		if (auto it = strNick.find(group); it != strNick.end() || (it = strNick.find(0)) != strNick.end()) {
			nick = it->second;
			return true;
		}
		return false;
	}
	void setNick(long long group, string val) {
		std::lock_guard<std::mutex> lock_queue(ex_user);
		strNick[group] = val;
	}
	bool rmNick(long long group) {
		std::lock_guard<std::mutex> lock_queue(ex_user);
		if (auto it = strNick.find(group); it != strNick.end()||(it = strNick.find(0)) != strNick.end()) {
			strNick.erase(it);
			return true;
		}
		return false;
	}
	void writeb(std::ofstream& fout){
		std::lock_guard<std::mutex> lock_queue(ex_user);
		fwrite(fout, ID);
		fwrite(fout, intConf);
		fwrite(fout, strConf);
		fwrite(fout, strNick);
	}
	void readb(std::ifstream& fin) {
		std::lock_guard<std::mutex> lock_queue(ex_user);
		ID = fread<long long>(fin);
		intConf = fread<string, int>(fin);
		strConf = fread<string, string>(fin);
		strNick = fread<long long, string>(fin);
	}
};
ifstream& operator>>(ifstream& fin, User& user);
ofstream& operator<<(ofstream& fout, const User& user);
extern map<long long, User>UserList;
User& getUser(long long qq);
short trustedQQ(long long qq);
int clearUser();

string getName(long long QQ, long long GroupID = 0);

static const map<string,short> mChatConf{//0-群管理员，2-白名单2级，3-白名单3级，4-管理员，5-系统操作
	{"忽略",4},
	{"拦截消息",0},
	{"停用指令",0},
	{"禁用回复",0},
	{"禁用jrrp",0},
	{"禁用draw",0},
	{"禁用me",0},
	{"禁用help",0},
	{"禁用ob",0},
	{"许可使用",1},
	{"未审核",1},
	{"免清",2},
	{"免黑",4},
	{"未进",5},
	{"已退",5}
};
//群聊记录
class Chat {
public:
	bool isGroup = 1;
	long long inviter = 0;
	long long ID = 0;
	string Name = "";
	time_t tCreated = time(NULL);
	time_t tUpdated = 0;
	time_t tLastMsg = 0;
	Chat() {}
	set<string> boolConf{};
	map<string, int> intConf{};
	map<string, string> strConf{};
	Chat& id(long long grp) {
		ID = grp;
		return *this;
	}
	Chat& group() {
		isGroup = true;
		return *this;
	}
	Chat& discuss() {
		isGroup = false;
		return *this;
	}
	Chat& name(string s) {
		Name = s;
		return *this;
	}
	Chat& create(time_t tt) {
		if (tt < tCreated)tCreated = tt;
		return *this;
	}
	Chat& update(time_t tt) {
		tUpdated = tt;
		return *this;
	}
	Chat& lastmsg(time_t tt) {
		tLastMsg = tt;
		return *this;
	}
	Chat& set(string item) {
		if(mChatConf.count(item))boolConf.insert(item);
		return *this;
	}
	Chat& reset(string item) {
		boolConf.erase(item);
		return *this;
	}
	void leave(string msg = "") {
		if (!msg.empty()) {
			if (isGroup)CQ::sendGroupMsg(ID, msg);
			else CQ::sendDiscussMsg(ID, msg);
			Sleep(500);
		}
		set("已退");
		if (isGroup)CQ::setGroupLeave(ID);
		else CQ::setDiscussLeave(ID);
	}
	bool isset(string key)const {
		return boolConf.count(key) || intConf.count(key) || strConf.count(key);
	}
	void setConf(string key, int val) {
		intConf[key] = val;
	}
	void rmConf(string key) {
		intConf.erase(key);
	}
	void setText(string key, string val) {
		strConf[key] = val;
	}
	void rmText(string key) {
		strConf.erase(key);
	}
	void writeb(std::ofstream& fout) {
		fwrite(fout, ID);
		if (!Name.empty()) {
			fwrite(fout, (short)0);
			fwrite(fout, Name);
		}
		if (!boolConf.empty()) {
			fwrite(fout, (short)1);
			fwrite(fout, boolConf);
		}
		if (!intConf.empty()) {
			fwrite(fout, (short)2);
			fwrite(fout, intConf);
		}
		if (!strConf.empty()) {
			fwrite(fout, (short)3);
			fwrite(fout, strConf);
		}
		fwrite(fout, (short)-1);
	}
	void readb(std::ifstream& fin) {
		ID = fread<long long>(fin);
		short tag = fread<short>(fin);
		while (tag != -1) {
			switch (tag) {
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
extern map<long long, Chat>ChatList;
Chat& chat(long long id);
int groupset(long long id, string st);
string printChat(Chat& grp);
ifstream& operator>>(ifstream& fin, Chat& grp);
ofstream& operator<<(ofstream& fout, const Chat& grp);

//被引用的图片列表
static set<string> sReferencedImage;
static void scanImage(string s, set<string>& list) {
	int l = 0, r = 0;
	while ((l = s.find('[', r)) != string::npos && (r = s.find(']', l)) != string::npos) {
		if (s.substr(l, 15) != CQ_IMAGE)continue;
		string strFile = s.substr(l + 15, r - l - 15);
		if (strFile.length() > 35)strFile += ".cqimg";
		list.insert(strFile);
	}
}
static void scanImage(const vector<string>& v, set<string>& list) {
	for (auto it : v) {
		scanImage(it, sReferencedImage);
	}
}
template<typename TVal, typename sort>
static void scanImage(const map<string, TVal, sort>& m, set<string>& list) {
	for (auto it : m) {
		scanImage(it.first, sReferencedImage);
		scanImage(it.second, sReferencedImage);
	}
}
template<typename TVal>
static void scanImage(const map<string, TVal>& m, set<string>& list) {
	for (auto it : m) {
		scanImage(it.first, sReferencedImage);
		scanImage(it.second, sReferencedImage);
	}
}
template<typename TKey, typename TVal>
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
	for (auto it : ChatList) {
		scanImage(it.second.strConf, sReferencedImage);
	}
	string strLog = "整理" + GlobalMsg["strSelfName"] + "被引用图片" + to_string(sReferencedImage.size()) + "项";
	console.log(strLog, 0b0, printSTNow());
	return clrDir("data\\image\\", sReferencedImage);
}

DWORD getRamPort();

/*static DWORD getRamPort() {
	typedef BOOL(WINAPI * func)(LPMEMORYSTATUSEX);
	MEMORYSTATUSEX stMem = { 0 };
	func GlobalMemoryStatusEx = (func)GetProcAddress(LoadLibrary(L"Kernel32.dll"), "GlobalMemoryStatusEx");
	stMem.dwLength = sizeof(stMem);
	GlobalMemoryStatusEx(&stMem);
	return stMem.dwMemoryLoad;
}*/

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
	//WaitForSingleObject(hEvent, 1000);
	Sleep(2000);
	res = GetSystemTimes(&idleTime, &kernelTime, &userTime);

	__int64 idle = compareFileTime(idleTime, preidleTime);
	__int64 kernel = compareFileTime(kernelTime, prekernelTime);
	__int64 user = compareFileTime(userTime, preuserTime);

	__int64 cpu = (kernel + user - idle) * 100 / (kernel + user);
	return cpu;
}

static int getProcessCpu()
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

	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftPreKernelTime, &ftPreUserTime)) { return -1; }
	log << ftPreKernelTime.dwLowDateTime << "\n" << ftPreUserTime.dwLowDateTime << "\n";
	HANDLE hEvent = CreateEventA(NULL, FALSE, FALSE, NULL); // 初始值为 nonsignaled ，并且每次触发后自动设置为nonsignaled
	WaitForSingleObject(hEvent, 1000);
	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime)) { return -1; }
	log << ftKernelTime.dwLowDateTime << "\n" << ftUserTime.dwLowDateTime << "\n";
	__int64 ullKernelTime = compareFileTime(ftKernelTime, ftPreKernelTime);
	__int64 ullUserTime = compareFileTime(ftUserTime, ftPreUserTime);
	log << ullKernelTime << "\n" << ullUserTime << "\n" << iCpuNum;
	__int64 dCpu = (ullKernelTime + ullUserTime) / (iCpuNum * 100);
	return (int)dCpu;
}