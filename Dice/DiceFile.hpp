/*
 * 文件读写
 * Copyright (C) 2019 String.Empty
 */
#include <fstream>
#include <cstdio>
#include <set>
#include <io.h>
#include <direct.h>

int mkDir(std::string dir) {
	if (_access(dir.c_str(), 0))	return _mkdir(dir.c_str());
	return -2;
}

template<typename T>
void loadFile(std::string strPath, std::set<T>&setTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		T item;
		while (fin >> item)
		{
			setTmp.insert(item);
		}
	}
	fin.close();
}

template<typename T1, typename T2>
void loadFile(std::string strPath, std::map<T1, T2>&mapTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		T1 key;
		T2 Val;
		while (fin >> key >> Val)
		{
			mapTmp[key] = Val;
		}
	}
	fin.close();
}

template<typename T1, typename T2>
void loadFile(std::string strPath, std::multimap<T1,T2>&mapTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		T1 key;
		T2 Val;
		while (fin >> key >> Val)
		{
			mapTmp.insert({ key,Val });
		}
	}
	fin.close();
}

template<typename T>
int loadDir(int load(std::string, T&), std::string strDir, T& tmp, std::string& strLog) {
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
			int Cnt = load(strDir + file.name, tmp);
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
void saveFile(std::string strPath, std::set<T>&setTmp) {
	if (setTmp.empty()) {
		std::ifstream fin(strPath);
		if (fin) {
			fin.close();
			remove(strPath.c_str());
		}
	}
	else {
		std::ofstream fout(strPath);
		for (auto it : setTmp)
		{
			fout << it << std::endl;
		}
		fout.close();
	}	
}

template<typename T1, typename T2>
void saveFile(std::string strPath, std::map<T1, T2>&mapTmp) {
	if (mapTmp.empty()) {
		std::ifstream fin(strPath);
		if (fin) {
			fin.close();
			remove(strPath.c_str());
		}
	}
	else {
		std::ofstream fout(strPath);
		for (auto it : mapTmp)
		{
			fout << it.first << " " << it.second << std::endl;
		}
		fout.close();
	}
}

template<typename T1, typename T2>
void saveFile(std::string strPath, std::multimap<T1, T2>&mapTmp) {
	if (mapTmp.empty()) {
		std::ifstream fin(strPath);
		if (fin) {
			fin.close();
			remove(strPath.c_str());
		}
	}
	else {
		std::ofstream fout(strPath);
		for (auto it : mapTmp)
		{
			fout << it.first << " " << it.second << std::endl;
		}
		fout.close();
	}
}