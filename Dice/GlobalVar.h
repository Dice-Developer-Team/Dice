/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2020 w4123溯洄
 * Copyright (C) 2019-2020 String.Empty
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
#ifndef DICE_GLOBAL_VAR
#define DICE_GLOBAL_VAR
#include "CQLogger.h"
#include <map>
#include "STLExtern.hpp"
#include "BotEnvironment.h"

 /*
  * 版本信息
  * 请勿修改Dice_Build, Dice_Ver_Without_Build，DiceRequestHeader以及Dice_Ver常量
  * 请修改Dice_Short_Ver或Dice_Full_Ver常量以达到版本自定义
  */
const unsigned short Dice_Build = 560u;
inline const std::string Dice_Ver_Without_Build = "2.4.0alpha";
constexpr auto DiceRequestHeader = "Dice/2.4.0ALPHA";
inline const std::string Dice_Ver = Dice_Ver_Without_Build + "(" + std::to_string(Dice_Build) + ")";
inline const std::string Dice_Short_Ver = "Dice! by 溯洄 Shiki Ver " + Dice_Ver;

#ifdef __clang__

#ifdef _MSC_VER
const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + __clang_version__ + " MSVC " + std::to_string(_MSC_FULL_VER) + " " + __DATE__ + " " + __TIME__;
#elif defined(__GNUC__)
const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + __clang_version__ + " GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__;
#else
const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + __clang_version__ + " UNKNOWN";
#endif /*__clang__*/

#else

#ifdef _MSC_VER
const std::string Dice_Full_Ver = std::string(Dice_Short_Ver) + "[" + __DATE__ + " " + __TIME__;
#elif defined(__GNUC__)
const std::string Dice_Full_Ver = Dice_Short_Ver + " [GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__;
#else
const std::string Dice_Full_Ver = Dice_Short_Ver + " [UNKNOWN COMPILER";
#endif /*__clang__*/

#endif /*_MSC_VER*/





// 应用是否被启用
extern bool Enabled;

// 是否在Mirai环境中运行
extern bool Mirai;
extern std::string Dice_Full_Ver_For;

// 消息发送线程是否正在运行
extern bool msgSendThreadRunning;

// 全局酷QLogger
extern CQ::logger DiceLogger;

// 回复信息, 此内容可以通过CustomMsg功能修改而无需修改源代码
extern std::map<std::string, std::string> GlobalMsg;
// 修改后的Global语句
extern std::map<std::string, std::string> EditedMsg;
// 帮助文档
extern const std::map<std::string, std::string, less_ci> HelpDoc;
// 修改后的帮助文档
inline std::map<std::string, std::string, less_ci> CustomHelp;
std::string getMsg(std::string key, std::map<std::string, std::string> tmp = {});

#endif /*DICE_GLOBAL_VAR*/
