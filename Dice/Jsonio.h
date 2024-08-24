#pragma once

// Json io

#include <fstream>
#include <map>
#include <vector>
#include <set>
#include "filesystem.hpp"
#include "fifo_json.hpp"

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
	fifo_json j(fifo_json::parse(strJson));
	j.get_to(setTmp);
	return j.size();
}
template <typename T1, typename T2>
int readJson(const std::string& strJson, std::map<T1, T2>& mapTmp)
{
	return readJMap(fifo_json::parse(strJson), mapTmp);
}

template<class Map>
int loadJMap(const std::filesystem::path& fpLoc, Map& mapTmp) {
	if (std::filesystem::exists(fpLoc)) {
		if (fifo_json j = freadJson(fpLoc); !j.is_null()) {
			return readJMap(j, mapTmp);
		}
		return 0;
	}
	return -2;
}

template <class C>
void saveJMap(const std::filesystem::path& fpLoc, const C& mapTmp)
{
	if (!mapTmp.empty()) {
		if (std::ofstream fout{ fpLoc }){
			fifo_json j;
			for (auto& [key, val] : mapTmp)
			{
				j[key] = val;
			}
			fout << j.dump(2);
			fout.close();
		}
	}
	else remove(fpLoc);
}
