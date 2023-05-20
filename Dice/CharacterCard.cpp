/*
 * 玩家人物卡
 * Copyright (C) 2019-2023 String.Empty
 */
#include "CharacterCard.h"
#include "DDAPI.h"

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
 * -21:PcLockKill 删除锁定
 * -22:PcLockName 名称锁定
 * -23:PcLockWrite 禁止写入/修改
 * -24:PcLockRead 禁止查看
 */
unordered_map<int, string> PlayerErrors{
	{-1,"strPcCardFull"},
	{-3,"strPCNameEmpty"},
	{-4,"strPCNameExist"},
	{-5,"strPcNameNotExist"},
	{-6,"strPCNameInvalid"},
	{-7,"strPcInitDelErr"},
	{-21,"strPCLockedKill"},
	{-22,"strPCLockedName"},
	{-23,"strPCLockedWrite"},
	{-24,"strPCLockedRead"},
};

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
	if (string type{ get_str("__Type") };
		!type.empty() && mCardTemplet.count(type))return mCardTemplet[type]; 
	return mCardTemplet["BRP"];
}

void CharaCard::update() {
	(*dict)["__Update"] = (long long)time(nullptr);
}
void CharaCard::setName(const string& strName) {
	Attr["__Name"] = Name = strName;
}
void CharaCard::setType(const string& strType) {
	Attr["__Type"] = strType;
}
AttrVar CharaCard::get(string key)const {
	if (has(key) || has(key = standard(key)))return at(key);
	if (auto& temp{ getTemplet() };temp.defaultSkill.count(key))return getTemplet().defaultSkill.find(key)->second;
	if (getTemplet().mAutoFill.count(key)){
		Attr[key] = cal(getTemplet().mAutoFill.find(key)->second);
		return at(key);
	}
	if (getTemplet().mVariable.count(key)){
		return cal(getTemplet().mVariable.find(key)->second);
	}
	return {};
}
int CharaCard::set(string key, const AttrVar& val) {
	if (key.empty())return -1;
	if (key == "__Name")return -8;
	if (val.is_text() && val.text.length() > 256)return -11;
	key = standard(key);
	if (getTemplet().defaultSkill.count(key) && val == getTemplet().defaultSkill.at(key)){
		if (has(key)) dict->erase(key);
		else return -1;
	}
	else {
		Attr[key] = val;
	}
	update();
	return 0;
}

int CharaCard::show(string key, string& val) const {
	if (has(key) || has(key = standard(key))) {
		val = print(key);
		return 0;
	}
	else if (getTemplet().defaultSkill.count(key)) {
		val = to_string(getTemplet().defaultSkill[key]);
		return 0;
	}
	return -1;
}

int CharaCard::call(string key)const {
	if (has(key))return get_int(key);
	key = standard(key);
	if (has(key))return get_int(key);
	if (auto& templet{ getTemplet() }; templet.mAutoFill.count(key))
	{
		Attr[key] = cal(templet.mAutoFill.find(key)->second);
		return get_int(key);
	}
	else if (templet.mVariable.count(key))
	{
		return cal(templet.mVariable.find(key)->second);
	}
	else if (templet.defaultSkill.count(key))return templet.defaultSkill.find(key)->second;
	return 0;
}
bool CharaCard::available(const string& strKey) const {
	if (has(strKey))return true;
	if (string key{ standard(strKey) }; has(key) || has("&" + key))return true;
	else {
		auto& temp{ getTemplet() };
		return temp.mAutoFill.count(key) || temp.mVariable.count(key)
			|| temp.defaultSkill.count(key);
	}
}

//求key对应掷骰表达式
string CharaCard::getExp(string& key, std::unordered_set<string> sRef){
	sRef.insert(key);
	key = standard(key);
	auto& temp{ getTemplet() };
	auto val = dict->find("&" + key);
	if (val != dict->end())return escape(val->second.to_str(), sRef);
	auto exp = temp.mExpression.find(key);
	if (exp != temp.mExpression.end()) return escape(exp->second, sRef);
	val = dict->find(key);
	if (val != dict->end())return escape(val->second.to_str(), sRef);
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
	dict = std::make_shared<AttrVars>(AttrVars{{"__Type",Attr["__Type"]},{"__Name",Attr["__Name"]}} );
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
				//else if (Attr.at(it).type == AttrVar::Type::Text)subList << it + ":" + strVal;
				else subList << it + ":" + strVal;
			}
		}
		Res << subList.show();
	}
	string strAttrRest;
	for (const auto& [key,val] : *dict) {
		if (sDefault.count(key) || key[0] == '_'
			|| (isWhole && val.type == AttrVar::Type::Number))continue;
		strAttrRest += key + ":" + val.print() + (val.type == AttrVar::Type::Number 
			? " " 
			: (val.type == AttrVar::Type::Text ? "\t" : "\n"));
	}
	Res << strAttrRest;
	return Res.show();
}

bool CharaCard::erase(string& key)
{
	if (has(key)) {
		reset(key);
		goto Update;
	}
	key = standard(key);
	if (has(key)) {
		reset(key);
		goto Update;
	}
	else if (has("&" + key)) {
		reset("&" + key);
		goto Update;
	}
	return false;
Update:
	update();
	return true;
}
void CharaCard::writeb(std::ofstream& fout) const {
	fwrite(fout, string("Name"));
	fwrite(fout, Name);
	fwrite(fout, string("Attrs"));
	AttrObject::writeb(fout);
	if (!locks.empty()) {
		fwrite(fout, string("Lock"));
		fwrite(fout, locks);
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
			AttrObject::readb(fin);
			break;
		case 11: {
			std::unordered_map<string, short>TempAttr;
			fread(fin, TempAttr);
			TempAttr.erase("");
			for (auto& [key, val] : TempAttr) {
				AttrObject::set(key, val);
			}
		}
			break;
		case 21: {
			std::unordered_map<string, string>TempExp;
			fread(fin, TempExp);
			for (auto& [key, val] : TempExp) {
				AttrObject::set("&" + key, val);
			}
		}
			break;
		case 103:
			fread(fin, locks);
			break;
		case 102: {
			std::unordered_map<string, string>TempInfo;
			fread(fin, TempInfo);
			for (auto& [key, val] : TempInfo) {
				AttrObject::set(key, val);
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
	Name = get_str("__Name");
}

void CharaCard::cntRollStat(int die, int face) {
	if (face <= 0 || die <= 0)return;
	string strFace{ to_string(face) };
	string keyStatCnt{ "__StatD" + strFace + "Cnt" };	//掷骰次数
	string keyStatSum{ "__StatD" + strFace + "Sum" };	//掷骰点数和
	string keyStatSqr{ "__StatD" + strFace + "SqrSum" };	//掷骰点数平方和
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	inc(keyStatCnt);
	inc(keyStatSum, die);
	inc(keyStatSqr, die * die);
	update();
}
void CharaCard::cntRcStat(int die, int rate) {
	if (rate <= 0 || rate >= 100 || die <= 0 || die > 100)return;
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	inc("__StatRcCnt");
	if(die <= rate)inc("__StatRcSumSuc");	//实际成功数
	if (die == 1)inc("__StatRcCnt1");	//统计出1
	if (die <= 5)inc("__StatRcCnt5");	//统计出1-5
	if (die >= 96)inc("__StatRcCnt96");	//统计出96-100
	if (die == 100)inc("__StatRcCnt100");	//统计出100
	Attr["__StatRcSumRate"] = get_int("__StatRcSumRate") + rate;	//总成功率
	update();
}
unordered_map<long long, Player> PList;

Player& getPlayer(long long uid)
{
	if (!PList.count(uid))PList[uid] = {};
	return PList[uid];
}
int Player::removeCard(const string& name){
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	if (!mNameIndex.count(name))return -5;
	const auto id = mNameIndex[name];
	if (!id)return -7;
	if (auto pc = mCardList[id]; pc->locked("q")) return -21;
	auto it = mGroupIndex.cbegin();
	while (it != mGroupIndex.cend())
	{
		if (it->second == id)
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
int Player::renameCard(const string& name, const string& name_new) 	{
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	if (name_new.empty())return -3;
	if (mNameIndex.count(name_new))return -4;
	if (name_new.find(":") != string::npos)return -6;
	const int i = mNameIndex[name];
	if (mCardList[i]->locked("n"))return -22;
	mNameIndex[name_new] = mNameIndex[name];
	mNameIndex.erase(name);
	mCardList[i]->setName(name_new);
	return 0;
}
int Player::copyCard(const string& name1, const string& name2, long long group)
{
	if (name1.empty() || name2.empty())return -3;
	//不存在则新建人物卡
	if (!mNameIndex.count(name1))
	{
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		//人物卡数量上限
		if (mCardList.size() > 16)return -1;
		if (name1.find(":") != string::npos)return -6;
		mCardList[++indexMax]->setName(name1);
		mNameIndex[name1] = indexMax;
	}
	*(*this)[name1] << *(*this)[name2];
	return 0;
}
PC Player::getCard(const string& name, long long group)
{
	if (!name.empty() && mNameIndex.count(name))return mCardList[mNameIndex[name]];
	if (mGroupIndex.count(group))return mCardList[mGroupIndex[group]];
	if (mGroupIndex.count(0))return mCardList[mGroupIndex[0]];
	return mCardList[0];
}
int Player::emptyCard(const string& s, long long group, const string& type)
{
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	//人物卡数量上限
	if (mCardList.size() > 16)return -1;
	//无效模板不再报错
	if (mNameIndex.count(s))return -4;
	if (s.find("=") != string::npos)return -6;
	mCardList.emplace(++indexMax, std::make_shared<CharaCard>(s, type));
	PC card{ mCardList[indexMax] };
	mNameIndex[s] = indexMax;
	return 0;
}
int Player::newCard(string& s, long long group, string type)
{
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	//人物卡数量上限
	if (mCardList.size() > 16)return -1;
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
	mCardList.emplace(++indexMax, std::make_shared<CharaCard>(s, type));
	PC card{ mCardList[indexMax] };
	// CardTemp& temp = mCardTemplet[type];
	while (!vOption.empty())
	{
		string para = vOption.top();
		vOption.pop();
		card->build(para);
		if (card->getName().empty())
		{
			std::vector<string> list = CharaCard::mCardTemplet[type].mBuildOption[para].vNameList;
			while (!list.empty())
			{
				s = CardDeck::draw(list[0]);
				if (mNameIndex.count(s))list.erase(list.begin());
				else
				{
					card->setName(s);
					break;
				}
			}
		}
	}
	if (card->getName().empty())
	{
		std::vector<string> list = CharaCard::mCardTemplet[type].mBuildOption["_default"].vNameList;
		while (!list.empty())
		{
			s = CardDeck::draw(list[0]);
			if (mNameIndex.count(s))list.erase(list.begin());
			else
			{
				card->setName(s);
				break;
			}
		}
		if (card->getName().empty())card->setName(to_string(indexMax + 1));
	}
	s = card->getName();
	mNameIndex[s] = indexMax;
	mGroupIndex[group] = indexMax;
	return 0;
}
int Player::buildCard(string& name, bool isClear, long long group)
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
		name = getCard(strName, group)->getName();
		(*this)[name]->buildv();
	}
	else
	{
		name = getCard(strName, group)->getName();
		if (isClear)(*this)[name]->clear();
		(*this)[name]->buildv(strType);
	}
	return 0;
}
string Player::listCard() {
	ResList Res;
	for (auto& [idx, pc] : mCardList) {
		Res << "[" + to_string(idx) + "]<" + pc->get_str("__Type") + ">" + pc->getName();
	}
	Res << "default:" + (*this)[0]->getName();
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
	if (short len = fread<short>(fin); len > 0) while (len--) {
		auto idx = fread<unsigned short>(fin);
		PC card = std::make_shared<CharaCard>();
		card->readb(fin);
		if (auto name{ card->getName() }; !name.empty()) {
			mCardList[idx] = card;
			mNameIndex[name] = idx;
		}
	}
	fread<unsigned long long, unsigned short>(fin, mGroupIndex);
}

AttrVar idx_pc(AttrObject& eve){
	if (eve.has("pc"))return eve["pc"];
	if (!eve.has("uid"))return {};
	long long uid{ eve.get_ll("uid") };
	long long gid{ eve.get_ll("gid") };
	if (PList.count(uid) && PList[uid][gid]->getName() != "角色卡")
		return eve["pc"] = PList[uid][gid]->getName();
	return idx_nick(eve);
}
