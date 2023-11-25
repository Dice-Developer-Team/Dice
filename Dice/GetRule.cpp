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
 * Copyright (C) 2019-2023 String.Empty
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
#include <string>
#include <cstring>
#include "OneBotAPI.h"
#include "DiceRule.h"
#include "GetRule.h"
#include "GlobalVar.h"
#include "EncodingConvert.h"
#include "DiceNetwork.h"


using namespace std;

namespace GetRule
{
	bool analyze(string& rawStr, string& des)
	{
		if (rawStr.empty())
		{
			des = getMsg("strRulesFormatErr");
			return false;
		}

		for (auto& chr : rawStr)chr = toupper(static_cast<unsigned char>(chr));

		if (rawStr.find(':') != string::npos)
		{
			const string name = rawStr.substr(rawStr.find(':') + 1);
			if (name.empty())
			{
				des = getMsg("strRulesFormatErr");
				return false;
			}
			const string rule = rawStr.substr(0, rawStr.find(':'));
			if (name.empty())
			{
				des = getMsg("strRulesFormatErr");
				return false;
			}
			return get(rule, name, des);
		}
		return get("", rawStr, des);
	}

	bool get(const std::string& rule, const std::string& name, std::string& des){
		if (rule.empty()) {
			if (auto entry{ ruleset->getManual(name) }) {
				des = *entry;
				return true;
			}
		}
		else if (auto rulebook{ ruleset->get_rule(rule) }) {
			if (auto entry{ rulebook->getManual(name) }) {
				des = *entry;
				return true;
			}
		}
		else if (auto entry{ ruleset->getManual(name) }) {
			des = *entry;
			return true;
		}
		const string ruleName = rule;
		const string itemName = name;

		string data = "Name=" + UrlEncode(itemName) + "&QQ=" + to_string(console.DiceMaid) + "&v=20190114";
		if (!ruleName.empty())
		{
			data += "&Type=Rules-" + UrlEncode(ruleName);
		}
		char* frmdata = new char[data.length() + 1];
#ifdef _MSC_VER
		strcpy_s(frmdata, data.length() + 1, data.c_str());
#else
		strcpy(frmdata, data.c_str());
#endif
		string temp;
		const bool reqRes = Network::POST("http://api.kokona.tech:5555/rules", frmdata, "", temp);
		delete[] frmdata;
		if (reqRes)
		{
			des = temp;
			return true;
		}
		if (temp == getMsg("strRequestNoResponse"))
		{
			des = getMsg("strRuleNotFound");
		}
		else
		{
			des = temp;
		}
		return false;
	}
}
