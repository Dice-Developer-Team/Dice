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
#include "MsgFormat.h"
#include <string>

std::string format(std::string str, const std::initializer_list<const std::string>& replace_str)
{
	auto counter = 0;
	for (const auto& element : replace_str)
	{
		auto replace = "{" + std::to_string(counter) + "}";
		auto replace_pos = str.find(replace);
		while (replace_pos != std::string::npos)
		{
			str.replace(replace_pos, replace.length(), element);
			replace_pos = str.find(replace);
		}
		counter++;
	}
	return str;
}
