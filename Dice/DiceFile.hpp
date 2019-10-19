/*
 * 文件读写
 * Copyright (C) 2019 String.Empty
 */
#include <fstream>
#include <cstdio>
#include <vector>
#include <map>
#include <set>
#include <io.h>
#include <direct.h>
#include "DiceMsgSend.h"

int mkDir(std::string dir) {
	if (_access(dir.c_str(), 0))	return _mkdir(dir.c_str());
	return -2;
}

template<typename T>
//typename std::enable_if<std::is_integral<T>::value, bool>::type fscan(std::ifstream& fin, T& t) {
inline bool fscan(std::ifstream & fin, T & t) {
	T val;
	if (fin >> val) {
		t = val;
		return true;
	}
	else return false;
}
template<>
inline bool fscan<std::string>(std::ifstream& fin, std::string& t) {
	std::string val;
	if (fin >> val) {
		while (val.find("{space}") != std::string::npos)val.replace(val.find("{space}"), 7, " ");
		while (val.find("{tab}") != std::string::npos)val.replace(val.find("{tab}"), 5, "\t");
		while (val.find("{endl}") != std::string::npos)val.replace(val.find("{endl}"), 6, "\n");
		while (val.find("{enter}") != std::string::npos)val.replace(val.find("{enter}"), 7, "\r");
		while (val.find("\\n") != std::string::npos)val.replace(val.find("\\n"), 2, "\n");
		while (val.find("\\s") != std::string::npos)val.replace(val.find("\\s"), 2, " ");
		while (val.find("\\t") != std::string::npos)val.replace(val.find("\\t"), 2, "	");
		t = val;
		return true;
	}
	else return false;
}
template<>
inline bool fscan<std::pair<long long, CQ::msgtype>>(std::ifstream& fin, std::pair<long long, CQ::msgtype>& t) {
	long long first;
	int second;
	if (fin >> first >> second) {
		t = { first,(CQ::msgtype)second };
		return true;
	}
	else return false;
}

template<typename T>
void loadFile(std::string strPath, std::set<T>&setTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		T item;
		while (fscan(fin, item))
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
		while (fscan(fin,key))
		{
			fscan(fin, Val);
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

//save
template<typename T>
inline void fprint(std::ofstream& fout, T t) {
	fout << t;
}
template<>
inline void fprint<std::string>(std::ofstream& fout, std::string s) {
	while (s.find(' ') != std::string::npos)s.replace(s.find(' '), 1, "{space}");
	while (s.find('\t') != std::string::npos)s.replace(s.find('\t'), 1, "{tab}");
	while (s.find('\n') != std::string::npos)s.replace(s.find('\n'), 1, "{endl}");
	while (s.find('\r') != std::string::npos)s.replace(s.find('\r'), 1, "{enter}");
	fout << s;
}
template<typename T1,typename T2>
inline void fprint(std::ofstream& fout, std::pair<T1, T2> t) {
	fprint(fout, t.first);
	fout << " ";
	fprint(fout, t.second);
}
template<typename T>
bool clrEmpty(std::string strPath, const T& tmp) {
	if (tmp.empty()) {
		std::ifstream fin(strPath);
		if (fin) {
			fin.close();
			remove(strPath.c_str());
		}
		return true;
	}
	return false;
}
template<typename T>
void saveFile(std::string strPath, const T& setTmp) {
	if (clrEmpty(strPath, setTmp))return;
	std::ofstream fout(strPath);
	for (auto it : setTmp)
	{
		fprint(fout, it);
		fout << std::endl;
	}
	fout.close();
}