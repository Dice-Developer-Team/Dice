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
#include "DiceAttrVar.h"
#include "StrExtern.hpp"
#include "DiceFile.hpp"

ByteS::ByteS(std::ifstream& fin) {
	len = fread<size_t>(fin);
	bytes = new char[len + 1];
	fin.read(bytes, len);
}
bool AnysTable::is(const string& key)const {
	return dict.count(key) ? bool(dict.at(key)) : false;
}
bool AnysTable::is_empty(const string& key)const {
	auto it{ dict.find(key) };
	return (it == dict.end())
		|| (it->second.type == AttrVar::Type::Text && it->second.text.empty())
		|| (it->second.type == AttrVar::Type::Table && it->second.table->empty());
}
bool AnysTable::is_table(const string& key)const {
	return dict.count(key) && dict.at(key).is_table();
}
bool AnysTable::empty()const {
	return dict.empty() && (!list || list->empty());
}
bool AnysTable::has(const string& key)const {
	return dict.count(key) && !dict.at(key).is_null();
}
void AnysTable::set(const string& key, const AttrVar& val){
	if (!key.empty()) {
		if (val.is_null())dict.erase(key);
		else dict[key] = val;
	}
}
void AnysTable::set(const string& key){
	if (!key.empty())dict[key] = true;
}
void AnysTable::set(int i, const AttrVar& val) {
	if (list) {
		if (list->size() == i) {
			list->emplace_back(val);
			return;
		}
		else if (list->size() > i) {
			(*list)[i] = val;
			return;
		}
	}
	else if (i == 0) {
		list = std::make_shared<VarArray>(VarArray{ val });
		return;
	}
	set(to_string(i), val);
}
void AnysTable::reset(const string& key){
	dict.erase(key);
}
vector<string> AnysTable::to_deck()const {
	vector<string> deck;
	if (list) {
		for (auto& it : *list) {
			deck.emplace_back(it.to_str());
		}
	}
	return deck;
}
AttrVar& AnysTable::at(const string& key){
	return dict[key];
}
const AttrVar& AnysTable::at(const string& key)const {
	return dict.at(key);
}
AttrVar& AnysTable::operator[](const char* key){
	return dict[key];
}
AttrVar AnysTable::get(const string& key, const AttrVar& val)const {
	return dict.count(key) ? dict.at(key)
		: val;
}
string AnysTable::get_str(const string& key)const {
	return dict.count(key) ? dict.at(key).to_str() : "";
}
string AnysTable::get_str(const string& key, const string& val)const {
	return dict.count(key) ? dict.at(key).to_str() : val;
}
string AnysTable::print(const string& key)const {
	return dict.count(key) ? dict.at(key).print() : "";
}
int AnysTable::get_int(const string& key)const {
	return dict.count(key) ? dict.at(key).to_int() : 0;
}
long long AnysTable::get_ll(const string& key)const {
	return dict.count(key) ? dict.at(key).to_ll() : 0;
}
double AnysTable::get_num(const string& key)const {
	return dict.count(key) ? dict.at(key).to_num() : 0;
}
AttrObject AnysTable::get_obj(const string& key)const {
	return dict.count(key) ? dict.at(key).to_obj() : ptr<AnysTable>();
}
std::optional<AttrVars*> AnysTable::get_dict(const string& key)const {
	return dict.count(key) ? dict.at(key).to_dict() : std::nullopt;
}
ptr<VarArray> AnysTable::get_list(const string& key)const {
	return dict.count(key) ? dict.at(key).to_list() : ptr<VarArray>();
}
AttrSet AnysTable::get_set(const string& key)const {
	return dict.count(key) ? dict.at(key).to_set() : AttrSet();
}
int AnysTable::inc(const string& key){
	if (key.empty())return 0;
	if (dict.count(key))return (++dict.at(key)).to_int();
	else dict.emplace(key, 1);
	return 1;
}
int AnysTable::inc(const string& key, int i){
	if (key.empty())return 0;
	if (dict.count(key))return (dict.at(key) += i).to_int();
	else dict.emplace(key, i);
	return 1;
}
void AnysTable::add(const string& key, const AttrVar& val){
	if (key.empty())return;
	dict[key] = dict[key] + val;
}
AnysTable& AnysTable::merge(const AttrVars& other) {
	for (const auto& [key, val] : other) {
		dict[key] = val;
	}
	return *this;
}

void AnysTable::writeb(std::ofstream& fout) const {
	AttrVars vars{ dict };
	if (list) {
		int idx{ 0 };
		for (auto& val : *list) {
			++idx;
			if (val)vars[to_string(idx)] = val.to_json();
		}
	}
	fwrite(fout, vars);
}
void AnysTable::readb(std::ifstream& fs) {
	short len = fread<short>(fs);
	if (len < 0)return;
	if (!fs.peek()) {
		fs.ignore(2);
	}
	while (len--) {
		dict[fread<string>(fs)].readb(fs);
	}
	if (string strI{ "0" }; dict.count(strI) || dict.count(strI = "1")) {
		list = std::make_shared<VarArray>();
		int idx{ strI == "0" ? 0 : 1 };
		do {
			list->push_back(dict.at(strI));
			dict.erase(strI);
		} while (dict.count(strI = to_string(++idx)));
	}
}
bool AnysTable::operator<(const AnysTable other)const { return dict < other.dict; }

AttrVar::AttrVar(const AttrVar& other) :type(other.type) {
	switch (type) {
	case Type::Boolean:
		bit = other.bit;
		break;
	case Type::Integer:
		attr = other.attr;
		break;
	case Type::Number:
		number = other.number;
		break;
	case Type::Text:
		new(&text)string(other.text);
		break;
	case Type::Table:
		new(&table) AttrObject(other.table);
		break;
	case Type::Function:
		new(&chunk)ByteS(other.chunk);
		break;
	case Type::ID:
		id = other.id;
		break;
	case Type::Set:
		new(&flags) AttrSet(other.flags);
		break;
	case Type::Nil:
	default:
		break;
	}
}
AttrVar::AttrVar(const HashedVar& vars) {
	if (std::holds_alternative<double>(vars)) {
		auto num = std::get<double>(vars);
		if ((int)num == num) {
			type = Type::Integer;
			attr = (int)num;
		}
		else if ((long long)num == num) {
			type = Type::ID;
			id = (long long)num;
		}
		else {
			type = Type::Number;
			number = num;
		}
	}
	else if (std::holds_alternative<string>(vars)) {
		type = Type::Text;
		new(&text)string(std::get<string>(vars));
	}
}
AttrVar::operator bool()const {
	return type != Type::Nil
		&& (type != Type::Boolean || bit == true);
}
AttrVar& AttrVar::operator=(const AttrVar& other) {
	this->~AttrVar();
	new(this)AttrVar(other);
	return *this;
}
AttrVar& AttrVar::operator=(bool other) {
	des();
	type = Type::Boolean;
	bit = other;
	return *this;
}
AttrVar& AttrVar::operator=(int other) {
	des();
	type = Type::Integer;
	attr = other;
	return *this;
}
AttrVar& AttrVar::operator=(double other) {
	des();
	type = Type::Number;
	number = other;
	return *this;
}
AttrVar& AttrVar::operator=(const string& other) {
	des();
	type = Type::Text;
	new(&text)string(other);
	return *this;
}
AttrVar& AttrVar::operator=(const char* other) {
	des();
	type = Type::Text;
	new(&text)string(other);
	return *this;
}
AttrVar& AttrVar::operator=(long long other) {
	des();
	type = Type::ID;
	id = other;
	return *this;
}
/*
bool AttrVar::operator==(const long long other)const {
	return (type == Type::ID && id == other)
		|| (type == Type::Integer && attr == other)
		|| (type == Type::Number && number == other);
}
*/
bool AttrVar::operator==(const string& other)const {
	return (type == Type::Text && text == other);
}
bool AttrVar::operator==(const char* other)const {
	return (type == Type::Text && text == other);
}
AttrVar& AttrVar::operator++() {
	switch (type) {
	case Type::Nil:
		type = Type::Integer;
		attr = 1;
		break;
	case Type::Boolean:
		bit = true;
		break;
	case Type::Integer:
		++attr;
		break;
	case Type::ID:
		++id;
		break;
	case Type::Number:
		++number;
		break;
	default:
		break;
	}
	return *this;
}
AttrVar AttrVar::operator+(const AttrVar& other) {
	if (type == Type::Text && other.type == Type::Text)
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
	case Type::Nil:
		return 0;
		break;
	case Type::Boolean:
		return bit;
		break;
	case Type::Integer:
		return attr;
		break;
	case Type::ID:
		return (int)id;
		break;
	case Type::Number:
		return number;
		break;
	case Type::Text:
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
	case Type::Nil:
		return 0;
		break;
	case Type::Boolean:
		return bit;
		break;
	case Type::Integer:
		return attr;
		break;
	case Type::ID:
		return id;
		break;
	case Type::Number:
		return number;
		break;
	case Type::Text:
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
	case Type::Nil:
		return 0;
		break;
	case Type::Boolean:
		return bit;
		break;
	case Type::Integer:
		return attr;
		break;
	case Type::ID:
		return id;
		break;
	case Type::Number:
		return number;
		break;
	case Type::Text:
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
	case Type::Nil:
		return {};
		break;
	case Type::Boolean:
		return to_string(bit);
		break;
	case Type::Integer:
		return to_string(attr);
		break;
	case Type::Number:
		return toString(number);
		break;
	case Type::Text:
		return text;
		break;
	case Type::Table: 
		return table->print();
		break;
	case Type::Function:
		return {};
		break;
	case Type::ID:
		return to_string(id);
		break;
	case Type::Set: {
		ShowList li;
		for (auto& elem : *flags) {
			if (std::holds_alternative<double>(elem.val)) {
				li << toString(get<double>(elem.val));
			}
			else if (std::holds_alternative<string>(elem.val)) {
				li << "'" + get<string>(elem.val) + "'";
			}
		}
		return "{" + li.show(",") + "}";
	}
		break;
	default:
		return {};
		break;
	}
}
ByteS AttrVar::to_bytes()const {
	switch (type) {
	case Type::Text:
		return { text.c_str(),text.length()};
		break;
	case Type::Function:
		return chunk;
		break;
	case Type::Nil:
		return {};
		break;
	case Type::Boolean:
	case Type::Integer:
	case Type::Number:
	case Type::ID:
	case Type::Table:
	case Type::Set:
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
			return AttrVar(true);
			break;
		case 2:
			return AttrVar(false);
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
string AnysTable::print()const {
	return UTF8toGBK(to_json().dump());
}
string AttrVar::print()const {
	switch (type) {
	case Type::Nil:
		return "null";
		break;
	case Type::Boolean:
		return bit ? "true" : "false";
		break;
	case Type::Integer:
		return to_string(attr);
		break;
	case Type::Number:
		return toString(number);
		break;
	case Type::Text:
		return text;
		break;
	case Type::Table:
		return table->print();
		break;
	case Type::ID:
		return to_string(id);
		break;
	case Type::Set: {
		ShowList li;
		for (auto& elem : *flags) {
			if (std::holds_alternative<double>(elem.val)) {
				li << toString(get<double>(elem.val));
			}
			else if (std::holds_alternative<string>(elem.val)) {
				li << get<string>(elem.val);
			}
		}
		return "{" + li.show(",") + "}";
	}
		break;
	case Type::Function:
		return "Function#" + to_string(chunk.len);
		break;
	}
	return {};
}
size_t AttrVar::len()const {
	switch (type) {
	case Type::Text:
		return text.length();
		break;
	case Type::Table:
		table->size();
		break;
	case Type::Set:
		return flags->size();
		break;
	}
	return 0;
}
bool AttrVar::str_empty()const{
	return type == Type::Text && text.empty();
}
AttrObject AttrVar::to_obj()const {
	if (type == Type::Table)return table;
	return {};
}
std::optional<AttrVars*> AttrVar::to_dict()const {
	if (type == Type::Table)return &table->as_dict(); 
	return {};
}

ptr<VarArray> AttrVar::to_list()const {
	if (type != Type::Table)return {};
	return table->to_list();
}
AttrSet AttrVar::to_set()const {
	if (type != Type::Set)return {};
	return flags;
}


bool AttrVar::is_numberic()const {
	switch (type) {
	case Type::Nil:
		return false;
		break;
	case Type::Boolean:
	case Type::Integer:
	case Type::ID:
	case Type::Number:
		return true; 
		break;
	case Type::Text:
		return isNumeric(text);
		break;
	default:
		return false;
	}
	return false;
}
bool AttrVar::equal(const AttrVar& other)const{
	if (other.type == Type::Nil) {
		return is_null();
	}
	else if (other.type == Type::Boolean) {
		return is_true() == other.is_true();
	}
	else if (other.type == Type::Text) {
		if(type == Type::Text)return text == other.text;
		else return to_str() == other.text;
	}
	else if (other.is_numberic() && is_numberic()) {
		return to_num() == other.to_num();
	}
	return false;
}
bool AttrVar::not_equal(const AttrVar& other)const {
	if (other.type == Type::Nil) {
		return !is_null();
	}
	else if (other.type == Type::Boolean) {
		return is_true() != other.is_true();
	}
	else if (other.type == Type::Text) {
		if (type == Type::Text)return text != other.text;
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
		type = Type::Nil;
		break;
	case fifo_json::value_t::object:
		type = Type::Table; {
			new(&table) AttrObject(AnysTable());
			unordered_set<string> idxs;
			if (string strI{ "0" }; j.count(strI)) {
				table->list = std::make_shared<VarArray>();
				int idx{ 0 };
				do {
					table->list->push_back(j[strI]);
					idxs.insert(strI);
				} while (j.count(strI = to_string(++idx)));
			}
			for (auto& it : j.items()) {
				if (idxs.count(it.key()))continue;
				if (!it.value().is_null())table->dict.emplace(UTF8toGBK(it.key()), it.value());
			}
		}
		break;
	case fifo_json::value_t::array:
		type = Type::Table; {
			new(&table) AttrObject(AnysTable());
			table->list = std::make_shared<VarArray>();
			for (auto& it : j) {
				table->list->push_back(it);
			}
		}
		break;
	case fifo_json::value_t::string:
		type = Type::Text;
		new(&text)string(UTF8toGBK(j.get<string>()));
		break;
	case fifo_json::value_t::boolean:
		type = Type::Boolean;
		j.get_to(bit);
		break;
	case fifo_json::value_t::number_integer:
	case fifo_json::value_t::number_unsigned:
		if (long long num{ j.get<long long>() }; num > 10000000 || num < -10000000) {
			type = Type::ID;
			id = num;
		}
		else {
			type = Type::Integer;
			j.get_to(attr);
		}
		break;
	case fifo_json::value_t::number_float:
		type = Type::Number;
		j.get_to(number);
		break;
	case fifo_json::value_t::binary:
	case fifo_json::value_t::discarded:
		break;
	}
}
fifo_json AttrVar::to_json()const {
	switch (type) {
	case Type::Nil:
		return fifo_json();
		break;
	case Type::Boolean:
		return bit;
		break;
	case Type::Integer:
		return attr;
		break;
	case Type::Number:
		return number;
		break;
	case Type::Text:
		return GBKtoUTF8(text);
		break;
	case Type::ID:
		return id;
		break;
	case Type::Table:
		return table->to_json();
		break;
	case Type::Set:
		return ::to_json(*flags);
	case Type::Function:
		return {};
	}
	return {};
}
fifo_json AnysTable::to_json()const {
	if (dict.empty() && list) {
		fifo_json j = fifo_json::array();
		for (auto& val : *list) {
			j.push_back(val ? val.to_json() : fifo_json());
		}
		return j;
	}
	else {
		fifo_json j = fifo_json::object();
		for (auto& [key, val] : dict) {
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
		type = Type::Nil;
		break;
	case toml::node_type::table:
		type = Type::Table; {
			new(&table) AttrObject(AnysTable());
			auto tab{ t.as_table() };
			unordered_set<string> idxs;
			if (string strI{ "0" }; tab->contains(strI)) {
				table->list = std::make_shared<VarArray>();
				int idx{ 0 };
				do {
					table->list->push_back(AttrVar(*(*tab)[strI].node()));
					idxs.insert(strI);
				} while (tab->contains(strI = to_string(++idx)));
			}
			for (auto& [key, val] : *tab) {
				if (string k{ UTF8toGBK(string(key.str())) };
					!idxs.count(k))
				table->dict[k] = val;
			}
		}
		break;
	case toml::node_type::array:
		type = Type::Table; {
			new(&table) AttrObject(AnysTable());
			table->list = std::make_shared<VarArray>();
			for (auto& it : *t.as_array()) {
				table->list->push_back(it);
			}
		}
		break;
	case toml::node_type::string:
		type = Type::Text;
		new(&text)string(UTF8toGBK(string(*t.as_string())));
		break;
	case toml::node_type::integer:
		if (int64_t num{ *t.as_integer() }; num == (int)num) {
			type = Type::Integer;
			attr = (int)num;
		}
		else {
			type = Type::ID;
			id = static_cast<long long>(num);
		}
		break;
	case toml::node_type::floating_point:
		type = Type::Number;
		number = double(*t.as_floating_point());
		break;
	case toml::node_type::boolean:
		type = Type::Boolean;
		bit = t.as_boolean();
		break;
	case toml::node_type::date:
	case toml::node_type::time:
	case toml::node_type::date_time:
	default:
		break; //Todo:parse time
	}
}
toml::table AnysTable::to_toml()const {
	toml::table tab;
	for (auto& [key, val] : dict) {
		if (val)switch (val.type) {
		case AttrVar::Type::Boolean:
			tab.insert(GBKtoUTF8(key), val.bit);
			break;
		case AttrVar::Type::Integer:
			tab.insert(GBKtoUTF8(key), val.attr);
			break;
		case AttrVar::Type::Number:
			tab.insert(GBKtoUTF8(key), val.number);
			break;
		case AttrVar::Type::Text:
			tab.insert(GBKtoUTF8(key), GBKtoUTF8(val.text));
			break;
		case AttrVar::Type::ID:
			tab.insert(GBKtoUTF8(key), val.id);
			break;
		case AttrVar::Type::Table:
			tab.insert(GBKtoUTF8(key), val.table->to_toml());
			break;
		case AttrVar::Type::Set:
			tab.insert(GBKtoUTF8(key), ::to_toml(*val.flags));
			break;
		default:
			break;
		}
	}
	return tab;
}
AttrVar::AttrVar(const YAML::Node& y) {
	if (y.IsScalar()) {
		if (auto as_bool = YAML::as_if<bool, std::optional<bool>>(y)();as_bool.has_value()) {
			type = Type::Boolean;
			bit = as_bool.value();
		}
		else if (auto as_int = YAML::as_if<long long, std::optional<long long>>(y)(); as_int.has_value()) {
			long long num = as_int.value();
			if (num == (int)num) {
				type = Type::Integer;
				attr = (int)num;
			}
			else {
				type = Type::ID;
				attr = num;
			}
		}
		else if (auto as_d = YAML::as_if<double, std::optional<double>>(y)(); as_d.has_value()) {
			type = Type::Number;
			number = as_d.value();
		}
		else if (auto as_s = YAML::as_if<string, std::optional<string>>(y)(); as_s.has_value()) {
			type = Type::Text;
			new(&text)string(UTF8toGBK(as_s.value()));
		}
	}
	else if (y.IsMap()) {
		type = Type::Table; {
			new(&table) AttrObject(AnysTable());
			unordered_set<string> idxs;
			string strIdx{ "0" };
			if (y[strIdx] || y[strIdx = "1"]) {
				table->list = std::make_shared<VarArray>();
				int idx{ 1 };
				do {
					table->list->push_back(y[strIdx]);
					idxs.insert(strIdx);
				} while (y[strIdx = to_string(++idx)]);
			}
			for (auto& it : y) {
				if (idxs.count(it.first.Scalar()))continue;
				table->dict.emplace(UTF8toGBK(it.first.Scalar()), it.second);
			}
		}
	}
	else if (y.IsSequence()) {
		type = Type::Table;
		VarArray list;
		for (YAML::Node it : y) {
			list.push_back(it);
		}
		new(&table) AttrObject(AnysTable(list));
	}
}
YAML::Node AttrVar::to_yaml()const {
	YAML::Node yaml;
	switch (type) {
	case Type::Boolean:
		yaml = bit;
		break;
	case Type::Integer:
		yaml = attr;
		break;
	case Type::Number:
		yaml = number;
		break;
	case Type::Text:
		yaml = GBKtoUTF8(text);
		break;
	case Type::ID:
		yaml = id;
		break;
	case Type::Table:
		for (auto& [k, v] : table->as_dict()) {
			yaml[GBKtoUTF8(k)] = v.to_yaml();
		}
		if (auto li{ table->to_list() }) {
			int i = 0;
			if (!yaml.IsMap()) {
				for (auto& v : *li) {
					yaml[i++] = v.to_yaml();
				}
			}
			else {
				for (auto& v : *li) {
					yaml[GBKtoUTF8(i++)] = v.to_yaml();
				}
			}
		}
		break;
	case Type::Set:{
		int i = 0;
		for (auto& v : *flags) {
			yaml[i++] = AttrVar(v.val).to_yaml();
		}
	}
		break;
	default:
		break;
	}
	return yaml;
}
string to_string(const AttrVar& var) {
	return var.to_str();
}
double AttrIndex::to_double()const {
	return std::holds_alternative<double>(val) ? get<double>(val) : 0;
}
string AttrIndex::to_string()const {
	return std::holds_alternative<string>(val) ? get<string>(val) : toString(get<double>(val));
}
fifo_json AttrIndex::to_json()const {
	if (std::holds_alternative<double>(val)) {
		double num{ get<double>(val) };
		if (num == (long long)num)return (long long)num;
		else return num;
	}
	else if (std::holds_alternative<string>(val)) {
		return GBKtoUTF8(get<string>(val));
	}
	return {};
}
fifo_json to_json(const fifo_set<AttrIndex>& vars) {
	fifo_json j = fifo_json::array();
	for (auto& val : vars) {
		j.emplace_back(val.to_json());
	}
	return j;
}
toml::array to_toml(const fifo_set<AttrIndex>& vars) {
	toml::array t;
	for (auto& val : vars) {
		if (std::holds_alternative<double>(val.val)) {
			t.emplace_back(std::get<double>(val.val));
		}
		else if (std::holds_alternative<string>(val.val)) {
			t.emplace_back(std::get<string>(val.val));
		}
	}
	return t;
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
	case Type::Nil:
		fwrite(fout, (char)0);
		break;
	case Type::Boolean:
		fwrite(fout, (char)1);
		fwrite(fout, bit);
		break;
	case Type::Integer:
		fwrite(fout, (char)2);
		fwrite(fout, attr);
		break;
	case Type::Number:
		fwrite(fout, (char)3);
		fwrite(fout, number);
		break;
	case Type::Text:
		fwrite(fout, (char)4);
		fwrite(fout, text);
		break;
	case Type::Table:
		fwrite(fout, (char)5);
		table->writeb(fout);
		break;
	case Type::Function:
		fwrite(fout, (char)6);
		fwrite(fout, chunk.len);
		fout.write(chunk.bytes, chunk.len);
		break;
	case Type::ID:
		fwrite(fout, (char)7);
		fwrite(fout, id);
		break;
	case Type::Set:
		fwrite(fout, (char)8);
		fwrite(fout, flags->size());
		for (auto& elem : *flags) {
			AttrVar(elem.val).writeb(fout);
		}
		break;
	}
}
void AttrVar::readb(std::ifstream& fin) {
	des();
	char tag{ fread<char>(fin) };
	switch (tag){
	case 1:
		type = Type::Boolean;
		bit = fread<bool>(fin);
		break;
	case 2:
		type = Type::Integer;
		attr = fread<int>(fin);
		break;
	case 3:
		type = Type::Number;
		number = fread<double>(fin);
		break;
	case 4:
		type = Type::Text;
		new(&text) string(fread<string>(fin));
		break;
	case 5:
		type = Type::Table;
		new(&table) AttrObject(AnysTable());
		table->readb(fin);
		break;
	case 6:
		type = Type::Function;
		new(&chunk) ByteS(fin);
		break;
	case 7:
		type = Type::ID;
		id = fread<long long>(fin);
		break;
	case 8:
		type = Type::Set;
		new(&flags) AttrSet(std::make_shared<fifo_set<AttrIndex>>());
		if (size_t cnt = fread<size_t>(fin)) {
			while (cnt--) {
				if (char tag{ fread<char>(fin) }; tag == 2) {
					flags->emplace((double)fread<int>(fin));
				}
				else if (tag == 3) {
					flags->emplace(fread<double>(fin));
				}
				else if (tag == 4) {
					flags->emplace(fread<string>(fin));
				}
				else if (tag == 7) {
					flags->emplace((double)fread<long long>(fin));
				}
			}
		}
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

AttrVar AnysTable::index(const string& key)const {
	if (dict.count(key))return dict.at(key);
	AttrVar var;
	size_t dot{ 0 };
	while ((dot = key.find('.', ++dot)) != string::npos) {
		string sub{ key.substr(0,dot) };
		if (dict.count(sub) && dict.at(sub).is_table()
			&& (var = dict.at(sub).table->index(key.substr(dot + 1)))) {
			break;
		}
	}
	return var;
}