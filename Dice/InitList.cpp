/*
 *Dice! QQ dice robot for TRPG game
 *Copyright (C) 2018 w4123溯洄
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
#include "InitList.h"
#include <string>
#include <map>
#include <vector>
#include <algorithm>
using namespace std;


void Initlist::insert(long long group, int value, string nickname)
{
	if (!mpInitlist.count(group))
	{
		mpInitlist[group] = vector<INIT>{ INIT{ nickname,value } };
	}
	else
	{
		for(auto it = mpInitlist[group].begin();it != mpInitlist[group].end();++it)
		{
			if (it->strNickName == nickname)
			{
				it->intValue = value;
				return;
			}
		}
		mpInitlist[group].push_back(INIT{ nickname,value });
	}
}

void Initlist::show(long long group, std::string &strMAns)
{
	if (!mpInitlist.count(group)||mpInitlist[group].empty())
	{
		strMAns = "错误:请先使用.ri指令投掷先攻值";
		return;
	}
	strMAns = "先攻顺序：";
	sort(mpInitlist[group].begin(), mpInitlist[group].end(), INIT());
	int i = 1;
	for (auto it = mpInitlist[group].begin(); it != mpInitlist[group].end(); ++it)
	{
		strMAns += '\n' + to_string(i) + "." + it->strNickName + " " + to_string(it->intValue);
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
