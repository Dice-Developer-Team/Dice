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
 * Copyright (C) 2019-2023 String.Empty
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
#include <variant>
#include "fifo_json.hpp"
#include "toml.hpp"
#include "DiceYaml.h"
#include "fifo_set.hpp"
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
using HashedVar = std::variant<double, string>;
struct AttrIndex {
	HashedVar val;
	AttrIndex(int i):val(double(i)){}
	AttrIndex(long long num) :val(double(num)) {}
	AttrIndex(double num) :val(num) {}
	AttrIndex(const string& s) :val(s) {}
	fifo_json to_json()const;
};
fifo_json to_json(const fifo_set<AttrIndex>& vars);
toml::array to_toml(const fifo_set<AttrIndex>& vars);
using AttrSet = ptr<fifo_set<AttrIndex>>;

template<>
struct std::hash<AttrIndex> {
	_NODISCARD size_t operator()(const AttrIndex& _Keyval) const noexcept {
		return hash<HashedVar>()(_Keyval.val);
	}
};
template<>
struct std::equal_to<AttrIndex> {
	_NODISCARD constexpr bool operator()(const AttrIndex& _Left, const AttrIndex& _Right) const {
		return _Left.val == _Right.val;
	}
};

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
	explicit AttrObject(const VarArray& vars) :dict(std::make_shared<AttrVars>()), list(std::make_shared<VarArray>(vars)) {}
	template<class T>
	AttrObject(const std::vector<T>& vars) :dict(std::make_shared<AttrVars>()), list(std::make_shared<VarArray>()) {
		for (auto& it : vars)list->emplace_back(it);
	}
	AttrObject(const AttrObject& other) :dict(other.dict), list(other.list) {}
	const ptr<AttrVars>& to_dict()const { return dict; }
	const ptr<VarArray>& to_list()const { return list; }
	std::vector<string> to_deck()const;
	const ptr<VarArray>& new_list() { return list = std::make_shared<VarArray>(); }
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
	size_t size()const { return dict->size(); }
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
	int inc(const string& key, int i)const;
	void add(const string& key, const AttrVar&)const;
	AttrObject& merge(const AttrVars& other);
	fifo_json to_json()const;
	toml::table to_toml()const;
	void writeb(std::ofstream&)const;
	void readb(std::ifstream&);
};

class AttrVar {
public:
	enum class Type { Nil, Boolean, Integer, Number, Text, Table, Function, ID, Set	};
	Type type{ 0 };
	union {
		bool bit;		//1
		int attr{ 0 };		//2
		double number;	//3
		string text;	//4
		AttrObject table;		//5
		ByteS chunk;		//6
		long long id;		//7
		AttrSet flags;
	};
	AttrVar() {}
	AttrVar(const AttrVar& other);
	explicit AttrVar(bool b) :type(Type::Boolean), bit(b) {}
	AttrVar(int n) :type(Type::Integer), attr(n) {}
	AttrVar(double n) :type(Type::Number), number(n) {}
	AttrVar(const char* s) :type(Type::Text), text(s) {}
	AttrVar(const string& s) :type(Type::Text), text(s) {}
	AttrVar(const char* s,size_t len) :type(Type::Function), chunk(s,len) {}
	AttrVar(ByteS&& fun) :type(Type::Function), chunk(fun) {}
	AttrVar(long long n) :type(Type::ID), id(n) {}
	AttrVar(const fifo_set<AttrIndex>& s) :type(Type::Set), flags(std::make_shared<fifo_set<AttrIndex>>(s)) {}
	AttrVar(const AttrSet& s) :type(Type::Set), flags(s) {}
	AttrVar(const HashedVar& vars);
	AttrVar(const fifo_json&);
	AttrVar(const toml::node&);
	AttrVar(const YAML::Node&);
	AttrVar(const AttrObject& vars) :type(Type::Table), table(vars) {}
	explicit AttrVar(const AttrVars& vars) :type(Type::Table), table(vars) {}
	void des() {
		if (type == Type::Text)text.~string();
		else if (type == Type::Table)table.~AttrObject();
		else if (type == Type::Set)flags.~AttrSet();
		else if (type == Type::Function)chunk.~ByteS();
		type = Type::Nil;
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
	AttrVar& operator+=(const AttrVar& other) { return *this = *this + other; }
	int to_int()const;
	long long to_ll()const;
	double to_num()const;
	string to_str()const;
	ByteS to_bytes()const;
	static AttrVar parse(const string& s);
	string print()const;
	bool str_empty()const;
	AttrObject to_obj()const;
	std::shared_ptr<AttrVars> to_dict()const;
	std::shared_ptr<VarArray> to_list()const;
	fifo_json to_json()const;
	YAML::Node to_yaml()const;

	using CMPR = bool(AttrVar::*)(const AttrVar&)const;
	bool is_null()const { return type == Type::Nil; }
	bool is_boolean()const { return type == Type::Boolean; }
	bool is_true()const { return operator bool(); }
	bool is_numberic()const;
	bool is_text()const { return type == Type::Text; }
	bool is_character()const { return type != Type::Nil && type != Type::Boolean && type != Type::Table && type != Type::Function && type != Type::Set; }
	bool is_table()const { return type == Type::Table; }
	bool is_function()const { return type == Type::Function; }
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
using AttrGetter = AttrVar(*)(AttrObject&);
using AttrGetters = std::unordered_map<string, AttrGetter>;