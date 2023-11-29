#pragma once
/*
 * ��Դģ��
 * Copyright (C) 2019-2023 String.Empty
 */

#include <utility>
#include <variant>
#include <regex>
#include "yaml-cpp/node/node.h"
#include "SHKQuerier.h"
#include "GlobalVar.h"
#include "DiceRule.h"
#ifndef __ANDROID__
#include "DiceGit.h"
#endif //ANDROID
#include "CharacterCard.h"
using std::variant;
namespace fs = std::filesystem;

class DiceEvent;
struct Version {
	string exp;
	int major{ 0 };
	int minor{ 0 };
	int revision{ 0 };
	int build{ 0 };
	Version(){}
	Version(const string& strVer):exp(strVer) {
		static std::regex re{ R"((\d{1,9})\.?(\d{0,9})\.?(\d{0,9})\w*\(?(\d{0,9}))" };
		std::smatch match;
		try {
			if (std::regex_search(exp, match, re)) {
				major = stoi(match[1].str());
			}
			if (match.size() > 2 && match[2].length()){
				minor = stoi(match[2].str());
				if (match.size() > 3 && match[3].length())revision = stoi(match[3].str());
			}
			if (match.size() > 4 && match[4].length())build = stoi(match[4].str());
		}
		catch (std::exception& e) {
			console.log("mod�汾����ƥ��ʧ��!" + string(e.what()), 1);
		}
	}
	bool operator<(const Version& other)const {
		if (major != other.major)return major < other.major;
		if (minor != other.minor)return minor < other.minor;
		if (revision != other.revision)return revision < other.revision;
		return (build && other.build) ? build < other.build : false;
	}
};

class DiceEventTrigger {
	enum class Mode { Nil, Clock, Cycle, Trigger };
	enum class ActType { Nil, Lua };
	//��֧��lua
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
	void free() { repo.reset(); }
	void remote(const string& url);
#endif //ANDROID
public:
	DiceMod(const string& mod, size_t i, bool b) :name(mod), index(i), active(b) {}
	string name;
	string title;
	string author;
	Version ver;
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
	bool reload(string& cb);
	void loadLua();
	string desc()const;
	string detail()const;
private:
	dict_ci<DiceRule> rules;
	dict_ci<CardTemp> card_models;
	dict_ci<>helpdoc;
	dict_ci<DiceSpeech>speech;
	//native path of .lua
	dict_ci<string>lua_scripts;
	//native path of .js
	dict_ci<string>js_scripts;
	//path of .py
	dict_ci<std::filesystem::path>py_scripts;
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
	vector<string> sourceList = {
		"https://raw.gitmirror.com/Dice-Developer-Team/DiceModIndex/main/index",
		"https://mirror.ghproxy.com/https://raw.githubusercontent.com/Dice-Developer-Team/DiceModIndex/main/index",
		"https://gitee.com/diceki/DiceModIndex/raw/main/",
		//"https://raw.sevencdn.com/Dice-Developer-Team/DiceModIndex/main/index",
	};
	//custom
	dict_ci<ptr<DiceMsgReply>> plugin_reply;
	dict_ci<ptr<DiceMsgReply>> custom_reply;
	//global
	dict_ci<> global_helpdoc;
	DiceReplyUnit final_reply;
	dict_ci<string> global_lua_scripts;
	dict_ci<string> global_js_scripts;
	dict_ci<std::filesystem::path> global_py_scripts;
	dict_ci<AttrVars> taskcall;
	AttrObjects global_events; //events by id
	//Event
	unordered_set<string> cycle_events; //����ʱΨһ�Լ��
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
	dict_ci<DiceSpeech> global_speech;
	multimap<Clock, string> clock_events;
	friend class CustomReplyApiHandler; 
	friend class ModListApiHandler;
	bool isIniting{ false };

	string list_mod()const;
	bool has_mod(const string& name)const { return modList.count(name); }
	ptr<DiceMod> get_mod(const string& name)const {
		return modList.count(name) ? modList.at(name) : ptr<DiceMod>();
	}
	void mod_on(DiceEvent*);
	void mod_off(DiceEvent*);
#ifndef __ANDROID__
	bool mod_clone(const string& name, const string& url);
#endif
	bool mod_dlpkg(const string& name, const string& url, string& des);
	void mod_install(DiceEvent&);
	void mod_reinstall(DiceEvent&);
	void mod_update(DiceEvent&);
	void turn_over(size_t);
	void uninstall(const string& name);
	bool reorder(size_t, size_t);

	AttrVar format(const string&, const AttrObject& = {},
		bool isTrust = true,
		const dict_ci<string>& = {}) const;
	string msg_get(const string& key)const;
	void msg_reset(const string& key);
	void msg_edit(const string& key, const string& val);

	fifo_dict_ci<size_t>cntHelp;
	[[nodiscard]] string get_help(const string&, AttrObject = {}) const;
	[[nodiscard]] string prev_help(const string&, AttrObject = {}) const;
	void _help(DiceEvent*);
	void set_help(const string&, const string&);
	void rm_help(const string&);

	void call_cycle_event(const string&);
	void call_clock_event(const string&);
	//return if event is blocked
	bool call_hook_event(AttrObject);

	bool listen_order(DiceEvent* msg) { return final_reply.listen(msg, 1); }
	bool listen_reply(DiceEvent* msg) { return final_reply.listen(msg, 2); }
	bool listen_game(DiceEvent* msg) { return final_reply.listen(msg, 4); }
	string list_reply(int type)const;
	void set_reply(const string&, ptr<DiceMsgReply> reply);
	bool del_reply(const string&);
	void save_reply();
	void reply_get(DiceEvent*);
	void reply_show(DiceEvent*);
	bool call_task(const string&);

	bool has_lua(const string& name)const { return global_lua_scripts.count(name); }
	bool has_js(const string& name)const { return global_js_scripts.count(name); }
	bool has_py(const string& name)const { return global_py_scripts.count(name); }
	string lua_path(const string& name)const;
	string js_path(const string& name)const;
	std::optional<std::filesystem::path> py_path(const string& name)const;

	void loadPlugin(ResList& res);
	int load(ResList&);
	void initCloud();
	void build();
	void clear();
	void save();
};

extern std::shared_ptr<DiceModManager> fmt;
void call_event(AttrObject eve, const ptr<AttrVars>& action);