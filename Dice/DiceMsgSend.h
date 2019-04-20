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
#include "DiceMsg.h"

/*
 *  加锁并将消息存入消息发送队列(移动)
 *  Param:
 *  Dice::DiceMsg&& dice_msg 消息信息
 */
void AddMsgToQueue(Dice::DiceMsg&& dice_msg);

/*
 *  加锁并将消息存入消息发送队列(复制)
 *  Param:
 *  const Dice::DiceMsg& dice_msg 消息信息
 */
void AddMsgToQueue(const Dice::DiceMsg& dice_msg);

/*
 * 消息发送线程函数
 * 注意: 切勿在主线程中调用此函数, 此函数仅用于初始化消息发送线程
 */
void SendMsg();
#endif /*DICE_MSG_SEND*/
