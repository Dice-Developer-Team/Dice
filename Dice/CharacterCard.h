/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Player & Character Card
 * Copyright (C) 2018-2021 w4123溯洄
 * Copyright (C) 2019-2024 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <utility>
#include <stack>
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
#include "DiceJS.h"

using std::string;
using std::to_string;
using std::vector;
using std::map;
using xml = tinyxml2::XMLDocument;
class CharaCard;
class CardTemp;

enum class trigger_time { AfterUpdate };
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
	//string title;
	vector<string> alias;
	TextType textType{ TextType::Plain };
	AttrVar defVal;
	AttrVar init(ptr<CardTemp>, CharaCard*); 
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
struct CardTrigger {
	string name;
	trigger_time time;
	string script;
};
class CardTemp
{
public:
	string type;
	dict_ci<> replaceName = {};
	//作成时生成
	vector<vector<string>> vBasicList = {};
	//元表
	dict_ci<AttrShape> AttrShapes;
	//生成参数
	dict_ci<CardPreset> presets = {};
	//
	ptr<js_context> js_ctx;
	string script;
	multimap<trigger_time, CardTrigger> triggers_by_time;
	fifo_dict_ci<CardTrigger> triggers;
	CardTemp() = default;

	CardTemp(const string& type, const dict_ci<>& replace, vector<vector<string>> basic,
		const dict_ci<>& dynamic, const dict_ci<>& exps,
		const dict_ci<int>& def_skill, const dict_ci<CardPreset>& option = {},
		const string& s = {}) : type(type),
			                                                            replaceName(replace), 
		                                                                vBasicList(basic), 
		presets(option),script(s)
	{
		for (auto& [attr, exp] : dynamic) {
			AttrShapes[attr] = AttrShape(exp, AttrShape::TextType::Dicexp);
		}
		for (auto& [attr, exp] : exps) {
			AttrShapes["&" + attr] = AttrShape(exp, AttrShape::TextType::Plain);
		}
		for (auto& [attr, val] : def_skill) {
			AttrShapes[attr] = val;
		}
	}
	CardTemp& merge(const CardTemp& other);
	void init();
	void after_update(const ptr<AnysTable>&);
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
class CharaCard: public AnysTable
{
private:
	string Name = "角色卡";
	unordered_set<string> locks;
	std::mutex cardMutex;
public:
	MetaType getType()const override{ return MetaType::Actor; }
	ptr<CardTemp> getTemplet()const;
	bool locked(const string& key)const { return locks.count(key); }
	bool lock(const string& key) {
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		if (key.empty() || locks.count(key))return false;
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
	string print()const override{ return Name; }
	void setName(const string&);
	void setType(const string&);
	void update();
	CharaCard(){
		dict["__Type"] = "COC7";
		dict["__Update"] = (long long)time(nullptr);
	}
	CharaCard(const CharaCard& pc){
		Name = pc.Name;
		dict = pc.dict;
	}

	CharaCard(const string& name, const string& type = "COC7") : Name(name)
	{
		dict["__Name"] = name;
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
	std::optional<int> cal(string exp);

	void build(const string& para);

	//解析生成参数
	void buildv(string para = "");

	[[nodiscard]] string standard(const string& key) const
	{
		if (auto temp{ getTemplet() };
			!key.empty() && temp->replaceName.count(key))
			return temp->replaceName.find(key)->second;
		return key;
	}

	AttrVar get(const string& key, const AttrVar& val = {})const override;

	int set(string key, const AttrVar& val);

	bool erase(string& key);
	void clear();

	std::optional<string> show(string key);
	string print(const string& key);

	[[nodiscard]] string show(bool isWhole);

	bool has(const string& key)const override;
	//can get attr by card or temp
	//bool available(const string& key) const;

	bool hasAttr(string& key) const;

	void cntRollStat(int die, int face);

	void cntRcStat(int die, int rate);

	void operator<<(const CharaCard& card){
		dict = card.dict;
		dict["__Name"] = Name;
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

	string listCard() const;

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

	PC getCardByID(long long id) const;
	PC operator[](long long id) const {
		return getCardByID(id);
	}

	PC operator[](const string& name) const;

	void writeb(std::ofstream& fout) const;

	void readb(std::ifstream& fin);
};

extern unordered_map<long long, Player> PList;

Player& getPlayer(long long qq);

AttrVar idx_pc(const AttrObject&);
