/*
 * 文件读写
 * Copyright (C) 2019-2020 String.Empty
 */
#pragma once
#include <fstream>
#include <sstream> 
#include <cstdio>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <io.h>
#include <direct.h>
#include "DiceMsgSend.h"
#include "DiceXMLTree.h"
#include "StrExtern.hpp"


using std::ifstream;
using std::ofstream;
using std::ios;
using std::stringstream;
using std::enable_if;
using std::map;
using std::multimap;
using std::set;
using std::unordered_set;
using std::unordered_map;

int mkDir(const std::string& dir);

int clrDir(std::string dir, const std::set<std::string>& exceptList);

template<typename TKey, typename TVal, typename sort>
void map_merge(map<TKey, TVal, sort>& m1, const map<TKey, TVal, sort>& m2) {
	for (auto &[k,v] : m2) {
		m1[k] = v;
	}
}
template<typename TKey, typename TVal>
inline TVal get(const map<TKey, TVal>& m, TKey key, TVal def) {
	return (m.count(key) ? m.at(key): def);
	/*auto it = m.find(key);
	if (it == m.end())return def;
	else return it->second;*/
}

vector<string> getLines(const string& s);

string printLine(string s);

template<typename T>
//typename std::enable_if<std::is_integral<T>::value, bool>::type fscan(std::ifstream& fin, T& t) {
inline bool fscan(std::ifstream & fin, T & t) {
	if (fin >> t) {
		return true;
	}
	else return false;
}
//template<>
bool fscan(std::ifstream& fin, std::string& t);

template<class C, bool(C::* U)(std::ifstream&) = &C::load>
inline bool fscan(std::ifstream& fin, C& obj) {
	if (obj.load(fin))return true;
	return false;
}

// 读取二进制文件——基础类型重载
template<typename T>
inline typename std::enable_if_t<std::is_fundamental_v<T>, T> fread(ifstream& fin) {
	T t;
	fin.read((char*)&t, sizeof(T));
	return t;
}

// 读取二进制文件——std::string重载
template<typename T>
typename std::enable_if_t<std::is_same_v<T, std::string>, T> fread(ifstream& fin) {
	short len = fread<short>(fin);
	char* buff = new char[len];
	fin.read(buff, sizeof(char) * len);
	std::string s(buff, len);
	delete[] buff;
	return s;
}

// 读取二进制文件——含readb函数类重载
template<class C, void(C::* U)(std::ifstream&) = &C::readb>
inline C fread(ifstream& fin) {
	C obj;
	obj.readb(fin);
	return obj;
}

// 读取二进制文件——std::map重载
template<typename T1,typename T2>
std::map<T1, T2> fread(ifstream& fin) {
	std::map<T1, T2>m;
	T1 key;
	T2 val;
	short len = fread<short>(fin);
	while (len--) {
		key = fread<T1>(fin);
		val = fread<T2>(fin);
		m[key] = val;
	}
	return m;
}

// 读取二进制文件——std::set重载
template<typename T,bool isLib>
std::set<T> fread(ifstream& fin) {
	T item;
	short len = fread<short>(fin);
	set<T> s{};
	while (len--) {
		item = fread<T>(fin);
		s.insert(item);
	}
	return s;
}
template<typename T1, typename T2>
void readini(string& line,std::pair<T1,T2>& p) {
	int pos = line.find('=');
	if (pos == std::string::npos)return;
	std::istringstream sin(line.substr(0, pos));
	sin >> p.first;
	sin.clear();
	sin.str(line.substr(pos + 1));
	sin >> p.second;
	return;
}

void readini(ifstream& fin, std::string& s);

template<typename T1, typename T2>
void readini(string s, std::map<T1,T2>& m) {
	std::pair<T1, T2> p;
	string line;
	stringstream ss(s);
	while (getline(ss,line)) {
		readini(line, p);
		m[p.first] = p.second;
	}
	return;
}

template<typename T>
void readini(string s, std::vector<T>& v) {
	T p;
	string line;
	stringstream ss(s);
	while (getline(ss, line)) {
		readini(line, p);
		v.push_back(p);
	}
	return;
}

template<typename T>
int loadFile(std::string strPath, std::set<T>&setTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		int Cnt = 0;
		T item;
		while (fscan(fin, item))
		{
			setTmp.insert(item);
			Cnt++;
		}
		return Cnt;
	}
	fin.close();
	return -1;
}

template<typename T>
int loadFile(std::string strPath, std::unordered_set<T>& setTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		int Cnt = 0;
		T item;
		while (fscan(fin, item))
		{
			setTmp.insert(item);
			Cnt++;
		}
		return Cnt;
	}
	fin.close();
	return -1;
}

template<typename T1, typename T2>
int loadFile(std::string strPath, std::map<T1, T2>&mapTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		int Cnt = 0;
		T1 key;
		while (fin >> key)
		{
			fscan(fin, mapTmp[key]);
			Cnt++;
		}
		return Cnt;
	}
	fin.close();
	return -1;
}

template<typename T1, typename T2>
int loadFile(std::string strPath, std::unordered_map<T1, T2>& mapTmp) {
	std::ifstream fin(strPath);
	if (fin)
	{
		int Cnt = 0;
		T1 key;
		while (fin >> key)
		{
			fscan(fin, mapTmp[key]);
			Cnt++;
		}
		return Cnt;
	}
	fin.close();
	return -1;
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

template<typename T, class C, void(C::* U)(std::ifstream&) = &C::readb>
int loadBFile(std::string strPath, std::map<T, C> & m) {
	std::ifstream fin(strPath, std::ios::in | std::ios::binary);
	if (!fin)return -1;
	int len = fread<int>(fin);
	int Cnt = 0;
	T key;
	C val;
	while (fin.peek() != EOF && len > Cnt++) {
		key = fread<T>(fin);
		m[key].readb(fin);
	}
	fin.close();
	return Cnt;
}

template<typename T, class C, void(C::* U)(std::ifstream&) = &C::readb>
int loadBFile(std::string strPath, std::unordered_map<T, C>& m) {
	std::ifstream fin(strPath, std::ios::in | std::ios::binary);
	if (!fin)return -1;
	int len = fread<int>(fin);
	int Cnt = 0;
	T key;
	C val;
	while (fin.peek() != EOF && len > Cnt++) {
		key = fread<T>(fin);
		m[key].readb(fin);
	}
	fin.close();
	return Cnt;
}
//读取伪ini
template<class C>
int loadINI(std::string strPath, std::map<std::string, C>& m) {
	std::ifstream fin(strPath, std::ios::in | std::ios::binary);
	if (!fin)return -1;
	std::string s, name;
	C val;
	getline(fin, s);
	readini(fin, name);
	val.readi(fin);
	m[name] = val;
	fin.close();
	return 1;
}

bool rdbuf(string strPath, string& s);

//读取伪xml
template<class C,std::string(C::* U)() = &C::getName>
int loadXML(const std::string& strPath, std::map<std::string, C>& m) {
	string s;
	if (!rdbuf(strPath, s))return -1;
	DDOM xml(s);
	C obj(xml);
	m[obj.getName()].readt(xml);
	return 1;
}

//遍历文件夹
int listDir(const string& dir, vector<std::filesystem::path>& files, bool isSub = false) noexcept;

template<typename T1, typename T2>
int _loadDir(int (*load)(const std::string&, T2&) ,const std::string& strDir, T2& tmp, int& intFile, int& intFailure, int& intItem, std::vector<std::string>& files)
{
	std::error_code err;
	for (const auto& p : T1(strDir, err))
	{
		if (p.is_regular_file())
		{
			intFile++;
			string path = convert_w2a(p.path().filename().wstring().c_str());
			int Cnt = load(strDir + path, tmp);
			if (Cnt < 0) {
				files.push_back(path);
				intFailure++;
			}
			else intItem += Cnt;
		}
	}
	if (err) return -1;
	return 0;
}

//读取文件夹
template<typename T>
int loadDir(int (*load)(const std::string&, T&), const std::string& strDir, T& tmp, std::string& strLog, bool isSubdir = false) {
	int intFile = 0, intFailure = 0, intItem = 0;
	std::vector<std::string> files;
	if (isSubdir)
	{
		if (_loadDir<std::filesystem::recursive_directory_iterator>(load, strDir, tmp, intFile, intFailure, intItem, files) == -1) return 0;
	}
	else
	{
		if (_loadDir<std::filesystem::directory_iterator>(load, strDir, tmp, intFile, intFailure, intItem, files) == -1) return 0;
	}

	if (!intFile)return 0;
	strLog += "读取" + strDir + "中的" + std::to_string(intFile) + "个文件, 共" + std::to_string(intItem) + "个条目\n";
	if (intFailure) {
		strLog += "读取失败" + std::to_string(intFailure) + "个:\n";
		for (auto &it : files) {
			strLog += it + "\n";
		}
	}
	return intFile;
}

//save
template<typename T>
void fprint(std::ofstream& fout,const T& t) {
	fout << t;
}

template<class C, void(C::* U)(std::ofstream&) = &C::save>
inline void fprint(std::ofstream& fout, C obj) {
	obj.save(fout);
}

void fprint(std::ofstream& fout, std::string s);

template<typename T1, typename T2>
inline void fprint(std::ofstream& fout, std::pair<T1, T2> t) {
	fprint(fout, t.first);
	fout << "\t";
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
inline typename std::enable_if<!std::is_class<T>::value, void>::type fwrite(ofstream& fout, T t) {
	T val = t;
	fout.write((char*)&val, sizeof(T));
}


void fwrite(ofstream& fout, const std::string& s);

template<class C, void(C::* U)(std::ofstream&) = &C::writeb>
inline void fwrite(ofstream& fout, C& obj) {
	obj.writeb(fout);
}
template<typename T1, typename T2>
inline void fwrite(ofstream& fout, const std::map<T1,T2>& m) {
	short len = (short)m.size();
	fwrite(fout, len);
	for (auto it : m) {
		fwrite(fout, it.first);
		fwrite(fout, it.second);
	}
}
template<typename T>
inline void fwrite(ofstream& fout, const std::set<T>& s) {
	short len = (short)s.size();
	fwrite(fout, len);
	for (auto it : s) {
		fwrite(fout, it);
	}
}
template<typename T>
void saveFile(std::string strPath, const T& setTmp) {
	if (clrEmpty(strPath, setTmp))return;
	std::ofstream fout(strPath);
	for (const auto &it : setTmp)
	{
		fprint(fout, it);
		fout << std::endl;
	}
	fout.close();
}
template<typename TKey, typename TVal>
void saveFile(std::string strPath, const map<TKey,TVal>& mTmp) {
	if (clrEmpty(strPath, mTmp))return;
	std::ofstream fout(strPath);
	for (const auto& [key,val] : mTmp)
	{
		fout << key << "\t";
		fprint(fout, val);
		fout << std::endl;
	}
	fout.close();
}


template<typename TKey, typename TVal>
void saveFile(std::string strPath, const unordered_map<TKey, TVal>& mTmp) {
	if (clrEmpty(strPath, mTmp))return;
	std::ofstream fout(strPath);
	for (const auto& [key, val] : mTmp)
	{
		fout << key << "\t";
		fprint(fout, val);
		fout << std::endl;
	}
	fout.close();
}

template<typename T, class C, void(C::* U)(std::ofstream&) = &C::writeb>
void saveBFile(std::string strPath, std::map<T, C>& m) {
	if (clrEmpty(strPath, m))return;
	std::ofstream fout(strPath, ios::out | ios::trunc | ios::binary);
	int len = m.size();
	fwrite<int>(fout, len);
	for (auto &[key,val] : m) {
		fwrite(fout, key);
		fwrite(fout, val);
	}
	fout.close();
}

template<typename T, class C, void(C::* U)(std::ofstream&) = &C::writeb>
void saveBFile(std::string strPath, std::unordered_map<T, C>& m) {
	if (clrEmpty(strPath, m))return;
	std::ofstream fout(strPath, ios::out | ios::trunc | ios::binary);
	int len = m.size();
	fwrite<int>(fout, len);
	for (auto& [key, val] : m) {
		fwrite(fout, key);
		fwrite(fout, val);
	}
	fout.close();
}

//读取伪xml
template<class C, std::string(C::* U)() = &C::writet>
void saveXML(std::string strPath, C& obj) {
	std::ofstream fout(strPath);
	fout << obj.writet();
}