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

#ifndef DICE_ENCODING_CONVERT
#define DICE_ENCODING_CONVERT
#include <string>
#include <vector>
#include <cstring>
#ifndef _WIN32
#include <iconv.h>
#endif
#include <unordered_map>
#include <fstream>
constexpr const char* chDigit{ "0123456789" };

bool checkUTF8(const std::string& strUTF8);
bool checkUTF8(std::ifstream&);

std::string GBKtoLocal(const std::string& strGBK);
std::string LocaltoGBK(const std::string& str);
std::string UTF8toLocal(const std::string& str);
std::string LocaltoUTF8(const std::string& str);
std::string GBKtoUTF8(const std::string& strGBK, bool isTrial = false);

std::string UTF8toGBK(const std::string& strUTF8, bool isTrial = false);

std::string UtoGBK(const wchar_t*);
std::string UtoNative(const wchar_t*);

std::string UrlEncode(const std::string& str);
std::string UrlDecode(const std::string& str);
std::string Base64urlEncode(const std::string& str);
#ifndef _WIN32
template <typename T, typename Q>
std::basic_string<T> ConvertEncoding(const std::basic_string<Q>& in, const std::string& InEnc, const std::string& OutEnc, const double CapFac = 4.0)
{
	const auto cd = iconv_open(OutEnc.c_str(), InEnc.c_str());
	if (cd == (iconv_t)-1)
	{
		return std::basic_string<T>();
	}
	size_t in_len = in.size() * sizeof(Q);
	size_t out_len = size_t(in_len * CapFac + sizeof(T));
	char* in_ptr = const_cast<char*>(reinterpret_cast<const char*> (in.c_str()));
	char* out_ptr = new char[out_len]();

	// As out_ptr would be modified by iconv(), store a copy of it pointing to the beginning of the array
	char* out_ptr_copy = out_ptr;
	if (iconv(cd, &in_ptr, &in_len, &out_ptr, &out_len) == (size_t)-1)
	{
		delete[] out_ptr_copy;
		iconv_close(cd);
		return std::basic_string<T>();
	}
	std::basic_string<T> ret(reinterpret_cast<T*>(out_ptr_copy), (out_ptr - out_ptr_copy) / sizeof(T));
	delete[] out_ptr_copy;
	iconv_close(cd);
	return ret;
}
#endif

#endif /*DICE_ENCODING_CONVERT*/
