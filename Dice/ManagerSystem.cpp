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
#include "DiceSelfData.h"
#include "DiceCensor.h"
#include "DiceMod.h"

std::filesystem::path dirExe;
std::filesystem::path DiceDir("DiceData");

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
};

User& getUser(long long uid){
	if (TinyList.count(uid))uid = TinyList[uid];
	if (!UserList.count(uid))UserList[uid].id(uid);
	return UserList[uid];
}
constexpr const char* chDigit{ "0123456789" };
AttrVar getUserItem(long long uid, const string& item) {
	if (!uid)return {};
	if (TinyList.count(uid))uid = TinyList[uid];
	if (item == "name") {
		if (string name{ DD::getQQNick(uid) }; !name.empty())
			return name;
	}
	else if (item == "nick") {
		return getName(uid);
	}
	else if (item == "trust") {
		return trustedQQ(uid);
	}
	else if (item == "isDiceMaid") {
		return DD::isDiceMaid(uid);
	}
	else if (item.find("nick#") == 0) {
		long long gid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		return getName(uid, gid);
	}
	else if (UserList.count(uid)) {
		User& user{ getUser(uid) };
		if (item == "firstCreate") return (long long)user.tCreated;
		else  if (item == "lastUpdate") return (long long)user.updated();
		else if (item == "nn") {
			if (string nick; user.getNick(nick))return nick;
		}
		else if (item.find("nn#") == 0) {
			string nick;
			long long gid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			if (user.getNick(nick, gid))return nick;
		}
		else if (user.isset(item)) {
			return user.confs.get(item);
		}
	}
	if (item == "gender") {
		string ret;
		if (string data{ fifo_json{
			{ "action","getGender" },
			{ "sid",console.DiceMaid },
			{ "uid",uid },
		}.dump() }; DD::getExtra(data, ret)) {
			return fifo_json::parse(ret, nullptr, false);
		}
	}
	return {};
}
AttrVar getGroupItem(long long id, const string& item) {
	if (!id)return {};
	if (item == "size") {
		return (int)DD::getGroupSize(id).currSize;
	}
	else if (item == "maxsize") {
		return (int)DD::getGroupSize(id).maxSize;
	}
	else if (item.find("card#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		if (string card{ DD::getGroupNick(id, uid) }; !card.empty())return card;
	}
	else if (item.find("auth#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		if (int auth{ DD::getGroupAuth(id, uid, 0) }; auth)return auth;
	}
	else if (item.find("lst#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		return DD::getGroupLastMsg(id, uid);
	}
	else if (ChatList.count(id)) {
		Chat& grp{ chat(id) };
		if (item == "name") {
			if (string name{ DD::getGroupName(id) };!name.empty())return grp.Name = name;
		}
		else if (item == "firstCreate") {
			return (long long)grp.tCreated;
		}
		else if (item == "lastUpdate") {
			return (long long)grp.updated();
		}
		else if (grp.confs.has(item)) {
			return grp.confs.get(item);
		}
	}
	else if (item == "name") {
		if (string name{ DD::getGroupName(id) }; !name.empty())return name;
	}
	return {};
}
AttrVar getSelfItem(string item) {
	AttrVar var;
	if (item.empty())return var;
	if (item[0] == '&')item = fmt->format(item);
	if (console.intDefault.count(item))return console[item];
	if (!(var = getUserItem(console.DiceMaid, item))) {
		string file, sub;
		while (!(sub = splitOnce(item)).empty()) {
			file += sub;
			if (selfdata_byStem.count(file)
				&& (var = selfdata_byStem[file]->data.to_obj().index(item))) {
				return var;
			}
			file += ".";
		}
	}
	return var;
}
AttrVar getContextItem(AttrObject context, string item, bool isTrust) {
	if (item.empty())return context;
	AttrVar var;
	if (item[0] == '&')item = fmt->format(item, context);
	string sub;
	if (!context.empty()) {
		if (context.has(item))return context.get(item);
		if (MsgIndexs.count(item))return MsgIndexs[item](context);
		if (item.find(':') <= item.find('.'))return var;
		if (!(sub = splitOnce(item)).empty()) {
			if (context.has(sub) && context.is_table(sub))
				return getContextItem(context.get_obj(sub), item);
			if (sub == "user")return getUserItem(context.get_ll("uid"), item);
			if (isTrust && (sub == "grp" || sub == "group"))return getGroupItem(context.get_ll("gid"), item);
		}
	}
	if (isTrust) {
		if (sub.empty())sub = splitOnce(item);
		if (sub == "self") {
			return item.empty() ? AttrVar(getMsg("strSelfCall")) : getSelfItem(item);
		}
		else if (selfdata_byStem.count(sub)) {
			var = selfdata_byStem[sub]->data.to_obj().index(item);
		}
	}
	return var;
}

[[nodiscard]] bool User::empty() const {
	return (!nTrust) && (!updated()) && confs.empty() && strNick.empty();
}
void User::setConf(const string& key, const AttrVar& val)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	if (val)confs.set(key, val);
	else confs.reset(key);
}
void User::rmConf(const string& key)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	confs.reset(key);
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
	confs.set("trust", (int)nTrust);
	confs.set("tCreated", (long long)tCreated);
	fwrite(fout, string("ID"));
	fwrite(fout, ID);
	if (!confs.empty()) {
		fwrite(fout, string("Conf"));
		confs.writeb(fout);
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
	map<string, int> intConf;
	fread(fin, intConf);
	for (auto& [key, val] : intConf) {
		confs.set(key, val);
	}
	map<string, string> strConf;
	fread(fin, strConf);
	for (auto& [key, val] : strConf) {
		confs.set(key, val);
	}
	fread(fin, strNick);
}
void User::readb(std::ifstream& fin)
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	string tag;
	while ((tag = fread<string>(fin)) != "END") {
		if (tag == "ID")ID = fread<long long>(fin);
		else if (tag == "Conf")confs.readb(fin);
		else if (tag == "Nick")fread(fin, strNick);
	}
	nTrust = confs.get_int("trust");
	if (confs.has("tCreated"))tCreated = confs.get_ll("tCreated");
	if (confs.has("tinyID"))TinyList.emplace(confs.get_ll("tinyID"), ID);
}
int trustedQQ(long long uid){
	if (TinyList.count(uid))uid = TinyList[uid];
	if (uid == console)return 256;
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
			time_t tLast{ user.updated() };
			if (!tLast)tLast = user.tCreated;
			if (auto s{ sessions.get_if({ uid }) })
				tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
			if (tLast >= userline)continue;
			UserDelete.push_back(uid);
			if (PList.count(uid)) {
				PList.erase(uid);
			}
		}
	}
	for (auto uid : UserDelete) {
		UserList.erase(uid);
		if (sessions.has_session(uid))sessions.end(uid);
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
		time_t tLast{ grp.updated() };
		if (auto s{ sessions.get_if({ 0,id }) })
			tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
		if (tLast && tLast < grpline)GrpDelete.push_back(id);
	}
	for (auto id : GrpDelete) {
		ChatList.erase(id);
		if (sessions.has_session({ 0,id }))sessions.end({ 0,id });
	}
	return GrpDelete.size();
}

unordered_map<long long, unordered_map<long long, std::pair<time_t, string>>>skipCards;
string getName(long long uid, long long GroupID){
	if (!uid)return {};
	// Self
	if (uid == console.DiceMaid) return getMsg("strSelfName");

	// nn
	string nick;
	if (UserList.count(uid) && getUser(uid).getNick(nick, GroupID)) return nick;

	// GroupCard
	if (GroupID){
		if (auto& card{ skipCards[GroupID][uid] };
				time(nullptr) < card.first){
			nick = card.second;
		}
		else {
			vector<string>kw;
			nick = strip(msg_decode(nick = DD::getGroupNick(GroupID, uid)));
			if (censor.search(nick, kw) > trustedQQ(uid)) {
				nick.clear();
			}
			card = { time(nullptr) + 600 ,nick };
		}
		if (!nick.empty()) return nick;
	}

	// QQNick
	if (auto& card{ skipCards[0][uid] };
		time(nullptr) < card.first) {
		nick = card.second;
	}
	else {
		vector<string>kw;
		nick = strip(msg_decode(nick = DD::getQQNick(uid)));
		if (censor.search(nick, kw) > trustedQQ(uid)) {
			nick = getMsg("stranger") + "(" + to_string(uid) + ")";
		}
		card = { time(nullptr) + 600 ,nick };
	}
	if (!nick.empty()) return nick;

	// Unknown
	return (UserList.count(uid) ? getMsg("strCallUser") : getMsg("stranger")) + "(" + to_string(uid) + ")";
}
AttrVar idx_nick(AttrObject& eve) {
	if (eve.has("nick"))return eve["nick"];
	if (!eve.has("uid"))return {};
	long long uid{ eve.get_ll("uid") };
	long long gid{ eve.get_ll("gid") };
	return eve["nick"] = getName(uid, gid);
}

string filter_CQcode(const string& raw, long long fromGID){
	string msg{ raw };
	size_t posL(0), posR(0);
	while ((posL = msg.find(CQ_AT)) != string::npos)
	{
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos)
		{
			std::string_view stvQQ(msg);
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
				msg.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGID));
			}
			else if (stvQQ == "all") 
			{
				msg.replace(posL, posR - posL + 1, "@全体成员");
			}
			else
			{
				msg.replace(posL + 1, 9, "@");
			}
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_IMAGE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "图片");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_FACE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "表情");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_POKE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, 11, "戳一戳");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_FILE)) != string::npos) {
		//检查at格式
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "文件");
		}
		else return msg;
	}
	while ((posL = msg.find("[CQ:")) != string::npos)
	{
		if (size_t posR = msg.find(']', posL); posR != string::npos) 
		{
			msg.erase(posL, posR - posL + 1);
		}
		else return msg;
	}
	return msg;
}
string forward_filter(const string& raw, long long fromGID) {
	string msg{ raw };
	size_t posL(0);
	while ((posL = msg.find(CQ_AT)) != string::npos)
	{
		//检查at格式
		if (size_t posR = msg.find(']', posL); posR != string::npos)
		{
			std::string_view stvQQ(msg);
			stvQQ = stvQQ.substr(posL + 10, posR - posL - 10);
			//检查QQ号格式
			bool isDig = true;
			for (auto ch : stvQQ)
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
				msg.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGID));
			}
			else if (stvQQ == "all")
			{
				msg.replace(posL, posR - posL + 1, "@全体成员");
			}
			else
			{
				msg.replace(posL + 1, 9, "@");
			}
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_POKE)) != string::npos) {
		//检查at格式
		if (size_t posR = msg.find(']', posL); posR != string::npos) {
			msg.replace(posL + 1, 11, "戳一戳");
		}
		else return msg;
	}
	return msg;
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
	reset("已入群").rmLst();
}
bool Chat::is_except()const {
	return confs.has("免黑") || confs.has("协议无效");
}
void Chat::writeb(std::ofstream& fout)
{
	confs["Name"] = Name;
	confs["inviter"] = (long long)inviter;
	confs["tCreated"] = (long long)tCreated;
	fwrite(fout, ID);
	if (!Name.empty())
	{
		fwrite(fout, static_cast<short>(0));
		fwrite(fout, Name);
	}
	if (!confs.empty())
	{
		fwrite(fout, static_cast<short>(10));
		confs.writeb(fout);
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
	AttrVars conf;
	while (tag != -1)
	{
		switch (tag)
		{
		case 0:
			Name = fread<string>(fin);
			break;
		case 1:
			for (auto& key : fread<string, true>(fin)) {
				confs.set(key, true);
			}
			break;
		case 2:
			for (auto& [key, val] : fread<string, int>(fin)) {
				confs.set(key, val);
			}
			break;
		case 3:
			for (auto& [key, val] : fread<string, string>(fin)) {
				confs.set(key, val);
			}
			break;
		case 10:
			confs.readb(fin);
			break;
		case 20:
			fread(fin, ChConf);
			break;
		default:
			return;
		}
		tag = fread<short>(fin);
	}
	if (confs.has("Name"))Name = confs.get_str("Name");
	inviter = confs.get_ll("inviter");
	if (confs.has("tCreated"))tCreated = confs.get_ll("tCreated");
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