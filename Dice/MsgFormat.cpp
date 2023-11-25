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
#include "MsgFormat.h"
#include <string>
#include <fstream>//
#include "DiceJob.h"
#include "DiceMod.h"
#include "EncodingConvert.h"
#include "StrExtern.hpp"
#include "CardDeck.h"
using std::string;

std::unordered_map<string, GobalTex> strFuncs{
	{"master_QQ",print_master},
	{"master_ID",print_master},
	{"list_extern_deck",list_extern_deck},
	{"list_all_deck",list_deck},
	{"list_reply_deck",[]() {return fmt->list_reply(2); }},
	{"list_extern_order",[]() {return fmt->list_reply(1); }},
};

string strip(std::string origin)
{
	while (true)
	{
		if (origin[0] == '!' || origin[0] == '.' || isspace(static_cast<unsigned char>(origin[0])))
		{
			origin.erase(origin.begin());
		}
		else if (origin.substr(0, 2) == "！" || origin.substr(0, 2) == "。" || origin.substr(0, 2) == "．")
		{
			origin.erase(origin.begin(), origin.begin() + 2);
		}
		else return origin;
	}
}

std::string to_binary(int b)
{
	ResList res;
	for (int i = 0; i < 14; i++)
	{
		if (b & (1 << i))res << std::to_string(i);
	}
	return res.dot("+").show();
}

unsigned int ResList::intPageLen = 512;
ResList& ResList::operator<<(std::string s) {
	while (isspace(static_cast<unsigned char>(s[0])))s.erase(s.begin());
	if (s.empty())return *this;
	if (isOrder)s = to_string(vRes.size() + 1) + ". " + s;
	vRes.push_back(s);
	if (size_t len = wstrlen(s.c_str());len > intMaxLen)intMaxLen = len;
	return *this;
}
std::string ResList::show(size_t limPage)const {
	if (empty())return {};
	std::string s, strHead, strSepa;
	unsigned int lenPage(0), cntPage(0), lenItem(0);
	if (intMaxLen > intLineLen || isLineBreak) {
		strHead = sHead + "\n";
		strSepa = strLongSepa;
	}
	else {
		strSepa = sDot;
	}
	for (auto it = vRes.begin(); it != vRes.end(); it++) {
		lenItem = wstrlen(it->c_str());
		//超过上限后分页
		if (lenPage + lenItem > intPageLen && !s.empty()) {
			if (limPage && limPage <= cntPage + 1)
				return s;
			if (cntPage++ == 0)s = "\f[第" + std::to_string(cntPage++) + "页]" + (strHead.empty() ? "\n" : "") + s;
			s += "\f[第" + std::to_string(cntPage) + "页]\n" + *it;
			lenPage = 0;
		}
		else if (it == vRes.begin())s = strHead + *it;
		else s += strSepa + *it;
		lenPage += lenItem;
	}
	return s;
}

std::string listDeck(const std::vector<std::string>& v)
{
	ResList list;
	list.setDot(" | ", "\n| ");
	for (auto& val : v) {
		list << val;
	}
	return list.show();
}
std::string listDeck(const VarArray& v)
{
	ResList list;
	list.setDot(" | ", "\n| ");
	for (auto& val : v) {
		list << val;
	}
	return list.show();
}