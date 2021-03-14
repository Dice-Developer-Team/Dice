#pragma once

// Json信息获取以及写入

#include <fstream>
#include <map>
#include <vector>
#include <set>
#include <filesystem>
#include "json.hpp"
#include "EncodingConvert.h"

using nlohmann::json;

class JsonList
{
	std::vector<std::string> vRes;
public:
	std::string dump()
	{
		if (vRes.empty())return "{}";
		if (vRes.size() == 1)return vRes[0];
		std::string s;
		for (auto it = vRes.begin(); it != vRes.end(); ++it)
		{
			if (it == vRes.begin())s = "[\n" + *it;
			else s += ",\n" + *it;
		}
		s += "\n]";
		return s;
	}

	JsonList& operator<<(std::string s)
	{
		vRes.push_back(s);
		return *this;
	}

	[[nodiscard]] bool empty() const
	{
		return vRes.empty();
	}

	[[nodiscard]] size_t size() const
	{
		return vRes.size();
	}
};

template <typename T>
std::enable_if_t<!std::is_arithmetic_v<T>, T> readJKey(const std::string& strJson)
{
	return UTF8toGBK(strJson);
}

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, T> readJKey(const std::string& strJson)
{
	return stoll(strJson);
}

[[deprecated]] nlohmann::json freadJson(const std::string& strPath);
nlohmann::json freadJson(const std::filesystem::path& path);
[[deprecated]] void fwriteJson(const std::string& strPath, const json& j);
void fwriteJson(const std::filesystem::path& strPath, const json& j);

template <class Map>
int readJMap(const nlohmann::json& j, Map& mapTmp)
{
	int intCnt = 0;
	for (auto it = j.cbegin(); it != j.cend(); ++it)
	{
		std::string key = UTF8toGBK(it.key());
		it.value().get_to(mapTmp[key]);
		mapTmp[key] = UTF8toGBK(mapTmp[key]);
		intCnt++;
	}
	return intCnt;
}
template <typename T>
int readJson(const std::string& strJson, std::set<T>& setTmp) {
	try {
		nlohmann::json j(nlohmann::json::parse(strJson));
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
		nlohmann::json j = nlohmann::json::parse(strJson);
		return readJMap(j, mapTmp);
	}
	catch (...)
	{
		return -1;
	}
}

template<class Map>
[[deprecated]] int loadJMap(const std::string& strLoc, Map& mapTmp) {
	nlohmann::json j = freadJson(strLoc);
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
	nlohmann::json j = freadJson(fpLoc);
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
		nlohmann::json j;
		for (auto& [key,val] : mapTmp)
		{
			j[GBKtoUTF8(key)] = GBKtoUTF8(val);
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
		nlohmann::json j;
		for (auto& [key,val] : mapTmp)
		{
			j[GBKtoUTF8(key)] = GBKtoUTF8(val);
		}
		fout << j.dump(2);
		fout.close();
	}
}
