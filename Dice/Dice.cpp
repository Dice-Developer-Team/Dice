/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123ËÝä§
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
#include <fstream>
#include <iostream>
#include "APPINFO.h"
#include "DiceMsg.h"
#include "DiceMsgSend.h"
#include "DiceMsgType.h"
#include "CQEVE.h"
#include "CQEVE_ALL.h"
#include "EventHandler.h"
#include "cqdefine.h"

using namespace std;
using namespace CQ;

Dice::EventHandler MainEventHandler;

EVE_Enable(eventEnable)
{
	MainEventHandler.HandleEnableEvent();
	return 0;
}

EVE_PrivateMsg_EX(eventPrivateMsg)
{
	if (eve.isSystem())return;
	bool block_msg = false;
	MainEventHandler.HandleMsgEvent(Dice::DiceMsg(std::move(eve.message), 0LL, eve.fromQQ, Dice::MsgType::Private), block_msg);
	if (block_msg) eve.message_block();
}

EVE_GroupMsg_EX(eventGroupMsg)
{
	if (eve.isSystem() || eve.isAnonymous()) return;
	bool block_msg = false;
	MainEventHandler.HandleMsgEvent(Dice::DiceMsg(std::move(eve.message), eve.fromGroup, eve.fromQQ, Dice::MsgType::Group), block_msg);
	if (block_msg) eve.message_block();
}

EVE_DiscussMsg_EX(eventDiscussMsg)
{
	if (eve.isSystem())return;
	bool block_msg = false;
	MainEventHandler.HandleMsgEvent(Dice::DiceMsg(std::move(eve.message), eve.fromDiscuss, eve.fromQQ, Dice::MsgType::Discuss), block_msg);
	if (block_msg) eve.message_block();
}

EVE_System_GroupMemberIncrease(eventGroupMemberIncrease)
{
	MainEventHandler.HandleGroupMemberIncreaseEvent(beingOperateQQ, fromGroup);
	return 0;
}

EVE_Disable(eventDisable)
{
	MainEventHandler.HandleDisableEvent();
	return 0;
}

EVE_Exit(eventExit)
{
	MainEventHandler.HandleExitEvent();
	return 0;
}

MUST_AppInfo_RETURN(CQAPPID);
