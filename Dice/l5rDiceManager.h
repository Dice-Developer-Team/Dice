#pragma once
#include<map>
#include<string>
#include"l5rDice.h"


class l5rDiceManager
{
	using l5rDKey = std::pair<long long, long long>;//qq,group
	using l5rDMap = std::map<l5rDKey, l5rDice>;
	using l5rDPair = std::pair<l5rDKey, l5rDice>;
private:
	bool noSQL;
	l5rDMap diceMap;
	static l5rDiceManager * instance;
	const int MAXSIZE = 20;
public:
	l5rDiceManager();
	~l5rDiceManager();
	std::string dice(const std::string&name,const std::string& command, long long fromQQ, long long fromGroup = 0);
private:
	static int sqlite3_callback_query_lfrdice(void * data, int argc, char ** argv, char ** azColName);
	std::string create(const std::string& command, long long fromQQ, long long fromGroup = 0);
	std::string reroll(const std::string& command, long long fromQQ, long long fromGroup = 0);
	std::string drop(const std::string& command, long long fromQQ, long long fromGroup = 0);
	std::string add(const std::string& command, long long fromQQ, long long fromGroup = 0);
	//std::string addSp(const std::string& command, long long fromQQ, long long fromGroup = 0);
	//std::string keep(const std::string& command, long long fromQQ, long long fromGroup = 0);
	
};

