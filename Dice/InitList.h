/*
 *Dice! QQ dice robot for TRPG game
 *Copyright (C) 2018 w4123ËÝä§
 *
 *This program is free software: you can redistribute it and/or modify it under the terms
 *of the GNU Affero General Public License as published by the Free Software Foundation,
 *either version 3 of the License, or (at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU Affero General Public License for more details.
 *
 *You should have received a copy of the GNU Affero General Public License along with this
 *program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef __INIT__
#define __INIT__
#include <string>
#include <map>
#include <vector>

struct INIT {
	std::string strNickName;
	int intValue;
	bool operator()(INIT first, INIT second) const {
		return first.intValue>second.intValue;
	}
};

class Initlist
{
	std::map<long long, std::vector<INIT>> mpInitlist;
public:
	void insert(long long group, int value, std::string nickname);
	void show(long long group, std::string &strMAns);
	bool clear(long long group);
};


#endif /*__INIT__*/