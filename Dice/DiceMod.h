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
    string mod_name;
    string auther;
    string ver;
    unsigned int build;
    unsigned int Dice_build;
    map<string, string, less_ci>m_helpdoc;
    map<string, vector<string>> m_public_deck;
    /*map<string, DiceGenerator> m_generator;*/
public:
    DiceMod() = default;
    DiceMod(string name,
        map<string, string, less_ci>helpdoc/*,
        map<string, vector<string>> private_deck,
        map<string, vector<string>> public_deck,
        map<string, DiceGenerator> generator*/
        ) : mod_name(std::move(name)), m_helpdoc(std::move(helpdoc))
        /*,m_private_deck(private_deck),m_public_deck(public_deck),m_generator(generator)*/
    {
    }
    friend class DiceModFactory;
};
class DiceModFactory :public DiceMod {
    string mod_name;
    string auther;
    string ver;
    unsigned int build;
    unsigned int Dice_build;
    map<string, string, less_ci>m_helpdoc;
    map<string, vector<string>> m_public_deck;
    /*map<string, DiceGenerator> m_generator;*/
public:
    DiceModFactory& name(string strMod){}
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
    void _help(DiceJobDetail*);
	void set_help(const string&, const string&);
	void rm_help(const string&);
	int load(string&);
    void init();
	void clear();
};

inline std::shared_ptr<DiceModManager> fmt;
