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

#ifndef DICE_NETWORK
#define DICE_NETWORK
#include <string>

namespace Network
{
	// 发出一个HTTP POST (form) 请求
	// @param serverName 服务器FQDN或IP
	// @param objectName 请求Path
	// @param port 端口号
	// @param frmdata 编码后的表单数据
	// @param des 存储返回数据
	// @param useHttps 是否进行https请求
	bool POST(const char* serverName, const char* objectName, unsigned short port, char* frmdata, std::string& des, bool useHttps = false);
	
	// 发出一个HTTP GET 请求
	// @param serverName 服务器FQDN或IP
	// @param objectName 请求Path
	// @param port 端口号
	// @param des 存储返回数据
	// @param useHttps 是否进行https请求
	bool GET(const char* serverName, const char* objectName, unsigned short port, std::string& des, bool useHttps = false);
}
#endif /*DICE_NETWORK*/
