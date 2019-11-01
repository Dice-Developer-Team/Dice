/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123溯洄
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
#include <fstream>
#include <sstream>
#include "json.hpp"
#include "GlobalVar.h"
#include "EncodingConvert.h"

std::map<std::string, std::string>strKeyReplace = {
	{"strWelcomeMsgIsEmptyErr","strWelcomeMsgClearErr"},
	{"strTurnNoticeReplyMsg", "strRollTurn"},
	{"strHiddenDiceReplyMsg", "strRollHidden"},
	{"strRollDiceReplyMsg", "strRollDice"},
	{"strObListReplyMsg", "strObList"},
	{"strObListEmptyNotice", "strObListEmpty"},
	{"strNickChangeReplyMsg", "strNameSet"},
	{"strNickDeleteReplyMsg", "strNameClr"},
	{"strObCommandSuccessfullyEnabledNotice", "strObOn"},
	{"strObCommandAlreadyEnabledErr","strObOnAlready"},
	{"strObCommandSuccessfullyDisabledNotice", "strObOff"},
	{"strObCommandAlreadyDisabledErr", "strObOffAlready"},
	{"strObCommandDisabledErr", "strObOffAlready"},
	{"strObListClearedNotice", "strObListClr"}
	/*{"strJrrpCommandSuccessfullyEnabledNotice", "成功在本群/讨论组中启用.jrrp命令!"},
	{"strJrrpCommandAlreadyEnabledErr", "错误: 在本群/讨论组中.jrrp命令已经被启用!"},
	{"strJrrpCommandSuccessfullyDisabledNotice", "成功在本群/讨论组中禁用.jrrp命令!"},
	{"strJrrpCommandAlreadyDisabledErr", "错误: 在本群/讨论组中.jrrp命令已经被禁用!"},
	{"strJrrpCommandDisabledErr", "管理员已在此群/讨论组中禁用.jrrp命令!"},
	{"strHelpCommandSuccessfullyEnabledNotice", "成功在本群/讨论组中启用.help命令!"},
	{"strHelpCommandAlreadyEnabledErr", "错误: 在本群/讨论组中.help命令已经被启用!"},
	{"strHelpCommandSuccessfullyDisabledNotice", "成功在本群/讨论组中禁用.help命令!"},
	{"strHelpCommandAlreadyDisabledErr", "错误: 在本群/讨论组中.help命令已经被禁用!"},
	{"strHelpCommandDisabledErr", "管理员已在此群/讨论组中禁用.help命令!"},*/
};

void ReadCustomMsg(std::ifstream& in)
{
	std::stringstream buffer;
	buffer << in.rdbuf();
	nlohmann::json customMsg = nlohmann::json::parse(buffer.str());
	for (nlohmann::json::iterator it = customMsg.begin(); it != customMsg.end(); ++it) {
		std::string strKey = it.key();
		if (strKeyReplace.count(strKey))strKey = strKeyReplace[strKey];
		if(GlobalMsg.count(strKey))
		{
			if (strKey != "strHlpMsg")
			{
				GlobalMsg[strKey] = UTF8toGBK(it.value().get<std::string>());
			}
			else
			{
				GlobalMsg[strKey] = Dice_Short_Ver + "\n" + UTF8toGBK(it.value().get<std::string>());
			}
			EditedMsg[strKey] = UTF8toGBK(it.value().get<std::string>());
		}
	}
}

void SaveCustomMsg(std::string strPath) {
	std::ofstream ofstreamCustomMsg(strPath);
	ofstreamCustomMsg << "{\n";
	for (auto it : EditedMsg)
	{
		while (it.second.find("\r\n") != std::string::npos)it.second.replace(it.second.find("\r\n"), 2, "\\n");
		while (it.second.find('\n') != std::string::npos)it.second.replace(it.second.find('\n'), 1, "\\n");
		while (it.second.find('\r') != std::string::npos)it.second.replace(it.second.find('\r'), 1, "\\r");
		while (it.second.find("\t") != std::string::npos)it.second.replace(it.second.find("\t"), 1, "\\t");
		int p = -1;
		while ((p = it.second.find("\"", ++p)) != std::string::npos)it.second.replace(p++, 1, "\\\"");
		std::string strMsg = GBKtoUTF8(it.second);
		ofstreamCustomMsg << "\"" << it.first << "\":\"" << strMsg << "\",\n";
	}
	ofstreamCustomMsg << "\"Number\":\"" << EditedMsg.size() << "\"\n}" << std::endl;
}
