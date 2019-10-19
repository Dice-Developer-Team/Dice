#pragma once
#include <fstream>
#include <map>
#include <vector>
#include <io.h>
#include "json.hpp"
#include "EncodingConvert.h" 

template<typename T>
typename std::enable_if<!std::is_arithmetic<T>::value, T>::type readJKey(std::string strJson) {
	return UTF8toGBK(strJson);
}
template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type readJKey(std::string strJson) {
	return stoll(strJson);
}

template<typename T1, typename T2>
int readJson(std::string strJson, std::map<T1, T2> &mapTmp) {
	nlohmann::json j;
	int intCnt = 0;
	try {
		j = nlohmann::json::parse(strJson);
		for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it)
		{
			T1 tKey = readJKey<T1>(it.key());
			T2 tVal = it.value();
			tVal = UTF8toGBK(tVal);
			mapTmp[tKey] = tVal;
			intCnt++;
		}
	}
	catch (...) {
		return -1;
	}
	return intCnt;
}

template<typename T1, typename T2>
int loadJMap(std::string strLoc, std::map<T1, T2> &mapTmp) {
	std::ifstream fin(strLoc);
	int Cnt = 0;
	if (fin) {
		nlohmann::json j;
		try {
			fin >> j;
			for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it)
			{
				T1 tKey = readJKey<T1>(it.key());
				T2 tVal = it.value();
				tVal = UTF8toGBK(tVal);
				mapTmp[tKey] = tVal;
				Cnt++;
			}
		}
		catch (...) {
			fin.close();
			return -1;
		}
	}
	fin.close();
	return Cnt;
}
//读取文件夹
template<typename T1, typename T2>
int loadJMaps(std::string strDir, std::map<T1, T2>& mapTmp, std::string &strLog) {
	_finddata_t file;
	long lf = _findfirst((strDir + "*").c_str(), &file);
	if (lf < 0)return 0;
	int intFile = 0, intFailure = 0, intItem = 0;
	std::vector<std::string> files;
	do {
		//遍历文件
		if (!strcmp(file.name, ".") || !strcmp(file.name, ".."))continue;
		if (file.attrib == _A_SUBDIR)continue;
		else {
			intFile++;
			int Cnt = loadJMap(strDir + file.name, mapTmp);
			if (Cnt < 0) {
				files.push_back(file.name);
				intFailure++;
			}
			else intItem += Cnt;
		}
	} while (!_findnext(lf, &file));
	strLog = "读取" + strDir + "中的" + std::to_string(intFile) + "个文件, 共" + std::to_string(intItem) + "个条目";
	if (intFailure) {
		strLog += "，读取失败" + std::to_string(intFailure) + "个:";
		for (auto it : files) {
			strLog += "\n" + it;
		}
	}
	return intFile;
}

template<typename T>
std::string writeJKey(typename std::enable_if<!std::is_arithmetic<T>::value, T>::type strJson) {
	return GBKtoUTF8(strJson);
}
template<typename T>
std::string writeJKey(typename std::enable_if<std::is_arithmetic<T>::value, T>::type llJson) {
	return std::to_string(llJson);
}

template<typename T1, typename T2>
int saveJMap(std::string strLoc, std::map<T1, T2> mapTmp) {
	if (mapTmp.empty())return 0;
	std::ofstream fout(strLoc);
	if (fout) {
		nlohmann::json j;
		for (auto it : mapTmp) {
			j[writeJKey<T1>(it.first)] = GBKtoUTF8(it.second);
		}
		fout << j;
		fout.close();
	}
	return 0;
}