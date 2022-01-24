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
namespace fs = std::filesystem;
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
				notes << (user_id_negative ? "- 以下用户不触发: " : "- 仅以下用户触发: ") + listID(user_id);
			}
		}
		else if (key == "grp_id") {
			size_t pos{ item.find_first_not_of(" ",colon + 1) };
			if (pos == string::npos)continue;
			if (item[pos] == '!')grp_id_negative = true;
			splitID(item.substr(pos), grp_id);
			if (!grp_id.empty()) {
				limits << (grp_id_negative ? "grp_id:!" : "grp_id:") + listID(grp_id);
				notes << (grp_id_negative ? "- 以下群聊不触发: " : "- 仅以下群聊触发: ") + listID(grp_id);
			}
		}
		else if (key == "cd") {
			if (colon == string::npos)continue;
			ShowList cds;
			ShowList cdnotes;
			for (auto& cd : split(item.substr(colon + 1), "&")) {
				string key;
				string type{ "chat" };
				if (cd.find('=') != string::npos) {
					pair<string, string>conf;
					readini(cd, conf);
					if (size_t pos{ conf.first.find('@') }; pos != string::npos) {
						key = conf.first.substr(0, pos);
						type = conf.first.substr(pos + 1);
					}
					else key = conf.first;
					cd = conf.second;
				}
				CDType cd_type{ (CDType)CDConfig::eType[type] };
				if (!isNumeric(cd))continue;
				time_t val{ (time_t)stoll(cd) };
				cd_timer.emplace_back(cd_type, key, val);
				cds << key + ((key.empty() && cd_type == CDType::Chat) ? ""
					: ("@" + CDConfig::eType[(size_t)cd_type] + "="))
					+ to_string(val);
				cdnotes << (cd_type == CDType::Chat ? "窗口"
					: cd_type == CDType::User ? "用户" : "全局") + key + "计" + to_string(val) + "秒";
			}
			if (cds.empty())continue;
			limits << "cd:" + cds.show("&");
			notes << "- 冷却计时: " + cdnotes.show();
		}
		else if (key == "today") {
			if (colon == string::npos)continue;
			ShowList sub;
			ShowList subnotes;
			for (auto& cd : split(item.substr(colon + 1), "&")) {
				string key;
				string type{ "chat" };
				if (cd.find('=') != string::npos) {
					pair<string, string>conf;
					readini(cd, conf);
					if (size_t pos{ conf.first.find('@') }; pos != string::npos) {
						key = conf.first.substr(0, pos);
						type = conf.first.substr(pos + 1);
					}
					else key = conf.first;
					cd = conf.second;
				}
				CDType cd_type{ (CDType)CDConfig::eType[type] };
				if (!isNumeric(cd))continue;
				time_t val{ (time_t)stoll(cd) };
				today_cnt.emplace_back(cd_type, key, val);
				sub << key + ((key.empty() && cd_type == CDType::Chat) ? ""
					: ("@" + CDConfig::eType[(size_t)cd_type] + "="))
					+ to_string(val);
				subnotes << (cd_type == CDType::Chat ? "窗口"
					: cd_type == CDType::User ? "用户" : "全局") + key + "计" + to_string(val) + "次";
			}
			if (sub.empty())continue;
			limits << "today:" + sub.show("&");
			notes << "- 当日计数: " + subnotes.show();
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
	if (!user_id.empty() && (user_id_negative ^ !user_id.count(msg->fromChat.uid)))return false;
	if (!grp_id.empty() && (grp_id_negative ^ !grp_id.count(msg->fromChat.gid)))return false;
	if (prob && RandomGenerator::Randint(1, 100) > prob)return false;
	if (!user_vary.empty()) {
		if (!UserList.count(msg->fromChat.uid))return false;
		User& user{ getUser(msg->fromChat.uid) };
		for (auto& [key, cmpr] : user_vary) {
			if (!user.confs.count(key) || !(user.confs[key].*cmpr.second)(cmpr.first))return false;
		}
	}
	if (!grp_vary.empty() && msg->fromChat.gid) {
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
	//冷却与上限最后处理
	if (!cd_timer.empty() || !today_cnt.empty()) {
		vector<CDQuest>timers;
		vector<CDQuest>counters;
		chatInfo chat{ msg->fromChat.gid ? chatInfo(0,msg->fromChat.gid,msg->fromChat.chid)
			: msg->fromChat };
		for (auto& conf : cd_timer) {
			string key{ conf.key };
			if (key.empty())key = msg->vars["keyword"].to_str();
			chatInfo chattype{ conf.type == CDType::Chat ? chat :
				conf.type == CDType::User ? chatInfo(msg->fromChat.uid) : chatInfo()
			};
			timers.emplace_back(chattype, key, conf.cd);
		}
		for (auto& conf : today_cnt) {
			string key{ conf.key };
			if (key.empty())key = msg->vars["keyword"].to_str();
			chatInfo chattype{ conf.type == CDType::Chat ? chat :
				conf.type == CDType::User ? chatInfo(msg->fromChat.uid) : chatInfo()
			};
			counters.emplace_back(chattype, key, conf.cd);
		}
		if (!sch.cnt_cd(timers, counters))return false;
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
	msg->vars["keyword"] = keyword;
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
		lua_msg_reply(msg, text);
		return true;
	}
	return false;
}
string DiceMsgReply::show()const {
	return "\n触发性质: " + strType[(int)type]
		+ (limit.print().empty() ? "" : ("\n限制条件:\n" + limit.note()))
		+ "\n匹配模式: " + strMode[(int)mode]
		+ "\n回复形式: " + strEcho[(int)echo]
		+ "\n回复内容: " + show_ans();
}
string DiceMsgReply::print()const {
	return "Type=" + sType[(int)type]
		+ (limit.print().empty() ? "" : ("\nLimit=" + limit.print()))
		+ "\n" + sMode[(int)mode] + "=" + keyword
		+ "\n" + sEcho[(int)echo] + "=" + show_ans();
}
string DiceMsgReply::show_ans()const {
	return echo == DiceMsgReply::Echo::Deck ?
		listDeck(deck) : text;
}

void DiceMsgReply::readJson(const json& j) {
	if (j.count("key"))keyword = fmt->format(UTF8toGBK(j["key"].get<string>()));
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
	j["key"] = GBKtoUTF8(keyword);
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

DiceModManager::DiceModManager() : helpdoc(HelpDoc){
}

string DiceModManager::format(string s, std::shared_ptr<AttrVars> context, const AttrIndexs& indexs, const dict_ci& dict) const{
	//直接重定向
	if (s[0] == '&')
	{
		const string key = s.substr(1);
		const auto it = dict.find(key);
		if (it != dict.end())
		{
			return format(it->second, context, indexs, dict);
		}
		if (context){
			if (auto uit = context->find(key); uit != context->end()) {
				return uit->second.to_str();
			}
		}
	}
	int l = 0, r = 0;
	int len = s.length();
	while ((l = s.find('{', r)) != string::npos && (r = s.find('}', l)) != string::npos){
		//左括号前加‘\’表示该括号内容不转义
		if (s[l - 1] == 0x5c){
			s.replace(l - 1, 1, "");
			continue;
		}
		string key = s.substr(l + 1, r - l - 1), val;
		if (context && context->find(key) != context->end()) {
			auto uit = context->find(key);
			if (key == "res")val = format(uit->second.to_str(), context, indexs, dict);
			else val = uit->second.to_str();
		}
		else if (auto idx = indexs.find(key); context && idx != indexs.end()) {
			val = idx->second(*context).to_str();
		}
		else if (key.find("help:") == 0) {
			key = key.substr(5);
			val = fmt->format(fmt->get_help(key), {}, {}, dict);
		}
		else if (key.find("sample:") == 0) {
			key = key.substr(7);
			vector<string> samples{ split(key,"|") };
			if (samples.empty())val = "";
			else
				val = fmt->format(samples[RandomGenerator::Randint(0, samples.size() - 1)], {}, {}, dict);
		}
		else if (auto cit = dict.find(key); cit != dict.end()) {
			val = format(cit->second, context, indexs, dict);
		}
		//局部屏蔽全局
		else if (auto it = GlobalChar.find(key);it != GlobalChar.end()) {
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
std::shared_mutex GlobalMsgMutex;
string DiceModManager::msg_get(const string& key)const {
	std::shared_lock lock(GlobalMsgMutex);
	const auto it = GlobalMsg.find(key);
	if (it != GlobalMsg.end()) return it->second;
	else return "";
}
void DiceModManager::msg_reset(const string& key)const {
	std::shared_lock lock(GlobalMsgMutex);
	const auto it = GlobalMsg.find(key);
	if (it != GlobalMsg.end()) {
		EditedMsg.erase(key);
		if (PlainMsg.count(key))it->second = PlainMsg.find(key)->second;
		else GlobalMsg.erase(it);
		saveJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg);
	}
}
void DiceModManager::msg_edit(const string& key, const string& val)const {
	std::shared_lock lock(GlobalMsgMutex);
	EditedMsg[key] = val;
	GlobalMsg[key] = val;
	saveJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg);
}

string DiceModManager::get_help(const string& key) const
{
	if (const auto it = helpdoc.find(key); it != helpdoc.end())
	{
		return format(it->second, {}, {}, helpdoc);
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
		job->reply(format(it->second, {}, {}, helpdoc));
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
	if (j.empty()) std::filesystem::remove(DiceDir / "conf" / "CustomMsgReply.json");
	else fwriteJson(DiceDir / "conf" / "CustomMsgReply.json", j);
}
void DiceModManager::reply_get(const shared_ptr<DiceJobDetail>& msg) {
	string key{ (*msg)["key"].to_str() };
	if (msgreply.count(key)) {
		(*msg)["show"] = msgreply[key].print();
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

string DiceModManager::script_path(const string& name)const {
	if (auto it{ scripts.find(name) }; it != scripts.end()) {
		return it->second;
	}
	return {};
}

int DiceModManager::load(ResList& resLog)
{
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
					sModErr.push_back(UTF8toGBK(pathFile.filename().u8string())
						+ "(build低于" + to_string(j["dice_build"].get<int>()) + ")");
					continue;
				}
			}
			if (j.count("helpdoc")) {
				cntItem += readJMap(j["helpdoc"], helpdoc);
			}
			if (j.count("global_char")) {
				cntItem += readJMap(j["global_char"], GlobalChar);
			}
			if (fs::path dirMod{ pathFile.replace_extension() }; fs::exists(dirMod)) {
				DD::debugLog("遍历读取" + dirMod.string());
				if (fs::exists(dirMod / "script")) {
					vector<std::filesystem::path> fScripts;
					listDir(dirMod / "script", fScripts);
					for (auto& p : fScripts) {
						scripts[p.stem().string()] = getNativePathString(p);
					}
				}
			}
		}
		cntItem += scripts.size();
		resLog << "读取/mod/中的" + std::to_string(cntFile) + "个文件, 共" + std::to_string(cntItem) + "项";
		if (!sModErr.empty()) {
			resLog << "读取失败" + std::to_string(sModErr.size()) + "个:";
			for (auto& it : sModErr) {
				resLog << it;
			}
		}
	}
	//读取plugin
	vector<std::filesystem::path> sLuaFile;
	int cntLuaFile = listDir(DiceDir / "plugin", sLuaFile);
	msgorder.clear();
	if (cntLuaFile > 0) {
		int cntOrder{ 0 };
		vector<string> sLuaErr;
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
					msgorder[format(key)] = { fileLua,func };
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
		resLog << "读取/plugin/中的" + std::to_string(cntLuaFile) + "个脚本, 共" + std::to_string(cntOrder) + "个指令";
		if (!sLuaErr.empty()) {
			resLog << "读取失败" + std::to_string(sLuaErr.size()) + "个:";
			for (auto& it : sLuaErr) {
				resLog << it;
			}
		}
	}
	//custom
	if (std::filesystem::path fCustomHelp{ DiceDir / "conf" / "CustomHelp.json" }; std::filesystem::exists(fCustomHelp)) {
		if (loadJMap(fCustomHelp, CustomHelp) == -1) {
			resLog << UTF8toGBK(fCustomHelp.u8string()) + "解析失败！";
		}
		else {
			merge(helpdoc, CustomHelp);
		}
	}
	//读取reply
	json jFile = freadJson(DiceDir / "conf" / "CustomMsgReply.json");
	if (!jFile.empty()) {
		try {
			for (auto reply = jFile.cbegin(); reply != jFile.cend(); ++reply) {
				if (std::string key = UTF8toGBK(reply.key()); !key.empty()) {
					msgreply[key].readJson(reply.value());
					if (msgreply[key].keyword.empty())msgreply[key].keyword = key;
				}
			}
			resLog << "读取/conf/CustomMsgReply.json中的" + std::to_string(msgreply.size()) + "条自定义回复";
		}
		catch (const std::exception& e) {
			resLog << "解析/conf/CustomMsgReply.json出错:" << e.what();
		}
	}
	else {
		std::map<std::string, std::vector<std::string>, less_ci> mRegexReplyDeck;
		std::map<std::string, std::vector<std::string>, less_ci> mReplyDeck;
		if (loadJMap(DiceDir / "conf" / "CustomReply.json", mReplyDeck) > 0) {
			resLog << "读取CustomReply" + to_string(mReplyDeck.size()) + "条";
			for (auto& [key, deck] : mReplyDeck) {
				DiceMsgReply& reply{ msgreply[key] };
				reply.deck = deck;
			}
		}
		if (loadJMap(DiceDir / "conf" / "CustomRegexReply.json", mRegexReplyDeck) > 0) {
			resLog << "读取正则Reply" + to_string(mRegexReplyDeck.size()) + "条";
			for (auto& [key, deck] : mRegexReplyDeck) {
				DiceMsgReply& reply{ msgreply[key] };
				reply.mode = DiceMsgReply::Mode::Regex;
				reply.deck = deck;
			}
		}
		save_reply();
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
	for (const auto& [key, word] : helpdoc) {
		querier.insert(key);
	}
	for (const auto& [key, order] : msgorder) {
		gOrder.insert(key, key);
	}
	for (const auto& [key, reply] : msgreply) {
		if (reply.mode == DiceMsgReply::Mode::Search)gReplySearcher.add(convert_a2w(key.c_str()), key);
		else if(reply.mode == DiceMsgReply::Mode::Prefix)gReplyPrefix.add(key, key);
		else if (reply.mode == DiceMsgReply::Mode::Regex)reply_regex.insert(key);
	}
	gReplySearcher.make_fail();
	gReplyPrefix.make_fail();
	isIniting = false;
}
void DiceModManager::clear()
{
	helpdoc.clear();
	querier.clear();
	msgorder.clear();
	gOrder.clear();
	taskcall.clear();
	gReplySearcher.clear();
	gReplyPrefix.clear();
}
