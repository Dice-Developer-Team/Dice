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
#pragma once
#ifndef DICE_INIT
#define DICE_INIT
#include <string>
#include <map>
#include <utility>
#include <vector>
#include "StorageBase.h"

struct INIT
{
	std::string strNickName;
	int intValue = 0;
	INIT() = default;

	INIT(std::string strNickName, const int intValue = 0) : strNickName(std::move(strNickName)), intValue(intValue)
	{
	}

	bool operator>(const INIT second) const
	{
		return this->intValue > second.intValue;
	}
};

class Initlist : public StorageBase
{
	std::map<long long, std::vector<INIT>> mpInitlist;
public:
	void insert(long long group, int value, std::string nickname);
	void show(long long group, std::string& strMAns);
	bool clear(long long group);
	void save() const override;
	void read() override;
	Initlist(const std::string& FilePath);
	~Initlist();
};


#endif /*DICE_INIT*/
