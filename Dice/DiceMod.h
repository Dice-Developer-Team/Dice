#pragma once

/*
 * 资源模块
 * Copyright (C) 2019-2022 String.Empty
 */

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <variant>
#include "yaml-cpp/node/node.h"
#include "STLExtern.hpp"
#include "SHKQuerier.h"
#include "SHKTrie.h"
#include "DiceSchedule.h"
#include "GlobalVar.h"
using std::string;
using std::vector;
using std::map;
using std::unordered_multimap;
using std::set;
using std::variant;
template<typename T>
using ptr = std::shared_ptr<T>;

class FromMsg;

class DiceTriggerLimit {
    string content;
    string comment;
    int prob{ 0 };
    set<long long>user_id;
    bool user_id_negative{ false };
    set<long long>grp_id;
    bool grp_id_negative{ false };
    vector<CDConfig>cd_timer;
    vector<CDConfig>today_cnt;
    unordered_map<string, pair<AttrVar::CMPR, AttrVar>>self_vary;
    unordered_map<string, pair<AttrVar::CMPR, AttrVar>>user_vary;
    unordered_map<string, pair<AttrVar::CMPR, AttrVar>>grp_vary;
    enum class Treat :size_t { Ignore, Only, Off };
    Treat to_dice{ Treat::Ignore };
public:
    DiceTriggerLimit& parse(const string&);
    DiceTriggerLimit& parse(const AttrVar&);
    const string& print()const { return content; }
    const string& note()const { return comment; }
    bool empty()const { return content.empty(); }
    bool check(FromMsg*)const;
};

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

class DiceMsgReply {
public:
    string title;
    enum class Type { Reply, Order };   //决定受控制的开关类型
    static enumap<string> sType;
    std::unique_ptr<vector<string>>keyMatch[4];
    static enumap_ci sMode;
    enum class Echo { Text, Deck, Lua };    //回复形式
    static enumap<string> sEcho;
    Type type{ Type::Reply };
    Echo echo{ Echo::Deck };
    DiceTriggerLimit limit;
    string text;
    std::vector<string> deck;
    string show()const;
    string show_ans()const;
    string print()const;
    bool exec(FromMsg*);
    void from_obj(AttrObject);
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

class DiceSpeech {
    variant<string, vector<string>> speech;
public:
    DiceSpeech(){}
    DiceSpeech(const string& s):speech(s) {}
    DiceSpeech(const char* s) :speech(s) {}
    DiceSpeech(const vector<string>& v) :speech(v) {}
    DiceSpeech(const YAML::Node& yaml);
    DiceSpeech(const json& j);
    string express()const;
};

class DiceMod
{
protected:
    string mod_name;
    string mod_author;
    string mod_ver;
    unsigned int mod_build{ 0 };
    unsigned int mod_Dice_build{ 0 };
    dict_ci<string> mod_helpdoc;
    dict_ci<vector<string>> mod_public_deck;
    using replys = dict_ci<DiceMsgReply>;
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
    //MOD_BUILD(dict_ci<string>, helpdoc)
   // MOD_BUILD(replys, msg_reply)
};

class DiceModConf {
public:
    bool active{ true };
    int index{ 0 };
};

class ResList;
class DiceModManager
{
    dict_ci<DiceMod> modList;
    vector<pair<string, bool>>modIndex;
    //custom
    dict_ci<ptr<DiceMsgReply>> custom_reply;
    //global
    dict_ci<DiceSpeech> global_speech;
    dict_ci<string> helpdoc;
    dict_ci<DiceMsgOrder> msgorder;
    dict_ci<ptr<DiceMsgReply>> final_msgreply;
    dict_ci<DiceMsgOrder> taskcall;
    dict_ci<string> scripts;
    //Trigger
    dict_ci<AttrObject> triggers; //triggers by id
    unordered_multimap<string, AttrObject> trigger_by_event;

    WordQuerier querier;
    TrieG<char, less_ci> gOrder;
    AttrObjects mod_reply_list;
    dict_ci<ptr<DiceMsgReply>> reply_match;
    dict_ci<ptr<DiceMsgReply>> reply_prefix;
    dict_ci<ptr<DiceMsgReply>> reply_search;
    dict<ptr<DiceMsgReply>> reply_regex;
    TrieG<char, less_ci> gReplyPrefix;
    TrieG<char16_t, less_ci> gReplySearcher;
public:
	DiceModManager();
    friend class CustomReplyApiHandler;
    bool isIniting{ false };
	string format(string, AttrObject = {},
        const AttrIndexs& = MsgIndexs,
        const dict_ci<string>& = EditedMsg) const;
    string msg_get(const string& key)const;
    void msg_reset(const string& key)const;
    void msg_edit(const string& key, const string& val)const;

    dict_ci<size_t>cntHelp;
	[[nodiscard]] string get_help(const string&) const;
    void _help(const shared_ptr<DiceJobDetail>&);
	void set_help(const string&, const string&);
	void rm_help(const string&);

    bool listen_order(DiceJobDetail*);
    bool listen_reply(FromMsg*);
    string list_reply()const;
    void set_reply(const string&, ptr<DiceMsgReply> reply);
    void reply_insert(const string&, ptr<DiceMsgReply> reply);
    void reply_erase(ptr<DiceMsgReply> reply);
    bool del_reply(const string&);
    void save_reply();
    void reply_get(const shared_ptr<DiceJobDetail>&);
    void reply_show(const shared_ptr<DiceJobDetail>&);
    bool call_task(const string&);
    string list_order();

    bool script_has(const string& name)const { return scripts.count(name); }
    string script_path(const string& name)const;

    void loadLuaMod(const vector<std::filesystem::path>&, ResList&);
	int load(ResList&);
    void init();
	void clear();
};

inline std::shared_ptr<DiceModManager> fmt;
