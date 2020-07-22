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
#include <fstream>//
#include "DiceJob.h"
#include "EncodingConvert.h"
using std::string;

std::map<string, string> GlobalChar{
	{"FormFeed","\f¡­"},
};

std::map<string, GobalTex> strFuncs{
	{"master_QQ",print_master},
	{"À©Õ¹ÅÆ¶Ñ",list_extern_deck},
	{"È«ÅÆ¶ÑÁÐ±í",list_deck},
};

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

string strip(std::string origin)
{
	while (true)
	{
		if (origin[0] == '!' || origin[0] == '.' || isspace(static_cast<unsigned char>(origin[0])))
		{
			origin.erase(origin.begin());
		}
		else if (origin.substr(0, 2) == "£¡" || origin.substr(0, 2) == "¡£" || origin.substr(0, 2) == "£®")
		{
			origin.erase(origin.begin(), origin.begin() + 2);
		}
		else return origin;
	}
}

std::string to_binary(int b)
{
	ResList res;
	for (int i = 0; i < 6; i++)
	{
		if (b & (1 << i))res << std::to_string(i);
	}
	return res.dot("+").show();
}
