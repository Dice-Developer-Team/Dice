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
#pragma once
#ifndef DICE_MSG_SEND
#define DICE_MSG_SEND
#include <string>
#include "CQMsgSend.h"

using chatType = std::pair<long long, CQ::msgtype>;
std::ifstream& operator>>(std::ifstream& fin, CQ::msgtype& t);
std::ifstream& operator>>(std::ifstream& fin, chatType& ct);
std::ofstream& operator<<(std::ofstream& fout, const chatType& ct);
/*
 *  加锁并将消息存入消息发送队列
 *  Param:
 *  const std::string& msg 消息内容字符串
 *  long long target_id 目标ID(QQ,群号或讨论组uin)
 *  MsgType msg_type 消息类型
 */
void AddMsgToQueue(const std::string& msg, long long target_id, CQ::msgtype msg_type = CQ::msgtype::Private);
void AddMsgToQueue(const std::string& msg, chatType ct);

/*
 * 消息发送线程函数
 * 注意: 切勿在主线程中调用此函数, 此函数仅用于初始化消息发送线程
 */
void SendMsg();
#endif /*DICE_MSG_SEND*/
