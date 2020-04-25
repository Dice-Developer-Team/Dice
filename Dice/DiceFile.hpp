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
#include <io.h>
#include <direct.h>
#include "DiceMsgSend.h"
#include "DiceXMLTree.h"

using std::ifstream;
using std::ofstream;
using std::ios;
using std::stringstream;
using std::enable_if;
using std::map;
using std::multimap;
using std::set;

static int mkDir(std::string dir) {
	if (_access(dir.c_str(), 0))	return _mkdir(dir.c_str());
	return -2;
}
static int clrDir(std::string dir,const std::set<std::string>& exceptList) {
	int nCnt = 0;
	_finddata_t file;
	long lf = _findfirst((dir + "*").c_str(), &file);
	//输入文件夹路径
	if (lf < 0) {
		return -1;
	}
	else {
		do {
			if (file.attrib == _A_SUBDIR)continue;
			if (!strcmp(file.name, ".") || !strcmp(file.name, ".."))continue;
			if (strlen(file.name) < 36)continue;
			if (exceptList.count(file.name))continue;
			if (!remove((dir + file.name).c_str()))nCnt++;
		} while (!_findnext(lf, &file));
	}
	_findclose(lf);
	return nCnt;
}

template<typename TKey, typename TVal, typename sort>
void map_merge(map<TKey, TVal, sort>& m1, const map<TKey, TVal, sort>& m2) {
	for (auto &[k,v] : m2) {
		m1[k] = v;
	}
}
template<typename TKey, typename TVal>
inline TVal get(const map<TKey, TVal>& m, TKey key, TVal def) {
	auto it = m.find(key);
	if (it == m.end())return def;
	else return it->second;
}
template<typename TVal>
inline TVal get(const map<string, TVal>& m, string key, TVal def) {
	auto it = m.find(key);
	if (it == m.end())return def;
	else return it->second;
}

static vector<string> getLines(string s) {
	vector<string>vLine;
	stringstream ss(s);
	string line;
	while (getline(ss, line)) {
		int r = line.length();
		while (r && isspace(static_cast<unsigned char>(line[r - 1])))r--;
		int l = 0;
		while (isspace(static_cast<unsigned char>(line[l])) && l < r)l++;
		line = line.substr(l, r - l);
		if (line.empty())continue;
		vLine.push_back(line);
	}
	return vLine;
}
static string printLine(string s) {
	while (s.find('\t') != std::string::npos)s.replace(s.find('\t'), 1, "\\t");
	while (s.find("\r\n") != std::string::npos)s.replace(s.find("\r\n"), 2, "\\n");
	while (s.find('\r') != std::string::npos)s.replace(s.find('\r'), 1, "\\n");
	while (s.find('\n') != std::string::npos)s.replace(s.find('\n'), 1, "\\n");
	return s;
}
template<typename T>
//typename std::enable_if<std::is_integral<T>::value, bool>::type fscan(std::ifstream& fin, T& t) {
inline bool fscan(std::ifstream & fin, T & t) {
	if (fin >> t) {
		return true;
	}
	else return false;
}
//template<>
inline bool fscan(std::ifstream& fin, std::string& t) {
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

template<class C, bool(C::* U)(std::ifstream&) = &C::load>
inline bool fscan(std::ifstream& fin, C& obj) {
	if (obj.load(fin))return true;
	return false;
}

template<typename T>
inline typename std::enable_if<std::is_fundamental<T>::value, T>::type fread(ifstream& fin) {
	T t;
	fin.read((char*)&t, sizeof(T));
	return t;
}
template<typename T>
typename std::enable_if<std::is_same<T, std::string>::value, T>::type fread(ifstream& fin) {
	short len = fread<short>(fin);
	char* buff = new char[len + 1];
	fin.read(buff, sizeof(char) * len);
	buff[len] = '\0';
	std::string s(buff);
	delete[] buff;
	return s;
}
template<class C, void(C::* U)(std::ifstream&) = &C::readb>
inline C fread(ifstream& fin) {
	C obj;
	obj.readb(fin);
	return obj;
}
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
static void readini(string& line,std::pair<T1,T2>& p) {
	int pos = line.find('=');
	if (pos == std::string::npos)return;
	std::istringstream sin(line.substr(0, pos));
	sin >> p.first;
	sin.clear();
	sin.str(line.substr(pos + 1));
	sin >> p.second;
	return;
}
static void readini(ifstream& fin, std::string& s) {
	std::string line;
	getline(fin, line);
	int pos = line.find('=');
	if (pos == std::string::npos)return;
	std::istringstream sin(line.substr(pos + 1));
	sin >> s;
	return;
}
template<typename T1, typename T2>
static void readini(string s, std::map<T1,T2>& m) {
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
static void readini(string s, std::vector<T>& v) {
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

template<typename T1, typename T2>
typename int loadFile(std::string strPath, std::map<T1, T2>&mapTmp) {
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
typename void loadFile(std::string strPath, std::multimap<T1,T2>&mapTmp) {
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
static bool rdbuf(string strPath,string& s) {
	std::ifstream fin(strPath);
	if (!fin)return false;
	stringstream ss;
	ss << fin.rdbuf();
	s = ss.str();
	return true;
}
//读取伪xml
template<class C,std::string(C::* U)() = &C::getName>
int loadXML(std::string strPath, std::map<std::string, C>& m) {
	string s;
	if (!rdbuf(strPath, s))return -1;
	DDOM xml(s);
	C obj(xml);
	m[obj.getName()].readt(xml);
	return 1;
}

//遍历文件夹
int listDir(string, set<string>&, bool = false, string subdir = "");

//读取文件夹
template<typename T>
int loadDir(int load(std::string, T&), std::string strDir, T& tmp, std::string& strLog, bool isSubdir = false) {
	_finddata_t file;
	long lf = _findfirst((strDir + "*").c_str(), &file);
	if (lf < 0)return 0;
	int intFile = 0, intFailure = 0, intItem = 0;
	std::vector<std::string> files;
	std::vector<std::string> dirs;
	do {
		//遍历文件
		if (!strcmp(file.name, ".") || !strcmp(file.name, ".."))continue;
		if (file.attrib == _A_SUBDIR)dirs.push_back(file.name);
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
	if (!intFile)return 0;
	strLog += "读取" + strDir + "中的" + std::to_string(intFile) + "个文件, 共" + std::to_string(intItem) + "个条目\n";
	if (intFailure) {
		strLog += "读取失败" + std::to_string(intFailure) + "个:\n";
		for (auto &it : files) {
			strLog += it + "\n";
		}
	}
	if (isSubdir)for (auto it : dirs) {
		loadDir(load, it + "\\", tmp, strLog, true);
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
inline void fprint(std::ofstream& fout, std::string s) {
	while (s.find(' ') != std::string::npos)s.replace(s.find(' '), 1, "{space}");
	while (s.find('\t') != std::string::npos)s.replace(s.find('\t'), 1, "{tab}");
	while (s.find('\n') != std::string::npos)s.replace(s.find('\n'), 1, "{endl}");
	while (s.find('\r') != std::string::npos)s.replace(s.find('\r'), 1, "{enter}");
	fout << s;
}
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


inline void fwrite(ofstream& fout, const std::string s) {
	short len = (short)s.length();
	fout.write((char*)& len, sizeof(short));
	fout.write(s.c_str(), sizeof(char) * s.length());
}
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
//读取伪xml
template<class C, std::string(C::* U)() = &C::writet>
void saveXML(std::string strPath, C& obj) {
	std::ofstream fout(strPath);
	fout << obj.writet();
}