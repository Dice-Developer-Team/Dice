#pragma once
#include"sqlite3/sqlite3.h"
#include<string>

const int i_data_database_create = 4;
const int i_data_database_update = 8;

class dbManager
{
private:
	static dbManager * instance;
	bool isDatabaseReady;
	sqlite3 * database;
public:
	static sqlite3 * getDatabase();
	static dbManager * getInstance();
	dbManager(const std::string&);
	~dbManager();
	int registerTable(std::string str_table_name, std::string str_table_sql);
	int isTableExist(const std::string & table_name, bool &isExist);
	static int sqlite3_callback(void * data, int argc, char ** argv, char ** azColName);
	static int sqlite3_callback_isTableExist(void * data, int argc, char ** argv, char ** azColName);
};