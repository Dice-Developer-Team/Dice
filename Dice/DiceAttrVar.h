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
#include <string>
using std::string;

class AttrVar {
public:
	enum class AttrType { Nil, Boolean, Integer, Number, Text };
	AttrType type{ 0 };
	union {
		bool bit;		//1
		int attr{ 0 };		//2
		double number;	//3
		string text;	//4
		//table;		//5
		//function;		//6
	};
	AttrVar() {}
	AttrVar(const AttrVar& other);
	AttrVar(int n) :type(AttrType::Integer), attr(n) {}
	AttrVar(const string& s) :type(AttrType::Text), text(s) {}
	void des() {
		if (type == AttrType::Text)text.~string();
	}
	~AttrVar() {
		des();
	}
	AttrVar& operator=(const AttrVar& other);
	AttrVar& operator=(bool other);
	AttrVar& operator=(int other);
	AttrVar& operator=(double other);
	AttrVar& operator=(const string& other);
	int to_int()const;
	string to_str()const;
	void writeb(std::ofstream& fout) const;
	void readb(std::ifstream& fin);
};
