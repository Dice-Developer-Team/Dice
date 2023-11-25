/**
 * 会话管理
 * 抽象于聊天窗口的单位
 * Copyright (C) 2019-2023 String.Empty
 */
#include <shared_mutex>
#include "filesystem.hpp"
#include "Jsonio.h"
#include "DiceSession.h"
#include "MsgFormat.h"
#include "EncodingConvert.h"
#include "DiceEvent.h"
#include "CardDeck.h"
#include "RandomGenerator.h"
#include "OneBotAPI.h"
#include "DiceMod.h"

DiceSessionManager sessions;
std::recursive_mutex sessionMutex;
#define LOCK_REC(ex) std::lock_guard<std::recursive_mutex> lock(ex) 
unordered_set<chatInfo>LogList;

const std::filesystem::path LogInfo::dirLog{ std::filesystem::path("user") / "log" };

size_t DiceSession::roll(size_t face) {
	if (roulette.count(face)) {
		auto res = roulette[face].roll();
		update();
		return res;
	} 
	return RandomGenerator::Randint(1, face);
}
string DiceSession::show()const {
	ShowList li;
	li << "桌号: " + name;
	if (attrs.has("rule"))li << "规则: " + attrs.get_str("rule");
	if (is_logging())li << "日志记录中:" + logger.name;
	if (!master->empty()) {
		ShowList sub;
		for (auto& id : *master) {
			sub << printUser((long long)id.to_double());
		}
		li << "GM: " + sub.show(" & ");
	}
	if (!player->empty()) {
		string sub{ "PL:" };
		for (auto& id : *player) {
			sub += "\n" + printUser((long long)id.to_double());
		}
		li << sub;
	}
	if (!obs->empty()) {
		string sub{ "OB:" };
		for (auto& id : *obs) {
			sub += "\n" + printUser((long long)id.to_double());
		}
		li << sub;
	}
	if (!roulette.empty()) {
		ShowList faces;
		for (auto& [face, rou] : roulette) {
			faces << to_string(face);
		}
		li << "轮盘骰: D" + faces.show("/");
	}
	return li.show("\n");
}
bool DiceSession::hasAttr(const string& key) {
	return attrs.has(key);
}
AttrVar DiceSession::getAttr(const string& key) {
	return attrs.get(key);
}

bool DiceSession::table_del(const string& tab, const string& item) {
	if (!attrs.has(tab) || !attrs.get_obj(tab).has(item))return false;
	attrs.get_obj(tab).reset(item);
	update();
	return true;
}

bool DiceSession::table_add(const string& tab, int prior, const string& item) {
	if (!attrs.has(tab))attrs.set(tab, AttrObject());
	attrs.get_obj(tab).set(item,prior);
	update();
	return true;
}

string DiceSession::table_prior_show(const string& tab) const{
	return attrs.is_table(tab) ? PriorList<AttrVar>(*attrs.get_dict(tab)).show() : "";
}

bool DiceSession::table_clr(const string& tab)
{
	if (attrs.has(tab)){
		attrs.reset(tab);
		update();
		return true;
	}
	return false;
}

void DiceSession::ob_enter(DiceEvent* msg)
{
	if (obs->count(msg->fromChat.uid))
	{
		msg->replyMsg("strObEnterAlready");
	}
	else
	{
		obs->insert(msg->fromChat.uid);
		msg->replyMsg("strObEnter");
		update();
	}
}

void DiceSession::ob_exit(DiceEvent* msg)
{
	if (del_ob(msg->fromChat.uid)){
		msg->replyMsg("strObExit");
	}
	else
	{
		msg->replyMsg("strObExitAlready");
	}
	update();
}

void DiceSession::ob_list(DiceEvent* msg) const
{
	if (obs->empty())msg->replyMsg("strObListEmpty");
	else
	{
		ResList res;
		for (auto& uid : *obs){
			res << printUser((long long)uid.to_double());
		}
		msg->reply(getMsg("strObList") + res.linebreak().show());
	}
}

void DiceSession::ob_clr(DiceEvent* msg)
{
	if (obs->empty())msg->replyMsg("strObListEmpty");
	else
	{
		obs->clear();
		msg->replyMsg("strObListClr");
	}
	update();
}

void DiceSession::log_new(DiceEvent* msg) {
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / logger.dirLog, ec);
	logger.tStart = time(nullptr);
	string nameLog{ msg->readFileName() };
	if (nameLog.empty())nameLog = to_string(logger.tStart);
	msg->set("log_name", logger.name = nameLog);
	logger.fileLog = name + "_" + nameLog + ".txt";
	logger.pathLog = DiceDir / logger.dirLog / GBKtoLocal(logger.fileLog);
	logger.isLogging = true;
	//先发消息后插入
	msg->replyMsg("strLogNew");
	for (const auto& ct : areas) {
		LogList.insert(ct);
	}
	update();
}
void DiceSession::log_on(DiceEvent* msg) {
	if (!logger.tStart) {
		log_new(msg);
		return;
	}
	if (string nameLog{ msg->readFileName() }; !nameLog.empty() && nameLog != logger.name) {
		logger.tStart = time(nullptr);
		logger.name = nameLog;
		logger.fileLog = LocaltoGBK(name) + "_" + nameLog + ".txt";
		logger.pathLog = DiceDir / logger.dirLog / GBKtoLocal(logger.fileLog);
	}
	else if (logger.isLogging) {
		msg->replyMsg("strLogOnAlready");
		return;
	}
	(*msg)["log_name"] = logger.name;
	logger.isLogging = true;
	msg->replyMsg("strLogOn");
	for (const auto& ct : areas) {
		LogList.insert(ct);
	}
	update();
}
void DiceSession::log_off(DiceEvent* msg) {
	if (!logger.tStart) {
		msg->replyMsg("strLogNullErr");
		return;
	}
	if (!logger.isLogging) {
		msg->replyMsg("strLogOffAlready");
		return;
	}
	logger.isLogging = false;
	//先擦除后发消息
	for (const auto& ct : areas) {
		LogList.erase(ct);
	}
	msg->replyMsg("strLogOff");
	update();
}
void DiceSession::log_end(DiceEvent* msg) {
	if (!logger.tStart) {
		msg->replyMsg("strLogNullErr");
		return;
	}
	for (const auto& ct : areas) {
		LogList.erase(ct);
	}
	logger.isLogging = false;
	logger.tStart = 0;
	if (std::filesystem::path pathFile(log_path()); !std::filesystem::exists(pathFile)) {
		msg->replyMsg("strLogEndEmpty");
		return;
	}
	(*msg)["log_file"] = logger.fileLog;
	(*msg)["log_path"] = log_path().string();
	msg->replyMsg("strLogEnd");
	update();
	msg->set("hook","LogEnd");
	if (!fmt->call_hook_event(*msg)) {
		msg->set("cmd", "uplog");
		sch.push_job(*msg);
	}
}
std::filesystem::path DiceSession::log_path()const {
	return logger.pathLog; 
}
/*
dict<> prep{
		{"to","->"},
		{"from","<-"},
		{"with","<->"},
};*/

DeckInfo::DeckInfo(const vector<string>& deck) :meta(deck) {
	init();
}
void DeckInfo::init() {
	idxs.reserve(meta.size());
	for (size_t idx = 0; idx < meta.size(); idx++) {
		size_t l, r;
		size_t cnt(1);
		if ((l = meta[idx].find("::")) != string::npos && (r = meta[idx].find("::", l + 2)) != string::npos) {
			string strCnt = CardDeck::draw(meta[idx].substr(l + 2, r - l - 2));
			if (strCnt.length() < 4 && !strCnt.empty() && isdigit(static_cast<unsigned char>(strCnt[0])) && strCnt != "0") {
				meta[idx].erase(meta[idx].begin(), meta[idx].begin() + r + 2);
				cnt = stoi(strCnt);
			}
		}
		while (cnt--) {
			idxs.push_back(idx);
		}
	}
	sizRes = idxs.size();
}
void DeckInfo::reset() {
	sizRes = idxs.size();
}
string DeckInfo::draw() {
	if (idxs.empty())return{};
	size_t idx = RandomGenerator::Randint(0, --sizRes);
	size_t res = idxs[idx];
	idxs[idx] = idxs[sizRes];
	idxs[sizRes] = res;
	return CardDeck::draw(meta[res]);
}
DiceRoulette::DiceRoulette(size_t f, size_t c) :face(f), copy(c) {
	pool.reserve(sizRes = face * copy);
	for (size_t idx = 1; idx <= face; ++idx) {
		for (size_t cnt = 0; cnt < copy; ++cnt) {
			pool.push_back(idx);
		}
	}
}
void DiceRoulette::reset() {
	sizRes = pool.size();
}
size_t DiceRoulette::roll() {
	if (!sizRes)sizRes = pool.size();
	size_t idx = RandomGenerator::Randint(0, --sizRes);
	size_t die = pool[idx];
	pool[idx] = pool[sizRes];
	pool[sizRes] = die;
	return die;
}
string DiceRoulette::hist() {
	ShowList his;
	size_t begin = face * copy;
	while (begin > sizRes) {
		his << to_string(pool[--begin]);
	}
	return his.show(" ");
}

void DiceSession::deck_set(DiceEvent* msg) {
	const string key{ (msg->at("deck_name") = msg->readAttrName()).to_str() };
	size_t pos = msg->strMsg.find('=', msg->intMsgCnt);
	AttrVar& strCiteDeck{ msg->at("deck_cited") = (pos == string::npos)
		? key 
		: (++msg->intMsgCnt, msg->readAttrName()) };
	if (key.empty()) {
		msg->replyMsg("strDeckNameEmpty");
	}
	else if (decks.size() > 9 && !decks.count(key)) {
		msg->replyMsg("strDeckListFull");
	}
	else {
		vector<string> DeckSet = {};
		if (strCiteDeck == "range") {
			string strL = msg->readDigit();
			string strR = msg->readDigit();
			string strStep = msg->readDigit();	//步长，不支持反向
			if (strL.empty()) {
				msg->replyMsg("strRangeEmpty");
				return;
			}
			else if (strL.length() > 16 || strR.length() > 16) {
				msg->replyMsg("strOutRange");
				return;
			}
			else {
				long long llBegin{ stoll(strL) };
				long long llEnd{ strR.empty() ? 0 : stoll(strR) };
				long long llStep{ strStep.empty() ? 1 : stoll(strStep) };
				if (llBegin == llEnd) {
					msg->replyMsg("strRangeEmpty");
					return;
				}
				else if (llBegin > llEnd) {
					long long temp = llBegin;
					llBegin = llEnd;
					llEnd = temp;
				}
				if ((llEnd - llBegin) > 1000 * llStep) {
					msg->replyMsg("strOutRange");
					return;
				}
				for (long long card = llBegin; card <= llEnd; card += llStep) {
					DeckSet.emplace_back(to_string(card));
				}
				decks[key] = DeckSet;
				strCiteDeck = strCiteDeck.to_str() + to_string(llBegin) + "~" + *(--DeckSet.end());
			}
		}
		else if (!CardDeck::mPublicDeck.count(strCiteDeck.to_str()) || strCiteDeck.to_str()[0] == '_') {
			msg->replyMsg("strDeckCiteNotFound");
			return;
		}
		else {
			decks[key] = CardDeck::mPublicDeck[strCiteDeck.to_str()];
		}
		if (key == strCiteDeck.to_str())msg->replyMsg("strDeckSet");
		else msg->replyMsg("strDeckSetRename");
		update();
	}
}
void DiceSession::deck_new(DiceEvent* msg) {
	AttrVar& key{ (*msg)["deck_name"] };
	size_t pos = msg->strMsg.find('=', msg->intMsgCnt);
	if (pos == string::npos) {
		key = "new";
	}
	else {
		key = msg->readAttrName();
		msg->intMsgCnt = pos + 1;
	}
	if (decks.size() > 9 && !decks.count(key.to_str())) {
		msg->replyMsg("strDeckListFull");
	}
	else {
		vector<string> DeckSet = {};
		msg->readItems(DeckSet);
		if (DeckSet.empty()) {
			msg->replyMsg("strDeckNewEmpty");
			return;
		}
		else if (DeckSet.size() > 256) {
			msg->replyMsg("strDeckOversize");
			return;
		}
		DeckInfo deck{ DeckSet };
		if (deck.idxs.size() > 1024) {
			msg->replyMsg("strDeckOversize");
			return;
		}
		decks[key.to_str()] = std::move(deck);
		msg->replyMsg("strDeckNew");
		update();
	}
}
string DiceSession::deck_draw(const string& key) {
	if (decks.count(key)) {
		if (!decks[key].sizRes)return getMsg("strDeckRestEmpty", AttrVars{ {"deck_name",key} });
		return decks[key].draw();
	}
	else if (CardDeck::mPublicDeck.count(key)) {
		vector<string>& deck = CardDeck::mPublicDeck[key];
		return CardDeck::draw(deck[RandomGenerator::Randint(0, deck.size() - 1)]);
	}
	return "{key}";
}
void DiceSession::_draw(DiceEvent* msg) {
	if (msg->is_empty("deck_name"))msg->set("deck_name",msg->readAttrName());
	DeckInfo& deck = decks[msg->get_str("deck_name")];
	int intCardNum = 1;
	switch (msg->readNum(intCardNum)) {
	case 0:
		if (intCardNum == 0) {
			msg->replyMsg("strNumCannotBeZero");
			return;
		}
		break;
	case -1: break;
	case -2:
		msg->replyMsg("strParaIllegal");
		console.log("提醒:" + printUser(msg->fromChat.uid) + "对" + getMsg("strSelfName") + "使用了非法指令参数\n" + msg->strMsg, 1,
					printSTNow());
		return;
	}
	ResList Res;
	while (deck.sizRes && intCardNum--) {
		Res << deck.draw();
		if (!deck.sizRes)break;
	}
	if(!Res.empty()){
		msg->set("res",Res.dot("|").show());
		msg->set("cnt",to_string(Res.size()));
		if (msg->is("hidden")) {
			msg->replyMsg("strDrawHidden");
			msg->replyHidden(getMsg("strDrawCard"));
		}
		else
			msg->replyMsg("strDrawCard");
		update();
	}
	if (!deck.sizRes) {
		msg->replyMsg("strDeckRestEmpty");
	}
}
void DiceSession::deck_show(DiceEvent* msg) {
	if (decks.empty()) {
		msg->replyMsg("strDeckListEmpty");
		return;
	}
	const string& strDeckName{ ((*msg)["deck_name"] = msg->readAttrName()).text };
	//默认列出所有牌堆
	if (strDeckName.empty()) {
		ResList res;
		for (auto& [key, val] : decks) {
			res << key + "[" + to_string(val.sizRes) + "/" + to_string(val.idxs.size()) + "]";
		}
		msg->set("res", res.show());
		msg->replyMsg("strDeckListShow");
	}
	else {
		if (decks.count(strDeckName)) {
			DeckInfo& deck{ decks[strDeckName] };
			ResList residxs;
			size_t idx(0);
			while (idx < deck.sizRes) {
				residxs << deck.meta[deck.idxs[idx++]];
			}
			msg->set("deck_rest", residxs.dot(" | ").show());
			msg->replyMsg("strDeckRestShow");
		}
		else {
			msg->replyMsg("strDeckNotFound");
		}
	}
}
void DiceSession::deck_reset(DiceEvent* msg) {
	AttrVar& key{ (*msg)["deck_name"] = msg->readAttrName() };
	if (key.str_empty())key = msg->readDigit();
	if (key.str_empty()) {
		msg->replyMsg("strDeckNameEmpty");
	}
	else if (!decks.count(key.to_str())) {
		msg->replyMsg("strDeckNotFound");
	}
	else {
		decks[key.to_str()].reset();
		msg->replyMsg("strDeckRestReset");
		update();
	}
}
void DiceSession::deck_del(DiceEvent* msg) {
	AttrVar& key{ (*msg)["deck_name"] = msg->readAttrName() };
	if (key.str_empty())key = msg->readDigit();
	if (key.str_empty()) {
		msg->replyMsg("strDeckNameEmpty");
	}
	else if (!decks.count(key.to_str())) {
		msg->replyMsg("strDeckNotFound");
	}
	else {
		decks.erase(key.to_str());
		msg->replyMsg("strDeckDelete");
		update();
	}
}
void DiceSession::deck_clr(DiceEvent* msg) {
	decks.clear();
	msg->replyMsg("strDeckListClr");
	update();
}

std::mutex exSessionSave;

void DiceSession::save() const
{
	std::error_code ec;
	std::filesystem::create_directories(DiceDir / "user" / "session", ec);
	std::filesystem::path fpFile{ DiceDir / "user" / "session" / (name + ".json") };
	fifo_json jData;
	if (!master->empty())jData["master"] = to_json(*master);
	if (!player->empty())jData["player"] = to_json(*player);
	if (!obs->empty())jData["observer"] = to_json(*obs);
	if (logger.tStart || !logger.fileLog.empty()) {
		fifo_json jLog;
		jLog["start"] = logger.tStart;
		jLog["lastMsg"] = logger.tLastMsg;
		jLog["name"] = logger.name;
		jLog["file"] = logger.fileLog;
		jLog["logging"] = logger.isLogging;
		jData["log"] = jLog;
	}
	if (!decks.empty()) {
		fifo_json jDecks;
		for (auto& [key,deck]:decks) {
			jDecks[key] = {
				{"meta",deck.meta},
				{"idxs",deck.idxs},
				{"size",deck.sizRes}
			};
		}
		jData["decks"] = jDecks;
	}
	if (!roulette.empty()) {
		fifo_json jDecks;
		for (auto& [face, rou] : roulette) {
			jDecks[to_string(face)] = {
				{"copy",rou.copy},
				{"pool",rou.pool},
				{"rest",rou.sizRes}
			};
		}
		jData["roulette"] = jDecks;
	}
	if (!attrs.empty())jData["data"] = attrs.to_json();
	std::lock_guard<std::mutex> lock(exSessionSave);
	if (jData.empty()) {
		remove(fpFile);
		return;
	}
	auto& jChat{ jData["chats"] = fifo_json::array() };
	for (const auto& chat : areas) {
		jChat.push_back(to_json(chat));
	}
	jData["create_time"] = tCreate;
	jData["update_time"] = tUpdate;
	fwriteJson(fpFile, jData, 1);
}

void DiceSessionManager::open(const ptr<Session>& game, chatInfo ct) {
	LOCK_REC(sessionMutex);
	if (auto session{ get_if(ct = ct.locate()) }) {
		session->areas.erase(ct);
		session->update();
	}
	SessionByChat[ct] = game;
	game->areas.insert(ct);
	game->update();
}
void DiceSessionManager::close(chatInfo ct) {
	if (auto session{ get_if(ct = ct.locate()) }) {
		LOCK_REC(sessionMutex);
		SessionByChat.erase(ct);
		session->update();
	}
}
void DiceSessionManager::over(chatInfo ct){
	auto session{ get_if(ct = ct.locate()) };
	if (!session)return;
	LOCK_REC(sessionMutex);
	SessionByName.erase(session->name);
	for (auto& it:session->areas) {
		SessionByChat.erase(it);
	}
	remove(DiceDir / "user" / "session" / (session->name + ".json"));
}
//const enumap<string> mSMTag{"type", "room", "gm", "log", "player", "observer", "tables"};

shared_ptr<Session> DiceSessionManager::newGame(const string& name, const chatInfo& ct) {
	LOCK_REC(sessionMutex);
	string g_name{ name + "#" + to_string(++inc) };
	while (SessionByName.count(g_name)) {
		g_name = name + "#" + to_string(++inc);
	}
	auto ptr{ std::make_shared<Session>(g_name) };
	const auto here{ ct.locate() };
	ptr->areas.insert(here);
	ptr->add_gm(ct.uid);
	SessionByName[g_name] = ptr;
	if (SessionByChat.count(here))SessionByChat[here]->areas.erase(here);
	if (LogList.count(here)) LogList.erase(here);
	SessionByChat[here] = ptr;
	save();
	return ptr;
}
shared_ptr<Session> DiceSessionManager::get(chatInfo ct) {
	if (SessionByChat.count(ct = ct.locate()))
		return SessionByChat[ct];
	else {
		string name{ ct.gid ? "g" + to_string(ct.gid) +
			(ct.chid ? "_ch" + to_string(ct.chid) : "")
			: "usr" + to_string(ct.uid) };
		LOCK_REC(sessionMutex);
		if (!SessionByName.count(name)) {
			auto ptr{ std::make_shared<Session>(name) };
			ptr->areas.insert(ct);
			SessionByName[name] = ptr;
			SessionByChat[ct] = ptr;
			return ptr;
		}
		else return SessionByChat[ct] = SessionByName[name];
	}
}
int DiceSessionManager::load() {
	if (auto fileGM{ DiceDir / "user" / "GameTable.toml" };std::filesystem::exists(fileGM)) {
		if (ifstream ifs{ fileGM }) {
			AttrObject cfg = AttrVar(toml::parse(ifs)).to_obj();
			inc = cfg.get_int("inc");
		}
	}
	LOCK_REC(sessionMutex);
	vector<std::filesystem::path> sFile;
	int cnt = listDir(DiceDir / "user" / "session", sFile);
	if (cnt > 0)for (auto& filename : sFile) {
		fifo_json j = freadJson(filename);
		if (j.is_null()) {
			remove(filename);
			--cnt;
			continue;
		}
		string name{ filename.stem().u8string() };
		auto pSession(std::make_shared<Session>(name));
		bool isUpdated{ false };
		try {
			pSession->create(j["create_time"]).update(j["update_time"]);
			if (j.count("room")) {
				if (j["room"].is_number()) {
					long long id{ j["room"] };
					pSession->areas.insert(id > 0 ? chatInfo{ 0, id } : chatInfo{ ~id });
				}
				else if (j["room"].is_array()) {
					for (auto& ct : j["room"]) {
						pSession->areas.insert(chatInfo::from_json(ct));
					}
				}
			}
			if (j.count("chats")) {
				for (auto& ct : j["chats"]) {
					pSession->areas.insert(chatInfo::from_json(ct));
				}
			}
			if (j.count("conf")) pSession->attrs = AttrVar(j["conf"]).to_obj();
			if (j.count("log")) {
				fifo_json& jLog = j["log"];
				jLog["start"].get_to(pSession->logger.tStart);
				jLog["lastMsg"].get_to(pSession->logger.tLastMsg);
				if (jLog.count("name"))pSession->logger.name = jLog["name"].get<string>();
				else pSession->logger.name = to_string(pSession->logger.tStart);
				jLog["file"].get_to(pSession->logger.fileLog);
				jLog["logging"].get_to(pSession->logger.isLogging);
				pSession->logger.update();
				pSession->logger.pathLog = DiceDir / pSession->logger.dirLog / GBKtoLocal(pSession->logger.fileLog);
				if (pSession->logger.isLogging) {
					for (const auto& chat : pSession->areas) {
						LogList.insert(chat);
					}
				}
			}
			if (j.count("decks")) for (auto& it : j["decks"].items()) {
				if (it.value()["meta"].empty())continue;
				std::string key = it.key();
				auto& deck{ pSession->decks[key] };
				it.value()["meta"].get_to(deck.meta);
				if (it.value().count("rest")) {
					it.value()["rest"].get_to(pSession->decks[key].idxs);
					pSession->decks[key].sizRes = pSession->decks[key].idxs.size();
				}
				else {
					it.value()["idxs"].get_to(pSession->decks[key].idxs);
					it.value()["size"].get_to(pSession->decks[key].sizRes);
				}
			}
			if(j.count("roulette"))for (auto& it : j["roulette"].items()) {
				size_t face = stoi(it.key());
				pSession->roulette.emplace(face, DiceRoulette(face, it.value()["copy"],
					it.value()["pool"].get<vector<size_t>>(), it.value()["rest"]));
			}
			if (j.count("tables"))for (auto& it : j["tables"].items()) {
				pSession->attrs.set(it.key(), it.value());
			}
			if (j.count("data"))for (auto& it : j["data"].items()) {
				pSession->attrs.set(it.key(), it.value());
			}
			if (j.count("master"))for (auto& it : j["master"]) {
				pSession->master->emplace(it.get<long long>());
			}
			if (j.count("player"))for (auto& it : j["player"]) {
				pSession->player->emplace(it.get<long long>());
			}
			if (j.count("observer"))for (auto& it : j["observer"]) {
				pSession->obs->emplace(it.get<long long>());
			}
		}
		catch (std::exception& e) {
			console.log("读取session文件" + filename.u8string() + "出错!" + e.what(), 1);
		}
		SessionByName[name] = pSession;
		for (const auto& chat : pSession->areas) {
			SessionByChat[chat] = pSession;
		}
		if (isUpdated)pSession->save();
	}
	return cnt;
}
void DiceSessionManager::save() {
	if (std::ofstream fs{ DiceDir / "user" / "GameTable.toml" }) {
		AttrObject cfg = AttrVars{ { "inc",inc } };
		fs << cfg.to_toml();
	}
}