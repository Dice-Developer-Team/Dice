/*
 * 后台系统
 * Copyright (C) 2019-2020 String.Empty
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <string_view>
#include <filesystem>
#include "ManagerSystem.h"

#include "CardDeck.h"
#include "GlobalVar.h"
#include "DDAPI.h"
#include "CQTools.h"

std::filesystem::path dirExe;
std::filesystem::path DiceDir("DiceData");
 //被引用的图片列表
unordered_set<string> sReferencedImage;

const map<string, short> mChatConf{
	//0-群管理员，2-信任2级，3-信任3级，4-管理员，5-系统操作
	{"忽略", 4},
	{"拦截消息", 0},
	{"停用指令", 0},
	{"禁用回复", 0},
	{"禁用jrrp", 0},
	{"禁用draw", 0},
	{"禁用me", 0},
	{"禁用help", 0},
	{"禁用ob", 0},
	{"许可使用", 1},
	{"未审核", 1},
	{"免清", 2},
	{"免黑", 4},
	{"协议无效", 3},
	{"未进", 5},
	{"已退", 5}
};

User& getUser(long long qq)
{
	if (!UserList.count(qq))UserList[qq].id(qq);
	return UserList[qq];
}
short trustedQQ(long long qq) 
{
	if (qq == console.master())return 256;
	if (qq == console.DiceMaid)return 255;
	else if (!UserList.count(qq))return 0;
	else return UserList[qq].nTrust;
}

int clearUser()
{
	vector<long long> QQDelete;
	for (const auto& [qq, user] : UserList)
	{
		if (user.empty())
		{
			QQDelete.push_back(qq);
		}
	}
	for (const auto& qq : QQDelete)
	{
		UserList.erase(qq);
	}
	return QQDelete.size();
}

string getName(long long QQ, long long GroupID)
{
	if (QQ == console.DiceMaid)return getMsg("strSelfCall");
	string nick;
	if (UserList.count(QQ) && getUser(QQ).getNick(nick, GroupID))return nick;
	if (GroupID && !(nick = DD::getGroupNick(GroupID, QQ)).empty()
		&& !(nick = strip(msg_decode(nick))).empty())return nick;
	if (nick = DD::getQQNick(QQ); !(nick = strip(msg_decode(nick))).empty())return nick;
	return GlobalMsg["stranger"] + "(" + to_string(QQ) + ")";
}
void filter_CQcode(string& nick, long long fromGroup)
{
	size_t posL(0);
	while ((posL = nick.find(CQ_AT)) != string::npos)
	{
		//检查at格式
		if (size_t posR = nick.find(']',posL); posR != string::npos) 
		{
			std::string_view stvQQ(nick);
			stvQQ = stvQQ.substr(posL + 10, posR - posL - 10);
			//检查QQ号格式
			bool isDig = true;
			for (auto ch: stvQQ) 
			{
				if (!isdigit(static_cast<unsigned char>(ch)))
				{
					isDig = false;
					break;
				}
			}
			//转义
			if (isDig && posR - posL < 29) 
			{
				nick.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGroup));
			}
			else if (stvQQ == "all") 
			{
				nick.replace(posL, posR - posL + 1, "@全体成员");
			}
			else
			{
				nick.replace(posL, posR - posL + 1, "@");
			}
		}
		else return;
	}
	while ((posL = nick.find(CQ_IMAGE)) != string::npos) {
		//检查at格式
		if (size_t posR = nick.find(']', posL); posR != string::npos) {
			nick.replace(posL, posR - posL + 1, "[图片]");
		}
		else return;
	}
	while ((posL = nick.find("[CQ:")) != string::npos)
	{
		if (size_t posR = nick.find(']', posL); posR != string::npos) 
		{
			nick.erase(posL, posR - posL + 1);
		}
		else return;
	}
}

Chat& chat(long long id)
{
	if (!ChatList.count(id))ChatList[id].id(id);
	return ChatList[id];
}
Chat& Chat::id(long long grp) {
	ID = grp;
	Name = DD::getGroupName(grp);
	if (!Enabled)return *this;
	if (DD::getGroupIDList().count(grp)) {
		isGroup = true;
	}
	else {
		boolConf.insert("未进");
	}
	return *this;
}

void Chat::leave(const string& msg) {
	if (!msg.empty()) {
		if (isGroup)DD::sendGroupMsg(ID, msg);
		else DD::sendDiscussMsg(ID, msg);
		std::this_thread::sleep_for(500ms);
	}
	isGroup ? DD::setGroupLeave(ID) : DD::setDiscussLeave(ID);
	set("已退");
}
bool Chat::is_except()const {
	return boolConf.count("免黑") || boolConf.count("协议无效");
}

int groupset(long long id, const string& st)
{
	if (!ChatList.count(id))return -1;
	return ChatList[id].isset(st);
}

string printChat(Chat& grp)
{
	string name{ DD::getGroupName(grp.ID) };
	if (!name.empty())return "[" + name + "](" + to_string(grp.ID) + ")";
	if (!grp.Name.empty())return "[" + grp.Name + "](" + to_string(grp.ID) + ")";
	if (grp.isGroup) return "群(" + to_string(grp.ID) + ")";
	return "讨论组(" + to_string(grp.ID) + ")";
}

void scanImage(const string& s, unordered_set<string>& list) {
	int l = 0, r = 0;
	while ((l = s.find('[', r)) != string::npos && (r = s.find(']', l)) != string::npos)
	{
		if (s.substr(l, 15) != CQ_IMAGE)continue;
		string strFile = s.substr(l + 15, r - l - 15);
		if (strFile.length() > 35)strFile += ".cqimg";
		list.insert(strFile);
	}
}

void scanImage(const vector<string>& v, unordered_set<string>& list) {
	for (const auto& it : v)
	{
		scanImage(it, sReferencedImage);
	}
}

#ifdef _WIN32

DWORD getRamPort()
{
	MEMORYSTATUSEX memory_status;
	memory_status.dwLength = sizeof(memory_status);
	GlobalMemoryStatusEx(&memory_status);
	return memory_status.dwMemoryLoad;
}

__int64 compareFileTime(const FILETIME& ft1, const FILETIME& ft2)
{
	__int64 t1 = ft1.dwHighDateTime;
	t1 = t1 << 32 | ft1.dwLowDateTime;
	__int64 t2 = ft2.dwHighDateTime;
	t2 = t2 << 32 | ft2.dwLowDateTime;
	return t1 - t2;
}

long long getWinCpuUsage() 
{
	FILETIME preidleTime;
	FILETIME prekernelTime;
	FILETIME preuserTime;
	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return -1;
	preidleTime = idleTime;
	prekernelTime = kernelTime;
	preuserTime = userTime;	

	Sleep(1000);
	if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return -1;

	const __int64 idle = compareFileTime(idleTime, preidleTime);
	const __int64 kernel = compareFileTime(kernelTime, prekernelTime);
	const __int64 user = compareFileTime(userTime, preuserTime);

	return (kernel + user - idle) * 1000 / (kernel + user);
}

long long getProcessCpu()
{
	const HANDLE hProcess = GetCurrentProcess();
	//if (INVALID_HANDLE_VALUE == hProcess){return -1;}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	const __int64 iCpuNum = si.dwNumberOfProcessors;

	FILETIME ftPreKernelTime, ftPreUserTime;
	FILETIME ftKernelTime, ftUserTime;
	FILETIME ftCreationTime, ftExitTime;
	std::ofstream log("System.log");

	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftPreKernelTime, &ftPreUserTime)) { return -1; }
	log << ftPreKernelTime.dwLowDateTime << "\n" << ftPreUserTime.dwLowDateTime << "\n";
	Sleep(1000);
	if (!GetProcessTimes(hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime)) { return -1; }
	log << ftKernelTime.dwLowDateTime << "\n" << ftUserTime.dwLowDateTime << "\n";
	const __int64 ullKernelTime = compareFileTime(ftKernelTime, ftPreKernelTime);
	const __int64 ullUserTime = compareFileTime(ftUserTime, ftPreUserTime);
	log << ullKernelTime << "\n" << ullUserTime << "\n" << iCpuNum;
	return (ullKernelTime + ullUserTime) / (iCpuNum * 10);
}

//获取空闲硬盘(千分比)
long long getDiskUsage(double& mbFreeBytes, double& mbTotalBytes){
	/*int sizStr = GetLogicalDriveStrings(0, NULL);//获得本地所有盘符存在Drive数组中
	char* chsDrive = new char[sizStr];//初始化数组用以存储盘符信息
	GetLogicalDriveStrings(sizStr, chsDrive);
	int DType;
	int si = 0;*/
	BOOL fResult;
	unsigned _int64 i64FreeBytesToCaller;
	unsigned _int64 i64TotalBytes;
	unsigned _int64 i64FreeBytes;
	fResult = GetDiskFreeSpaceEx(
		NULL,
		(PULARGE_INTEGER)&i64FreeBytesToCaller,
		(PULARGE_INTEGER)&i64TotalBytes,
		(PULARGE_INTEGER)&i64FreeBytes);
	//GetDiskFreeSpaceEx函数，可以获取驱动器磁盘的空间状态,函数返回的是个BOOL类型数据
	if (fResult) {
		mbTotalBytes = i64TotalBytes * 1000 / 1024 / 1024 / 1024 / 1000.0;//磁盘总容量
		mbFreeBytes = i64FreeBytesToCaller * 1000 / 1024 / 1024 / 1024 / 1000.0;//磁盘剩余空间
		return 1000 - 1000 * i64FreeBytesToCaller / i64TotalBytes;
	}
	return 0;
}

#endif