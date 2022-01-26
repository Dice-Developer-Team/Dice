//#pragma comment(lib, "lua.lib")  
extern "C"{
#include <lua.h>  
#include <lualib.h>  
#include <lauxlib.h>  
#include <lobject.h>  
};
#include "ManagerSystem.h"
#include "DiceEvent.h"
#include "RandomGenerator.h"
#include "CharacterCard.h"
#include "CardDeck.h"
#include "DiceSession.h"
#include "DiceMod.h"
#include "DDAPI.h"

unordered_set<lua_State*> UTF8Luas;
constexpr const char* chDigit{ "0123456789" };

class LuaState {
	lua_State* state;
	//bool isValid;
public:
	LuaState(const char* file);
	LuaState();
	//operator bool()const { return isValid; }
	operator lua_State* () { return state; }
	~LuaState() {
		if(state)lua_close(state);
		UTF8Luas.erase(state);
	}
	void regist();
};
double lua_to_number(lua_State* L, int idx) {
	return luaL_checknumber(L, idx);
}
long long lua_to_int(lua_State* L, int idx) {
	return luaL_checkinteger(L, idx);
}

// Return a GB18030 string
string lua_to_gb18030_string(lua_State* L, int idx) {
	const char* str{ luaL_checkstring(L, idx) };
	if (!str) return {};
	return UTF8toGBK(str, true);
}

// Return a GB18030 string on Windows and a UTF-8 string on other platforms
string lua_to_string(lua_State* L, int idx) {
	const char* str{ luaL_checkstring(L, idx) };
	if (!str) return {};
#ifdef _WIN32
	return UTF8toGBK(str, true);
#else
	return GBKtoUTF8(str, true);
#endif
}


void lua_push_string(lua_State* L, const string& str) {
	if (UTF8Luas.count(L))
		lua_pushstring(L, GBKtoUTF8(str, true).c_str());
	else
		lua_pushstring(L, UTF8toGBK(str, true).c_str());
}
void lua_set_field(lua_State* L, int idx, const string& str) {
	if (UTF8Luas.count(L))
		lua_setfield(L, idx, GBKtoUTF8(str, true).c_str());
	else
		lua_setfield(L, idx, UTF8toGBK(str, true).c_str());
}

void lua_push_msg(lua_State* L, FromMsg* msg) {
	msg->vars["fromQQ"] = to_string(msg->fromChat.uid);
	msg->vars["fromGroup"] = to_string(msg->fromChat.gid);
	AttrVars** p{ (AttrVars**)lua_newuserdata(L, sizeof(AttrVars*)) };
	*p = &msg->vars;
	luaL_setmetatable(L, "Context");
}

void lua_push_attr(lua_State* L, const AttrVar& attr) {
	switch (attr.type) {
	case AttrVar::AttrType::Nil:
		lua_pushnil(L);
		break;
	case AttrVar::AttrType::Boolean:
		lua_pushboolean(L, attr.bit);
		break;
	case AttrVar::AttrType::Integer:
		lua_pushnumber(L, attr.attr);
		break;
	case AttrVar::AttrType::ID:
		lua_pushnumber(L, attr.id);
		break;
	case AttrVar::AttrType::Number:
		lua_pushnumber(L, attr.number);
		break;
	case AttrVar::AttrType::Text:
		lua_push_string(L, attr.text.c_str());
		break;
	case AttrVar::AttrType::Table:
		lua_newtable(L);
		int idx{ 0 };
		unordered_set<string> idxs;
		for (auto& val : attr.table.idxs) {
			val ? lua_push_attr(L, *val.get()) : lua_pushnil(L);
			lua_seti(L, -2, ++idx);
			idxs.insert(to_string(idx));
		}
		for (auto& [key, val] : attr.table.dict) {
			if (idxs.count(key))continue;
			val ? lua_push_attr(L, *val.get()) : lua_pushnil(L);
			lua_set_field(L, -2, key.c_str());
		}
		break;
	}
}
AttrVar lua_to_attr(lua_State* L, int idx = -1) {
	switch (lua_type(L, idx)) {
	case LUA_TBOOLEAN:
		return (bool)lua_toboolean(L, idx);
		break;
	case LUA_TNUMBER:
		if (lua_isinteger(L, idx)) {
			return lua_tointeger(L, idx);
		}
		else {
			return lua_tonumber(L, idx);
		}
		break;
	case LUA_TSTRING:
		return lua_to_gb18030_string(L, idx);
		break;
	case LUA_TTABLE:
		AttrVars tab;
		lua_pushnil(L);
		while (lua_next(L, idx)) {
			if (lua_type(L, -2) == LUA_TNUMBER) {
				tab[to_string(lua_tointeger(L, -2))] = lua_to_attr(L, -1);
			}
			else {
				tab[lua_to_gb18030_string(L, -2)] = lua_to_attr(L, -1);
			}
			lua_pop(L, 1);
		}
		return AttrVar(tab);
		break;
	}
	return {};
}

void CharaCard::pushTable(lua_State* L) {
	lua_newtable(L);
	for (auto& [key, attr] : Attr) {
		lua_push_attr(L, attr);
		lua_set_field(L, -2, key.c_str());
	}
}
void CharaCard::toCard(lua_State* L) {

}
//读取指定lua文件的函数，file为原生系统编码
bool lua_msg_order(FromMsg* msg, const char* file, const char* func) {
#ifndef _WIN32
	// 转换separator
	string fileStr(file);
	for (auto& c : fileStr)
	{
		if (c == '\\') c = '/';
	}
	file = fileStr.c_str();
#endif
	LuaState L(file);
	if (!L)return false;
#ifdef _WIN32
	// 转换为GB18030
	string fileGB18030(file);
#else
	string fileGB18030(UTF8toGBK(file, true));
#endif
	if (lua_pcall(L, 0, 0, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "运行lua文件" + fileGB18030 + "失败:" + pErrorMsg, 0b10);
		return 0;
	}
	lua_getglobal(L, func); 
	lua_push_msg(L, msg);
	if (lua_pcall(L, 1, 2, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "失败!\n" + pErrorMsg, 0b10);
		msg->reply(getMsg("strOrderLuaErr"));
		return false;
	}
	if (lua_gettop(L) && lua_type(L, 1) != LUA_TNIL) {
		if (!lua_isstring(L, 1)) {
			console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "返回值格式错误!", 1);
			msg->reply(getMsg("strOrderLuaErr"));
			return false;
		}
		if (!(msg->vars["msg_reply"] = lua_to_gb18030_string(L, 1)).str_empty()) {
			msg->reply(msg->vars["msg_reply"].to_str());
		}
		if (lua_type(L, 2) != LUA_TNIL) {
			if (!lua_isstring(L, 2)) {
				console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "返回值格式错误!", 1);
				return false;
			}
			if (!(msg->vars["msg_hidden"] = lua_to_gb18030_string(L, 2)).str_empty()) {
				msg->replyHidden(msg->vars["msg_hidden"].to_str());
			}
		}
	}
	return true;
}

//为msg直接调用lua语句回复
bool lua_msg_reply(FromMsg* msg, const string& luas) {
	bool isFile{ fmt->script_has(luas) };
	LuaState L{ isFile ? fmt->script_path(luas).c_str() : nullptr };
	if (!L)return false;
	lua_push_msg(L, msg);
	lua_setglobal(L, "msg");
	if (isFile) {
		if (lua_pcall(L, 0, 2, 0)) {
			string pErrorMsg = lua_to_gb18030_string(L, -1);
			console.log(getMsg("strSelfName") + "运行lua脚本" + luas + "失败:" + pErrorMsg, 0b10);
			return 0;
		}
	}
	else if (luaL_loadstring(L, luas.c_str()) || lua_pcall(L, 0, 2, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "调用回复lua语句失败!\n" + pErrorMsg, 0b10);
		msg->reply(getMsg("strOrderLuaErr"));
		return false;
	}
	if (lua_gettop(L) && lua_type(L, 1) != LUA_TNIL) {
		if (!lua_isstring(L, 1)) {
			console.log(getMsg("strSelfName") + "调用回复lua语句返回值格式错误!", 0b10);
			msg->reply(getMsg("strOrderLuaErr"));
			return false;
		}
		if (!(msg->vars["msg_reply"] = lua_to_gb18030_string(L, 1)).str_empty()) {
			msg->reply(msg->vars["msg_reply"].to_str());
		}
		if (lua_type(L, 2) != LUA_TNIL) {
			if (!lua_isstring(L, 2)) {
				console.log(getMsg("strSelfName") + "调用回复lua语句返回值格式错误!", 1);
				return false;
			}
			if (!(msg->vars["msg_hidden"] = lua_to_gb18030_string(L, 2)).str_empty()) {
				msg->replyHidden(msg->vars["msg_hidden"].to_str());
			}
		}
	}
	return true;
}

bool lua_call_task(const char* file, const char* func) {
#ifndef _WIN32
	// 转换separator
	string fileStr(file);
	for (auto& c : fileStr)
	{
		if (c == '\\') c = '/';
	}
	file = fileStr.c_str();
#endif
	LuaState L(file);
	if (!L)return false;
#ifdef _WIN32
	// 转换为GB18030
	string fileGB18030(file);
#else
	string fileGB18030(UTF8toGBK(file, true));
#endif
	if (lua_pcall(L, 0, 0, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "运行lua文件" + fileGB18030 + "失败:" + pErrorMsg, 0b10);
		return 0;
	}
	lua_getglobal(L, func);
	if (lua_pcall(L, 0, 0, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "失败!\n" + pErrorMsg, 0b10);
		return false;
	}
	return true;
}

/**
 * 供lua调用的函数
 */

 //输出日志
int log(lua_State* L) {
	string info{ lua_to_string(L, 1) };
	if (info.empty())return 0;
	int note_lv{ 0 };
	for (int idx = lua_gettop(L); idx > 1; --idx) {
		auto type{ lua_to_int(L,idx) };
		if (type < 0 || type > 9)continue;
		else{
			note_lv |= (1 << type);
		}
	}
	console.log(info, note_lv);
	return 0;
}
 //加载其他lua脚本
int loadLua(lua_State* L) {
	string nameLua{ lua_to_string(L, 1) };
	if (nameLua.empty())return 0;
#ifdef _WIN32 // 转换separator
	for (auto& c : nameLua)
	{
		if (c == '/') c = '\\';
	}
	bool hasSlash{ nameLua.find('\\') != string::npos };
#else
	for (auto& c : nameLua)
	{
		if (c == '\\') c = '/';
	}
	bool hasSlash{ nameLua.find('/') != string::npos };
#endif
	std::filesystem::path pathFile{ nameLua };
	if (fmt->script_has(nameLua)) {
		pathFile = fmt->script_path(nameLua);
	}
	else {
		if ((pathFile.extension() != ".lua") && (pathFile.extension() != ".LUA"))pathFile = nameLua + ".lua";
		if (pathFile.is_relative())pathFile = DiceDir / "plugin" / pathFile;
		if (!std::filesystem::exists(pathFile) && !hasSlash)
			pathFile = DiceDir / "plugin" / nameLua / "init.lua";
	}
	string strLua;
	if (std::filesystem::exists(pathFile)) {
		readFile(pathFile, strLua);
		bool isStateUTF8{ (bool)UTF8Luas.count(L) }, isLuaUTF8{ checkUTF8(strLua) };	//令文件加载入lua的编码保持一致
		if (isStateUTF8 && !isLuaUTF8) {
			strLua = GBKtoUTF8(strLua);
		}
		else if (!isStateUTF8 && isLuaUTF8) {
			strLua = UTF8toGBK(strLua);
		}
	}
	else {
		DD::debugLog("待加载Lua未找到:"+ UTF8toGBK(pathFile.u8string()));
		return 0;
	}
	if (luaL_loadstring(L, strLua.c_str())) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "读取lua文件" + UTF8toGBK(pathFile.u8string()) + "失败:"+ pErrorMsg, 0b10);
		return 0;
	}
	if (lua_pcall(L, 0, 1, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "运行lua文件" + UTF8toGBK(pathFile.u8string()) + "失败:"+ pErrorMsg, 0b10);
		return 0;
	}
	return 1;
}
 //获取DiceMaid
int getDiceQQ(lua_State* L) {
	lua_push_string(L, to_string(console.DiceMaid));
	return 1;
}
//获取DiceDir存档目录
int getDiceDir(lua_State* L) {
	lua_push_string(L, DiceDir.u8string());
	return 1;
}
int mkDirs(lua_State* L) {
	string dir{ lua_to_string(L, 1) };
	mkDir(dir);
	return 0;
}
int getGroupConf(lua_State* L) {
	int top{ lua_gettop(L) };
	if (top > 3)lua_settop(L, top = 3);
	else if (top < 2)return 0;
	long long id{ lua_to_int(L, 1) };
	string item{ lua_to_gb18030_string(L, 2) };
	if (!id || item.empty())return 0;
	if (item == "size") {
		lua_pushnumber(L, (double)DD::getGroupSize(id).currSize);
	}
	else  if (item == "maxsize") {
		lua_pushnumber(L, (double)DD::getGroupSize(id).maxSize);
	}
	else if (item.find("card#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		string card{ DD::getGroupNick(id, uid) };
		if (!card.empty())lua_push_string(L, card);
	}
	else if (item.find("auth#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		lua_pushnumber(L, DD::getGroupAuth(id, uid, 0));
	}
	else if (item.find("lst#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		lua_pushnumber(L, DD::getGroupLastMsg(id, uid));
	}
	else if (ChatList.count(id)) {
		Chat& grp{ chat(id) }; 
		if (item == "name") {
			if (grp.Name.empty())lua_push_string(L, grp.Name = DD::getGroupName(id));
			else lua_push_string(L, grp.Name);
		}
		else if (item == "firstCreate") {
			lua_pushnumber(L, (double)grp.tCreated);
		}
		else if (item == "lastUpdate") {
			lua_pushnumber(L, (double)grp.tUpdated);
		}
		else if (grp.confs.count(item)) {
			lua_push_attr(L, grp.confs.find(item)->second);
		}
	}
	else if (item == "name") {
		lua_push_string(L, DD::getGroupName(id));
	}
	if (lua_gettop(L) == top) {
		lua_pushnil(L);
		lua_insert(L, 3);
	}
	return 1;
}
int setGroupConf(lua_State* L) {
	long long id{ lua_to_int(L, 1) };
	string item{ lua_to_gb18030_string(L, 2) };
	if (!id || item.empty())return 0;
	Chat& grp{ chat(id) };
	if (item.find("card#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		string card{ lua_to_gb18030_string(L, 3) };
		DD::setGroupCard(id, uid, card);
		return 0;
	}
	else if (lua_isnoneornil(L, 3)) {
		grp.reset(item);
	}
	else grp.set(item, lua_to_attr(L, 3));
	return 0;
}
int getUserConf(lua_State* L) {
	int top{ lua_gettop(L) };
	if (top > 3)lua_settop(L, top = 3);
	else if (top < 2)return 0;
	long long uid{ lua_to_int(L, 1) };
	if (!uid)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	if (item == "name") {
		if (string name{ DD::getQQNick(uid) };!name.empty())
			lua_push_string(L, name);
	}
	else if (item == "nick" ) {
		lua_push_string(L, getName(uid));
	}
	else if (item.find("nick#") == 0) {
		long long gid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		lua_push_string(L, getName(uid, gid));
	}
	else if (item == "trust") {
		lua_pushnumber(L, trustedQQ(uid));
	}
	else if (UserList.count(uid)) {
		User& user{ getUser(uid) };
		if (item == "firstCreate") {
			lua_pushnumber(L, (double)user.tCreated);
		}
		else  if (item == "lastUpdate") {
			lua_pushnumber(L, (double)user.tUpdated);
		}
		else if (item == "nn") {
			string nick;
			user.getNick(nick);
			lua_push_string(L, nick);
		}
		else if (item.find("nn#") == 0) {
			string nick;
			long long gid{ 0 };
			if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
				gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
			}
			user.getNick(nick, gid);
			lua_push_string(L, nick);
		}
		else if (user.confs.count(item)) {
			lua_push_attr(L, user.confs[item]);
		}
	}
	if (lua_gettop(L) == top) {
		lua_pushnil(L);
		lua_insert(L, 3);
	}
	return 1;
}
int setUserConf(lua_State* L) {
	long long uid{ lua_to_int(L, 1) };
	if (!uid)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	if (item == "trust") {
		if (!lua_isnumber(L, 3))return 0;
		int trust{ (int)lua_tonumber(L, 3) };
		if (trust > 4 || trust < 0)return 0;
		User& user{ getUser(uid) };
		if (user.nTrust > 4)return 0;
		user.trust(trust);
	}
	else if (lua_isnoneornil(L, 3)) {
		getUser(uid).rmConf(item);
	}
	else getUser(uid).setConf(item, lua_to_attr(L, 3));
	return 0;
}
int getUserToday(lua_State* L) {
	long long uid{ lua_to_int(L, 1) };
	if (!uid)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	if (item == "jrrp")
		lua_pushnumber(L, today->getJrrp(uid));
	else
		lua_pushnumber(L, today->get(uid, item));
	return 1;
}
int setUserToday(lua_State* L) {
	long long uid{ lua_to_int(L, 1) };
	if (!uid)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	int val{ (int)lua_to_number(L, 3) };
	today->set(uid, item, val);
	return 0;
}

int getPlayerCardAttr(lua_State* L) {
	if (int argc{ lua_gettop(L) }; argc > 4)lua_settop(L, 4);
	else if (argc == 3)lua_pushnil(L);
	else if (argc < 3)return 0;
	long long plQQ{ lua_to_int(L, 1) };
	long long group{ lua_to_int(L, 2) };
	string key{ lua_to_gb18030_string(L, 3) };
	if (!plQQ || key.empty())return 0;
	CharaCard& pc = getPlayer(plQQ)[group];
	if (pc.Attr.count(key)) {
		lua_push_attr(L, pc.Attr.find(key)->second);
	}
	else if (key = pc.standard(key); pc.Attr.count(key)) {
		lua_push_attr(L, pc.Attr.find(key)->second);
	}
	else if (key == "note") {
		lua_push_string(L, pc.Note);
	}
	else if (pc.Attr.count("&" + key)) {
		lua_push_string(L, pc.Attr.find("&" + key)->second.to_str());
	}
	else {
		lua_pushnil(L);
		lua_insert(L, 4);
	}
	return 1;
}
int getPlayerCard(lua_State* L) {
	long long plQQ{ lua_to_int(L, 1) };
	if (!plQQ)return 0;
	long long group{ lua_to_int(L, 2) };
	if (PList.count(plQQ)) {
		getPlayer(plQQ)[group].pushTable(L);
		return 1;
	}
	return 0;
}
int setPlayerCardAttr(lua_State* L) {
	long long plQQ{ lua_to_int(L, 1) };
	long long group{ lua_to_int(L, 2) };
	string item{ lua_to_gb18030_string(L, 3) };
	if (!plQQ || item.empty())return 0;
	//参数4为空则视为删除,__Name除外
	CharaCard& pc = getPlayer(plQQ)[group];
	if (item == "__Name") {
		getPlayer(plQQ).renameCard(pc.getName(), lua_to_gb18030_string(L, 4));
	}
	else if (lua_isnoneornil(L, 4)) {
		pc.erase(item); 
	}
	else pc.set(item, lua_to_attr(L, 4));
	return 0;
}

//取随机数
int ranint(lua_State* L) {
	int l{ (int)lua_to_int(L, 1) };
	int r{ (int)lua_to_int(L, 2) };
	lua_pushnumber(L, RandomGenerator::Randint(l,r));
	return 1;
}
//线程等待
int sleepTime(lua_State* L) {
	int ms{ (int)lua_to_int(L, 1) };
	if (ms <= 0)return 0;
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	return 0;
}

int drawDeck(lua_State* L) {
	long long fromGID{ lua_to_int(L, 1) };
	long long fromUID{ lua_to_int(L, 2) };
	if (!fromGID && !fromUID)return 0;
	string nameDeck{ lua_to_gb18030_string(L, 3) };
	if (nameDeck.empty())return 0;
	long long fromSession{ fromGID ? fromGID : ~fromUID };
	if (gm->has_session(fromSession)) {
		lua_push_string(L, gm->session(fromSession).deck_draw(nameDeck));
	}
	else if (CardDeck::findDeck(nameDeck)) {
		vector<string>& deck = CardDeck::mPublicDeck[nameDeck];
		lua_push_string(L, CardDeck::draw(deck[RandomGenerator::Randint(0, deck.size() - 1)]));
	}
	else {
		lua_push_string(L, "{" + nameDeck + "}");
	}
	return 1;
}

int sendMsg(lua_State* L) {
	string fromMsg{ lua_to_gb18030_string(L, 1) };
	long long fromGID{ lua_to_int(L, 2) };
	long long fromUID{ lua_to_int(L, 3) };
	if (!fromGID && !fromUID)return 0;
	msgtype type{ fromGID ?
		chat(fromGID).isGroup ? msgtype::Group : msgtype::Discuss
		: msgtype::Private };
	AddMsgToQueue(fmt->format(fromMsg),
		{ fromUID,fromGID });
	return 0;
}
int eventMsg(lua_State* L) {
	string fromMsg{ lua_to_gb18030_string(L, 1) };
	long long fromGID{ lua_to_int(L, 2) };
	long long fromUID{ lua_to_int(L, 3) };
	msgtype type{ fromGID ?
		chat(fromGID).isGroup ? msgtype::Group : msgtype::Discuss
		: msgtype::Private };
	std::thread th([=]() {
		FromMsg msg(fromGID
			? FromMsg(AttrVars{ {"fromMsg",fromMsg},{"gid",fromGID}, {"uid", fromUID} }
				, { fromUID ,fromGID,0 })
			: FromMsg(AttrVars{ {"fromMsg",fromMsg}, {"uid", fromUID} }, { fromUID ,0,0 }));
		msg.virtualCall();
	});
	th.detach();
	return 0;
}

int Context_index(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	AttrVars& vars{ **(AttrVars**)luaL_checkudata(L, 1, "Context") };
	string key{ lua_to_string(L, 2) };
	if (vars.count(key)) {
		lua_push_attr(L, vars[key]);
		return 1;
	}
	return 0;
}
int Context_newindex(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	AttrVars& vars{ **(AttrVars**)luaL_checkudata(L, 1, "Context") };
	string key{ lua_to_string(L, 2) };
	if (lua_gettop(L) < 3) {
		vars.erase(key);
	}
	else if (AttrVar val{ lua_to_attr(L, 3) }; val.is_null()) {
		vars.erase(key);
	}
	else {
		vars[key] = val;
	}
	return 0;
}
static const luaL_Reg Context_funcs[] = {
	{"__index", Context_index},
	{"__newindex", Context_newindex},
	{NULL, NULL}
};
int luaopen_Context(lua_State* L) {
	luaL_newmetatable(L, "Context");
	luaL_setfuncs(L, Context_funcs, 0);
	return 1;
}

int httpGet(lua_State* L) {
	string url{ lua_to_string(L,1) };
	if (url.empty()) {
		return 0;
	}
	string ret;
	lua_pushboolean(L, Network::GET(url, ret));
	lua_push_string(L, ret);
	return 2;
}
int httpPost(lua_State* L) {
	string url{ lua_tostring(L,1) };
	string json{ lua_tostring(L,2) };
	if (url.empty() || json.empty()) {
		return 0;
	}
	string ret;
	lua_pushboolean(L, Network::POST(url, json, "application/json", ret));
	lua_push_string(L, ret);
	return 2;
}
int httpUrlEncode(lua_State* L) {
	string url{ lua_to_string(L,1) };
	lua_push_string(L, UrlEncode(url));
	return 1;
}
int httpUrlDecode(lua_State* L) {
	string url{ lua_to_string(L,1) };
	lua_push_string(L, UrlDecode(url));
	return 1;
}

static const luaL_Reg http_funcs[] = {
	{"get", httpGet},
	{"post", httpPost},
	{"urlEncode", httpUrlEncode},
	{"urlDecode", httpUrlDecode},
	{NULL, NULL}
};
int luaopen_http(lua_State* L) {
	luaL_newlib(L, http_funcs);
	return 1;
}

#define REGIST(func) {#func, func},

void LuaState::regist() {
	static const luaL_Reg DiceFucs[] = {
		REGIST(log)
		REGIST(loadLua)
		REGIST(getDiceQQ)
		REGIST(getDiceDir)
		REGIST(mkDirs)
		REGIST(getGroupConf)
		REGIST(setGroupConf)
		REGIST(getUserConf)
		REGIST(setUserConf)
		REGIST(getUserToday)
		REGIST(setUserToday)
		REGIST(getPlayerCardAttr)
		REGIST(setPlayerCardAttr)
		REGIST(getPlayerCard)
		REGIST(ranint)
		REGIST(sleepTime)
		REGIST(drawDeck)
		REGIST(sendMsg)
		REGIST(eventMsg)
		{nullptr, nullptr},
	};
	for (const luaL_Reg* lib = DiceFucs; lib->func; lib++) {
		lua_register(state, lib->name, lib->func);
	}
	static const luaL_Reg Dicelibs[] = {
		{"Context", luaopen_Context},
		{"http", luaopen_http},
		{NULL, NULL}
	};
	for (const luaL_Reg* lib = Dicelibs; lib->func; lib++) {
		luaL_requiref(state, lib->name, lib->func, 1);
		lua_pop(state, 1);  /* remove lib */
	}
	//modify package.path
	lua_getglobal(state, "package");
	lua_getfield(state, -1, "path");
	static string strPath{ (DiceDir / "plugin" / "?.lua").string() + ";"
		+ (DiceDir / "plugin" / "?" / "init.lua").string() + ";"
		+ (dirExe / "Diceki" / "lua" / "?.lua").string() + ";"
		+ (dirExe / "Diceki" / "lua" / "?" / "init.lua").string() + ";"
		+ lua_tostring(state, -1) };
	lua_push_string(state, strPath.c_str());
	lua_setfield(state, -3, "path");
	lua_pop(state, 1);
	lua_getfield(state, -1, "cpath");
	static string strCPath{ (dirExe / "Diceki" / "lua" / "?.dll").string() + ";"
		+ (dirExe / "Diceki" / "lib" / "?.dll").string() + ";"
		+ lua_tostring(state, -1) };
	lua_push_string(state, strCPath.c_str());
	lua_setfield(state, -3, "cpath");
	lua_pop(state, 2);
}

LuaState::LuaState() {//:isValid(false) {
	state = luaL_newstate();
	if (!state)return;
	luaL_openlibs(state);
	regist();
}
LuaState::LuaState(const char* file) {
#ifndef _WIN32
	// 转换separator
	string fileStr(file);
	for (auto& c : fileStr)
	{
		if (c == '\\') c = '/';
	}
	file = fileStr.c_str();
#endif
	state = luaL_newstate();
	if (!state)return;
	if (file) {
		if (luaL_loadfile(state, file)) {
			string pErrorMsg = lua_to_gb18030_string(state, -1);
			console.log(getMsg("strSelfName") + "读取lua文件" + file + "失败:" + pErrorMsg, 0b10);
			lua_close(state);
			state = nullptr;
			return;
		}
		ifstream fs(file);
		if (checkUTF8(fs))UTF8Luas.insert(state);
	}
	luaL_openlibs(state);
	regist();
}

int lua_readStringTable(const char* file, const char* var, std::unordered_map<std::string, std::string>& tab) {
#ifndef _WIN32
	// 转换separator
	string fileStr(file);
	for (auto& c : fileStr)
	{
		if (c == '\\') c = '/';
	}
	file = fileStr.c_str();
#endif
	LuaState L(file);
	if (!L)return -1;
	if (lua_pcall(L, 0, 0, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(getMsg("strSelfName") + "运行lua文件" + file + "失败:" + pErrorMsg, 0b10);
		return 0;
	}
	lua_getglobal(L, var);
	if (lua_type(L, 1) == LUA_TNIL) {
		return 0;
	}
	if (lua_type(L, 1) != LUA_TTABLE) {
		return -2;
	}
	try {
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			if (!lua_isstring(L, -1) || !lua_isstring(L, -2)) {
				return -1;
			}
			string key = lua_to_gb18030_string(L, -2);
			string value = lua_to_gb18030_string(L, -1);
			tab[key] = value;
			lua_pop(L, 1);
		}
		return tab.size();
	} catch (...) {
		return -3;
	}
}