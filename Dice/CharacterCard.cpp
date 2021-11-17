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
 * -8:PcNameSetErr 卡名非法更改
 * -11:PcTextTooLong 文本过长
 */


void CardTemp::readt(const DDOM& d) 	{
	for (const auto& node : d.vChild) 		{
		switch (mTempletTag[node.tag]) 			{
		case 2:
			type = node.strValue;
			break;
		case 20:
			readini(node.strValue, replaceName);
			break;
		case 31:
			vBasicList.clear();
			for (const auto& sub : node.vChild) 				{
				vBasicList.push_back(getLines(sub.strValue));
			}
			break;
		case 102:
			for (const auto& sub : getLines(node.strValue)) 				{
				sInfoList.insert(sub);
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
			for (const auto& sub : node.vChild) 				{
				mBuildOption[sub.strValue] = CardBuild(sub);
			}
			break;
		default:
			break;
		}
	}
}

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

void CharaCard::update() {
	Attr["__Update"] = (long long)time(nullptr);
}
void CharaCard::setName(const string& strName) {
	Name = strName;
	Attr["__Name"] = strName;
}
void CharaCard::setType(const string& strType) {
	Attr["__Type"] = strType;
	pTemplet = &getCardTemplet(Attr["__Type"].to_str());
}
int CharaCard::set(string key, int val)
{
	if (key.empty())return -1;
	key = standard(key);
	if (pTemplet->defaultSkill.count(key) && val == pTemplet->defaultSkill.find(key)->second)
	{
		if (Attr.count(key)) Attr.erase(key);
		return -1;
	}
	Attr[key] = val;
	update();
	return 0;
}
int CharaCard::set(const string& key, const string& s) {
	if (key.empty() || s.length() > 255)return -11;
	if (key == "__Name")return -8;
	Attr[key] = s;
	if (key == "__Type")
		pTemplet = &getCardTemplet(s);
	update();
	return 0;
}

int CharaCard::show(string key, string& val) const {
	if (Attr.count(key)) {
		val = Attr.find(key)->second.to_str();
		return 0;
	}
	if (key == "note") {
		val = Note;
		return 2;
	}
	key = standard(key);
	if (Attr.count(key)) {
		val = Attr.find(key)->second.to_str();
		return 0;
	}
	return -1;
}

bool CharaCard::count(const string& strKey) const {
	if (Attr.count(strKey))return true;
	string key{ standard(strKey) };
	return Attr.count(key) || Attr.count("&" + key)
		|| pTemplet->mAutoFill.count(key) || pTemplet->mVariable.count(key)
		|| pTemplet->defaultSkill.count(key);
}
AttrVar& CharaCard::operator[](const string& strKey) {
	if (Attr.count(strKey))return Attr[strKey];
	string key{ standard(strKey) };
	if (!Attr.count(key)) {
		if (pTemplet->mAutoFill.count(key))Attr[key] = cal(pTemplet->mAutoFill.find(key)->second);
		if (pTemplet->defaultSkill.count(key))Attr[key] = pTemplet->defaultSkill.find(key)->second;
	}
	return Attr[key];
}

//求key对应掷骰表达式
string CharaCard::getExp(string& key, std::set<string> sRef){
	sRef.insert(key);
	key = standard(key);
	std::map<string, AttrVar>::const_iterator val = Attr.find("&" + key);
	if (val != Attr.end())return escape(val->second.to_str(), sRef);
	auto exp = pTemplet->mExpression.find(key);
	if (exp != pTemplet->mExpression.end()) return escape(exp->second, sRef);
	val = Attr.find(key);
	if (val != Attr.end())return escape(val->second.to_str(), sRef);
	exp = pTemplet->mVariable.find(key);
	if (exp != pTemplet->mVariable.end())return to_string(cal(exp->second));
	std::map<string, short>::const_iterator def{ pTemplet->defaultSkill.find(key) };
	if (def != pTemplet->defaultSkill.end())return to_string(def->second);
	return "0";
}

void CharaCard::buildv(string para)
{
	std::stack<string> vOption;
	int Cnt;
	vOption.push("_default");
	while ((Cnt = para.rfind(':')) != string::npos)
	{
		vOption.push(para.substr(Cnt + 1));
		para.erase(para.begin() + Cnt, para.end());
	}
	if (!para.empty())vOption.push(para);
	while (!vOption.empty())
	{
		const string para2 = vOption.top();
		vOption.pop();
		build(para2);
	}
	update();
}

void CharaCard::clear() {
	map<string, AttrVar, less_ci> attr_new{ {"__Type",Attr["__Type"]},{"__Name",Attr["__Name"]} };
	Attr.swap(attr_new);
	Note.clear();
}
[[nodiscard]] string CharaCard::show(bool isWhole) const {
	std::set<string> sDefault;
	ResList Res;
	for (const auto& list : pTemplet->vBasicList) {
		ResList subList;
		string strVal;
		for (const auto& it : list) {
			if (!show(it, strVal)) {
				sDefault.insert(it);
				if (it[0] == '&')subList << it + "=" + strVal;
				else if (Attr.find(it)->second.type == AttrVar::AttrType::Text)subList << it + ":【" + strVal + "】";
				else subList << it + ":" + strVal;
			}
		}
		Res << subList.show();
	}
	string strAttrRest;
	for (const auto& [key,val] : Attr) {
		if (sDefault.count(key) || key[0] == '_'
			|| (!isWhole && val.type == AttrVar::AttrType::Text))continue;
		strAttrRest += key + ":" + val.to_str() + (val.type == AttrVar::AttrType::Text ? "\t" : " ");
	}
	Res << strAttrRest;
	if (isWhole && !Note.empty())Res << "====================\n" + Note;
	return Res.show();
}

void CharaCard::writeb(std::ofstream& fout) const {
	fwrite(fout, string("Name"));
	fwrite(fout, Name);
	if (!Attr.empty()) {
		fwrite(fout, string("Attrs"));
		fwrite(fout, Attr);
	}
	if (!Note.empty()) {
		fwrite(fout, string("Note"));
		fwrite(fout, Note);
	}
	fwrite(fout, string("END"));
}
void CharaCard::readb(std::ifstream& fin) {
	string tag = fread<string>(fin);
	while (tag != "END") {
		switch (mCardTag[tag]) {
		case 1:
			setName(fread<string>(fin));
			break;
		case 2:
			Attr["__Type"] = fread<string>(fin);
			break;
		case 3:
			fread(fin, Attr);
			break;
		case 11: {
			std::map<string, short>TempAttr;
			fread(fin, TempAttr);
			TempAttr.erase("");
			for (auto& [key, val] : TempAttr) {
				Attr[key] = val;
			}
		}
			break;
		case 21: {
			std::map<string, string>TempExp;
			fread(fin, TempExp);
			TempExp.erase("");
			for (auto& [key, val] : TempExp) {
				Attr["&" + key] = val;
			}
		}
			break;
		case 102: {
			std::map<string, string>TempInfo;
			fread(fin, TempInfo);
			TempInfo.erase("");
			for (auto& [key, val] : TempInfo) {
				Attr[key] = val;
			}
		}
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
	Name = Attr["__Name"].to_str();
	pTemplet = &getCardTemplet(Attr["__Type"].to_str());
}

void CharaCard::cntRollStat(int die, int face) {
	if (face <= 0 || die <= 0)return;
	string strFace{ to_string(face) };
	string keyStatCnt{ "__StatD" + strFace + "Cnt" };	//掷骰次数
	string keyStatSum{ "__StatD" + strFace + "Sum" };	//掷骰点数和
	string keyStatSqr{ "__StatD" + strFace + "SqrSum" };	//掷骰点数平方和
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	Attr[keyStatCnt] = Attr[keyStatCnt].to_int() + 1;
	Attr[keyStatSum] = Attr[keyStatSum].to_int() + die;
	Attr[keyStatSqr] = Attr[keyStatSqr].to_int() + die * die;
	update();
}
void CharaCard::cntRcStat(int die, int rate) {
	if (rate <= 0 || rate >= 100 || die <= 0 || die > 100)return;
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	Attr["__StatRcCnt"] = Attr["__StatRcCnt"].to_int() + 1;
	if(die <= rate)Attr["__StatRcSumSuc"] = Attr["__StatRcSumSuc"].to_int() + 1;	//实际成功数
	if (die == 1)Attr["__StatRcCnt1"] = Attr["__StatRcCnt1"].to_int() + 1;	//统计出1
	if (die <= 5)Attr["__StatRcCnt5"] = Attr["__StatRcCnt5"].to_int() + 1;	//统计出1-5
	if (die >= 96)Attr["__StatRcCnt96"] = Attr["__StatRcCnt96"].to_int() + 1;	//统计出96-100
	if (die == 100)Attr["__StatRcCnt100"] = Attr["__StatRcCnt100"].to_int() + 1;	//统计出100
	Attr["__StatRcSumRate"] = Attr["__StatRcSumRate"].to_int() + rate;	//总成功率
	update();
}

Player& getPlayer(long long uid)
{
	if (!PList.count(uid))PList[uid] = {};
	return PList[uid];
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
CharaCard& Player::getCard(const string& name, long long group)
{
	if (!name.empty() && mNameIndex.count(name))return mCardList[mNameIndex[name]];
	if (mGroupIndex.count(group))return mCardList[mGroupIndex[group]];
	if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
	return mCardList[0];
}
string Player::listCard() {
	ResList Res;
	for (auto& [idx, pc] : mCardList) {
		Res << "[" + to_string(idx) + "]<" + getCardTemplet(pc.Attr["__Type"].to_str()).type + ">" + pc.getName();
	}
	Res << "default:" + (*this)[0].getName();
	return Res.show();
}


void Player::readb(std::ifstream& fin)
{
	indexMax = fread<short>(fin);
	fread<unsigned short, CharaCard>(fin, mCardList);
	for (const auto& card : mCardList)
	{
		if(!card.second.getName().empty())mNameIndex[card.second.getName()] = card.first;
	}
	mGroupIndex = fread<unsigned long long, unsigned short>(fin);
}

void getPCName(AttrVars& msg)
{
	msg["pc"] = (PList.count(msg["uid"].to_ll()) && PList[msg["uid"].to_ll()][msg["gid"].to_ll()].getName() != "角色卡")
		? PList[msg["uid"].to_ll()][msg["gid"].to_ll()].getName()
		: ((msg["nick"]) ? msg["nick"]
			: msg["nick"] = getName(msg["uid"].to_ll(), msg["gid"].to_ll()));
}
