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
#include "fifo_json.hpp"
#include "toml.hpp"
using std::string;
template<typename T>
using ptr = std::shared_ptr<T>;
template<typename T>
using fifo_dict = nlohmann::fifo_map<std::string, T>;

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
using AttrVars = fifo_dict<AttrVar>;
using VarArray = std::vector<AttrVar>;
class lua_State;
class AttrObject {
protected:
	ptr<AttrVars>dict;
	ptr<VarArray>list;
	friend class AttrVar;
	friend AttrVar lua_to_attr(lua_State*, int);
	friend void lua_push_attr(lua_State*, const AttrVar&);
public:
	AttrObject() :dict(std::make_shared<AttrVars>()) {}
	AttrObject(const AttrVars& vars) :dict(std::make_shared<AttrVars>(vars)) {}
	//explicit AttrObject(const VarArray& vars) :dict(std::make_shared<AttrVars>()), list(std::make_shared<VarArray>(vars)) {}
	AttrObject(const AttrObject& other) :dict(other.dict), list(other.list) {}
	const ptr<AttrVars>& to_dict()const { return dict; }
	const ptr<VarArray>& to_list()const { return list; }
	AttrVars* operator->()const {
		return dict.get();
	}
	AttrVar& at(const string& key)const;
	AttrVar& operator[](const char* key)const;
	bool operator<(const AttrObject other)const;
	//bool operator<(const AttrObject& other)const { return dict < other.dict; }
	bool empty()const;
	bool has(const string& key)const;
	void set(const string& key, const AttrVar& val)const;
	void set(const string& key)const;
	void reset(const string& key)const;
	bool is(const string& key)const;
	bool is_empty(const string& key)const;
	bool is_table(const string& key)const;
	AttrVar index(const string& key)const;
	AttrVar get(const string& key, ptr<AttrVar> val = {})const;
	string get_str(const string& key)const;
	string get_str(const string& key, const string& val)const;
	string print(const string& key)const;
	int get_int(const string& key)const;
	long long get_ll(const string& key)const;
	double get_num(const string& key)const;
	AttrObject get_obj(const string& key)const;
	ptr<AttrVars> get_dict(const string& key)const;
	ptr<VarArray> get_list(const string& key)const;
	int inc(const string& key)const;
	void add(const string& key, const AttrVar&)const;
	AttrObject& merge(const AttrVars& other);
	fifo_json to_json()const;
	toml::table to_toml()const;
	void writeb(std::ofstream&)const;
	void readb(std::ifstream&);
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
		AttrObject table;		//5
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
	AttrVar(const fifo_json&);
	AttrVar(const toml::node&);
	AttrVar(const AttrObject& vars) :type(AttrType::Table), table(vars) {}
	explicit AttrVar(const AttrVars& vars) :type(AttrType::Table), table(vars) {}
	void des() {
		if (type == AttrType::Text)text.~string();
		else if (type == AttrType::Table)table.~AttrObject();
		else if (type == AttrType::Function)chunk.~ByteS();
		type = AttrType::Nil;
	}
	~AttrVar() {
		des();
	}
	explicit operator bool()const;
	operator string() const { return to_str(); }
	AttrVar& operator=(const AttrVar& other);
	AttrVar& operator=(bool other);
	AttrVar& operator=(int other);
	AttrVar& operator=(double other);
	AttrVar& operator=(const string& other);
	AttrVar& operator=(const char* other);
	AttrVar& operator=(const long long other);
	AttrVar& operator=(const fifo_json& other) { new(this)AttrVar(other); return *this; };
	template<typename T>
	bool operator!=(const T other)const { return !(*this == other); }
	//bool operator==(const long long other)const;
	bool operator==(const string& other)const;
	bool operator==(const char* other)const;
	AttrVar& operator++();
	AttrVar operator+(const AttrVar& other);
	int to_int()const;
	long long to_ll()const;
	double to_num()const;
	string to_str()const;
	ByteS to_bytes()const;
	static AttrVar parse(const string& s);
	static AttrVar parse_toml(std::ifstream& s);
	string print()const;
	bool str_empty()const;
	AttrObject to_obj()const;
	std::shared_ptr<AttrVars> to_dict()const;
	std::shared_ptr<VarArray> to_list()const;
	fifo_json to_json()const;

	using CMPR = bool(AttrVar::*)(const AttrVar&)const;
	bool is_null()const { return type == AttrType::Nil; }
	bool is_boolean()const { return type == AttrType::Boolean; }
	bool is_true()const { return operator bool(); }
	bool is_numberic()const;
	bool is_text()const { return type == AttrType::Text; }
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
string to_string(const AttrVar& var);
fifo_json to_json(const AttrVars& vars);
void from_json(const fifo_json& j, AttrVars&);
string showAttrCMPR(AttrVar::CMPR);

using AttrObjects = std::unordered_map<string, AttrObject>;
using AttrIndex = AttrVar(*)(AttrObject&);
using AttrIndexs = std::unordered_map<string, AttrIndex>;