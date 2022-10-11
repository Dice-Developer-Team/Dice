#pragma once
#include <unordered_set>
#include "filesystem.hpp"
#include "STLExtern.hpp"
#include "DiceAttrVar.h"
#include "DiceMsgSend.h"

using std::pair;
using std::string;
using std::to_string;
using std::vector;
using std::map;
using std::multimap;
using std::unordered_set;
using std::shared_ptr;
namespace fs = std::filesystem;

class DiceEvent;
class DiceTableMaster;

struct LogInfo{
	static const std::filesystem::path dirLog;
	bool isLogging{ false };
	//创建时间，为0则不存在
	time_t tStart{ 0 };
	time_t tLastMsg{ 0 };
	string name;
	string fileLog;
	//路径不保存，初始化时生成
	std::filesystem::path pathLog;
	void update() {
		tLastMsg = time(nullptr);
	}
};

struct LinkInfo {
	bool isLinking{ false };
	string typeLink;
	//对象窗口，为0则不存在
	chatInfo target{ 0 };
};
class DiceChatLink {
	unordered_map<chatInfo, LinkInfo>LinkList;
	//禁止桥接等花哨操作
	unordered_map<chatInfo, pair<chatInfo, bool>>LinkFromChat;
public:
	friend class DiceSessionManager;
	pair<chatInfo, bool> get_aim(chatInfo ct)const {
		return LinkFromChat.count(ct = ct.locate()) ? LinkFromChat.find(ct)->second : pair<chatInfo, bool>();
	}
	//link指令
	void build(DiceEvent*);
	void start(DiceEvent*);
	string show(const chatInfo& ct);
	string list();
	void show(DiceEvent*);
	void close(DiceEvent*);
	void load();
	void save();
};

struct DeckInfo {
	//元表
	vector<string> meta;
	//剩余牌
	vector<size_t> idxs;
	size_t sizRes{ 0 };
	DeckInfo() = default;
	DeckInfo(const vector<string>& deck);
	void init();
	void reset();
	string draw();
};

class DiceSession{
	//数值表
	dict<AttrObject> mTable;
	//旁观者
	unordered_set<long long> sOB;
	//日志
	LogInfo logger;
	//牌堆
	map<string, DeckInfo, less_ci> decks;
public:
	//native filename
	const string name;
	unordered_set<chatInfo> windows;
	//设置
	AttrVars conf;

	DiceSession(const string& s) : name(s) {
		tUpdate = tCreate = time(nullptr);
	}
	friend class DiceSessionManager;

	//记录创建时间
	time_t tCreate;
	//最后更新时间
	time_t tUpdate;

	DiceSession& create(time_t tt) {
		tCreate = tt;
		return *this;
	}

	DiceSession& update(time_t tt) {
		tUpdate = tt;
		return *this;
	}

	DiceSession& update()
	{
		tUpdate = time(nullptr);
		save();
		return *this;
	}
	void setConf(const string& key, const AttrVar& val) {
		conf[key] = val;
		update();
	}
	void rmConf(const string& key) {
		if (conf.count(key)) {
			conf.erase(key);
			update();
		}
	}

	[[nodiscard]] bool table_count(const string& key) const { return mTable.count(key); }
	bool table_del(const string&, const string&);
	int table_add(const string&, int, const string&);
	[[nodiscard]] string table_prior_show(const string& key) const;
	bool table_clr(const string& key);

	//旁观指令
	void ob_enter(DiceEvent*);
	void ob_exit(DiceEvent*);
	void ob_list(DiceEvent*) const;
	void ob_clr(DiceEvent*);
	[[nodiscard]] unordered_set<long long> get_ob() const { return sOB; }

	DiceSession& clear_ob()
	{
		sOB.clear();
		return *this;
	}
	
	//log指令
	void log_new(DiceEvent*);
	void log_on(DiceEvent*);
	void log_off(DiceEvent*);
	void log_end(DiceEvent*);
	[[nodiscard]] std::filesystem::path log_path()const;
	[[nodiscard]] bool is_logging() const { return logger.isLogging; }

	//deck指令
	map<string, DeckInfo, less_ci>& get_deck() { return decks; }
	DeckInfo& get_deck(const string& key) { return decks[key]; }
	void deck_set(DiceEvent*);
	string deck_draw(const string&);
	void _draw(DiceEvent*);
	void deck_show(DiceEvent*);
	void deck_reset(DiceEvent*);
	void deck_del(DiceEvent*);
	void deck_clr(DiceEvent*);
	void deck_new(DiceEvent*);
	[[nodiscard]] bool has_deck(const string& key) const { return decks.count(key); }

	void save() const;
};

using Session = DiceSession;

class DiceSessionManager {
	dict_ci<shared_ptr<Session>> SessionByName;
	//聊天窗口对Session，允许多对一
	unordered_map<chatInfo, shared_ptr<Session>> SessionByChat;
public:
	DiceChatLink linker;
	[[nodiscard]] bool is_linking(const chatInfo& ct) {
		auto chat{ ct.locate() };
		return chat && linker.LinkFromChat.count(chat) && linker.LinkFromChat[chat].second;
	}

	int load();
	void clear() { SessionByName.clear(); SessionByChat.clear(); linker = {}; }
	shared_ptr<Session> get(const chatInfo& ct);
	shared_ptr<Session> get_if(const chatInfo& ct) {
		auto chat{ ct.locate() };
		return chat && SessionByChat.count(chat) ? SessionByChat[chat] : shared_ptr<Session>();
	}
	bool has_session(const chatInfo& ct)const {
		return SessionByChat.count(ct.locate());
	}
	void end(chatInfo ct);

};
extern DiceSessionManager sessions;
