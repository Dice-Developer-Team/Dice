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
 * Copyright (C) 2018-2021 w4123ËÝä§
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

bool checkUTF8(const std::string& strUTF8);
bool checkUTF8(std::ifstream&);

template <typename T,
	typename = std::enable_if_t<!std::is_convertible_v<T, std::string> &&
	!std::is_convertible_v<T, std::vector<std::string>> >>
	T GBKtoUTF8(const T& TGBK) {
	return TGBK;
}
std::string GBKtoUTF8(const std::string& strGBK, bool isTrial = false);
std::vector<std::string> GBKtoUTF8(const std::vector<std::string>& strGBK);
template <typename K, typename V>
std::unordered_map<K, V> GBKtoUTF8(const std::unordered_map<K, V>& TGBK) {
	std::unordered_map<K, V> TUTF8;
	for (auto& [key, val] : TGBK) {
		TUTF8[GBKtoUTF8(key)] = GBKtoUTF8(val);
	}
	return TUTF8;
}


template <typename T,
	typename = std::enable_if_t<!std::is_convertible_v<T, std::string> &&
	!std::is_convertible_v<T, std::vector<std::string>> >>
	T UTF8toGBK(const T& TUTF8) {
	return TUTF8;
}
std::string UTF8toGBK(const std::string& strUTF8, bool isTrial = false);
std::vector<std::string> UTF8toGBK(const std::vector<std::string>& strUTF8);
template <typename K, typename V>
std::unordered_map<K, V> UTF8toGBK(const std::unordered_map<K, V>& TUTF8) {
	std::unordered_map<K, V> TGBK;
	for (auto& [key, val] : TUTF8) {
		TGBK[UTF8toGBK(key)] = UTF8toGBK(val);
	}
	return TGBK;
}


std::string UrlEncode(const std::string& str);
std::string UrlDecode(const std::string& str);
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
