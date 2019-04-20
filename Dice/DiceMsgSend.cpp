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
#include "CQAPI_EX.h"
#include "DiceMsg.h"
#include "DiceMsgType.h"
#include "GlobalVar.h"
using namespace std;


// 消息发送队列
std::queue<Dice::DiceMsg> msgQueue;

// 消息发送队列锁
mutex msgQueueMutex;

void AddMsgToQueue(Dice::DiceMsg&& dice_msg)
{
	lock_guard<std::mutex> lock_queue(msgQueueMutex);
	msgQueue.push(move(dice_msg));
}

void AddMsgToQueue(const Dice::DiceMsg& dice_msg)
{
	lock_guard<std::mutex> lock_queue(msgQueueMutex);
	msgQueue.push(dice_msg);
}


void SendMsg()
{
	Enabled = true;
	msgSendThreadRunning = true;
	while (Enabled)
	{
		Dice::DiceMsg dice_msg;
		{
			lock_guard<std::mutex> lock_queue(msgQueueMutex);
			if (!msgQueue.empty())
			{
				dice_msg = std::move(msgQueue.front());
				msgQueue.pop();
			}
		}
		if (!dice_msg.msg.empty())
		{
			if (dice_msg.msg_type == Dice::MsgType::Private)
			{
				CQ::sendPrivateMsg(dice_msg.qq_id, dice_msg.msg);
			}
			else if (dice_msg.msg_type == Dice::MsgType::Group)
			{
				CQ::sendGroupMsg(dice_msg.group_id, dice_msg.msg);
			}
			else
			{
				CQ::sendDiscussMsg(dice_msg.group_id, dice_msg.msg);
			}
		}
		else
		{
			this_thread::sleep_for(chrono::milliseconds(20));
		}
	}
	msgSendThreadRunning = false;
}
