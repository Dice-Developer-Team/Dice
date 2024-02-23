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
 * Copyright (C) 2019-2024 String.Empty
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
#include <regex>
#include "DiceMod.h"
#include "GlobalVar.h"
#include "ManagerSystem.h"
#include "Jsonio.h"
#include "DiceFile.hpp"
#include "DiceEvent.h"
#include "DiceLua.h"
#include "DiceJS.h"
#include "DicePython.h"
#include "DiceMsgReply.h"
#include "DiceSelfData.h"
#include "RandomGenerator.h"
#include "DDAPI.h"
#include "DiceYaml.h"
#include "DiceFormatter.h"

std::shared_ptr<DiceModManager> fmt;

DiceSpeech::DiceSpeech(const YAML::Node& yaml) {
	if (yaml.IsScalar())speech = UTF8toGBK(yaml.Scalar());
	else if (yaml.IsSequence()) {
		speech = UTF8toGBK(yaml.as<vector<string>>());
	}
}
DiceSpeech::DiceSpeech(const fifo_json& j) {
	if (j.is_string())speech = UTF8toGBK(j.get<string>());
	else if (j.is_array()) {
		speech = UTF8toGBK(j.get<vector<string>>());
	}
}
string DiceSpeech::express()const {
	if (std::holds_alternative<string>(speech)) {
		return std::get<string>(speech);
	}
	else if (const auto p{ std::get_if<vector<string>>(&speech) }) {
		return p->at((size_t)RandomGenerator::Randint(0, p->size() - 1));
	}
	return {};
}

static dict_ci<DiceSpeech> transpeech{
	{ "br", "\n" },
	{ "sp"," " },
	{ "amp","&" },
	{ "FormFeed","\f" },
	{ "Vertical","|" },
	{ "LBrace","{" },
	{ "RBrace","}" },
	{ "LBracket","[" },
	{ "RBracket","]" },
	{ "Question","?" },
	{ "Equal","=" },
};

DiceModManager::DiceModManager() {
#ifndef __ANDROID__
	git_libgit2_init();
#endif
}
std::mutex ModMutex;
string DiceModManager::list_mod()const {
	std::lock_guard lock(ModMutex);
	ResList list;
	for (auto& mod : modOrder) {
		list << to_string(mod->index) + ". "
			+ (mod->title.empty() || mod->title == mod->name ? mod->name
				: (mod->title + "/" + mod->name))
			+ (mod->active
				? (mod->loaded ? " √" : " ?")
				: " ×");
	}
	return list.show();
}
void DiceModManager::mod_on(DiceEvent* msg) {
	string modName{ msg->get_str("mod") };
	if (modList[modName]->active) {
		msg->replyMsg("strModOnAlready");
	}
	else {
		modList[modName]->active = true;
		modList[modName]->loadDir();
		save();
		build();
		msg->note("{strModOn}", 1);
	}
}
void DiceModManager::mod_off(DiceEvent* msg) {
	string modName{ msg->get_str("mod") };
	if (!modList[modName]->active) {
		msg->replyMsg("strModOffAlready");
	}
	else {
		modList[modName]->active = false;
		save();
		build();
		msg->note("{strModOff}", 1);
	}
}
#ifndef __ANDROID__
bool DiceModManager::mod_clone(const string& name, const string& repo) {
	auto mod{ std::make_shared<DiceMod>(DiceMod{ name,modOrder.size(),repo}) };
	if (!mod->loaded)return false;
	modList[name] = mod;
	modOrder.push_back(mod);
	save();
	build();
	return true;
}
#endif
bool DiceModManager::mod_dlpkg(const string& name, const string& pkg, string& des) {
	if (!Network::GET(pkg, des)) {
		des = "\n下载失败(" + pkg + "):" + des;
		return false;
	}
	std::error_code ec1;
	Zip::extractZip(des, DiceDir / "mod");
	auto pathJson{ DiceDir / "mod" / (name + ".json") };
	if (!fs::exists(pathJson)) {
		if (fs::path desc{ DiceDir / "mod" / name / "descriptor.json" }; fs::exists(desc)) {
			fs::copy(desc, pathJson);
		}
		else {
			des = "\npkg don't exist " + name + ".json";
			return false;
		}
	}
	auto mod{ std::make_shared<DiceMod>(DiceMod{ name,modOrder.size(),true}) };
	modList[name] = mod;
	modOrder.push_back(mod);
	if (string err; !mod->file(pathJson).loadDesc(err)) {
		des = "\n" + err + "(" + pkg + ")";
		return false;
	}
	save();
	build();
	des = mod->ver.exp;
	return true;
}
void DiceModManager::mod_install(DiceEvent& msg) {
	std::lock_guard lock(ModMutex);
	std::string desc;
	string name{ msg.get_str("mod") };
	if (modList.count(name) && modList[name]->loaded) {
		msg.set("mod_desc", modList[name]->desc());
		msg.replyMsg("strModDescLocal");
		return;
	}
	for (auto& url : sourceList) {
		if (!Network::GET(url + name, desc)) {
			msg.set("err", msg.get_str("err") + "\n访问" + url + name + "失败:" + desc);
			continue;
		}
		try {
			if (desc.find("404") == 0)continue;
			fifo_json j = fifo_json::parse(desc);
			//todo: dice_build check
#ifndef __ANDROID__
			if (j.count("repo") && !j["repo"].empty()) {
				string repo{ j["repo"] };
				if (mod_clone(name, repo)) {
					msg.set("mod_ver", modList[name]->ver.exp);
					msg.replyMsg("strModInstalled");
					return;
				}
				else {
					msg.set("err", msg.get_str("err") + "\ngit clone失败:" + repo);
					continue;
				}
			}
#endif //ANDROID
			if (j.count("pkg")) {
				string pkg{ j["pkg"] };
				std::string des;
				if (mod_dlpkg(name, pkg, des)) {
					msg.set("mod_ver", des);
					msg.replyMsg("strModInstalled");
				}
				else msg.set("err", msg.get_str("err") + des);
			}
			msg.set("err", msg.get_str("err") + "\n未写出mod地址(repo/pkg):" + url + name);
		}
		catch (std::exception& e) {
			console.log("安装" + url + name + "失败:" + e.what(), 0b01);
			msg.set("err", msg.get_str("err") + "\n" + url + name + ":" + e.what());
		}
	}
	if (!msg.has("err"))msg.set("err", "\n未找到Mod源");
	msg.replyMsg("strModInstallErr");
}
void DiceModManager::mod_reinstall(DiceEvent& msg) {
	std::lock_guard lock(ModMutex);
	std::string desc;
	std::error_code ec1;
	string name{ msg.get_str("mod") };
	auto mod{ modList[name] };
	msg.set("ex_ver", mod->ver.exp);
	for (auto& url : sourceList) {
		if (!Network::GET(url + name, desc)) {
			msg.set("err", msg.get_str("err") + "\n访问" + url + name + "失败:" + desc);
			continue;
		}
		try {
			fifo_json j = fifo_json::parse(desc);
			//todo: dice_build check
#ifndef __ANDROID__
			if (j.count("repo") && !j["repo"].empty()) {
				string repo{ j["repo"] };
				auto idx{ mod->index };
				mod->free();
				fs::remove_all(DiceDir / "mod" / name, ec1);
				mod = std::make_shared<DiceMod>(DiceMod{ name,modOrder.size(),repo});
				modList[name] = mod;
				modOrder[idx] = mod;
				if (!mod->loaded) {
					msg.set("err", msg.get_str("err") + "\ngit clone失败:" + repo);
					continue;
				}
				save();
				build();
				msg.set("mod_ver", mod->ver.exp);
				msg.replyMsg("strModReinstalled");
				return;
			}
#endif //ANDROID
			if (j.count("pkg")) {
				string pkg{ j["pkg"] };
				std::string des;
				if (!Network::GET(pkg, des)) {
					msg.set("err", msg.get_str("err") + "\n下载失败(" + pkg + "):" + des);
					continue;
				}
				fs::remove_all(DiceDir / "mod" / name, ec1);
				Zip::extractZip(des, DiceDir / "mod");
				auto pathJson{ DiceDir / "mod" / (name + ".json") };
				if (!fs::exists(pathJson)) {
					msg.set("err", msg.get_str("err") + "\npkg解压无文件" + name + ".json");
					continue;
				}
				string err;
				if (mod->loadDesc(err)) {
					save();
					build();
					msg.set("mod_ver", mod->ver.exp);
					msg.replyMsg("strModReinstalled");
					return;
				}
				else {
					msg.set("err", msg.get_str("err") + "\n" + err + "(" + url + name + ")");
					continue;
				}
			}
			msg.set("err", msg.get_str("err") + "\n未写出mod地址(repo/pkg):" + url + name);
		} catch (std::exception& e) {
			console.log("安装" + url + name + "失败:" + e.what(), 0b01);
			msg.set("err", msg.get_str("err") + "\n" + url + name + ":" + e.what());
		}
	}
	if (!msg.has("err"))msg.set("err", "\n未找到Mod源");
	msg.replyMsg("strModInstallErr");
}
void DiceModManager::mod_update(DiceEvent& msg) {
	std::lock_guard lock(ModMutex);
	string name{ msg.get_str("mod") };
	auto mod{ get_mod(name) };
	std::string des;
	msg.set("ex_ver", mod->ver.exp);
#ifndef __ANDROID__
	if (mod->repo) {
		if (mod->repo->update(des)&&
			(fs::copy_file(mod->pathDir / "descriptor.json", mod->pathJson, fs::copy_options::overwrite_existing),
				mod->reload(des))) {
			fmt->build();
			msg.set("mod_ver", mod->ver.exp);
			msg.replyMsg("strModUpdated");
		}
		else {
			msg.set("err", "\n" + des);
			msg.replyMsg("strModUpdateErr");
		}
		return;
	}
#endif //ANDROID
	string desc;
	for (auto& url : sourceList) {
		if (!Network::GET(url + name, desc)) {
			console.log("访问" + url + name + "失败:" + desc, 0);
			msg.set("err", msg.get_str("err") + "\n访问" + url + name + "失败:" + desc);
			continue;
		}
		try {
			fifo_json j = fifo_json::parse(desc);
			if (!j.count("ver") || !j.count("pkg"))continue;
			Version ver{ j["ver"] };
			if (mod->ver < ver) {
				string pkg{ j["pkg"] };
				if (!Network::GET(pkg, des)) {
					msg.set("err", msg.get_str("err") + "\n下载失败(" + pkg + "):" + des);
					continue;
				}
				std::error_code ec1;
				fs::remove_all(DiceDir / "mod" / name, ec1);
				Zip::extractZip(des, DiceDir / "mod");
				auto pathJson{ DiceDir / "mod" / (name + ".json") };
				if (!fs::exists(pathJson)) {
					msg.set("err", msg.get_str("err") + "\npkg解压无文件" + name + ".json");
					continue;
				}
				string err;
				if (mod->loadDesc(err)) {
					build();
					msg.set("mod_ver", mod->ver.exp);
					msg.replyMsg("strModUpdated");
					return;
				}
				else {
					msg.set("err", msg.get_str("err") + "\n" + err + "(" + url + name + ")");
					continue;
				}
			}
			else {
				msg.set("err", "无更新于" + mod->ver.exp + "的版本!");
			}
		} catch (std::exception& e) {
			console.log("安装" + url + name + "失败:" + e.what(), 0b01);
			msg.set("err", msg.get_str("err") + "\n" + url + name + ":" + e.what());
		}
	}
	if (!msg.has("err"))msg.set("err", "\n未找到Mod源");
	msg.replyMsg("strModInstallErr");
}

void DiceModManager::uninstall(const string& modName) {
	std::lock_guard lock(ModMutex);
	auto mod{ modList[modName] };
	if (!mod)return;
	auto idx{ mod->index };
	modList.erase(modName);
	for (auto it{ modOrder.erase(modOrder.begin() + idx) };
		it != modOrder.end(); ++it) {
		--(*it)->index;
	}
	save();
	remove(mod->pathJson);
	remove_all(mod->pathDir);
	build();
}
bool DiceModManager::reorder(size_t oldIdx, size_t newIdx) {
	if (oldIdx == newIdx || (oldIdx > newIdx ? oldIdx: newIdx) >= modOrder.size())return false;
	auto mod{ modOrder[oldIdx] };
	mod->index = newIdx;
	int step{ oldIdx > newIdx ? -1 : 1 };
	while (oldIdx != newIdx) {
		(modOrder[oldIdx] = modOrder[oldIdx + step])->index = oldIdx;
		oldIdx += step;
	}
	modOrder[newIdx] = mod;
	return true;
}
void DiceModManager::turn_over(size_t idx) {
	std::lock_guard lock(ModMutex);
	if (idx >= modOrder.size())return;
	auto mod{ modOrder[idx] };
	mod->active = !mod->active;
	save();
	build();
}
const std::string getMsg(const std::string& key, const AttrObject& maptmp) {
	return fmt->format(fmt->msg_get(key), maptmp);
}

AttrVar DiceModManager::format(const string& s, const AttrObject& context, bool isTrust, const dict_ci<string>& dict) const {
	//直接重定向
	if (s[0] == '&') {
		const string key = s.substr(1);
		if (auto val{ getContextItem(context, key, isTrust) }) {
			return val;
		}
		else if (const auto it = dict.find(key); it != dict.end()) {
			return format(it->second, context, isTrust, dict);
		}
		else if (const auto it = global_speech.find(key); it != global_speech.end()) {
			return format(it->second.express(), context, isTrust, dict);
		}
	}
	if (s.find('{') == string::npos)return s;
	return formatMsg(s, context, isTrust, dict);
}
std::shared_mutex GlobalMsgMutex;
string DiceModManager::msg_get(const string& key)const {
	std::shared_lock lock(GlobalMsgMutex);
	if (const auto it = EditedMsg.find(key); it != EditedMsg.end())
		return it->second;
	else if (const auto sp = global_speech.find(key); sp != global_speech.end())
		return sp->second.express();
	else
		return "";
}
void DiceModManager::msg_reset(const string& key){
	std::shared_lock lock(GlobalMsgMutex);
	if (const auto it = EditedMsg.find(key); it != EditedMsg.end()) {
		EditedMsg.erase(key);
		if (PlainMsg.count(key)) {
			global_speech[key] = GlobalMsg[key] = PlainMsg.find(key)->second;
		}
		else {
			global_speech.erase(key);
			GlobalMsg.erase(it->first);
		}
		saveJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg);
	}
}
void DiceModManager::msg_edit(const string& key, const string& val){
	std::shared_lock lock(GlobalMsgMutex);
	global_speech[key] = GlobalMsg[key] = EditedMsg[key] = val;
	saveJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg);
}

string DiceModManager::get_help(const string& key, const AttrObject& context) const{
	if (const auto it = global_helpdoc.find(key); it != global_helpdoc.end()){
		return format(it->second, context, true, global_helpdoc);
	}
	return {};
}

struct help_sorter {
	bool operator()(const string& _Left, const string& _Right) const {
		auto lit = fmt->cntHelp.find(_Left),
			rit = fmt->cntHelp.find(_Right);
		if (lit != rit) {
			if (rit != fmt->cntHelp.end())
				return true;
			else if (lit != fmt->cntHelp.end())
				return false;
			else if (lit->second != rit->second)
				return lit->second < rit->second;
		}
		else if (_Left.length() != _Right.length()) {
			return _Left.length() > _Right.length();
		}
		return _Left < _Right;
	}
};
string DiceModManager::prev_help(const string& key, const AttrObject& context) const {
	if (const auto it = global_helpdoc.find(key); it != global_helpdoc.end()) {
		return it->second;
	}
	else if (auto keys = querier.search(key); !keys.empty()) {
		if (keys.size() == 1) {
			auto word{ *keys.begin() };
			context->set("redirect_key", word);
			context->set("redirect_res", global_helpdoc.at(word));
			return getMsg("strHelpRedirect", context);
		}
		else {
			std::priority_queue<string, vector<string>, help_sorter> qKey;
			for (auto& key : keys) {
				qKey.emplace(".help " + key);
			}
			ShowList res;
			while (!qKey.empty()) {
				res << qKey.top();
				qKey.pop();
				if (res.size() > 20)break;
			}
			context->set("res", res.show("\n"));
			return getMsg("strHelpSuggestion", context);
		}
	}
	else return getMsg("strHelpNotFound", context);
	return {};
}

void DiceModManager::_help(DiceEvent* job) {
	string word{ job->get_str("help_word") };
	if (word.empty()) {
		job->reply(getMsg("strBotHeader") + Dice_Short_Ver + "\n" + getMsg("strHlpMsg"));
		return;
	}
	else if (const auto it = global_helpdoc.find(word);
		it != global_helpdoc.end()) {
		job->reply(format(it->second, *job, true, global_helpdoc));
	}
	else if (auto keys = querier.search(word);!keys.empty()) {
		if (keys.size() == 1) {
			auto word{ *keys.begin() };
			job->set("redirect_key", word);
			job->set("redirect_res", get_help(word, *job));
			job->replyMsg("strHelpRedirect");
		}
		else {
			std::priority_queue<string, vector<string>, help_sorter> qKey;
			for (auto& key : keys) {
				qKey.emplace(".help " + key);
			}
			ShowList res;
			while (!qKey.empty()) {
				res << qKey.top();
				qKey.pop();
				if (res.size() > 20)break;
			}
			job->set("res", res.show("\n"));
			job->replyMsg("strHelpSuggestion");
		}
	}
	else job->replyMsg("strHelpNotFound");
	cntHelp[word] += 1;
	saveJMap(DiceDir / "user" / "HelpStatic.json", cntHelp);
}

void DiceModManager::set_help(const string& key, const string& val){
	if (!global_helpdoc.count(key))querier.insert(key);
	global_helpdoc[key] = CustomHelp[key] = (val == "NULL") ? "" : val;
	saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
}
void DiceModManager::rm_help(const string& key){
	if(CustomHelp.erase(key)){
		saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
		global_helpdoc.erase(key);
	}
}

time_t parse_seconds(const AttrVar& time) {
	if (time.is_numberic())return time.to_int();
	if (auto t{ time.to_obj() }; !t->empty())
		return t->get_ll("second") + t->get_ll("minute") * 60 + t->get_ll("hour") * 3600 + t->get_ll("day") * 86400;
	return 0;
}
Clock parse_clock(const AttrVar& time) {
	Clock clock{ 0,0 };
	if (auto t{ time.to_obj() }; !t->empty()) {
		clock.first = t->get_int("hour");
		clock.second = t->get_int("minute");
	}
	return clock;
}

void DiceModManager::call_cycle_event(const string& id) {
	if (id.empty() || !global_events.count(id))return;
	AttrObject& eve{ global_events[id] };
	if (auto trigger{ eve->get_obj("trigger") }; trigger->has("cycle")) {
		sch.add_job_for(parse_seconds(trigger->at("cycle")), eve);
	}
	if (auto action{ eve->get_obj("action") })call_event(eve.p, action);
}
void DiceModManager::call_clock_event(const string& id) {
	if (id.empty() || !global_events.count(id))return;
	AttrObject& eve{ global_events[id] };
	if (auto action{ eve->get_obj("action") })call_event(eve.p, action);
}
bool DiceModManager::call_hook_event(const AttrObject& eve) {
	string hookEvent{ eve->has("hook") ? eve->get_str("hook") : eve->get_str("Event") };
	if (hookEvent.empty())return false;
	for (auto& [id, hook] : multi_range(hook_events, hookEvent)) {
		if (auto action{ hook->get_obj("action")}) {
			if (hookEvent == "StartUp" || hookEvent == "DayEnd" || hookEvent == "DayNew") {
				if (action->has("lua")) {
					std::thread th(lua_call_event, eve.p, action->at("lua"));
					th.detach();
				}
				if (action->has("js")) {
					std::thread th(js_call_event, eve.p, action->at("js"));
					th.detach();
				}
#ifdef DICE_PYTHON
				if (action->has("py")) {
					std::thread th(py_call_event, eve.p, action->at("py"));
					th.detach();
				}
#endif //DICE_PYTHON
			}
			else call_event(eve.p, action);
		}
	}
	return eve->is("blocked");
}

string DiceModManager::list_reply(int type)const {
	ShowList listTotal;
	ResList listMatch, listSearch, listPrefix, listRegex;
	for (const auto& [key, reply] : final_reply.match_items) {
		if (type & (int)reply->type)listMatch << key;
	}
	for (const auto& [key, reply] : final_reply.prefix_items) {
		if (type & (int)reply->type)listPrefix << key;
	}
	for (const auto& [key, reply] : final_reply.search_items) {
		if (type & (int)reply->type)listSearch << key;
	}
	for (const auto& [key, reply] : final_reply.regex_items) {
		if (type & (int)reply->type)listRegex << key;
	}
	if (!listMatch.empty())listTotal << "[完全匹配] " + listMatch.dot(" | ").show();
	if (!listPrefix.empty())listTotal << "[前缀匹配] " + listPrefix.dot(" | ").show();
	if (!listSearch.empty())listTotal << "[模糊匹配] " + listSearch.dot(" | ").show();
	if (!listRegex.empty())listTotal << "[正则匹配] " + listRegex.dot(" | ").show();
	return "\n" + ((listTotal.size() > 1 && listTotal.length() > 256)
		? listTotal.show("\f") : listTotal.show("\n"));
}
void DiceModManager::set_reply(const string& key, ptr<DiceMsgReply> reply) {
	final_reply.insert(key, custom_reply[key] = reply);
	save_reply();
}
bool DiceModManager::del_reply(const string& key) {
	if (custom_reply.erase(key)) {
		final_reply.erase(key);
		save_reply();
		return true;
	}
	return false;
}
void DiceModManager::save_reply() {
	fifo_json j = fifo_json::object();
	for (const auto& [word, reply] : custom_reply) {
		j[GBKtoUTF8(word)] = reply->writeJson();
	}
	if (j.empty()) std::filesystem::remove(DiceDir / "conf" / "CustomMsgReply.json");
	else fwriteJson(DiceDir / "conf" / "CustomMsgReply.json", j, 0);
}
void DiceModManager::reply_get(DiceEvent* msg) {
	string key{ msg->get_str("key")};
	if (final_reply.items.count(key)) {
		msg->set("show",final_reply.items[key]->print());
		msg->reply(getMsg("strReplyShow"));
	}
	else {
		msg->reply(getMsg("strReplyKeyNotFound"));
	}
}
void DiceModManager::reply_show(DiceEvent* msg) {
	string key{ msg->get_str("key") };
	if (final_reply.items.count(key)) {
		msg->set("show", final_reply.items[key]->show());
		msg->reply(getMsg("strReplyShow"));
	}
	else {
		msg->reply(getMsg("strReplyKeyNotFound"));
	}
}

bool DiceModManager::call_task(const string& task) {
	if (auto it{ taskcall.find(task) }; it != taskcall.end()) {
		std::thread th(lua_call_task, it->second);
		th.detach();
		return true;
	}
	return false;
}

string DiceModManager::lua_path(const string& name)const {
	if (auto it{ global_lua_scripts.find(name) }; it != global_lua_scripts.end()) {
		return it->second;
	}
	return {};
}
string DiceModManager::js_path(const string& name)const {
	if (auto it{ global_js_scripts.find(name) }; it != global_js_scripts.end()) {
		return it->second;
	}
	return {};
}
std::optional<std::filesystem::path> DiceModManager::py_path(const string& name)const {
	if (auto it{ global_py_scripts.find(name) }; it != global_py_scripts.end()) {
		return it->second;
	}
	return std::nullopt;
}

#ifndef __ANDROID__
DiceMod::DiceMod(const string& mod, size_t i, const string& url) :name(mod), index(i), pathDir(DiceDir / "mod" / mod),
repo(std::make_shared<DiceRepo>(pathDir, url)) {
	if (!fs::exists(pathDir)) {
		console.log(getMsg("strSelfNick") + "安装mod「" + mod + "」仓库失败！", 0);
	}
	else if (!fs::exists(pathDir / "descriptor.json")) {
		console.log(getMsg("strSelfNick") + "安装mod「" + mod + "」异常:找不到descriptor.json", 1);
	}
	else {
		pathJson = DiceDir / "mod" / (mod + ".json");
		fs::copy_file(pathDir / "descriptor.json", pathJson, fs::copy_options::overwrite_existing);
		string reason;
		if (!loadDesc(reason)) {
			console.log(getMsg("strSelfNick") + "安装安装「" + mod + "」失败:" + reason, 0);
		}
	}
}
void DiceMod::remote(const string& url) {
	if (repo)repo.reset();
	repo = std::make_shared<DiceRepo>(pathDir, url);
}
#endif //ANDROID

string DiceMod::desc()const {
	ShowList li;
	li << "[" + to_string(index) + "]" + (title.empty() ? name : title);
	if (!ver.exp.empty())li << "- 版本: " + ver.exp;
	if (!author.empty())li << "- 作者: " + author;
	if (!brief.empty())li << "- 简介: " + brief;
	return li.show("\n");
}
string DiceMod::detail()const {
	ShowList li;
	if (!rules.empty())li << "- 规则集: " + to_string(rules.size()) + " 部";
	if (!card_models.empty())li << "- 角色卡: " + to_string(card_models.size()) + " 版";
	if (!events.empty())li << "- 事件: " + to_string(events.size()) + "条";
	if (!reply_list.empty())li << "- 回复: " + to_string(reply_list.size()) + "项";
	if (!lua_scripts.empty())li << "- lua脚本: " + to_string(lua_scripts.size()) + "份";
	if (!js_scripts.empty())li << "- js脚本: " + to_string(js_scripts.size()) + "份";
	if (!py_scripts.empty())li << "- py脚本: " + to_string(py_scripts.size()) + "份";
	if (!helpdoc.empty())li << "- 帮助: " + to_string(helpdoc.size()) + "条";
	if (!speech.empty())li << "- 台词: " + to_string(speech.size()) + "项";
	if (cntImage)li << "- 图像: " + to_string(cntImage) + "份";
	if (cntAudio)li << "- 音频: " + to_string(cntAudio) + "份";
	return desc() + "\n" + li.show("\n");
}
bool DiceMod::loadDesc(string& cb) {
	try {
		fifo_json j = freadJson(pathJson);
		if (j.is_null()) {
			cb = "(未读取到json)";
			return false;
		}
		if (j.count("dice_build")) {
			if (j["dice_build"] > Dice_Build) {
				cb = "(Dice!build低于所需" + to_string(j["dice_build"].get<int>()) + ")";
				return false;
			}
		}
		if (j.count("require")) {
			ShowList fault;
			ShowList post;
			for (auto& depend : j["require"]) {
				if (!fmt->modList.count(depend)) {
					fault << depend;
				}
				else if (fmt->modList[depend]->index > index) {
					post << depend;
				}
			}
			if (!fault.empty()) {
				cb = "(缺少前置mod:" + fault.show() + ")";
				return false;
			}
			else if (!post.empty()) {
				console.log("警告：" + name
					+ "与前置mod[" + post.show() + "]顺序倒置", 1);
			}
		}
		if (j.count("title"))title = UTF8toGBK(j["title"].get<string>());
		else if (j.count("mod"))title = UTF8toGBK(j["mod"].get<string>());
		if (j.count("ver"))ver = UTF8toGBK(j["ver"].get<string>());
		if (j.count("author"))author = UTF8toGBK(j["author"].get<string>());
		if (j.count("brief"))brief = UTF8toGBK(j["brief"].get<string>());
		if (j.count("helpdoc")) {
			readJMap(j["helpdoc"], helpdoc);
		}
		if (j.count("speech")) {
			for (auto& it : j["speech"].items()) {
				speech[UTF8toGBK(it.key())] = it.value();
			}
		}
		loadDir();
	}
	catch (fifo_json::exception& e) {
		cb = "(json解析错误)";
		return false;
	}
	return true;
}
void DiceMod::loadDir() {
	if (loaded)return;
	if (fs::exists(pathDir)) {
#ifndef __ANDROID__
		if (!repo && fs::exists(pathDir / ".git")) {
			repo = std::make_shared<DiceRepo>(pathDir);
		}
#endif //ANDROID
		if (fs::exists(pathDir / "speech")) {
			vector<std::filesystem::path> fSpeech;
			listDir(pathDir / "speech", fSpeech, true);
			for (auto& p : fSpeech) {
				try {
					YAML::Node yaml{ YAML::LoadFile(getNativePathString(p)) };
					if (yaml.IsMap()) {
						for (auto it : yaml) {
							speech[UTF8toGBK(it.first.Scalar())] = it.second;
						}
					}
				}
				catch (std::exception& e) {
					console.log(UTF8toGBK(cut_relative(p, pathDir).u8string()) + "解析错误!" + e.what(), 0b10);
				}
			}
		}
		if (auto dirScript{ pathDir / "script" }; fs::exists(dirScript)) {
			vector<std::filesystem::path> fScripts;
			listDir(dirScript, fScripts, true);
			for (auto p : fScripts) {
				if (p.extension() == ".lua") {
					string script_name{ cut_stem((p.stem() == "init") ? p.parent_path() : p,dirScript) };
					lua_scripts[script_name] = getNativePathString(p);
				}
				else if (p.extension() == ".js") {
					string script_name{ cut_stem(p,dirScript) };
					js_scripts[script_name] = getNativePathString(p);
				}
				else if (p.extension() == ".py") {
					string script_name{ cut_stem(p,dirScript) };
					py_scripts[script_name] = p;
				}
			}
		}
		listDir(pathDir / "reply", luaFiles, true);
		listDir(pathDir / "event", luaFiles, true);
		loadLua();
		if (auto dirRule{ pathDir / "rulebook" }; fs::exists(dirRule)) {
			if(vector<std::filesystem::path> fSpeech; listDir(dirRule, fSpeech))
			for (auto& p : fSpeech) {
				try {
					if (YAML::Node yaml{ YAML::LoadFile(getNativePathString(p)) }; yaml.IsMap()) {
						string rulename{ UTF8toGBK(yaml["rule"].Scalar()) };
						auto& rule{ rules[rulename]};
						if (yaml["manual"])for (auto it : yaml["manual"]) {
							rule.manual[UTF8toGBK(it.first.Scalar())] = UTF8toGBK(it.second.Scalar());
						}
						if (yaml["tape"])for (auto it : yaml["tape"]) {
							rule.cassettes[UTF8toGBK(it.first.Scalar())] = AttrVar(it.second).to_obj();
						}
					}
					else console.log(UTF8toGBK(cut_relative(p, pathDir).u8string()) + "yaml格式不为对象!", 0b10);
				}
				catch (std::exception& e) {
					console.log(UTF8toGBK(cut_relative(p, pathDir).u8string()) + "解析错误!" + e.what(), 0b10);
				}
			}
		}
		if (auto dirModel{ pathDir / "model" }; fs::exists(dirModel)) {
			if (vector<std::filesystem::path> fModels; listDir(dirModel, fModels, true))
				for (auto& p : fModels) {
					loadCardTemp(p, card_models);
				}
		}
		if (fs::exists(pathDir / "image")) {
			std::filesystem::create_directories(dirExe / "data" / "image");
			std::filesystem::copy(pathDir / "image", dirExe / "data" / "image",
				std::filesystem::copy_options::recursive |
				(Enabled ? std::filesystem::copy_options::update_existing : std::filesystem::copy_options::overwrite_existing));
			cntImage = cntDirFile(pathDir / "image");
		}
		if (fs::exists(pathDir / "audio")) {
			std::filesystem::create_directories(dirExe / "data" / "record");
			std::filesystem::copy(pathDir / "audio", dirExe / "data" / "record",
				(Enabled ? std::filesystem::copy_options::update_existing : std::filesystem::copy_options::overwrite_existing));
			cntAudio = cntDirFile(pathDir / "audio");
		}
	}
	loaded = true;
}
bool DiceMod::reload(string& cb) {
	helpdoc.clear();
	speech.clear();
	lua_scripts.clear();
	js_scripts.clear();
	py_scripts.clear();
	luaFiles.clear();
	reply_list.clear();
	events.clear();
	cntAudio = cntImage = 0;
	loaded = false;
	return loadDesc(cb);
}
int DiceModManager::load(ResList& resLog){
	//读取mod管理文件
	auto jFile{ freadJson(DiceDir / "conf" / "ModList.json") };
	if (!jFile.empty()) {
		for (auto& j : jFile) {
			if (!j.count("name"))continue;
			string modName{ UTF8toGBK(j["name"].get<string>()) };
			auto mod{ std::make_shared<DiceMod>(DiceMod{ modName,modOrder.size(),
				j.count("active") ? bool(j["active"]) : true}) };
			modList[modName] = mod;
			modOrder.push_back(mod);
		}
	}
	//读取mod
	vector<std::filesystem::path> ModFile;
	vector<string> sModErr;
	auto dirMod{ DiceDir / "mod" };
	if (int cntMod{ listDir(dirMod, ModFile) }; cntMod > 0) {
		bool newMod{ false };
		for (auto& pathMod : ModFile) {
			if (pathMod.extension() != ".json")continue;
			if (string modName{ UTF8toGBK(pathMod.stem().u8string()) }; modList.count(modName)) {
				auto mod{ modList[modName] };
				mod->file(pathMod);
			}
			else {
				auto mod{ std::make_shared<DiceMod>(DiceMod{ modName,modOrder.size(),true}) };
				modList[modName] = mod;
				modOrder.push_back(mod);
				mod->file(pathMod);
				newMod = true;
			}
		}
		if (newMod)save();
	}
	if (!modList.empty()) {
		int cntMod{ 0 };
		string reason;
		for (auto& mod : modOrder) {
			if (mod->pathJson.empty())continue;
			try {
				if (mod->loadDesc(reason))
					++cntMod;
				else sModErr.push_back(mod->name + reason);
			}
			catch (fifo_json::exception& e) {
				sModErr.push_back(mod->name + "(.json解析错误)");
				continue;
			}
		}
		if (cntMod)resLog << "读取/mod/中的" + std::to_string(cntMod) + "枚mod";
		if (!sModErr.empty()) {
			resLog << "读取失败" + std::to_string(sModErr.size()) + "项:";
			for (auto& it : sModErr) {
				resLog << it;
			}
		}
	}
	//custom_help
	if (loadJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp) == -1)
		resLog << "解析/conf/CustomHelp.json失败！";
	else if(!CustomHelp.empty())
		resLog << "读取/conf/CustomHelp.json中的" + std::to_string(CustomHelp.size()) + "条帮助词条";
	loadJMap(DiceDir / "user" / "HelpStatic.json", cntHelp);
	//custom_reply
	loadPlugin(resLog);
	if (fifo_json jFile = freadJson(DiceDir / "conf" / "CustomMsgReply.json"); !jFile.empty()) {
		try {
			for (auto& reply : jFile.items()) {
				if (std::string key = UTF8toGBK(reply.key()); !key.empty()) {
					ptr<DiceMsgReply> p{ custom_reply[key] = std::make_shared<DiceMsgReply>() };
					p->title = key; 
					p->readJson(reply.value());
				}
			}
			resLog << "读取/conf/CustomMsgReply.json中的" + std::to_string(jFile.size()) + "条自定义回复";
		}
		catch (const std::exception& e) {
			resLog << "解析/conf/CustomMsgReply.json出错:" << e.what();
		}
	}
	else {
		std::map<std::string, std::vector<std::string>, less_ci> mRegexReplyDeck;
		std::map<std::string, std::vector<std::string>, less_ci> mReplyDeck;
		if (loadJMap(DiceDir / "conf" / "CustomReply.json", mReplyDeck) > 0) {
			resLog << "迁移CustomReply" + to_string(mReplyDeck.size()) + "条";
			for (auto& [key, deck] : mReplyDeck) {
				ptr<DiceMsgReply> reply{ custom_reply[key] = std::make_shared<DiceMsgReply>() };
				reply->title = key;
				reply->keyMatch[0] = std::make_unique<vector<string>>(vector<string>{ key });
				reply->answer = AnysTable(deck);
			}
		}
		if (loadJMap(DiceDir / "conf" / "CustomRegexReply.json", mRegexReplyDeck) > 0) {
			resLog << "迁移正则Reply" + to_string(mRegexReplyDeck.size()) + "条";
			for (auto& [key, deck] : mRegexReplyDeck) {
				ptr<DiceMsgReply> reply{ custom_reply[key] = std::make_shared<DiceMsgReply>() };
				reply->title = key;
				reply->keyMatch[3] = std::make_unique<vector<string>>(vector<string>{ key });
				reply->answer = AnysTable(deck);
			}
		}
		if(!custom_reply.empty())save_reply();
	}
	//init
	std::thread th{ &DiceModManager::initCloud,this };
	th.detach();
	build();
	return 0;
}
void DiceModManager::initCloud() {
	if (auto p{ DiceDir / "conf" / "mod" / "source.list" }; fs::exists(p)) {
		std::ifstream fin{ p };
		sourceList.clear();
		string url;
		while (getline(fin >> std::skipws, url)) {
			if (0 == url.find("#"))continue;
			sourceList.push_back(url);
		}
	}
}
void DiceModManager::build() {
	isIniting = true;
	ShowList resLog;
	size_t cntSpeech{ 0 }, cntHelp{ 0 }, cntModel{ 0 };
	//init
	map_merge(global_speech = transpeech, GlobalMsg);
	global_helpdoc = HelpDoc;
	global_lua_scripts.clear();
	global_js_scripts.clear();
	global_py_scripts.clear();
	global_events.clear();
	final_reply = {};
	auto rules_new = std::make_shared<DiceRuleSet>();
	dict_ci<ptr<CardTemp>> models{
	{"BRP", std::make_shared<CardTemp>(ModelBRP),},
	{"COC7", std::make_shared<CardTemp>(ModelCOC7),},
	};
	//merge mod
	for (auto& mod : modOrder) {
		if (!mod->active || !mod->loaded)continue;
		map_merge(global_lua_scripts, mod->lua_scripts);
		map_merge(global_js_scripts, mod->js_scripts);
		map_merge(global_py_scripts, mod->py_scripts);
		cntSpeech += map_merge(global_speech, mod->speech);
		cntHelp += map_merge(global_helpdoc, mod->helpdoc);
		rules_new->merge(mod->rules);
		map_merge(final_reply.items, mod->reply_list);
		map_merge(global_events, mod->events);
		for (auto& [name, model] : mod->card_models) {
			if (models.count(name)) {
				models[name]->merge(model);
			}
			else {
				models[name] = std::make_shared<CardTemp>(model);
			}
			++cntModel;
		}
	}
	//merge custom
	if (rules_new->build())resLog << "注册规则集 " + to_string(rules_new->rules.size()) + " 部";
	ruleset.swap(rules_new);
	if (cntModel)resLog << "注册角色卡模板 " + to_string(cntModel) + " 版";
	for (auto& [name, model] : models) {
		model->init();
	}
	if (cntModel || CardModels.size() > 2)CardModels.swap(models);
	if (cntSpeech += map_merge(global_speech, EditedMsg))
		resLog << "注册speech " + to_string(cntSpeech) + " 项";
	if (cntHelp += map_merge(global_helpdoc, CustomHelp))
		resLog << "注册help " + to_string(cntHelp) + " 项";
	querier = {};
	for (const auto& [key, word] : global_helpdoc) {
		querier.insert(key);
	}
	map_merge(final_reply.items, plugin_reply);
	map_merge(final_reply.items, custom_reply);
	if (!final_reply.items.empty()) {
		resLog << "注册reply " + to_string(final_reply.items.size()) + " 项";
		final_reply.build();
	}
	if (!global_lua_scripts.empty())resLog << "注册lua脚本 " + to_string(global_lua_scripts.size()) + " 份";
	if (!global_js_scripts.empty())resLog << "注册js脚本 " + to_string(global_js_scripts.size()) + " 份";
	if (!global_py_scripts.empty())resLog << "注册py脚本 " + to_string(global_py_scripts.size()) + " 份";
	if (!global_events.empty()) {
		resLog << "注册event " + to_string(global_events.size()) + " 项";
		clock_events.clear();
		hook_events.clear();
		unordered_set<string> cycle;
		for (auto& [id, eve] : global_events) {
			eve->at("id") = id;
			auto trigger{ eve->get_obj("trigger") };
			if (trigger->has("cycle")) {
				if (!cycle_events.count(id)) {
					call_cycle_event(id);
				}
				cycle.insert(id);
			}
			if (trigger->has("clock")) {
				auto& clock{ trigger->at("clock") };
				if (auto list{ clock.to_list() }) {
					for (auto& clc : *list) {
						clock_events.emplace(parse_clock(clc), id);
					}
				}
				else {
					clock_events.emplace(parse_clock(clock), id);
				}
			}
			if (trigger->has("hook")) {
				string nameEvent{ trigger->get_str("hook") };
				hook_events.emplace(nameEvent, eve);
			}
		}
		cycle_events.swap(cycle);
	}
	if (!resLog.empty()) {
		resLog << "模块加载完毕√";
		console.log(getMsg("strSelfName") + "\n" + resLog.show("\n"), int(Enabled), printSTNow());
	}
	isIniting = false;
}
void DiceModManager::clear(){
	modOrder.clear();
	modList.clear();
	selfdata_byFile.clear();
	selfdata_byStem.clear();
}

void DiceModManager::save() {
	fifo_json jFile = fifo_json::array();
	for (auto& mod : modOrder) {
		fifo_json j = fifo_json::object();
		j["name"] = GBKtoUTF8(mod->name);
		j["active"] = mod->active;
		jFile.push_back(j);
	}
	if (!jFile.empty()) {
		fwriteJson(DiceDir / "conf" / "ModList.json", jFile, 0);
	}
	else {
		remove(DiceDir / "conf" / "ModList.json");
	}
}
void call_event(const ptr<AnysTable>& eve, const AttrObject& action) {
	if (action->has("lua"))lua_call_event(eve, action->at("lua"));
	if (action->has("js"))js_call_event(eve, action->at("js"));
#ifdef DICE_PYTHON
	if (action->has("py"))py_call_event(eve, action->at("py"));
#endif //DICE_PYTHON
}