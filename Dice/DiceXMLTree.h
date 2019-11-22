/*
 * 树状结构
 * Copyright (C) 2019 String.Empty
 * 实际并非真正意义上的XML格式
 */
#pragma once
#include <string>
#include <vector>
using std::string;
using std::vector;

class DDOM {
public:
	string tag;
	DDOM(string& s){
		int intBeginL = s.find('<');
		int intBeginR = s.find('>', intBeginL);
		if (intBeginR == string::npos)return;
		tag = s.substr(intBeginL + 1, intBeginR - intBeginL - 1);
		s = s.substr(intBeginR + 1);
		parse(s);
	}
	vector<DDOM> vChild;
	string strValue;
	void parse(string& s) {
		while (isspace(static_cast<unsigned char>(s[0])))s.erase(s.begin());
		while (isspace(static_cast<unsigned char>(*(s.end() - 1))))s.erase(s.end() - 1);
		int intL, intR;
		while(!s.empty()){
			intL = s.find('<');
			if (intL) {
				strValue += s.substr(0, intL);
				while (isspace(static_cast<unsigned char>(*(strValue.end() - 1))))strValue.erase(strValue.end() - 1);
				s.erase(s.begin(), s.begin() + intL);
			}
			intR = s.find('>');
			if (intR == string::npos) {
				strValue += s;
				s.clear();
				return;
			}
			if (s[1] == '/') {
				s.erase(s.begin(), s.begin() + intR + 1);
				return;
			}
			else{
				vChild.emplace_back(s);
			}
			while (isspace(static_cast<unsigned char>(s[0])))s.erase(s.begin());
		}
	}
};