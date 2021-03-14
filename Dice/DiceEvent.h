#pragma once

/*
 * 消息处理
 * Copyright (C) 2019 String.Empty
 */
#ifndef DICE_EVENT
#define DICE_EVENT
#include <map>
#include <set>
#include <utility>
#include <string>
#include "MsgMonitor.h"
#include "DiceSchedule.h"
#include "DiceMsgSend.h"
#include "GlobalVar.h"

using std::string;

//打包待处理消息
class FromMsg : public DiceJobDetail {
public:
	string strLowerMessage;
	long long fromGroup = 0;
	long long fromSession;
	Chat* pGrp = nullptr;
	string strReply;
	FromMsg(std::string message, long long qq) :DiceJobDetail(qq, { qq,msgtype::Private }, message){
		fromSession = ~fromQQ;
	}
	FromMsg(std::string message, long long fromGroup, msgtype msgType, long long qq) :DiceJobDetail(qq, { fromGroup,msgType }, message), fromGroup(fromGroup), fromSession(fromGroup){
		pGrp = &chat(fromGroup);
	}

	bool isBlock = false;

	void formatReply();

	void reply(const std::string& strReply, bool isFormat = true)override;

	FromMsg& initVar(const std::initializer_list<const std::string>& replace_str);
	void reply(const std::string& strReply, const std::initializer_list<const std::string>& replace_str);
	void replyHidden(const std::string& strReply);

	void reply(bool isFormat = true);

	void replyHidden();

	//通知
	void note(std::string strMsg, int note_lv = 0b1)
	{
		strMsg = format(strMsg, GlobalMsg, strVar);
		ofstream fout(DiceDir / "audit" / "log" / (to_string(console.DiceMaid) + "_" + printDate() + ".txt"),
		              ios::out | ios::app);
		fout << printSTNow() << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
		fout.close();
		reply(strMsg);
		const string note = getName(fromQQ) + strMsg;
		for (const auto& [ct,level] : console.NoticeList) 
		{
			if (!(level & note_lv) || pair(fromQQ, msgtype::Private) == ct || ct == fromChat)continue;
			AddMsgToQueue(note, ct);
		}
	}

	//打印消息来源
	std::string printFrom()
	{
		std::string strFwd;
		if (fromChat.second == msgtype::Group)strFwd += "[群:" + to_string(fromGroup) + "]";
		if (fromChat.second == msgtype::Discuss)strFwd += "[讨论组:" + to_string(fromGroup) + "]";
		strFwd += getName(fromQQ, fromGroup) + "(" + to_string(fromQQ) + "):";
		return strFwd;
	}

	//转发消息
	void fwdMsg();
	int AdminEvent(const string& strOption);
	int MasterSet();
	int BasicOrder();
	int InnerOrder();
	//int CustomOrder();
	int CustomReply();
	//判断是否响应
	bool DiceFilter();
	bool WordCensor();
	void operator()();
	short trusted = 0;

private:
	bool isVirtual = false;
	//是否响应
	bool isAns = false;
	bool isDisabled = false;
	bool isCalled = false;
	bool isAuth = false;

	int getGroupAuth(long long group = 0);
public:
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
			&& intMsgCnt != strLowerMessage.length()) {
			strPara += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		return strPara;
	}

	//读取数字
	string readDigit(bool isForce = true)
	{
		string strMum;
		if (isForce)while (intMsgCnt < strMsg.length() && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
		{
			if (strMsg[intMsgCnt] < 0)intMsgCnt++;
			intMsgCnt++;
		}
		else while(intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt < strMsg.length() && isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
		{
			strMum += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		if (intMsgCnt < strMsg.length() && strMsg[intMsgCnt] == ']')intMsgCnt++;
		return strMum;
	}

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
			if (inBracket)
			{
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
	string readAttrName()
	{
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		const int intBegin = intMsgCnt;
		int intEnd = intBegin;
		const unsigned int len = strMsg.length();
		while (intMsgCnt < len && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))
			&& strMsg[intMsgCnt] != '=' && strMsg[intMsgCnt] != ':'
			&& strMsg[intMsgCnt] != '+' && strMsg[intMsgCnt] != '-' && strMsg[intMsgCnt] != '*' && strMsg[intMsgCnt] !=
			'/')
		{
			if (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || (!isspace(
				static_cast<unsigned char>(strMsg[intEnd]))))intEnd = intMsgCnt;
			if (strMsg[intMsgCnt] < 0)intMsgCnt += 2;
			else intMsgCnt++;
		}
		if (intMsgCnt == strLowerMessage.length() && strLowerMessage.find(' ', intBegin) != string::npos)
		{
			intMsgCnt = strLowerMessage.find(' ', intBegin);
		}
		else if (isspace(static_cast<unsigned char>(strMsg[intEnd])))intMsgCnt = intEnd;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}

	//
	int readChat(chatType& ct, bool isReroll = false);

	int readClock(Console::Clock& cc)
	{
		const string strHour = readDigit();
		if (strHour.empty())return -1;
		const unsigned short nHour = stoi(strHour);
		if (nHour > 23)return -2;
		cc.first = nHour;
		if (strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '.')intMsgCnt++;
		if (strMsg.substr(intMsgCnt, 2) == "：")intMsgCnt += 2;
		readSkipSpace();
		if (intMsgCnt >= strMsg.length() || !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
		{
			cc.second = 0;
			return 0;
		}
		const string strMin = readDigit();
		const unsigned short nMin = stoi(strMin);
		if (nMin > 59)return -2;
		cc.second = nMin;
		return 0;
	}

	//读取分项
	string readItem()
	{
		string strMum;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == '|')intMsgCnt++;
		while (strMsg[intMsgCnt] != '|' && intMsgCnt != strMsg.length())
		{
			strMum += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		return strMum;
	}
	void readItems(vector<string>&);
};

#endif /*DICE_EVENT*/
