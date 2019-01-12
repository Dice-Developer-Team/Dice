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
#include "json.hpp"
#include "GlobalVar.h"
#include "EncodingConvert.h"
#include <fstream>
#include <sstream>
#include "RDConstant.h"

void ReadCustomMsg(std::ifstream& in)
{
	std::stringstream buffer;
	buffer << in.rdbuf();
	nlohmann::json customMsg = nlohmann::json::parse(buffer.str());
	for (nlohmann::json::iterator it = customMsg.begin(); it != customMsg.end(); ++it) {
		if(GlobalMsg.count(it.key()))
		{
			if (it.key() != "strHlpMsg")
			{
				GlobalMsg[it.key()] = UTF8toGBK(it.value().get<std::string>());
			}
			else
			{
				GlobalMsg[it.key()] = Dice_Short_Ver + "\n" + UTF8toGBK(it.value().get<std::string>());
			}
		}
	}
}
