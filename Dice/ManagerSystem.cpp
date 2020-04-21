/*
 * 后台系统
 * Copyright (C) 2019-2020 String.Empty
 */
#include <windows.h>
#include "ManagerSystem.h"

User& getUser(long long qq) {
	if (!UserList.count(qq))UserList[qq].id(qq);
	return UserList[qq];
}
short trustedQQ(long long qq) {
	if (!UserList.count(qq))return 0;
	else return UserList[qq].nTrust;
}
int clearUser() {
	int cnt = 0;
	for (auto& [qq, user] : UserList) {
		if (user.empty()) {
			UserList.erase(qq);
			cnt++;
		}
	}
	return cnt;
}

string getName(long long QQ, long long GroupID)
{
	string nick;
	if (getUser(QQ).getNick(nick, GroupID))return nick;
	if (GroupID && !(nick = strip(CQ::getGroupMemberInfo(GroupID, QQ).GroupNick)).empty())return nick;
	if (!(nick = strip(CQ::getStrangerInfo(QQ).nick)).empty())return nick;
	return GlobalMsg["stranger"] + "(" + to_string(QQ) + ")";
}

Chat& chat(long long id) {
	if (!ChatList.count(id))ChatList[id].id(id);
	return ChatList[id];
}
int groupset(long long id, string st) {
	if (!ChatList.count(id))return -1;
	else return ChatList[id].isset(st);
}
string printChat(Chat& grp) {
	if (CQ::getGroupList().count(grp.ID))return CQ::getGroupList()[grp.ID] + "(" + to_string(grp.ID) + ")";
	if (grp.isset("群名"))return grp.strConf["群名"] + "(" + to_string(grp.ID) + ")";
	if (grp.isGroup) return "群" + to_string(grp.ID) + "";
	return "讨论组" + to_string(grp.ID) + "";
}
DWORD getRamPort() {
	MEMORYSTATUSEX memory_status;
	memory_status.dwLength = sizeof(memory_status);
	GlobalMemoryStatusEx(&memory_status);
	return memory_status.dwMemoryLoad;
}