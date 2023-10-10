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
#include <mutex>
#include "CQTools.h"
#include "RDConstant.h"
#include "RD.h"
#include "tinyxml2.h"
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
using xml = tinyxml2::XMLDocument;
class CharaCard;

inline unordered_map<string, short> mCardTag = {
	{"Name", 1},
	{"Type", 2},
	{"Attrs", 3},
	{"Lock", 103},
	{"Attr", 11},	//older
	{"DiceExp", 21},
	{"Note", 101},	//older
	{"Info", 102},	//older
	{"End", 255}
};

class AttrShape {
public:
	enum class DataType : unsigned char { Any, Nature, Int, };
	enum class TextType : unsigned char { Plain, Dicexp, JavaScript, };
	AttrShape() = default;
	AttrShape(const tinyxml2::XMLElement* node, bool isUTF8);
	AttrShape(int i) :defVal(i) {}
	AttrShape(const string& s):defVal(s){}
	AttrShape(const string& s, TextType tt) :defVal(s), textType(tt){}
	DataType type{ DataType::Any };
	string title;
	TextType textType{ TextType::Plain };
	AttrVar defVal;
	AttrVar init(const CharaCard*);
	int check(AttrVar& val);
	bool equalDefault(const AttrVar& val)const { return TextType::Plain == textType && val == defVal; }
};
using Shapes = dict_ci<AttrShape>;

class CardPreset {
public:
	fifo_dict_ci<AttrShape> shapes;
	CardPreset() = default;
	CardPreset(const std::vector<std::pair<std::string, std::string>>& v) {
		for (auto& [key, val] : v) {
			shapes[key] = { val, AttrShape::TextType::Dicexp };
		}
	}
	CardPreset(const tinyxml2::XMLElement* d, bool isUTF8);
};

class CardTemp
{
public:
	string type;
	fifo_dict_ci<> replaceName = {};
	//作成时生成
	vector<vector<string>> vBasicList = {};
	//元表
	fifo_dict_ci<AttrShape> AttrShapes;
	//表达式
	fifo_dict_ci<> mExpression = {};
	//生成参数
	dict_ci<CardPreset> presets = {};
	CardTemp() = default;

	CardTemp(const string& type, const fifo_dict_ci<>& replace, vector<vector<string>> basic,
		const fifo_dict_ci<>& dynamic, const fifo_dict_ci<>& exp,
		const fifo_dict_ci<int>& def_skill, const dict_ci<CardPreset>& option = {}) : type(type),
			                                                            replaceName(replace), 
		                                                                vBasicList(basic), 
		                                                                mExpression(exp), 
		presets(option)
	{
		for (auto& [attr, exp] : dynamic) {
			AttrShapes[attr] = AttrShape(exp, AttrShape::TextType::Dicexp);
		}
		for (auto& [attr, val] : def_skill) {
			AttrShapes[attr] = val;
		}
	}
	CardTemp& merge(const CardTemp& other);
	bool equalDefault(const string& attr, const AttrVar& val)const { return AttrShapes.count(attr) && AttrShapes.at(attr).equalDefault(val); }

	//CardTemp(const xml* d) { }

	bool canGet(const string& attr)const {
		return AttrShapes.count(attr) && !AttrShapes.at(attr).defVal.is_null();
	}
	string getName() { return type; }

	string showItem()
	{
		const string strItem = listKey(presets);
		if (strItem.empty())return type;
		return type + "[" + strItem + "]";
	}
	string show();
}; 
extern CardTemp ModelBRP;
extern CardTemp ModelCOC7;
extern dict_ci<ptr<CardTemp>> CardModels;
int loadCardTemp(const std::filesystem::path& fpPath, dict_ci<CardTemp>& m);

extern unordered_map<int, string> PlayerErrors;

struct lua_State;
class CharaCard: public AttrObject
{
private:
	string Name = "角色卡";
	unordered_set<string> locks;
	std::mutex cardMutex;
public:
	ptr<CardTemp> getTemplet()const;
	bool locked(const string& key)const { return locks.count(key); }
	bool lock(const string& key) {
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		if(key.empty() || locks.count(key))return false;
		locks.insert(key);
		return true;
	}
	bool unlock(const string& key) {
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		if (key.empty() || !locks.count(key))return false;
		locks.erase(key);
		return true;
	}
	const string& getName()const { return Name; }
	void setName(const string&);
	void setType(const string&);
	void update();
#define Attr (*dict)
	CharaCard(){
		(*dict)["__Type"] = "COC7";
		(*dict)["__Update"] = (long long)time(nullptr);
	}
	CharaCard(const CharaCard& pc){
		Name = pc.Name;
		dict = std::make_shared<AttrVars>(*pc.dict);
	}

	CharaCard(const string& name, const string& type = "COC7") : Name(name)
	{
		(*dict)["__Name"] = name;
		setType(type);
	}

	//int call(string key)const;

	//表达式转义
	string escape(string exp, const unordered_set<string>& sRef)
	{
		if (exp[0] == '&')
		{
			string key = exp.substr(1);
			if (sRef.count(key))return "";
			return getExp(key);
		}
		size_t intCnt = 0, lp, rp;
		while ((lp = exp.find('{', intCnt)) != std::string::npos && (rp = exp.find('}', lp)) != std::string::npos)
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
	string getExp(string& key, unordered_set<string> sRef = {});

	bool countExp(const string& key)const;

	//计算表达式
	std::optional<int> cal(string exp)const;

	void build(const string& para)
	{
		if (const auto it = getTemplet()->presets.find(para);
			it != getTemplet()->presets.end()) {
			auto& preset = it->second;
			for (auto& [attr, shape] : preset.shapes) {
				if (!dict->count(attr) || dict->at(attr).is_null())set(attr, shape.init(this));
			}
		}
	}

	//解析生成参数
	void buildv(string para = "");

	[[nodiscard]] string standard(const string& key) const
	{
		if (auto temp{ getTemplet() }; temp->replaceName.count(key))return temp->replaceName.find(key)->second;
		return key;
	}

	AttrVar get(const string& key)const;

	int set(string key, const AttrVar& val);

	bool erase(string& key);
	void clear();

	std::optional<string> show(string key) const;
	string print(const string& key)const;

	[[nodiscard]] string show(bool isWhole) const;

	bool has(const string& key)const;
	//can get attr by card or temp
	//bool available(const string& key) const;

	bool stored(string& key) const{
		key = standard(key);
		return has(key) || getTemplet()->canGet(key);
	}

	void cntRollStat(int die, int face);

	void cntRcStat(int die, int rate);

	void operator<<(const CharaCard& card){
		dict = std::make_shared<AttrVars>(*card.dict);
		AttrObject::set("__Name", Name);
	}

	void writeb(std::ofstream& fout) const;

	void readb(std::ifstream& fin);
};
using PC = std::shared_ptr<CharaCard>;

class Player
{
private:
	short indexMax = 0;
	map<unsigned short, PC> mCardList;
	dict_ci<unsigned short> mNameIndex;
	unordered_map<unsigned long long, unsigned short> mGroupIndex{{0, 0}};
	// 人物卡互斥
	std::mutex cardMutex;
public:
	Player() {
		mCardList[0] = std::make_shared<CharaCard>( "角色卡" );
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

	int emptyCard(const string& s, long long group, const string& type);
	int newCard(string& s, long long group = 0, string type = "COC7");

	int buildCard(string& name, bool isClear, long long group = 0);

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

	int removeCard(const string& name);

	int renameCard(const string& name, const string& name_new);

	int copyCard(const string& name1, const string& name2, long long group = 0);

	string listCard();

	string listMap()
	{
		ResList Res;
		for (const auto& it : mGroupIndex)
		{
			if (!it.first)Res << "default:" + mCardList[it.second]->getName();
			else Res << "(" + to_string(it.first) + ")" + mCardList[it.second]->getName();
		}
		return Res.show();
	}

	PC getCard(const string& name, long long group = 0);

	PC operator[](long long id)
	{
		if (mGroupIndex.count(id))return mCardList[mGroupIndex[id]];
		if (mCardList.count(id))return mCardList[id];
		if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
		return mCardList[0];
	}

	PC operator[](const string& name)
	{
		if (mNameIndex.count(name))return mCardList[mNameIndex[name]];
		if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
		return mCardList[0];
	}

	void writeb(std::ofstream& fout) const;

	void readb(std::ifstream& fin);
};

extern unordered_map<long long, Player> PList;

Player& getPlayer(long long qq);

AttrVar idx_pc(AttrObject&);
