/*
*Dice! QQ dice robot for TRPG game
*Copyright (C) 2018 w4123ËÝä§
*
*This program is free software: you can redistribute it and/or modify it under the terms
*of the GNU Affero General Public License as published by the Free Software Foundation,
*either version 3 of the License, or (at your option) any later version.
*
*This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
*without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*See the GNU Affero General Public License for more details.
*
*You should have received a copy of the GNU Affero General Public License along with this
*program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef __NAMESTORAGE__
#define __NAMESTORAGE__
#include <map>
#include <string>
#include "StorageBase.h"
class NameStorage : public StorageBase
{
	std::map<long long, std::map<long long, std::string>> Name;
public:
	NameStorage(const std::string& FilePath);
	~NameStorage();

	void read();
	void save() const;

	std::string get(long long GroupID, long long QQ);
	bool set(long long GroupID, long long QQ, std::string name);
	void clear();
	bool del(long long GroupID, long long QQ);

};
#endif /*__NAMESTORAGE__*/
