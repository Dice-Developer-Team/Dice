/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123���
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
#include <queue>
#include <mutex>
#include <string>
#include <thread>
#include "DDAPI.h"
#include "DiceMsgSend.h"
#include "MsgFormat.h"
#include "GlobalVar.h"
#include "DiceConsole.h"
using namespace std;

chatInfo::chatInfo(long long u, long long g, long long c) :uid(u), gid(g), chid(c) {
	if (gid) {
		type = chid ? msgtype::Channel : msgtype::Group;
	}
	else {
		type = chid ? msgtype::ChannelPrivate : msgtype::Private;
	}
}
chatInfo::chatInfo(const AnysTable& obj):uid(obj.get_ll("uid")), gid(obj.get_ll("gid")), chid(obj.get_ll("chid")) {
	type = (gid = obj.get_ll("gid"))
		? chid ? msgtype::ChannelPrivate : msgtype::Private
		: chid ? msgtype::Channel : msgtype::Group;
}
bool chatInfo::operator<(const chatInfo& other)const {
	return type == other.type
		? uid == other.uid
		? gid == other.gid
		? chid < other.chid
		: gid < other.gid
		: uid < other.uid
		: type < other.type;
}
bool chatInfo::operator==(const chatInfo& other)const {
	return uid == other.uid
		&& gid == other.gid
		&& chid == other.chid;
}
fifo_json to_json(const chatInfo& chat) {
	fifo_json j;
	if (chat.chid)j["chid"] = chat.chid;
	if (chat.gid)j["gid"] = chat.gid;
	if (chat.uid)j["uid"] = chat.uid;
	return j;
}
chatInfo chatInfo::from_json(const fifo_json& chat) {
	if (chat.is_null())return {};
	long long uid{ 0 }, gid{ 0 }, chid{ 0 };
	if (chat.count("uid"))chat["uid"].get_to(uid);
	if (chat.count("gid"))chat["gid"].get_to(gid);
	if (chat.count("chid"))chat["chid"].get_to(chid);
	return chatInfo{ uid,gid,chid };
}

// ��Ϣ���ʹ洢�ṹ��
struct msg_t
{
	string msg;
	chatInfo target{};
	msg_t() = default;

	msg_t(const string& s, chatInfo ct) : msg(s),target(ct)
	{
	}
};

// ��Ϣ���Ͷ���
std::queue<msg_t> msgQueue;

// ��Ϣ���Ͷ�����
mutex msgQueueMutex;

void AddMsgToQueue(const string& msg, long long target_id)
{
	lock_guard<std::mutex> lock_queue(msgQueueMutex);
	msgQueue.emplace(msg_t(msg, { target_id }));
}

void AddMsgToQueue(const std::string& msg, chatInfo ct)
{
	lock_guard<std::mutex> lock_queue(msgQueueMutex);
	msgQueue.emplace(msg_t(msg, ct));
}


void SendMsg()
{
	msgSendThreadRunning = true;
	while (Enabled)
	{
		msg_t msg;
		{
			lock_guard<std::mutex> lock_queue(msgQueueMutex);
			if (!msgQueue.empty())
			{
				msg = msgQueue.front();
				msgQueue.pop();
			}
		}
		if (!msg.msg.empty())
		{
			if (int pos = msg.msg.find_first_not_of(" \t\r\n"); pos && pos != string::npos) {
				msg.msg = msg.msg.substr(pos);
			}
			if (int pos = msg.msg.find('\f'); pos != string::npos) 
			{
				AddMsgToQueue(msg.msg.substr(pos + 1), msg.target);
				msg.msg = msg.msg.substr(0, pos);
			}
			if (msg.target.type == msgtype::Private)
			{
				if (console["DisableStrangerChat"] && !DD::isFriend(msg.target.uid, true))continue;
				DD::sendPrivateMsg(msg.target.uid, msg.msg);
			}
			else if (msg.target.type == msgtype::Group)
			{
				DD::sendGroupMsg(msg.target.gid, msg.msg);
			}
			else if (msg.target.type == msgtype::Channel)
			{
				DD::sendChannelMsg(msg.target.gid, msg.target.chid, msg.msg);
			}
		}
		if (msgQueue.size() > 2)this_thread::sleep_for(chrono::milliseconds(console["SendIntervalBusy"]));
		else this_thread::sleep_for(chrono::milliseconds(console["SendIntervalIdle"]));
	}
	msgSendThreadRunning = false;
}
