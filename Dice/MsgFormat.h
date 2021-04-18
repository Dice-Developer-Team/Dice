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

#ifndef DICE_MSG_FORMAT
#define DICE_MSG_FORMAT
#include <string>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>
#include "STLExtern.hpp"
using std::string;

extern std::map<string, string> GlobalChar;
typedef string(*GobalTex)();
extern std::map<string, GobalTex> strFuncs;

std::string format(std::string str, const std::initializer_list<const std::string>& replace_str);

std::string format(std::string s, const std::map<std::string, std::string, less_ci>& replace_str,
				   const std::unordered_map<std::string, std::string>& str_tmp = {});

class ResList
{
	std::vector<std::string> vRes;
	unsigned int intMaxLen = 0;
	bool isLineBreak = false;
	unsigned int intLineLen = 10;
	static unsigned int intPageLen;
	string sHead = "";
	string sDot = " ";
	string strLongSepa = "\n";
public:
	ResList() = default;

	ResList(const std::string& s, std::string dot) : sDot(std::move(dot))
	{
		vRes.push_back(s);
		intMaxLen = s.length();
	}

	ResList& operator<<(std::string s);

	std::string show(size_t = 0)const;

	ResList& head(string s) {
		sHead = s;
		return *this;
	}
	ResList& dot(string s)
	{
		sDot = std::move(s);
		return *this;
	}

	ResList& linebreak()
	{
		isLineBreak = true;
		return *this;
	}

	ResList& line(unsigned int l)
	{
		intLineLen = l;
		return *this;
	}

	void setDot(string s, string sLong)
	{
		sDot = std::move(s);
		strLongSepa = std::move(sLong);
	}

	[[nodiscard]] bool empty() const
	{
		return vRes.empty();
	}

	[[nodiscard]] size_t size() const
	{
		return vRes.size();
	}
};

//按属性名输出项目
class AttrList {
	std::unordered_map<string, string> mItem;
	std::vector<string> vKey;
public:
	string show() {
		string res;
		int index = 0;
		for (auto& key : vKey) {
			res += "\n" + key + "=" + mItem[key];
		}
		return res;
	}
};

template <typename T, typename sort>
std::string listKey(std::map<std::string, T, sort> m)
{
	ResList list;
	list.setDot("/", "/");
	for (auto& [key,val] : m)
	{
		if (key[0] == '_')continue;
		list << key;
	}
	return list.show();
}

std::string to_binary(int b);
std::string strip(std::string);
#endif /*DICE_MSG_FORMAT*/
