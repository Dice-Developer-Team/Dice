/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123溯洄
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


// 消息发送存储结构体
struct msg_t
{
	string msg;
	long long target_id = 0;
	msgtype msg_type{};
	msg_t() = default;

	msg_t(string msg, long long target_id, msgtype msg_type) : msg(move(msg)), target_id(target_id),
	                                                           msg_type(msg_type)
	{
	}
};

// 消息发送队列
std::queue<msg_t> msgQueue;

// 消息发送队列锁
mutex msgQueueMutex;

void AddMsgToQueue(const string& msg, long long target_id, msgtype msg_type)
{
	lock_guard<std::mutex> lock_queue(msgQueueMutex);
	msgQueue.emplace(msg_t(msg, target_id, msg_type));
}

void AddMsgToQueue(const std::string& msg, chatType ct)
{
	AddMsgToQueue(msg, ct.first, ct.second);
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
				AddMsgToQueue(msg.msg.substr(pos + 1), msg.target_id, msg.msg_type);
				msg.msg = msg.msg.substr(0, pos);
			}
			if (msg.msg_type == msgtype::Private)
			{
				DD::sendPrivateMsg(msg.target_id, msg.msg);
			}
			else if (msg.msg_type == msgtype::Group)
			{
				DD::sendGroupMsg(msg.target_id, msg.msg);
			}
			else
			{
				DD::sendDiscussMsg(msg.target_id, msg.msg);
			}
		}
		if (msgQueue.size() > 2)this_thread::sleep_for(chrono::milliseconds(console["SendIntervalBusy"]));
		else this_thread::sleep_for(chrono::milliseconds(console["SendIntervalIdle"]));
	}
	msgSendThreadRunning = false;
}
