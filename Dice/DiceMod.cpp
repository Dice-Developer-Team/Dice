/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2022 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <set>
#include "DiceMod.h"
#include "GlobalVar.h"
#include "ManagerSystem.h"
#include "Jsonio.h"
#include "DiceFile.hpp"
#include "DiceEvent.h"
#include "DiceLua.h"
#include "RandomGenerator.h"
#include "CardDeck.h"
#include "DDAPI.h"
#include <regex>
using std::set;
void parse_vary(string& raw, unordered_map<string, pair<double, AttrVar::CMPR>>& vary) {
	ShowList vars;
	for (auto& var : split(raw, "&")) {
		pair<string, string>conf;
		readini(var, conf);
		if (conf.first.empty())continue;
		string strCMPR;
		AttrVar::CMPR cmpr{ &AttrVar::equal };
		if ('+' == *--conf.second.end()) {
			strCMPR = "+";
			cmpr = &AttrVar::equal_or_more;
			conf.second.erase(conf.second.end() - 1);
		}
		else if ('-' == *--conf.second.end()) {
			strCMPR = "-";
			cmpr = &AttrVar::equal_or_less;
			conf.second.erase(conf.second.end() - 1);
		}
		//only number
		if (!isNumeric(conf.second))continue;
		double val{ stod(conf.second) };
		vary[conf.first] = { val,cmpr };
		vars << conf.first + "=" + toString(val, 4) + strCMPR;
	}
	raw = vars.show("&");
}

enumap<string> LimitTreat{ "ignore","only","off" };
DiceTriggerLimit& DiceTriggerLimit::parse(const string& raw) {
	if (content == raw)return *this;
	new(this)DiceTriggerLimit();
	ShowList limits;
	ShowList notes;
	size_t colon{ 0 };
	std::istringstream sin;
	for (auto& item : getLines(raw, ';')) {
		if (item.empty())continue;
		string key{ item.substr(0,colon = item.find(":")) };
		if (key == "prob") {
			if (colon == string::npos)continue;
			sin.str(item.substr(colon + 1));
			sin >> prob;
			sin.clear();
			if (prob <= 0 || prob >= 100) {
				prob = 0;
				continue;
			}
			limits << "prob:" + to_string(prob);
			notes << "- 以概率触发: " + to_string(prob) + "%";
		}
		else if (key == "user_id") {
			size_t pos{ item.find_first_not_of(" ",colon + 1) };
			if (pos == string::npos)continue;
			if (item[pos] == '!')user_id_negative = true;
			splitID(item.substr(pos), user_id);
			if (!user_id.empty()) {
				limits << (user_id_negative ? "user_id:!" : "user_id:") + listID(user_id);
				notes << (user_id_negative ? "- 以下用户不触发" : "- 仅以下用户触发: ") + listID(user_id);
			}
		}
		else if (key == "grp_id") {
			size_t pos{ item.find_first_not_of(" ",colon + 1) };
			if (pos == string::npos)continue;
			if (item[pos] == '!')grp_id_negative = true;
			splitID(item.substr(pos), grp_id);
			if (!grp_id.empty()) {
				limits << (grp_id_negative ? "grp_id:!" : "grp_id:") + listID(grp_id);
				notes << (grp_id_negative ? "- 以下群聊不触发" : "- 仅以下群聊触发: ") + listID(grp_id);
			}
		}
		else if (key == "user_var") {
			if (colon == string::npos)continue;
			string code{ item.substr(colon + 1) };
			parse_vary(code, user_vary);
			if (code.empty())continue;
			limits << "user_var:" + code;
			ShowList vars;
			for (auto& [key, cmpr] : user_vary) {
				vars << key + (cmpr.second == &AttrVar::equal_or_more ? "达到"
					: cmpr.second == &AttrVar::equal ? "等于" : "不超过") + toString(cmpr.first);
			}
			notes << "- 用户触发阈值: " + vars.show();
		}
		else if (key == "grp_var") {
			if (colon == string::npos)continue;
			string code{ item.substr(colon + 1) };
			parse_vary(code, grp_vary);
			if (code.empty())continue; 
			limits << "grp_var:" + code;
			ShowList vars;
			for (auto& [key, cmpr] : grp_vary) {
				vars << key + (cmpr.second == &AttrVar::equal_or_more ? "达到"
					: cmpr.second == &AttrVar::equal ? "等于" : "不超过") + toString(cmpr.first);
			}
			notes << "- 群聊触发阈值: " + vars.show();
		}
		else if (key == "self_var") {
			if (colon == string::npos)continue;
			string code{ item.substr(colon + 1) };
			parse_vary(code, self_vary);
			if (code.empty())continue; 
			limits << "self_var:" + code;
			ShowList vars;
			for (auto& [key, cmpr] : self_vary) {
				vars << key + (cmpr.second == &AttrVar::equal_or_more ? "达到"
					: cmpr.second == &AttrVar::equal ? "等于" : "不超过") + toString(cmpr.first);
			}
			notes << "- 自身触发阈值: " + vars.show();
		}
		else if (key == "dicemaid") {
			if (colon == string::npos)continue;
			string val{ item.substr(colon + 1) };
			if (Treat t{ LimitTreat[val] }; t != Treat::Ignore) {
				to_dice = t;
				limits << "dicemaid:" + LimitTreat[(size_t)t];
				notes << (to_dice == Treat::Only ? "- 识别Dice骰娘: 才触发" : "- 识别Dice骰娘: 不触发");
			}
		}
	}
	content = limits.show(";");
	comment = notes.show("\n");
	return *this;
}
bool DiceTriggerLimit::check(FromMsg* msg)const {
	if (!user_id.empty() && !user_id.count(msg->fromChat.uid))return false;
	if (!grp_id.empty() && (!grp_id.count(msg->fromChat.gid) ^ grp_id_negative))return false;
	if (prob && RandomGenerator::Randint(1, 100) > prob)return false;
	if (!user_vary.empty()) {
		if (!UserList.count(msg->fromChat.uid))return false;
		User& user{ getUser(msg->fromChat.uid) };
		for (auto& [key, cmpr] : user_vary) {
			if (!user.confs.count(key) || !(user.confs[key].*cmpr.second)(cmpr.first))return false;
		}
	}
	if (!grp_vary.empty()) {
		Chat& grp{ chat(msg->fromChat.gid) };
		for (auto& [key, cmpr] : grp_vary) {
			if (!grp.confs.count(key) || !(grp.confs[key].*cmpr.second)(cmpr.first))return false;
		}
	}
	if (!self_vary.empty()) {
		User& user{ getUser(console.DiceMaid) };
		for (auto& [key, cmpr] : self_vary) {
			if (!user.confs.count(key) || !(user.confs[key].*cmpr.second)(cmpr.first))return false;
		}
	}
	if (to_dice != Treat::Ignore && console.DiceMaid != msg->fromChat.uid) {
		if (DD::isDiceMaid(msg->fromChat.uid) ^ (to_dice == Treat::Only))return false;
	}
	return true;
}

enumap<string> DiceMsgReply::sType{ "Reply","Order" };
enumap<string> DiceMsgReply::sMode{ "Match", "Prefix", "Search", "Regex" };
enumap<string> DiceMsgReply::sEcho{ "Text", "Deck", "Lua" };
enumap<string> strType{ "回复","指令" };
enumap<string> strMode{ "完全", "前缀", "模糊", "正则" };
enumap<string> strEcho{ "纯文本", "牌堆（多选一）", "Lua" };
bool DiceMsgReply::exec(FromMsg* msg) {
	int chon{ msg->pGrp ? msg->pGrp->getChConf(msg->fromChat.chid,"order",0) : 0 };
	//limit
	if (!limit.check(msg))return false;
	if (type == Type::Reply) {
		if (!msg->isCalled && (chon < 0 ||
			(!chon && (msg->pGrp->isset("禁用回复") || msg->pGrp->isset("认真模式")))))
			return false;
	}
	else {	//type == Type::Order
		if (!msg->isCalled && (chon < 0 ||
			(!chon && msg->pGrp->isset("停用指令"))))
			return false;
	}
	if (msg->WordCensor()) {
		return true;
	}

	if (echo == Echo::Text) {
		msg->reply(text);
		return true;
	}
	else if (echo == Echo::Deck) {
		msg->reply(CardDeck::drawCard(deck, true));
		return true;
	}
	else if (echo == Echo::Lua) {
		lua_msg_reply(msg, text.c_str());
		return true;
	}
	return false;
}
string DiceMsgReply::show()const {
	return "\n触发性质: " + strType[(int)type]
		+ (limit.print().empty() ? "" : ("\n限制条件:\n" + limit.note()))
		+ "\n匹配模式: " + strMode[(int)mode]
		+ "\n回复形式: " + strEcho[(int)echo]
		+ "\n回复内容:\n" + show_ans();
}
string DiceMsgReply::print(const string& key)const {
	return "Type=" + sType[(int)type]
		+ (limit.print().empty() ? "" : ("\nLimit=" + limit.print()))
		+ "\n" + sMode[(int)mode] + "=" + key
		+ "\n" + sEcho[(int)echo] + "=" + show_ans();
}
string DiceMsgReply::show_ans()const {
	return echo == DiceMsgReply::Echo::Deck ?
		listDeck(deck) : text;
}

void DiceMsgReply::readJson(const json& j) {
	if (j.count("type"))type = (Type)sType[j["type"].get<string>()];
	if (j.count("mode"))mode = (Mode)sMode[j["mode"].get<string>()];
	if (j.count("limit"))limit.parse(UTF8toGBK(j["limit"].get<string>()));
	if (j.count("echo"))echo = (Echo)sEcho[j["echo"].get<string>()];
	if (j.count("answer")) {
		if (echo == Echo::Deck)deck = UTF8toGBK(j["answer"].get<vector<string>>());
		else text = UTF8toGBK(j["answer"].get<string>());
	}
}
json DiceMsgReply::writeJson()const {
	json j;
	j["type"] = sType[(int)type];
	j["mode"] = sMode[(int)mode];
	j["echo"] = sEcho[(int)echo];
	if(!limit.empty())j["limit"] = GBKtoUTF8(limit.print());
	if (echo == Echo::Deck)j["answer"] = GBKtoUTF8(deck);
	else j["answer"] = GBKtoUTF8(text);
	return j;
}

bool DiceMsgOrder::exec(FromMsg* msg) {
	if (type == OrderType::Lua) {
		//std::thread th(lua_msg_order, msg, fileLua.c_str(), funcLua.c_str());
		//th.detach();
		lua_msg_order(msg, fileLua.c_str(), funcLua.c_str());
		return true;
	}
	return false;
}
bool DiceMsgOrder::exec() {
	if (type == OrderType::Lua) {
		std::thread th(lua_call_task, fileLua.c_str(), funcLua.c_str());
		th.detach();
		//lua_call_task(fileLua.c_str(), funcLua.c_str());
		return true;
	}
	return false;
}

DiceModManager::DiceModManager() : helpdoc(HelpDoc)
{
}

string DiceModManager::format(string s, const map<string, string, less_ci>& dict, const char* mod_name) const
{
	//直接重定向
	if (s[0] == '&')
	{
		const string key = s.substr(1);
		const auto it = dict.find(key);
		if (it != dict.end())
		{
			return format(it->second, dict, mod_name);
		}
		//调用本mod词条
	}
	int l = 0, r = 0;
	int len = s.length();
	while ((l = s.find('{', r)) != string::npos && (r = s.find('}', l)) != string::npos)
	{
		if (s[l - 1] == 0x5c)
		{
			s.replace(l - 1, 1, "");
			continue;
		}
		string key = s.substr(l + 1, r - l - 1), val;
		if (key.find("help:") == 0) {
			key = key.substr(5);
			val = fmt->format(fmt->get_help(key), dict);
		}
		else if (key.find("sample:") == 0) {
			key = key.substr(7);
			vector<string> samples{ split(key,"|") };
			if (samples.empty())val = "";
			else
				val = fmt->format(samples[RandomGenerator::Randint(0, samples.size() - 1)], dict);
		}
		else if (auto it = dict.find(key); it != dict.end()){
			val = format(it->second, dict, mod_name);
		}
		//局部屏蔽全局
		else if ((it = GlobalChar.find(key)) != GlobalChar.end()) {
			val = it->second;
		}
		else if (auto func = strFuncs.find(key); func != strFuncs.end())
		{
			val = func->second();
		}
		else continue;
		s.replace(l, r - l + 1, val);
		r = l + val.length();
		//调用本mod词条
	}
	return s;
}

string DiceModManager::get_help(const string& key) const
{
	if (const auto it = helpdoc.find(key); it != helpdoc.end())
	{
		return format(it->second, helpdoc);
	}
	return "{strHelpNotFound}";
}

struct help_sorter {
	bool operator()(const string& _Left, const string& _Right) const {
		if (fmt->cntHelp.count(_Right) && !fmt->cntHelp.count(_Left))
			return true;
		else if (fmt->cntHelp.count(_Left) && !fmt->cntHelp.count(_Right))
			return false;
		else if (fmt->cntHelp.count(_Left) && fmt->cntHelp.count(_Right) && fmt->cntHelp[_Left] != fmt->cntHelp[_Right])
			return fmt->cntHelp[_Left] < fmt->cntHelp[_Right];
		else if (_Left.length() != _Right.length()) {
			return _Left.length() > _Right.length();
		}
		else return _Left > _Right;
	}
};

void DiceModManager::_help(const shared_ptr<DiceJobDetail>& job) {
	if ((*job)["help_word"].str_empty()) {
		job->reply(getMsg("strBotHeader") + Dice_Short_Ver + "\n" + getMsg("strHlpMsg"));
		return;
	}
	else if (const auto it = helpdoc.find((*job)["help_word"].to_str()); it != helpdoc.end()) {
		job->reply(format(it->second, helpdoc));
	}
	else if (unordered_set<string> keys = querier.search((*job)["help_word"].to_str());!keys.empty()) {
		if (keys.size() == 1) {
			(*job)["redirect_key"] = *keys.begin();
			(*job)["redirect_res"] = get_help(*keys.begin());
			job->reply("{strHelpRedirect}");
		}
		else {
			std::priority_queue<string, vector<string>, help_sorter> qKey;
			for (auto key : keys) {
				qKey.emplace(".help " + key);
			}
			ResList res;
			while (!qKey.empty()) {
				res << qKey.top();
				qKey.pop();
				if (res.size() > 20)break;
			}
			(*job)["res"] = res.dot("/").show(1);
			job->reply(getMsg("strHelpSuggestion"));
		}
	}
	else job->reply(getMsg("strHelpNotFound"));
	cntHelp[(*job)["help_word"].to_str()] += 1;
	saveJMap(DiceDir / "user" / "HelpStatic.json",cntHelp);
}

void DiceModManager::set_help(const string& key, const string& val)
{
	if (!helpdoc.count(key))querier.insert(key);
	helpdoc[key] = val;
}

void DiceModManager::rm_help(const string& key)
{
	helpdoc.erase(key);
}

bool DiceModManager::listen_reply(FromMsg* msg) {
	string& strMsg{ msg->strMsg };
	if (msgreply.count(strMsg) && msgreply[strMsg].exec(msg)) {
		return true;
	}
	if (stack<string> sPrefix; gReplyPrefix.match_head(msg->strMsg, sPrefix)) {
		while (!sPrefix.empty()){
			string key{ sPrefix.top() };
			sPrefix.pop();
			if (!msgreply.count(key))continue;
			DiceMsgReply& reply{ msgreply[key] };
			if (reply.mode == DiceMsgReply::Mode::Prefix && reply.exec(msg))return true;
		}
	}
	//模糊匹配禁止自我触发
	if (vector<string>res; (*msg)["uid"] != console.DiceMaid && gReplySearcher.search(convert_a2w(strMsg.c_str()), res)) {
		for (const auto& key : res) {
			if (!msgreply.count(key) || msg->strMsg.find(key) == string::npos)continue;
			DiceMsgReply& reply{ msgreply[key] };
			if (reply.mode == DiceMsgReply::Mode::Search && reply.exec(msg))return true;
		}
	}
	//regex
	try {
		for (auto& key : reply_regex)
		{
			if (!msgreply.count(key) || msgreply[key].mode != DiceMsgReply::Mode::Regex)continue;
			// libstdc++ 使用了递归式 dfs 匹配正则表达式
			// 递归层级很多，非常容易爆栈
			// 然而，每个 Java Thread 在32位 Linux 下默认大小为320K，600字符的匹配即会爆栈
			// 64位下还好，默认是1M，1800字符会爆栈
			// 这里强制限制输入为400字符，以避免此问题
			// @seealso https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86164

			// 未来优化：预先构建regex并使用std::regex::optimize
			std::wregex exp(convert_a2realw(key.c_str()), std::regex::ECMAScript | std::regex::icase);
			std::wstring LstrMsg = convert_a2realw(strMsg.c_str());
			if (strMsg.length() <= 400 && std::regex_match(LstrMsg, msg->msgMatch, exp))
			{
				if(msgreply[key].exec(msg))return true;
			}
		}
	}
	catch (const std::regex_error& e)
	{
		msg->reply(e.what());
	}
	return 0;
}
string DiceModManager::list_reply()const {
	ResList listTotal, listMatch, listSearch, listPrefix, listRegex;
	for (const auto& [key, reply] : msgreply) {
		if (reply.mode == DiceMsgReply::Mode::Search)listSearch << key;
		else if (reply.mode == DiceMsgReply::Mode::Prefix)listPrefix << key;
		else if (reply.mode == DiceMsgReply::Mode::Regex)listRegex << key;
		else listMatch << key;
	}
	if (!listMatch.empty())listTotal << "[完全匹配] " + listMatch.dot(" | ").show();
	if (!listPrefix.empty())listTotal << "[前缀匹配] " + listPrefix.dot(" | ").show();
	if (!listSearch.empty())listTotal << "[模糊匹配] " + listSearch.dot(" | ").show();
	if (!listRegex.empty())listTotal << "[正则匹配] " + listRegex.dot(" | ").show();
	return listTotal.show();
}
void DiceModManager::set_reply(const string& key, DiceMsgReply& reply) {
	msgreply[key] = reply;
	if (reply.mode == DiceMsgReply::Mode::Regex)
		reply_regex.insert(key);
	else reply_regex.erase(key);
	if (reply.mode == DiceMsgReply::Mode::Search)gReplySearcher.insert(convert_a2w(key.c_str()), key);
	if (reply.mode == DiceMsgReply::Mode::Prefix)gReplyPrefix.insert(key, key);
	save_reply();
}
bool DiceModManager::del_reply(const string& key) {
	if (!msgreply.count(key))return false;
	msgreply.erase(key);
	save_reply();
	return true;
}
void DiceModManager::save_reply() {
	json j;
	for (const auto& [word, reply] : msgreply) {
		j[GBKtoUTF8(word)] = reply.writeJson();
	}
	fwriteJson(DiceDir / "conf" / "CustomMsgReply.json", j);
}
void DiceModManager::reply_get(const shared_ptr<DiceJobDetail>& msg) {
	string key{ (*msg)["key"].to_str() };
	if (msgreply.count(key)) {
		(*msg)["show"] = msgreply[key].print(key);
		msg->reply(getMsg("strReplyShow"));
	}
	else {
		msg->reply(getMsg("strReplyKeyNotFound"));
	}
}
void DiceModManager::reply_show(const shared_ptr<DiceJobDetail>& msg) {
	string key{ (*msg)["key"].to_str()};
	if (msgreply.count(key)) {
		(*msg)["show"] = msgreply[key].show();
		msg->reply(getMsg("strReplyShow"));
	}
	else {
		msg->reply(getMsg("strReplyKeyNotFound"));
	}
}

bool DiceModManager::listen_order(DiceJobDetail* msg) {
	string nameOrder;
	if(!gOrder.match_head(msg->strMsg, nameOrder))return false;
	if (((FromMsg*)msg)->WordCensor()) {
		return true;
	}
	msgorder[nameOrder].exec((FromMsg*)msg);
	return true;
}
string DiceModManager::list_order() {
	return msgorder.empty() ? "" : "扩展指令:" + listKey(msgorder);
}

bool DiceModManager::call_task(const string& task) {
	return taskcall[task].exec();
}

int DiceModManager::load(ResList* resLog) 
{
	//读取reply
	json jFile = freadJson(DiceDir / "conf" / "CustomMsgReply.json");
	if (!jFile.empty()) {
		try {
			for (auto reply = jFile.cbegin(); reply != jFile.cend(); ++reply) {
				if(std::string key = UTF8toGBK(reply.key());!key.empty())
					msgreply[key].readJson(reply.value());
			}
			*resLog << "读取/conf/CustomMsgReply.json中的" + std::to_string(msgreply.size()) + "条自定义回复";
		}
		catch (const std::exception& e) {
			*resLog << "解析/conf/CustomMsgReply.json出错:" << e.what();
		}
	}
	else {
		std::map<std::string, std::vector<std::string>, less_ci> mRegexReplyDeck;
		std::map<std::string, std::vector<std::string>, less_ci> mReplyDeck;
		if (loadJMap(DiceDir / "conf" / "CustomReply.json", mReplyDeck) > 0) {
			*resLog << "读取CustomReply" + to_string(mReplyDeck.size()) + "条";
			for (auto& [key, deck] : mReplyDeck) {
				DiceMsgReply& reply{ msgreply[key] };
				reply.deck = deck;
			}
		}
		if (loadJMap(DiceDir / "conf" / "CustomRegexReply.json", mRegexReplyDeck) > 0) {
			*resLog << "读取正则Reply" + to_string(mRegexReplyDeck.size()) + "条";
			for (auto& [key, deck] : mRegexReplyDeck) {
				DiceMsgReply& reply{ msgreply[key] };
				reply.mode = DiceMsgReply::Mode::Regex;
				reply.deck = deck;
			}
		}
		save_reply();
	}
	for (const auto& [key, reply] : msgreply) {
		if (reply.mode == DiceMsgReply::Mode::Search)gReplySearcher.add(convert_a2w(key.c_str()), key);
		else if (reply.mode == DiceMsgReply::Mode::Regex)reply_regex.insert(key);
	}
	gReplySearcher.make_fail();
	//读取mod
	vector<std::filesystem::path> sModFile;
	vector<string> sModErr;
	int cntFile = listDir(DiceDir / "mod" , sModFile);
	int cntItem{ 0 };
	if (cntFile > 0) {
		for (auto& pathFile : sModFile) {
			nlohmann::json j = freadJson(pathFile);
			if (j.is_null()) {
				sModErr.push_back(UTF8toGBK(pathFile.filename().u8string()));
				continue;
			}
			if (j.count("dice_build")) {
				if (j["dice_build"] > Dice_Build) {
					sModErr.push_back(UTF8toGBK(pathFile.filename().u8string()) + "(Dice版本过低)");
					continue;
				}
			}
			if (j.count("helpdoc")) {
				cntItem += readJMap(j["helpdoc"], helpdoc);
			}
			if (j.count("global_char")) {
				cntItem += readJMap(j["global_char"], GlobalChar);
			}
		}
		*resLog << "读取/mod/中的" + std::to_string(cntFile) + "个文件, 共" + std::to_string(cntItem) + "个条目";
		if (!sModErr.empty()) {
			*resLog << "读取失败" + std::to_string(sModErr.size()) + "个:";
			for (auto& it : sModErr) {
				*resLog << it;
			}
		}
	}
	//读取plugin
	vector<std::filesystem::path> sLuaFile;
	int cntLuaFile = listDir(DiceDir / "plugin", sLuaFile);
	int cntOrder{ 0 };
	if (cntLuaFile <= 0)return cntLuaFile;
	vector<string> sLuaErr; 
	msgorder.clear();
	for (auto& pathFile : sLuaFile) {
		string fileLua = getNativePathString(pathFile);
		if ((fileLua.rfind(".lua") != fileLua.length() - 4) && (fileLua.rfind(".LUA") != fileLua.length() - 4)) {
			sLuaErr.push_back(UTF8toGBK(pathFile.filename().u8string()));
			continue;
		}
		std::unordered_map<std::string, std::string> mOrder;
		int cnt = lua_readStringTable(fileLua.c_str(), "msg_order", mOrder);
		if (cnt > 0) {
			for (auto& [key, func] : mOrder) {
				msgorder[::format(key, GlobalMsg)] = { fileLua,func };
			}
			cntOrder += mOrder.size();
		}
		else if (cnt < 0) {
			sLuaErr.push_back(UTF8toGBK(pathFile.filename().u8string()));
		}
		std::unordered_map<std::string, std::string> mJob;
		cnt = lua_readStringTable(fileLua.c_str(), "task_call", mJob);
		if (cnt > 0) {
			for (auto& [key, func] : mJob) {
				taskcall[key] = { fileLua,func };
			}
			cntOrder += mJob.size();
		}
		else if (cnt < 0
				 && *sLuaErr.rbegin() != UTF8toGBK(pathFile.filename().u8string())) {
			sLuaErr.push_back(UTF8toGBK(pathFile.filename().u8string()));
		}
	}
	*resLog << "读取/plugin/中的" + std::to_string(cntLuaFile) + "个脚本, 共" + std::to_string(cntOrder) + "个指令";
	if (!sLuaErr.empty()) {
		*resLog << "读取失败" + std::to_string(sLuaErr.size()) + "个:";
		for (auto& it : sLuaErr) {
			*resLog << it;
		}
	}
	//init
	std::thread factory(&DiceModManager::init, this);
	factory.detach();
	if (cntHelp.empty()) {
		cntHelp.reserve(helpdoc.size());
		loadJMap(DiceDir / "user" / "HelpStatic.json", cntHelp);
	}
	return cntFile;
}
void DiceModManager::init() {
	isIniting = true;
	for (auto& [key, word] : helpdoc) {
		querier.insert(key);
	}
	for (const auto& [key, order] : msgorder) {
		gOrder.insert(key, key);
	}
	isIniting = false;
}
void DiceModManager::clear()
{
	helpdoc.clear();
	msgorder.clear();
	taskcall.clear();
}
