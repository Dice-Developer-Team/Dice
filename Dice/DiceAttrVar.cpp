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
 * Copyright (C) 2019-2022 String.Empty
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

ByteS::ByteS(std::ifstream& fin) {
	len = fread<size_t>(fin);
	bytes = new char[len + 1];
	fin.read(bytes, len);
}

VarTable::VarTable(const AttrVars& m) {
	for (auto& [key, val] : m) {
		dict[key] = std::make_shared<AttrVar>(val);
	}
	init_idx();
}
void VarTable::init_idx() {
	int idx{ 1 };
	string strI{ to_string(idx) };
	while (dict.count(strI)) {
		idxs.push_back(dict[strI]);
		strI = to_string(++idx);
	}
}
AttrVars VarTable::to_dict()const {
	AttrVars vars;
	for (auto& [key, val] : dict) {
		if (val && !val->is_null())vars[key] = *val;
	}
	return vars;
}
VarArray VarTable::to_list()const {
	VarArray vars;
	for (auto& val : idxs) {
		vars.push_back(val ? *val : AttrVar());
	}
	return vars;
}
void VarTable::writeb(std::ofstream& fout) const {
	fwrite(fout, static_cast<int>(dict.size()));
	for (auto& [key, val] : dict) {
		fwrite(fout, key);
		val->writeb(fout);
	}
}
void VarTable::readb(std::ifstream& fin) {
	int len = fread<int>(fin);
	if (len < 0)return;
	while (len--) {
		string key{ fread<string>(fin) };
		AttrVar val;
		val.readb(fin);
		if (key.empty())continue;
		dict[key] = std::make_shared<AttrVar>(val);
	}
	init_idx();
}

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
	case AttrType::Table:
		new(&table)VarTable(other.table);
		break;
	case AttrType::Function:
		new(&chunk)ByteS(other.chunk);
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
/*
bool AttrVar::operator==(const long long other)const {
	return (type == AttrType::ID && id == other)
		|| (type == AttrType::Integer && attr == other)
		|| (type == AttrType::Number && number == other);
}
*/
bool AttrVar::operator==(const string& other)const {
	return (type == AttrType::Text && text == other);
}
bool AttrVar::operator==(const char* other)const {
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
	default:
		return 0;
		break;
	}
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
	default:
		return 0;
		break;
	}
}
double AttrVar::to_num()const {
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
			return stod(text);
		}
		catch (...) {
			return 0;
		}
		break;
	default:
		return 0;
		break;
	}
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
	case AttrType::Table: 
		return UTF8toGBK(to_json().dump());
		break;
	case AttrType::Function:
		return {};
		break;
	case AttrType::ID:
		return to_string(id);
		break;
	default:
		return {};
		break;
	}
}
ByteS AttrVar::to_bytes()const {
	switch (type) {
	case AttrType::Text:
		return { text.c_str(),text.length()};
		break;
	case AttrType::Function:
		return chunk;
		break;
	case AttrType::Nil:
		return {};
		break;
	case AttrType::Boolean:
	case AttrType::Integer:
	case AttrType::Number:
	case AttrType::ID:
	case AttrType::Table:
		return {};
		break;
	}
	return {};
}
string AttrVar::print()const {
	switch (type) {
	case AttrType::Nil:
		return "null";
		break;
	case AttrType::Boolean:
		return bit ? "true" : "false";
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
	case AttrType::Table:
		return UTF8toGBK(to_json().dump());
		break;
	case AttrType::ID:
		return to_string(id);
		break;
	case AttrType::Function:
		return "Function#" + to_string(chunk.len);
		break;
	}
	return {};
}
string AttrVar::show()const {
	switch (type) {
	case AttrType::Nil:
		return "无";
		break;
	case AttrType::Boolean:
		return bit ? "真" : "假";
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
	case AttrType::Table:
		return UTF8toGBK(to_json().dump());
		break;
	case AttrType::ID:
		return to_string(id);
		break;
	case AttrType::Function:
		return "Function#" + to_string(chunk.len);
		break;
	}
	return {};
}
bool AttrVar::str_empty()const{
	return type == AttrType::Text && text.empty();
}
VarTable AttrVar::to_table()const {
	if (type != AttrType::Table)return {};
	return table;
}
AttrVars AttrVar::to_dict()const {
	if (type != AttrType::Table)return {};
	return table.to_dict();
}

VarArray AttrVar::to_list()const {
	if (type != AttrType::Table)return {};
	return table.to_list();
}


bool AttrVar::is_numberic()const {
	switch (type) {
	case AttrType::Nil:
		return false;
		break;
	case AttrType::Boolean:
	case AttrType::Integer:
	case AttrType::ID:
	case AttrType::Number:
		return true; 
		break;
	case AttrType::Text:
		return isNumeric(text);
		break;
	default:
		return false;
	}
	return false;
}
#include "DDAPI.h"
bool AttrVar::equal(const AttrVar& other)const{
	if (other.type == AttrType::Nil) {
		return is_null();
	}
	else if (other.type == AttrType::Boolean) {
		return is_true() ^ !other;
	}
	else if (other.type == AttrType::Text) {
		if(type == AttrType::Text)return text == other.text;
		else return to_str() == other.text;
	}
	else if (other.is_numberic() && is_numberic()) {
		return to_num() == other.to_num();
	}
	return false;
}
bool AttrVar::not_equal(const AttrVar& other)const {
	if (other.type == AttrType::Nil) {
		return !is_null();
	}
	else if (other.type == AttrType::Boolean) {
		return is_true() != other.is_true();
	}
	else if (other.type == AttrType::Text) {
		if (type == AttrType::Text)return text != other.text;
		else return to_str() != other.text;
	}
	else if (other.is_numberic() && is_numberic()) {
		return to_num() != other.to_num();
	}
	return true;
}
bool AttrVar::more(const AttrVar& other)const {
	return is_numberic() && to_num() > other.to_num();
}
bool AttrVar::less(const AttrVar& other)const {
	return is_numberic() && to_num() < other.to_num();
}
bool AttrVar::equal_or_more(const AttrVar& other)const {
	return is_numberic() && to_num() >= other.to_num();
}
bool AttrVar::equal_or_less(const AttrVar& other)const {
	return is_numberic() && to_num() <= other.to_num();
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
	case json::value_t::number_unsigned:
		if (long long num{ j.get<long long>() }; num > 10000000 || num < -10000000) {
			type = AttrType::ID;
			id = num;
		}
		else {
			type = AttrType::Integer;
			j.get_to(attr);
		}
		break;
	case json::value_t::number_float:
		type = AttrType::Number;
		j.get_to(number);
		break;
	case json::value_t::string:
		type = AttrType::Text;
		new(&text)string(UTF8toGBK(j.get<string>()));
		break;
	case json::value_t::object:
		type = AttrType::Table; {
			AttrVars vars;
			for (auto it = j.cbegin(); it != j.cend(); ++it) {
				if(!it.value().is_null())vars[UTF8toGBK(it.key())] = it.value();
			}
			new(&table)VarTable(vars);
		}
		break;
	case json::value_t::array:
		type = AttrType::Table; {
			AttrVars vars;
			int idx{ 0 };
			for (auto it :j) {
				vars[to_string(++idx)] = it;
			}
			new(&table)VarTable(vars);
		}
		break;
	case json::value_t::binary:
	case json::value_t::discarded:
		return *this;
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
	case AttrType::Table: {
		if (table.dict.size() == table.idxs.size()) {
			json j = json::array();
			for (auto& val : table.idxs) {
				j.push_back(val ? val->to_json() : json());
			}
			return j;
		}
		else {
			json j = json::object();
			for (auto& [key, val] : table.dict) {
				if (val)j[GBKtoUTF8(key)] = val->to_json();
			}
			return j;
		}
	}
		break;
	case AttrType::Function:
		return {};
		break;
	}
	return {};
}
json to_json(AttrVars& vars) {
	json j;
	for (auto& [key, val] : vars) {
		if(val)j[GBKtoUTF8(key)] = val.to_json();
	}
	return j;
}
void from_json(const json& j, AttrVars& vars) {
	for (auto& [key, val] : j.items()) {
		vars[UTF8toGBK(key)] = val;
	}
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
	case AttrType::Table:
		fwrite(fout, (char)5);
		table.writeb(fout);
		break;
	case AttrType::Function:
		fwrite(fout, (char)6);
		fwrite(fout, chunk.len);
		fout.write(chunk.bytes, chunk.len);
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
	case 5:
		des();
		type = AttrType::Table;
		new(&table) VarTable();
		table.readb(fin);
		break;
	case 6:
		des();
		type = AttrType::Function;
		new(&chunk) ByteS(fin);
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

string showAttrCMPR(AttrVar::CMPR cmpr) {
	if (cmpr == &AttrVar::equal) {
		return "等于";
	}
	else if (cmpr == &AttrVar::equal_or_more) {
		return "不低于";
	}
	else if(cmpr == &AttrVar::equal_or_less) {
		return "不高于";
	}
	else if(cmpr == &AttrVar::more) {
		return "高于";
	}
	else if (cmpr == &AttrVar::less) {
		return "低于";
	}
	return {};
}

void AttrObject::writeb(std::ofstream& fout)const { fwrite(fout, *obj); }
void AttrObject::readb(std::ifstream& fs) {
	fread(fs, *obj);
}