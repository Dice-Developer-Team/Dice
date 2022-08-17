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
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "json.hpp"
using std::string;
using json = nlohmann::json;
template<typename T>
using dict = std::unordered_map<string, T>;
template<typename T>
using ptr = std::shared_ptr<T>;

struct ByteS {
	char* bytes{ nullptr };
	size_t len{ 0 };
	bool isUTF8{ true };
	ByteS() {}
	ByteS(const char* s, size_t l, bool b = true) :len(l), isUTF8(b) {
		bytes = new char[len + 1];
		memcpy(bytes, s, len);
	}
	ByteS(const ByteS& other) :len(other.len), isUTF8(other.isUTF8) {
		bytes = new char[len + 1];
		memcpy(bytes, other.bytes, len);
	}
	ByteS(std::ifstream& fin);
	~ByteS() { if (bytes)delete bytes; }
};

class AttrVar;
using AttrVars = dict<AttrVar>;
using VarArray = std::vector<AttrVar>;
class lua_State;
class VarTable {
	friend class AttrVar;
	friend void lua_push_attr(lua_State* L, const AttrVar& attr);
	dict<ptr<AttrVar>>dict;
	std::vector<ptr<AttrVar>>idxs;
	void init_idx();
public:
	VarTable(){}
	VarTable(const AttrVars&);
	AttrVars to_dict()const;
	VarArray to_list()const;
	std::unordered_map<string, ptr<AttrVar>>& get_dict() { return dict; };
	const std::unordered_map<string, ptr<AttrVar>>& get_dict()const { return dict; };
	std::vector<std::shared_ptr<AttrVar>>& get_list() { return idxs; };
	const std::vector<std::shared_ptr<AttrVar>>& get_list()const { return idxs; };
	bool empty()const { return dict.empty(); }
	void writeb(std::ofstream& fout) const;
	void readb(std::ifstream& fin);
};

class AttrVar {
public:
	enum class AttrType { Nil, Boolean, Integer, Number, Text, Table, Function, ID };
	AttrType type{ 0 };
	union {
		bool bit;		//1
		int attr{ 0 };		//2
		double number;	//3
		string text;	//4
		VarTable table;		//5
		ByteS chunk;		//6
		long long id;		//7
	};
	AttrVar() {}
	AttrVar(const AttrVar& other);
	AttrVar(bool b) :type(AttrType::Boolean), bit(b) {}
	AttrVar(int n) :type(AttrType::Integer), attr(n) {}
	AttrVar(double n) :type(AttrType::Number), number(n) {}
	AttrVar(const char* s) :type(AttrType::Text), text(s) {}
	AttrVar(const string& s) :type(AttrType::Text), text(s) {}
	AttrVar(const char* s,size_t len) :type(AttrType::Function), chunk(s,len) {}
	AttrVar(ByteS&& fun) :type(AttrType::Function), chunk(fun) {}
	AttrVar(long long n) :type(AttrType::ID), id(n) {}
	explicit AttrVar(const AttrVars& vars) :type(AttrType::Table), table(vars) {}
	void des() {
		if (type == AttrType::Text)text.~string();
		else if (type == AttrType::Table)table.~VarTable();
		else if (type == AttrType::Function)chunk.~ByteS();
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
	//bool operator==(const long long other)const;
	bool operator==(const string& other)const;
	bool operator==(const char* other)const;
	int to_int()const;
	long long to_ll()const;
	double to_num()const;
	string to_str()const;
	ByteS to_bytes()const;
	string print()const;
	string show()const;
	bool str_empty()const;
	VarTable to_table()const;
	AttrVars to_dict()const;
	VarArray to_list()const;
	json to_json()const;

	using CMPR = bool(AttrVar::*)(const AttrVar&)const;
	bool is_null()const { return type == AttrType::Nil; }
	bool is_boolean()const { return type == AttrType::Boolean; }
	bool is_true()const { return operator bool(); }
	bool is_numberic()const;
	bool is_character()const { return type != AttrType::Nil && type != AttrType::Boolean && type != AttrType::Table && type != AttrType::Function; }
	bool is_table()const { return type == AttrType::Table; }
	bool is_function()const { return type == AttrType::Function; }
	bool equal(const AttrVar&)const;
	bool operator==(const AttrVar& other)const { return equal(other); }
	bool not_equal(const AttrVar&)const;
	bool operator!=(const AttrVar& other)const { return not_equal(other); }
	bool more(const AttrVar&)const;
	bool operator>(const AttrVar& other)const { return more(other); }
	bool less(const AttrVar&)const;
	bool operator<(const AttrVar& other)const { return less(other); }
	bool equal_or_more(const AttrVar&)const;
	bool operator>=(const AttrVar& other)const { return equal_or_more(other); }
	bool equal_or_less(const AttrVar&)const;
	bool operator<=(const AttrVar& other)const { return equal_or_less(other); }

	void writeb(std::ofstream& fout) const;
	void readb(std::ifstream& fin);
};
json to_json(AttrVars& vars);
void from_json(const json& j, AttrVars&);
string showAttrCMPR(AttrVar::CMPR);

class AttrObject {
	std::shared_ptr<AttrVars>obj;
public:
	AttrObject():obj(std::make_shared<AttrVars>()) {}
	AttrObject(const AttrVars& vars) :obj(std::make_shared<AttrVars>(vars)) {}
	AttrObject(const AttrObject& other) :obj(other.obj) {}
	AttrVars* operator->()const {
		return obj.get();
	}
	AttrVars& operator*()const {
		return *obj;
	}
	AttrVar& operator[](const string& key)const {
		return (*obj)[key];
	}
	bool operator<(const AttrObject& other)const {
		return obj < other.obj;
	}
	bool empty()const {
		return obj->empty();
	}
	bool has(const string& key)const {
		return obj->count(key) && !obj->at(key).is_null();
	}
	void set(const string& key, const AttrVar& val)const {
		if (!val)obj->erase(key);
		else (*obj)[key] = val;
	}
	void reset(const string& key)const {
		obj->erase(key);
	}
	bool is(const string& key)const {
		return obj->count(key) ? bool(obj->at(key)) :false;
	}
	bool is_table(const string& key)const {
		return obj->count(key) && obj->at(key).is_table();
	}
	AttrVar index(const string& key)const;
	AttrVar get(const string& key, const AttrVar& val = {})const {
		return obj->count(key) ? obj->at(key) : val;
	}
	string get_str(const string& key)const {
		return obj->count(key) ? obj->at(key).to_str() : "";
	}
	int get_int(const string& key)const {
		return obj->count(key) ? obj->at(key).to_int() : 0;
	}
	long long get_ll(const string& key)const {
		return obj->count(key) ? obj->at(key).to_ll() : 0;
	}
	VarTable get_tab(const string& key)const {
		return obj->count(key) ? obj->at(key).to_table() : VarTable();
	}
	AttrVars get_dict(const string& key)const {
		return obj->count(key) ? obj->at(key).to_dict() : AttrVars();
	}
	AttrObject& merge(const AttrVars& other) {
		for (const auto& [key, val] : other) {
			(*obj)[key] = val;
		}
		return *this;
	}
	void writeb(std::ofstream&)const;
	void readb(std::ifstream&);
};
using AttrObjects = dict<AttrObject>;
using AttrIndex = AttrVar(*)(AttrObject&);
using AttrIndexs = dict<AttrIndex>;