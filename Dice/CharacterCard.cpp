/*
 * 玩家人物卡
 * Copyright (C) 2019-2021 String.Empty
 */
#include "CharacterCard.h"

/**
 * 错误返回值
 * -1:PcCardFull 角色卡已满
 * -2:PcTempInvalid 模板无效（已弃用）
 * -3:PcNameEmpty 卡名为空
 * -4:PCNameExist 卡名已存在
 * -5:PcNameNotExist 卡名不存在
 * -6:PCNameInvalid 卡名无效
 * -7:PcInitDelErr 删除初始卡
 */


string CardTemp::show() {
	ResList res;
	if (!mVariable.empty()) {
		ResList resVar;
		for (const auto& [key, val] : mVariable) {
			resVar << key;
		}
		res << "动态变量:" + resVar.show();
	}
	if (!mExpression.empty()) {
		ResList resExp;
		for (const auto& [key, val] : mExpression) {
			resExp << key;
		}
		res << "掷骰公式:" + resExp.show();
	}
	return "pc模板:" + type + res.show();
}

CardTemp& getCardTemplet(const string& type)
{
	if (type.empty() || !mCardTemplet.count(type))return mCardTemplet["BRP"];
	return  mCardTemplet[type];
}

void CharaCard::setName(const string& strName) {
	Name = strName;
	Info["__Name"] = strName;
}
void CharaCard::writeb(std::ofstream& fout) const {
	fwrite(fout, string("Name"));
	fwrite(fout, Name);
	if (!Attr.empty()) {
		fwrite(fout, string("Attr"));
		fwrite(fout, Attr);
	}
	if (!Info.empty()) {
		fwrite(fout, string("Info"));
		fwrite(fout, Info);
	}
	if (!DiceExp.empty()) {
		fwrite(fout, string("DiceExp"));
		fwrite(fout, DiceExp);
	}
	if (!Note.empty()) {
		fwrite(fout, string("Note"));
		fwrite(fout, Note);
	}
	fwrite(fout, string("END"));
}
int CharaCard::setInfo(const string& key, const string& s) {
	if (key.empty() || s.length() > 255)return -1;
	Info[key] = s;
	if (key == "__Type")
		pTemplet = &getCardTemplet(s);
	return 0;
}

int CharaCard::show(string key, string& val) const {
	if (Info.count(key)) {
		val = Info.find(key)->second;
		return 3;
	}
	if (key == "note") {
		val = Note;
		return 2;
	}
	if (DiceExp.count(key)) {
		val = DiceExp.find(key)->second;
		return 1;
	}
	key = standard(key);
	if (Attr.count(key)) {
		val = to_string(Attr.find(key)->second);
		return 0;
	}
	return -1;
}

bool CharaCard::count(const string& strKey) const {
	if (Attr.count(strKey))return true;
	string key{ standard(strKey) };
	return Attr.count(key) || DiceExp.count(key) || pTemplet->mAutoFill.count(key) || pTemplet->mVariable.count(key)
		|| pTemplet->defaultSkill.count(key);
}
short& CharaCard::operator[](const string& strKey) {
	if (Attr.count(strKey))return Attr[strKey];
	string key{ standard(strKey) };
	if (!Attr.count(key)) {
		if (pTemplet->mAutoFill.count(key))Attr[key] = cal(pTemplet->mAutoFill.find(key)->second);
		if (pTemplet->defaultSkill.count(key))Attr[key] = pTemplet->defaultSkill.find(key)->second;
	}
	return Attr[key];
}

void CharaCard::clear() {
	Attr.clear();
	map<string, string, less_ci> info_new{ {"__Type",Info["__Type"]},{"__Name",Info["__Name"]} };
	Info.swap(info_new);
	DiceExp.clear();
	Note.clear();
}
[[nodiscard]] string CharaCard::show(bool isWhole) const {
	std::set<string> sDefault;
	ResList Res;
	for (const auto& list : pTemplet->vBasicList) {
		ResList subList;
		string strVal;
		for (const auto& it : list) {
			switch (show(it, strVal)) {
			case 0:
				sDefault.insert(it);
				subList << it + ":" + strVal;
				break;
			case 1:
				sDefault.insert(it);
				subList << "&" + it + "=" + strVal;
				break;
			case 3:
				sDefault.insert(it);
				subList.dot("\t");
				subList << it + ":" + strVal;
				break;
			default:
				continue;
			}
		}
		Res << subList.show();
	}
	string strAttrRest;
	for (const auto& [key,val] : Attr) {
		if (sDefault.count(key) ||
			(key[0] == '_' && (key.length() < 2 || key[1] != '_' || !isWhole)))continue;
		strAttrRest += key + ":" + to_string(val) + " ";
	}
	Res << strAttrRest;
	if (isWhole && !Info.empty())
		for (const auto& it : Info) {
			if (sDefault.count(it.first) || it.first[0] == '_')continue;
			Res << it.first + ":" + it.second;
		}
	if (isWhole && !DiceExp.empty())
		for (const auto& it : DiceExp) {
			if (sDefault.count(it.first) || it.first[0] == '_')continue;
			Res << "&" + it.first + "=" + it.second;
		}
	if (isWhole && !Note.empty())Res << "====================\n" + Note;
	return Res.show();
}
void CharaCard::readb(std::ifstream& fin) {
	string tag = fread<string>(fin);
	while (tag != "END") {
		switch (mCardTag[tag]) {
		case 1:
			setName(fread<string>(fin));
			break;
		case 2:
			Info["__Type"] = fread<string>(fin);
			break;
		case 11:
			fread(fin, Attr);
			Attr.erase("");
			break;
		case 21:
			fread(fin, DiceExp);
			DiceExp.erase("");
			break;
		case 102:
			fread(fin, Info);
			Info.erase("");
			scanImage(Info, sReferencedImage);
			break;
		case 101:
			Note = fread<string>(fin);
			scanImage(Note, sReferencedImage);
			break;
		default:
			break;
		}
		tag = fread<string>(fin);
	}
	pTemplet = &getCardTemplet(Info["__Type"]);
}

Player& getPlayer(long long qq)
{
	if (!PList.count(qq))PList[qq] = {};
	return PList[qq];
}
int Player::renameCard(const string& name, const string& name_new) 	{
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	if (name_new.empty())return -3;
	if (mNameIndex.count(name_new))return -4;
	if (name_new.find(":") != string::npos)return -6;
	const int i = mNameIndex[name_new] = mNameIndex[name];
	mNameIndex.erase(name);
	mCardList[i].setName(name_new);
	return 0;
}
string Player::listCard() {
	ResList Res;
	for (auto& [idx, pc] : mCardList) {
		Res << "[" + to_string(idx) + "]<" + getCardTemplet(pc.Info["__Type"]).type + ">" + pc.getName();
	}
	Res << "default:" + (*this)[0].getName();
	return Res.show();
}

void getPCName(FromMsg& msg)
{
	msg["pc"] = (PList.count(msg.fromQQ) && PList[msg.fromQQ][msg.fromGroup].getName() != "角色卡")
		? PList[msg.fromQQ][msg.fromGroup].getName()
		: ((!msg.strVar.count("nick") || msg["nick"].empty())
		   ? msg["nick"] = getName(msg.fromQQ, msg.fromGroup) : msg["nick"]);
}
