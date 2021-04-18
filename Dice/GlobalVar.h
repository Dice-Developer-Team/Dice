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
 * Copyright (C) 2019-2021 String.Empty
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
#include <map>
#include <unordered_map>
#include "STLExtern.hpp"

/*
 * 版本信息
 * 请勿修改Dice_Build, Dice_Ver_Without_Build，DiceRequestHeader以及Dice_Ver常量
 * 请修改Dice_Short_Ver或Dice_Full_Ver常量以达到版本自定义
 */
const unsigned short Dice_Build = 578u;
inline const std::string Dice_Ver_Without_Build = "2.5.1";
constexpr auto DiceRequestHeader = "Dice/2.5.1";
inline const std::string Dice_Ver = Dice_Ver_Without_Build + "(" + std::to_string(Dice_Build) + ")";
inline const std::string Dice_Short_Ver = "Dice! by 溯洄 & Shiki Ver " + Dice_Ver;

#ifdef __clang__

#ifdef _MSC_VER
inline const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__) + " with MSVC " + std::to_string(_MSC_VER) + " " + __DATE__ + " " + __TIME__;
#elif defined(__GNUC__)
inline const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__) +  " with GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__;
#else
inline const std::string Dice_Full_Ver = Dice_Short_Ver + " [CLANG " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#endif

#else

#ifdef _MSC_VER
inline const std::string Dice_Full_Ver = std::string(Dice_Short_Ver) + "[MSVC " + std::to_string(_MSC_VER) + " " + __DATE__ +
	" " + __TIME__ + "]";
#elif defined(__GNUC__)
inline const std::string Dice_Full_Ver = Dice_Short_Ver + " [GNUC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + " " + __DATE__ + " " + __TIME__;
#else
inline const std::string Dice_Full_Ver = Dice_Short_Ver + " [UNKNOWN COMPILER";
#endif

#endif

#ifdef _WIN32
// DLL hModule
extern HMODULE hDllModule;
#endif 

// 应用是否被启用
extern bool Enabled;

// 运行环境
//enum class QQFrame { CoolQ, Mirai, XianQu };
//extern QQFrame frame;

// Dice最完整的版本字符串
extern std::string Dice_Full_Ver_On;

// 可执行文件位置
//extern std::string strModulePath;

// 消息发送线程是否正在运行
extern bool msgSendThreadRunning;

// 回复信息, 此内容可以通过CustomMsg功能修改而无需修改源代码
extern std::map<std::string, std::string, less_ci> GlobalMsg;
// 修改后的Global语句
extern std::map<std::string, std::string, less_ci> EditedMsg;
// 帮助文档
extern const std::map<std::string, std::string, less_ci> HelpDoc;
// 修改后的帮助文档
inline std::map<std::string, std::string, less_ci> CustomHelp;
std::string getMsg(const std::string& key, const std::unordered_map<std::string, std::string>& tmp = {});

#endif /*DICE_GLOBAL_VAR*/
