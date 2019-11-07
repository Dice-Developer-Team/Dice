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
	{"Name",1},
	{"Replace",10},
	{"Build",11},
	{"AutoFill",12},
	{"Dynamic",13},
	{"DiceExp",21},
	{"Default",31},
	{"Note",101}
};
static map<string, short>mCardTag = {
	{"Name",1},
	{"Type",2},
	{"Attr",11},
	{"DiceExp",21},
	{"Note",101},
	{"End",255}
};

class CardTemp {
public:
	//随机姓名
	vector<string>vNameList = {};
	map<string, string>replaceName = {};
	//作成时生成
	vector<std::pair<string, string>>vBuildList = {};
	//调用时生成
	map<string, string>mAutoFill = {};
	//动态引用
	map<string, string>mDynamic = {};
	//表达式
	map<string, string>mExpression = {};
	//默认值
	map<string, short>defaultSkill = {};
	//备选note
	vector<string>vNoteList = {};
	CardTemp() = default;
	CardTemp(vector<string>name,map<string, string>replace, vector<std::pair<string, string>>build, map<string, string>autofill, map<string, string>dynamic, map<string, string>exp, map<string, short>skill, vector<string>note) :vNameList(name),replaceName(replace), vBuildList(build), mAutoFill(autofill), mDynamic(dynamic), mExpression(exp), defaultSkill(skill), vNoteList(note) {}
	void readi(ifstream& fin) {
		string line;
		string tag; 
		while (fin.peek() != EOF) {
			getline(fin, line);
			tag = line.substr(1, line.find(']') - 1);
			switch(mTempletTag[tag]) {
				case 1:
					readini(fin, vNameList);
					break;
				case 10:
					readini(fin, replaceName);
					break;
				case 11:
					readini(fin, vBuildList);
					break;
				case 12:
					readini(fin, mAutoFill);
					break;
				case 13:
					readini(fin, mDynamic);
					break;
				case 21:
					readini(fin, mExpression);
					break;
				case 31:
					readini(fin, defaultSkill);
					break;
				case 101:
					readini(fin, vNoteList);
					break;
				default:
					break;
			}
		}
		return;
	}
};
//人物卡模板列表
//static CardTemp CardCOC7(SkillNameReplace, SkillDefaultVal, CreatingCOC7);
static map<string, CardTemp>mCardTemplet = {
	{"COC7",{CardDeck::mPublicDeck["随机姓名"],SkillNameReplace,BuildCOC7,AutoFillCOC7,mDynamicCOC7,ExpressionCOC7,SkillDefaultVal,CardDeck::mPublicDeck["调查员信息"]}},
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
		else if (pTemplet->mDynamic.count(key)) {
			return cal(pTemplet->mDynamic.find(key)->second);
		}
		else if (pTemplet->defaultSkill.count(key))return pTemplet->defaultSkill.find(key)->second;
		else return 0;
	}
	short getVal(string key)const {
		if (Attr.count(key))return Attr.find(key)->second;
		else if (pTemplet->mDynamic.count(key)) {
			string exp = pTemplet->mDynamic.find(key)->second;
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
	void build() {
		for (auto it : pTemplet->vBuildList) {
			if (Attr.count(it.first))continue;
			Attr[it.first] = cal(it.second);
		}
		if (!Note.empty())return;
		std::vector<string>deck = pTemplet->vNoteList;
		if (!deck.empty())Note = CardDeck::drawCard(deck, true);
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
		if (exp.length() > 64)return -1;
		DiceExp[key] = exp;
		return 0;
	}
	int setNote(string note) {
		if (note.length() > 255)return -11;
		Note = note; 
		scanImage(note, sReferencedImage);
		return 0;
	}
	string erase(string key, bool isExp = false) {
		string strKey = standard(key);
		if (!isExp && Attr.count(strKey)) {
			Attr.erase(strKey);
			return strKey;
		}
		else if (isExp && Attr.count(strKey)) {
			Attr.erase(strKey);
			return strKey;
		}
		return "";
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
		string strRes;
		std::set<string>sDefault;
		for (auto it : pTemplet->vBuildList) {
			if (!Attr.count(it.first))continue;
			sDefault.insert(it.first);
			strRes += it.first + ":" + to_string(Attr.find(it.first)->second) + " ";
		}
		if(sDefault.size()<Attr.size())strRes += "\n";
		for (auto it : Attr) {
			if (sDefault.count(it.first))continue;
			strRes += it.first + ":" + to_string(it.second) + " ";
		}
		for (auto it : DiceExp) {
			strRes += "\n&" + it.first + ":" + it.second + " ";
		}
		if (!Note.empty())strRes += "\n————————————\n" + Note;
		return strRes;
	}
	bool count(string& key)const {
		key = standard(key);
		return Attr.count(key)|| pTemplet->mAutoFill.count(key) || pTemplet->mDynamic.count(key) || pTemplet->defaultSkill.count(key);
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
	int newCard(string s,long long group = 0) {
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		//人物卡数量上限
		if (mCardList.size() > 16)return -1;
		string type = "COC7";
		string name = strip(s);
		int Cnt = s.find(":");
		if (Cnt != string::npos) {
			type = s.substr(0, Cnt);
			name = strip(s.substr(Cnt + 1));
			if (type == "COC")type = "COC7";
		}
		else if (mCardTemplet.count(name)) {
			type = name;
			name.clear();
		}
		//无效模板
		if (!mCardTemplet.count(type))return -2;
		if (name.empty()) {
			std::vector<string>deck = mCardTemplet[type].vNameList;
			while (name.empty() && !deck.empty()) {
				name = CardDeck::drawCard(deck);
				if (mNameIndex.count(name))name.clear();
			} 
			if(name.empty())name = to_string(indexMax + 1);
		}
		if (mNameIndex.count(name))return -4;
		if (name.find(":") != string::npos)return -6;
		mCardList[++indexMax] = CharaCard(name, type);
		mNameIndex[name] = indexMax;
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