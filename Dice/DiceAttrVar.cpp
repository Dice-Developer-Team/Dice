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
#include "DiceAttrVar.h"
#include "StrExtern.hpp"
#include "DiceFile.hpp"
using std::to_string;

AttrVar::AttrVar(const AttrVar& other) :type(other.type) {
	switch (type) {
	case AttrType::Boolean:
		bit = other.bit;
		break;
	case AttrType::Integer:
		attr = other.attr;
		break;
	case AttrType::Number:
		number = other.number;
		break;
	case AttrType::Text:
		new(&text)string(other.text);
		break;
	case AttrType::ID:
		id = other.id;
		break;
	case AttrType::Nil:
	default:
		break;
	}
}
AttrVar::operator bool()const {
	return type != AttrType::Nil
		&& (type != AttrType::Boolean || bit == true);
}
AttrVar& AttrVar::operator=(const AttrVar& other) {
	this->~AttrVar();
	new(this)AttrVar(other);
	return *this;
}
AttrVar& AttrVar::operator=(bool other) {
	des();
	type = AttrType::Boolean;
	bit = other;
	return *this;
}
AttrVar& AttrVar::operator=(int other) {
	des();
	type = AttrType::Integer;
	attr = other;
	return *this;
}
AttrVar& AttrVar::operator=(double other) {
	des();
	type = AttrType::Number;
	number = other;
	return *this;
}
AttrVar& AttrVar::operator=(const string& other) {
	des();
	type = AttrType::Text;
	new(&text)string(other);
	return *this;
}
AttrVar& AttrVar::operator=(const char* other) {
	des();
	type = AttrType::Text;
	new(&text)string(other);
	return *this;
}
AttrVar& AttrVar::operator=(long long other) {
	des();
	type = AttrType::ID;
	id = other;
	return *this;
}
bool AttrVar::operator==(const long long other)const {
	return (type == AttrType::ID && id == other)
		|| (type == AttrType::Integer && attr == other)
		|| (type == AttrType::Number && number == other);
}
bool AttrVar::operator==(const string& other)const {
	return (type == AttrType::Text && text == other);
}

int AttrVar::to_int()const {
	switch (type) {
	case AttrType::Nil:
		return 0;
		break;
	case AttrType::Boolean:
		return bit;
		break;
	case AttrType::Integer:
		return attr;
		break;
	case AttrType::ID:
		return (int)id;
		break;
	case AttrType::Number:
		return number;
		break;
	case AttrType::Text:
		try {
			return stoi(text);
		}
		catch (...) {
			return 0;
		}
		break;
	}
	return 0;
}
long long AttrVar::to_ll()const {
	switch (type) {
	case AttrType::Nil:
		return 0;
		break;
	case AttrType::Boolean:
		return bit;
		break;
	case AttrType::Integer:
		return attr;
		break;
	case AttrType::ID:
		return id;
		break;
	case AttrType::Number:
		return number;
		break;
	case AttrType::Text:
		try {
			return stoll(text);
		}
		catch (...) {
			return 0;
		}
		break;
	}
	return 0;
}
string AttrVar::to_str()const {
	switch (type) {
	case AttrType::Nil:
		return {};
		break;
	case AttrType::Boolean:
		return to_string(bit);
		break;
	case AttrType::Integer:
		return to_string(attr);
		break;
	case AttrType::Number:
		return toString(number);
		break;
	case AttrType::Text:
		return text;
		break;
	case AttrType::ID:
		return to_string(id);
		break;
	}
	return {};
}
bool AttrVar::str_empty()const{
	return type == AttrType::Text && text.empty();
}

AttrVar& AttrVar::operator=(const json& j) {
	des();
	switch (j.type()) {
	case json::value_t::null:
		type = AttrType::Nil;
		break;
	case json::value_t::boolean:
		type = AttrType::Boolean;
		j.get_to(bit);
		break;
	case json::value_t::number_integer:
		type = AttrType::Integer;
		j.get_to(attr);
		break;
	case json::value_t::number_float:
		type = AttrType::Number;
		j.get_to(number);
		break;
	case json::value_t::string:
		type = AttrType::Text;
		new(&text)string(UTF8toGBK(j.get<string>()));
		break;
	case json::value_t::number_unsigned:
		type = AttrType::ID;
		j.get_to(id);
		break;
	}
	return *this;
}
json AttrVar::to_json()const {
	switch (type) {
	case AttrType::Nil:
		return nlohmann::json();
		break;
	case AttrType::Boolean:
		return bit;
		break;
	case AttrType::Integer:
		return attr;
		break;
	case AttrType::Number:
		return number;
		break;
	case AttrType::Text:
		return GBKtoUTF8(text);
		break;
	case AttrType::ID:
		return id;
		break;
	}
	return {};
}

void AttrVar::writeb(std::ofstream& fout) const {
	switch (type) {
	case AttrType::Nil:
		fwrite(fout, (char)0);
		break;
	case AttrType::Boolean:
		fwrite(fout, (char)1);
		fwrite(fout, bit);
		break;
	case AttrType::Integer:
		fwrite(fout, (char)2);
		fwrite(fout, attr);
		break;
	case AttrType::Number:
		fwrite(fout, (char)3);
		fwrite(fout, number);
		break;
	case AttrType::Text:
		fwrite(fout, (char)4);
		fwrite(fout, text);
		break;
	case AttrType::ID:
		fwrite(fout, (char)7);
		fwrite(fout, id);
		break;
	}
}
void AttrVar::readb(std::ifstream& fin) {
	char tag{ fread<char>(fin) };
	switch (tag){
	case 1:
		des();
		type = AttrType::Boolean;
		bit = fread<bool>(fin);
		break;
	case 2:
		des();
		type = AttrType::Integer;
		attr = fread<int>(fin);
		break;
	case 3:
		des();
		type = AttrType::Number;
		number = fread<double>(fin);
		break;
	case 4:
		des();
		type = AttrType::Text;
		new(&text) string(fread<string>(fin));
		break;
	case 7:
		des();
		type = AttrType::ID;
		id = fread<long long>(fin);
		break;
	case 0:
	default:
		break;
	}
}