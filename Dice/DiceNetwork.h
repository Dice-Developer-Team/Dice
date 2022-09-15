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
 * Copyright (C) 2019-2022 String.Empty
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
#ifndef _Win32
	size_t curlWriteToString(void* contents, size_t size, size_t nmemb, std::string* s);
#endif //  Win32
	// 发出一个HTTP POST 请求
	// @param url 
	// @param postContent 编码后的数据
	// @param contentType 数据类型，可为空
	// @param des 存储返回数据
	bool POST(const string& url, const string& postContent, const string& contentType, std::string& des);
	// 发出一个HTTP GET 请求
	// @param url 
	// @param des 存储返回数据
	bool GET(const string& url, std::string& des);
}
#endif /*DICE_NETWORK*/
