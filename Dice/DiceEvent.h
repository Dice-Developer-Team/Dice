/*
 * 消息处理
 * Copyright (C) 2019 String.Empty
 */
#ifndef DICE_EVENT
#define DICE_EVENT
#include <map>
#include <set>
#include <queue>
#include "CQAPI_EX.h"
#include "MsgMonitor.h"
#include "DiceMsgSend.h"

using std::string;
//打包待处理消息
class FromMsg {
public:
	std::string strMsg;
	string strLowerMessage;
	long long fromID = 0;
	CQ::msgtype fromType = CQ::msgtype::Private;
	long long fromQQ = 0;
	long long fromGroup = 0;
	Chat* pGrp = nullptr;
	chatType fromChat;
	time_t fromTime = time(NULL);
	string strReply;
	//临时变量库
	map<string, string> strVar = {};
	FromMsg(std::string message, long long fromNum) :strMsg(message), fromID(fromNum), fromQQ(fromNum) {
		fromChat = { fromID,CQ::msgtype::Private };
	}
	FromMsg(std::string message, long long fromGroup, CQ::msgtype msgType, long long fromNum) :strMsg(message), fromID(fromGroup), fromType(msgType), fromQQ(fromNum), fromGroup(fromGroup), fromChat({ fromGroup,fromType }) {
		pGrp = &chat(fromGroup);
	}
	bool isBlock = false;
	void reply(std::string strReply, bool isFormat) {
		isAns = true;
		if (isFormat)
			AddMsgToQueue(format(strReply, GlobalMsg, strVar), fromID, fromType); 
		else AddMsgToQueue(strReply, fromID, fromType);
	}
	void reply(std::string strReply, const std::initializer_list<const std::string> replace_str = {}, bool isFormat = true) {
		isAns = true;
		if (!isFormat) {
			AddMsgToQueue(strReply, fromID, fromType);
			return;
		}
		int index = 0;
		for (auto s : replace_str) {
			strVar[to_string(index++)] = s;
		}
		AddMsgToQueue(format(strReply, GlobalMsg, strVar), fromID, fromType);
	}
	void reply() {
		reply(strReply);
	}
	//通知
	void note(std::string strMsg, int note_lv = 0b1) {
		strMsg = format(strMsg, GlobalMsg, strVar);
		ofstream fout(string(DiceDir + "\\audit\\log") + to_string(console.DiceMaid) + "_" + printDate() + ".txt", ios::out | ios::app);
		fout << printSTNow() << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
		fout.close();
		reply(strMsg);
		string note = getName(fromQQ) + strMsg;
		for (const auto &[ct,level] : console.NoticeList) {
			if (!(level & note_lv) || pair(fromQQ, CQ::msgtype::Private) == ct || ct == fromChat)continue;
			AddMsgToQueue(note, ct);
		}
	}
	//打印消息来源
	std::string printFrom() {
		std::string strFwd;
		if (fromType == CQ::msgtype::Group)strFwd += "[群:" + to_string(fromGroup) + "]";
		if (fromType == CQ::msgtype::Discuss)strFwd += "[讨论组:" + to_string(fromGroup) + "]";
		strFwd += getName(fromQQ, fromGroup) + "(" + to_string(fromQQ) + "):";
		return strFwd;
	}
	//转发消息
	void FwdMsg(string message);
	int AdminEvent(string strOption);
	int MasterSet();
	int DiceReply();
	int CustomReply();
	//判断是否响应
	bool DiceFilter();
	short trusted = 0;

private:
	//是否响应
	bool isAns = false;
	unsigned int intMsgCnt = 0;
	bool isBotOff = false;
	bool isCalled = false;
	bool isAuth = false;
	bool isLinkOrder = false;
	short getGroupAuth(long long group = 0) {
		if (trusted > 0)return trusted;
		if (ChatList.count(group)) {
			int per = CQ::getGroupMemberInfo(group, fromQQ).permissions;
			if (per > 1)return 0;
			if (per)return -1;
		}
		return -2;
	}
	//跳过空格
	void readSkipSpace() {
		while (intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
	}
	void readSkipColon() {
		readSkipSpace();
		while (intMsgCnt < strMsg.length() && strMsg[intMsgCnt] == ':')intMsgCnt++;
	}
	string readUntilSpace() {
		string strPara;
		readSkipSpace(); 
		while (intMsgCnt < strMsg.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strPara += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		return strPara;
	}
	//读取至非空格空白符
	string readUntilTab() {
		while (intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		int intBegin = intMsgCnt;
		int intEnd = intBegin;
		unsigned int len = strMsg.length();
		while (intMsgCnt < len && (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == ' '))
		{
			if (strMsg[intMsgCnt] != ' ' || strMsg[intEnd] != ' ')intEnd = intMsgCnt;
			if (strMsg[intMsgCnt] < 0 && intMsgCnt < len)intMsgCnt += 2;
			else intMsgCnt++;
		}
		if (strMsg[intEnd] == ' ')intMsgCnt = intEnd;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}
	string readRest() {
		readSkipSpace();
		return strMsg.substr(intMsgCnt);
	}
	//读取参数(统一小写)
	string readPara() {
		string strPara;
		while (intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt < strMsg.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
			&& (strLowerMessage[intMsgCnt] != '-') && (strLowerMessage[intMsgCnt] != '+') && (strLowerMessage[intMsgCnt] != '[') && (strLowerMessage[intMsgCnt] != ']') && (strLowerMessage[intMsgCnt] != '=') && (strLowerMessage[intMsgCnt] != ':')
			&& intMsgCnt != strLowerMessage.length()) {
			strPara += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		return strPara;
	}
	//读取数字
	string readDigit(bool isForce = true) {
		string strMum;
		if (isForce)while (intMsgCnt < strMsg.length() && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
			if (strMsg[intMsgCnt] < 0)intMsgCnt++;
			intMsgCnt++;
		}
		else while(intMsgCnt < strMsg.length() && isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		while (intMsgCnt < strMsg.length() && isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
			strMum += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		if (intMsgCnt < strMsg.length() && strMsg[intMsgCnt] == ']')intMsgCnt++;
		return strMum;
	}
	//读取数字并存入整型
	int readNum(int&);
	//读取群号
	long long readID() {
		string strGroup = readDigit();
		if (strGroup.empty() || strGroup.length() > 18) return 0;
		return stoll(strGroup);
	}
	//是否可看做掷骰表达式
	bool isRollDice() {
		readSkipSpace();
		if (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
			|| strLowerMessage[intMsgCnt] == 'd' || strLowerMessage[intMsgCnt] == 'k'
			|| strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b'
			|| strLowerMessage[intMsgCnt] == 'f'
			|| strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-'
			|| strLowerMessage[intMsgCnt] == 'a'
			|| strLowerMessage[intMsgCnt] == 'x' || strLowerMessage[intMsgCnt] == '*') {
			return true;
		}
		else return false;
	}
	//读取掷骰表达式
	string readDice(){
		string strDice;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
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
	string readExp() {
		bool inBracket = false;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		int intBegin = intMsgCnt;
		while (intMsgCnt != strMsg.length()) {
			if (inBracket) {
				if (strMsg[intMsgCnt] == ']')inBracket = false;
				intMsgCnt++;
				continue;
			}
			else if (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
				|| strLowerMessage[intMsgCnt] == 'd' || strLowerMessage[intMsgCnt] == 'k'
				|| strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b'
				|| strLowerMessage[intMsgCnt] == 'f'
				|| strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-'
				|| strLowerMessage[intMsgCnt] == 'a'
				|| strLowerMessage[intMsgCnt] == 'x' || strLowerMessage[intMsgCnt] == '*' || strLowerMessage[intMsgCnt] == '/') {
				intMsgCnt++;
			}
			else if (strMsg[intMsgCnt] == '[') {
				inBracket = true;
				intMsgCnt++;
			}
			else break;
		}
		while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt - 1]))) intMsgCnt--;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}
	//读取到冒号或等号停止的文本
	string readToColon() {
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		int intBegin = intMsgCnt;
		int intEnd = intBegin;
		unsigned int len = strMsg.length();
		while (intMsgCnt < len && strMsg[intMsgCnt] != '=' && strMsg[intMsgCnt] != ':')		{
			if (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || (!isspace(static_cast<unsigned char>(strMsg[intEnd]))))intEnd = intMsgCnt;
			if (strMsg[intMsgCnt] < 0)intMsgCnt += 2;
			else intMsgCnt++;
		}
		if (isspace(static_cast<unsigned char>(strMsg[intEnd])))intMsgCnt = intEnd;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}
	//读取大小写不敏感的技能名
	string readAttrName() {
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
		int intBegin = intMsgCnt;
		int intEnd = intBegin;
		unsigned int len = strMsg.length();
		while (intMsgCnt < len && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))
			&& strMsg[intMsgCnt] != '=' && strMsg[intMsgCnt] != ':'
			&& strMsg[intMsgCnt] != '+' && strMsg[intMsgCnt] != '-' && strMsg[intMsgCnt] != '*' && strMsg[intMsgCnt] != '/')
		{
			if (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || (!isspace(static_cast<unsigned char>(strMsg[intEnd]))))intEnd = intMsgCnt;
			if (strMsg[intMsgCnt] < 0)intMsgCnt += 2;
			else intMsgCnt++;
		}
		if (intMsgCnt == strLowerMessage.length() && strLowerMessage.find(' ', intBegin) != string::npos) {
			intMsgCnt = strLowerMessage.find(' ', intBegin);
		}
		else if (isspace(static_cast<unsigned char>(strMsg[intEnd])))intMsgCnt = intEnd;
		return strMsg.substr(intBegin, intMsgCnt - intBegin);
	}
	//
	int readChat(chatType& ct, bool isReroll = false);
	int readClock(Console::Clock& cc) {
		string strHour = readDigit();
		if (strHour.empty())return -1;
		unsigned short nHour = stoi(strHour);
		if (nHour > 23)return -2;
		cc.first = nHour;
		if (strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '.')intMsgCnt++;
		if (strMsg.substr(intMsgCnt, 2) == "：")intMsgCnt += 2;
		readSkipSpace();
		if (intMsgCnt >= strMsg.length() || !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
			cc.second = 0;
			return 0;
		}
		string strMin = readDigit();
		unsigned short nMin = stoi(strMin);
		if (nHour > 59)return -2;
		cc.second = nMin;
		return 0;
	}
	//读取分项
	string readItem() {
		string strMum;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == '|')intMsgCnt++;
		while (strMsg[intMsgCnt] != '|'&& intMsgCnt != strMsg.length()) {
			strMum += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		return strMum;
	}
};

#endif /*DICE_EVENT*/