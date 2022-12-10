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
#include "DiceMod.h"
#include "GlobalVar.h"
#include "ManagerSystem.h"
#include "Jsonio.h"
#include "DiceFile.hpp"
#include "DiceEvent.h"
#include "DiceLua.h"
#include "DiceMsgReply.h"
#include "DiceSelfData.h"
#include "RandomGenerator.h"
#include "DDAPI.h"
#include "yaml-cpp/yaml.h"
#include <regex>

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
		return p->at(RandomGenerator::Randint(0, p->size() - 1));
	}
	return {};
}

dict_ci<DiceSpeech> transpeech{
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
			fifo_json j = fifo_json::parse(desc);
			//todo: dice_build check
#ifndef __ANDROID__
			if (j.count("repo") && !j["repo"].empty()) {
				string repo{ j["repo"] };
				auto mod{ std::make_shared<DiceMod>(DiceMod{ name,modOrder.size(),repo}) };
				modList[name] = mod;
				modOrder.push_back(mod);
				if (!mod->loaded) {
					msg.set("err", msg.get_str("err") + "\ngit clone失败:" + repo);
					continue;
				}
				save();
				build();
				msg.set("mod_ver", mod->ver.exp);
				msg.replyMsg("strModInstalled");
				return;
			}
#endif //ANDROID
			if (j.count("pkg")) {
				string pkg{ j["pkg"] };
				std::string des;
				if (!Network::GET(pkg, des))	{
					msg.set("err", msg.get_str("err") + "\n下载失败(" + pkg + "):" + des);
					continue;
				}
				std::error_code ec1;
				Zip::extractZip(des, DiceDir / "mod");
				auto pathJson{ DiceDir / "mod" / (name + ".json") };
				if (!fs::exists(pathJson)) {
					if (fs::path desc{ DiceDir / "mod" / name / "descriptor.json" }; fs::exists(desc)) {
						fs::copy(desc, pathJson);
					}
					else {
						msg.set("err", msg.get_str("err") + "\npkg解压无文件" + name + ".json");
						continue;
					}
				}
				auto mod{ std::make_shared<DiceMod>(DiceMod{ name,modOrder.size(),true}) };
				modList[name] = mod;
				modOrder.push_back(mod);
				string err;
				if (mod->file(pathJson).loadDesc(err)) {
					save();
					build();
					msg.set("mod_ver", mod->ver.exp);
					msg.replyMsg("strModInstalled");
					return;
				}
				else {
					msg.set("err", msg.get_str("err") + "\n" + err + "(" + url + name + ")");
					continue;
				}
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
void DiceModManager::turn_over(size_t idx) {
	std::lock_guard lock(ModMutex);
	if (idx >= modOrder.size())return;
	auto mod{ modOrder[idx] };
	mod->active = !mod->active;
	save();
	build();
}
static enumap<string> methods{ "var","print","help","sample","case","vary","grade","at","ran","wait" };
enum class FmtMethod { Var, Print, Help, Sample, Case, Vary, Grade, At, Ran, Wait};
struct ParseNode {
	string leaf;
	size_t pos;
	char token;
	ParseNode(const string& s, size_t p, char t) :leaf(s), pos(p), token(t) {}
	shared_ptr<ParseNode> next;
	shared_ptr<ParseNode> first_kid;
	//shared_ptr<ParseNode> last_kid;
};
class Parser {
	//string exp;
	AttrObject context;
	bool isTrust{ true };
	dict_ci<string> dict;
	shared_ptr<ParseNode> root;
public:
	Parser(const string& s, const AttrObject& obj, bool t = true, const dict_ci<string>& con = {})
		:root(std::make_shared<ParseNode>(s, 0, char(0xAA))), context(obj), isTrust(t), dict(con) {
		string& exp{ root->leaf };
		auto parent{ root };
		char chSign[3]{ char(0xAA),char(0xAA),'\0' };
		size_t preL{ 0 }, lastL{ 0 }, lastR{ 0 }; 
		while ((lastR = exp.find('}', ++lastR)) != string::npos
			&& (lastL = exp.rfind('{', lastR)) != string::npos) {
			string key;
			//括号前加‘\’表示该括号内容不转义
			if (exp[lastR - 1] == '\\') {
				lastL = lastR - 1;
				key = "}";
			}
			else if (lastL > 0 && exp[lastL - 1] == '\\') {
				lastR = lastL--;
				key = "{";
			}
			else key = exp.substr(lastL + 1, lastR - lastL - 1);
			auto node{ std::make_shared<ParseNode>(key,lastL,++chSign[1]) };
			if (!root->first_kid){
				parent = root->first_kid = node;
			}
			else if (lastL >= preL) {
				parent = parent->next = node;
			}
			else if (lastL < root->first_kid->pos) {
				node->first_kid = root->first_kid;
				parent = root->first_kid = node;
			}
			else {
				parent = root->first_kid;
				while (parent->next) {
					if (parent->next->pos < lastL) {
						parent = parent->next;
					}
					else {
						node->first_kid = parent->next;
						parent = parent->next = node;
						break;
					}
				}
			}
			exp.replace(lastL, lastR - lastL + 1, chSign);
			lastR = (preL = lastL) + 1;
		}
		if(root->first_kid)format_token(exp, root);
	}
	AttrVar format_token(string& s, shared_ptr<ParseNode> it) {
		char chSign[3]{ char(0xAA),char(0xAA),'\0' };
		size_t pos{ 0 };
		it = it->first_kid;
		while (it) {
			chSign[1] = it->token;
			if ((pos = s.find(chSign)) != string::npos) {
				string& key{ it->leaf };
				AttrVar val{ "{" + key + "}" };
				if (key == "{" || key == "}") {
					val = key;
				}
				else if (context.has(key)) {
					if (key == "res")val = fmt->format(context.print(key), context, isTrust, dict);
					else val = context.print(key);
				}
				else if (AttrVar res{ getContextItem(context, key, isTrust) }) {
					val = res;
				}
				else if (auto cit = dict.find(key); cit != dict.end()) {
					val = fmt->format(cit->second, context, isTrust, dict);
				}
				//语境优先于全局
				else if (auto sp = fmt->global_speech.find(key); sp != fmt->global_speech.end()) {
					val = fmt->format(sp->second.express(), context, isTrust, dict);
					if (!isTrust && val == "\f")val = "\f< ";
				}
				else if (size_t colon{ key.find(':') }; colon != string::npos) {
					string method{ key.substr(0,colon) };
					string para{ key.substr(colon + 1) };
					if (methods.count(method))switch ((FmtMethod)methods[method]) {
						case FmtMethod::Var:{
							auto posQ = para.find('?'), posE = para.find('=');
							if (posQ < posE) {
								auto [field, exp] = readini<string, string>(para, '?');
								field = format_token(field, it);
								context.set(field, val = (exp[0] == char(0xAA)) ? format_token(exp, it) : AttrVar::parse(exp));
							}
							else {
								auto exps{ splitPairs(para,'=','&') };
								for (auto& [field, exp] : exps) {
									if (exp.empty())context.set(field);
									else context.set(field, (exp[0] == char(0xAA)) ? format_token(exp, it) : AttrVar::parse(exp));
								}
								val.des();
							}
						} break;
						case FmtMethod::Help:
							val = fmt->get_help(format_token(para, it).to_str(), context);
							break;
						case FmtMethod::Sample:
							if (vector<string> samples{ split(para,"|") }; samples.empty())val = "";
							else
								val = samples[RandomGenerator::Randint(0, samples.size() - 1)];
							break;
						case FmtMethod::At:
							if (format_token(para, it) == "self") {
								val = "[CQ:at,qq=" + to_string(console.DiceMaid) + "]";
							}
							else if (!para.empty()) {
								val = "[CQ:at,qq=" + para + "]";
							}
							else if (context.has("uid")) {
								val = "[CQ:at,qq=" + context.get_str("uid") + "]";
							}
							else val.des();
							break;
						case FmtMethod::Print:
							if (auto paras{ splitPairs(para,'=','&') }; paras.count("uid")) {
								if (isTrust && paras["uid"].empty())val = printUser(context.get_ll("uid"));
								else val = printUser(AttrVar(paras["uid"]).to_ll());
							}
							else if (paras.count("gid")) {
								if (isTrust && paras["gid"].empty())val = printGroup(context.get_ll("gid"));
								else val = printGroup(AttrVar(paras["gid"]).to_ll());
							}
							else if (paras.count("master")) {
								val = console ? printUser(console) : "[无主]";
							}
							else val.des();
							break;
						case FmtMethod::Case:
						case FmtMethod::Vary: {
							auto [item, strVary] = readini<string, string>(para, '?');
							auto paras{ splitPairs(strVary,'=','&') };
							auto itemVal{ getContextItem(context, format_token(item, it), isTrust) };
							if (auto it{ paras.find(itemVal.print()) }; it != paras.end()
								|| (it = paras.find("else")) != paras.end()) {
								val = it->second;
							}
							else val.des();
						}break;
						case FmtMethod::Grade: {
							auto [item, strVary] = readini<string, string>(para, '?');
							auto paras{ splitPairs(strVary,'=','&') };
							auto itemVal{ getContextItem(context, format_token(item, it), isTrust) };
							grad_map<double, string>grade;
							for (auto& [step, value] : paras) {
								if (step == "else")grade.set_else(value);
								else if (isNumeric(step))
									grade.set_step(stod(step), value);
								else if (auto stepVal{ getContextItem(context,step, isTrust) }; stepVal.is_numberic())
									grade.set_step(stepVal.to_num(), value);
							}
							if (itemVal.is_numberic())val = grade[itemVal.to_num()];
							else val = grade.get_else();
						}break;
						case FmtMethod::Ran: {
							auto [min, max] = readini<string, string>(para, '~');
							int l = AttrVar::parse(format_token(min, it)).to_int(),
								r = AttrVar::parse(format_token(max, it)).to_int();
							val = (l == r) ? to_string(l)
								: (l < r) ? to_string(RandomGenerator::Randint(l, r))
								: to_string(RandomGenerator::Randint(r, l));
						}break;
						case FmtMethod::Wait:
							if (long long ms{ AttrVar::parse(format_token(para, it)).to_ll() }; 0 < ms && ms < 600000)
								std::this_thread::sleep_for(std::chrono::milliseconds(ms));
							val.des();
							break;
						default:
							break;
						}
				}
				else if (auto func = strFuncs.find(key); func != strFuncs.end()) {
					val = func->second();
				}
				if (s == chSign) {
					s = val;
					return val;
				}
				else if (val.is_null()) s.erase(pos, 2);
				else s.replace(pos, 2, val.print());
			}
			format_token(s, it);
			it = it->next;
		}
		return s;
	}
	operator string() {
		return root->leaf;
	}
};

string DiceModManager::format(const string& s, AttrObject context, bool isTrust, const dict_ci<string>& dict) const {
	//直接重定向
	if (s[0] == '&') {
		const string key = s.substr(1);
		if (auto val{ getContextItem(context, key, isTrust) }) {
			return val.print();
		}
		else if (const auto it = dict.find(key); it != dict.end()) {
			return format(it->second, context, isTrust, dict);
		}
		else if (const auto it = global_speech.find(key); it != global_speech.end()) {
			return fmt->format(it->second.express(), context, isTrust, dict);
		}
	}
	if (s.find('{') == string::npos)return s;
	return Parser(s, context, isTrust, dict);
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

string DiceModManager::get_help(const string& key, AttrObject context) const{
	if (const auto it = global_helpdoc.find(key); it != global_helpdoc.end()){
		return format(it->second, context, {}, global_helpdoc);
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

void DiceModManager::_help(DiceEvent* job) {
	if (job->is_empty("help_word")) {
		job->reply(getMsg("strBotHeader") + Dice_Short_Ver + "\n" + getMsg("strHlpMsg"));
		return;
	}
	else if (const auto it = global_helpdoc.find(job->get_str("help_word"));
		it != global_helpdoc.end()) {
		job->reply(format(it->second, *job, {}, global_helpdoc));
	}
	else if (auto keys = querier.search(job->get_str("help_word"));!keys.empty()) {
		if (keys.size() == 1) {
			auto word{ *keys.begin() };
			job->set("redirect_key", word);
			job->set("redirect_res", get_help(word, *job));
			console.log("近似匹配" + word + ":"+job->get_str("redirect_res"), 0);
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
	cntHelp[job->get_str("help_word")] += 1;
	saveJMap(DiceDir / "user" / "HelpStatic.json",cntHelp);
}

void DiceModManager::set_help(const string& key, const string& val){
	CustomHelp[key] = val;
	saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
	if (!global_helpdoc.count(key))querier.insert(key);
	global_helpdoc[key] = val;
}
void DiceModManager::rm_help(const string& key){
	if(CustomHelp.erase(key)){
		saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
		global_helpdoc.erase(key);
	}
}

time_t parse_seconds(const AttrVar& time) {
	if (time.is_numberic())return time.to_int();
	if (AttrObject t{ time.to_obj() }; !t.empty())
		return t.get_ll("second") + t.get_ll("minute") * 60 + t.get_ll("hour") * 3600 + t.get_ll("day") * 86400;
	return 0;
}
Clock parse_clock(const AttrVar& time) {
	Clock clock{ 0,0 };
	if (AttrObject t{ time.to_obj() }; !t.empty()) {
		clock.first = t.get_int("hour");
		clock.second = t.get_int("minute");
	}
	return clock;
}

void DiceModManager::call_cycle_event(const string& id) {
	if (id.empty() || !global_events.count(id))return;
	AttrObject eve{ global_events[id] };
	if (eve["action"].is_function()) {
		lua_call_event(eve, eve["action"]);
	}
	else if (auto action{ eve.get_dict("action") }; action->count("lua")) {
		lua_call_event(eve, action->at("lua"));
	}
	auto trigger{ eve.get_dict("trigger") };
	if (trigger->count("cycle")) {
		sch.add_job_for(parse_seconds(trigger->at("cycle")), eve);
	}
}
void DiceModManager::call_clock_event(const string& id) {
	if (id.empty() || !global_events.count(id))return;
	AttrObject eve{ global_events[id] };
	auto action{ eve["action"] };
	if (!action)return;
	else if (action.is_table() && action.to_dict()->count("lua")) {
		action = action.to_dict()->at("lua");
	}
	lua_call_event(eve, action);
}
bool DiceModManager::call_hook_event(AttrObject eve) {
	string hookEvent{ eve.has("hook") ? eve.get_str("hook") : eve.get_str("Event") };
	if (hookEvent.empty())return false;
	for (auto& [id, hook] : multi_range(hook_events, hookEvent)) {
		auto action{ hook["action"] };
		if (!action)continue;
		else if (action.is_table() && action.to_dict()->count("lua")) {
			action = action.to_obj()["lua"];
		}
		if (hookEvent == "StartUp" || hookEvent == "DayEnd" || hookEvent == "DayNew") {
			std::thread th(lua_call_event, eve, action);
			th.detach();
		}
		else lua_call_event(eve, action);
	}
	return eve.is("blocked");
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

string DiceModManager::script_path(const string& name)const {
	if (auto it{ global_scripts.find(name) }; it != global_scripts.end()) {
		return it->second;
	}
	return {};
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
	if (!events.empty())li << "- 事件: " + to_string(events.size()) + "条";
	if (!reply_list.empty())li << "- 回复: " + to_string(reply_list.size()) + "项";
	if (!scripts.empty())li << "- 脚本: " + to_string(scripts.size()) + "份";
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
					if (!yaml.IsMap()) {
						continue;
					}
					for (auto it : yaml) {
						speech[UTF8toGBK(it.first.Scalar())] = it.second;
					}
				}
				catch (std::exception& e) {
					console.log(getNativePathString(cut_relative(p, pathDir)) + "解析错误!" + e.what(), 0b10);
				}
			}
		}
		if (fs::exists(pathDir / "image")) {
			std::filesystem::copy(pathDir / "image", dirExe / "data" / "image",
				std::filesystem::copy_options::recursive |
				(Enabled ? std::filesystem::copy_options::update_existing : std::filesystem::copy_options::overwrite_existing));
			cntImage = cntDirFile(pathDir / "image");
		}
		if (fs::exists(pathDir / "audio")) {
			std::filesystem::copy(pathDir / "audio", dirExe / "data" / "record",
				std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
			cntAudio = cntDirFile(pathDir / "audio");
		}
		if (auto dirScript{ pathDir / "script" }; fs::exists(dirScript)) {
			vector<std::filesystem::path> fScripts;
			listDir(dirScript, fScripts, true);
			for (auto p : fScripts) {
				if (p.extension() != ".lua")continue;
				string script_name{ cut_stem((p.stem() == "init") ? p.parent_path() : p,dirScript) };
				string strPath{ getNativePathString(p) };
				scripts[script_name] = strPath;
			}
		}
		listDir(pathDir / "reply", luaFiles, true);
		listDir(pathDir / "event", luaFiles, true);
		loadLua();
	}
	loaded = true;
}
bool DiceMod::reload(string& cb) {
	helpdoc.clear();
	speech.clear();
	scripts.clear();
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
				reply->deck = deck;
			}
		}
		if (loadJMap(DiceDir / "conf" / "CustomRegexReply.json", mRegexReplyDeck) > 0) {
			resLog << "迁移正则Reply" + to_string(mRegexReplyDeck.size()) + "条";
			for (auto& [key, deck] : mRegexReplyDeck) {
				ptr<DiceMsgReply> reply{ custom_reply[key] = std::make_shared<DiceMsgReply>() };
				reply->title = key;
				reply->keyMatch[3] = std::make_unique<vector<string>>(vector<string>{ key });
				reply->deck = deck;
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
	size_t cntSpeech{ 0 }, cntHelp{ 0 };
	//init
	map_merge(global_speech = transpeech, GlobalMsg);
	global_helpdoc = HelpDoc;
	global_scripts.clear();
	global_events.clear();
	final_reply = {};
	//merge mod
	for (auto& mod : modOrder) {
		if (!mod->active || !mod->loaded)continue;
		map_merge(global_scripts, mod->scripts);
		cntSpeech += map_merge(global_speech, mod->speech);
		cntHelp += map_merge(global_helpdoc, mod->helpdoc);
		map_merge(final_reply.items, mod->reply_list);
		map_merge(global_events, mod->events);
	}
	//merge custom
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
	if (!global_scripts.empty())resLog << "注册script " + to_string(global_scripts.size()) + " 份";
	if (!global_events.empty()) {
		resLog << "注册event " + to_string(global_events.size()) + " 项";
		clock_events.clear();
		hook_events.clear();
		unordered_set<string> cycle;
		for (auto& [id, eve] : global_events) {
			eve["id"] = id;
			auto trigger{ eve.get_dict("trigger") };
			if (trigger->count("cycle")) {
				if (!cycle_events.count(id)) {
					call_cycle_event(id);
				}
				cycle.insert(id);
			}
			if (trigger->count("clock")) {
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
			if (trigger->count("hook")) {
				string nameEvent{ trigger->at("hook").to_str() };
				hook_events.emplace(nameEvent, eve);
			}
		}
		cycle_events.swap(cycle);
	}
	if (!resLog.empty()) {
		resLog << "模块加载完毕√";
		console.log(getMsg("strSelfName") + "\n" + resLog.show("\n"), 1, printSTNow());
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