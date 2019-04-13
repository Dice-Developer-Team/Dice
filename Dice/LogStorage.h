/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 ÐÓÑÀ
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
#pragma once
#ifndef DICE_LOG_STORAGE
#define DICE_LOG_STORAGE
#include <map>
#include <string>
#include <list>
#include "StorageBase.h"

class LogStorage : public StorageBase
{
	// [user,group,theme,log]
	std::map<long long, std::map<long long, std::map<std::string, std::list<std::string>>>> Log;
	// [group,theme]
	std::map<long long, std::string> LogFlag;
	std::map<long long, long long> LogRecorder;
public:
	LogStorage(const std::string& FilePath);
	~LogStorage();
	
	void read() override;
	void save() const override;

	std::string get(long long QQ, long long GroupID, std::string name);
	std::string get(long long QQ, long long GroupID);
	void record(long long GroupID, std::string nickName, std::string logItem);
	void set(long long QQ, long long GroupID, std::string name, std::string logItem);
	void set(long long GroupID, std::string name);
	void set(long long GroupID, long long QQ);
	void clear();
	bool del(long long QQ, long long GroupID, std::string name);
	bool logOn(long long QQ, long long GroupID, std::string name);
	bool logOff(long long GroupID);
	bool haveAuthority(long long QQ, long long GroupID);
};
#endif /*DICE_LOG_STORAGE*/
