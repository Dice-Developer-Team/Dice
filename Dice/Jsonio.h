#pragma once
#include <fstream>
#include <map>
#include <vector>
#include "json.hpp"
#include "EncodingConvert.h" 

template<typename T>
T readJson(std::string strJson) {
	return UTF8toGBK(strJson);
}

template<>
long long readJson(std::string strJson) {
	return stoll(strJson);
}

template<typename T>
std::vector<T> readJVec(std::vector<std::string> vJson) {
	std::vector<T> vTmp;
	for (auto it : vJson) {
		vTmp.push_back(readJson<T>(it));
	}
	return vTmp;
}

template<typename T1, typename T2>
int loadJMap(std::string strLoc, std::map<T1, std::vector<T2>> &mapTmp) {
	std::ifstream fin(strLoc);
	if (fin) {
		nlohmann::json j;
		fin >> j;
		for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it)
		{
			T1 tKey = readJson<T1>(it.key());
			std::vector <T2> tVal = readJVec<T2>(it.value());
			mapTmp[tKey] = tVal;
		}
		fin.close();
	}
	return 0;
}

std::string writeJson(std::string strJson) {
	return GBKtoUTF8(strJson);
}

std::string writeJson(long long llJson) {
	return std::to_string(llJson);
}

template<typename T>
std::vector<std::string> writeJVec(std::vector<T> vJson) {
	std::vector<std::string> vTmp;
	for (auto it : vJson) {
		vTmp.push_back(writeJson(it));
	}
	return vTmp;
}

template<typename T1, typename T2>
int saveJMap(std::string strLoc, std::map<T1, std::vector<T2>> mapTmp) {
	if (mapTmp.empty())return 0;
	std::ofstream fout(strLoc);
	if (fout) {
		nlohmann::json j;
		for (auto it : mapTmp) {
			j[writeJson(it.first)] = writeJVec(it.second);
		}
		fout << j;
		fout.close();
	}
	return 0;
}