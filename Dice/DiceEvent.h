#pragma once

/*
 * ��Ϣ����
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
//�����������Ϣ
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

	//֪ͨ
	void note(std::string strMsg, int note_lv = 0b1);

	//��ӡ��Ϣ��Դ
	std::string printFrom();

	//ת����Ϣ
	void fwdMsg();
	void logEcho();
	int AdminEvent(const string& strOption);
	int MasterSet();
	int BasicOrder();
	int InnerOrder();
	bool monitorFrq();
	//�ж��Ƿ���Ӧ
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
	//�����ո�
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

	//��ȡ���ǿո�հ׷�
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

	//��ȡ����(ͳһСд)
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

	//��ȡ����
	string readDigit(bool isForce = true);

	//��ȡ���ֲ���������
	int readNum(int&);
	//��ȡȺ��
	long long readID()
	{
		const string strGroup = readDigit();
		if (strGroup.empty() || strGroup.length() > 18) return 0;
		return stoll(strGroup);
	}

	//�Ƿ�ɿ����������ʽ
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

	//��ȡ�������ʽ
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

	//��ȡ��ת��ı��ʽ
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

	//��ȡ��ð�Ż�Ⱥ�ֹͣ���ı�
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

	//��ȡ��Сд�����еļ�����
	string readAttrName();
	string readFileName();
	//
	int readChat(chatInfo& ct, bool isReroll = false);

	int readClock(Clock& cc);

	//��ȡ����
	string readItem();
	int readItems(vector<string>&);
};
void reply(const AttrObject&, string, bool isFormat = true);
void MsgNote(const AttrObject&, string, int);

#endif /*DICE_EVENT*/
