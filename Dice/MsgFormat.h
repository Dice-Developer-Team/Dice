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
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2024 String.Empty
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
#include <map>
#include <utility>
#include <vector>
#include "STLExtern.hpp"
#include "DiceAttrVar.h"
using std::string;

typedef string(*GlobalTex)();
extern std::unordered_map<string, GlobalTex> strFuncs;

class ResList
{
	std::vector<std::string> vRes;
	unsigned int intMaxLen = 0;
	bool isLineBreak = false;
	bool isOrder = false;
	unsigned int intLineLen = 10;
	static unsigned int intPageLen;
	string sHead = "";
	string sDot = " ";
	string strLongSepa = "\n";
public:
	ResList() = default;

	ResList(const std::string& s, const  std::string& dot) : sDot(dot)
	{
		vRes.push_back(s);
		intMaxLen = s.length();
	}

	ResList& operator<<(std::string s);

	std::string show(size_t = 0)const;

	ResList& order() {
		isOrder = true;
		return *this;
	}
	ResList& head(string s) {
		sHead = s;
		return *this;
	}
	ResList& dot(const string& s)
	{
		sDot = s;
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

	ResList& setDot(const string& s, const string& sLong){
		sDot = s;
		strLongSepa = sLong;
		return *this;
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

template <typename T>
std::string showKey(T& m) {
	ResList list;
	for (auto& [key, val] : m){
		if (key.empty() || key[0] == '_')continue;
		list << key;
	}
	return list.setDot("/", "/").show();
}
template <typename T>
std::string listKey(T& m){
	ShowList list;
	for (auto& [key,val] : m)
	{
		if (key.empty() || key[0] == '_')continue;
		list << key;
	}
	return list.show("/");
}

std::string listDeck(const std::vector<std::string>& v);
std::string listDeck(const VarArray& v);

std::string to_binary(int b);
std::string strip(std::string);

#endif /*DICE_MSG_FORMAT*/
