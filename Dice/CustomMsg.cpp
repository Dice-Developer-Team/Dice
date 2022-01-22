/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123���
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
	/*{"strJrrpCommandSuccessfullyEnabledNotice", "�ɹ��ڱ�Ⱥ/������������.jrrp����!"},
	{"strJrrpCommandAlreadyEnabledErr", "����: �ڱ�Ⱥ/��������.jrrp�����Ѿ�������!"},
	{"strJrrpCommandSuccessfullyDisabledNotice", "�ɹ��ڱ�Ⱥ/�������н���.jrrp����!"},
	{"strJrrpCommandAlreadyDisabledErr", "����: �ڱ�Ⱥ/��������.jrrp�����Ѿ�������!"},
	{"strJrrpCommandDisabledErr", "����Ա���ڴ�Ⱥ/�������н���.jrrp����!"},
	{"strHelpCommandSuccessfullyEnabledNotice", "�ɹ��ڱ�Ⱥ/������������.help����!"},
	{"strHelpCommandAlreadyEnabledErr", "����: �ڱ�Ⱥ/��������.help�����Ѿ�������!"},
	{"strHelpCommandSuccessfullyDisabledNotice", "�ɹ��ڱ�Ⱥ/�������н���.help����!"},
	{"strHelpCommandAlreadyDisabledErr", "����: �ڱ�Ⱥ/��������.help�����Ѿ�������!"},
	{"strHelpCommandDisabledErr", "����Ա���ڴ�Ⱥ/�������н���.help����!"},*/
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
