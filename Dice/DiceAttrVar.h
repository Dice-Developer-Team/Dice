/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123ËÝä§
 * Copyright (C) 2019-2021 String.Empty
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
#include <string>
#include <unordered_map>
#include "json.hpp"
using std::string;
using json = nlohmann::json;

class AttrVar {
public:
	enum class AttrType { Nil, Boolean, Integer, Number, Text, ID };
	AttrType type{ 0 };
	union {
		bool bit;		//1
		int attr{ 0 };		//2
		double number;	//3
		string text;	//4
		//table;		//5
		//function;		//6
		long long id;		//7
	};
	AttrVar() {}
	AttrVar(const AttrVar& other);
	AttrVar(int n) :type(AttrType::Integer), attr(n) {}
	AttrVar(const string& s) :type(AttrType::Text), text(s) {}
	explicit AttrVar(long long n) :type(AttrType::ID), id(n) {}
	void des() {
		if (type == AttrType::Text)text.~string();
	}
	~AttrVar() {
		des();
	}
	explicit operator bool()const;
	AttrVar& operator=(const AttrVar& other);
	AttrVar& operator=(bool other);
	AttrVar& operator=(int other);
	AttrVar& operator=(double other);
	AttrVar& operator=(const string& other);
	AttrVar& operator=(const char* other);
	AttrVar& operator=(const long long other);
	AttrVar& operator=(const json&);
	template<typename T>
	bool operator!=(const T other)const { return !(*this == other); }
	bool operator==(const long long other)const;
	bool operator==(const string& other)const;
	int to_int()const;
	long long to_ll()const;
	double to_num()const;
	string to_str()const;
	bool str_empty()const;
	json to_json()const;

	using CMPR = bool(AttrVar::*)(double)const;
	bool is_numberic()const;
	bool equal(double)const;
	bool equal_or_more(double)const;
	bool equal_or_less(double)const;

	void writeb(std::ofstream& fout) const;
	void readb(std::ifstream& fin);
};
using AttrVars = std::unordered_map<string, AttrVar>;