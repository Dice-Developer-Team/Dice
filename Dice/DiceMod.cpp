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
 * Copyright (C) 2019-2021 String.Empty
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
using std::set;

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
		else if (auto it = dict.find(key); it != dict.end()){
			val = format(it->second, dict, mod_name);
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
	if ((*job)["help_word"].empty()) {
		job->reply(string(Dice_Short_Ver) + "\n" + GlobalMsg["strHlpMsg"]);
		return;
	}
	else if (const auto it = helpdoc.find((*job)["help_word"]); it != helpdoc.end()) {
		job->reply(format(it->second, helpdoc));
	}
	else if (unordered_set<string> keys = querier.search((*job)["help_word"]);!keys.empty()) {
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
			job->reply("{strHelpSuggestion}");
		}
	}
	else job->reply("{strHelpNotFound}");
	cntHelp[(*job)["help_word"]] += 1;
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
	vector<std::filesystem::path> sModFile;
	//读取mod
	vector<string> sModErr;
	int cntFile = listDir(DiceDir / "mod" , sModFile);
	int cntItem{0};
	if (cntFile > 0) {
		for (auto& pathFile : sModFile) {
			nlohmann::json j = freadJson(pathFile);
			if (j.is_null()) {
				sModErr.push_back(pathFile.filename().string());
				continue;
			}
			if (j.count("dice_build")) {
				if (j["dice_build"] > Dice_Build) {
					sModErr.push_back(pathFile.filename().string() + "(Dice版本过低)");
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
		string fileLua = pathFile.string();
		if (fileLua.rfind(".lua") != fileLua.length() - 4) {
			sLuaErr.push_back(pathFile.filename().string());
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
			sLuaErr.push_back(pathFile.filename().string());
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
				 && *sLuaErr.rbegin() != pathFile.filename().string()) {
			sLuaErr.push_back(pathFile.filename().string());
		}
	}
	*resLog << "读取/plugin/中的" + std::to_string(cntLuaFile) + "个脚本, 共" + std::to_string(cntOrder) + "个指令";
	if (!sLuaErr.empty()) {
		*resLog << "读取失败" + std::to_string(sLuaErr.size()) + "个:";
		for (auto& it : sLuaErr) {
			*resLog << it;
		}
	}
	std::thread factory(&DiceModManager::init,this);
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
	gOrder.build(msgorder);
	isIniting = false;
}
void DiceModManager::clear()
{
	helpdoc.clear();
	msgorder.clear();
	taskcall.clear();
}
