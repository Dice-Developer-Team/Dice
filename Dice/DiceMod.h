/*
 * 资源模块
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <memory>
#include "STLExtern.hpp"
#include "SHKQuerier.h"
#include "DiceSchedule.h"
using std::string;
using std::vector;
using std::map;

class DiceGenerator
{
    //冷却时间
    //int cold_time;
    //单次抽取上限
    //int draw_limit = 1;
    string expression;
    //string cold_msg = "冷却时间中×";
public:
	string getExp() { return expression; }
};

class BaseDeck
{
public:
	vector<string> cards;
};

class DiceMod
{
protected:
    string mod_name;
    string mod_author;
    string mod_ver;
    unsigned int mod_build{ 0 };
    unsigned int mod_Dice_build{ 0 };
    using dir = map<string, string, less_ci>;
    dir mod_helpdoc;
    map<string, vector<string>> mod_public_deck;
    /*map<string, DiceGenerator> m_generator;*/
public:
    DiceMod() = default;
    friend class DiceModFactory;
};

#define MOD_BUILD(TYPE, MEM) DiceModFactory& MEM(const TYPE& val){ \
        mod_##MEM = val;  \
		return *this; \
	} 
class DiceModFactory :public DiceMod {
public:
    DiceModFactory() {}
    MOD_BUILD(string, name)
    MOD_BUILD(string, author)
    MOD_BUILD(string, ver)
    MOD_BUILD(unsigned int, build)
    MOD_BUILD(unsigned int, Dice_build)
    MOD_BUILD(dir, helpdoc)
};

class DiceModManager
{
	map<string, DiceMod> mNameIndex;
	map<string, string, less_ci> helpdoc;
    WordQuerier querier;
public:
	DiceModManager();
	friend void loadData();
    bool isIniting{ false };
	string format(string, const map<string, string, less_ci>&, const char*) const;
    unordered_map<string, size_t>cntHelp;
	[[nodiscard]] string get_help(const string&) const;
    void _help(const shared_ptr<DiceJobDetail>&);
	void set_help(const string&, const string&);
	void rm_help(const string&);
	int load(string&);
    void init();
	void clear();
};

inline std::shared_ptr<DiceModManager> fmt;
