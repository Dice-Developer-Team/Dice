#pragma once

/*
 * 消息处理
 * Copyright (C) 2018-2021 w4123
 * Copyright (C) 2019-2024 String.Empty
 */
#ifndef DICE_EVENT
#define DICE_EVENT
#include <map>
#include <set>
#include <utility>
#include <string>
#include <regex>
#include "DiceMsgSend.h"
#include "ManagerSystem.h"

using std::string;
class RD;

class DiceSession;
//打包待处理消息
class DiceEvent : public AnysTable {
public:
	MetaType getType()const override { return MetaType::Context; }
	chatInfo fromChat;
	string strLowerMessage;
	Chat* pGrp = nullptr;
	ptr<DiceSession> thisGame();
	string& strMsg;
	string strReply;
	std::wsmatch msgMatch;
	DiceEvent(const AttrVars& var, const chatInfo& ct);
	DiceEvent(const AnysTable& var);

	bool isPrivate()const;
	bool isChannel()const;
	bool isFromMaster()const;

	void formatReply();

	void reply(const std::string& strReply, bool isFormat = true) ;

	void replyHidden(const std::string& strReply);

	void reply(bool isFormat = true);
	void replyMsg(const std::string& key);
	void replyHelp(const std::string& key);
	void replyRollDiceErr(int, const RD&);
	void replyHidden();

	//通知
	void note(std::string strMsg, int note_lv = 0b1);

	//打印消息来源
	std::string printFrom();

	//转发消息
	void fwdMsg();
	void logEcho();
	int AdminEvent(const string& strOption);
	int MasterSet();
	int BasicOrder();
	int InnerOrder();
	bool monitorFrq();
	//判断是否响应
	bool DiceFilter();
	bool WordCensor();
	void virtualCall();
	short trusted = 0;
	bool isCalled = false;

private:
	bool isDisabled = false;
	std::optional<string> getGameRule();
	bool canRoomHost();

	int getGroupTrust(long long group = 0);
public:
	bool isVirtual = false;
	unsigned int intMsgCnt = 0;
	//跳过空格
	void readSkipSpace()
	{
		while (intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
	}

	void readSkipColon();

	string readUntilSpace()
	{
		string strPara;
		readSkipSpace(); 
		while (intMsgCnt < strMsg.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		{
			strPara += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		return strPara;
	}

	//读取至非空格空白符
	string readUntilTab()
	{
		while (intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		const int intBegin = intMsgCnt;
		int intEnd = intBegin;
		const unsigned int len = strMsg.length();
		while (intMsgCnt < len && (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == ' '))
		{
			if (strMsg[intMsgCnt] != ' ' || strMsg[intEnd] != ' ')intEnd = intMsgCnt;
			if (intMsgCnt < len && strMsg[intMsgCnt] < 0)intMsgCnt += 2;
			else intMsgCnt++;
		}
		if (strMsg[intEnd] == ' ')intMsgCnt = intEnd;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}

	string readRest()
	{
		readSkipSpace();
		return strMsg.substr(intMsgCnt);
	}

	//读取参数(统一小写)
	string readPara()
	{
		string strPara;
		while (intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt < strMsg.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
			&& (strLowerMessage[intMsgCnt] != '-') && (strLowerMessage[intMsgCnt] != '+') 
			&& (strLowerMessage[intMsgCnt] != '[') && (strLowerMessage[intMsgCnt] != ']') 
			&& (strLowerMessage[intMsgCnt] != '=') && (strLowerMessage[intMsgCnt] != ':')
			&& (strLowerMessage[intMsgCnt] != '#')
			&& intMsgCnt != strLowerMessage.length()) {
			strPara += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		return strPara;
	}

	//读取数字
	string readDigit(bool isForce = true);

	//读取数字并存入整型
	int readNum(int&);
	//读取群号
	long long readID()
	{
		const string strGroup = readDigit();
		if (strGroup.empty() || strGroup.length() > 18) return 0;
		return stoll(strGroup);
	}

	//是否可看做掷骰表达式
	bool isRollDice()
	{
		readSkipSpace();
		if (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
			|| strLowerMessage[intMsgCnt] == 'd' || strLowerMessage[intMsgCnt] == 'k'
			|| strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b'
			|| strLowerMessage[intMsgCnt] == 'f'
			|| strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-'
			|| strLowerMessage[intMsgCnt] == 'a'
			|| strLowerMessage[intMsgCnt] == 'x' || strLowerMessage[intMsgCnt] == '*')
		{
			return true;
		}
		return false;
	}

	//读取掷骰表达式
	string readDice()
	{
		string strDice;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' ||
			strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
			|| strLowerMessage[intMsgCnt] == 'd' || strLowerMessage[intMsgCnt] == 'k'
			|| strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b'
			|| strLowerMessage[intMsgCnt] == 'f'
			|| strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-'
			|| strLowerMessage[intMsgCnt] == 'a'
			|| strLowerMessage[intMsgCnt] == 'x' || strLowerMessage[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/'
			|| strLowerMessage[intMsgCnt] == '#')
		{
			strDice += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		return strDice;
	}

	//读取含转义的表达式
	string readExp()
	{
		bool inBracket = false;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' ||
			strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		const int intBegin = intMsgCnt;
		while (intMsgCnt != strMsg.length())
		{
			if (inBracket){
				if (strMsg[intMsgCnt] == ']')inBracket = false;
				intMsgCnt++;
				continue;
			}
			if (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
				|| strLowerMessage[intMsgCnt] == 'd' || strLowerMessage[intMsgCnt] == 'k'
				|| strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b'
				|| strLowerMessage[intMsgCnt] == 'f'
				|| strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-'
				|| strLowerMessage[intMsgCnt] == 'a'
				|| strLowerMessage[intMsgCnt] == 'x' || strLowerMessage[intMsgCnt] == '*' || strLowerMessage[intMsgCnt]
				== '/')
			{
				intMsgCnt++;
			}
			else if (strMsg[intMsgCnt] == '[')
			{
				inBracket = true;
				intMsgCnt++;
			}
			else break;
		}
		while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && isalpha(
			static_cast<unsigned char>(strLowerMessage[intMsgCnt - 1]))) intMsgCnt--;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}

	//读取到冒号或等号停止的文本
	string readToColon()
	{
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		const int intBegin = intMsgCnt;
		int intEnd = intBegin;
		const unsigned int len = strMsg.length();
		while (intMsgCnt < len && strMsg[intMsgCnt] != '=' && strMsg[intMsgCnt] != ':')
		{
			if (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || (!isspace(
				static_cast<unsigned char>(strMsg[intEnd]))))intEnd = intMsgCnt;
			if (strMsg[intMsgCnt] < 0)intMsgCnt += 2;
			else intMsgCnt++;
		}
		if (isspace(static_cast<unsigned char>(strMsg[intEnd])))intMsgCnt = intEnd;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}

	//读取大小写不敏感的技能名
	string readAttrName();
	string readFileName();
	//
	int readChat(chatInfo& ct, bool isReroll = false);

	int readClock(Clock& cc);

	//读取分项
	string readItem();
	int readItems(vector<string>&);
};
void reply(const AttrObject&, string, bool isFormat = true);
void MsgNote(const AttrObject&, string, int);

#endif /*DICE_EVENT*/
