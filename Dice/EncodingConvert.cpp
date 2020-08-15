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

#define CP_GBK (936)
#define WIN32_LEAN_AND_MEAN
#include "EncodingConvert.h"
#include <Windows.h>
#include <string>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iterator>

// 现在是GBK了
std::string GBKtoUTF8(const std::string& strGBK)
{
	const int UTF16len = MultiByteToWideChar(CP_GBK, 0, strGBK.c_str(), -1, nullptr, 0);
	auto* const strUTF16 = new wchar_t[UTF16len];
	MultiByteToWideChar(CP_GBK, 0, strGBK.c_str(), -1, strUTF16, UTF16len);
	const int UTF8len = WideCharToMultiByte(CP_UTF8, 0, strUTF16, -1, nullptr, 0, nullptr, nullptr);
	auto* const strUTF8 = new char[UTF8len];
	WideCharToMultiByte(CP_UTF8, 0, strUTF16, -1, strUTF8, UTF8len, nullptr, nullptr);
	std::string strOutUTF8(strUTF8);
	delete[] strUTF16;
	delete[] strUTF8;
	return strOutUTF8;
}

std::vector<std::string> GBKtoUTF8(const std::vector<std::string>& strGBK)
{
	std::vector<std::string> vOutUTF8;
	std::transform(strGBK.begin(), strGBK.end(), std::back_inserter(vOutUTF8), [](const std::string& s) { return GBKtoUTF8(s); });
	return vOutUTF8;
}

// 事实上是GB18030
std::string UTF8toGBK(const std::string& strUTF8)
{
	const int UTF16len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, nullptr, 0);
	auto* const strUTF16 = new wchar_t[UTF16len];
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, strUTF16, UTF16len);
	const int GBKlen = WideCharToMultiByte(CP_GBK, 0, strUTF16, -1, nullptr, 0, nullptr, nullptr);
	auto* const strGBK = new char[GBKlen];
	WideCharToMultiByte(CP_GBK, 0, strUTF16, -1, strGBK, GBKlen, nullptr, nullptr);
	std::string strOutGBK(strGBK);
	delete[] strUTF16;
	delete[] strGBK;
	return strOutGBK;
}

std::vector<std::string> UTF8toGBK(const std::vector<std::string>& vUTF8)
{
	std::vector<std::string> vOutGBK;
	std::transform(vUTF8.begin(), vUTF8.end(), std::back_inserter(vOutGBK), [](const std::string& s) { return UTF8toGBK(s); });
	return vOutGBK;
}

unsigned char ToHex(const unsigned char x)
{
	return x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(const unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else y = x - '0';
	return y;
}

std::string UrlEncode(const std::string& str)
{
	std::string strTemp;
	const size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum(static_cast<unsigned char>(str[i])) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex(static_cast<unsigned char>(str[i]) >> 4);
			strTemp += ToHex(static_cast<unsigned char>(str[i]) % 16);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp;
	const size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (str[i] == '+') strTemp += ' ';
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			const unsigned char high = FromHex(static_cast<unsigned char>(str[++i]));
			const unsigned char low = FromHex(static_cast<unsigned char>(str[++i]));
			strTemp += static_cast<char>(high * 16 + low);
		}
		else strTemp += str[i];
	}
	return strTemp;
}
