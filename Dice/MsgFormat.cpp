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
#include <map>
#include <sstream>

std::string format(const std::string& str, const std::map<const std::string, const std::string>& args)
{
	std::stringstream buffer;
	size_t index_first = str.find('{');
	size_t index_second = str.find('}', index_first + 1);
	size_t first_index_not_processed = 0;
	std::map<const std::string, const std::string>::const_iterator it;
	while (index_first != std::string::npos && index_second != std::string::npos)
	{
		if ((it = args.find(str.substr(index_first + 1, index_second - index_first - 1))) != args.cend())
		{
			buffer << str.substr(first_index_not_processed, index_first - first_index_not_processed);
			buffer << it->second;
		}
		else
		{
			buffer << str.substr(first_index_not_processed, index_second - first_index_not_processed + 1);
		}
		first_index_not_processed = index_second + 1;
		index_first = str.find('{', index_second + 1);
		index_second = str.find('}', index_first + 1);
	}
	buffer << str.substr(first_index_not_processed);
	return buffer.str();
}
