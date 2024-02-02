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
#pragma once
#include "STLExtern.hpp"
#include "SHKTrie.h"
#include "DiceAttrVar.h"
#include "DiceSchedule.h"
#include <list>
#include <regex>
using std::unordered_multimap;
using ex_lock = ptr<std::unique_lock<std::mutex>>;
using chat_locks = std::list<ex_lock>;
class DiceTriggerLimit {
	string content;
	string comment;
	int prob{ 0 };
	unordered_set<long long>user_id;
	bool user_id_negative{ false };
	unordered_set<long long>grp_id;
	bool grp_id_negative{ false };
	vector<CDConfig>locks;
	vector<CDConfig>cd_timer;
	vector<CDConfig>today_cnt;
	fifo_dict_ci<pair<AttrVar::CMPR, AttrVar>>self_vary;
	fifo_dict_ci<pair<AttrVar::CMPR, AttrVar>>user_vary;
	fifo_dict_ci<pair<AttrVar::CMPR, AttrVar>>grp_vary;
	enum class Treat :size_t { Ignore, Only, Off };
	Treat to_dice{ Treat::Ignore };
public:
	DiceTriggerLimit& parse(const string&);
	DiceTriggerLimit& parse(const AttrVar&);
	const string& print()const { return content; }
	const string& note()const { return comment; }
	bool empty()const { return content.empty(); }
	bool check(DiceEvent*, chat_locks&)const;
};
class DiceMsgReply {
public:
	string title;
	enum class Type { Nor, Order, Reply, Both, Game };   //决定受控制的开关类型
	static enumap_ci sType;
	std::unique_ptr<vector<string>>keyMatch[4];
	static enumap_ci sMode;
	enum class Echo { Text, Deck, Lua, JavaScript, Python};    //回复形式
	static enumap_ci sEcho;
	Type type{ Type::Reply };
	Echo echo{ Echo::Deck };
	DiceTriggerLimit limit;
	AttrObject answer;
	static ptr<DiceMsgReply> set_order(const string& key, const AttrVars&);
	string show()const;
	string show_ans()const;
	string print()const;
	bool exec(DiceEvent*);
	void from_obj(AttrObject);
	void readJson(const fifo_json&);
	fifo_json writeJson()const;
	fifo_json to_line()const;
};

class DiceReplyUnit {
	TrieG<char, less_ci> gPrefix;
	TrieG<char16_t, less_ci> gSearcher;
public:
	dict<ptr<DiceMsgReply>> items;
	//formated kw of match, remove while matched
	dict_ci<ptr<DiceMsgReply>> match_items;
	//raw kw of prefix, remove while erase
	dict_ci<ptr<DiceMsgReply>> prefix_items;
	//raw kw of search, remove while erase
	dict_ci<ptr<DiceMsgReply>> search_items;
	//regex mode without formating
	dict<ptr<DiceMsgReply>> regex_items;
	unordered_multimap<string, std::wregex> regex_exp;
	//ptr<DiceMsgReply>& operator[](const string& key) { return items[key]; }
	void add(const string& key, ptr<DiceMsgReply> reply);
	void build();
	void erase(ptr<DiceMsgReply> reply);
	void erase(const string& key) { if (items.count(key))erase(items[key]); }
	void insert(const string&, ptr<DiceMsgReply> reply);
	bool listen(DiceEvent*, int);
};