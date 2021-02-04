/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123ËÝä§
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
#include "DDAPI.h"
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
			des = GlobalMsg["strRulesFormatErr"];
			return false;
		}

		for (auto& chr : rawStr)chr = toupper(static_cast<unsigned char>(chr));

		if (rawStr.find(':') != string::npos)
		{
			const string name = rawStr.substr(rawStr.find(':') + 1);
			if (name.empty())
			{
				des = GlobalMsg["strRulesFormatErr"];
				return false;
			}
			const string rule = rawStr.substr(0, rawStr.find(':'));
			if (name.empty())
			{
				des = GlobalMsg["strRulesFormatErr"];
				return false;
			}
			return get(rule, name, des);
		}
		return get("", rawStr, des);
	}

	bool get(const std::string& rule, const std::string& name, std::string& des)
	{
		const string ruleName = GBKtoUTF8(rule);
		const string itemName = GBKtoUTF8(name);

		string data = "Name=" + UrlEncode(itemName) + "&QQ=" + to_string(DD::getLoginQQ()) + "&v=20190114";
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
		const bool reqRes = Network::POST("api.kokona.tech", "/rules", 5555, frmdata, temp);
		delete[] frmdata;
		if (reqRes)
		{
			des = UTF8toGBK(temp);
			return true;
		}
		if (temp == GlobalMsg["strRequestNoResponse"])
		{
			des = GlobalMsg["strRuleNotFound"];
		}
		else
		{
			des = temp;
		}
		return false;
	}
}
