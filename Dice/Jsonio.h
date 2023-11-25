#pragma once

// Json信息获取以及写入

#include <fstream>
#include <map>
#include <vector>
#include <set>
#include "filesystem.hpp"
#include "EncodingConvert.h"
#include "fifo_json.hpp"

template <typename T>
std::enable_if_t<!std::is_arithmetic_v<T>, T> readJKey(const std::string& strJson)
{
	return strJson;
}

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, T> readJKey(const std::string& strJson)
{
	return stoll(strJson);
}

fifo_json freadJson(const std::filesystem::path& path);
void fwriteJson(const std::filesystem::path& strPath, const fifo_json& j, const int indent = -1);

template <class Map>
int readJMap(const fifo_json& j, Map& mapTmp)
{
	int intCnt = 0;
	for (auto& it : j.items())
	{
		std::string key = it.key();
		it.value().get_to(mapTmp[key]);
		mapTmp[key] = mapTmp[key];
		intCnt++;
	}
	return intCnt;
}
template <typename T>
int readJson(const std::string& strJson, std::set<T>& setTmp) {
	try {
		fifo_json j(fifo_json::parse(strJson));
		j.get_to(setTmp);
		return j.size();
	} catch (...) {
		return -1;
	}
}
template <typename T1, typename T2>
int readJson(const std::string& strJson, std::map<T1, T2>& mapTmp)
{
	try
	{
		fifo_json j = fifo_json::parse(strJson);
		return readJMap(j, mapTmp);
	}
	catch (...)
	{
		return -1;
	}
}

template<class Map>
[[deprecated]] int loadJMap(const std::string& strLoc, Map& mapTmp) {
	fifo_json j = freadJson(strLoc);
	if (j.is_null())return -2;
	try 
	{
		return readJMap(j, mapTmp);
	}
	catch (...)
	{
		return -1;
	}
}

template<class Map>
int loadJMap(const std::filesystem::path& fpLoc, Map& mapTmp) {
	if (!std::filesystem::exists(fpLoc))return -2;
	fifo_json j = freadJson(fpLoc);
	if (j.is_null())return 0;
	try 
	{
		return readJMap(j, mapTmp);
	}
	catch (...)
	{
		return -1;
	}
}


//template <class C, class TKey, class TVal, TVal& (C::* U)(const TKey&) = &C::operator[]>
template <class C>
[[deprecated]] void saveJMap(const std::string& strLoc, const C& mapTmp)
{
	if (mapTmp.empty()) {
		remove(strLoc.c_str());
		return;
	}
	std::ofstream fout(strLoc);
	if (fout)
	{
		fifo_json j;
		for (auto& [key,val] : mapTmp)
		{
			j[key] = val;
		}
		fout << j.dump(2);
		fout.close();
	}
}

template <class C>
void saveJMap(const std::filesystem::path& fpLoc, const C& mapTmp)
{
	if (mapTmp.empty()) {
		remove(fpLoc);
		return;
	}
	std::ofstream fout(fpLoc);
	if (fout)
	{
		fifo_json j;
		for (auto& [key,val] : mapTmp)
		{
			j[key] = val;
		}
		fout << j.dump(2);
		fout.close();
	}
}
