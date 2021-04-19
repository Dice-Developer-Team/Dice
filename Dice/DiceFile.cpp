/*
 * 文件读写
 * Copyright (C) 2019-2020 String.Empty
 */
#include <fstream>
#include <filesystem>
#include "DiceMsgSend.h"
#include "ManagerSystem.h"
#include "StrExtern.hpp"
#include "DiceFile.hpp"

int mkDir(const std::string& dir)
{
	std::error_code err;
	std::filesystem::create_directories(dir, err);
	return err.value();
	/*if (_access(dir.c_str(), 0))	return _mkdir(dir.c_str());
	return -2;*/
}

int clrDir(const std::string& dir, const std::unordered_set<std::string>& exceptList)
{
	int nCnt = 0;
	std::error_code err;
	for (const auto& p : std::filesystem::directory_iterator(dir, err))
	{
		if (p.is_regular_file())
		{
			std::string path = convert_w2a(p.path().filename().u16string().c_str());
			if (path.length() >= 36 && !exceptList.count(path))
			{
				std::error_code err2;
				std::filesystem::remove(p.path(), err2);
				if (!err2) nCnt++;
			}
		}
	}
	return (err ? err.value() : nCnt);

	/* int nCnt = 0;
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
	return nCnt;*/
}

vector<string> getLines(const string& s)
{
	vector<string> vLine;
	stringstream ss(s);
	string line;
	while (getline(ss, line))
	{
		int r = line.length();
		while (r && isspace(static_cast<unsigned char>(line[r - 1])))r--;
		int l = 0;
		while (l < r && isspace(static_cast<unsigned char>(line[l])))l++;
		line = line.substr(l, r - l);
		if (line.empty())continue;
		vLine.push_back(std::move(line));
	}
	return vLine;
}

string printLine(string s)
{
	while (s.find('\t') != std::string::npos)s.replace(s.find('\t'), 1, "\\t");
	while (s.find("\r\n") != std::string::npos)s.replace(s.find("\r\n"), 2, "\\n");
	while (s.find('\r') != std::string::npos)s.replace(s.find('\r'), 1, "\\n");
	while (s.find('\n') != std::string::npos)s.replace(s.find('\n'), 1, "\\n");
	return s;
}

bool fscan(std::ifstream& fin, std::string& t)
{
	std::string val;
	if (fin >> val)
	{
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
	return false;
}

[[deprecated]] bool rdbuf(const string& strPath, string& s)
{
	const std::ifstream fin(strPath);
	if (!fin)return false;
	stringstream ss;
	ss << fin.rdbuf();
	s = ss.str();
	return true;
}

bool rdbuf(const std::filesystem::path& fpPath, string& s)
{
	const std::ifstream fin(fpPath);
	if (!fin)return false;
	stringstream ss;
	ss << fin.rdbuf();
	s = ss.str();
	return true;
}

void fprint(std::ofstream& fout, std::string s)
{
	while (s.find(' ') != std::string::npos)s.replace(s.find(' '), 1, "{space}");
	while (s.find('\t') != std::string::npos)s.replace(s.find('\t'), 1, "{tab}");
	while (s.find('\n') != std::string::npos)s.replace(s.find('\n'), 1, "{endl}");
	while (s.find('\r') != std::string::npos)s.replace(s.find('\r'), 1, "{enter}");
	fout << s;
}

void fwrite(ofstream& fout, const std::string& s)
{
	auto len = static_cast<short>(s.length());
	fout.write(reinterpret_cast<char*>(&len), sizeof(short));
	fout.write(s.c_str(), sizeof(char) * s.length());
}
void fwrite(ofstream& fout, const var& val) {
	short idx(-1);
	switch (val.index()) {
	case 0:
		fout.write(reinterpret_cast<char*>(&idx), sizeof(short));
		break;
	case 1: {
		idx = -2;
		fout.write(reinterpret_cast<char*>(&idx), sizeof(short));
		int i = std::get<int>(val);
		fout.write(reinterpret_cast<char*>(&i), sizeof(int));
		break;
	}
	case 2: {
		idx = -3;
		fout.write(reinterpret_cast<char*>(&idx), sizeof(short));
		double f = std::get<double>(val);
		fout.write(reinterpret_cast<char*>(&f), sizeof(double));
		break;
	}
	case 3:
		fwrite(fout, std::get<string>(val));
		break;
	}
}

void readini(ifstream& fin, std::string& s)
{
	std::string line;
	getline(fin, line);
	const int pos = line.find('=');
	if (pos == std::string::npos)return;
	std::istringstream sin(line.substr(pos + 1));
	sin >> s;
}


using namespace std;

std::ifstream& operator>>(std::ifstream& fin, msgtype& t)
{
	fin >> reinterpret_cast<int&>(t);
	return fin;
}

std::ifstream& operator>>(std::ifstream& fin, chatType& ct)
{
	int t;
	fin >> ct.first >> t;
	ct.second = static_cast<msgtype>(t);
	return fin;
}

std::ofstream& operator<<(std::ofstream& fout, const chatType& ct)
{
	fout << ct.first << "\t" << static_cast<int>(ct.second);
	return fout;
}

ifstream& operator>>(ifstream& fin, User& user)
{
	fin >> user.ID >> user.nTrust >> user.tCreated >> user.tUpdated;
	return fin;
}

ofstream& operator<<(ofstream& fout, const User& user)
{
	fout << user.ID << '\t' << user.nTrust << '\t' << user.tCreated << '\t' << user.tUpdated;
	return fout;
}

ifstream& operator>>(ifstream& fin, Chat& grp)
{
	fin >> grp.ID >> grp.isGroup >> grp.inviter >> grp.tCreated >> grp.tUpdated >> grp.tLastMsg;
	return fin;
}

ofstream& operator<<(ofstream& fout, const Chat& grp)
{
	fout << grp.ID << '\t' << grp.isGroup << '\t' << grp.inviter << '\t' << grp.tCreated << '\t' << grp.tUpdated << '\t'
		<< grp.tLastMsg;
	return fout;
}

template <typename T>
[[deprecated]] int _listDir(const string& dir, vector<std::filesystem::path>& files)
{
	int intFile = 0;
	std::error_code err;
	for (const auto& file : T(dir, err))
	{
		if (file.is_regular_file())
		{
			intFile++;
			files.push_back(file.path());
		}
	}
	return err ? -1 : intFile;
}

template <typename T>
int _listDir(const std::filesystem::path& dir, vector<std::filesystem::path>& files)
{
	int intFile = 0;
	std::error_code err;
	for (const auto& file : T(dir, err))
	{
		if (file.is_regular_file() && file.path().filename().string().substr(0, 1) != ".")
		{
			intFile++;
			files.push_back(file.path());
		}
	}
	return err ? -1 : intFile;
}

[[deprecated]] int listDir(const string& dir, vector<std::filesystem::path>& files, bool isSub)
{
	if (isSub)
	{
		return _listDir<std::filesystem::recursive_directory_iterator>(dir, files);
	}
	return _listDir<std::filesystem::directory_iterator>(dir, files);
}

int listDir(const std::filesystem::path& dir, vector<std::filesystem::path>& files, bool isSub)
{
	if (isSub)
	{
		return _listDir<std::filesystem::recursive_directory_iterator>(dir, files);
	}
	return _listDir<std::filesystem::directory_iterator>(dir, files);
}
