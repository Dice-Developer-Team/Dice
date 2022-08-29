#pragma once
/*
 * 资源模块
 * Copyright (C) 2019-2022 String.Empty
 */

#include <utility>
#include <list>
#include <variant>
#include <regex>
#include "yaml-cpp/node/node.h"
#include "STLExtern.hpp"
#include "SHKQuerier.h"
#include "SHKTrie.h"
#include "DiceSchedule.h"
#include "GlobalVar.h"
using std::unordered_multimap;
using std::variant;
template<typename T>
using ptr = std::shared_ptr<T>;
using ex_lock = ptr<std::unique_lock<std::mutex>>;
using chat_locks = std::list<ex_lock>;
namespace fs = std::filesystem;

class FromMsg;

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
	bool check(FromMsg*, chat_locks&)const;
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
	enum class Type { Nor, Order, Reply, Both };   //决定受控制的开关类型
	static enumap_ci sType;
	std::unique_ptr<vector<string>>keyMatch[4];
	static enumap_ci sMode;
	enum class Echo { Text, Deck, Lua };    //回复形式
	static enumap_ci sEcho;
	Type type{ Type::Reply };
	Echo echo{ Echo::Deck };
	DiceTriggerLimit limit;
	AttrVar text;
	std::vector<string> deck;
	string show()const;
	string show_ans()const;
	string print()const;
	bool exec(FromMsg*);
	void from_obj(AttrObject);
	void readJson(const json&);
	json writeJson()const;
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
	void add(const string& key, ptr<DiceMsgReply> reply);
	void add_order(const string& key, AttrVars);
	void build();
	void erase(ptr<DiceMsgReply> reply);
	void erase(const string& key) { if (items.count(key))erase(items[key]); }
	void insert(const string&, ptr<DiceMsgReply> reply);
	bool listen(FromMsg*, int);
};
class DiceEvent {
	enum class Mode { Nil, Clock, Cycle, Trigger };
	enum class ActType { Nil, Lua };
	//仅支持lua
	ActType type{ ActType::Nil };
	string script;
public:
	DiceEvent() = default;
	DiceEvent(const AttrObject&);
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

/*
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
*/

class DiceModConf {
public:
	string name;
	size_t index{ 0 };
	bool active{ true };
	bool loaded{ false };
};

using Clock = std::pair<unsigned short, unsigned short>;
class ResList;
class DiceModManager
{
	dict_ci<ptr<DiceModConf>> modList;
	vector<ptr<DiceModConf>> modIndex;
	//custom
	dict_ci<ptr<DiceMsgReply>> custom_reply;
	//global
	dict_ci<DiceSpeech> global_speech;
	dict_ci<string> helpdoc;
	DiceReplyUnit final_reply;
	dict_ci<AttrVars> taskcall;
	dict_ci<string> scripts;
	//Event
	dict_ci<AttrObject> events; //events by id
	unordered_set<string> cycle_events; //重载时唯一性检查
	multidict_ci<AttrObject> hook_events;

	WordQuerier querier;
	AttrObjects mod_reply_list;
public:
	DiceModManager();
	multimap<Clock, string> clock_events;
	friend class CustomReplyApiHandler;
	bool isIniting{ false };

	string list_mod()const;
	void mod_on(FromMsg*);
	void mod_off(FromMsg*);

	string format(string, AttrObject = {},
		const AttrIndexs& = MsgIndexs,
		const dict_ci<string>& = EditedMsg) const;
	string msg_get(const string& key)const;
	void msg_reset(const string& key);
	void msg_edit(const string& key, const string& val);

	dict_ci<size_t>cntHelp;
	[[nodiscard]] string get_help(const string&, AttrObject = {}) const;
	void _help(const shared_ptr<DiceJobDetail>&);
	void set_help(const string&, const string&);
	void rm_help(const string&);

	void call_cycle_event(const string&);
	void call_clock_event(const string&);
	//return if event is blocked
	bool call_hook_event(AttrObject);

	bool listen_order(FromMsg* msg) { return final_reply.listen(msg, 1); }
	bool listen_reply(FromMsg* msg) { return final_reply.listen(msg, 2); }
	string list_reply(int type)const;
	void set_reply(const string&, ptr<DiceMsgReply> reply);
	bool del_reply(const string&);
	void save_reply();
	void reply_get(const shared_ptr<DiceJobDetail>&);
	void reply_show(const shared_ptr<DiceJobDetail>&);
	bool call_task(const string&);

	bool script_has(const string& name)const { return scripts.count(name); }
	string script_path(const string& name)const;

	void loadLuaMod(const vector<fs::path>&, ResList&);
	void loadPlugin(ResList& res);
	int load(ResList&);
	void init();
	void clear();
	void reload();
	void save();
};

extern std::shared_ptr<DiceModManager> fmt;
