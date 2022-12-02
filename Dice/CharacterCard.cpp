/*
 * 玩家人物卡
 * Copyright (C) 2019-2022 String.Empty
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

fifo_dict_ci<CardTemp> CharaCard::mCardTemplet{
	{
		"COC7", CardTemp{
			"COC7", SkillNameReplace, BasicCOC7, AutoFillCOC7, mVariableCOC7, ExpressionCOC7,
			SkillDefaultVal, {
				{"_default", CardBuild({BuildCOC7},  {"{随机姓名}"}, {})},
				{
					"bg", CardBuild({
										{"性别", "{性别}"}, {"年龄", "7D6+8"}, {"职业", "{调查员职业}"}, {"个人描述", "{个人描述}"},
										{"重要之人", "{重要之人}"}, {"思想信念", "{思想信念}"}, {"意义非凡之地", "{意义非凡之地}"},
										{"宝贵之物", "{宝贵之物}"}, {"特质", "{调查员特点}"}
									}, {"{随机姓名}"}, {})
				}
			}
		}
	},
	{"BRP", {
			"BRP", {}, {}, {}, {}, {}, {
				{"__DefaultDice",100}
			}, {
				{"_default", CardBuild({},  {"{随机姓名}"}, {})},
				{
					"bg", CardBuild({
										{"性别", "{性别}"}, {"年龄", "7D6+8"}, {"职业", "{调查员职业}"}, {"个人描述", "{个人描述}"},
										{"重要之人", "{重要之人}"}, {"思想信念", "{思想信念}"}, {"意义非凡之地", "{意义非凡之地}"},
										{"宝贵之物", "{宝贵之物}"}, {"特质", "{调查员特点}"}
									}, {"{随机姓名}"}, {})
				}
			}
	}},
	{"DND", {
			"DND", {}, {}, {}, {}, {}, {
				{"__DefaultDice",20}
			}, {
				{"_default", CardBuild({}, {"{随机姓名}"}, {})},
				{
					"bg", CardBuild({
										{"性别", "{性别}"},
									},  {"{随机姓名}"}, {})
				}
			}
	}},
};


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
		case 102:
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

CardTemp& CharaCard::getTemplet()const{
	if (string type{Attr.get_str("__Type")};
		!type.empty() && mCardTemplet.count(type))return mCardTemplet[type]; 
	return mCardTemplet["BRP"];
	
}

void CharaCard::update() {
	Attr["__Update"] = (long long)time(nullptr);
}
void CharaCard::setName(const string& strName) {
	Attr["__Name"] = Name = strName;
}
void CharaCard::setType(const string& strType) {
	Attr["__Type"] = strType;
}
AttrVar CharaCard::get(string key)const {
	if (Attr.has(key)) return Attr.get(key);
	key = standard(key);
	if (Attr.has(key)) return Attr.get(key);
	if (auto& temp{ getTemplet() };temp.defaultSkill.count(key))return getTemplet().defaultSkill.find(key)->second;
	if (getTemplet().mAutoFill.count(key)){
		Attr.set(key, cal(getTemplet().mAutoFill.find(key)->second));
		return Attr.get(key);
	}
	if (getTemplet().mVariable.count(key)){
		return cal(getTemplet().mVariable.find(key)->second);
	}
	return {};
}
int CharaCard::set(string key, const AttrVar& val) {
	if (key.empty())return -1;
	if (key == "__Name")return -8;
	if (val.is_text()&&val.to_str().length()>256)return -11;
	key = standard(key);
	if (getTemplet().defaultSkill.count(key) && val == getTemplet().defaultSkill.at(key)){
		if (Attr.has(key)) Attr.reset(key);
		else return -1;
	}
	else {
		Attr.set(key, val);
	}
	update();
	return 0;
}

int CharaCard::show(string key, string& val) const {
	if (Attr.has(key)) {
		val = Attr.get_str(key);
		return 0;
	}
	key = standard(key);
	if (Attr.has(key)) {
		val = Attr.get_str(key);
		return 0;
	}
	else if (getTemplet().defaultSkill.count(key)) {
		val = to_string(getTemplet().defaultSkill[key]);
		return 0;
	}
	return -1;
}

int CharaCard::call(string key)const {
	if (Attr.has(key))return Attr.get_int(key);
	key = standard(key);
	if (Attr.has(key))return Attr.get_int(key);
	if (auto& templet{ getTemplet() }; templet.mAutoFill.count(key))
	{
		Attr.set(key, cal(templet.mAutoFill.find(key)->second));
		return Attr.get_int(key);
	}
	else if (templet.mVariable.count(key))
	{
		return cal(templet.mVariable.find(key)->second);
	}
	else if (templet.defaultSkill.count(key))return templet.defaultSkill.find(key)->second;
	return 0;
}
bool CharaCard::available(const string& strKey) const {
	if (Attr.has(strKey))return true;
	string key{ standard(strKey) };
	auto& temp{ getTemplet() };
	return Attr.has(key) || Attr.has("&" + key)
		|| temp.mAutoFill.count(key) || temp.mVariable.count(key)
		|| temp.defaultSkill.count(key);
}

//求key对应掷骰表达式
string CharaCard::getExp(string& key, std::set<string> sRef){
	sRef.insert(key);
	key = standard(key);
	auto& temp{ getTemplet() };
	auto val = Attr->find("&" + key);
	if (val != Attr->end())return escape(val->second.to_str(), sRef);
	auto exp = temp.mExpression.find(key);
	if (exp != temp.mExpression.end()) return escape(exp->second, sRef);
	val = Attr->find(key);
	if (val != Attr->end())return escape(val->second.to_str(), sRef);
	exp = temp.mVariable.find(key);
	if (exp != temp.mVariable.end())return to_string(cal(exp->second));
	if (auto def{ temp.defaultSkill.find(key) };
		def != temp.defaultSkill.end())return to_string(def->second);
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
	Attr = AttrObject{ {{"__Type",Attr["__Type"]},{"__Name",Attr["__Name"]}} };
}
[[nodiscard]] string CharaCard::show(bool isWhole) const {
	std::set<string> sDefault;
	ResList Res;
	for (const auto& list : getTemplet().vBasicList) {
		ResList subList;
		string strVal;
		for (const auto& it : list) {
			if (!show(it, strVal)) {
				sDefault.insert(it);
				if (it[0] == '&')subList << it + "=" + strVal;
				//else if (Attr.at(it).type == AttrVar::AttrType::Text)subList << it + ":" + strVal;
				else subList << it + ":" + strVal;
			}
		}
		Res << subList.show();
	}
	string strAttrRest;
	for (const auto& [key,val] : *Attr.to_dict()) {
		if (sDefault.count(key) || key[0] == '_'
			|| (!isWhole && val.type == AttrVar::AttrType::Text))continue;
		strAttrRest += key + ":" + val.to_str() + (val.type == AttrVar::AttrType::Text ? "\t" : " ");
	}
	Res << strAttrRest;
	return Res.show();
}

bool CharaCard::erase(string& key, bool isExp)
{
	if (Attr.has(key)) {
		Attr.reset(key);
		return true;
	}
	key = standard(key);
	if (Attr.has(key)) {
		Attr.reset(key);
		return true;
	}
	else if (Attr.has("&" + key)) {
		Attr.reset("&" + key);
		return true;
	}
	return false;
}
void CharaCard::writeb(std::ofstream& fout) const {
	fwrite(fout, string("Name"));
	fwrite(fout, Name);
	if (!Attr.empty()) {
		fwrite(fout, string("Attrs"));
		Attr.writeb(fout);
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
			Attr.readb(fin);
			break;
		case 11: {
			std::map<string, short>TempAttr;
			fread(fin, TempAttr);
			TempAttr.erase("");
			for (auto& [key, val] : TempAttr) {
				Attr.set(key, val);
			}
		}
			break;
		case 21: {
			std::map<string, string>TempExp;
			fread(fin, TempExp);
			TempExp.erase("");
			for (auto& [key, val] : TempExp) {
				Attr.set("&" + key, val);
			}
		}
			break;
		case 102: {
			std::map<string, string>TempInfo;
			fread(fin, TempInfo);
			TempInfo.erase("");
			for (auto& [key, val] : TempInfo) {
				Attr.set(key, val);
			}
		}
			break;
		case 101:
			Attr["note"] = fread<string>(fin);
			break;
		default:
			break;
		}
		tag = fread<string>(fin);
	}
	Name = Attr.get_str("__Name");
}

void CharaCard::cntRollStat(int die, int face) {
	if (face <= 0 || die <= 0)return;
	string strFace{ to_string(face) };
	string keyStatCnt{ "__StatD" + strFace + "Cnt" };	//掷骰次数
	string keyStatSum{ "__StatD" + strFace + "Sum" };	//掷骰点数和
	string keyStatSqr{ "__StatD" + strFace + "SqrSum" };	//掷骰点数平方和
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	Attr.set(keyStatCnt,Attr.get_int(keyStatCnt) + 1);
	Attr.set(keyStatSum,Attr.get_int(keyStatSum) + die);
	Attr.set(keyStatSqr,Attr.get_int(keyStatSqr) + die * die);
	update();
}
void CharaCard::cntRcStat(int die, int rate) {
	if (rate <= 0 || rate >= 100 || die <= 0 || die > 100)return;
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	Attr["__StatRcCnt"] = Attr["__StatRcCnt"].to_int() + 1;
	if(die <= rate)Attr["__StatRcSumSuc"] = Attr.get_int("__StatRcSumSuc") + 1;	//实际成功数
	if (die == 1)Attr["__StatRcCnt1"] = Attr.get_int("__StatRcCnt1") + 1;	//统计出1
	if (die <= 5)Attr["__StatRcCnt5"] = Attr.get_int("__StatRcCnt5") + 1;	//统计出1-5
	if (die >= 96)Attr["__StatRcCnt96"] = Attr.get_int("__StatRcCnt96") + 1;	//统计出96-100
	if (die == 100)Attr["__StatRcCnt100"] = Attr.get_int("__StatRcCnt100") + 1;	//统计出100
	Attr["__StatRcSumRate"] = Attr.get_int("__StatRcSumRate") + rate;	//总成功率
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
		Res << "[" + to_string(idx) + "]<" + pc.Attr.get_str("__Type") + ">" + pc.getName();
	}
	Res << "default:" + (*this)[0].getName();
	return Res.show();
}


void Player::writeb(std::ofstream& fout) const
{
	fwrite(fout, indexMax);
	fwrite(fout, mCardList);
	fwrite(fout, mGroupIndex);
}
void Player::readb(std::ifstream& fin)
{
	indexMax = fread<short>(fin);
	fread(fin, mCardList);
	for (const auto& card : mCardList)
	{
		if(!card.second.getName().empty())mNameIndex[card.second.getName()] = card.first;
	}
	fread<unsigned long long, unsigned short>(fin, mGroupIndex);
}

AttrVar idx_pc(AttrObject& eve){
	if (eve.has("pc"))return eve["pc"];
	if (!eve.has("uid"))return {};
	long long uid{ eve.get_ll("uid") };
	long long gid{ eve.get_ll("gid") };
	if (PList.count(uid) && PList[uid][gid].getName() != "角色卡")
		return eve["pc"] = PList[uid][gid].getName();
	return idx_nick(eve);
}
