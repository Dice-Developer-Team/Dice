#pragma once
#include<map>
#include<string>

class nameGetter
{
public:
	nameGetter() noexcept;
	~nameGetter();
	static nameGetter * instance;
	std::string strip(std::string& origin);
	std::string getNickName(const long long fromQQ, const long long fromGroup = 0);
	bool setNickName(std::string& nickname, const long long fromQQ, const long long fromGroup = 0);
private:
	std::map<std::pair<long long, long long>, std::string> * nameCache;
	bool noSQL = false;
	static int sqlite3_callback_query_name(void * data, int argc, char ** argv, char ** azColName);
	std::string getDefaultName(const long long fromQQ, const long long fromGroup = 0);
};
