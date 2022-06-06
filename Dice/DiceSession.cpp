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

bool DiceSession::table_del(const string& tab, const string& item) {
	if (!mTable.count(tab) || !mTable[tab].count(item))return false;
	mTable[tab].erase(item);
	update();
	return true;
}

int DiceSession::table_add(const string& key, int prior, const string& item)
{
	mTable[key][item] = prior;
	update();
	return 0;
}

string DiceSession::table_prior_show(const string& key) const{
	return mTable.count(key) ? PriorList(mTable.at(key)).show() : "";
}

bool DiceSession::table_clr(const string& key)
{
	if (const auto it = mTable.find(key); it != mTable.end())
	{
		mTable.erase(it);
		update();
		return true;
	}
	return false;
}

void DiceSession::ob_enter(FromMsg* msg)
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

void DiceSession::ob_exit(FromMsg* msg)
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

void DiceSession::ob_list(FromMsg* msg) const
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

void DiceSession::ob_clr(FromMsg* msg)
{
	if (sOB.empty())msg->replyMsg("strObListEmpty");
	else
	{
		sOB.clear();
		msg->replyMsg("strObListClr");
	}
	update();
}

void DiceSession::log_new(FromMsg* msg) {
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / logger.dirLog, ec);
	string nameLog{ msg->readFileName() };
	if (nameLog.empty())nameLog = to_string(logger.tStart);
	msg->vars["log_name"] = nameLog;
	logger.tStart = time(nullptr);
	logger.isLogging = true;
	logger.fileLog = LocaltoGBK(name) + "_" + nameLog + ".txt";
	logger.pathLog = DiceDir / logger.dirLog / GBKtoLocal(logger.fileLog);
	//先发消息后插入
	msg->replyMsg("strLogNew");
	for (const auto& ct : windows) {
		LogList.insert(ct);
	}
	update();
}
void DiceSession::log_on(FromMsg* msg) {
	if (!logger.tStart) {
		log_new(msg);
		return;
	}
	if (logger.isLogging) {
		msg->replyMsg("strLogOnAlready");
		return;
	}
	logger.isLogging = true;
	msg->replyMsg("strLogOn");
	for (const auto& ct : windows) {
		LogList.insert(ct);
	}
	update();
}
void DiceSession::log_off(FromMsg* msg) {
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
void DiceSession::log_end(FromMsg* msg) {
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
	msg->vars["log_file"] = logger.fileLog;
	msg->vars["log_path"] = log_path().string();
	msg->replyMsg("strLogEnd");
	update();
	AttrObject eve{ *msg->vars };
	eve["Event"] = "LogEnd";
	if (!fmt->call_hook_event(eve)) {
		eve["cmd"] = "uplog";
		sch.push_job(eve);
	}
}
std::filesystem::path DiceSession::log_path()const {
	return logger.pathLog; 
}

void DiceChatLink::load() {
	json jFile{ freadJson(DiceDir / "conf" / "LinkList.json") };
	for (auto& jLink : jFile) {
		auto ct{ chatInfo::from_json(jLink["origin"]) };
		LinkInfo& link{ LinkList[ct] = { jLink["linking"].get<bool>(),
			jLink["type"].get<string>(),
			chatInfo::from_json(jLink["target"]) } };
	}
	for (auto& [ct,link] : LinkList) {
		if (link.isLinking) {
			LinkFromChat[ct] = { link.target ,link.typeLink != "from" };
			LinkFromChat[link.target] = { ct ,link.typeLink != "to" };
		}
	}
	if (jFile.empty() && !LinkList.empty())save();
}
void DiceChatLink::save() {
	if (LinkList.empty()) {
		remove(DiceDir / "conf" / "LinkList.json");
		return;
	}
	json jFile{ json::array() };
	for (auto& [ct, linker] : LinkList) {
		json jLink;
		jLink["type"] = linker.typeLink;
		jLink["origin"] = to_json(ct);
		jLink["target"] = to_json(linker.target);
		jLink["linking"] = linker.isLinking;
		jFile.push_back(jLink);
	}
	fwriteJson(DiceDir / "conf" / "LinkList.json", jFile, 0);
}
void DiceChatLink::build(FromMsg* msg) {
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
		link = { true ,msg->vars.get_str("option"),target };
		LinkFromChat[here] = { target ,link.typeLink != "from" };
		LinkFromChat[target] = { here ,link.typeLink != "to" };
		msg->vars["target"] = printChat(target);
		msg->replyMsg("strLinked");
		save();
	}
}
void DiceChatLink::start(FromMsg* msg) {
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
			msg->vars["target"] = printChat(link.target);
			msg->replyMsg("strLinked");
			save();
		}
	}
	else {
		msg->replyMsg("strLinkNotFound");
	}
}
string DiceChatLink::show(const chatInfo& here) {
	string info{ "[无]" };
	static dict<> prep{
		{"to","->"},
		{"from","<-"},
		{"with","<->"},
	};
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
	static dict<> prep{
		{"to","->"},
		{"from","<-"},
		{"with","<->"},
	};
	ShowList li;
	for (auto& [here,link]:LinkList) {
		li << printChat(here) + prep[link.typeLink] + printChat(link.target)
			+ (link.isLinking ? "√" : "×");
	}
	return li.show("\n");
}
void DiceChatLink::show(FromMsg* msg) {
	auto here{ msg->fromChat.locate() };
	if (LinkFromChat.count(here) || LinkList.count(here)) {
		msg->vars["link_info"] = show(here);
		msg->replyMsg("strLinkState");
	}
	else {
		msg->replyMsg("strLinkNotFound");
	}
}
void DiceChatLink::close(FromMsg* msg) {
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

void DiceSession::deck_set(FromMsg* msg) {
	const string key{ (msg->vars["deck_name"] = msg->readAttrName()).to_str() };
	size_t pos = msg->strMsg.find('=', msg->intMsgCnt);
	AttrVar& strCiteDeck{ msg->vars["deck_cited"] = pos == string::npos 
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
void DiceSession::deck_new(FromMsg* msg) {
	AttrVar& key{ msg->vars["deck_name"] };
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
		if (!decks[key].sizRes)return getMsg("strDeckRestEmpty", { {"deck_name",key} });
		return decks[key].draw();
	}
	else if (CardDeck::mPublicDeck.count(key)) {
		vector<string>& deck = CardDeck::mPublicDeck[key];
		return CardDeck::draw(deck[RandomGenerator::Randint(0, deck.size() - 1)]);
	}
	return "{key}";
}
void DiceSession::_draw(FromMsg* msg) {
	shared_ptr<DiceJobDetail> job{ msg->shared_from_this() };
	if ((*job)["deck_name"].str_empty())(*job)["deck_name"] = msg->readAttrName();
	DeckInfo& deck = decks[(*job)["deck_name"].to_str()];
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
		(*job)["res"] = Res.dot("|").show();
		(*job)["cnt"] = to_string(Res.size());
		if (msg->vars.is("hidden")) {
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
void DiceSession::deck_show(FromMsg* msg) {
	if (decks.empty()) {
		msg->replyMsg("strDeckListEmpty");
		return;
	}
	const string strDeckName{ (msg->vars["deck_name"] = msg->readAttrName()).to_str() };
	//默认列出所有牌堆
	if (strDeckName.empty()) {
		ResList res;
		for (auto& [key, val] : decks) {
			res << key + "[" + to_string(val.sizRes) + "/" + to_string(val.idxs.size()) + "]";
		}
		msg->vars["res"] = res.show();
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
			msg->vars["deck_rest"] = residxs.dot(" | ").show();
			msg->replyMsg("strDeckRestShow");
		}
		else {
			msg->replyMsg("strDeckNotFound");
		}
	}
}
void DiceSession::deck_reset(FromMsg* msg) {
	AttrVar& key{ msg->vars["deck_name"] = msg->readAttrName() };
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
void DiceSession::deck_del(FromMsg* msg) {
	AttrVar& key{ msg->vars["deck_name"] = msg->readAttrName() };
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
void DiceSession::deck_clr(FromMsg* msg) {
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
	nlohmann::json jData;
	if (!conf.empty()) {
		json& jConf{ jData["conf"] };
		for (auto& [key, val] : conf) {
			jConf[GBKtoUTF8(key)] = val.to_json();
		}
	}
	if (!sOB.empty())jData["observer"] = sOB;
	if (!mTable.empty())
		for (auto& [key, table] : mTable)
		{
			string strTable = GBKtoUTF8(key);
			for (auto& [item, val] : table)
			{
				jData["tables"][strTable][GBKtoUTF8(item)] = val;
			}
		}
	if (logger.tStart || !logger.fileLog.empty()) {
		json jLog;
		jLog["start"] = logger.tStart;
		jLog["lastMsg"] = logger.tLastMsg;
		jLog["file"] = GBKtoUTF8(logger.fileLog);
		jLog["logging"] = logger.isLogging;
		jData["log"] = jLog;
	}
	if (!decks.empty()) {
		json jDecks;
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
	auto& jChat{ jData["chats"] = json::array() };
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
		SessionByName[name] = ptr;
		SessionByChat[here] = ptr;
		return ptr;
	}
}
int DiceSessionManager::load() 
{
    string strLog;
    std::unique_lock<std::shared_mutex> lock(sessionMutex);
    vector<std::filesystem::path> sFile;
    int cnt = listDir(DiceDir / "user" / "session", sFile);
    if (cnt > 0)for (auto& filename : sFile) {
		nlohmann::json j = freadJson(filename);
		if (j.is_null()) {
			remove(filename);
			cnt--;
			continue;
		}
		auto pSession(std::make_shared<Session>(filename.stem().string()));
		bool isUpgrated{ false };
		pSession->create(j["create_time"]).update(j["update_time"]);
		if (j.count("room")) {
			if (j["room"].is_number()) {
				long long id{ j["room"].get<long long>() };
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
			json& jConf{ j["conf"] };
			for (auto it = jConf.cbegin(); it != jConf.cend(); ++it) {
				pSession->conf[UTF8toGBK(it.key())] = it.value();
			}
		}
		if (j.count("log")) {
			json& jLog = j["log"];
			jLog["start"].get_to(pSession->logger.tStart);
			jLog["lastMsg"].get_to(pSession->logger.tLastMsg);
			jLog["file"].get_to(pSession->logger.fileLog);
			pSession->logger.fileLog = UTF8toGBK(pSession->logger.fileLog);
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
			json& jLink = j["link"];
			const auto& ct{ *pSession->windows.begin() };
			long long gid{ jLink["target"].get<long long>() };
			LinkInfo& link{ linker.LinkList[ct] = {jLink["linking"].get<bool>(),
				jLink["type"].get<string>(),
				gid > 0 ? chatInfo{0,gid} : chatInfo{~gid} } };
			isUpgrated = true;
		}
		if (j.count("decks")) {
			json& jDecks = j["decks"];
			for (auto it = jDecks.cbegin(); it != jDecks.cend(); ++it) {
				if (it.value()["meta"].empty())continue;
				std::string key = UTF8toGBK(it.key());
				auto& deck{ pSession->decks[key] };
				deck.meta = UTF8toGBK(it.value()["meta"].get<vector<string>>());
				if (it.value().count("rest")) {
					it.value()["rest"].get_to(pSession->decks[key].idxs);
					pSession->decks[key].sizRes = pSession->decks[key].meta.size();
				}
				else {
					it.value()["idxs"].get_to(pSession->decks[key].idxs);
					it.value()["size"].get_to(pSession->decks[key].sizRes);
				}
			}
		}
		if (j.count("observer")) j["observer"].get_to(pSession->sOB);
		if (j.count("tables")){
			for (nlohmann::json::iterator itTable = j["tables"].begin(); itTable != j["tables"].end(); ++itTable)
			{
				string strTable = UTF8toGBK(itTable.key());
				for (nlohmann::json::iterator itItem = itTable.value().begin(); itItem != itTable.value().end(); ++itItem)
				{
					pSession->mTable[strTable].emplace(UTF8toGBK(itItem.key()), itItem.value());
				}
			}
		}
		SessionByName[UTF8toGBK(filename.u8string())] = pSession;
		for (const auto& chat : pSession->windows) {
			SessionByChat[chat] = pSession;
		}
		if (isUpgrated)pSession->save();
	}
	linker.load();
    return cnt;
}