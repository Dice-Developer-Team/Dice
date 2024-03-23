/*
 * 玩家人物卡
 * Copyright (C) 2019-2024 String.Empty
 */
#include "CharacterCard.h"
#include "DDAPI.h"
#include "quickjs.h"
#include "DiceJS.h"
#include "DiceMod.h"

static dict_ci<trigger_time> trigger_names{
	{ "after_update",trigger_time::AfterUpdate },
};

#define Text2GBK(s) (s ? (isUTF8 ? UTF8toGBK(s) :s) : "")
AttrShape::AttrShape(const tinyxml2::XMLElement* node, bool isUTF8) {
	if (auto text{ node->Attribute("alias") }) {
		alias = split(Text2GBK(text), "/");
	}
	if (auto exp{ node->GetText() }) {
		string s{ Text2GBK(exp) };
		if (auto text{ node->Attribute("text") }) {
			if (auto lower{ toLower(text) }; lower == "dicexp") {
				textType = AttrShape::TextType::Dicexp;
				defVal = s;
				return;
			}
			else if (lower == "javascript") {
				textType = AttrShape::TextType::JavaScript;
				defVal = s;
				return;
			}
		}
		defVal = AttrVar::parse(s);
	}
}
AttrVar AttrShape::init(ptr<CardTemp> temp, CharaCard* pc) {
	if (textType == TextType::JavaScript) {
		if (auto ret = temp->js_ctx->evalStringLocal(defVal, temp->type, pc->shared_from_this());
			!JS_IsException(ret)) {
			return temp->js_ctx->getValue(ret);
		}
		else console.log("生成<" + temp->type + ">属性时执行js失败!\n" + temp->js_ctx->getException(), 0b10);
	}
	else if (textType == TextType::Dicexp && !defVal.is_null()) {
		if (auto exp{ fmt->format(defVal, pc->shared_from_this()) }; exp.is_text()) {
			return pc->cal(exp);
		}
		else return exp;
	}
	return defVal;
}
 /**
  * 错误返回值
  * 3:大于上限
  * 2:小于下限
  * 1:浮点数规整
  * -1:类型无法匹配
  */
int AttrShape::check(AttrVar& val) {
	return 0;
}

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
std::vector<std::pair<std::string, std::string>> BuildCOC7 = {
	{"__Name", "{随机姓名}"},
	{"力量", "3D6*5"},
	{"体质", "3D6*5"},
	{"体型", "2D6*5+30"},
	{"敏捷", "3D6*5"},
	{"外貌", "3D6*5"},
	{"智力", "2D6*5+30"},
	{"意志", "3D6*5"},
	{"教育", "2D6*5+30"},
	{"幸运", "3D6*5"},
};
std::vector<std::pair<std::string, std::string>> COC7_BG{ {"性别", "{性别}"}, {"年龄", "7D6+8"}, {"职业", "{调查员职业}"}, {"个人描述", "{个人描述}"},
										{"重要之人", "{重要之人}"}, {"思想信念", "{思想信念}"}, {"意义非凡之地", "{意义非凡之地}"},
										{"宝贵之物", "{宝贵之物}"}, {"特质", "{调查员特点}"},
};
CardTemp ModelCOC7{ "COC7", SkillNameReplace, BasicCOC7, mVariableCOC7, ExpressionCOC7,
			SkillDefaultVal, dict_ci<CardPreset>{
				{"pc", CardPreset{BuildCOC7}},
				{"bg", CardPreset{COC7_BG}},
			} };
dict_ci<ptr<CardTemp>> CardModels{ 
	{"COC7", std::make_shared<CardTemp>(ModelCOC7),},
};

CardPreset::CardPreset(const tinyxml2::XMLElement* d, bool isUTF8) {
	for (auto elem = d->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
		if (auto name{ elem->Attribute("name") }) {
			shapes[Text2GBK(name)] = { elem,isUTF8 };
		}
	}
}
int loadCardTemp(const std::filesystem::path& fpPath, dict_ci<CardTemp>& m) {
	tinyxml2::XMLDocument doc;
	if (auto err{ doc.LoadFile(fpPath.string().c_str()) }; tinyxml2::XML_SUCCESS == err) {
		bool isUTF8(false);
		if (auto declar{ doc.FirstChild()->ToDeclaration() }) {
			isUTF8 = string(declar->Value()).find("UTF-8") != string::npos;
		}
		if (auto root{ doc.FirstChildElement() }) {
			if (auto tp_name{ root->Attribute("name") }) {
				string model_name{ Text2GBK(tp_name) };
				auto& tp{ m[model_name] };
				tp.type = model_name;
				if (auto raw_alias{ root->Attribute("alias") }) {
					tp.alias = split(Text2GBK(raw_alias), "/");
				}
				for (auto elem = root->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
					if (string tag{ elem->Name()}; tag == "basic") {
						tp.vBasicList.clear();
						for (auto kid = elem->FirstChildElement(); kid; kid = kid->NextSiblingElement()) {
							if (auto text{ kid->GetText() })tp.vBasicList.push_back(getLines(Text2GBK(text)));
						}
					}
					else if (tag == "property") {
						for (auto kid = elem->FirstChildElement(); kid; kid = kid->NextSiblingElement()) {
							if (auto name{ kid->Attribute("name") }) {
								string attr{ Text2GBK(name) };
								auto& shape{ tp.AttrShapes[attr] = { kid,isUTF8 } };
								if (!shape.alias.empty())for (auto& key : shape.alias) {
									tp.replaceName[key] = attr;
								}
							}
						}
					}
					else if (tag == "presets") {
						for (auto kid = elem->FirstChildElement(); kid; kid = kid->NextSiblingElement()) {
							if (auto opt{ kid->Attribute("name") })tp.presets[Text2GBK(opt)] = CardPreset(kid, isUTF8);
						}
					}
					else if (tag == "script") {
						tp.script = Text2GBK(elem->GetText());
						for (auto kid = elem->FirstChildElement(); kid; kid = kid->NextSiblingElement()) {
							if (auto name{ kid->Attribute("name") }; name && string(kid->Name()) == "trigger") {
								string trigger_name{ Text2GBK(name) };
								//auto& trigger{ tp.triggers[trigger_name] };
								tp.triggers.emplace(trigger_name, CardTrigger{ trigger_name,
									trigger_names[Text2GBK(kid->Attribute("time"))],
									Text2GBK(kid->GetText()) });
							}
						}
					}
				}
				return 1;
			}
		}
	}
	else {
		console.log("读取" + fpPath.string() + "失败:" + doc.ErrorStr(), 0);
		return -1;
	}
	return 0;
}

CardTemp& CardTemp::merge(const CardTemp& other) {
	if (type.empty())type = other.type;
	map_merge(AttrShapes, other.AttrShapes);
	map_merge(replaceName, other.replaceName);
	if (!other.script.empty())script += "\n" + other.script;
	for (auto& [opt, preset] : other.presets) {
		map_merge(presets[opt].shapes, preset.shapes);
	}
	map_merge(triggers, other.triggers);
	return *this;
}
void CardTemp::init() {
	js_ctx = std::make_shared<js_context>();
	if (!script.empty()) {
		if (auto ret{ js_ctx->evalString(script, "model.init") };
			JS_IsException(ret)) {
			console.log("初始化<" + type + ">js脚本失败!\n" + js_ctx->getException(), 0b10);
		}
	}
	if (!triggers.empty())for (auto& [name, trigger] : triggers) {
		triggers_by_time.emplace(trigger.time, trigger);
	}
}
string CardTemp::show() {
	ResList res;
	if (!AttrShapes.empty()) {
		ShowList resVar;
		for (const auto& [key, val] : AttrShapes) {
			resVar << key;
		}
		res << "预设属性:" + resVar.show("/");
	}
	return "pc模板:" + type + res.show();
}
void CardTemp::after_update(const ptr<AnysTable>& eve) {
	for (auto& [t, trigger] : multi_range(triggers_by_time, trigger_time::AfterUpdate)) {
		if (auto ret = js_ctx->evalStringLocal(trigger.script, trigger.name, eve);
			JS_IsException(ret)) {
			console.log("执行<" + type + ">after_update触发器" + trigger.name + "失败!\n" + js_ctx->getException(), 0b10);
		}
	}
}

ptr<CardTemp> CharaCard::getTemplet()const{
	if (string type{ get_str("__Type") };
		!type.empty() && CardModels.count(type))return CardModels[type];
	return CardModels["COC"];
}

void CharaCard::update() {
	dict["__Update"] = (long long)time(nullptr);
}
void CharaCard::setName(const string& strName) {
	dict["__Name"] = Name = strName;
}
void CharaCard::setType(const string& strType) {
	dict["__Type"] = strType;
}
AttrVar CharaCard::get(const string& key, const AttrVar& val)const{
	if (dict.count(key))return dict.at(key);
	auto temp{ getTemplet() };
	string attr{ standard(key) };
	if (dict.count(attr)) {
		return dict.at(attr);
	}
	if (temp->canGet(attr)) {
		return temp->AttrShapes.at(attr).init(temp, const_cast<CharaCard*>(this));
	}
	return val;
}
int CharaCard::set(string key, const AttrVar& val) {
	if (key.empty())return -1;
	if (key == "__Name")return -8;
	if (val.is_text() && val.text.length() > 256)return -11;
	key = standard(key);
	if (getTemplet()->equalDefault(key, val)){
		if (dict.count(key)) dict.erase(key);
		else return -1;
	}
	else {
		dict[key] = val;
	}
	update();
	return 0;
}

string CharaCard::print(const string& key){
	if (dict.count(key))return dict.at(key).print();
	if (auto temp{ getTemplet() }; temp->canGet(key)) {
		return temp->AttrShapes.at(key).init(temp, this).print();
	}
	return {};
}
std::optional<string> CharaCard::show(string& key) {
	if (dict.count(key) || has(key = standard(key))) {
		if (auto res{ get(key) }; !res.is_null())return res.print();
	}
	return std::nullopt;
}
std::optional<string> CharaCard::show(const string& key) {
	if (dict.count(key) || has(standard(key))) {
		if (auto res{ get(key) }; !res.is_null())return res.print();
	}
	return std::nullopt;
}

[[nodiscard]] string CharaCard::standard(const string& key) const {
	if (auto temp{ getTemplet() };
		!key.empty() && temp->replaceName.count(key)) {
		return temp->replaceName.find(key)->second;
	}
	return key;
}
bool CharaCard::has(const string& key)const {
	if (dict.count(key))return true;
	if (string attr{ standard(key) }; dict.count(attr)) {
		return !dict.at(attr).is_null();
	}
	else return getTemplet()->canGet(attr);
}
bool CharaCard::hasAttr(string& key) const {
	return dict.count(key) || dict.count(key = standard(key));
}

//求key对应掷骰表达式
string CharaCard::getExp(string& key, std::unordered_set<string> sRef){
	sRef.insert(key = standard(key));
	auto temp{ getTemplet() };
	auto val = dict.find("&" + key);
	if (val != dict.end())return escape(val->second.to_str(), sRef);
	else if ((val = dict.find(key)) != dict.end())return escape(val->second.to_str(), sRef);
	else if (auto exp = temp->AttrShapes.find("&" + key); exp != temp->AttrShapes.end())return escape(exp->second.init(temp, this).to_str(), sRef);
	else if (auto exp = temp->AttrShapes.find(key); exp != temp->AttrShapes.end())return escape(exp->second.init(temp, this).to_str(), sRef);
	return "0";
}
bool CharaCard::countExp(const string& key)const {
	return key[0] == '&' ? (has(key) || getTemplet()->canGet(key))
		: (has("&" + key) || getTemplet()->canGet("&" + key));
}
std::optional<int> CharaCard::cal(string exp){
	if (exp[0] == '&'){
		if (auto res{ get(exp.substr(1)) };res.is_numberic()) {
			return res.to_int();
		}
	}
	else {
		size_t r = 0, l = 0;
		while ((r = exp.find(')')) != string::npos && (l = exp.rfind('(', r)) != string::npos) {
			if (auto res{ cal(exp.substr(l + 1, r - l - 1)) })
				exp.replace(l, r + 1, to_string(*res));
			else return std::nullopt;
		}
		if (const RD Res(exp); !Res.Roll()) {
			return Res.intTotal;
		}
	}
	return std::nullopt;
}

void CharaCard::build(const string& para)
{
	auto tmp{ getTemplet() };
	if (const auto it = tmp->presets.find(para);
		it != tmp->presets.end()) {
		auto& preset = it->second;
		for (auto& [attr, shape] : preset.shapes) {
			if (!dict.count(attr) || dict.at(attr).is_null())set(attr, shape.init(tmp, this));
		}
	}
}
void CharaCard::buildv(string para)
{
	std::stack<string> vOption;
	int Cnt = 0;
	while ((Cnt = para.rfind(':')) != string::npos)
	{
		vOption.push(para.substr(Cnt + 1));
		para.erase(para.begin() + Cnt, para.end());
	}
	if (!para.empty())vOption.push(para);
	if (vOption.empty())build("pc");
	else while (!vOption.empty())
	{
		const string para2 = vOption.top();
		vOption.pop();
		build(para2);
	}
	update();
}

void CharaCard::clear() {
	dict = AttrVars{{"__Type",dict["__Type"]},{"__Name",dict["__Name"]}};
}
[[nodiscard]] string CharaCard::show(bool isWhole){
	std::set<string> sDefault;
	ResList Res;
	for (const auto& list : getTemplet()->vBasicList) {
		ResList subList;
		for (const auto& it : list) {
			if (auto val{ show(it) }) {
				sDefault.insert(it);
				if (it[0] == '&')subList << it + "=" + *val;
				else subList << it + ":" + *val;
			}
		}
		Res << subList.show();
	}
	string strAttrRest;
	for (const auto& [key,val] : dict) {
		if (sDefault.count(key) || key[0] == '_'
			|| (isWhole && val.type == AttrVar::Type::Number))continue;
		strAttrRest += key + ":" + val.print() + (val.type == AttrVar::Type::Number 
			? " " 
			: (val.type == AttrVar::Type::Text ? "\t" : "\n"));
	}
	Res << strAttrRest;
	return Res.show();
}

bool CharaCard::erase(string& key) {
	if (dict.count(key) || dict.count(key = standard(key))) {
		dict.erase(key);
	}
	else if (dict.count("&" + key)) {
		dict.erase("&" + key);
	}
	else return false;
	update();
	return true;
}
void CharaCard::writeb(std::ofstream& fout) const {
	fwrite(fout, string("Name"));
	fwrite(fout, Name);
	fwrite(fout, string("Attrs"));
	AnysTable::writeb(fout);
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
			dict["__Type"] = fread<string>(fin);
			break;
		case 3:
			AnysTable::readb(fin);
			break;
		case 11: {
			std::unordered_map<string, short>TempAttr;
			fread(fin, TempAttr);
			for (auto& [key, val] : TempAttr) {
				AnysTable::set(key, val);
			}
		}
			break;
		case 21: {
			std::unordered_map<string, string>TempExp;
			fread(fin, TempExp);
			for (auto& [key, val] : TempExp) {
				AnysTable::set("&" + key, val);
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
				AnysTable::set(key, val);
			}
		}
			break;
		case 101:
			dict["note"] = fread<string>(fin);
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
	dict["__StatRcSumRate"] = get_int("__StatRcSumRate") + rate;	//总成功率
	update();
}
unordered_map<long long, Player> PList;

Player& getPlayer(long long uid)
{
	//if (!PList.count(uid))PList[uid] = {};
	return PList[uid];
}
int Player::changeCard(const string& name, long long group)
{
	if (name.empty())
	{
		mGroupCard.erase(group);
		return 1;
	}
	if (!NameList.count(name))return -5;
	mGroupCard[group] = NameList[name];
	return 0;
}
int Player::removeCard(const string& name){
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	if (!NameList.count(name))return -5;
	const auto id = NameList[name]->getID();
	if (!id)return -7;
	if (auto pc = mCardList[id]; pc->locked("q")) return -21;
	auto it = mGroupCard.cbegin();
	while (it != mGroupCard.cend())
	{
		if (it->second->getID() == id)
		{
			it = mGroupCard.erase(it);
		}
		else
		{
			++it;
		}
	}
	mCardList.erase(id);
	while (!mCardList.count(indexMax))indexMax--;
	NameList.erase(name);
	return 0;
}
int Player::renameCard(const string& name, const string& name_new) 	{
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	if (name_new.empty())return -3;
	if (NameList.count(name_new))return -4;
	if (name_new.find(":") != string::npos)return -6;
	const auto i = NameList[name]->getID();
	if (mCardList[i]->locked("n"))return -22;
	NameList[name_new] = NameList[name];
	NameList.erase(name);
	mCardList[i]->setName(name_new);
	return 0;
}
int Player::copyCard(const string& name1, const string& name2, long long group)
{
	if (name1.empty() || name2.empty())return -3;
	//不存在则新建人物卡
	if (!NameList.count(name2)){
		return -5;
	}
	else if (!NameList.count(name1)){
		std::lock_guard<std::mutex> lock_queue(cardMutex);
		//人物卡数量上限
		if (mCardList.size() > 32)return -1;
		if (name1.find(":") != string::npos)return -6;
		mCardList.emplace(++indexMax, std::make_shared<CharaCard>(name1, indexMax));
		NameList[name1] = mCardList[indexMax];
	}
	*getCard(name1) << *getCard(name2);
	return 0;
}
PC Player::getCard(const string& name, long long group)
{
	if (!name.empty() && NameList.count(name))return NameList[name];
	if (mGroupCard.count(group))return mGroupCard[group];
	if (mGroupCard.count(0))return mGroupCard[0];
	return mCardList.begin()->second;
}
PC Player::getCardByID(long long id)const {
	if (mGroupCard.count(id))return mGroupCard.at(id);
	if (mCardList.count(id))return mCardList.at(id);
	if (mGroupCard.count(0))return mGroupCard.at(0);
	return mCardList.begin()->second;
}
PC Player::operator[](const string& name)const {
	if (NameList.count(name))return NameList.at(name);
	if (mGroupCard.count(0))return mGroupCard.at(0);
	return mCardList.begin()->second;
}
int Player::emptyCard(const string& s, long long group, const string& type)
{
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	//人物卡数量上限
	if (mCardList.size() > 16)return -1;
	//无效模板不再报错
	if (NameList.count(s))return -4;
	if (s.find("=") != string::npos)return -6;
	PC card{ std::make_shared<CharaCard>(s, ++indexMax, type) };
	mCardList.emplace(indexMax, card);
	NameList[s] = card;
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
	}
	else if (CardModels.count(s))
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
	//if (!getCardModels().count(type))return -2;
	if (NameList.count(s))return -4;
	if (s.find("=") != string::npos)return -6;
	PC card{ std::make_shared<CharaCard>(s, ++indexMax, type) };
	mCardList.emplace(indexMax, card);
	auto temp{ card->getTemplet()};
	while (!vOption.empty())
	{
		string para = vOption.top();
		vOption.pop();
		card->build(para);
		if (card->getName().empty() && temp->presets.count(para) && temp->presets[para].shapes.count("__Name")) {
			if (!NameList.count(s = temp->presets[para].shapes["__Name"].init(temp, card.get())))card->setName(s);
		}
	}
	if (card->getName().empty()){
		if (temp->presets.count("pc") && temp->presets["pc"].shapes.count("__Name")) {
			if (!NameList.count(s = temp->presets["pc"].shapes["__Name"].init(temp, card.get())))card->setName(s);
		}
		if (card->getName().empty())card->setName(to_string(indexMax + 1));
	}
	s = card->getName();
	mGroupCard[group] = NameList[s] = card;
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
	if (!strName.empty() && !NameList.count(strName))
	{
		if (const int res = newCard(name, group))return res;
		//PC pc{ getCard(strName, group) };
		getCard(strName, group)->buildv();
	}
	else{
		auto pc{ getCard(strName, group) };
		name = pc->getName();
		if (isClear)pc->clear();
		pc->buildv(strType);
		if (NameList[name] != mGroupCard[group])mGroupCard[group] = pc;
	}
	return 0;
}
string Player::listCard() const{
	ResList Res;
	for (auto& [idx, pc] : mCardList) {
		Res << "[" + to_string(idx) + "]<" + pc->get_str("__Type") + ">" + pc->getName();
	}
	Res << "default:" + getCardByID(0)->getName();
	return Res.show();
}

void Player::writeb(std::ofstream& fout) const {
	std::lock_guard<std::mutex> lock_queue(cardMutex);
	fwrite(fout, indexMax);
	fwrite(fout, mCardList);
	if (const auto len = static_cast<short>(mGroupCard.size()); len != 1 || mGroupCard.begin()->second->getID() != 0) {
		fwrite(fout, len);
		for (const auto& [gid, pc] : mGroupCard)
		{
			fwrite(fout, gid);
			fwrite(fout, unsigned short(pc ? pc->getID() : 0));
		}
	}
	else fwrite(fout, (short)0);
}
void Player::readb(std::ifstream& fin)
{
	indexMax = fread<short>(fin);
	if (short len = fread<short>(fin); len > 0) while (len--) {
		auto idx = fread<unsigned short>(fin);
		PC card = std::make_shared<CharaCard>(idx);
		card->readb(fin);
		if (auto name{ card->getName() }; !name.empty()) {
			mCardList[idx] = card;
			NameList[name] = card;
		}
	}
	if (short len = fread<short>(fin); len > 0){
		while (len--) {
			unsigned long long gid = fread<unsigned long long>(fin);
			unsigned short pcid = fread<unsigned short>(fin);
			if(mCardList.count(pcid))mGroupCard[gid] = mCardList[pcid];
		}
	}
}

AttrVar idx_pc(const AttrObject& eve){
	if (eve->has("pc"))return eve->at("pc");
	if (!eve->has("uid"))return {};
	long long uid{ eve->get_ll("uid") };
	long long gid{ eve->get_ll("gid") };
	if (PList.count(uid) && PList[uid][gid]->getName() != "角色卡")
		return eve->at("pc") = PList[uid][gid]->getName();
	return idx_nick(eve);
}
