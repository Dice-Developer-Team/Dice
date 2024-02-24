#pragma once

/*
 * 文件读写
 * Copyright (C) 2018-2021 w4123
 * Copyright (C) 2019-2024 String.Empty
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <set>
#include "filesystem.hpp"
#include <functional>
#include <unordered_set>
#include <cstdio>
#include "tinyxml2.h"
#include "DiceMsgSend.h"
#include "MsgFormat.h"
#include "EncodingConvert.h"
#include "GlobalVar.h"

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

int mkDir(const std::filesystem::path& dir);

int clrDir(const std::string& dir, const unordered_set<std::string>& exceptList);

template <typename Char, typename Trait, typename Alloc>
bool readFile(const std::filesystem::path& p, std::basic_string<Char, Trait, Alloc>& str) {
	ifstream fs(p);
	if (!fs)return false;
	std::basic_stringstream<Char, Trait, Alloc> buffer;
	buffer << fs.rdbuf();
	str = buffer.str();
	return true;
}

std::optional<std::string> readFile(const std::filesystem::path& p);

template <typename TKey, typename TVal>
TVal get(const std::unordered_map<TKey, TVal>& m, TKey key, TVal def)
{
	return (m.count(key) ? m.at(key) : def);
	/*auto it = m.find(key);
	if (it == m.end())return def;
	else return it->second;*/
}

string printLine(string s);

template <typename T>
//typename std::enable_if<std::is_integral<T>::value, bool>::type fscan(std::ifstream& fin, T& t) {
bool fscan(std::ifstream& fin, T& t)
{
	if (fin >> t)
	{
		return true;
	}
	return false;
}

//template<>
bool fscan(std::ifstream& fin, std::string& t);

template <class C, bool(C::* U)(std::ifstream&) = &C::load>
bool fscan(std::ifstream& fin, C& obj)
{
	if (obj.load(fin))return true;
	return false;
}

// 读取二进制文件——基础类型重载
template <typename T>
std::enable_if_t<std::is_fundamental_v<T>, T> fread(ifstream& fin)
{
	T t;
	fin.read(reinterpret_cast<char*>(&t), sizeof(T));
	return t;
}

// 读取二进制文件——std::string重载
template <typename T>
std::enable_if_t<std::is_same_v<T, std::string>, T> fread(ifstream& fin)
{
	const short len = fread<short>(fin);
	char* buff = new char[len];
	fin.read(buff, sizeof(char) * len);
	std::string s(buff, len);
	delete[] buff;
	return s;
}

// 读取二进制文件——含readb函数类重载
template <class C, void(C::* U)(std::ifstream&) = &C::readb>
C fread(ifstream& fin)
{
	C obj;
	obj.readb(fin);
	return obj;
}

template <typename T>
std::enable_if_t<std::is_fundamental_v<T> || std::is_same_v<T, std::string>, void> fread(ifstream& fin, T& ref) {
	ref = fread<T>(fin);
}

template <class C, void(C::* U)(std::ifstream&) = &C::readb>
void fread(ifstream& fin, C& ref) {
	ref.readb(fin);
}

template <typename T1, typename T2, typename sort>
std::map<T1, T2, sort>& fread(ifstream& fin, std::map<T1, T2, sort>& dir) {
	if (short len = fread<short>(fin); len > 0) while (len--) {
		T1 key = fread<T1>(fin);
		fread(fin, dir[key]);
	}
	return dir;
}
template <typename T1, typename T2>
std::unordered_map<T1, T2>& fread(ifstream& fin, std::unordered_map<T1, T2>& dir) {
	if (short len = fread<short>(fin); len > 0) while (len--) {
		T1 key = fread<T1>(fin);
		fread<T2>(fin, dir[key]);
	}
	return dir;
}
template <typename T1, typename T2>
fifo_map<T1, T2> fread(ifstream& fin) {
	fifo_map<T1, T2> dir;
	if (short len = fread<short>(fin); len > 0) while (len--) {
		T1 key = fread<T1>(fin);
		fread<T2>(fin, dir[key]);
	}
	return dir;
}
// 读取二进制文件——std::set重载
template <typename T>
void fread(ifstream& fin, std::unordered_set<T>& s){
	short len = fread<short>(fin);
	if (len > 0)while (len--){
		const T item = fread<T>(fin);
		s.insert(item);
	}
}
template <typename T, bool isLib>
std::set<T> fread(ifstream& fin)
{
	short len = fread<short>(fin);
	if (len < 0)return {};
	set<T> s{};
	while (len--)
	{
		const T item = fread<T>(fin);
		s.insert(item);
	}
	return s;
}

template <typename T1, typename T2>
void readini(const string& line, std::pair<T1, T2>& p, char delim = '=')
{
	const size_t pos = line.find(delim);
	if (pos == std::string::npos)return;
	std::istringstream sin(line.substr(0, pos));
	sin >> p.first;
	sin.clear();
	sin.str(line.substr(pos + 1));
	sin >> p.second;
}
template <typename T1 = std::string, typename T2 = std::string>
std::pair<T1, T2> readini(const string& line, char delim = '='){
	const size_t pos = line.find(delim);
	std::pair<T1, T2> p;
	std::istringstream sin(line.substr(0, pos));
	sin >> p.first;
	if (pos == std::string::npos)return p;
	sin.clear();
	sin.str(line.substr(pos + 1));
	sin >> p.second;
	return p;
}

void readini(ifstream& fin, std::string& s);

template <typename T1, typename T2, class Hasher, class Equal>
void readini(string s, unordered_map<T1, T2, Hasher, Equal>& m)
{
	std::pair<T1, T2> p;
	string line;
	stringstream ss(s);
	while (getline(ss, line))
	{
		readini(line, p);
		m[p.first] = p.second;
	}
}

template <typename T>
void readini(string s, std::vector<T>& v)
{
	T p;
	string line;
	stringstream ss(s);
	while (getline(ss, line))
	{
		readini(line, p);
		v.push_back(p);
	}
}

template <typename T>
int loadFile(const std::filesystem::path& fpPath, std::set<T>& setTmp)
{
	std::ifstream fin(fpPath);
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

template <typename T>
int loadFile(const std::filesystem::path& fpPath, std::unordered_set<T>& setTmp)
{
	std::ifstream fin(fpPath);
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

template <typename T1, typename T2>
int loadFile(const std::filesystem::path& fpPath, std::map<T1, T2>& mapTmp)
{
	std::ifstream fin(fpPath);
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
template <typename T1, typename T2>
int loadFile(const std::filesystem::path& fpPath, std::unordered_map<T1, T2>& mapTmp) 
{
	std::ifstream fin(fpPath);
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
template <typename T1, typename T2>
int loadFile(const std::filesystem::path& fpPath, nlohmann::fifo_map<T1, T2>& mapTmp)
{
	std::ifstream fin(fpPath);
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

template <typename T, class C, void(C::* U)(std::ifstream&) = &C::readb>
int loadBFile(const std::filesystem::path& fpPath, std::unordered_map<T, std::shared_ptr<C>>& m)
{
	if (std::ifstream fin{ fpPath, std::ios::in | std::ios::binary }) {
		int Cnt = 0;
		const int len = fread<int>(fin);
		while (fin.peek() != EOF && len > Cnt++){
			auto key = fread<T>(fin);
			auto val{ std::make_shared<C>(key) };
			((*val).*U)(fin);
			m.emplace(key, val);
		}
		return Cnt;
	} 
	return -1;
}
template <typename T, class C, void(C::* U)(std::ifstream&) = &C::readb>
int loadBFile(const std::filesystem::path& fpPath, std::unordered_map<T, C>& m){
	if (std::ifstream fin{ fpPath, std::ios::in | std::ios::binary }) {
		int Cnt = 0;
		const int len = fread<int>(fin);
		while (fin.peek() != EOF && len > Cnt++){
			auto key = fread<T>(fin);
			(m[key].*U)(fin);
		}
		fin.close();
		return Cnt;
	}
	return -1;
}

bool rdbuf(const std::filesystem::path& fpPath, string& s);

//遍历文件夹
int listDir(const std::filesystem::path& dir, vector<std::filesystem::path>& files, bool isSub = false);
size_t cntDirFile(const std::filesystem::path& dir);

template <typename T1, typename T2>
int _loadDir(int (*load)(const std::filesystem::path&, T2&), const std::filesystem::path& fpDir, T2& tmp, int& intFile, int& intFailure,
             int& intItem, std::vector<std::string>& failureFiles)
{
	std::error_code err;
	for (const auto& p : T1(fpDir, err))
	{
		if (std::filesystem::is_regular_file(p.status()) && p.path().filename().u8string().substr(0, 1) != ".")
		{
			intFile++;
			const int Cnt = load(p, tmp);
			if (Cnt < 0)
			{
				failureFiles.push_back(UTF8toGBK(p.path().filename().u8string()));
				intFailure++;
			}
			else intItem += Cnt;
		}
	}
	if (err) return -1;
	return 0;
}

//读取文件夹
template <typename T>
int loadDir(int (*load)(const std::filesystem::path&, T&), const std::filesystem::path& fpDir, T& tmp, ResList& logList,
            bool isSubdir = false)
{
	int intFile = 0, intFailure = 0, intItem = 0;
	std::vector<std::string> files;
	if (isSubdir)
	{
		if (_loadDir<std::filesystem::recursive_directory_iterator>(load, fpDir, tmp, intFile, intFailure, intItem,
		                                                            files) == -1) return 0;
	}
	else
	{
		if (_loadDir<std::filesystem::directory_iterator>(load, fpDir, tmp, intFile, intFailure, intItem, files) == -1)
			return 0;
	}

	if (!intFile)return 0;
	logList << "读取" + UTF8toGBK(fpDir.filename().u8string()) + "/中的" + std::to_string(intFile) + "个文件, 共" + std::to_string(intItem) + "个条目";
	if (intFailure)
	{
		logList << "读取失败" + std::to_string(intFailure) + "个:";
		for (auto& it : files)
		{
			logList << it;
		}
	}
	return intFile;
}

//save
template <typename T>
void fprint(std::ofstream& fout, const T& t)
{
	fout << t;
}

template <class C, void(C::* U)(std::ofstream&) = &C::save>
void fprint(std::ofstream& fout, C obj)
{
	obj.save(fout);
}

template <typename T>
bool clrEmpty(const std::filesystem::path& fpPath, const T& tmp)
{
	if (tmp.empty())
	{
		std::ifstream fin(fpPath);
		if (fin)
		{
			fin.close();
			remove(fpPath);
		}
		return true;
	}
	return false;
}

template <typename T>
typename std::enable_if<!std::is_class<T>::value, void>::type fwrite(ofstream& fout, T t)
{
	T val = t;
	fout.write(reinterpret_cast<char*>(&val), sizeof(T));
}


void fwrite(ofstream& fout, const std::string& s);

template <class C, void(C::* U)(std::ofstream&) const = &C::writeb>
void fwrite(ofstream& fout, const C& obj)
{
	obj.writeb(fout);
}
template <class C, void(C::* U)(std::ofstream&) const = &C::writeb>
void fwrite(ofstream& fout, const std::shared_ptr<C>& obj){
	obj->writeb(fout);
}

template <class C, void(C::* U)(std::ofstream&) = &C::writeb>
void fwrite(ofstream& fout, C& obj)
{
	obj.writeb(fout);
}


template <typename T1, typename T2>
void fwrite(ofstream& fout, const std::map<T1, T2>&m)
{
	const auto len = static_cast<short>(m.size());
	fwrite(fout, len);
	for (const auto& it : m)
	{
		fwrite(fout, it.first);
		fwrite(fout, it.second);
	}
}
template <typename T1, typename T2>
void fwrite(ofstream& fout, const std::unordered_map<T1, T2>& m)
{
	const auto len = static_cast<short>(m.size());
	fwrite(fout, len);
	for (const auto& it : m)
	{
		fwrite(fout, it.first);
		fwrite(fout, it.second);
	}
}
template <typename T1, typename T2>
void fwrite(ofstream& fout, const nlohmann::fifo_map<T1, T2>& m)
{
	const auto len = static_cast<short>(m.size());
	fwrite(fout, len);
	for (const auto& it : m)
	{
		fwrite(fout, it.first);
		fwrite(fout, it.second);
	}
}

template <typename T>
void fwrite(ofstream& fout, const std::unordered_set<T>& s)
{
	const auto len = static_cast<size_t>(s.size());
	fwrite(fout, len);
	for (const auto& it : s)
	{
		fwrite(fout, it);
	}
}
template <typename T>
void fwrite(ofstream& fout, const std::set<T>& s)
{
	const auto len = static_cast<short>(s.size());
	fwrite(fout, len);
	for (const auto& it : s)
	{
		fwrite(fout, it);
	}
}

template <typename T>
void saveFile(const std::filesystem::path& fpPath, const T& setTmp)
{
	if (clrEmpty(fpPath, setTmp))return;
	std::ofstream fout(fpPath);
	for (const auto& it : setTmp)
	{
		fprint(fout, it);
		fout << std::endl;
	}
	fout.close();
}

template <typename TKey, typename TVal>
void saveFile(const std::filesystem::path& fpPath, const map<TKey, TVal>& mTmp)
{
	if (clrEmpty(fpPath, mTmp))return;
	std::ofstream fout(fpPath);
	for (const auto& [key,val] : mTmp)
	{
		fout << key << "\t";
		fprint(fout, val);
		fout << std::endl;
	}
	fout.close();
}

template <typename TKey, typename TVal>
void saveFile(const std::filesystem::path& fpPath, const unordered_map<TKey, TVal>& mTmp)
{
	if (clrEmpty(fpPath, mTmp))return;
	std::ofstream fout(fpPath);
	for (const auto& [key, val] : mTmp)
	{
		fout << key << "\t";
		fprint(fout, val);
		fout << std::endl;
	}
	fout.close();
}

template <typename T, class C, void(C::* U)(std::ofstream&) const = &C::writeb>
void saveBFile(const std::filesystem::path& fpPath, std::unordered_map<T, C>& m)
{
	if (clrEmpty(fpPath, m))return;
	std::ofstream fout(fpPath, ios::out | ios::trunc | ios::binary);
	const int len = m.size();
	fwrite<int>(fout, len);
	for (auto& [key, val] : m)
	{
		fwrite(fout, key);
		fwrite(fout, val);
	}
	fout.close();
}

template <typename T, class C, void(C::* U)(std::ofstream&) const = &C::writeb>
void saveBFile(const std::filesystem::path& fpPath, std::unordered_map<T, std::shared_ptr<C>>& m)
{
	if (clrEmpty(fpPath, m))return;
	std::ofstream fout(fpPath, ios::out | ios::trunc | ios::binary);
	const int len = m.size();
	fwrite<int>(fout, len);
	for (auto& [key, val] : m)
	{
		fwrite(fout, key);
		val->writeb(fout);
	}
	fout.close();
}

std::string getNativePathString(const std::filesystem::path& fpPath);
std::string cut_stem(std::filesystem::path branch, const std::filesystem::path& main = {});
std::filesystem::path cut_relative(std::filesystem::path branch, const std::filesystem::path& main = {});