#pragma once

/*
 * 树状结构
 * Copyright (C) 2019-2020 String.Empty
 * 实际并非真正意义上的XML格式
 */

#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;

class DDOM
{
public:
	string tag;

	DDOM(string& s)
	{
		const int intBeginL = s.find('<');
		const int intBeginR = s.find('>', intBeginL);
		if (intBeginR == string::npos)return;
		tag = s.substr(intBeginL + 1, intBeginR - intBeginL - 1);
		s = s.substr(intBeginR + 1);
		parse(s);
	}

	DDOM(string key, string val): tag(std::move(key)), strValue(std::move(val))
	{
	}

	vector<DDOM> vChild{};
	std::map<string, size_t> mChild{};
	string strValue{};

	static string printTab(unsigned short cnt)
	{
		string s;
		while (cnt--)s += "\t";
		return s;
	}

	void push(DDOM dom)
	{
		mChild.insert({dom.tag, vChild.size()});
		vChild.push_back(std::move(dom));
	}

	[[nodiscard]] bool count(const string& key) const
	{
		return mChild.count(key);
	}

	DDOM& operator[](const string& key)
	{
		return vChild[mChild[key]];
	}

	void parse(string& s)
	{
		while (isspace(static_cast<unsigned char>(s[0])))s.erase(s.begin());
		while (isspace(static_cast<unsigned char>(*(s.end() - 1))))s.erase(s.end() - 1);
		while (!s.empty())
		{
			const int intL = s.find('<');
			if (intL)
			{
				strValue += s.substr(0, intL);
				while (isspace(static_cast<unsigned char>(*(strValue.end() - 1))))strValue.erase(strValue.end() - 1);
				s.erase(s.begin(), s.begin() + intL);
			}
			const int intR = s.find('>');
			if (intR == string::npos)
			{
				strValue += s;
				s.clear();
				return;
			}
			if (s[1] == '/')
			{
				s.erase(s.begin(), s.begin() + intR + 1);
				return;
			}
			push(DDOM(s));
			while (isspace(static_cast<unsigned char>(s[0])))s.erase(s.begin());
		}
	}

	string dump(int nTab = 0)
	{
		string res;
		res += printTab(nTab) + '<' + tag + '>';
		if (vChild.empty() && strValue.find('\n') == string::npos)return res + strValue + "</" + tag + '>';
		if (!strValue.empty())res += "\n" + printTab(nTab) + strValue;
		for (auto child : vChild)
		{
			res += "\n" + child.dump(nTab + 1);
		}
		res += "\n" + printTab(nTab) + "</" + tag + '>';
		return res;
	}
};
