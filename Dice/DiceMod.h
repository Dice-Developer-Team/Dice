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
#ifndef __ANDROID__
#include "DiceGit.h"
#endif //ANDROID
using std::unordered_multimap;
using std::variant;
template<typename T>
using ptr = std::shared_ptr<T>;
using ex_lock = ptr<std::unique_lock<std::mutex>>;
using chat_locks = std::list<ex_lock>;
namespace fs = std::filesystem;

class DiceEvent;

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
	bool check(DiceEvent*, chat_locks&)const;
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
	static ptr<DiceMsgReply> set_order(const string& key, const AttrVars&);
	string show()const;
	string show_ans()const;
	string print()const;
	bool exec(DiceEvent*);
	void from_obj(AttrObject);
	void readJson(const fifo_json&);
	fifo_json writeJson()const;
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
class DiceEventTrigger {
	enum class Mode { Nil, Clock, Cycle, Trigger };
	enum class ActType { Nil, Lua };
	//仅支持lua
	ActType type{ ActType::Nil };
	string script;
public:
	DiceEventTrigger() = default;
	DiceEventTrigger(const AttrObject&);
	bool exec(DiceEvent*);
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
	DiceSpeech(const fifo_json& j);
	string express()const;
};

class DiceMod {
	size_t index{ 0 };
	fs::path pathJson;
	fs::path pathDir;
	DiceMod& file(const fs::path& p) {
		pathJson = p;
		(pathDir = p).replace_extension();
		return *this;
	}
	friend class DiceModManager;
#ifndef __ANDROID__
	ptr<DiceRepo> repo;
	DiceMod(const string& mod, size_t i, const string& url);
#endif //ANDROID
public:
	DiceMod(const string& mod, size_t i, bool b) :name(mod), index(i), active(b) {}
	string name;
	string title;
	string author;
	string ver;
	string brief;
	bool active{ true };
	DiceMod& on() {
		active = true;
		return *this;
	}
	DiceMod& off() {
		active = false;
		return *this;
	}
	bool loaded{ false };
	bool loadDesc(string&);
	void loadDir();
	void loadLua();
	string desc()const;
	string detail()const;
private:
	dict<>helpdoc;
	dict<DiceSpeech>speech;
	dict_ci<string>scripts;
	vector<fs::path>luaFiles;
	dict<ptr<DiceMsgReply>>reply_list;
	AttrObjects events;
	size_t cntImage{ 0 };
	size_t cntAudio{ 0 };
};

using Clock = std::pair<unsigned short, unsigned short>;
class ResList;
class DiceModManager {
	dict_ci<ptr<DiceMod>> modList;
	vector<ptr<DiceMod>> modOrder;
	vector<string> sourceList = { "https://raw.sevencdn.com/Dice-Developer-Team/DiceModIndex/main/" };
	//custom
	dict_ci<ptr<DiceMsgReply>> plugin_reply;
	dict_ci<ptr<DiceMsgReply>> custom_reply;
	//global
	dict_ci<DiceSpeech> global_speech;
	dict_ci<> global_helpdoc;
	DiceReplyUnit final_reply;
	dict_ci<string> global_scripts;
	dict_ci<AttrVars> taskcall;
	AttrObjects global_events; //events by id
	//Event
	unordered_set<string> cycle_events; //重载时唯一性检查
	multidict_ci<AttrObject> hook_events;

	WordQuerier querier;
	friend class DiceMod;
public:
	DiceModManager();
	~DiceModManager() {
#ifndef __ANDROID__
		git_libgit2_shutdown();
#endif
	}
	multimap<Clock, string> clock_events;
	friend class CustomReplyApiHandler;
	bool isIniting{ false };

	string list_mod()const;
	bool has_mod(const string& name)const { return modList.count(name); }
	ptr<DiceMod> get_mod(const string& name)const {
		return modList.count(name) ? modList.at(name) : ptr<DiceMod>();
	}
	void mod_on(DiceEvent*);
	void mod_off(DiceEvent*);
	void mod_install(DiceEvent&);

	string format(string, AttrObject = {},
		const AttrIndexs& = MsgIndexs,
		const dict_ci<string>& = {}) const;
	string msg_get(const string& key)const;
	void msg_reset(const string& key);
	void msg_edit(const string& key, const string& val);

	fifo_dict_ci<size_t>cntHelp;
	[[nodiscard]] string get_help(const string&, AttrObject = {}) const;
	void _help(DiceEvent*);
	void set_help(const string&, const string&);
	void rm_help(const string&);

	void call_cycle_event(const string&);
	void call_clock_event(const string&);
	//return if event is blocked
	bool call_hook_event(AttrObject);

	bool listen_order(DiceEvent* msg) { return final_reply.listen(msg, 1); }
	bool listen_reply(DiceEvent* msg) { return final_reply.listen(msg, 2); }
	string list_reply(int type)const;
	void set_reply(const string&, ptr<DiceMsgReply> reply);
	bool del_reply(const string&);
	void save_reply();
	void reply_get(DiceEvent*);
	void reply_show(DiceEvent*);
	bool call_task(const string&);

	bool script_has(const string& name)const { return global_scripts.count(name); }
	string script_path(const string& name)const;

	void loadPlugin(ResList& res);
	int load(ResList&);
	void initCloud();
	void build();
	void clear();
	void save();
};

extern std::shared_ptr<DiceModManager> fmt;
