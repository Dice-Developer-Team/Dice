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
#include <Windows.h>
#include <string>
#include <fstream>
#include "CQEVE_ALL.h"
#include "EncodingConvert.h"
#include "DiceNetwork.h"
#include "GlobalVar.h"

EVE_Menu(__eventMenu)
{
	std::string ver;
	if (!Network::GET("api.kokona.tech", "/getVer", 5555, ver))
	{
		MessageBoxA(nullptr, ("检查更新时遇到错误: \n" + ver).c_str(), "Dice!更新错误", MB_OK | MB_ICONWARNING);
		return -1;
	}

	ver = UTF8toGBK(ver);
	const std::string serverBuild = ver.substr(ver.find('(') + 1, ver.find(')') - ver.find('(') - 1);
	if (Dice_Build >= stoi(serverBuild))
	{
		MessageBoxA(nullptr, "您正在使用的Dice!已为最新版本!", "Dice!更新", MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	const std::string updateNotice = "发现新版本! \n\n当前版本: " + Dice_Ver + "\n新版本: " + ver + "\n\n警告: 如果正在使用的Dice!为修改版, 此更新会将其覆盖为原版!\n点击确定键开始更新, 取消键取消更新";
	
	if (MessageBoxA(nullptr, updateNotice.c_str(), "Dice!更新", MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
	{
		MessageBoxA(nullptr, "更新已取消", "Dice!更新", MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	char buffer[MAX_PATH];
	const DWORD length = GetModuleFileNameA(nullptr, buffer, sizeof buffer);

	if (length == MAX_PATH || length == 0)
	{
		MessageBoxA(nullptr, "路径读取失败!", "Dice!更新错误", MB_OK | MB_ICONERROR);
		return -1;
	}

	std::string filePath(buffer, length);
	filePath = filePath.substr(0, filePath.find_last_of("\\")) + "\\app\\com.w4123.dice.cpk";
	
	std::string fileContent;
	if (!Network::GET("api.kokona.tech", "/getDice", 5555, fileContent))
	{
		MessageBoxA(nullptr, ("新版本文件下载失败! 请检查您的网络状态! 错误信息: " + fileContent).c_str(), "Dice!更新错误", MB_OK | MB_ICONERROR);
		return -1;
	}
	
	std::ofstream streamUpdate(filePath, std::ios::trunc | std::ios::binary | std::ios::out);
	if (!streamUpdate)
	{
		MessageBoxA(nullptr, "文件打开失败!", "Dice!更新错误", MB_OK | MB_ICONERROR);
		return -1;
	}

	streamUpdate << fileContent;
	streamUpdate.close();
	MessageBoxA(nullptr, "Dice!已更新, 请重新启动酷Q!", "Dice!更新", MB_OK | MB_ICONINFORMATION);
	return 0;
}

