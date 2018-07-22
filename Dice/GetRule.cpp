#include "GetRule.h"
#include <string>
#include <aws/core/Aws.h>
#include <aws/core/utils/Outcome.h> 
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <CQLogger.h>
#include <Windows.h>
#include <fstream>
#include "RDConstant.h"
extern CQ::logger logger;
using namespace std;
using namespace Aws;
using namespace DynamoDB;

std::string GetRule::GBKtoUTF8(const std::string & strGBK)
{
	int len = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, nullptr, 0);
	wchar_t * str1 = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, len);
	len = WideCharToMultiByte(CP_UTF8, 0, str1, -1, nullptr, 0, nullptr, nullptr);
	char * str2 = new char[len + 1];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, len, nullptr, nullptr);
	string strOutUTF8(str2);
	delete[] str1;
	delete[] str2;
	return strOutUTF8;
}

std::string GetRule::UTF8toGBK(const std::string & strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, nullptr, 0);
	wchar_t * wszGBK = new wchar_t[len + 1];
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, nullptr, 0, nullptr, nullptr);
	char *szGBK = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, nullptr, nullptr);
	string strTemp(szGBK);
	delete[] szGBK;
	delete[] wszGBK;
	return strTemp;
}

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
	logger.Warning(ErrMsg);

}


GetRule::~GetRule()
{
	if(!failed)
		ShutdownAPI(options);
}

bool GetRule::analyse(string & rawstr, string& des)
{
	if (failed)
	{
		des = strRulesFailedErr;
		return false;
	}
	for (auto &chr : rawstr)chr = toupper(chr);

	if (rawstr.find(':')!=string::npos)
	{
		const string name = rawstr.substr(rawstr.find(':') + 1);
		if(name.empty())
		{
			des = strRulesFormatErr;
			return false;
		}
		string rule = rawstr.substr(0, rawstr.find(':'));
		if (ruleNameReplace.count(rule))rule = ruleNameReplace.at(rule);
		return get(rule, name, des);

	}
	if (rawstr.empty())
	{
		des = strRulesFormatErr;
		return false;
	}
	for (const auto& rule:rules)
	{
		if (get(rule, rawstr, des))
		{
			return true;
		}
	}
	return false;
}

bool GetRule::get(const std::string & rule, const std::string & name, std::string & des,bool isUTF8) const 
{
	Model::GetItemRequest req;
	const string ruleName = isUTF8 ? rule : GBKtoUTF8(rule);
	const string itemName = isUTF8 ? name : GBKtoUTF8(name);
	const Model::AttributeValue haskKey("Rules-" + ruleName),sortKey(itemName);
	req.AddKey("Type", haskKey).AddKey("Name", sortKey).SetTableName("DiceDB");

	const Model::GetItemOutcome& result = client->GetItem(req);
	
	if(result.IsSuccess())
	{
		const auto& item = result.GetResult().GetItem();
		if (!item.empty())
		{
			if (item.count("Content"))
			{
				des = UTF8toGBK(item.at("Content").GetS());
				return true;	
			}
			if (item.count("Redirect"))
			{
				return get(ruleName, item.at("Redirect").GetS(), des, true);
			}
			des = strRuleNotFound;
			return false;
			
		}
		des = strRuleNotFound;
		return false;
	}
	logger.Warning(("获取规则数据失败! 详细信息:\n" + result.GetError().GetMessage()).data());
	des = result.GetError().GetMessage();
	return false;
}
