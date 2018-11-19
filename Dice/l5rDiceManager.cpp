#include "l5rDiceManager.h"
#include"dbManager.h"
#include<string>
#include<sstream>
#include"GlobalVar.h"
using namespace std;

#define TABLENAME "LFRDICE"
#define TABLEDEFINE "create table " TABLENAME \
				"(qqid      int     NOT NULL," \
				"groupid   int     NOT NULL," \
				"source     text    NOT NULL," \
				"primary    key    (QQID,GROUPID));"

l5rDiceManager* l5rDiceManager::instance = nullptr;

l5rDiceManager::l5rDiceManager()
{
	dbManager *dbCtrl = dbManager::getInstance();
	int ret = dbCtrl->registerTable(TABLENAME, TABLEDEFINE);
	noSQL = ret != SQLITE_OK;
	

	if (this->instance != nullptr)
		delete this->instance;
	else
		this->instance = this;

}

l5rDiceManager::~l5rDiceManager()
{
	if (instance == this)
	{
		delete this;
		instance = nullptr;
	}
}

string l5rDiceManager::dice(const string& name,const std::string & command, long long fromQQ, long long fromGroup)
{
	string reply;
	if (command[0] == 'r')
	{
		//reroll
		reply =reroll(command.substr(1), fromQQ, fromGroup);
		if (reply == "1")
			reply = GlobalMsg["strLFRDontExist"];
		else if (reply == "2")
			reply = GlobalMsg["strDBFailToOpen"];
		else
			reply = name + reply;
	}
	//else if (command[0] == 'k')
	//{
	//	//keep
	//	reply = keep(command.substr(1), fromQQ, fromGroup);
	//}
	else if (command[0] == 'd')
	{
		//drop
		reply = drop(command.substr(1), fromQQ, fromGroup);
		if (reply == "1")
			reply = GlobalMsg["strLFRDontExist"];
		else if (reply == "2")
			reply = GlobalMsg["strDBFailToOpen"];
		else
			reply = name + reply;
	}
	else if (command[0] == 'a')
	{

		//if (command[1] == 's')
		//{	//add specific
		//	reply = addSp(command.substr(2), fromQQ, fromGroup);
		//}
		//else
		//{
			//add
			reply =add(command.substr(1), fromQQ, fromGroup);
			if (reply == "1")
				reply = GlobalMsg["strLFRDontExist"];
			else if (reply == "2")
				reply = GlobalMsg["strDBFailToOpen"];
			else
				reply = name + reply;
		//}
	}
	else
	{
		//creat
		reply = create(command.substr(0), fromQQ, fromGroup);
		if (reply == "1")
			reply = GlobalMsg["strDiceTooBigErr"];
		else if (reply == "2")
			reply = GlobalMsg["strValueErr"];
		else
			reply = name + reply;
	}
	
	return reply;
}

int l5rDiceManager::sqlite3_callback_query_lfrdice(void * data, int argc, char ** argv, char ** azColName)
{
	if (argc == 3) {
		std::string * pstr_ret = (std::string *) data;
		*pstr_ret = std::string(argv[2]);
	}
	return SQLITE_OK;
}

string l5rDiceManager::create(const std::string & command, long long fromQQ, long long fromGroup)
{
	auto key = l5rDKey(fromQQ, fromGroup);
	auto iter = diceMap.find(key);
	l5rDice newDice;
	stringstream ss;
	ss.str(command);
	string notation;
	string reason;
	string reply;
	ss >> notation;
	ss >> reason;

	//generate dice
	int diceCnt = 0;
	char charBuf;
	ss.clear();
	ss.str(notation);

	while (ss >> charBuf)
	{
		if (isdigit(charBuf))
		{
			ss.putback(charBuf);
			ss >> diceCnt;
		}
		else if (charBuf == 'b')
		{
			if (diceCnt)
			{
				newDice.insertB(diceCnt);
				diceCnt = 0;
			}
			else
			{
				newDice.insertB(1);
			}
		}
		else if (charBuf == 'w')
		{
			if (diceCnt)
			{
				newDice.insertW(diceCnt);
				diceCnt = 0;
			}
			else
			{
				newDice.insertW(1);
			}
		}
	}

	if (newDice.getDiceCnt() > MAXSIZE){reply =="1"; return reply;}
	else if (newDice.getDiceCnt() == 0) {reply =="2"; return reply;}

	if (!reason.empty())
		reply += "由于" + reason;
	reply += "投掷" + notation + '=';
	reply += newDice.roll();

	if (iter != diceMap.end())
	{
		// fresh cache
		iter->second = newDice;
	}
	else
	{
		// insert cache
		diceMap.insert(l5rDPair(key, newDice));
	}

	//fresh db
	if (!noSQL)
	{

		auto *db = dbManager::getDatabase();
		string encoded = newDice.encodeToDB();
		ostringstream selectCommand(ostringstream::ate);
		selectCommand << "SELECT * FROM " TABLENAME " where qqid =" << fromQQ << " and groupid =" << fromGroup;
		string readFromDB;
		char* errMsg = nullptr;
		int ret= sqlite3_exec(db, selectCommand.str().c_str(), &sqlite3_callback_query_lfrdice, (void*)&readFromDB, &errMsg);

		if (ret == SQLITE_OK)
		{

			if (readFromDB.length() > 0)
			{
				ostringstream updateCmd(ostringstream::ate);
				updateCmd.str("update " TABLENAME " set ");
				updateCmd << " source ='" << encoded << "'" << " where qqid =" << fromQQ << " and groupid =" << fromGroup;
				int ret2 = sqlite3_exec(db, updateCmd.str().c_str(), &dbManager::sqlite3_callback, (void*)&readFromDB, &errMsg);
	
			}
			else
			{
				ostringstream insertCmd(ostringstream::ate);
				insertCmd.str("insert into " TABLENAME " values ( ");
				insertCmd  << fromQQ << ","<< fromGroup << ",'"<< encoded << "');";
				int ret2 = sqlite3_exec(db, insertCmd.str().c_str(), &dbManager::sqlite3_callback, (void*)&readFromDB, &errMsg);
			}
		}
		else
			noSQL = true;
	}

	return reply;
}

std::string l5rDiceManager::reroll(const std::string & command, long long fromQQ, long long fromGroup)
{
	stringstream ss;
	auto key = l5rDKey(fromQQ, fromGroup);
	ss.str(command);
	int NO;
	auto iter = diceMap.find(key);
	string reply="重骰结果=";

	if (iter != diceMap.end())
	{
		//fresh cache
		while (ss >> NO)
			iter->second.reRoll(NO);
		reply += iter->second.getResult();

		ss.clear();
		string reason;
		ss >> reason;

		if (!reason.empty())
		{
			reply = "由于" + reason + reply;
		}

		//fresh db
		if (!noSQL)
		{
			sqlite3* db = dbManager::getDatabase();
			char* errMsg;
			ostringstream updateCmd(ostringstream::ate);
			updateCmd.str("update " TABLENAME " set ");
			updateCmd <<" source ='" << iter->second.encodeToDB() << "'"
				<< " where qqid =" << fromQQ << " and groupid =" << fromGroup;
			int ret= sqlite3_exec(db, updateCmd.str().c_str(), &dbManager::sqlite3_callback, (void*)&i_data_database_update, &errMsg);
		}
	}
	else
	{
		if (!noSQL)
		{
			//数据库中查找
			sqlite3* db = dbManager::getDatabase();
			ostringstream selectCmd(std::ostringstream::ate);
			selectCmd << "SELECT * FROM " TABLENAME " where qqid =" << fromQQ << " and groupid =" << fromGroup;
			string strRead;
			char* errMsg;
			int ret = sqlite3_exec(db, selectCmd.str().c_str(), &sqlite3_callback_query_lfrdice, (void*)&strRead, &errMsg);
			if (ret == SQLITE_OK)
			{
				if (strRead.length()>0)
				{
					l5rDice newDice;
					newDice.decode(strRead);
					while (ss >> NO)
						newDice.reRoll(NO);

					reply += newDice.getResult();
					ss.clear();
					string reason;
					ss >> reason;

					if (!reason.empty())
					{
						reply = "由于" + reason + reply;
					}
					diceMap.insert(l5rDPair(l5rDKey(fromQQ, fromGroup), newDice));
				}
				else
				{
					reply = "1";//不存在数据
				}
			}
			else
			{
				noSQL = true;
				reply = "2";//数据库打开失败
			}
		}
		else
			reply = "1";
	}

	return reply;
}

std::string l5rDiceManager::drop(const std::string & command, long long fromQQ, long long fromGroup)
{
	int NO;
	stringstream ss;
	l5rDKey key(fromQQ, fromGroup);
	ss.str(command);
	auto iter = diceMap.find(key);
	string reply = "的删除结果=";

	if (iter != diceMap.end())
	{
		//fresh cache
		while (ss >> NO)
			iter->second.drop(NO);

		reply += iter->second.getResult();

		ss.clear();
		string reason;
		ss >> reason;

		if (!reason.empty())
		{
			reply = "由于" + reason + reply;
		}

		//fresh db
		if (!noSQL)
		{
			sqlite3* db = dbManager::getDatabase();
			char* errMsg;
			ostringstream updateCmd(ostringstream::ate);
			updateCmd.str("update " TABLENAME " set ");
			updateCmd << " source ='" << iter->second.encodeToDB() << "'"
				<< " where qqid =" << fromQQ << " and groupid =" << fromGroup;
			int ret = sqlite3_exec(db, updateCmd.str().c_str(), &dbManager::sqlite3_callback, (void*)&i_data_database_update, &errMsg);
		}
	}
	else
	{
		if (!noSQL)
		{
			//数据库中查找
			sqlite3* db = dbManager::getDatabase();
			ostringstream selectCmd(std::ostringstream::ate);
			selectCmd << "SELECT * FROM " TABLENAME " where qqid =" << fromQQ << " and groupid =" << fromGroup;
			string strRead;
			char* errMsg;
			int ret = sqlite3_exec(db, selectCmd.str().c_str(), &sqlite3_callback_query_lfrdice, (void*)&strRead, &errMsg);
			if (ret == SQLITE_OK)
			{
				if (strRead.length()>0)
				{
					l5rDice newDice;
					newDice.decode(strRead);
					while (ss >> NO)
						newDice.drop(NO);

					reply += newDice.getResult();
					ss.clear();
					string reason;
					ss >> reason;

					if (!reason.empty())
					{
						reply = "由于" + reason + reply;
					}
					diceMap.insert(l5rDPair(l5rDKey(fromQQ, fromGroup), newDice));
				}
				else
				{
					reply = "1";//不存在数据
				}
			}
			else
			{
				noSQL = true;
				reply = "2";//数据库打开失败
			}
		}
		else
			reply = "1";
	}

	return reply;
}

std::string l5rDiceManager::add(const std::string & command, long long fromQQ, long long fromGroup)
{

	stringstream ss;
	l5rDKey key(fromQQ, fromGroup);
	ss.str(command);
	auto iter = diceMap.find(key);
	string reply = "的增加结果=";

	string notation;
	string reason;

	ss >> notation;
	ss >> reason;
	
	ss.clear();
	ss.str(notation);

	char charBuf;
	int diceCnt = 0;

		if (iter != diceMap.end())
		{
			while (ss >> charBuf)
			{
				if (isdigit(charBuf))
				{
					ss.putback(charBuf);
					ss >> diceCnt;
				}
				else if (charBuf == 'b')
				{
					if (diceCnt)
					{
						iter->second.insertB(diceCnt,true);
						diceCnt = 0;
					}
					else
					{
						iter->second.insertB(1,true);
					}
				}
				else if (charBuf == 'w')
				{
					if (diceCnt)
					{
						iter->second.insertW(diceCnt,true);
						diceCnt = 0;
					}
					else
					{
						iter->second.insertW(1,true);
					}
				}
			}
			reply += iter->second.getResult();
		if (!reason.empty())
		{
			reply = "由于" + reason + reply;
		}

		//fresh db
		if (!noSQL)
		{
			sqlite3* db = dbManager::getDatabase();
			char* errMsg;
			ostringstream updateCmd(ostringstream::ate);
			updateCmd.str("update " TABLENAME " set ");
			updateCmd << " source ='" << iter->second.encodeToDB() << "'"
				<< " where qqid =" << fromQQ << " and groupid =" << fromGroup;
			int ret = sqlite3_exec(db, updateCmd.str().c_str(), &dbManager::sqlite3_callback, (void*)&i_data_database_update, &errMsg);
		}
	}
	else
	{
		if (!noSQL)
		{
			//数据库中查找
			sqlite3* db = dbManager::getDatabase();
			ostringstream selectCmd(std::ostringstream::ate);
			selectCmd << "SELECT * FROM " TABLENAME " where qqid =" << fromQQ << " and groupid =" << fromGroup;
			string strRead;
			char* errMsg;
			int ret = sqlite3_exec(db, selectCmd.str().c_str(), &sqlite3_callback_query_lfrdice, (void*)&strRead, &errMsg);
			if (ret == SQLITE_OK)
			{
				if (strRead.length()>0)
				{
					l5rDice newDice;
					newDice.decode(strRead);

					while (ss >> charBuf)
					{
						if (isdigit(charBuf))
						{
							ss.putback(charBuf);
							ss >> diceCnt;
						}
						else if (charBuf == 'b')
						{
							if (diceCnt)
							{
								newDice.insertB(diceCnt,true);
								diceCnt = 0;
							}
							else
							{
								newDice.insertB(1,true);
							}
						}
						else if (charBuf == 'w')
						{
							if (diceCnt)
							{
								newDice.insertW(diceCnt,true);
								diceCnt = 0;
							}
							else
							{
								newDice.insertW(1,true);
							}
						}
					}
					reply += newDice.getResult();

					if (!reason.empty())
					{
						reply = "由于" + reason + reply;
					}

					diceMap.insert(l5rDPair(l5rDKey(fromQQ, fromGroup), newDice));
				}
				else
				{
					reply = "1";//不存在数据
				}
			}
			else
			{
				noSQL = true;
				reply = "2";//数据库打开失败
			}
		}
		else
			reply = "1";
	}

	return reply;
}

//std::string l5rDiceManager::addSp(const std::string & command, long long fromQQ, long long fromGroup)
//{
//	return "addSp";
//}

//std::string l5rDiceManager::keep(const std::string & command, long long fromQQ, long long fromGroup)
//{
//	return "keep";
//}
