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
#include <string>
#include <Windows.h>
#include <cassert>

std::string GBKtoUTF8(const std::string& strGBK)
{
	int len = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, nullptr, 0);
	auto* const str1 = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, len);
	len = WideCharToMultiByte(CP_UTF8, 0, str1, -1, nullptr, 0, nullptr, nullptr);
	auto* const str2 = new char[len + 1];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, len, nullptr, nullptr);
	std::string strOutUTF8(str2);
	delete[] str1;
	delete[] str2;
	return strOutUTF8;
}

std::string UTF8toGBK(const std::string& strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, nullptr, 0);
	auto* const wszGBK = new wchar_t[len + 1];
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, nullptr, 0, nullptr, nullptr);
	auto* const szGBK = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, nullptr, nullptr);
	std::string strTemp(szGBK);
	delete[] szGBK;
	delete[] wszGBK;
	return strTemp;
}

unsigned char ToHex(const unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
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
			unsigned char high = FromHex(static_cast<unsigned char>(str[++i]));
			unsigned char low = FromHex(static_cast<unsigned char>(str[++i]));
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}