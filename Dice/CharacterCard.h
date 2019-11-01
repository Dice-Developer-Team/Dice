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
using std::string;
using std::to_string;
using std::vector;
using std::map;

#define NOT_FOUND -32767

class CardTemp {
public:
	map<string, string>replaceName = {};
	//作成时生成
	vector<std::pair<string, string>>vBuildList = {};
	//调用时生成
	map<string, string>mAutoFill = {};
	//动态引用
	map<string, string>mDynamic = {};
	//默认值
	map<string, short>defaultSkill = {};
	CardTemp() = default;
	CardTemp(map<string, string>replace,vector<std::pair<string, string>>build,map<string,string>autofill, map<string, string>dynamic, map<string, short>skill) :replaceName(replace), vBuildList(build), mAutoFill(autofill),mDynamic(dynamic),defaultSkill(skill) {}
};
//人物卡模板列表
//static CardTemp CardCOC7(SkillNameReplace, SkillDefaultVal, CreatingCOC7);
static map<string, CardTemp>mCardTemplet = {
	{"COC7",{SkillNameReplace,BuildCOC7,AutoFillCOC7,DynamicCOC7,SkillDefaultVal}}
};

class CharaCard {
private:
public:
	string Name = "角色卡";
	string Type = "COC7";
	map<string, short>Attr;
	string Note;
	const CardTemp* pTemplet = &mCardTemplet[Type];
	CharaCard(){}
	CharaCard(string name, string type = "COC7") :Name(name), Type(type){}
	short call(string key) {
		key = standard(key);
		if (Attr.count(key))return Attr[key];
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
	short getVal(string& key) {
		key = standard(key);
		if (Attr.count(key))return Attr[key];
		else if (pTemplet->mAutoFill.count(key)) {
			//key = pTemplet->mAutoFill.find(key)->second;//
			//return cal(key);//
			Attr[key] = cal(pTemplet->mAutoFill.find(key)->second);
			return Attr[key];
		}
		else if (pTemplet->mDynamic.count(key)) {
			//key = pTemplet->mDynamic.find(key)->second;//
			//return cal(key);
			return cal(pTemplet->mDynamic.find(key)->second);
		}
		else if (pTemplet->defaultSkill.count(key))return pTemplet->defaultSkill.find(key)->second;
		else return NOT_FOUND;
	}
	//计算表达式
	short cal(string exp) {
		if (exp[0] == '@') {
			return call(exp.substr(1));
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
	int setNote(string note) {
		if (note.length() > 512)return -11;
		Note = note;
		return 0;
	}
	string erase(string key) {
		string strKey = Attr.count(key) ? key : standard(key);
		if (Attr.count(strKey)) {
			Attr.erase(strKey);
			return strKey;
		}
		return "";
	}
	void clear() {
		Attr.clear();
		Note.clear();
	}
	int show(string key,string& val)const{
		if (key == "note") {
			val = Note;
			return 1;
		}
		else if (Attr.count(key)) {
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
		for (auto it : Attr) {
			if (sDefault.count(it.first))continue;
			strRes += it.first + ":" + to_string(it.second) + " ";
		}
		if (!Note.empty())strRes += "\n————————————\n" + Note;
		return strRes;
	}
	int copy(const CharaCard& pc){
		if (Type != pc.Type)return -8;
		Attr = pc.Attr;
		Note = pc.Note;
		return 0;
	}
	bool count(string& key)const {
		key = standard(key);
		return Attr.count(key)|| pTemplet->mAutoFill.count(key) || pTemplet->mDynamic.count(key) || pTemplet->defaultSkill.count(key);
	}
	bool isStored(string& key)const {
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
		fwrite(fout, Name);
		fwrite(fout, Type);
		fwrite(fout, Attr);
		fwrite(fout, Note);
	}
	void readb(std::ifstream& fin) {
		Name = fread<string>(fin);
		Type = fread<string>(fin);
		Attr = fread<string, short>(fin);
		Note = fread<string>(fin);
	}
};

class Player {
private:
	int indexMax = 0;
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
		if (mCardList.size() > 9)return -1;
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
		if (name.empty())name = to_string(indexMax + 1);
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
		if (!name.empty()&&!mNameIndex.count(strName)) {
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
	int copyCard(string name, long long group = 0) {
		if (name.empty())return -3;
		//不存在则新建人物卡
		if (!mNameIndex.count(name)) {
			std::lock_guard<std::mutex> lock_queue(cardMutex);
			//人物卡数量上限
			if (mCardList.size() > 9)return -1;
			if (name.find(":") != string::npos)return -6;
			mCardList[++indexMax] = (*this)[group];
			mNameIndex[name] = indexMax;
			mCardList[indexMax].Name = name;
			return 0;
		}
		if (int res = (*this)[name].copy((*this)[group]))return res;
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