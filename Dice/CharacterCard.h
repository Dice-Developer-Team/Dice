/*
 * 玩家人物卡
 * Copyright (C) 2019 String.Empty
 */
#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <mutex>
#include "CQTools.h"
#include "Unpack.h"
#include "RD.h"
#include "NameStorage.h"
#include "CardDeck.h"
using std::string;
using std::to_string;
using std::vector;
using std::map;

#define NOT_FOUND -32767

static map<string, short>mTempletTag = {
	{"name",1},
	{"type",2},
	{"alias",20},
	{"basic",31},
	{"autofill",22},
	{"variable",23},
	{"diceexp",21},
	{"default",12},
	{"build",41},
	{"generate",24},
	{"note",101},
};
static map<string, short>mCardTag = {
	{"Name",1},
	{"Type",2},
	{"Attr",11},
	{"DiceExp",21},
	{"Note",101},
	{"End",255}
};
//生成模板
class CardBuild {
public:
	CardBuild() = default;
	CardBuild(vector<std::pair<string, string>>attr,vector<string>name, vector<string>note):vBuildList(attr), vNameList(name), vNoteList(note) {}
	//属性生成
	vector<std::pair<string, string>>vBuildList = {};
	//随机姓名
	vector<string>vNameList = {};
	//Note生成
	vector<string>vNoteList = {};
	CardBuild(DDOM d) {
		for (auto sub : d.vChild) {
			switch (mTempletTag[sub.tag]) {
			case 1:
				vNameList = getLines(sub.strValue);
				break;
			case 24:
				readini(sub.strValue, vBuildList);
				break;
			case 101:
				vNoteList = getLines(sub.strValue);
				break;
			default:break;
			}
		}
	}
};
class CardTemp {
public:
	string type;
	map<string, string>replaceName = {};
	//作成时生成
	vector<vector<string>>vBasicList = {};
	//调用时生成
	map<string, string>mAutoFill = {};
	//动态引用
	map<string, string>mVariable = {};
	//表达式
	map<string, string>mExpression = {};
	//默认值
	map<string, short>defaultSkill = {};
	//生成参数
	map<string, CardBuild>mBuildOption = {};
	CardTemp() = default;
	CardTemp(string type, map<string, string>replace, vector<vector<string>>basic, map<string, string>autofill, map<string, string>dynamic, map<string, string>exp, map<string, short>skill, map<string, CardBuild> option) :type(type),replaceName(replace), vBasicList(basic), mAutoFill(autofill), mVariable(dynamic), mExpression(exp), defaultSkill(skill), mBuildOption(option) {}
	CardTemp(DDOM d) {
		readt(d);
	}
	void readt(DDOM d) {
		for (auto node : d.vChild) {
			switch (mTempletTag[node.tag]) {
			case 2:
				type = node.strValue;
				break;
			case 20:
				readini(node.strValue, replaceName);
				break;
			case 31:
				for (auto sub : node.vChild) {
					vBasicList.push_back(getLines(sub.strValue));
				}
				break;
			case 22:
				readini(node.strValue, mAutoFill);
				break;
			case 23:
				readini(node.strValue, mVariable);
				break;
			case 21:
				readini(node.strValue, mExpression);
				break;
			case 12:
				readini(node.strValue, defaultSkill);
				break;
			case 41:
				for (auto sub : node.vChild) {
					mBuildOption[sub.strValue] = CardBuild(sub);
				}
				break;
			default:
				break;
			}
		}
	}
	string getName() {
		return type;
	}
};


static map<string, CardTemp>mCardTemplet = {
	{"COC7",{"COC7",SkillNameReplace,BasicCOC7,AutoFillCOC7,mVariableCOC7,ExpressionCOC7,SkillDefaultVal,{
		{"",CardBuild({BuildCOC7},CardDeck::mPublicDeck["随机姓名"],CardDeck::mPublicDeck["调查员信息"])}
	}}}
};

class CharaCard {
private:
public:
	string Name = "角色卡";
	string Type = "COC7";
	map<string, short>Attr;
	map<string, string>DiceExp;
	string Note;
	const CardTemp* pTemplet = &mCardTemplet[Type];
	CharaCard(){}
	CharaCard(string name, string type = "COC7") :Name(name), Type(type){}
	short call(string &key){
		key = standard(key);
		if (Attr.count(key))return Attr.find(key)->second;
		else if (pTemplet->mAutoFill.count(key)){
			Attr[key] = cal(pTemplet->mAutoFill.find(key)->second);
			return Attr[key];
		}
		else if (pTemplet->mVariable.count(key)) {
			return cal(pTemplet->mVariable.find(key)->second);
		}
		else if (pTemplet->defaultSkill.count(key))return pTemplet->defaultSkill.find(key)->second;
		else return 0;
	}
	short getVal(string key)const {
		if (Attr.count(key))return Attr.find(key)->second;
		else if (pTemplet->mVariable.count(key)) {
			string exp = pTemplet->mVariable.find(key)->second;
			RD Res(exp);
			Res.Roll();
			return Res.intTotal;
		}
		else if (pTemplet->defaultSkill.count(key))return pTemplet->defaultSkill.find(key)->second;
		else return 0;
	}
	//表达式转义
	string escape(string exp) const{
		if (exp[0] == '&') {
			string key = exp.substr(1);
			return getExp(key);
		}
		int intCnt = 0, lp = 0, rp = 0;
		while ((lp = exp.find('[', intCnt)) != std::string::npos && (rp = exp.find(']', lp)) != std::string::npos) {
			string strProp = exp.substr(lp + 1, rp - lp - 1);
			string val = count(strProp) ? to_string(getVal(strProp)) : getExp(strProp);
			exp.replace(exp.begin() + lp, exp.begin() + rp + 1, val);
			intCnt = lp + val.length();
		}
		return exp;
	}
	//求key对应掷骰表达式
	string getExp(string& key) const {
		auto exp = DiceExp.find(key);
		if (exp != DiceExp.end()) {
			return escape(exp->second);
		}
		exp = pTemplet->mExpression.find(key);
		if (exp != pTemplet->mExpression.end()) {
			return escape(exp->second);
		}
		return "0";
	}
	bool countExp(string key) {
		return DiceExp.count(key) || pTemplet->mExpression.count(key);
	}
	//计算表达式
	short cal(string exp){
		if (exp[0] == '&') {
			string key = exp.substr(1);
			return call(key);
		}
		int intCnt = 0, lp = 0, rp = 0;
		while ((lp = exp.find('[', intCnt)) != std::string::npos && (rp = exp.find(']', lp)) != std::string::npos) {
			string strProp = exp.substr(lp + 1, rp - lp - 1);
			short val = call(strProp);
			exp.replace(exp.begin() + lp, exp.begin() + rp + 1, std::to_string(val));
			intCnt = lp + std::to_string(val).length();
		}
		RD Res(exp);
		Res.Roll();
		return Res.intTotal;
	}
	void build(string para = "") {
		auto it = pTemplet->mBuildOption.find(para);
		if (it == pTemplet->mBuildOption.end())return;
		CardBuild build = it->second;
		while (Name.empty() && !build.vNameList.empty()) {
			Name = CardDeck::drawCard(build.vNameList);
		}
		for (auto it : build.vBuildList) {
			if (Attr.count(it.first))continue;
			Attr[it.first] = cal(it.second);
		}
		if (Note.empty() && !build.vNoteList.empty()) {
			setNote(CardDeck::drawCard(build.vNoteList));
		}
	}
	void rebuild() {
		clear();
		build();
	}
	string standard(string key)const{
		if (pTemplet->replaceName.count(key))return pTemplet->replaceName.find(key)->second;
		return key;
	}
	int set(string key, short val) {
		if (val == pTemplet->defaultSkill.find(key)->second) {
			Attr.erase(key);
			return -1;
		}
		Attr[key] = val;
		return 0;
	}
	int setExp(string key, string exp) {
		if (exp.length() > 63)return -1;
		DiceExp[key] = exp;
		return 0;
	}
	int setNote(string note) {
		if (note.length() > 255)return -11;
		Note = note; 
		scanImage(note, sReferencedImage);
		return 0;
	}
	bool erase(string& key, bool isExp = false) {
		string strKey = standard(key);
		if (!isExp && Attr.count(strKey)) {
			Attr.erase(strKey);
			key = strKey;
			return true;
		}
		else if (isExp && DiceExp.count(key)) {
			DiceExp.erase(key);
			return true;
		}
		return false;
	}
	void clear() {
		Attr.clear();
		DiceExp.clear();
		Note.clear();
	}
	int show(string key,string& val)const{
		if (key == "note") {
			val = Note;
			return 2;
		}
		if (DiceExp.count(key)) {
			val = to_string(Attr.find(key)->second);
			return 1;
		}
		key = standard(key);
		if (Attr.count(key)) {
			val = to_string(Attr.find(key)->second);
			return 0;
		}
		else{
			return -1;
		}
	}
	string show()const{
		std::set<string>sDefault; 
		ResList Res;
		for (auto list : pTemplet->vBasicList) {
			ResList subList;
			subList.setDot("\t");
			for (auto it : list) {
				if (!Attr.count(it))continue;
				sDefault.insert(it);
				subList << it + ":" + to_string(Attr.find(it)->second) + " ";
			}
			Res << subList.show();
		}
		string strAttrRest;
		for (auto it : Attr) {
			if (sDefault.count(it.first))continue;
			strAttrRest += it.first + ":" + to_string(it.second) + " ";
		}
		Res << strAttrRest;
		for (auto it : DiceExp) {
			Res << "&" + it.first + ":" + it.second + " ";
		}
		if (!Note.empty())Res << "————————————\n" + Note;
		return Res.show();
	}
	bool count(string& key)const {
		key = standard(key);
		return Attr.count(key)|| pTemplet->mAutoFill.count(key) || pTemplet->mVariable.count(key) || pTemplet->defaultSkill.count(key);
	}
	bool stored(string& key)const {
		key = standard(key);
		return Attr.count(key) || pTemplet->defaultSkill.count(key);
	}
	short& operator[](string& key){
		key = standard(key);
		if (!Attr.count(key)&&pTemplet->mAutoFill.count(key)) {
			Attr[key] = cal(pTemplet->mAutoFill.find(key)->second);
		}
		return Attr[key];
	}
	void writeb(std::ofstream& fout) {
		fwrite(fout, (string)"Name");
		fwrite(fout, Name);
		fwrite(fout, (string)"Type");
		fwrite(fout, Type);
		if (!Attr.empty()) {
			fwrite(fout, (string)"Attr");
			fwrite(fout, Attr);
		}
		if (!DiceExp.empty()) {
			fwrite(fout, (string)"DiceExp");
			fwrite(fout, DiceExp);
		}
		if (!Note.empty()) {
			fwrite(fout, (string)"Note");
			fwrite(fout, Note);
		}
		fwrite(fout, (string)"END");
	}
	void readb(std::ifstream& fin) {
		string tag = fread<string>(fin);
		while (tag != "END") {
			switch (mCardTag[tag]) {
			case 1:
				Name = fread<string>(fin);
				break;
			case 2:
				Type = fread<string>(fin);
				break;
			case 11:
				Attr = fread<string, short>(fin);
				break;
			case 21:
				DiceExp = fread<string, string>(fin);
				break;
			case 101:
				Note = fread<string>(fin);
				scanImage(Note, sReferencedImage);
				break;
			default:
				break;
				break;
			}
			tag = fread<string>(fin);
		}
		pTemplet = mCardTemplet.count(Type) ? &mCardTemplet[Type] : &mCardTemplet["COC7"];
	}
};

class Player {
private:
	short indexMax = 0;
	map<unsigned short, CharaCard>mCardList;
	map<string, unsigned short>mNameIndex;
	map<unsigned long long, unsigned short>mGroupIndex;
	// 人物卡互斥
	std::mutex cardMutex;
public:
	Player() {
		mCardList = { {0,{"角色卡"}} };
		mGroupIndex = { {0,0} };
	}
	Player(const Player& pl) {
		*this = pl;
	}
	Player& operator=(const Player& pl) {
		indexMax = pl.indexMax;
		mCardList = pl.mCardList;
		mNameIndex = pl.mNameIndex;
		mGroupIndex = pl.mGroupIndex;
		return *this;
	}
	bool count(long long group)const {
		return mGroupIndex.count(group);
	}
	bool count(string name)const {
		return mNameIndex.count(name);
	}
	int newCard(string& s,long long group = 0) {
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		//人物卡数量上限
		if (mCardList.size() > 16)return -1;
		string type = "COC7";
		s = strip(s);
		std::stack<string> vOption;
		int Cnt = s.rfind(":");
		if (Cnt != string::npos) {
			type = s.substr(0, Cnt);
			s.erase(s.begin(), s.begin() + Cnt + 1);
			if (type == "COC")type = "COC7";
		}
		else if (mCardTemplet.count(s)) {
			type = s;
			s.clear();
		}
		while ((Cnt = type.rfind(':')) != string::npos) {
			vOption.push(type.substr(Cnt + 1));
			type.erase(type.begin() + Cnt, type.end());
		}
		//无效模板
		if (!mCardTemplet.count(type))return -2;
		if (mNameIndex.count(s))return -4;
		if (s.find("=") != string::npos)return -6;
		mCardList[++indexMax] = CharaCard(s, type);
		CharaCard& card = mCardList[indexMax];
		CardTemp& temp = mCardTemplet[type];
		while(!vOption.empty()) {
			string para = vOption.top();
			vOption.pop();
			card.build(para);
			if (mNameIndex.count(card.Name))card.Name.clear();
		}
		s = card.Name;
		if (s.empty()) {
			std::vector<string>deck = mCardTemplet[type].mBuildOption[""].vNameList;
			while (s.empty() && !deck.empty()) {
				s = CardDeck::drawCard(deck);
				if (mNameIndex.count(s))s.clear();
			}
			if (s.empty())s = to_string(indexMax + 1);
			card.Name = s;
		}
		mNameIndex[s] = indexMax;
		mGroupIndex[group] = indexMax;
		return 0;
	}
	int buildCard(string name, long long group = 0) {
		string strName = (name.find(":") != string::npos) ? strip(name.substr(name.find(":") + 1)) : name;
		//不存在则新建人物卡
		if (!name.empty() && !mNameIndex.count(strName)) {
			if (int res = newCard(name, group))return res;
		}
		getCard(strName, group).build();
		return 0;
	}
	int rebuildCard(string name, long long group = 0) {
		string strName = (name.find(":") != string::npos) ? strip(name.substr(name.find(":") + 1)) : name;
		//不存在则新建人物卡
		if (!name.empty() && !mNameIndex.count(strName)) {
			if (int res = newCard(name, group))return res;
		}
		getCard(strName, group).build();
		return 0;
	}
	int changeCard(string name, long long group) {
		if (name.empty()) {
			mGroupIndex.erase(group);
			return 1;
		}
		if (!mNameIndex.count(name))return -5;
		mGroupIndex[group] = mNameIndex[name];
		return 0;
	}
	int removeCard(string name) {
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		if (!mNameIndex.count(name))return -5;
		if (!mNameIndex[name])return -7;
		for (auto it : mGroupIndex) {
			if (it.second == mNameIndex[name])mGroupIndex.erase(it.first);
		}
		mCardList.erase(mNameIndex[name]);
		while(!mCardList.count(indexMax))indexMax--;
		mNameIndex.erase(name);
		return 0;
	}
	int renameCard(string name, string name_new) {
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		if (mNameIndex.count(name_new))return -4;
		if (name_new.find(":") != string::npos)return -6;
		int i = mNameIndex[name_new] = mNameIndex[name];
		mNameIndex.erase(name);
		mCardList[i].Name = name_new;
		return 0;
	}
	int copyCard(string name1, string name2, long long group = 0) {
		if (name1.empty() || name2.empty())return -3;
		//不存在则新建人物卡
		if (!mNameIndex.count(name1)) {
			std::lock_guard<std::mutex> lock_queue(cardMutex);
			//人物卡数量上限
			if (mCardList.size() > 16)return -1;
			if (name1.find(":") != string::npos)return -6;
			mCardList[++indexMax] = (*this)[name2];
			mNameIndex[name1] = indexMax;
			mCardList[indexMax].Name = name1;
			return 0;
		}
		CharaCard& pc1 = (*this)[name1];
		pc1 = (*this)[name2];
		pc1.Name = name1;
		return 0;
	}
	string listCard(){
		string strRes;
		for (auto it : mCardList) {
			strRes += "[" + to_string(it.first) + "]" + it.second.Name + "\n";
		}
		strRes += "default:" + (*this)[0].Name;
		return strRes;
	}
	string listMap() {
		string strRes;
		for (auto it : mGroupIndex) {
			if (!it.first)strRes += "（默认）" + mCardList[it.second].Name;
			else strRes += "\n(" + to_string(it.first) + ")" + mCardList[it.second].Name;
		}
		return strRes;
	}
	CharaCard& getCard(string name, long long group = 0) {
		if (mNameIndex.count(name))return mCardList[mNameIndex[name]];
		else if (mGroupIndex.count(group))return mCardList[mGroupIndex[group]];
		else if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
		else return mCardList[0];
	}
	CharaCard& operator[](long long id){
		if (mGroupIndex.count(id))return mCardList[mGroupIndex[id]];
		else if(mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
		else return mCardList[0];
	}
	CharaCard& operator[](string name) {
		if (mNameIndex.count(name))return mCardList[mNameIndex[name]];
		else if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
		else return mCardList[0];
	}
	bool load(std::ifstream& fin) {
		string s;
		if (!(fin >> s))return false;
		Unpack pack(base64_decode(s));
		indexMax = pack.getInt();
		Unpack cards = pack.getUnpack();
		while (cards.len() > 0) {
			Unpack card = cards.getUnpack();
			int nIndex = card.getInt();
			string name = card.getstring();
			string type = card.getstring();
			mCardList[nIndex] = CharaCard(name, type);
			mNameIndex[name] = nIndex;
			Unpack skills = card.getUnpack();
			string skillname;
			while (skills.len() > 0) {
				skillname = skills.getstring();
				mCardList[nIndex][skillname] = skills.getInt();
			}
			mCardList[nIndex].Note = card.getstring();
		}
		Unpack groups = pack.getUnpack();
		while (groups.len() > 0) {
			mGroupIndex[groups.getLong()] = groups.getInt();
		}
		return true;
	}
	void save(std::ofstream& fout) {
		Unpack pack;
		pack.add(indexMax);
		Unpack cards;
		for (auto it : mCardList) {
			Unpack card;
			card.add(it.first);
			card.add(it.second.Name);
			card.add(it.second.Type);
			Unpack skills;
			for (auto skill : it.second.Attr) {
				skills.add(skill.first);
				skills.add(skill.second);
			}
			card.add(skills);
			card.add(it.second.Note);
			cards.add(card);
		}
		pack.add(cards);
		Unpack groups;
		for (auto it : mGroupIndex) {
			groups.add((long long)it.first);
			groups.add(it.second);
		}
		pack.add(groups);
		fout << base64_encode(pack.getAll());
	}
	void writeb(std::ofstream& fout) {
		fwrite(fout, indexMax);
		fwrite(fout, mCardList);
		fwrite(fout, mGroupIndex);
	}
	void readb(std::ifstream& fin) {
		indexMax = fread<short>(fin);
		mCardList = fread<unsigned short,CharaCard>(fin);
		for (auto card : mCardList) {
			mNameIndex[card.second.Name] = card.first;
		}
		mGroupIndex = fread<unsigned long long, unsigned short>(fin);
	}
};

static map<long long, Player>PList;

Player& getPlayer(long long qq) {
	if (!PList.count(qq))PList[qq] = {};
	return PList[qq];
}

string getPCName(long long qq, long long group) {
	if (PList.count(qq) && PList[qq][group].Name != "角色卡")return PList[qq][group].Name;
	return getName(qq, group);
}