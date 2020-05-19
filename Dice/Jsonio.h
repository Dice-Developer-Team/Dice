// Json信息获取以及写入
#pragma once
#include <fstream>
#include <map>
#include <vector>
#include <filesystem>
#include "json.hpp"
#include "EncodingConvert.h" 

class JsonList {
	std::vector<std::string> vRes;
public:
	std::string dump() {
		if (vRes.empty())return "{}";
		if (vRes.size() == 1)return vRes[0];
		std::string s;
		for (auto it = vRes.begin(); it != vRes.end(); it++) {
			if (it == vRes.begin())s = "[\n" + *it;
			else s += ",\n" + *it;
		}
		s += "\n]";
		return s;
	}
	JsonList& operator<<(std::string s) {
		vRes.push_back(s);
		return *this;
	}
	bool empty()const {
		return vRes.empty();
	}
	size_t size()const {
		return vRes.size();
	}
};

template<typename T>
typename std::enable_if_t<!std::is_arithmetic_v<T>, T> readJKey(std::string strJson) {
	return UTF8toGBK(strJson);
}
template<typename T>
typename std::enable_if_t<std::is_arithmetic_v<T>, T> readJKey(std::string strJson) {
	return stoll(strJson);
}

nlohmann::json freadJson(std::string strPath);
nlohmann::json freadJson(const std::filesystem::path& path);

template<typename T1, typename T2, class sort>
int readJMap(const nlohmann::json& j, std::map<T1, T2, sort>& mapTmp) {
	int intCnt = 0;
	for (auto it = j.cbegin(); it != j.cend(); ++it) {
		T1 tKey = readJKey<T1>(it.key());
		T2 tVal = it.value();
		tVal = UTF8toGBK(tVal);
		mapTmp[tKey] = tVal;
		intCnt++;
	}
	return intCnt;
}

template<typename T1, typename T2>
int readJson(const std::string& strJson, std::map<T1, T2> &mapTmp) {
	try {
		nlohmann::json j = nlohmann::json::parse(strJson);
		return readJMap(j, mapTmp);
	}
	catch (...) {
		return -1;
	}
}

template<typename T1, typename T2, typename sort>
int loadJMap(const std::string& strLoc, std::map<T1, T2, sort> &mapTmp) {
	std::ifstream fin(strLoc);
	if (fin) {
		try {
			nlohmann::json j;
			fin >> j;
			fin.close();
			return readJMap(j, mapTmp);
		}
		catch (...) {
			fin.close();
			return -1;
		}
	}
	return -2;
}

template<typename T>
std::string writeJKey(typename std::enable_if_t<!std::is_arithmetic_v<T>, T> strJson) {
	return GBKtoUTF8(strJson);
}
template<typename T>
std::string writeJKey(typename std::enable_if_t<std::is_arithmetic_v<T>, T> llJson) {
	return std::to_string(llJson);
}

template<typename T1, typename T2, typename sort>
int saveJMap(const std::string& strLoc, std::map<T1, T2, sort> mapTmp) {
	if (mapTmp.empty())return 0;
	std::ofstream fout(strLoc);
	if (fout) {
		nlohmann::json j;
		for (auto it : mapTmp) {
			j[writeJKey<T1>(it.first)] = GBKtoUTF8(it.second);
		}
		fout << j.dump(2);
		fout.close();
	}
	return 0;
}

