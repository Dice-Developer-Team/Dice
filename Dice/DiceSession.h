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
	//filestem, gbk
	string name;
	//filepath, gbk
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
struct DiceRoulette {
	//面数*
	size_t face{ 0 };
	size_t copy{ 0 };
	//剩余牌
	vector<size_t> pool;
	size_t sizRes{ 0 };
	DiceRoulette(){}
	DiceRoulette(size_t f, size_t c = 1);
	DiceRoulette(size_t f, size_t c, const vector<size_t>& p, size_t r) :face(f), copy(c), pool(p), sizRes(r) {};
	void reset();
	size_t roll();
	string hist();
};

class DiceSession{
	//管理员
	AttrSet master;
	//玩家
	AttrSet player;
	//旁观者
	AttrSet obs;
	//日志
	LogInfo logger;
	//牌堆
	dict_ci<DeckInfo> decks;
	void save() const;
public:
	//数值表
	AttrObject attrs;
	//native filename
	const string name;
	fifo_set<chatInfo> areas;
	fifo_map<size_t, DiceRoulette> roulette;
	size_t roll(size_t face);

	DiceSession(const string& s) : name(s),
		master(std::make_shared<fifo_set<AttrIndex>>()),
		player(std::make_shared<fifo_set<AttrIndex>>()),
		obs(std::make_shared<fifo_set<AttrIndex>>()) {
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
	string show()const;
	bool hasAttr(const string& key);
	AttrVar getAttr(const string& key);
	void setAttr(const string& key, const AttrVar& val) {
		attrs.set(key, val);
		update();
	}
	void rmAttr(const string& key) {
		if (attrs.has(key)) {
			attrs.reset(key);
			update();
		}
	}

	[[nodiscard]] AttrSet get_gm() const { return master; }
	[[nodiscard]] bool is_gm(long long uid) const { return master->count(uid); }
	void add_gm(long long uid) {
		master->emplace(uid);
		update();
	}
	bool del_gm(long long uid) {
		if (master->count(uid))master->erase(uid);
		else return false;
		update();
		return true;
	}
	[[nodiscard]] AttrSet get_pl() const { return player; }
	[[nodiscard]] bool is_pl(long long uid) const { return player->count(uid); }
	bool add_pl(long long uid) {
		if (!player->count(uid))player->emplace(uid);
		else return false;
		update();
		return true;
	}
	bool del_pl(long long uid) {
		if (player->count(uid))player->erase(uid);
		else return false;
		update();
		return true;
	}
	bool is_simple() const { return player->empty() && master->empty(); }
	bool is_part(long long uid) const { return player->count(uid) || master->count(uid); }
	bool del_ob(long long uid) {
		if (obs->count(uid))obs->erase(uid);
		else return false;
		update();
		return true;
	}

	[[nodiscard]] bool table_count(const string& key) const { return attrs.has(key); }
	bool table_del(const string&, const string&);
	bool table_add(const string&, int, const string&);
	[[nodiscard]] string table_prior_show(const string& key) const;
	bool table_clr(const string& key);

	//旁观指令
	void ob_enter(DiceEvent*);
	void ob_exit(DiceEvent*);
	void ob_list(DiceEvent*) const;
	void ob_clr(DiceEvent*);
	[[nodiscard]] AttrSet get_ob() const { return obs; }

	DiceSession& clear_ob(){
		obs->clear();
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
	dict_ci<DeckInfo>& get_deck() { return decks; }
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
};

using Session = DiceSession;

class DiceSessionManager {
	dict_ci<shared_ptr<Session>> SessionByName;
	//聊天窗口对Session，允许多对一
	unordered_map<chatInfo, shared_ptr<Session>> SessionByChat;
	int inc = 0;
public:
	DiceChatLink linker;
	[[nodiscard]] bool is_linking(const chatInfo& ct) {
		auto chat{ ct.locate() };
		return chat && linker.LinkFromChat.count(chat) && linker.LinkFromChat[chat].second;
	}

	int load();
	void save();
	void clear() { SessionByName.clear(); SessionByChat.clear(); linker = {}; }
	shared_ptr<Session> get(chatInfo);
	shared_ptr<Session> newGame(const string& name, const chatInfo& ct);
	shared_ptr<Session> get_if(const chatInfo& ct)const {
		auto chat{ ct.locate() };
		return chat && SessionByChat.count(chat) ? SessionByChat.at(chat) : shared_ptr<Session>();
	}
	shared_ptr<Session> getByName(const string& name)const {
		return SessionByName.count(name) ? SessionByName.at(name) : ptr<Session>();
	}
	bool has_session(const chatInfo& ct)const {
		return SessionByChat.count(ct.locate());
	}
	void open(const ptr<Session>& game, chatInfo ct);
	void close(chatInfo ct);
	void over(chatInfo ct);
};
extern DiceSessionManager sessions;
