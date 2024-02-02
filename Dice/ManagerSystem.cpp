 /*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123���
 * Copyright (C) 2019-2024 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <string_view>
#include "filesystem.hpp"
#include "ManagerSystem.h"

#include "CharacterCard.h"
#include "GlobalVar.h"
#include "DDAPI.h"
#include "CQTools.h"
#include "DiceSession.h"
#include "DiceSelfData.h"
#include "DiceCensor.h"
#include "DiceMod.h"
#include "BlackListManager.h"

std::filesystem::path dirExe;
std::filesystem::path DiceDir("DiceData");

const map<string, short> mChatConf{
	//0-Ⱥ����Ա��2-����2����3-����3����4-����Ա��5-ϵͳ����
	{"����", 4},
	{"ͣ��ָ��", 0},
	{"���ûظ�", 0},
	{"����jrrp", 0},
	{"����draw", 0},
	{"����me", 0},
	{"����help", 0},
	{"����ob", 0},
	{"���ʹ��", 1},
	{"δ���", 1},
	{"����", 2},
	{"���", 4},
	{"Э����Ч", 3},
};

User& getUser(long long uid){
	if (TinyList.count(uid))uid = TinyList[uid];
	if (!UserList.count(uid))UserList.emplace(uid,std::make_shared<User>(uid));
	return *UserList[uid];
}
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
	else if (item == "danger") {
		return blacklist->get_user_danger(uid);
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
		else if (user.is(item)) {
			return user.get(item);
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
		else if (grp.has(item)) {
			return grp.get(item);
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
	if ((var = getUserItem(console.DiceMaid, item)).is_null()) {
		string file, sub;
		while (!(sub = splitOnce(item)).empty()) {
			file += sub;
			if (selfdata_byStem.count(file)
				&& !(var = selfdata_byStem[file]->data.to_obj()->index(item)).is_null()) {
				return var;
			}
			file += ".";
		}
	}
	return var;
}
AttrVar getContextItem(const AttrObject& context, string item, bool isTrust) {
	if (!context)return {};
	if (item.empty())return context;
	AttrVar var;
	if (item[0] == '&')item = fmt->format(item, context);
	string sub;
	if (!context->empty()) {
		if (context->has(item))return context->get(item);
		if (MsgIndexs.count(item))return MsgIndexs[item](context);
		if (item.find(':') <= item.find('.'))return var;
		if (!(sub = splitOnce(item)).empty()) {
			if (context->has(sub) && context->is_table(sub))
				return getContextItem(context->get_obj(sub), item);
			if (sub == "user")return getUserItem(context->get_ll("uid"), item);
			if (isTrust && (sub == "grp" || sub == "group"))return getGroupItem(context->get_ll("gid"), item);
		}
	}
	if (isTrust) {
		if (sub.empty())sub = splitOnce(item);
		if (sub == "self") {
			return item.empty() ? AttrVar(getMsg("strSelfCall")) : getSelfItem(item);
		}
		else if (selfdata_byStem.count(sub)) {
			var = selfdata_byStem[sub]->data.to_obj()->index(item);
		}
	}
	return var;
}

[[nodiscard]] bool User::empty() const {
	return (!nTrust) && (!updated()) && dict.empty() && strNick.empty();
}
void User::setConf(const string& key, const AttrVar& val)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	if (val)set(key, val);
	else reset(key);
}
void User::rmConf(const string& key)
{
	if (key.empty())return;
	std::lock_guard<std::mutex> lock_queue(ex_user);
	reset(key);
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
	set("trust", (int)nTrust);
	set("tCreated", (long long)tCreated);
	fwrite(fout, string("ID"));
	fwrite(fout, ID);
	if (!empty()) {
		fwrite(fout, string("Conf"));
		writeb(fout);
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
		set(key, val);
	}
	map<string, string> strConf;
	fread(fin, strConf);
	for (auto& [key, val] : strConf) {
		set(key, val);
	}
	fread(fin, strNick);
}
void User::readb(std::ifstream& fin)
{
	std::lock_guard<std::mutex> lock_queue(ex_user);
	string tag;
	while ((tag = fread<string>(fin)) != "END") {
		if (tag == "ID")ID = fread<long long>(fin);
		else if (tag == "Conf")readb(fin);
		else if (tag == "Nick")fread(fin, strNick);
	}
	nTrust = get_int("trust");
	if (has("tCreated"))tCreated = get_ll("tCreated");
	if (has("tinyID"))TinyList.emplace(get_ll("tinyID"), ID);
}
int trustedQQ(long long uid){
	if (TinyList.count(uid))uid = TinyList[uid];
	if (uid == console)return 256;
	if (uid == console.DiceMaid)return 255;
	else if (!UserList.count(uid))return 0;
	else return UserList[uid]->nTrust;
}

int clearUser() {
	vector<long long> UserDelete;
	time_t tNow{ time(nullptr) };
	time_t userline{ tNow - console["InactiveUserLine"] * (time_t)86400 };
	bool isClearInactive{ console["InactiveUserLine"] > 0 };
	for (const auto& [uid, user] : UserList) {
		if (user->nTrust > 0 || console.is_self(uid))continue;
		if (user->empty()) {
			UserDelete.push_back(uid);
		}
		else if (isClearInactive) {
			time_t tLast{ user->updated() };
			if (!tLast)tLast = user->tCreated;
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
		if (sessions.has_session(uid))sessions.over(uid);
	}
	return UserDelete.size();
}
int clearGroup() {
	if (console["InactiveGroupLine"] <= 0)return 0;
	vector<long long> GrpDelete;
	time_t tNow{ time(nullptr) };
	time_t grpline{ tNow - console["InactiveGroupLine"] * (time_t)86400 };
	for (const auto& [id, grp] : ChatList) {
		if (grp->is_except() || grp->is("����") || grp->is("����"))continue;
		time_t tLast{ grp->updated() };
		if (auto s{ sessions.get_if({ 0,id }) })
			tLast = s->tUpdate > tLast ? s->tUpdate : tLast;
		if (tLast && tLast < grpline)GrpDelete.push_back(id);
	}
	for (auto id : GrpDelete) {
		ChatList.erase(id);
		if (sessions.has_session({ 0,id }))sessions.over({ 0,id });
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
AttrVar idx_nick(const AttrObject& eve) {
	if (eve->has("nick"))return eve->get("nick");
	if (!eve->has("uid"))return {};
	long long uid{ eve->get_ll("uid") };
	long long gid{ eve->get_ll("gid") };
	return eve->at("nick") = getName(uid, gid);
}

string filter_CQcode(const string& raw, long long fromGID){
	string msg{ raw };
	size_t posL(0), posR(0);
	while ((posL = msg.find(CQ_AT)) != string::npos)
	{
		//���at��ʽ
		if ((posR = msg.find(']', posL)) != string::npos)
		{
			std::string_view stvQQ(msg);
			stvQQ = stvQQ.substr(posL + 10, posR - posL - 10);
			//���QQ�Ÿ�ʽ
			bool isDig = true;
			for (auto ch: stvQQ) 
			{
				if (!isdigit(static_cast<unsigned char>(ch)))
				{
					isDig = false;
					break;
				}
			}
			//ת��
			if (isDig && posR - posL < 29) 
			{
				msg.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGID));
			}
			else if (stvQQ == "all") 
			{
				msg.replace(posL, posR - posL + 1, "@ȫ���Ա");
			}
			else
			{
				msg.replace(posL + 1, 9, "@");
			}
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_IMAGE)) != string::npos) {
		//���at��ʽ
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "ͼƬ");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_FACE)) != string::npos) {
		//���at��ʽ
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "����");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_POKE)) != string::npos) {
		//���at��ʽ
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, 11, "��һ��");
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_FILE)) != string::npos) {
		//���at��ʽ
		if ((posR = msg.find(']', posL)) != string::npos) {
			msg.replace(posL + 1, posR - posL - 1, "�ļ�");
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
		//���at��ʽ
		if (size_t posR = msg.find(']', posL); posR != string::npos)
		{
			std::string_view stvQQ(msg);
			stvQQ = stvQQ.substr(posL + 10, posR - posL - 10);
			//���QQ�Ÿ�ʽ
			bool isDig = true;
			for (auto ch : stvQQ)
			{
				if (!isdigit(static_cast<unsigned char>(ch)))
				{
					isDig = false;
					break;
				}
			}
			//ת��
			if (isDig && posR - posL < 29)
			{
				msg.replace(posL, posR - posL + 1, "@" + getName(stoll(string(stvQQ)), fromGID));
			}
			else if (stvQQ == "all")
			{
				msg.replace(posL, posR - posL + 1, "@ȫ���Ա");
			}
			else
			{
				msg.replace(posL + 1, 9, "@");
			}
		}
		else return msg;
	}
	while ((posL = msg.find(CQ_POKE)) != string::npos) {
		//���at��ʽ
		if (size_t posR = msg.find(']', posL); posR != string::npos) {
			msg.replace(posL + 1, 11, "��һ��");
		}
		else return msg;
	}
	return msg;
}

Chat& chat(long long id)
{
	if (!ChatList.count(id)) {
		ChatList.emplace(id, std::make_shared<Chat>(id));
	}
	return *ChatList[id];
}

void Chat::leave(const string& msg) {
	if (!msg.empty()) {
		DD::sendGroupMsg(ID, msg);
		std::this_thread::sleep_for(500ms);
	}
	DD::setGroupLeave(ID);
	reset("����Ⱥ").rmLst();
}
bool Chat::is_except()const {
	return has("���") || has("Э����Ч");
}
void Chat::writeb(std::ofstream& fout)
{
	dict["Name"] = Name;
	dict["inviter"] = (long long)inviter;
	dict["tCreated"] = (long long)tCreated;
	fwrite(fout, ID);
	if (!Name.empty())
	{
		fwrite(fout, static_cast<short>(0));
		fwrite(fout, Name);
	}
	if (!empty())
	{
		fwrite(fout, static_cast<short>(10));
		writeb(fout);
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
				set(key, true);
			}
			break;
		case 2:
			for (auto& [key, val] : fread<string, int>(fin)) {
				set(key, val);
			}
			break;
		case 3:
			for (auto& [key, val] : fread<string, string>(fin)) {
				set(key, val);
			}
			break;
		case 10:
			readb(fin);
			break;
		case 20:
			fread(fin, ChConf);
			break;
		default:
			return;
		}
		tag = fread<short>(fin);
	}
	if (has("Name"))Name = get_str("Name");
	inviter = get_ll("inviter");
	if (has("tCreated"))tCreated = get_ll("tCreated");
}
int groupset(long long id, const string& st){
	return ChatList.count(id) ? ChatList[id]->is(st) : -1;
}

Chat& Chat::update() {
	dict["tUpdated"] = (long long)time(nullptr);
	return *this;
}
Chat& Chat::update(time_t tt) {
	dict["tUpdated"] = (long long)tt;
	return *this;
}
Chat& Chat::setLst(time_t t) {
	dict["lastMsg"] = (long long)t;
	return *this;
}
string Chat::print(){
	if (string name{ DD::getGroupName(ID) }; !name.empty())Name = name;
	if (!Name.empty())return "[" + Name + "](" + to_string(ID) + ")";
	return "Ⱥ(" + to_string(ID) + ")";
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

//��ȡ����Ӳ��(ǧ�ֱ�)
long long getDiskUsage(double& mbFreeBytes, double& mbTotalBytes){
	/*int sizStr = GetLogicalDriveStrings(0, NULL);//��ñ��������̷�����Drive������
	char* chsDrive = new char[sizStr];//��ʼ���������Դ洢�̷���Ϣ
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
	//GetDiskFreeSpaceEx���������Ի�ȡ���������̵Ŀռ�״̬,�������ص��Ǹ�BOOL��������
	if (fResult) {
		mbTotalBytes = i64TotalBytes * 1000 / 1024 / 1024 / 1024 / 1000.0;//����������
		mbFreeBytes = i64FreeBytesToCaller * 1000 / 1024 / 1024 / 1024 / 1000.0;//����ʣ��ռ�
		return 1000 - 1000 * i64FreeBytesToCaller / i64TotalBytes;
	}
	return 0;
}

#endif