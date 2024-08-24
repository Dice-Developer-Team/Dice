#pragma once

/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
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

#ifndef DICE_MSG_SEND
#define DICE_MSG_SEND
#include "fifo_json.hpp"

class AnysTable;
enum class msgtype : int { Private = 0, Group = 1, Discuss = 2, ChannelPrivate = 3, Channel = 4};
struct chatInfo {
	long long uid{ 0 };
	long long gid{ 0 };
	long long chid{ 0 };
	msgtype type{ 0 };
	chatInfo() {}
	chatInfo(long long, long long = 0, long long = 0);
	chatInfo(const AnysTable&);
	bool operator<(const chatInfo&)const;
	bool operator==(const chatInfo&)const;
	operator bool()const { return uid || gid || chid; }
	chatInfo locate()const { return gid ? chatInfo{ 0,gid,chid } : *this; }
	static chatInfo from_json(const fifo_json& chat);
};

fifo_json to_json(const chatInfo& chat);
template<>
struct std::hash<chatInfo> {
	size_t operator()(const chatInfo& chat)const {
		return std::hash<long long>()(chat.uid)
			^ std::hash<long long>()(chat.gid)
			^ std::hash<long long>()(chat.chid);
	}
};
std::ifstream& operator>>(std::ifstream& fin, msgtype& t);
std::ifstream& operator>>(std::ifstream& fin, chatInfo& ct);
/*
 *  加锁并将消息存入消息发送队列
 *  Param:
 *  const std::string& msg 消息内容字符串
 *  long long target_id 目标ID(QQ,群号或讨论组uin)
 *  MsgType msg_type 消息类型
 */
void AddMsgToQueue(const std::string& msg, long long target_id);
void AddMsgToQueue(const std::string& msg, chatInfo ct);

/*
 * 消息发送线程函数
 * 注意: 切勿在主线程中调用此函数, 此函数仅用于初始化消息发送线程
 */
void SendMsg();
#endif /*DICE_MSG_SEND*/
