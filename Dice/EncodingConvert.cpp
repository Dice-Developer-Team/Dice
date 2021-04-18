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
#include "EncodingConvert.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <iconv.h>
#endif
#include <string>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iterator>

bool checkUTF8(const std::string& strUTF8) {
	size_t cntUTF8(0);
	int num = 0;
	size_t i = 0;
	while (i < strUTF8.length()) {
		if ((strUTF8[i] & 0x80) == 0x00) {
			i++;
			continue;
		}
		else if ((strUTF8[i] & 0xc0) == 0xc0 && (strUTF8[i] & 0xfe) != 0xfe) {
			// 110X_XXXX 10XX_XXXX
			// 1110_XXXX 10XX_XXXX 10XX_XXXX
			// 1111_0XXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
			// 1111_10XX 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
			// 1111_110X 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
			unsigned char mask = 0x80;
			for (num = 0; num < 8; ++num) {
				if ((strUTF8[i] & mask) == mask) {
					mask = mask >> 1;
				}
				else
					break;
			}
			for (int j = 0; j < num - 1; j++) {
				if ((strUTF8[++i] & 0xc0) != 0x80) {
					return false;
				}
			}
			++i;
			if (++cntUTF8 > 10)return true;
		}
		else {
			return false;
		}
	}
	return cntUTF8;
}
bool checkUTF8(std::ifstream& fin) {
	size_t cntUTF8(0);
	int num = 0;
	char ch{0};
	while (fin >> ch) {
		if ((ch & 0x80) == 0x00) {
			continue;
		}
		else if ((ch & 0xc0) == 0xc0 && (ch & 0xfe) != 0xfe) {
			// 110X_XXXX 10XX_XXXX
			// 1110_XXXX 10XX_XXXX 10XX_XXXX
			// 1111_0XXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
			// 1111_10XX 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
			// 1111_110X 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX 10XX_XXXX
			unsigned char mask = 0x80;
			for (num = 0; num < 8; ++num) {
				if ((ch & mask) == mask) {
					mask = mask >> 1;
				}
				else
					break;
			}
			for (int j = 0; j < num - 1; j++) {
				if ((fin >> ch) && (ch & 0xc0) != 0x80) {
					return false;
				}
			}
			if (++cntUTF8 > 10)return true;
		}
		else {
			return false;
		}
	}
	return cntUTF8;
}
// 现在是GBK了
std::string GBKtoUTF8(const std::string& strGBK, bool isTrial)
{
	if (isTrial && checkUTF8(strGBK))return strGBK;
#ifdef _WIN32
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
#else
	return ConvertEncoding<char>(strGBK, "gb18030", "utf-8");
#endif
}

std::vector<std::string> GBKtoUTF8(const std::vector<std::string>& strGBK)
{
	std::vector<std::string> vOutUTF8;
	std::transform(strGBK.begin(), strGBK.end(), std::back_inserter(vOutUTF8), [](const std::string& s) { return GBKtoUTF8(s); });
	return vOutUTF8;
}

// 事实上是GB18030
std::string UTF8toGBK(const std::string& strUTF8, bool isTrial)
{
	if (isTrial && !checkUTF8(strUTF8))return strUTF8;
#ifdef _WIN32
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
#else
	return ConvertEncoding<char>(strUTF8, "utf-8", "gb18030");
#endif
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
