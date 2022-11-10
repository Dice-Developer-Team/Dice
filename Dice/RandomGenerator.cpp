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
#include "RandomGenerator.h"
#include <random>
#include <chrono>

#if defined(__i386__) || defined(__x86_64__)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif
constexpr const char digit_chars[]{"0123456789"};
constexpr const char hex_chars[]{"0123456789abcdef"};
constexpr const char alpha_chars[]{"abcdefghijklmnopqrstuvwxyz"};
constexpr const char alnum_chars[]{ "abcdefghijklmnopqrstuvwxyz0123456789" };
constexpr const char base64_chars[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
constexpr const char base64url_chars[]{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

namespace RandomGenerator
{
	unsigned long long GetCycleCount()
	{
#if defined(__i386__) || defined(__x86_64__)
		return __rdtsc();
#else
		return static_cast<unsigned long long> (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
#endif	
	}
	
#if defined(__i386__) || defined(__x86_64__)
	int Randint(int lowest, int highest)
	{
		std::mt19937 gen(static_cast<unsigned int>(GetCycleCount()));
		std::uniform_int_distribution<int> dis(lowest, highest);
		return dis(gen);
	}
#else
	std::mt19937 gen(static_cast<unsigned int>(GetCycleCount()));
	int Randint(int lowest, int highest)
	{
		std::uniform_int_distribution<int> dis(lowest, highest);
		return dis(gen);
	}
#endif
	std::string genKey(size_t len, Code mode) {
		std::string res;
		std::string charset;
		switch (mode) {
		case RandomGenerator::Code::Hex:
			charset = hex_chars;
			break;
		case RandomGenerator::Code::Alpha:
			charset = alpha_chars;
			break;
		case RandomGenerator::Code::Alnum:
			charset = alnum_chars;
			break;
		case RandomGenerator::Code::Base64:
			charset = base64_chars;
			break;
		case RandomGenerator::Code::UrlBase64:
			charset = base64url_chars;
			break;
		case RandomGenerator::Code::Decimal:
		default:
			charset = digit_chars;
			break;
		}
		size_t size{ charset.length() - 1 };
		for (size_t i = 0; i < len; ++i) {
			res += charset[Randint(0, size)];
		}
		return res;
	}
}
