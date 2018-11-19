#include "nameGetter.h"
#include<string>
#include<map>
#include"dbManager.h"
#include"CQAPI_EX.h"
#include"CQTools.h"
#include<sstream>
using namespace std;
using namespace CQ;

#define NICK_TABLE_NAME "nickname"
#define NICK_TABLE_DEFINE "create table " NICK_TABLE_NAME \
				"(qqid      int     NOT NULL," \
				"groupid    int     NOT NULL," \
				"name       text    NOT NULL," \
				"primary    key    (QQID,GROUPID));"

nameGetter * nameGetter::instance = nullptr;

nameGetter::nameGetter()noexcept
{
	dbManager *db = dbManager::getInstance();
	int retCode = db->registerTable(NICK_TABLE_NAME, NICK_TABLE_DEFINE);
	noSQL = retCode;
	nameCache = new map<pair<long long, long long>, string>();
	if (nameGetter::instance != nullptr)
	{
		delete (nameGetter::instance);
	}
	else
		nameGetter::instance = this;
}


nameGetter::~nameGetter()
{
	delete this->nameCache;
	if (nameGetter::instance == this)
	{
		nameGetter::instance = nullptr;
	}
}

std::string nameGetter::getNickName(const long long fromQQ, const long long fromGroup)
{
	sqlite3* db = dbManager::getDatabase();
	auto iter = nameCache->find(pair<long long,long long>(fromQQ,fromGroup));
	if (iter != nameCache->end())
	{
		return iter->second;
	}
	else
	{
		string name="pink fluffy unicorn";
		if (noSQL)
		{
			name = getDefaultName(fromQQ, fromGroup);
			nameCache->insert(std::pair<std::pair<long long, long long>, std::string>
				(std::pair<long long, long long>(fromQQ, fromGroup), name));
		}
		else
		{
			ostringstream selectCmd(std::ostringstream::ate);
			selectCmd << "SELECT * FROM " NICK_TABLE_NAME " where qqid =" << fromQQ << " and groupid =" << fromGroup;
			std::string nickEncoded;
			char * errMsg = nullptr;
			int ret = sqlite3_exec(db, selectCmd.str().c_str(), 
													&sqlite3_callback_query_name,
													(void*)&nickEncoded, 
													&errMsg);

			if (ret == SQLITE_OK) 
			{
				if (nickEncoded.length() > 0) 
				{
					name=base64_decode(nickEncoded);
					nameCache->insert(std::pair<std::pair<int64_t, int64_t>, std::string>
						(std::pair<int64_t, int64_t>(fromQQ, fromGroup), name));
				}
				else {
					name = getDefaultName(fromQQ, fromGroup);
					setNickName(name, fromQQ, fromGroup);
				}
			}
			else 
			{
				noSQL = true;
			}

		}
		return name;
	}
}

bool nameGetter::setNickName( std::string & nickname, const long long fromQQ, const long long fromGroup)
{
	sqlite3* db = dbManager::getDatabase();

	if (nickname=="")
	{
		nickname = getDefaultName(fromQQ, fromGroup);
	}
	else
	{
		strip(nickname);
	}

	auto iter = nameCache->find(pair<long long, long long>(fromQQ, fromGroup));
	//fresh cache
	if (iter != nameCache->end())
	{
		iter->second = nickname;
	}
	else
	{
		nameCache->insert(pair<pair<long long, long long>, string>(pair<long long, long long>(fromQQ, fromGroup), nickname));
	}

	//fresh db
	if (!noSQL) {
		string enCodeName = base64_encode(nickname);
		ostringstream selectCmd(std::ostringstream::ate);
		selectCmd << "SELECT * FROM " NICK_TABLE_NAME " where qqid =" << fromQQ << " and groupid =" << fromGroup;
	
		std::string str_nick_endcoded;
		char * pchar_err_message = nullptr;
		int ret_code = sqlite3_exec(db, selectCmd.str().c_str(), &sqlite3_callback_query_name, (void*)&str_nick_endcoded, &pchar_err_message);
		
		if (ret_code == SQLITE_OK) {
			if (str_nick_endcoded.length() > 0) {
				std::ostringstream updateCmd(std::ostringstream::ate);
				updateCmd.str("update " NICK_TABLE_NAME " set ");
				updateCmd << " name ='" << enCodeName << "'"
				<< " where qqid =" << fromQQ << " and groupid =" << fromGroup;
				int ret_code_2 = sqlite3_exec(db, updateCmd.str().c_str(), &dbManager::sqlite3_callback, (void*)&i_data_database_update, &pchar_err_message);
				if (!ret_code_2)
					return true;
				else
					return false;
			}
			else {
				std::ostringstream insertCmd(std::ostringstream::ate);
				insertCmd.str("insert into " NICK_TABLE_NAME " values ( ");
				insertCmd << fromQQ << ", " << fromGroup << ", '" << enCodeName << "'" << ");";
				int ret_code_2 = sqlite3_exec(db, insertCmd.str().c_str(), &dbManager::sqlite3_callback, (void*)&i_data_database_update, &pchar_err_message);
				if (!ret_code_2) 
					return true;
				else
					return false;
			}
		}
		else
		{
			noSQL = true;
			return true;
		}
	}
}

int nameGetter::sqlite3_callback_query_name(void * data, int argc, char ** argv, char ** azColName)
{
	if (argc == 3) {
		std::string * pstr_ret = (std::string *) data;
		*pstr_ret = std::string(argv[2]);
	}
	return SQLITE_OK;
}

string nameGetter::getDefaultName(const long long fromQQ, const long long fromGroup)
{
	string name;
	if (fromGroup)
	{
		if (fromGroup < 1000000000)
		{
			//group
			name = getGroupMemberInfo(fromGroup,fromQQ).GroupNick;
			if (name.empty())
				name = getStrangerInfo(fromQQ).nick;
			strip(name);
		}
		else
		{
			//discuss
			name = getStrangerInfo(fromQQ).nick;
			strip(name);
		}
	}
	else
	{
		//person
		name = getStrangerInfo(fromQQ).nick;
		strip(name);
	}

	return name;
}

string nameGetter::strip(string & origin)
{
	bool flag = true;
	while (flag)
	{
		flag = false;
		if (origin[0] == '!' || origin[0] == '.')
		{
			origin.erase(origin.begin());
			flag = true;
		}
		else if (origin.substr(0, 2) == "£¡" || origin.substr(0, 2) == "¡£")
		{
			origin.erase(origin.begin());
			origin.erase(origin.begin());
			flag = true;
		}
	}
	return origin;
}


