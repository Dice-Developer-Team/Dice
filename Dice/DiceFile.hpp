/*
 * ÎÄ¼þ¶ÁÐ´
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