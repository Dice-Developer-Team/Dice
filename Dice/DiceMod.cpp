#include <set>
#include "DiceMod.h"
#include "GlobalVar.h"
#include "ManagerSystem.h"
#include "Jsonio.h"
#include "DiceFile.hpp"
using std::set;

DiceModManager::DiceModManager() : helpdoc(HelpDoc)
{
}

string DiceModManager::format(string s, const map<string, string, less_ci>& dict, const char* mod_name = "") const
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
		auto it = dict.find(key);
		if (it != dict.end())
		{
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

void DiceModManager::_help(DiceJobDetail* job) {
	if ((*job)["help_word"].empty()) {
		job->reply(string(Dice_Short_Ver) + "\n" + GlobalMsg["strHlpMsg"]);
		return;
	}
	else if (const auto it = helpdoc.find((*job)["help_word"]); it != helpdoc.end()) {
		job->reply(format(it->second, helpdoc));
	}
	else if (unordered_set<string> keys = querier.search((*job)["help_word"]);!keys.empty()) {
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
	else job->reply("{strHelpNotFound}");
	cntHelp[(*job)["help_word"]] += 1;
	saveJMap(DiceDir + "\\user\\HelpStatic.json",cntHelp);
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

int DiceModManager::load(string& strLog) 
{
	vector<std::filesystem::path> sFile;
	vector<string> sFileErr;
	int cntFile = listDir(DiceDir + "\\mod\\", sFile, true);
	int cntItem{0};
	if (cntFile <= 0)return cntFile;
	for (auto& filename : sFile) 
	{
		nlohmann::json j = freadJson(filename);
		if (j.is_null())
		{
			sFileErr.push_back(filename.filename().string());
			continue;
		}
		if (j.count("dice_build")) {
			if (j["dice_build"] > Dice_Build) {
				sFileErr.push_back(filename.filename().string()+"(Dice版本过低)");
				continue;
			}
		}
		if (j.count("helpdoc"))
		{
			cntItem += readJMap(j["helpdoc"], helpdoc);
		}
		if (j.count("global_char"))
		{
			cntItem += readJMap(j["global_char"], GlobalChar);
		}
	}
	strLog += "读取" + DiceDir + "\\mod\\中的" + std::to_string(cntFile) + "个文件, 共" + std::to_string(cntItem) + "个条目\n";
	if (!sFileErr.empty())
	{
		strLog += "读取失败" + std::to_string(sFileErr.size()) + "个:\n";
		for (auto& it : sFileErr)
		{
			strLog += it + "\n";
		}
	}
	std::thread factory(&DiceModManager::init,this);
	factory.detach();
	cntHelp.reserve(helpdoc.size());
	loadJMap(DiceDir + "\\user\\HelpStatic.json", cntHelp);
	return cntFile;
}
void DiceModManager::init() {
	isIniting = true;
	for (auto& [key, word] : helpdoc) {
		querier.insert(key);
	}
	isIniting = false;
}
void DiceModManager::clear()
{
	helpdoc.clear();
}
