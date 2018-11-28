/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018 w4123溯洄
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
#include <aws/core/Aws.h>
#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <CQLogger.h>
#include <Windows.h>
#include "GetRule.h"
#include "GlobalVar.h"
#include "EncodingConvert.h"
using namespace std;
using namespace Aws;
using namespace DynamoDB;



/*
 * 避免Windows.h中的GetMessage宏与Aws SDK中的GetMessage函数冲突
 */
#undef GetMessage

GetRule::GetRule()
{
	InitAPI(options);
	Client::ClientConfiguration config;
	config.region = Region::AP_SOUTHEAST_1;
	client = make_unique<DynamoDBClient>(Auth::AWSCredentials(ACCESS_KEY, SECRET_ACCESS_KEY), config);

	Model::GetItemRequest req;
	const Model::AttributeValue haskKey("Rules"), sortKey("List");
	req.AddKey("Type", haskKey).AddKey("Name", sortKey).SetTableName("DiceDB");
	string ErrMsg;
	for (auto i = 0; i != 3; ++i)
	{
		const Model::GetItemOutcome& result = client->GetItem(req);

		if (result.IsSuccess())
		{
			const auto& item = result.GetResult().GetItem();
			if (!item.empty() && item.count("RulesList"))
			{
				auto list = item.at("RulesList").GetSS();
				for (const auto& rule : list)
				{
					rules.push_back(rule);
				}
			}
			return;
		}
		ErrMsg = result.GetError().GetMessage();
		Sleep(200);
	}
	ShutdownAPI(options);
	failed = true;
	ErrMsg = "尝试连接规则服务器失败!Rules功能将不可用! 具体信息:" + ErrMsg;
	DiceLogger.Warning(ErrMsg);
}


GetRule::~GetRule()
{
	if (!failed)
		ShutdownAPI(options);
}

bool GetRule::analyze(string& rawStr, string& des)
{
	if (failed)
	{
		des = GlobalMsg["strRulesFailedErr"];
		return false;
	}
	for (auto& chr : rawStr)chr = toupper(static_cast<unsigned char> (chr));

	if (rawStr.find(':') != string::npos)
	{
		const string name = rawStr.substr(rawStr.find(':') + 1);
		if (name.empty())
		{
			des = GlobalMsg["strRulesFormatErr"];
			return false;
		}
		string rule = rawStr.substr(0, rawStr.find(':'));
		if (ruleNameReplace.count(rule))rule = ruleNameReplace.at(rule);
		return get(rule, name, des);
	}
	if (rawStr.empty())
	{
		des = GlobalMsg["strRulesFormatErr"];
		return false;
	}
	for (const auto& rule : rules)
	{
		if (get(rule, rawStr, des))
		{
			return true;
		}
	}
	return false;
}

bool GetRule::get(const std::string& rule, const std::string& name, std::string& des, bool isUTF8) const
{
	Model::GetItemRequest req;
	const string ruleName = isUTF8 ? rule : GBKtoUTF8(rule);
	const string itemName = isUTF8 ? name : GBKtoUTF8(name);
	const Model::AttributeValue haskKey("Rules-" + ruleName), sortKey(itemName);
	req.AddKey("Type", haskKey).AddKey("Name", sortKey).SetTableName("DiceDB");

	const Model::GetItemOutcome& result = client->GetItem(req);

	if (result.IsSuccess())
	{
		const auto& item = result.GetResult().GetItem();
		if (!item.empty())
		{
			if (item.count("Content"))
			{
				des = "规则: " + ruleName + "\n" + UTF8toGBK(item.at("Content").GetS());
				return true;
			}
			if (item.count("Redirect"))
			{
				return get(ruleName, item.at("Redirect").GetS(), des, true);
			}
			des = GlobalMsg["strRuleNotFound"];
			return false;
		}
		des = GlobalMsg["strRuleNotFound"];
		return false;
	}
	DiceLogger.Warning(("获取规则数据失败! 详细信息:\n" + result.GetError().GetMessage()).data());
	des = result.GetError().GetMessage();
	return false;
}
