/**
 * 会话管理
 * 抽象于聊天窗口的单位
 * Copyright (C) 2019-2022 String.Empty
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
#include "DDAPI.h"
#include "DiceMod.h"

DiceSessionManager sessions;
std::shared_mutex sessionMutex;
unordered_set<chatInfo>LogList;

const std::filesystem::path LogInfo::dirLog{ std::filesystem::path("user") / "log" };

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
	if (sOB.count(msg->fromChat.uid))
	{
		msg->replyMsg("strObEnterAlready");
	}
	else
	{
		sOB.insert(msg->fromChat.uid);
		msg->replyMsg("strObEnter");
		update();
	}
}

void DiceSession::ob_exit(DiceEvent* msg)
{
	if (sOB.count(msg->fromChat.uid))
	{
		sOB.erase(msg->fromChat.uid);
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
	if (sOB.empty())msg->replyMsg("strObListEmpty");
	else
	{
		ResList res;
		for (auto uid : sOB)
		{
			res << printUser(uid);
		}
		msg->reply(getMsg("strObList") + res.linebreak().show());
	}
}

void DiceSession::ob_clr(DiceEvent* msg)
{
	if (sOB.empty())msg->replyMsg("strObListEmpty");
	else
	{
		sOB.clear();
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
	(*msg)["log_name"] = logger.name = nameLog;
	logger.fileLog = LocaltoGBK(name) + "_" + nameLog + ".txt";
	logger.pathLog = DiceDir / logger.dirLog / GBKtoLocal(logger.fileLog);
	logger.isLogging = true;
	//先发消息后插入
	msg->replyMsg("strLogNew");
	for (const auto& ct : windows) {
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
	for (const auto& ct : windows) {
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
	for (const auto& ct : windows) {
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
	for (const auto& ct : windows) {
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

void DiceChatLink::load() {
	if (fifo_json jFile{ freadJson(DiceDir / "conf" / "LinkList.json") }; !jFile.is_null()){
		for (auto& jLink : jFile) {
			LinkInfo& link{ LinkList[chatInfo::from_json(jLink["origin"])]
				= { jLink["linking"], jLink["type"],
				chatInfo::from_json(jLink["target"]) } };
		}
	}
	else if (!LinkList.empty())save();
	for (auto& [ct,link] : LinkList) {
		if (link.isLinking) {
			LinkFromChat[ct] = { link.target ,link.typeLink != "from" };
			LinkFromChat[link.target] = { ct ,link.typeLink != "to" };
		}
	}
}
void DiceChatLink::save() {
	if (LinkList.empty()) {
		remove(DiceDir / "conf" / "LinkList.json");
		return;
	}
	fifo_json jFile = fifo_json::array();
	for (auto& [ct, linker] : LinkList) {
		fifo_json jLink;
		jLink["type"] = linker.typeLink;
		jLink["origin"] = to_json(ct);
		jLink["target"] = to_json(linker.target);
		jLink["linking"] = linker.isLinking;
		jFile.push_back(jLink);
	}
	fwriteJson(DiceDir / "conf" / "LinkList.json", jFile, 1);
}
void DiceChatLink::build(DiceEvent* msg) {
	auto here{ msg->fromChat.locate() };
	chatInfo target;
	if (msg->readChat(target)) {
		msg->replyMsg("strLinkNotFound");
		return;
	}
	else if (target.gid && !ChatList.count(target.gid)) {
		msg->replyMsg("strLinkNotFound");
		return;
	}
	if (LinkFromChat.count(target)) {
		msg->replyMsg("strLinkBusy");
	}
	else {
		LinkInfo& link{ LinkList[here] };
		//重置已存在的链接
		LinkFromChat.erase(link.target);
		link = { true ,msg->get_str("option"),target };
		LinkFromChat[here] = { target ,link.typeLink != "from" };
		LinkFromChat[target] = { here ,link.typeLink != "to" };
		msg->set("target", printChat(target));
		msg->replyMsg("strLinked");
		save();
	}
}
void DiceChatLink::start(DiceEvent* msg) {
	auto here{ msg->fromChat.locate() };
	if (LinkList.count(here)) {
		if (LinkFromChat.count(here)) {
			msg->replyMsg("strLinkingAlready");
		}
		else if (LinkInfo& link{ LinkList[here] }; LinkFromChat.count(link.target)) {
			msg->replyMsg("strLinkBusy");
		}
		else {
			LinkFromChat[here] = { link.target ,link.typeLink != "from" };
			LinkFromChat[link.target] = { here ,link.typeLink != "to" };
			link.isLinking = true;
			msg->set("target", printChat(link.target));
			msg->replyMsg("strLinked");
			save();
		}
	}
	else {
		msg->replyMsg("strLinkNotFound");
	}
}
dict<> prep{
		{"to","->"},
		{"from","<-"},
		{"with","<->"},
};
string DiceChatLink::show(const chatInfo& here) {
	string info{ "[无]" };
	if (LinkList.count(here)) {
		const auto& link{ LinkList[here] };
		info = printChat(here) + prep[link.typeLink] + printChat(link.target)
			+ (link.isLinking ? "√" : "×");
	}
	else if (!LinkFromChat.count(here)) {
		return info;
	}
	if (auto link{ LinkList.find(LinkFromChat[here].first) }; link != LinkList.end()) {
		return printChat(LinkFromChat[here].first) + prep[link->second.typeLink] + printChat(here)
			+ (link->second.isLinking ? "√" : "×");
	}
	return info;
}
string DiceChatLink::list() {
	ShowList li;
	for (auto& [here,link]:LinkList) {
		li << printChat(here) + prep[link.typeLink] + printChat(link.target)
			+ (link.isLinking ? "√" : "×");
	}
	return li.show("\n");
}
void DiceChatLink::show(DiceEvent* msg) {
	auto here{ msg->fromChat.locate() };
	if (LinkFromChat.count(here) || LinkList.count(here)) {
		msg->set("link_info", show(here));
		msg->replyMsg("strLinkState");
	}
	else {
		msg->replyMsg("strLinkNotFound");
	}
}
void DiceChatLink::close(DiceEvent* msg) {
	auto here{ msg->fromChat.locate() };
	if (auto link{ LinkFromChat.find(here) }; link != LinkFromChat.end()) {
		auto there{ link->second.first };
		if(LinkList.count(here))LinkList[here].isLinking = false;
		else if (LinkList.count(there))LinkList[there].isLinking = false;
		LinkFromChat.erase(link);
		LinkFromChat.erase(there);
		msg->replyMsg("strLinkClose");
		save();
	}
	else {
		msg->replyMsg("strLinkCloseAlready");
	}
}

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
		if ((strCiteDeck == "群成员" || (strCiteDeck == "member" && !(strCiteDeck = "群成员").str_empty())) && !msg->isPrivate()) {
			
			if (auto list{ DD::getGroupMemberList(msg->fromChat.gid) }; list.empty()) {
				msg->reply("群成员列表获取失败×");
			}
			else for (auto each : list) {
				DeckSet.push_back(printUser(each));
			}
			decks[key] = DeckSet;
		}
		else if (strCiteDeck == "range") {
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
	if (!conf.empty()) {
		fifo_json& jConf{ jData["conf"] };
		for (auto& [key, val] : conf) {
			jConf[GBKtoUTF8(key)] = val.to_json();
		}
	}
	if (!sOB.empty())jData["observer"] = sOB;
	if (!attrs.empty())jData["tables"] = attrs.to_json();
	if (logger.tStart || !logger.fileLog.empty()) {
		fifo_json jLog;
		jLog["start"] = logger.tStart;
		jLog["lastMsg"] = logger.tLastMsg;
		jLog["name"] = GBKtoUTF8(logger.name);
		jLog["file"] = GBKtoUTF8(logger.fileLog);
		jLog["logging"] = logger.isLogging;
		jData["log"] = jLog;
	}
	if (!decks.empty()) {
		fifo_json jDecks;
		for (auto& [key,deck]:decks) {
			jDecks[GBKtoUTF8(key)] = {
				{"meta",GBKtoUTF8(deck.meta)},
				{"idxs",deck.idxs},
				{"size",deck.sizRes}
			};
		}
		jData["decks"] = jDecks;
	}
	std::lock_guard<std::mutex> lock(exSessionSave);
	if (jData.empty()) {
		remove(fpFile);
		return;
	}
	auto& jChat{ jData["chats"] = fifo_json::array() };
	for (const auto& chat : windows) {
		jChat.push_back(to_json(chat));
	}
	jData["create_time"] = tCreate;
	jData["update_time"] = tUpdate;
	fwriteJson(fpFile, jData, 1);
}

void DiceSessionManager::end(chatInfo ct){
	auto session{ get_if(ct = ct.locate()) };
	if (!session)return;
	std::unique_lock<std::shared_mutex> lock(sessionMutex);
	SessionByName.erase(session->name);
	if (session->windows.erase(ct); session->windows.empty()) {
		SessionByChat.erase(ct);
		remove(DiceDir / "user" / "session" / (session->name + ".json"));
	}
}

const enumap<string> mSMTag{"type", "room", "gm", "log", "player", "observer", "tables"};

shared_ptr<Session> DiceSessionManager::get(const chatInfo& ct) {
	if (const auto here{ ct.locate() }; SessionByChat.count(here)) 
		return SessionByChat[here];
	else {
		string name{ ct.chid ? "ch" + to_string(ct.chid)
			: ct.gid ? "g" + to_string(ct.gid)
			: "usr" + to_string(ct.uid) };
		while (SessionByName.count(name)) {
			name += '+';
		}
		std::unique_lock<std::shared_mutex> lock(sessionMutex);
		auto ptr{ std::make_shared<Session>(name)};
		ptr->windows.insert(here);
		SessionByName[name] = ptr;
		SessionByChat[here] = ptr;
		return ptr;
	}
}
int DiceSessionManager::load() {
	string strLog;
	std::unique_lock<std::shared_mutex> lock(sessionMutex);
	vector<std::filesystem::path> sFile;
	int cnt = listDir(DiceDir / "user" / "session", sFile);
	if (cnt > 0)for (auto& filename : sFile) {
		fifo_json j = freadJson(filename);
		if (j.is_null()) {
			remove(filename);
			--cnt;
			continue;
		}
		auto pSession(std::make_shared<Session>(filename.stem().string()));
		bool isUpdated{ false };
		try {
			pSession->create(j["create_time"]).update(j["update_time"]);
			if (j.count("room")) {
				if (j["room"].is_number()) {
					long long id{ j["room"] };
					pSession->windows.insert(id > 0 ? chatInfo{ 0, id } : chatInfo{ ~id });
				}
				else if (j["room"].is_array()) {
					for (auto& ct : j["room"]) {
						pSession->windows.insert(chatInfo::from_json(ct));
					}
				}
			}
			if (j.count("chats")) {
				for (auto& ct : j["chats"]) {
					pSession->windows.insert(chatInfo::from_json(ct));
				}
			}
			if (j.count("conf")) {
				fifo_json& jConf{ j["conf"] };
				for (auto& it : jConf.items()) {
					pSession->conf.emplace(UTF8toGBK(it.key()), it.value());
				}
			}
			if (j.count("log")) {
				fifo_json& jLog = j["log"];
				jLog["start"].get_to(pSession->logger.tStart);
				jLog["lastMsg"].get_to(pSession->logger.tLastMsg);
				if (jLog.count("name"))pSession->logger.name = UTF8toGBK(jLog["name"].get<string>());
				else pSession->logger.name = to_string(pSession->logger.tStart);
				pSession->logger.fileLog = UTF8toGBK(jLog["file"].get<string>());
				jLog["logging"].get_to(pSession->logger.isLogging);
				pSession->logger.update();
				pSession->logger.pathLog = DiceDir / pSession->logger.dirLog / GBKtoLocal(pSession->logger.fileLog);
				if (pSession->logger.isLogging) {
					for (const auto& chat : pSession->windows) {
						LogList.insert(chat);
					}
				}
			}
			if (j.count("link")) {
				fifo_json& jLink = j["link"];
				const auto& ct{ *pSession->windows.begin() };
				long long gid{ jLink["target"] };
				LinkInfo& link{ linker.LinkList[ct] = {jLink["linking"],
					jLink["type"],
					(gid > 0) ? chatInfo{0,gid} : chatInfo{~gid} } };
				isUpdated = true;
			}
			if (j.count("decks")) {
				fifo_json& jDecks = j["decks"];
				for (auto it = jDecks.cbegin(); it != jDecks.cend(); ++it) {
					if (it.value()["meta"].empty())continue;
					std::string key = UTF8toGBK(it.key());
					auto& deck{ pSession->decks[key] };
					deck.meta = UTF8toGBK(it.value()["meta"].get<vector<string>>());
					if (it.value().count("rest")) {
						it.value()["rest"].get_to(pSession->decks[key].idxs);
						pSession->decks[key].sizRes = pSession->decks[key].idxs.size();
					}
					else {
						it.value()["idxs"].get_to(pSession->decks[key].idxs);
						it.value()["size"].get_to(pSession->decks[key].sizRes);
					}
				}
			}
			if (j.count("observer")) j["observer"].get_to(pSession->sOB);
			if (j.count("tables"))pSession->attrs = AttrVar(j["tables"]).to_obj();
		}
		catch (std::exception& e) {
			console.log("读取session文件" + UTF8toGBK(filename.u8string()) + "出错!" + e.what(), 1);
		}
		SessionByName[UTF8toGBK(filename.u8string())] = pSession;
		for (const auto& chat : pSession->windows) {
			SessionByChat[chat] = pSession;
		}
		if (isUpdated)pSession->save();
	}
	try {
		linker.load();
	}
	catch (const std::exception& e) {
		console.log("读取link记录时遇到意外错误，请尝试排除/conf/LinkList.json中的异常!"
			+ string(e.what()), 0b1000, printSTNow());
	}
	return cnt;
}