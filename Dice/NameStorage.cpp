/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123ËÝä§
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
#include "NameStorage.h"
#include <string>
#include <utility>
#include <fstream>
#include "CQTools.h"
using namespace std;

void NameStorage::read()
{
	ifstream ifName(FilePath);
	if (ifName)
	{
		long long GroupID = 0, QQ = 0;
		string name;
		while (ifName >> GroupID >> QQ >> name)
		{
			set(GroupID, QQ, base64_decode(name));
		}
	}
}

bool NameStorage::set(long long GroupID, long long QQ, string name)
{
	Name[GroupID][QQ] = name;
	return true;
}

string NameStorage::get(long long GroupID, long long QQ)
{
	//Î´ÉèÖÃ·µ»ØÎª¿Õ×Ö·û´®
	if (Name.count(GroupID) && Name[GroupID].count(QQ))
		return Name[GroupID][QQ];
	return "";
}

void NameStorage::save() const
{
	ofstream ofName(FilePath, ios::out | ios::trunc);
	for (const auto& it1 : Name)
	{
		for (const auto& it2 : it1.second)
		{
			ofName << it1.first << " " << it2.first << " " << base64_encode(it2.second) << endl;
		}
	}
}

void NameStorage::clear()
{
	Name.clear();
}

bool NameStorage::del(long long GroupID, long long QQ)
{
	if (Name.count(GroupID) && Name[GroupID].count(QQ))
	{
		Name[GroupID].erase(QQ);
		if (Name[GroupID].empty())
		{
			Name.erase(GroupID);
		}
		return true;
	}
	return false;
}

NameStorage::NameStorage(const string& FilePath) : StorageBase(FilePath)
{
	NameStorage::read();
}

NameStorage::~NameStorage()
{
	NameStorage::save();
}
