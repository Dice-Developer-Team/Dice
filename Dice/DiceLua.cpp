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
	lua_newtable(L);
	//lua_pushnumber(L, (double)msg->fromChat.uid);
	lua_push_string(L, to_string(msg->fromChat.uid));
	lua_setfield(L, -2, "fromQQ");
	//lua_pushnumber(L, (double)msg->fromGID);
	lua_push_string(L, to_string(msg->fromChat.gid));
	lua_setfield(L, -2, "fromGroup");
	lua_push_string(L, msg->strMsg);
	lua_setfield(L, -2, "fromMsg");
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
	}
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
	LuaState L;
	if (!L)return false; 
	//UTF8Luas.insert(L);	//Win输入msg是GBK而调用UTF8文件会导致乱码，因此统一UTF8
	lua_push_msg(L, msg);
	lua_setglobal(L, "msg");
	if (luaL_loadstring(L, luas.c_str()) || lua_pcall(L, 0, 2, 0)) {
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
	lua_getglobal(L, func);
#ifdef _WIN32
	// 转换为GB18030
	string fileGB18030(file);
#else
	string fileGB18030(UTF8toGBK(file, true));
#endif
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

 //加载其他lua脚本
int loadLua(lua_State* L) {
	string nameFile{ lua_to_string(L, 1) };
#ifdef _WIN32 // 转换separator
	for (auto& c : nameFile)
	{
		if (c == '/') c = '\\';
	}
#else
	for (auto& c : nameFile)
	{
		if (c == '\\') c = '/';
	}
#endif
	std::filesystem::path pathFile{ nameFile };
	if ((pathFile.extension() != ".lua") && (pathFile.extension() != ".LUA"))pathFile = nameFile + ".lua";
	if (pathFile.is_relative())pathFile = DiceDir / "plugin" / pathFile;
	if (!std::filesystem::exists(pathFile) && nameFile.find('\\') == string::npos && nameFile.find('/') == string::npos)
		pathFile = DiceDir / "plugin" / nameFile / "init.lua";
	string strLua;
	if (std::filesystem::exists(pathFile)) {
		ifstream fs(pathFile);
		std::stringstream buffer;
		buffer << fs.rdbuf();
		strLua = buffer.str();
		bool isStateUTF8{ (bool)UTF8Luas.count(L) }, isLuaUTF8{ false };	//令文件加载入lua的编码保持一致
		if (checkUTF8(strLua))isLuaUTF8 = true;
		if (isStateUTF8 && !isLuaUTF8) {
			strLua = GBKtoUTF8(strLua);
		}
		else if (!isStateUTF8 && isLuaUTF8) {
			strLua = UTF8toGBK(strLua);
		}
		fs.close();
	}
	else {
		DD::debugLog("待加载Lua文件未找到:"+ UTF8toGBK(pathFile.u8string()));
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
		return 1;
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
	long long id{ lua_to_int(L, 1) };
	string item{ lua_to_gb18030_string(L, 2) };
	if (!id || item.empty())return 0;
	int top{ lua_gettop(L) };
	if (top > 3)lua_settop(L, top = 3);
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
	if (lua_isnil(L, 3)) {
		grp.reset(item);
	}
	else if (lua_isboolean(L,3)) {
		grp.set(item, lua_toboolean(L, 3));
	}
	else if (lua_isinteger(L, 3)) {
		grp.set(item, (int)lua_tonumber(L, 3));
	}
	else if (lua_isnumber(L, 3)) {
		grp.set(item, lua_tonumber(L, 3));
	}
	else if (lua_isstring(L, 3)) {
		grp.set(item, lua_to_gb18030_string(L, 3));
	}
	return 0;
}
int getUserConf(lua_State* L) {
	long long uid{ lua_to_int(L, 1) };
	if (!uid)return 0;
	if (int argc{ lua_gettop(L) }; argc > 3)lua_settop(L, 3);
	else if (argc == 2)lua_pushnil(L);
	else if (argc < 2)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	if (item == "nick" ) {
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
			lua_push_string(L, getName(uid, gid));
		}
		else if (user.confs.count(item)) {
			lua_push_attr(L, user.confs[item]);
		}
		else {
			lua_pushnil(L);
			lua_insert(L, 3);
		}
	}
	else {
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
	if (lua_isnumber(L, 3)) {
		getUser(uid).setConf(item, (int)lua_tonumber(L, 3));
	}
	else if (lua_isstring(L, 3)) {
		getUser(uid).setConf(item, lua_to_gb18030_string(L, 3));
	}
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
	else if (lua_isinteger(L, 4)) {
		pc.set(item, (int)lua_tointeger(L, -1));
	}
	else if (lua_isnumber(L, 4)) {
		pc.set(item, lua_tonumber(L, -1));
	}
	else if (lua_isstring(L, 4)) {
		pc.set(item, lua_to_gb18030_string(L, -1));
	}
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
	AddMsgToQueue(format(fromMsg, GlobalMsg),
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

void LuaState::regist() {
	static const luaL_Reg DiceFucs[] = {
		{"loadLua", loadLua},
		{"getDiceQQ", getDiceQQ},
		{"getDiceDir", getDiceDir},
		{"mkDirs", mkDirs},
		{"getGroupConf", getGroupConf},
		{"setGroupConf", setGroupConf},
		{"getUserConf", getUserConf},
		{"setUserConf", setUserConf},
		{"getUserToday", getUserToday},
		{"setUserToday", setUserToday},
		{"getPlayerCardAttr", getPlayerCardAttr},
		{"setPlayerCardAttr", setPlayerCardAttr},
		{"getPlayerCard", getPlayerCard},
		{"ranint", ranint},
		{"sleepTime", sleepTime},
		{"drawDeck", drawDeck},
		{"sendMsg", sendMsg},
		{"eventMsg", eventMsg},
		{nullptr, nullptr},
	};
	for (const luaL_Reg* lib = DiceFucs; lib->func; lib++) {
		lua_register(state, lib->name, lib->func);
	}
	static const luaL_Reg Dicelibs[] = {
	  {"http", luaopen_http},
	  {NULL, NULL}
	};
	for (const luaL_Reg* lib = Dicelibs; lib->func; lib++) {
		luaL_requiref(state, lib->name, lib->func, 1);
		lua_pop(state, 1);  /* remove lib */
	}
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
LuaState::LuaState(const char* file) {//:isValid(false) {
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
	if (luaL_loadfile(state, file)) {
		string pErrorMsg = lua_to_gb18030_string(state, -1);
		console.log(getMsg("strSelfName") + "读取lua文件" + file + "失败:" + pErrorMsg, 0b10);
		lua_close(state);
		state = nullptr;
		return;
	}
	luaL_openlibs(state);
	regist();
	ifstream fs(file);
	if (checkUTF8(fs))UTF8Luas.insert(state);
	fs.close();
	if (lua_pcall(state, 0, 0, 0)) {
		string pErrorMsg = lua_to_gb18030_string(state, -1);
		console.log(getMsg("strSelfName") + "运行lua文件" + file + "失败:" + pErrorMsg, 0b10);
		lua_close(state);
		state = nullptr;
		return;
	}
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