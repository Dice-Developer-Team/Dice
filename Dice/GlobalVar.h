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

#ifndef DICE_GLOBAL_VAR
#define DICE_GLOBAL_VAR
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <shared_mutex>
#include "STLExtern.hpp"

/*
 * Dice Version
 * Please Do not modify Dice_Build, Dice_Ver_Without_Build, DiceRequestHeader or Dice_Ver
 * To costum version info, please modify const Dice_Short_Ver or Dice_Full_Ver
 */
constexpr unsigned short Dice_Build = 662u;
inline const std::string Dice_Ver_Without_Build = "2.7.0beta8";
constexpr auto DiceRequestHeader = "Dice/2.7.0";
inline const std::string Dice_Ver = Dice_Ver_Without_Build + "(" + std::to_string(Dice_Build) + ")";
extern std::string Dice_Short_Ver;
constexpr bool isDev = true;
extern std::string Dice_Full_Ver;

#ifdef _WIN32
// DLL hModule
extern HMODULE hDllModule;
#endif 

// if is enabled
extern bool Enabled;

// Dice最完整的版本字符串
extern std::string Dice_Full_Ver_On;

//extern std::string strModulePath;

// 消息发送线程是否正在运行
extern bool msgSendThreadRunning;

// 回复信息, 此内容可以通过CustomMsg功能修改而无需修改源代码
extern std::shared_mutex GlobalMsgMutex;
extern dict_ci<string> GlobalMsg;
extern const dict_ci<string> PlainMsg;
// 修改后的Global语句
extern fifo_dict_ci<string> EditedMsg;
// 语句注释
extern const dict_ci<string> GlobalComment;
// 帮助文档
extern const dict_ci<string> HelpDoc;
// 修改后的帮助文档
extern fifo_dict_ci<string> CustomHelp;
const std::string getComment(const std::string& key);

#endif /*DICE_GLOBAL_VAR*/
