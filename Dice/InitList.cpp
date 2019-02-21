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
#include "InitList.h"
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include "CQTools.h"
#include <functional>
using namespace std;


void Initlist::insert(long long group, int value, string nickname)
{
	if (!mpInitlist.count(group))
	{
		mpInitlist[group] = vector<INIT>{INIT(nickname, value)};
	}
	else
	{
		for (auto& it : mpInitlist[group])
		{
			if (it.strNickName == nickname)
			{
				it.intValue = value;
				return;
			}
		}
		mpInitlist[group].push_back(INIT(nickname, value));
	}
}

void Initlist::show(long long group, string& strMAns)
{
	if (!mpInitlist.count(group) || mpInitlist[group].empty())
	{
		strMAns = "错误:请先使用.ri指令投掷先攻值";
		return;
	}
	strMAns = "先攻顺序：";
	sort(mpInitlist[group].begin(), mpInitlist[group].end(), greater<>());
	int i = 1;
	for (const auto& it : mpInitlist[group])
	{
		strMAns += '\n' + to_string(i) + "." + it.strNickName + " " + to_string(it.intValue);
		++i;
	}
}

bool Initlist::clear(long long group)
{
	if (mpInitlist.count(group))
	{
		mpInitlist.erase(group);
		return true;
	}
	return false;
}

void Initlist::save() const
{
	ofstream ofINIT(FilePath, ios::out | ios::trunc);
	for (const auto& it : mpInitlist)
	{
		for (auto it1 : it.second)
		{
			ofINIT << it.first << " " << base64_encode(it1.strNickName) << " " << it1.intValue << endl;
		}
	}
	ofINIT.close();
}

void Initlist::read()
{
	ifstream ifINIT(FilePath);
	if (ifINIT)
	{
		long long Group = 0;
		int value;
		string nickname;
		while (ifINIT >> Group >> nickname >> value)
		{
			insert(Group, value, base64_decode(nickname));
		}
	}
	ifINIT.close();
}

Initlist::Initlist(const std::string& FilePath) : StorageBase(FilePath)
{
	Initlist::read();
}

Initlist::~Initlist()
{
	Initlist::save();
}
