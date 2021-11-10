#pragma once

/*
 * 资源模块
 * Copyright (C) 2019-2021 String.Empty
 */

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "STLExtern.hpp"
#include "SHKQuerier.h"
#include "SHKTrie.h"
#include "DiceSchedule.h"
using std::string;
using std::vector;
using std::map;
using std::set;

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

class FromMsg;
class DiceMsgReply {
public:
    enum class Type { Reply, Order };   //决定受控制的开关类型
    static enumap<string> sType;
    enum class Mode { Match, Search, Regex };    //匹配模式
    static enumap<string> sMode;
    enum class Echo { Text, Deck, Lua };    //回复形式
    static enumap<string> sEcho;
    Type type{ Type::Reply };
    Mode mode{ Mode::Match };
    Echo echo{ Echo::Deck };
    string text;
    std::vector<string> deck;
    string show_ans()const;
    bool exec(FromMsg*);
    void readJson(const json&);
    json writeJson()const;
};

class DiceMsgOrder {
    enum class OrderType { Nil, Lua };
    //仅支持lua
    OrderType type{ OrderType::Nil };
    string fileLua;
    string funcLua;
public:
    DiceMsgOrder() = default;
    DiceMsgOrder(const string& file, const string& func): fileLua(file), funcLua(func){
        type = OrderType::Lua;
    }
    bool exec(FromMsg*);
    bool exec();
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
    using orders = map<string, DiceMsgOrder, less_ci>;
    orders mod_msg_order;
    using replys = map<string, DiceMsgReply>;
    replys mod_msg_reply;
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
    MOD_BUILD(orders, msg_order)
    MOD_BUILD(replys, msg_reply)
};

class ResList;
class DiceModManager
{
	map<string, DiceMod> mNameIndex;
	map<string, string, less_ci> helpdoc;
    map<string, DiceMsgOrder, less_ci> msgorder;
    map<string, DiceMsgOrder, less_ci> taskcall;
    WordQuerier querier;
    TrieG<char, less_ci> gOrder;
    map<string, DiceMsgReply, less_ci> msgreply;
    set<string, less_ci> reply_regex;
    TrieG<char16_t, less_ci> gReplySearcher;
public:
	DiceModManager();
	friend void loadData();
    friend class CustomReplyApiHandler;
    bool isIniting{ false };
	string format(string, const map<string, string, less_ci>&, const char* mod_name = "") const;
    unordered_map<string, size_t>cntHelp;
	[[nodiscard]] string get_help(const string&) const;
    void _help(const shared_ptr<DiceJobDetail>&);
	void set_help(const string&, const string&);
	void rm_help(const string&);

    bool listen_order(DiceJobDetail*);
    bool listen_reply(FromMsg*);
    string list_reply()const;
    void set_reply(const string&, DiceMsgReply& reply);
    bool del_reply(const string&);
    void save_reply();
    void show_reply(const shared_ptr<DiceJobDetail>&);
    bool call_task(const string&);
    string list_order();
	int load(ResList*);
    void init();
	void clear();
};

inline std::shared_ptr<DiceModManager> fmt;
