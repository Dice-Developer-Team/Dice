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
bool AttrObject::is(const string& key)const {
	return dict->count(key) ? bool(dict->at(key)) : false;
}
bool AttrObject::is_empty(const string& key)const {
	auto it{ dict->find(key) };
	return (it == dict->end())
		|| (it->second.type == AttrVar::AttrType::Text && it->second.text.empty())
		|| (it->second.type == AttrVar::AttrType::Table && it->second.table.empty());
}
bool AttrObject::is_table(const string& key)const {
	return dict->count(key) && dict->at(key).is_table();
}
bool AttrObject::empty()const {
	return dict->empty() && (!list || list->empty());
}
bool AttrObject::has(const string& key)const {
	return dict->count(key) && !dict->at(key).is_null();
}
void AttrObject::set(const string& key, const AttrVar& val)const {
	if (key.empty())return;
	if (val.is_null())dict->erase(key);
	else (*dict)[key] = val;
}
void AttrObject::set(const string& key)const {
	if (key.empty())return;
	(*dict)[key] = true;
}
void AttrObject::reset(const string& key)const {
	dict->erase(key);
}
AttrVar& AttrObject::at(const string& key)const {
	return (*dict)[key];
}
AttrVar& AttrObject::operator[](const char* key)const {
	return (*dict)[key];
}
AttrVar AttrObject::get(const string& key, ptr<AttrVar> val)const {
	return dict->count(key) ? dict->at(key)
		: val ? *val : AttrVar();
}
string AttrObject::get_str(const string& key)const {
	return dict->count(key) ? dict->at(key).to_str() : "";
}
string AttrObject::get_str(const string& key, const string& val)const {
	return dict->count(key) ? dict->at(key).to_str() : val;
}
string AttrObject::print(const string& key)const {
	return dict->count(key) ? dict->at(key).print() : "";
}
int AttrObject::get_int(const string& key)const {
	return dict->count(key) ? dict->at(key).to_int() : 0;
}
long long AttrObject::get_ll(const string& key)const {
	return dict->count(key) ? dict->at(key).to_ll() : 0;
}
double AttrObject::get_num(const string& key)const {
	return dict->count(key) ? dict->at(key).to_num() : 0;
}
AttrObject AttrObject::get_obj(const string& key)const {
	return dict->count(key) ? dict->at(key).to_obj() : AttrObject();
}
ptr<AttrVars> AttrObject::get_dict(const string& key)const {
	return dict->count(key) ? dict->at(key).to_dict() : ptr<AttrVars>();
}
ptr<VarArray> AttrObject::get_list(const string& key)const {
	return dict->count(key) ? dict->at(key).to_list() : ptr<VarArray>();
}
int AttrObject::inc(const string& key)const {
	if (key.empty())return 0;
	if (dict->count(key))return (++dict->at(key)).to_int();
	else dict->emplace(key, 1);
	return 1;
}
void AttrObject::add(const string& key, const AttrVar& val)const {
	if (key.empty())return;
	(*dict)[key] = (*dict)[key] + val;
}
AttrObject& AttrObject::merge(const AttrVars& other) {
	for (const auto& [key, val] : other) {
		(*dict)[key] = val;
	}
	return *this;
}

void AttrObject::writeb(std::ofstream& fout) const {
	AttrVars vars{ *dict };
	if (list) {
		int idx{ 0 };
		for (auto& val : *list) {
			++idx;
			if (val)vars[to_string(idx)] = val.to_json();
		}
	}
	fwrite(fout, vars);
}
void AttrObject::readb(std::ifstream& fs) {
	short len = fread<short>(fs);
	if (len < 0)return;
	if (!fs.peek()) {
		fs.ignore(2);
	}
	while (len--) {
		(*dict)[fread<string>(fs)].readb(fs);
	}
	if (string strI{ "1" }; dict->count(strI)) {
		list = std::make_shared<VarArray>();
		int idx{ 1 };
		do {
			list->push_back(dict->at(strI));
			dict->erase(strI);
		} while (dict->count(strI = to_string(++idx)));
	}
}
bool AttrObject::operator<(const AttrObject other)const { return dict < other.dict; }

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
		new(&table)AttrObject(other.table);
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
AttrVar& AttrVar::operator++() {
	switch (type) {
	case AttrType::Nil:
		type = AttrType::Integer;
		attr = 1;
		break;
	case AttrType::Boolean:
		bit = true;
		break;
	case AttrType::Integer:
		++attr;
		break;
	case AttrType::ID:
		++id;
		break;
	case AttrType::Number:
		++number;
		break;
	default:
		break;
	}
	return *this;
}
AttrVar AttrVar::operator+(const AttrVar& other) {
	if (type == AttrType::Text && other.type == AttrType::Text)
		return text + other.text;
	else if (is_numberic() && other.is_numberic()) {
		double sum{ to_num() + other.to_num() };
		return (sum == (int)sum) ? (int)sum :
			((sum == (long long)sum) ? (long long)sum : sum);
	}
	return other ? AttrVar() : *this;
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
enumap<string> reserved{ "null","true","false" };
AttrVar AttrVar::parse(const string& s) {
	if (reserved.count(s)) {
		switch (reserved[s]){
		case 0:
			return AttrVar();
			break;
		case 1:
			return true;
			break;
		case 2:
			return false;
			break;
		default:
			break;
		}
	}
	if (s.find_first_of("\"[{") == 0) {
		try {
			return fifo_json::parse(s);
		}catch(...){}
	}
	if (isNumeric(s)) {
		if (s.find('.') != string::npos)return stod(s);
		else if (s.length() < 10)return stoi(s);
		else return stoll(s);
	}
	return s;
}
AttrVar AttrVar::parse_toml(std::ifstream& s) {
	try	{
		return toml::parse(s);
	}
	catch (const toml::parse_error& err){
		return {};
	}
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
bool AttrVar::str_empty()const{
	return type == AttrType::Text && text.empty();
}
AttrObject AttrVar::to_obj()const {
	if (type != AttrType::Table)return {};
	return table;
}
std::shared_ptr<AttrVars> AttrVar::to_dict()const {
	if (type != AttrType::Table)return {};
	return table.to_dict();
}

std::shared_ptr<VarArray> AttrVar::to_list()const {
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
bool AttrVar::equal(const AttrVar& other)const{
	if (other.type == AttrType::Nil) {
		return is_null();
	}
	else if (other.type == AttrType::Boolean) {
		return is_true() == other.is_true();
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

AttrVar::AttrVar(const fifo_json& j) {
	switch (j.type()) {
	case fifo_json::value_t::null:
		type = AttrType::Nil;
		break;
	case fifo_json::value_t::object:
		type = AttrType::Table; {
			new(&table)AttrObject();
			unordered_set<string> idxs;
			if (j.count("1")) {
				table.list = std::make_shared<VarArray>();
				int idx{ 1 };
				string strI{ "1" };
				do {
					table.list->push_back(j[strI]);
					idxs.insert(strI);
				} while (j.count(strI = to_string(++idx)));
			}
			for (auto& it : j.items()) {
				if (idxs.count(it.key()))continue;
				if (!it.value().is_null())table.dict->emplace(UTF8toGBK(it.key()), it.value());
			}
		}
		break;
	case fifo_json::value_t::array:
		type = AttrType::Table; {
			new(&table)AttrObject();
			table.list = std::make_shared<VarArray>();
			for (auto& it : j) {
				table.list->push_back(it);
			}
		}
		break;
	case fifo_json::value_t::string:
		type = AttrType::Text;
		new(&text)string(UTF8toGBK(j.get<string>()));
		break;
	case fifo_json::value_t::boolean:
		type = AttrType::Boolean;
		j.get_to(bit);
		break;
	case fifo_json::value_t::number_integer:
	case fifo_json::value_t::number_unsigned:
		if (long long num{ j.get<long long>() }; num > 10000000 || num < -10000000) {
			type = AttrType::ID;
			id = num;
		}
		else {
			type = AttrType::Integer;
			j.get_to(attr);
		}
		break;
	case fifo_json::value_t::number_float:
		type = AttrType::Number;
		j.get_to(number);
		break;
	case fifo_json::value_t::binary:
	case fifo_json::value_t::discarded:
		break;
	}
}
fifo_json AttrVar::to_json()const {
	switch (type) {
	case AttrType::Nil:
		return fifo_json();
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
	case AttrType::Table:
		return table.to_json();
		break;
	case AttrType::Function:
		return {};
	}
	return {};
}
fifo_json AttrObject::to_json()const {
	if (dict->empty() && list) {
		fifo_json j = fifo_json::array();
		for (auto& val : *list) {
			j.push_back(val ? val.to_json() : fifo_json());
		}
		return j;
	}
	else {
		fifo_json j = fifo_json::object();
		for (auto& [key, val] : *dict) {
			if (val)j[GBKtoUTF8(key)] = val.to_json();
		}
		if (list) {
			int idx{ 0 };
			for (auto& val : *list) {
				++idx;
				if (val)j[to_string(idx)] = val.to_json();
			}
		}
		return j;
	}
}
AttrVar::AttrVar(const toml::node& t) {
	switch (t.type()){
	case toml::node_type::none:
		type = AttrType::Nil;
		break;
	case toml::node_type::table:
		type = AttrType::Table; {
			new(&table)AttrObject();
			auto tab{ t.as_table() };
			unordered_set<string> idxs;
			if (tab->contains("1")) {
				table.list = std::make_shared<VarArray>();
				int idx{ 1 };
				string strI{ "1" };
				do {
					table.list->push_back(AttrVar(*(*tab)[strI].node()));
					idxs.insert(strI);
				} while (tab->contains(strI = to_string(++idx)));
			}
			for (auto& [key,val] : *tab) {
				if (idxs.count(string(key.str())))continue;
				table.dict->emplace(UTF8toGBK(string(key.str())), val);
			}
		}
		break;
	case toml::node_type::array:
		type = AttrType::Table; {
			new(&table)AttrObject();
			table.list = std::make_shared<VarArray>();
			for (auto& it : *t.as_array()) {
				table.list->push_back(it);
			}
		}
		break;
	case toml::node_type::string:
		type = AttrType::Text;
		new(&text)string(UTF8toGBK(string(*t.as_string())));
		break;
	case toml::node_type::integer:
		if (int64_t num{ *t.as_integer() }; num > 10000000 || num < -10000000) {
			type = AttrType::ID;
			id = static_cast<long long>(num);
		}
		else {
			type = AttrType::Integer;
			attr = static_cast<long long>(num);
		}
		break;
	case toml::node_type::floating_point:
		type = AttrType::Number;
		number = double(*t.as_floating_point());
		break;
	case toml::node_type::boolean:
		type = AttrType::Boolean;
		bit = t.as_boolean();
		break;
	case toml::node_type::date:
	case toml::node_type::time:
	case toml::node_type::date_time:
	default:
		break; //Todo:parse time
	}
}
toml::table AttrObject::to_toml()const {
	toml::table tab;
	for (auto& [key, val] : *dict) {
		if (val)switch (val.type) {
		case AttrVar::AttrType::Boolean:
			tab.insert(GBKtoUTF8(key), val.bit);
			break;
		case AttrVar::AttrType::Integer:
			tab.insert(GBKtoUTF8(key), val.attr);
			break;
		case AttrVar::AttrType::Number:
			tab.insert(GBKtoUTF8(key), val.number);
			break;
		case AttrVar::AttrType::Text:
			tab.insert(GBKtoUTF8(key), GBKtoUTF8(val.text));
			break;
		case AttrVar::AttrType::ID:
			tab.insert(GBKtoUTF8(key), val.id);
			break;
		case AttrVar::AttrType::Table:
			tab.insert(GBKtoUTF8(key), val.table.to_toml());
			break;
		default:
			break;
		}
	}
	return tab;
}
string to_string(const AttrVar& var) {
	return var.to_str();
}
fifo_json to_json(const AttrVars& vars) {
	fifo_json j;
	for (auto& [key, val] : vars) {
		j[GBKtoUTF8(key)] = val.to_json();
	}
	return j;
}
void from_json(const fifo_json& j, AttrVars& vars) {
	for (auto& [key, val] : j.items()) {
		vars[UTF8toGBK(key)] = AttrVar(val);
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
		new(&table) AttrObject();
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
	else if (cmpr == &AttrVar::not_equal) {
		return "不等于";
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

AttrVar AttrObject::index(const string& key)const {
	if (dict->count(key))return dict->at(key);
	AttrVar var;
	size_t dot{ 0 };
	while ((dot = key.find('.', ++dot)) != string::npos) {
		string sub{ key.substr(0,dot) };
		if (dict->count(sub) && dict->at(sub).is_table()
			&& (var = dict->at(sub).table.index(key.substr(dot + 1)))) {
			break;
		}
	}
	return var;
}