#pragma once

/*
 * 玩家人物卡
 * Copyright (C) 2019-2023 String.Empty
 */

#include <fstream>
#include <utility>
#include <vector>
#include <stack>
#include <mutex>
#include "CQTools.h"
#include "RDConstant.h"
#include "RD.h"
#include "DiceXMLTree.h"
#include "DiceFile.hpp"
#include "ManagerSystem.h"
#include "MsgFormat.h"
#include "CardDeck.h"
#include "DiceEvent.h"
#include "DiceAttrVar.h"

using std::string;
using std::to_string;
using std::vector;
using std::map;

constexpr short NOT_FOUND = -32767;

inline unordered_map<string, short> mTempletTag = {
	{"name", 1},
	{"type", 2},
	{"alias", 20},
	{"basic", 31},
	{"info", 102},
	{"autofill", 22},
	{"variable", 23},
	{"diceexp", 21},
	{"default", 12},
	{"build", 41},
	{"generate", 24},
	{"note", 101},
};
inline unordered_map<string, short> mCardTag = {
	{"Name", 1},
	{"Type", 2},
	{"Attrs", 3},
	{"Attr", 11},	//older
	{"DiceExp", 21},
	{"Note", 101},	//older
	{"Info", 102},	//older
	{"End", 255}
};

//生成模板
class CardBuild
{
public:
	CardBuild() = default;

	CardBuild(const vector<std::pair<string, string>>& attr, const vector<string>& name, const vector<string>& note):
		vBuildList(attr), vNameList(name), vNoteList(note)
	{
	}

	//属性生成
	vector<std::pair<string, string>> vBuildList = {};
	//随机姓名
	vector<string> vNameList = {};
	//Note生成
	vector<string> vNoteList = {};

	CardBuild(const DDOM& d)
	{
		for (const auto& sub : d.vChild)
		{
			switch (mTempletTag[sub.tag])
			{
			case 1:
				vNameList = getLines(sub.strValue);
				break;
			case 24:
				readini(sub.strValue, vBuildList);
				break;
			case 101:
				vNoteList = getLines(sub.strValue);
				break;
			default: break;
			}
		}
	}
};

class CardTemp
{
public:
	string type;
	fifo_dict_ci<> replaceName = {};
	//作成时生成
	vector<vector<string>> vBasicList = {};
	//调用时生成
	fifo_dict_ci<> mAutoFill = {};
	//动态引用
	fifo_dict_ci<> mVariable = {};
	//表达式
	fifo_dict_ci<> mExpression = {};
	//默认值
	fifo_dict_ci<int> defaultSkill = {};
	//生成参数
	fifo_dict_ci<CardBuild> mBuildOption = {};
	CardTemp() = default;

	CardTemp(const string& type, const fifo_dict_ci<>& replace, vector<vector<string>> basic,
		const fifo_dict_ci<>& autofill, const fifo_dict_ci<>& dynamic, const fifo_dict_ci<>& exp,
		const fifo_dict_ci<int>& skill, const fifo_dict_ci<CardBuild>& option) : type(type),
			                                                            replaceName(replace), 
		                                                                vBasicList(basic), 
		                                                                mAutoFill(autofill), 
		                                                                mVariable(dynamic), 
		                                                                mExpression(exp), 
		                                                                defaultSkill(skill), 
		                                                                mBuildOption(option)
	{
	}

	CardTemp(const DDOM& d)
	{
		readt(d);
	}

	void readt(const DDOM& d);

	string getName()
	{
		return type;
	}

	string showItem()
	{
		const string strItem = listKey(mBuildOption);
		if (strItem.empty())return type;
		return type + "[" + strItem + "]";
	}
	string show();
};

struct lua_State;
class CharaCard
{
private:
	string Name = "角色卡";
	std::mutex cardMutex;
public:
	static fifo_dict_ci<CardTemp> mCardTemplet;
	CardTemp& getTemplet()const;
	const string& getName()const { return Name; }
	void setName(const string&);
	void setType(const string&);
	void update();
	//string Type = "COC7";
	AttrObject Attr{ {
		{"__Type",AttrVar("COC7")},
		{"__Update",AttrVar((long long)time(nullptr))},
	} };
	//map<string, string, less_ci> Info{  };
	//map<string, string, less_ci> DiceExp{};

	CharaCard() = default;
	CharaCard(const CharaCard& pc){
		Name = pc.Name;
		Attr = pc.Attr;
	}
	CharaCard& operator=(const CharaCard& pc)
	{
		Name = pc.Name;
		Attr = pc.Attr;
		return *this;
	}

	CharaCard(const string& name, const string& type = "COC7") : Name(name)
	{
		Attr["__Name"] = name;
		setType(type);
	}

	int call(string key)const;

	//表达式转义
	string escape(string exp, const set<string>& sRef)
	{
		if (exp[0] == '&')
		{
			string key = exp.substr(1);
			if (sRef.count(key))return "";
			return getExp(key);
		}
		size_t intCnt = 0, lp, rp;
		while ((lp = exp.find('[', intCnt)) != std::string::npos && (rp = exp.find(']', lp)) != std::string::npos)
		{
			string strProp = exp.substr(lp + 1, rp - lp - 1);
			if (sRef.count(strProp))return "";
			string val = getExp(strProp);
			exp.replace(exp.begin() + lp, exp.begin() + rp + 1, val);
			intCnt = lp + val.length();
		}
		return exp;
	}

	//求key对应掷骰表达式
	string getExp(string& key, set<string> sRef = {});

	bool countExp(const string& key)
	{
		return (Attr.has(key) && Attr.at(key).type == AttrVar::AttrType::Text)
			|| (Attr.has("&" + key))
			|| getTemplet().mExpression.count(key);
	}

	//计算表达式
	int cal(string exp)const
	{
		if (exp[0] == '&')
		{
			string key = exp.substr(1);
			return call(key);
		}
		int intCnt = 0, lp, rp;
		while ((lp = exp.find('[', intCnt)) != std::string::npos && (rp = exp.find(']', lp)) != std::string::npos)
		{
			string strProp = exp.substr(lp + 1, rp - lp - 1);
			const short val = call(strProp);
			exp.replace(exp.begin() + lp, exp.begin() + rp + 1, std::to_string(val));
			intCnt = lp + std::to_string(val).length();
		}
		const RD Res(exp);
		Res.Roll();
		return Res.intTotal;
	}

	void build(const string& para = "")
	{
		const auto it = getTemplet().mBuildOption.find(para);
		if (it == getTemplet().mBuildOption.end())return;
		CardBuild build = it->second;
		for (auto& it2 : build.vBuildList) {
			//exp
			if (it2.first[0] == '&')
			{
				if (Attr.has(it2.first))continue;
				Attr.set(it2.first, it2.second);
			}
				//attr
			else
			{
				if (Attr.has(it2.first))continue;
				Attr.set(it2.first, cal(it2.second));
			}
		}
	}

	//解析生成参数
	void buildv(string para = "");

	[[nodiscard]] string standard(const string& key) const
	{
		if (getTemplet().replaceName.count(key))return getTemplet().replaceName.find(key)->second;
		return key;
	}

	AttrVar get(string key)const;

	int set(string key, const AttrVar& val);

	bool erase(string& key, bool isExp = false);
	void clear();

	int show(string key, string& val) const;

	[[nodiscard]] string show(bool isWhole) const;

	//can get attr by card or temp
	bool available(const string& key) const;

	bool stored(string& key) const
	{
		key = standard(key);
		return Attr.has(key) || getTemplet().mAutoFill.count(key) || getTemplet().defaultSkill.count(key);
	}

	void cntRollStat(int die, int face);

	void cntRcStat(int die, int rate);

	void operator<<(const CharaCard& card)
	{
		const string name = Name;
		*this = card;
		Attr["__Name"] = Name = name;
	}

	void writeb(std::ofstream& fout) const;

	void readb(std::ifstream& fin);
};

class Player
{
private:
	short indexMax = 0;
	map<unsigned short, CharaCard> mCardList;
	map<string, unsigned short> mNameIndex;
	map<unsigned long long, unsigned short> mGroupIndex{{0, 0}};
	// 人物卡互斥
	std::mutex cardMutex;
public:
	Player() {
		mCardList[0] = { "角色卡" };
	}

	Player(const Player& pl)
	{
		*this = pl;
	}

	Player& operator=(const Player& pl)
	{
		indexMax = pl.indexMax;
		mCardList = pl.mCardList;
		mNameIndex = pl.mNameIndex;
		mGroupIndex = pl.mGroupIndex;
		return *this;
	}

	[[nodiscard]] size_t size() const
	{
		return mCardList.size();
	}

	[[nodiscard]] bool count(long long group) const
	{
		return mGroupIndex.count(group);
	}

	[[nodiscard]] bool count(const string& name) const
	{
		return mNameIndex.count(name);
	}

	int newCard(string& s, long long group = 0)
	{
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		//人物卡数量上限
		if (mCardList.size() > 16)return -1;
		string type = "COC7";
		s = strip(s);
		std::stack<string> vOption;
		int Cnt = s.rfind(':');
		if (Cnt != string::npos)
		{
			type = s.substr(0, Cnt);
			s.erase(s.begin(), s.begin() + Cnt + 1);
			if (type == "COC")type = "COC7";
		}
		else if (CharaCard::mCardTemplet.count(s))
		{
			type = s;
			s.clear();
		}
		while ((Cnt = type.rfind(':')) != string::npos)
		{
			vOption.push(type.substr(Cnt + 1));
			type.erase(type.begin() + Cnt, type.end());
		}
		//无效模板不再报错
		//if (!getmCardTemplet().count(type))return -2;
		if (mNameIndex.count(s))return -4;
		if (s.find("=") != string::npos)return -6;
		mCardList.emplace(++indexMax, CharaCard{ s, type });
		CharaCard& card = mCardList[indexMax];
		// CardTemp& temp = mCardTemplet[type];
		while (!vOption.empty())
		{
			string para = vOption.top();
			vOption.pop();
			card.build(para);
			if (card.getName().empty())
			{
				std::vector<string> list = CharaCard::mCardTemplet[type].mBuildOption[para].vNameList;
				while (!list.empty())
				{
					s = CardDeck::draw(list[0]);
					if (mNameIndex.count(s))list.erase(list.begin());
					else
					{
						card.setName(s);
						break;
					}
				}
			}
		}
		if (card.getName().empty())
		{
			std::vector<string> list = CharaCard::mCardTemplet[type].mBuildOption["_default"].vNameList;
			while (!list.empty())
			{
				s = CardDeck::draw(list[0]);
				if (mNameIndex.count(s))list.erase(list.begin());
				else
				{
					card.setName(s);
					break;
				}
			}
			if (card.getName().empty())card.setName(to_string(indexMax + 1));
		}
		s = card.getName();
		mNameIndex[s] = indexMax;
		mGroupIndex[group] = indexMax;
		return 0;
	}

	int buildCard(string& name, bool isClear, long long group = 0)
	{
		string strName = name;
		string strType;
		if (name.find(":") != string::npos)
		{
			strName = strip(name.substr(name.rfind(":") + 1));
			strType = name.substr(0, name.rfind(":"));
		}
		//不存在则新建人物卡
		if (!strName.empty() && !mNameIndex.count(strName))
		{
			if (const int res = newCard(name, group))return res;
			name = getCard(strName, group).getName();
			(*this)[name].buildv();
		}
		else
		{
			name = getCard(strName, group).getName();
			if (isClear)(*this)[name].clear();
			(*this)[name].buildv(strType);
		}
		return 0;
	}

	int changeCard(const string& name, long long group)
	{
		if (name.empty())
		{
			mGroupIndex.erase(group);
			return 1;
		}
		if (!mNameIndex.count(name))return -5;
		mGroupIndex[group] = mNameIndex[name];
		return 0;
	}

	int removeCard(const string& name)
	{
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		if (!mNameIndex.count(name))return -5;
		if (!mNameIndex[name])return -7;
		auto it = mGroupIndex.cbegin();
		while (it != mGroupIndex.cend())
		{
			if (it->second == mNameIndex[name])
			{
				it = mGroupIndex.erase(it);
			}
			else
			{
				++it;
			}
		}
		mCardList.erase(mNameIndex[name]);
		while (!mCardList.count(indexMax))indexMax--;
		mNameIndex.erase(name);
		return 0;
	}

	int renameCard(const string& name, const string& name_new);

	int copyCard(const string& name1, const string& name2, long long group = 0)
	{
		if (name1.empty() || name2.empty())return -3;
		//不存在则新建人物卡
		if (!mNameIndex.count(name1))
		{
			std::lock_guard<std::mutex> lock_queue(cardMutex);
			//人物卡数量上限
			if (mCardList.size() > 16)return -1;
			if (name1.find(":") != string::npos)return -6;
			mCardList[++indexMax].setName(name1);
			mNameIndex[name1] = indexMax;
		}
		(*this)[name1] << (*this)[name2];
		return 0;
	}

	string listCard();

	string listMap()
	{
		ResList Res;
		for (const auto& it : mGroupIndex)
		{
			if (!it.first)Res << "default:" + mCardList[it.second].getName();
			else Res << "(" + to_string(it.first) + ")" + mCardList[it.second].getName();
		}
		return Res.show();
	}

	CharaCard& getCard(const string& name, long long group = 0);

	CharaCard& operator[](long long id)
	{
		if (mGroupIndex.count(id))return mCardList[mGroupIndex[id]];
		if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
		return mCardList[0];
	}

	CharaCard& operator[](const string& name)
	{
		if (mNameIndex.count(name))return mCardList[mNameIndex[name]];
		if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
		return mCardList[0];
	}

	void writeb(std::ofstream& fout) const;

	void readb(std::ifstream& fin);
};

inline map<long long, Player> PList;

Player& getPlayer(long long qq);

AttrVar idx_pc(AttrObject&);
