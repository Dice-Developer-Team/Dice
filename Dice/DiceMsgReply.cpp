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
#include "DiceMod.h"
#include "DiceLua.h"
#include "DiceJS.h"
#include "DicePython.h"
#include "CardDeck.h"
#include "RandomGenerator.h"
#include "DDAPI.h"
void parse_vary(string& raw, fifo_dict_ci<pair<AttrVar::CMPR, AttrVar>>& vary) {
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
			conf.second.pop_back();
		}
		else if ('-' == *--conf.second.end()) {
			strCMPR = "-";
			cmpr = &AttrVar::equal_or_less;
			conf.second.pop_back();
		}
		else if ('!' == *--conf.second.end()) {
			strCMPR = "!";
			cmpr = &AttrVar::not_equal;
			conf.second.pop_back();
		}
		vary[conf.first] = { cmpr ,AttrVar::parse(conf.second) };
		vars << conf.first + "=" + conf.second + strCMPR;
	}
	raw = vars.show("&");
}
string parse_vary(const AttrVars& raw, fifo_dict_ci<pair<AttrVar::CMPR, AttrVar>>& vary) {
	ShowList vars;
	for (auto& [key, exp] : raw) {
		string var{ fmt->format(key) };
		if (!exp.is_table()) {
			vary[var] = { &AttrVar::equal,exp };
			vars << var + "=" + exp.print();
		}
		else {
			auto& tab{ exp.table };
			if (tab->has("equal")) {
				vary[var] = { &AttrVar::equal,tab->get("equal") };
				vars << var + "=" + tab->print("equal");
			}
			else if (tab->has("neq")) {
				vary[var] = { &AttrVar::not_equal , tab->get("neq") };
				vars << var + "=" + tab->print("neq") + "!";
			}
			else if (tab->has("at_least")) {
				vary[var] = { &AttrVar::equal_or_more , tab->get("at_least") };
				vars << var + "=" + tab->print("at_least") + "+";
			}
			else if (tab->has("at_most")) {
				vary[var] = { &AttrVar::equal_or_less , tab->get("at_most") };
				vars << var + "=" + tab->print("at_most") + "-";
			}
			else if (tab->has("more")) {
				vary[var] = { &AttrVar::more , tab->get("more") };
				vars << var + "=" + tab->print("more") + "++";
			}
			else if (tab->has("less")) {
				vary[var] = { &AttrVar::less , tab->get("less") };
				vars << var + "=" + tab->print("less") + "--";
			}
		}
	}
	return vars.show("&");
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
		else if (key == "lock") {
			ShowList sub;
			ShowList subnotes;
			if (colon == string::npos) {
				locks.emplace_back(CDType::Global, "", 1);
			}
			for (auto& key : split(item.substr(colon + 1), "&")) {
				string type{ "chat" };
				if (size_t pos{ key.find('@') }; pos != string::npos) {
					type = key.substr(pos + 1);
					key = key.substr(0, pos);
				}
				CDType cd_type{ (CDType)CDConfig::eType[type] };
				locks.emplace_back(cd_type, key, 1);
				sub << key + ((key.empty() && cd_type == CDType::Chat) ? ""
					: ("@" + CDConfig::eType[(size_t)cd_type]));
				subnotes << (cd_type == CDType::Chat ? "窗口锁"
					: cd_type == CDType::User ? "用户锁" : "全局锁") + key;
			}
			if (sub.empty())continue;
			limits << "lock:" + sub.show("&");
			notes << "- 同步锁: " + subnotes.show();
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
					if (type == "echo") {
						cd_notice = conf.second;
						continue;
					}
					else cd = conf.second;
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
			if (!cds.empty()) {
				if (!cd_notice.empty()) {
					cds << "@echo=" + cd_notice;
					cdnotes << "冷却回复: " + cd_notice;
				}
				limits << "cd:" + cds.show("&");
				notes << "- 冷却计时: " + cdnotes.show("\n - ");
			}
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
					if (type == "echo") {
						daylimit_notice = conf.second;
						continue;
					}
					else cd = conf.second;
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
			if (!sub.empty()) {
				if (!daylimit_notice.empty()) {
					sub << "@echo=" + daylimit_notice;
					subnotes << "限额回复: " + daylimit_notice;
				}
				limits << "today:" + sub.show("&");
				notes << "- 当日计数: " + subnotes.show("\n - ");
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
				vars << key + showAttrCMPR(cmpr.first) + cmpr.second.print();
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
				vars << key + showAttrCMPR(cmpr.first) + cmpr.second.print();
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
				vars << key + showAttrCMPR(cmpr.first) + cmpr.second.print();
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
DiceTriggerLimit& DiceTriggerLimit::parse(const AttrVar& var) {
	if (var.type == AttrVar::Type::U8String) {
		return parse(var.to_str());
	}
	else if (var.type == AttrVar::Type::Table) {
		new(this)DiceTriggerLimit();
		ShowList limits;
		ShowList notes;
		for (auto& [key, item] : **var.to_dict()) {
			if (key == "prob") {
				if (int prob{ item.to_int() }) {
					limits << "prob:" + to_string(prob);
					notes << "- 以概率触发: " + to_string(prob) + "%";
				}
			}
			else if (key == "user_id") {
				if (item.is_numberic()) {
					user_id = { item.to_ll() };
				}
				else if (!item.is_table())continue;
				AttrObject& tab{ item.table };
				if (tab->has("nor")) {
					user_id_negative = true;
					auto id{ tab->get("nor") };
					if (id.is_numberic()) {
						user_id = { item.to_ll() };
					}
					else if (id.to_list())for (auto& i : *id.to_list()) {
						user_id.emplace(i.to_ll());
					}
				}
				else for (auto& id : *tab->to_list()) {
					user_id.emplace(id.to_ll());
				}
				if (!user_id.empty()) {
					limits << (user_id_negative ? "user_id:!" : "user_id:") + listID(user_id);
					notes << (user_id_negative ? "- 不触发用户名单: " : "- 仅触发用户名单: ") + listID(user_id);
				}
			}
			else if (key == "grp_id") {
				if (item.is_numberic()) {
					grp_id = { item.to_ll() };
				}
				else if (!item.is_table())continue;
				AttrObject& tab{ item.table };
				if (tab->has("nor")) {
					grp_id_negative = true;
					auto id{ tab->get("nor") };
					if (id.is_numberic()) {
						grp_id = { item.to_ll() };
					}
					else if (id.to_list()) for (auto& i : *id.to_list()) {
						grp_id.emplace(i.to_ll());
					}
				}
				else if (tab->to_list())for (auto& id : *tab->to_list()) {
					grp_id.emplace(id.to_ll());
				}
				if (!grp_id.empty()) {
					limits << (grp_id_negative ? "grp_id:!" : "grp_id:") + listID(grp_id);
					notes << (grp_id_negative ? "- 不触发群聊名单: " : "- 仅触发群聊名单: ") + listID(grp_id);
				}
			}
			else if (key == "cd") {
				ShowList cds;
				ShowList cdnotes;
				string name;
				CDType type{ CDType::Chat };
				if (item.is_numberic()) {
					cd_timer.emplace_back(type, name, (time_t)item.to_ll());
				}
				else if (!item.is_table())continue;
				else {
					if (auto v{ item.to_list() }) {
						cd_timer.emplace_back(type, name, (time_t)v->begin()->to_ll());
					}
					for (auto& [subkey, value] : **item.to_dict()) {
						if (CDConfig::eType.count(subkey)) {
							type = (CDType)CDConfig::eType[subkey];
							if (value.is_numberic()) {
								cd_timer.emplace_back(type, name, (time_t)value.to_ll());
								continue;
							}
							if (auto v{ value.to_list() }) {
								cd_timer.emplace_back(type, name, (time_t)v->begin()->to_ll());
							}
							if (value.is_table())for (auto& [name, ct] : **value.to_dict()) {
								cd_timer.emplace_back(type, name, (time_t)ct.to_ll());
							}
							continue;
						}
						else if (subkey == "echo") {
							cd_notice = value;
							continue;
						}
						cd_timer.emplace_back(CDType::Chat, subkey, (time_t)value.to_ll());
					}
				}
				if (!cd_timer.empty()) {
					for (auto& it : cd_timer) {
						cds << it.key + ((it.key.empty() && it.type == CDType::Chat) ? ""
							: ("@" + CDConfig::eType[(size_t)it.type] + "="))
							+ to_string(it.cd);
						cdnotes << (it.type == CDType::Chat ? "窗口"
							: it.type == CDType::User ? "用户" : "全局") + it.key + "计" + to_string(it.cd) + "秒";
					}
					if (!cd_notice.empty()) {
						cds << "@echo=" + cd_notice;
						cdnotes << "冷却回复: " + cd_notice;
					}
					limits << "cd:" + cds.show("&");
					notes << "- 冷却计时: " + cdnotes.show("\n - ");
				}
			}
			else if (key == "today") {
				ShowList sub;
				ShowList subnotes;
				string name;
				CDType type{ CDType::Chat };
				if (item.is_numberic()) {
					today_cnt.emplace_back(type, name, (time_t)item.to_ll());
				}
				else if (!item.is_table())continue;
				else {
					if (auto v{ item.to_list() }) {
						today_cnt.emplace_back(type, name, (time_t)v->begin()->to_ll());
					}
					for (auto& [subkey, value] : **item.to_dict()) {
						if (CDConfig::eType.count(subkey)) {
							type = (CDType)CDConfig::eType[subkey];
							if (value.is_numberic()) {
								today_cnt.emplace_back(type, name, (time_t)value.to_ll());
								continue;
							}
							if (auto v{ value.to_list() }) {
								today_cnt.emplace_back(type, name, (time_t)v->begin()->to_ll());
							}
							if (value.is_table())for (auto& [name, ct] : **value.to_dict()) {
								today_cnt.emplace_back(type, name, (time_t)ct.to_ll());
							}
							continue;
						}
						today_cnt.emplace_back(CDType::Chat, subkey, (time_t)value.to_ll());
					}
				}
				if (!today_cnt.empty()) {
					for (auto& it : today_cnt) {
						sub << it.key + ((it.key.empty() && it.type == CDType::Chat) ? ""
							: ("@" + CDConfig::eType[(size_t)it.type] + "="))
							+ to_string(it.cd);
						subnotes << (it.type == CDType::Chat ? "窗口"
							: it.type == CDType::User ? "用户" : "全局") + it.key + "计" + to_string(it.cd) + "次";
					}
					if (!daylimit_notice.empty()) {
						sub << "@echo=" + daylimit_notice;
						subnotes << "限额回复: " + daylimit_notice;
					}
					limits << "today:" + sub.show("&");
					notes << "- 当日计数: " + subnotes.show("\n - ");
				}
			}
			else if (key == "lock") {
				ShowList sub;
				ShowList subnotes;
				string name;
				CDType type{ CDType::Global };
				if (item.is_boolean()) {
					if (item)locks.emplace_back(type, name, 1);
				}
				else if (item.is_character()) {
					locks.emplace_back(type, item.to_str(), 1);
				}
				else if (!item.is_table())continue;
				else {
					if (auto v{ item.to_list() }) {
						for (auto& k : *v) {
							locks.emplace_back(type, k.to_str(), 1);
						}
					}
					for (auto& [subkey, value] : **item.to_dict()) {
						if (CDConfig::eType.count(subkey)) {
							type = (CDType)CDConfig::eType[subkey];
							if (value.is_character()) {
								locks.emplace_back(type, value.to_str(), 1);
							}
							else if (auto v{ value.to_list() }) {
								for (auto& k : *v) {
									locks.emplace_back(type, k.to_str(), 1);
								}
							}
						}
					}
				}
				if (!locks.empty()) {
					for (auto& it : locks) {
						sub << it.key + ((it.key.empty() && it.type == CDType::Chat) ? ""
							: ("@" + CDConfig::eType[(size_t)it.type] + "="));
						subnotes << (it.type == CDType::Chat ? "窗口锁"
							: it.type == CDType::User ? "用户锁" : "全局锁") + it.key;
					}
					limits << "lock:" + sub.show("&");
					notes << "- 同步锁: " + subnotes.show();
				}
			}
			else if (key == "user_var") {
				if (!item.is_table())continue;
				if (string code{ parse_vary(**item.to_dict(), user_vary) }; !user_vary.empty()) {
					limits << "user_var:" + code;
					ShowList vars;
					for (auto& [key, cmpr] : user_vary) {
						vars << key + showAttrCMPR(cmpr.first) + cmpr.second.print();
					}
					notes << "- 用户触发阈值: " + vars.show();
				}
			}
			else if (key == "grp_var") {
				if (!item.is_table())continue;
				if (string code{ parse_vary(**item.to_dict(), grp_vary) }; !code.empty()) {
					limits << "grp_var:" + code;
					ShowList vars;
					for (auto& [key, cmpr] : grp_vary) {
						vars << key + showAttrCMPR(cmpr.first) + cmpr.second.print();
					}
					notes << "- 群聊触发阈值: " + vars.show();
				}
			}
			else if (key == "self_var") {
				if (!item.is_table())continue;
				if (string code{ parse_vary(**item.to_dict(), self_vary) }; !code.empty()) {
					limits << "self_var:" + code;
					ShowList vars;
					for (auto& [key, cmpr] : self_vary) {
						vars << key + showAttrCMPR(cmpr.first) + cmpr.second.print();
					}
					notes << "- 自身触发阈值: " + vars.show();
				}
			}
			else if (key == "dicemaid") {
				string val{ item.to_str() };
				if (Treat t{ LimitTreat[val] }; t != Treat::Ignore) {
					to_dice = t;
					limits << "dicemaid:" + LimitTreat[(size_t)t];
					notes << (to_dice == Treat::Only ? "- 识别Dice骰娘: 才触发" : "- 识别Dice骰娘: 不触发");
				}
			}
		}
		content = limits.show(";");
		comment = notes.show("\n");
	}
	return *this;
}
bool DiceTriggerLimit::check(DiceEvent* msg, chat_locks& lock_list)const {
	if (!user_id.empty() && (user_id_negative ^ !user_id.count(msg->fromChat.uid)))return false;
	if (!grp_id.empty() && (grp_id_negative ^ !grp_id.count(msg->fromChat.gid)))return false;
	if (prob && RandomGenerator::Randint(1, 100) > prob)return false;
	if (!locks.empty()) {
		chatInfo chat{ msg->fromChat.locate() };
		for (auto& conf : locks) {
			string key{ conf.key };
			if (key.empty())key = msg->get_str("reply_title");
			chatInfo ct{ conf.type == CDType::Chat ? chat :
				conf.type == CDType::User ? chatInfo(msg->fromChat.uid) : chatInfo()
			};
			lock_list.emplace_back(std::make_shared<std::unique_lock<std::mutex>>(sch.locker[ct][key]));
		}
	}
	if (!user_vary.empty()) {
		for (auto& [key, cmpr] : user_vary) {
			if (!(getUserItem(msg->fromChat.uid, key).*cmpr.first)(cmpr.second))return false;
		}
	}
	if (!grp_vary.empty() && msg->fromChat.gid) {
		for (auto& [key, cmpr] : grp_vary) {
			if (!(getGroupItem(msg->fromChat.gid, key).*cmpr.first)(cmpr.second))return false;
		}
	}
	if (!self_vary.empty()) {
		for (auto& [key, cmpr] : self_vary) {
			if (!(getSelfItem(key).*cmpr.first)(cmpr.second))return false;
		}
	}
	if (to_dice != Treat::Ignore && console.DiceMaid != msg->fromChat.uid) {
		if (DD::isDiceMaid(msg->fromChat.uid) != (to_dice == Treat::Only))return false;
	}
	//冷却与上限最后处理
	if (!cd_timer.empty() || !today_cnt.empty()) {
		vector<CDQuest>timers;
		vector<CDQuest>counters;
		chatInfo chat{ msg->fromChat.locate()};
		for (auto& conf : cd_timer) {
			string key{ conf.key };
			if (key.empty())key = msg->get_str("reply_title");
			chatInfo chattype{ conf.type == CDType::Chat ? chat :
				conf.type == CDType::User ? chatInfo(msg->fromChat.uid) : chatInfo()
			};
			timers.emplace_back(chattype, key, conf.cd);
		}
		for (auto& conf : today_cnt) {
			string key{ conf.key };
			if (key.empty())key = msg->get_str("reply_title");
			chatInfo chattype{ conf.type == CDType::Chat ? chat :
				conf.type == CDType::User ? chatInfo(msg->fromChat.uid) : chatInfo()
			};
			counters.emplace_back(chattype, key, conf.cd);
		}
		if (auto eno = sch.cnt_cd(timers, counters)){
			if (eno == -1) {
				if (!cd_notice.empty())msg->reply(cd_notice);
			}
			else if (eno == -2) {
				if (!daylimit_notice.empty())msg->reply(daylimit_notice);
			}
			return false;
		}
	}
	return true;
}

enumap_ci DiceMsgReply::sType{ "Nor","Order","Reply","Both","Game", };
enumap_ci DiceMsgReply::sMode{ "Match", "Prefix", "Search", "Regex" };
enumap_ci DiceMsgReply::sEcho{ "Text", "Deck", "Lua", "JS", "Py" };
std::array<string, 5> strType{ "无","指令","回复","同时","游戏" };
enumap<string> strMode{ "完全", "前缀", "模糊", "正则" };
enumap<string> strEcho{ "纯文本", "牌堆（多选一）", "Lua", "JavaScript",
#ifdef DICE_PYTHON
"Python" };
#else
"Python（不支持）" };
#endif // DICE_PYTHON

ptr<DiceMsgReply> DiceMsgReply::set_order(const string& key, const AttrVars& order) {
	auto reply{ std::make_shared<DiceMsgReply>() };
	reply->title = key;
	reply->type = DiceMsgReply::Type::Order;
	reply->keyMatch[1] = std::make_unique<vector<string>>(vector<string>{fmt->format(key)});
	reply->echo = DiceMsgReply::Echo::Lua;
	reply->answer = AnysTable(order);
	return reply;
}
bool DiceMsgReply::exec(DiceEvent* msg) {
	int chon{ msg->pGrp ? msg->pGrp->getChConf(msg->fromChat.chid,"order",0) : 0 };
	msg->set("reply_title", title);
	//limit
	chat_locks lock_list;
	if (!limit.check(msg, lock_list))return false;
	if (type == Type::Reply) {
		if (!msg->isCalled && (chon < 0 ||
			(!chon && (msg->pGrp->is("禁用回复")))))
			return false;
	}
	else {	//type == Type::Order
		if (!msg->isCalled && (chon < 0 ||
			(!chon && msg->pGrp->is("停用指令"))))
			return false;
	}
	if (msg->WordCensor()) {
		return true;
	}

	if (echo == Echo::Text) {
		msg->reply(answer->get_str("text"));
		return true;
	}
	else if (echo == Echo::Deck) {
		msg->reply(CardDeck::drawOne(answer->to_deck()));
		return true;
	}
	else if (echo == Echo::Lua) {
		lua_msg_call(msg, answer->get("lua"));
		return true;
	}
	else if (echo == Echo::JavaScript) {
		js_msg_call(msg, answer->get("js"));
		return true;
	}
#ifdef DICE_PYTHON
	else if (echo == Echo::Python && py) {
		if(py)py->call_reply(msg, answer->get("py"));
		return true;
	}
#endif //DICE_PYTHON
	return false;
}
string DiceMsgReply::show()const {
	return "\n触发性质: " + strType[(int)type]
		+ (limit.print().empty() ? "" : ("\n限制条件:\n" + limit.note()))
		+ "\n匹配模式: "
		+ (keyMatch[0] ? ("\n- 完全匹配: " + listDeck(*keyMatch[0])) : "")
		+ (keyMatch[1] ? ("\n- 前缀匹配: " + listDeck(*keyMatch[1])) : "")
		+ (keyMatch[2] ? ("\n- 模糊匹配: " + listDeck(*keyMatch[2])) : "")
		+ (keyMatch[3] ? ("\n- 正则匹配: " + listDeck(*keyMatch[3])) : "")
		+ "\n回复形式: " + strEcho[(int)echo]
		+ "\n回复内容: " + show_ans();
}
string DiceMsgReply::print()const {
	return (!title.empty() ? ("Title=" + title + "\n") : "")
		+ "Type=" + sType[(int)type]
		+ (limit.print().empty() ? "" : ("\nLimit=" + limit.print()))
		+ (keyMatch[0] ? ("\nMatch=" + listItem(*keyMatch[0])) : "")
		+ (keyMatch[1] ? ("\nPrefix=" + listItem(*keyMatch[1])) : "")
		+ (keyMatch[2] ? ("\nSearch=" + listItem(*keyMatch[2])) : "")
		+ (keyMatch[3] ? ("\nRegex=" + listItem(*keyMatch[3])) : "")
		+ "\n" + sEcho[(int)echo] + "=" + show_ans();
}
string DiceMsgReply::show_ans()const {
	switch (echo) {
	case DiceMsgReply::Echo::Text:
		return answer->get_str("text");
		break;
	case DiceMsgReply::Echo::Deck:
		return listDeck(*answer->to_list());
		break;
	case DiceMsgReply::Echo::Lua:
		return answer->get_str("lua");
		break;
	case DiceMsgReply::Echo::JavaScript:
		return answer->get_str("js");
		break;
	case DiceMsgReply::Echo::Python:
		return answer->get_str("py");
		break;
	}
	return {};
}

void DiceMsgReply::from_obj(AttrObject obj) {
	if (obj->is_table("keyword")) {
		for (auto& [match, word] : obj->get_obj("keyword")->as_dict()) {
			if (!sMode.count(match))continue;
			if (word.is_character()) {
				keyMatch[sMode[match]] = std::make_unique<vector<string>>(vector<string>{ word.to_str() });
			}
			else if (auto ary{ word.to_list() }) {
				vector<string> words;
				words.reserve(ary->size());
				for (auto& val : *ary) {
					words.push_back(val.to_str());
				}
				keyMatch[sMode[match]] = std::make_unique<vector<string>>(words);
			}
		}
	}
	if (obj->has("type"))type = (Type)sType[obj->get_str("type")];
	if (obj->has("limit"))limit.parse(obj->at("limit"));
	if (obj->has("echo")) {
		AttrVar& ans{ obj->at("echo") };
		if (ans.is_character()) {
			echo = Echo::Text;
			answer->set("text", ans);
		}
		else if (ans.is_function()) {
			echo = Echo::Lua;
			answer->set("lua", ans);
		}
		else if (auto tab{ ans.to_obj() }; tab->has("lua")) {
			echo = Echo::Lua;
			answer = *tab;
		}
		else if (tab->has("js")) {
			echo = Echo::JavaScript;
			answer = *tab;
		}
		else if (tab->has("py")) {
			echo = Echo::Python;
			answer = *tab;
		}
		else if (auto v{ answer->to_list() }) {
			auto li{ answer->new_list() };
			for (auto& item : *v) {
				li->push_back(item.to_str());
			}
		}
	}
}
void DiceMsgReply::readJson(const fifo_json& j) {
	try {
		if (j.count("type"))type = (Type)sType[j["type"].get<string>()];
		if (j.count("mode")) {
			size_t mode{ sMode[j["mode"].get<string>()] };
			string keyword{ j.count("keyword") ?
				j["keyword"] : title
			};
			keyMatch[mode] = std::make_unique<vector<string>>
				(mode == 3 ? vector<string>{keyword} : getLines(keyword, '|'));
		}
		if (j.count("match")) {
			keyMatch[0] = std::make_unique<vector<string>>(j["match"].get<vector<string>>());
		}
		if (j.count("prefix")) {
			keyMatch[1] = std::make_unique<vector<string>>(j["prefix"].get<vector<string>>());
		}
		if (j.count("search")) {
			keyMatch[2] = std::make_unique<vector<string>>(j["search"].get<vector<string>>());
		}
		if (j.count("regex")) {
			keyMatch[3] = std::make_unique<vector<string>>(j["regex"].get<vector<string>>());
		}
		if (!(keyMatch[0] || keyMatch[1] || keyMatch[2] || keyMatch[3])) {
			int idx{ 0 };
			keyMatch[0] = std::make_unique<vector<string>>(getLines(title, '|'));
		}
		if (j.count("limit"))limit.parse(j["limit"].get<string>());
		if (j.count("echo"))echo = (Echo)sEcho[j["echo"].get<string>()];
		if (j.count("answer")) {
			if (echo == Echo::Deck)answer->from_json(j["answer"]);
			else if (echo == Echo::Lua)answer->set("lua", j["answer"]);
			else if (echo == Echo::JavaScript)answer->set("js", j["answer"]);
			else if (echo == Echo::Python)answer->set("py", j["answer"]);
			else answer->set("text", j["answer"]);
		}
	}
	catch (std::exception& e) {
		console.log(string("reply解析json错误:") + e.what(), 0b1000);
	}
}
fifo_json DiceMsgReply::writeJson()const {
	fifo_json j;
	j["type"] = sType[(int)type];
	j["echo"] = sEcho[(int)echo];
	if (keyMatch[0])j["match"] = *keyMatch[0];
	if (keyMatch[1])j["prefix"] = *keyMatch[1];
	if (keyMatch[2])j["search"] = *keyMatch[2];
	if (keyMatch[3])j["regex"] = *keyMatch[3];
	if (!limit.empty())j["limit"] = limit.print();
	if (echo == Echo::Deck)j["answer"] = answer->to_json();
	else if (echo == Echo::Text)j["answer"] = answer->get_str("text");
	else if (echo == Echo::Lua)j["answer"] = answer->get_str("lua");
	else if (echo == Echo::JavaScript)j["answer"] = answer->get_str("js");
	else if (echo == Echo::Python)j["answer"] = answer->get_str("py");
	return j;
}
fifo_json DiceMsgReply::to_line()const {
	fifo_json j;
	j["name"] = title;
	j["keyword"] = keyMatch[0] ? listItem(*keyMatch[0]) :
		keyMatch[1] ? listItem(*keyMatch[1]) :
		keyMatch[2] ? listItem(*keyMatch[2]) :
		keyMatch[3] ? listItem(*keyMatch[3]) : "";
	j["type"] = sType[(int)type];
	j["mode"] = keyMatch[0] ? "Match" :
		keyMatch[1] ? "Prefix" :
		keyMatch[2] ? "Search" : "Regex";
	if (!limit.empty())j["limit"] = limit.print();
	j["echo"] = sEcho[(int)echo];
	if (echo == Echo::Deck) j["answer"] = listDeck(*answer->to_list());
	else if (echo == Echo::Text)j["answer"] = answer->get_str("text");
	else if (echo == Echo::Lua)j["answer"] = answer->get_str("lua");
	else if (echo == Echo::JavaScript)j["answer"] = answer->get_str("js");
	else if (echo == Echo::Python)j["answer"] = answer->get_str("py");
	return j;
}

bool DiceReplyUnit::listen(DiceEvent* msg, int type) {
	string& strMsg{ msg->strMsg };
	if (auto it{ match_items.find(strMsg) }; it != match_items.end()) {
		if (!items.count(it->second->title))
			match_items.erase(it);
		else if ((type & (int)it->second->type)
			&& it->second->exec(msg))return true;
	}
	if (stack<std::pair<size_t,string>> sPrefix; gPrefix.match_head(strMsg, sPrefix)) {
		while (!sPrefix.empty()) {
			msg->set("suffix", strMsg.substr(sPrefix.top().first));
			if (auto it{ prefix_items.find(sPrefix.top().second) }; it != prefix_items.end()
				&& (type & (int)it->second->type)
				&& (it->second->exec(msg)))return true;
			sPrefix.pop();
		}
	}
	//模糊匹配禁止自我触发
	if (vector<string>vSearch; msg->fromChat.uid != console.DiceMaid
		&& gSearcher.search(convert_a2w(strMsg.c_str()), vSearch)) {
		for (const auto& word : vSearch) {
			if (auto it{ search_items.find(word) }; it != search_items.end()
				&& (type & (int)it->second->type)
				&& it->second->exec(msg))return true;
		}
	}
	//regex
	try {
		bool isAns{ false };
		for (auto& [title, exp] : regex_exp) {
			if (!items.count(title))continue;
			auto reply{ items[title] };
			if (!(type & (int)reply->type))continue;
			// libstdc++ 使用了递归式 dfs 匹配正则表达式
			// 然而，每个 Java Thread 在32位 Linux 下默认大小为320K，600字符的匹配即会爆栈
			// 64位下还好，默认是1M，1800字符会爆栈
			// char limit 400
			// @seealso https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86164

			// 未来优化：预先构建regex并使用std::regex::optimize
			std::wstring LstrMsg = convert_a2realw(strMsg.c_str());
			if (strMsg.length() <= 400 && std::regex_match(LstrMsg, msg->msgMatch, exp)) {
				if (reply->exec(msg))isAns = true;
			}
		}
		return isAns;
	}
	catch (const std::regex_error& e)
	{
		msg->reply(e.what());
	}
	return 0;
}
void DiceReplyUnit::add(const string& key, ptr<DiceMsgReply> reply) {
	items[key] = reply;
}
void DiceReplyUnit::build() {
	for (auto& [title, reply] : items) {
		if (reply->keyMatch[0]) {
			for (auto& word : *reply->keyMatch[0]) {
				match_items[fmt->format(word)] = reply;
			}
		}
		if (reply->keyMatch[1]) {
			for (auto& word : *reply->keyMatch[1]) {
				prefix_items[word] = reply;
				gPrefix.add(fmt->format(word), word);
			}
		}
		if (reply->keyMatch[2]) {
			for (auto& word : *reply->keyMatch[2]) {
				search_items[word] = reply;
				gSearcher.add(convert_a2w(fmt->format(word).to_str().c_str()), word);
			}
		}
		if (reply->keyMatch[3]) {
			for (auto& word : *reply->keyMatch[3]) {
				try {
					std::wregex exp(convert_a2realw(word.c_str()),
						std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
					regex_exp.emplace(title, exp);
					regex_items[word] = reply;
				}
				catch (const std::regex_error& e) {
					console.log("正则关键词解析错误，表达式:\n" + word + "\n" + e.what(), 0b10);
				}
			}
		}
	}
	gPrefix.make_fail();
	gSearcher.make_fail();
}
void DiceReplyUnit::insert(const string& key, ptr<DiceMsgReply> reply) {
	erase(key);
	if (reply->keyMatch[0]) {
		for (auto& word : *reply->keyMatch[0]) {
			match_items[fmt->format(word)] = reply;
		}
	}
	if (reply->keyMatch[1]) {
		for (auto& word : *reply->keyMatch[1]) {
			for (auto& word : *reply->keyMatch[1]) {
				prefix_items[word] = reply;
				gPrefix.add(fmt->format(word), word);
			}
		}
		gPrefix.make_fail();
	}
	if (reply->keyMatch[2]) {
		for (auto& word : *reply->keyMatch[2]) {
			search_items[word] = reply;
			gSearcher.add(convert_a2w(fmt->format(word).to_str().c_str()), word);
		}
		gSearcher.make_fail();
	}
	if (reply->keyMatch[3]) {
		for (auto& word : *reply->keyMatch[3]) {
			try {
				std::wregex exp(convert_a2realw(word.c_str()),
					std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
				regex_exp.emplace(key, exp);
				regex_items[word] = reply;
			}
			catch (const std::regex_error& e) {
				console.log("正则关键词解析错误，表达式:\n" + word + "\n" + e.what(), 0b10);
			}
		}
	}
	items[key] = reply;
}
void DiceReplyUnit::erase(ptr<DiceMsgReply> reply) {
	items.erase(reply->title);
	if (reply->keyMatch[1]) {
		for (auto& word : *reply->keyMatch[1]) {
			prefix_items.erase(word);
		}
	}
	if (reply->keyMatch[2]) {
		for (auto& word : *reply->keyMatch[2]) {
			search_items.erase(word);
		}
	}
	if (reply->keyMatch[3]) {
		for (auto& word : *reply->keyMatch[3]) {
			regex_items.erase(word);
			regex_exp.erase(word);
		}
	}
}