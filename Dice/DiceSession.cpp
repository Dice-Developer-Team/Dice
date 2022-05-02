/**
 * 黑名单明细
 * 更数据库式的管理
 * Copyright (C) 2019-2020 String.Empty
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

std::shared_mutex sessionMutex;

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
		msg->reply(getMsg("strObEnterAlready"));
	}
	else
	{
		sOB.insert(msg->fromChat.uid);
		msg->reply(getMsg("strObEnter"));
		update();
	}
}

void DiceSession::ob_exit(FromMsg* msg)
{
	if (sOB.count(msg->fromChat.uid))
	{
		sOB.erase(msg->fromChat.uid);
		msg->reply(getMsg("strObExit"));
	}
	else
	{
		msg->reply(getMsg("strObExitAlready"));
	}
	update();
}

void DiceSession::ob_list(FromMsg* msg) const
{
	if (sOB.empty())msg->reply(getMsg("strObListEmpty"));
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
	if (sOB.empty())msg->reply(getMsg("strObListEmpty"));
	else
	{
		sOB.clear();
		msg->reply(getMsg("strObListClr"));
	}
	update();
}

void DiceSession::log_new(FromMsg* msg) {
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / logger.dirLog, ec);
	logger.tStart = time(nullptr);
	logger.isLogging = true;
	logger.fileLog = (type == "solo")
		? ("qq_" + to_string(msg->fromChat.uid) + "_" + to_string(logger.tStart) + ".txt")
		: ("group_" + to_string(msg->fromChat.gid) + "_" + to_string(logger.tStart) + ".txt");
	logger.pathLog = DiceDir / logger.dirLog / logger.fileLog;
	//先发消息后插入
	msg->reply(getMsg("strLogNew"));
	LogList.insert(room);
	update();
}
void DiceSession::log_on(FromMsg* msg) {
	if (!logger.tStart) {
		log_new(msg);
		return;
	}
	if (logger.isLogging) {
		msg->reply(getMsg("strLogOnAlready"));
		return;
	}
	logger.isLogging = true;
	msg->reply(getMsg("strLogOn"));
	LogList.insert(room);
	update();
}
void DiceSession::log_off(FromMsg* msg) {
	if (!logger.tStart) {
		msg->reply(getMsg("strLogNullErr"));
		return;
	}
	if (!logger.isLogging) {
		msg->reply(getMsg("strLogOffAlready"));
		return;
	}
	logger.isLogging = false;
	//先擦除后发消息
	LogList.erase(room);
	msg->reply(getMsg("strLogOff"));
	update();
}
void DiceSession::log_end(FromMsg* msg) {
	if (!logger.tStart) {
		msg->reply(getMsg("strLogNullErr"));
		return;
	}
	LogList.erase(room);
	logger.isLogging = false;
	logger.tStart = 0;
	if (std::filesystem::path pathFile(log_path()); !std::filesystem::exists(pathFile)) {
		msg->reply(getMsg("strLogEndEmpty"));
		return;
	}
	msg->vars["log_file"] = logger.fileLog;
	msg->vars["log_path"] = UTF8toGBK(log_path().u8string());
	msg->reply(getMsg("strLogEnd"));
	update();
	msg->cmd_key = "uplog"; 
	sch.push_job(msg->vars);
}
std::filesystem::path DiceSession::log_path()const {
	return logger.pathLog; 
}


void DiceSession::link_new(FromMsg* msg) {
	string strType = msg->readPara();
	string strID = msg->readDigit();
	if (strID.empty()) {
		msg->reply(getMsg("strLinkNotFound"));
		return;
	}
	long long id = stoll(strID);
	if (strType == "qq" || strType == "q") {
		id = ~id;
	}
	else if (!ChatList.count(id)) {
		msg->reply(getMsg("strLinkNotFound"));
		return;
	}
	linker.linkFwd = id;
	//重置已存在的链接
	if (linker.isLinking) {
		LinkList.erase(room);
		LinkList.erase(linker.linkFwd);
	}
	//占线不可用
	if (LinkList.count(room)) {
		msg->reply(getMsg("strLinkedAlready"));
	}
	else if (LinkList.count(linker.linkFwd)) {
		msg->reply(getMsg("strLinkBusy"));
	}
	else {
		linker.typeLink = msg->vars["option"].to_str();
		LinkList[room] = { linker.linkFwd ,linker.typeLink != "from" };
		LinkList[linker.linkFwd] = { room ,linker.typeLink != "to" };
		linker.isLinking = true;
		msg->reply(getMsg("strLinked"));
		update();
	}
}
void DiceSession::link_start(FromMsg* msg) {
	if (linker.linkFwd) {
		if (LinkList.count(room)) {
			msg->reply(getMsg("strLinkingAlready"));
		}
		else if (LinkList.count(linker.linkFwd)) {
			msg->reply(getMsg("strLinkBusy"));
		}
		else {
			LinkList[room] = { linker.linkFwd ,linker.typeLink != "from" };
			LinkList[linker.linkFwd] = { room ,linker.typeLink != "to" };
			linker.isLinking = true;
			msg->reply(getMsg("strLinked"));
			update();
		}
	}
	else {
		msg->reply(getMsg("strLinkNotFound"));
	}
}
void DiceSession::link_close(FromMsg* msg) {
	if (auto link = LinkList.find(room); link != LinkList.end()) {
		linker.isLinking = false;
		if (gm->mSession.count(link->second.first)) {
			gm->session(link->second.first).linker.isLinking = false;
			gm->session(link->second.first).update();
		}
		LinkList.erase(link->second.first);
		LinkList.erase(link->first);
		msg->reply(getMsg("strLinkClose"));
		update();
	}
	else {
		msg->reply(getMsg("strLinkCloseAlready"));
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
		msg->reply(getMsg("strDeckNameEmpty"));
	}
	else if (decks.size() > 9 && !decks.count(key)) {
		msg->reply(getMsg("strDeckListFull"));
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
				msg->reply(getMsg("strRangeEmpty"));
				return;
			}
			else if (strL.length() > 16 || strR.length() > 16) {
				msg->reply(getMsg("strOutRange"));
				return;
			}
			else {
				long long llBegin{ stoll(strL) };
				long long llEnd{ strR.empty() ? 0 : stoll(strR) };
				long long llStep{ strStep.empty() ? 1 : stoll(strStep) };
				if (llBegin == llEnd) {
					msg->reply(getMsg("strRangeEmpty"));
					return;
				}
				else if (llBegin > llEnd) {
					long long temp = llBegin;
					llBegin = llEnd;
					llEnd = temp;
				}
				if ((llEnd - llBegin) > 1000 * llStep) {
					msg->reply(getMsg("strOutRange"));
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
			msg->reply(getMsg("strDeckCiteNotFound"));
			return;
		}
		else {
			decks[key] = CardDeck::mPublicDeck[strCiteDeck.to_str()];
		}
		if (key == strCiteDeck.to_str())msg->reply(getMsg("strDeckSet"));
		else msg->reply(getMsg("strDeckSetRename"));
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
		msg->reply(getMsg("strDeckListFull"));
	}
	else {
		vector<string> DeckSet = {};
		msg->readItems(DeckSet);
		if (DeckSet.empty()) {
			msg->reply(getMsg("strDeckNewEmpty"));
			return;
		}
		else if (DeckSet.size() > 256) {
			msg->reply(getMsg("strDeckOversize"));
			return;
		}
		DeckInfo deck{ DeckSet };
		if (deck.idxs.size() > 1024) {
			msg->reply(getMsg("strDeckOversize"));
			return;
		}
		decks[key.to_str()] = std::move(deck);
		msg->reply(getMsg("strDeckNew"));
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
			msg->reply(getMsg("strNumCannotBeZero"));
			return;
		}
		break;
	case -1: break;
	case -2:
		msg->reply(getMsg("strParaIllegal"));
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
		if (msg->vars.has("hidden")) {
			msg->reply(getMsg("strDrawHidden"));
			msg->replyHidden(getMsg("strDrawCard"));
		}
		else
			msg->reply(getMsg("strDrawCard"));
		update();
	}
	if (!deck.sizRes) {
		msg->reply(getMsg("strDeckRestEmpty"));
	}
}
void DiceSession::deck_show(FromMsg* msg) {
	if (decks.empty()) {
		msg->reply(getMsg("strDeckListEmpty"));
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
		msg->reply(getMsg("strDeckListShow"));
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
			msg->reply(getMsg("strDeckRestShow"));
		}
		else {
			msg->reply(getMsg("strDeckNotFound"));
		}
	}
}
void DiceSession::deck_reset(FromMsg* msg) {
	AttrVar& key{ msg->vars["deck_name"] = msg->readAttrName() };
	if (key.str_empty())key = msg->readDigit();
	if (key.str_empty()) {
		msg->reply(getMsg("strDeckNameEmpty"));
	}
	else if (!decks.count(key.to_str())) {
		msg->reply(getMsg("strDeckNotFound"));
	}
	else {
		decks[key.to_str()].reset();
		msg->reply(getMsg("strDeckRestReset"));
		update();
	}
}
void DiceSession::deck_del(FromMsg* msg) {
	AttrVar& key{ msg->vars["deck_name"] = msg->readAttrName() };
	if (key.str_empty())key = msg->readDigit();
	if (key.str_empty()) {
		msg->reply(getMsg("strDeckNameEmpty"));
	}
	else if (!decks.count(key.to_str())) {
		msg->reply(getMsg("strDeckNotFound"));
	}
	else {
		decks.erase(key.to_str());
		msg->reply(getMsg("strDeckDelete"));
		update();
	}
}
void DiceSession::deck_clr(FromMsg* msg) {
	decks.clear();
	msg->reply(getMsg("strDeckListClr"));
	update();
}

std::mutex exSessionSave;

void DiceSession::save() const
{
	std::error_code ec;
	std::filesystem::create_directories(DiceDir / "user" / "session", ec);
	std::filesystem::path fpFile = (type == "solo")
		? (DiceDir / "user" / "session" / ("Q" + to_string(~room) + ".json") )
		: (DiceDir / "user" / "session" / (to_string(room) + ".json"));
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
		jLog["file"] = logger.fileLog;
		jLog["logging"] = logger.isLogging;
		jData["log"] = jLog;
	}
	if (linker.linkFwd) {
		json jLink;
		jLink["type"] = linker.typeLink;
		jLink["target"] = linker.linkFwd;
		jLink["linking"] = linker.isLinking;
		jData["link"] = jLink;
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
	jData["type"] = type;
	jData["room"] = room;
	jData["create_time"] = tCreate;
	jData["update_time"] = tUpdate;
	ofstream fout(fpFile);
	if (!fout) {
		console.log("开团信息保存失败:" + UTF8toGBK(fpFile.u8string()), 1);
		return;
	}
	fout << jData.dump(1);
}

bool DiceTableMaster::has_session(long long group) {
	return mSession.count(group);
}

Session& DiceTableMaster::session(long long group)
{
	if (!mSession.count(group))
	{
		std::unique_lock<std::shared_mutex> lock(sessionMutex);
		if (group < 0)mSession.emplace(group, std::make_shared<Session>(group, "solo"));
		else mSession.emplace(group, std::make_shared<Session>(group));
	}
	return *mSession[group];
}

void DiceTableMaster::session_end(long long room)
{
	std::unique_lock<std::shared_mutex> lock(sessionMutex);
	remove(DiceDir / "user" / "session" /
		(room < 0 ? "Q" + to_string(~room) : to_string(~room) + ".json"));
	mSession.erase(room);
}

const enumap<string> mSMTag{"type", "room", "gm", "log", "player", "observer", "tables"};

/*
void DiceTableMaster::save()
{
	mkDir(DiceDir + "/user/session");
	std::shared_lock<std::shared_mutex> lock(sessionMutex);
	for (auto [grp, pSession] : mSession)
	{
		pSession->save();
	}
}
*/

int DiceTableMaster::load() 
{
    string strLog;
    std::unique_lock<std::shared_mutex> lock(sessionMutex);
    vector<std::filesystem::path> sFile;
    int cnt = listDir(DiceDir / "user" / "session", sFile);
    if (cnt <= 0)return cnt;
    for (auto& filename : sFile)
	{
        nlohmann::json j = freadJson(filename);
        if (j.is_null())
		{
            cnt--;
            continue;
        }
		auto pSession(std::make_shared<Session>(j["room"]));
		j["type"].get_to(pSession->type);
		pSession->create(j["create_time"]).update(j["update_time"]);
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
			jLog["logging"].get_to(pSession->logger.isLogging);
			pSession->logger.update();
			pSession->logger.pathLog = DiceDir / pSession->logger.dirLog / pSession->logger.fileLog;
			if (pSession->logger.isLogging) {
				LogList.insert(pSession->room);
			}
		}
		if (j.count("link")) {
			json& jLink = j["link"];
			jLink["type"].get_to(pSession->linker.typeLink);
			jLink["target"].get_to(pSession->linker.linkFwd);
			jLink["linking"].get_to(pSession->linker.isLinking);
			if (pSession->linker.isLinking) {
				LinkList[pSession->room] = { pSession->linker.linkFwd,pSession->linker.typeLink != "from" };
				LinkList[pSession->linker.linkFwd] = { pSession->room,pSession->linker.typeLink != "to" };
			}
		}
		if (j.count("decks")) {
			json& jDecks = j["decks"];
			for (auto it = jDecks.cbegin(); it != jDecks.cend(); ++it) {
				std::string key = UTF8toGBK(it.key());
				pSession->decks[key].meta = UTF8toGBK(it.value()["meta"].get<vector<string>>());
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
        if (j["type"] == "simple")
		{
            if (j.count("observer")) pSession->sOB = j["observer"].get<set<long long>>();
            if (j.count("tables"))
			{
                for (nlohmann::json::iterator itTable = j["tables"].begin(); itTable != j["tables"].end(); ++itTable)
				{
                    string strTable = UTF8toGBK(itTable.key());
                    for (nlohmann::json::iterator itItem = itTable.value().begin(); itItem != itTable.value().end(); ++itItem)
					{
                        pSession->mTable[strTable].emplace(UTF8toGBK(itItem.key()), itItem.value());
                    }
                }
            }
		}
		mSession[pSession->room] = std::move(pSession);
    }
    return cnt;
}