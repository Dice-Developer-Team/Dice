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
#ifndef DICE_MSG_FORMAT
#define DICE_MSG_FORMAT
#include <string>
#include <map>
#include <vector>
using std::string;
std::string format(std::string str, const std::initializer_list<const std::string>& replace_str);
template<typename sort>
std::string format(std::string s, const std::map<std::string, std::string, sort>& replace_str, std::map<std::string, std::string> str_tmp = {}) {
	if (s[0] == '&') {
		string key = s.substr(1);
		auto it = replace_str.find(key);
		if (it != replace_str.end()) {
			return format(it->second, replace_str, str_tmp);
		}
		else if ((it = str_tmp.find(key)) != str_tmp.end()) {
			return it->second;
		}
	}
	int l = 0, r = 0;
	int len = s.length();
	while ((l = s.find('{', r)) != string::npos && (r = s.find('}', l)) != string::npos) {
		if (l - 1 >= 0 && s[l - 1] == 0x5c) {
			s.replace(l - 1, 1, "");
			continue;
		}
		string key = s.substr(l + 1, r - l - 1);
		auto it = replace_str.find(key);
		if (it != replace_str.end()) {
			s.replace(l, r - l + 1, format(it->second, replace_str, str_tmp));
			r += s.length() - len + 1;
			len = s.length();
		}
		else if ((it = str_tmp.find(key)) != str_tmp.end()) {
			if (key == "res")s.replace(l, r - l + 1, format(it->second, replace_str, str_tmp));
			else s.replace(l, r - l + 1, it->second);
			r += s.length() - len + 1;
			len = s.length();
		}
	}
	return s;
}

class ResList {
	std::vector<std::string> vRes;
	unsigned int intMaxLen = 0;
	bool isLineBreak = false;
	unsigned int intLineLen = 16;
	string sDot = " ";
	string strLongSepa = "\n";
public:
	ResList() = default;
	ResList(std::string s, std::string dot) :sDot(dot) {
		vRes.push_back(s);
		intMaxLen = s.length();
	}
	ResList& operator<<(std::string s) {
		while (isspace(static_cast<unsigned char>(s[0])))s.erase(s.begin());
		if (s.empty())return *this;
		vRes.push_back(s);
		if (s.length() > intMaxLen)intMaxLen = s.length();
		return *this;
	}
	std::string show() {
		std::string s;
		if (intMaxLen > intLineLen || isLineBreak) {
			for (auto it : vRes) {
				for (auto it = vRes.begin(); it != vRes.end(); it++) {
					if (it == vRes.begin())s = "\n" + *it;
					else s += strLongSepa + *it;
				}
			}
		}
		else {
			for (auto it = vRes.begin(); it != vRes.end(); it++) {
				if (it == vRes.begin())s = *it;
				else s += sDot + *it;
			}
		}
		return s;
	}
	ResList& dot(string s) {
		sDot = s;
		return *this;
	}
	ResList& linebreak() {
		isLineBreak = true;
		return *this;
	}
	ResList& line(unsigned int l) {
		intLineLen = l;
		return *this;
	}
	void setDot(string s,string sLong) {
		sDot = s;
		strLongSepa = sLong;
	}
	bool empty()const {
		return vRes.empty();
	}
	size_t size()const {
		return vRes.size();
	}
};

template<typename T,typename sort>
std::string listKey(std::map<std::string, T, sort>m) {
	ResList list;
	list.setDot("/", "/");
	for (auto &[key,val] : m) {
		if (key[0] == '_')continue;
		list << key;
	}
	return list.show();
}

std::string to_binary(int b);
std::string strip(std::string);
#endif /*DICE_MSG_FORMAT*/
