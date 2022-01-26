/*
 * 后台系统
 * Copyright (C) 2019-2020 String.Empty
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <string_view>
#include "filesystem.hpp"
#include "ManagerSystem.h"

#include "CardDeck.h"
#include "CharacterCard.h"
#include "GlobalVar.h"
#include "DDAPI.h"
#include "CQTools.h"
#include "DiceSession.h"

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

User& getUser(long long uid)
{
	if (TinyList.count(uid))uid = TinyList[uid];
	if (!UserList.count(uid))UserList[uid].id(uid);
	return UserList[uid];
}

[[nodiscard]] bool User::empty() const {
	return (!nTrust) && (!tUpdated) && confs.empty() && strNick.empty();
}
[[nodiscard]] string User::show() const {
	ResList res;
	for (auto& [key, val] : confs)
	{
		res << key + ":" + val.to_str();
	}
	return res.show();
}
void User::setConf(const string& key, const AttrVar& val)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	if (val)confs[key] = val;
	else confs.erase(key);
}
void User::rmConf(const string& key)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	confs.erase(key);
}

bool User::getNick(string& nick, long long group) const
{
	if (auto it = strNick.find(group); it != strNick.end() 
		|| (it = strNick.find(0)) != strNick.end())
	{
		nick = it->second;
		return true;
	}
	else if (strNick.size() == 1) {
		nick = strNick.begin()->second;
	}
	return false;
}
void User::writeb(std::ofstream& fout)
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	confs["trust"] = (int)nTrust;
	confs["tCreated"] = (long long)tCreated;
	confs["tUpdated"] = (long long)tUpdated;
	fwrite(fout, string("ID"));
	fwrite(fout, ID);
	if (!confs.empty()) {
		fwrite(fout, string("Conf"));
		fwrite(fout, confs);
	}
	if (!strNick.empty()) {
		fwrite(fout, string("Nick"));
		fwrite(fout, strNick);
	}
	fwrite(fout, string("END"));
}
void User::old_readb(std::ifstream& fin)
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	ID = fread<long long>(fin);
	map<string, int> intConf{ fread<string, int>(fin) };
	for (auto& [key, val] : intConf) {
		confs[key] = val;
	}
	map<string, string> strConf{ fread<string, string>(fin) };
	for (auto& [key, val] : strConf) {
		confs[key] = val;
	}
	strNick = fread<long long, string>(fin);
}
void User::readb(std::ifstream& fin)
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	string tag;
	while ((tag = fread<string>(fin)) != "END") {
		if (tag == "ID")ID = fread<long long>(fin);
		else if (tag == "Conf")fread(fin, confs);
		else if (tag == "Nick")strNick = fread<long long, string>(fin);
	}
	if (confs.count("trust"))nTrust = confs["trust"].to_int();
	if (confs.count("tCreated"))tCreated = confs["tCreated"].to_ll();
	if (confs.count("tUpdated"))tUpdated = confs["tUpdated"].to_ll();
	if (confs.count("tinyID"))TinyList.emplace(confs["tinyID"].to_ll(), ID);
}
int trustedQQ(long long uid)
{
	if (TinyList.count(uid))uid = TinyList[uid];
	if (uid == console.master())return 256;
	if (uid == console.DiceMaid)return 255;
	else if (!UserList.count(uid))return 0;
	else return UserList[uid].nTrust;
}


int clearUser() {
	vector<long long> UserDelete;
	time_t tNow{ time(nullptr) };
	time_t userline{ tNow - console["InactiveUserLine"] * (time_t)86400 };
	bool isClearInactive{ console["InactiveUserLine"] > 0 };
	for (const auto& [uid, user] : UserList) {
		if (user.nTrust > 0)continue;
		if (user.empty()) {
			UserDelete.push_back(uid);
		}
		else if (isClearInactive) {
			time_t tLast{ user.tUpdated };
			if (!tLast)tLast = user.tCreated;
			if (gm->has_session(~uid) && gm->session(~uid).tUpdate > tLast)tLast = gm->session(~uid).tUpdate;
			if (tLast >= userline)continue;
			UserDelete.push_back(uid);
			if (PList.count(uid)) {
				PList.erase(uid);
			}
		}
	}
	for (const auto& uid : UserDelete) {
		UserList.erase(uid);
		if (gm->has_session(~uid))gm->session_end(~uid);
	}
	return UserDelete.size();
}
int clearGroup() {
	if (console["InactiveGroupLine"] <= 0)return 0;
	vector<long long> GrpDelete;
	time_t tNow{ time(nullptr) };
	time_t grpline{ tNow - console["InactiveGroupLine"] * (time_t)86400 };
	for (const auto& [id, grp] : ChatList) {
		if (grp.is_except() || grp.isset("免清") || grp.isset("忽略"))continue;
		time_t tLast{ grp.tUpdated };
		if (gm->has_session(id) && gm->session(id).tUpdate > grp.tUpdated)tLast = gm->session(id).tUpdate;
		if (tLast < grpline)GrpDelete.push_back(id);
	}
	for (const auto& id : GrpDelete) {
		ChatList.erase(id);
	}
	return GrpDelete.size();

}

string getName(long long uid, long long GroupID)
{
	// Self
	if (uid == console.DiceMaid) return getMsg("strSelfName");

	// nn
	string nick;
	if (UserList.count(uid) && getUser(uid).getNick(nick, GroupID)) return nick;

	// GroupCard
	if (GroupID)
	{
		nick = DD::getGroupNick(GroupID, uid);
		nick = strip(msg_decode(nick));
		if (!nick.empty()) return nick;
	}

	// QQNick
	nick = DD::getQQNick(uid);
	nick = strip(msg_decode(nick));
	if (!nick.empty()) return nick;

	// Unknown
	return (UserList.count(uid) ? getMsg("strCallUser") : getMsg("stranger")) + "(" + to_string(uid) + ")";
}
AttrVar idx_nick(AttrObject& eve) {
	if (eve.has("nick"))return eve["nick"];
	if (!eve.has("uid"))return {};
	long long uid{ eve["uid"].to_ll() };
	long long gid{ eve.has("gid") ? eve["gid"].to_ll() : 0 };
	return eve["nick"] = getName(uid, gid);
}

void filter_CQcode(string& nick, long long fromGID)
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
				nick.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGID));
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
	return *this;
}

void Chat::leave(const string& msg) {
	if (!msg.empty()) {
		if (isGroup)DD::sendGroupMsg(ID, msg);
		else DD::sendDiscussMsg(ID, msg);
		std::this_thread::sleep_for(500ms);
	}
	isGroup ? DD::setGroupLeave(ID) : DD::setDiscussLeave(ID);
	set("已退").reset("已入群");
}
bool Chat::is_except()const {
	return confs.count("免黑") || confs.count("协议无效");
}
void Chat::writeb(std::ofstream& fout)
{
	confs["Name"] = Name;
	confs["inviter"] = (long long)inviter;
	confs["tCreated"] = (long long)tCreated;
	confs["tUpdated"] = (long long)tUpdated;
	fwrite(fout, ID);
	if (!Name.empty())
	{
		fwrite(fout, static_cast<short>(0));
		fwrite(fout, Name);
	}
	if (!confs.empty())
	{
		fwrite(fout, static_cast<short>(10));
		fwrite(fout, confs);
	}
	if (!ChConf.empty())
	{
		fwrite(fout, static_cast<short>(20));
		fwrite(fout, ChConf);
	}
	fwrite(fout, static_cast<short>(-1));
}
void Chat::readb(std::ifstream& fin)
{
	ID = fread<long long>(fin);
	short tag = fread<short>(fin);
	while (tag != -1)
	{
		switch (tag)
		{
		case 0:
			Name = fread<string>(fin);
			break;
		case 1:
			for (auto& key : fread<string, true>(fin)) {
				confs[key] = true;
			}
			break;
		case 2:
			for (auto& [key, val] : fread<string, int>(fin)) {
				confs[key] = val;
			}
			break;
		case 3:
			for (auto& [key, val] : fread<string, string>(fin)) {
				confs[key] = val;
			}
			break;
		case 10:
			fread(fin, confs);
			break;
		case 20:
			fread(fin, ChConf);
			break;
		default:
			return;
		}
		tag = fread<short>(fin);
	}
	if (confs.count("Name"))Name = confs["Name"].to_str();
	if (confs.count("inviter"))inviter = confs["inviter"].to_ll();
	if (confs.count("tCreated"))tCreated = confs["tCreated"].to_ll();
	if (confs.count("tUpdated"))tUpdated = confs["tUpdated"].to_ll();
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