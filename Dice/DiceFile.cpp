/*
 * 文件读写
 * Copyright (C) 2019-2020 String.Empty
 */
#include <fstream>
#include "DiceMsgSend.h"
#include "ManagerSystem.h"

using namespace std;
std::ifstream& operator>>(std::ifstream& fin, CQ::msgtype& t) {
	fin >> (int&)t;
	return fin;
}
std::ifstream& operator>>(std::ifstream& fin, chatType& ct) {
	int t;
	fin >> ct.first >> t;
	ct.second = (CQ::msgtype)t;
	return fin;
}
std::ofstream& operator<<(std::ofstream& fout, const chatType& ct) {
	fout << ct.first << "\t" << (int)ct.second;
	return fout;
}

ifstream& operator>>(ifstream& fin, User& user) {
	fin >> user.ID >> user.nTrust >> user.tCreated >> user.tUpdated;
	return fin;
}
ofstream& operator<<(ofstream& fout, const User& user) {
	fout << user.ID << '\t' << user.nTrust << '\t' << user.tCreated << '\t' << user.tUpdated;
	return fout;
}

ifstream& operator>>(ifstream& fin, Chat& grp) {
	fin >> grp.ID >> grp.isGroup >> grp.inviter >> grp.tCreated >> grp.tUpdated >> grp.tLastMsg;
	return fin;
}
ofstream& operator<<(ofstream& fout, const Chat& grp) {
	fout << grp.ID << '\t' << grp.isGroup <<  '\t' << grp.inviter << '\t' << grp.tCreated << '\t' << grp.tUpdated << '\t' << grp.tLastMsg;
	return fout;
}

int listDir(string dir, set<string>& files, bool isSub, string subdir) {
	_finddata_t file;
	long lf = _findfirst((dir + subdir + "*").c_str(), &file);
	if (lf < 0)return -1;
	int intFile = 0;
	std::set<std::string> dirs;
	do {
		//遍历文件
		if (!strcmp(file.name, ".") || !strcmp(file.name, ".."))continue;
		if (file.attrib == _A_SUBDIR)dirs.insert(file.name);
		else {
			files.insert(subdir + file.name);
			intFile++;
		}
	} while (!_findnext(lf, &file));
	if (isSub)for (auto &it : dirs) {
		listDir(dir, files, true, subdir + it + "\\");
	}
	return intFile;
}