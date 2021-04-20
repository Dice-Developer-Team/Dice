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

class LuaState {
	lua_State* state;
	//bool isValid;
public:
	LuaState(const char* file);
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
	//lua_pushnumber(L, (double)msg->fromQQ);
	lua_push_string(L, to_string(msg->fromQQ));
	lua_setfield(L, -2, "fromQQ");
	//lua_pushnumber(L, (double)msg->fromGroup);
	lua_push_string(L, to_string(msg->fromGroup));
	lua_setfield(L, -2, "fromGroup");
	lua_push_string(L, msg->strMsg);
	lua_setfield(L, -2, "fromMsg");
}
void CharaCard::pushTable(lua_State* L) {
	lua_newtable(L);
	for (auto& [key, val] : Info) {
		lua_push_string(L, val);
		lua_set_field(L, -2, key.c_str());
	}
	for (auto& [key, val] : Attr) {
		lua_pushnumber(L, val);
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
		console.log(GlobalMsg["strSelfName"] + "调用" + fileGB18030 + "函数" + func + "失败!\n" + pErrorMsg, 1);
		msg->reply(GlobalMsg["strOrderLuaErr"]);
		return false;
	}
	if (lua_gettop(L) && lua_type(L, 1) != LUA_TNIL) {
		if (!lua_isstring(L, 1)) {
			console.log(GlobalMsg["strSelfName"] + "调用" + fileGB18030 + "函数" + func + "返回值格式错误!", 1);
			msg->reply(GlobalMsg["strOrderLuaErr"]);
			return false;
		}
		if (!(msg->strVar["msg_reply"] = lua_to_gb18030_string(L, 1)).empty()) {
			msg->reply(msg->strVar["msg_reply"]);
		}
		if (lua_type(L, 2) != LUA_TNIL) {
			if (!lua_isstring(L, 2)) {
				console.log(GlobalMsg["strSelfName"] + "调用" + fileGB18030 + "函数" + func + "返回值格式错误!", 1);
				return false;
			}
			if (!(msg->strVar["msg_hidden"] = lua_to_gb18030_string(L, 2)).empty()) {
				msg->replyHidden(msg->strVar["msg_hidden"]);
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
		console.log(GlobalMsg["strSelfName"] + "调用" + fileGB18030 + "函数" + func + "失败!\n" + pErrorMsg, 1);
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
#ifndef _WIN32
	// 转换separator
	for (auto& c : nameFile)
	{
		if (c == '\\') c = '/';
	}
#endif
	std::filesystem::path pathFile = DiceDir / "plugin" / (nameFile + ".lua");
	if (!std::filesystem::exists(pathFile) && nameFile.find('\\') == string::npos && nameFile.find('/') == string::npos)
		pathFile = DiceDir / "plugin" / nameFile / "init.lua";
	if (luaL_loadfile(L, pathFile.string().c_str())) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(GlobalMsg["strSelfName"] + "读取lua文件" + UTF8toGBK(pathFile.u8string()) + "失败:"+ pErrorMsg, 0b10);
		return 0;
	}
	if (lua_pcall(L, 0, 1, 0)) {
		string pErrorMsg = lua_to_gb18030_string(L, -1);
		console.log(GlobalMsg["strSelfName"] + "运行lua文件" + UTF8toGBK(pathFile.u8string()) + "失败:"+ pErrorMsg, 0b10);
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
	Chat& grp{ chat(id) };
	if (item == "name") {
		lua_push_string(L, grp.Name);
	}
	else if (mChatConf.count(item)) {
		lua_pushboolean(L, grp.boolConf.count(item));
	}
	else if (grp.intConf.count(item)) {
		lua_pushnumber(L, (double)grp.intConf[item]);
	}
	else if (grp.strConf.count(item)) {
		lua_push_string(L, grp.strConf[item]);
	}
	else {
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
	if (mChatConf.count(item)) {
		lua_toboolean(L, 3) ? grp.set(item) : grp.reset(item);
	}
	else if (lua_isnumber(L, 3)) {
		grp.setConf(item, (int)lua_tonumber(L, 3));
	}
	else if (lua_isstring(L, 3)) {
		grp.setText(item, lua_to_gb18030_string(L, 3));
	}
	return 0;
}
int getUserConf(lua_State* L) {
	long long qq{ lua_to_int(L, 1) };
	if (!qq)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	User& user{ getUser(qq) };
	if (item == "nick" ) {
		lua_push_string(L, getName(qq));
	}
	else if (item == "trust") {
		lua_pushnumber(L, trustedQQ(qq));
	}
	else if (user.intConf.count(item)) {
		lua_pushnumber(L, (double)user.intConf[item]);
	}
	else if (user.strConf.count(item)) {
		lua_push_string(L, user.strConf[item]);
	}
	else {
		lua_pushnil(L);
		lua_insert(L, 3);
	}
	return 1;
}
int setUserConf(lua_State* L) {
	long long qq{ lua_to_int(L, 1) };
	if (!qq)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	if (lua_isnumber(L, 3)) {
		getUser(qq).setConf(item, (int)lua_tonumber(L, 3));
	}
	else if (lua_isstring(L, 3)) {
		getUser(qq).setConf(item, lua_to_gb18030_string(L, 3));
	}
	return 0;
}
int getUserToday(lua_State* L) {
	long long qq{ lua_to_int(L, 1) };
	if (!qq)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	if (item == "jrrp")
		lua_pushnumber(L, today->getJrrp(qq));
	else
		lua_pushnumber(L, today->get(qq, item));
	return 1;
}
int setUserToday(lua_State* L) {
	long long qq{ lua_to_int(L, 1) };
	if (!qq)return 0;
	string item{ lua_to_gb18030_string(L, 2) };
	if (item.empty())return 0;
	int val{ (int)lua_to_number(L, 3) };
	today->set(qq, item, val);
	return 0;
}

int getPlayerCardAttr(lua_State* L) {
	long long plQQ{ lua_to_int(L, 1) };
	long long group{ lua_to_int(L, 2) };
	string key{ lua_to_gb18030_string(L, 3) };
	if (!plQQ || key.empty())return 0;
	CharaCard& pc = getPlayer(plQQ)[group];
	if (pc.Info.count(key)) {
		lua_push_string(L, pc.Info.find(key)->second);
		return 3;
	}
	else if (key == "note") {
		lua_push_string(L, pc.Note);
		return 2;
	}
	else if (pc.DiceExp.count(key)) {
		lua_push_string(L, pc.DiceExp.find(key)->second);
	}
	else if (key = pc.standard(key); pc.Attr.count(key)) {
		lua_pushnumber(L, (double)pc.Attr.find(key)->second);
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
	else if (lua_isnumber(L, 4)) {
		pc.set(item, (int)lua_tonumber(L, -1));
	}
	else if (lua_isstring(L, 4)) {
		if (item[0] == '&')pc.setExp(item.substr(1), lua_to_gb18030_string(L, -1));
		else pc.setInfo(item, lua_to_gb18030_string(L, -1));
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
	long long fromGroup{ lua_to_int(L, 1) };
	long long fromQQ{ lua_to_int(L, 2) };
	if (!fromGroup && !fromQQ)return 0;
	string nameDeck{ lua_to_gb18030_string(L, 3) };
	if (nameDeck.empty())return 0;
	long long fromSession{ fromGroup ? fromGroup : ~fromQQ };
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
	long long fromGroup{ lua_to_int(L, 2) };
	long long fromQQ{ lua_to_int(L, 3) };
	if (!fromGroup && !fromQQ)return 0;
	msgtype type{ fromGroup ?
		chat(fromGroup).isGroup ? msgtype::Group : msgtype::Discuss
		: msgtype::Private };
	AddMsgToQueue(format(fromMsg, GlobalMsg), 
				  fromGroup ? fromGroup : fromQQ, type);
	return 0;
}
int eventMsg(lua_State* L) {
	string fromMsg{ lua_to_gb18030_string(L, 1) };
	long long fromGroup{ lua_to_int(L, 2) };
	long long fromQQ{ lua_to_int(L, 3) };
	msgtype type{ fromGroup ?
		chat(fromGroup).isGroup ? msgtype::Group : msgtype::Discuss
		: msgtype::Private };
	FromMsg* Msg = new FromMsg(fromGroup ? FromMsg(fromMsg, fromGroup, type, fromQQ)
							   : FromMsg(fromMsg, fromQQ));
	std::thread th(*Msg);
	th.detach();
	return 0;
}

void LuaState::regist() {
	const luaL_Reg Dicelibs[] = {
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
	for (const luaL_Reg* lib = Dicelibs; lib->func; lib++) {
		lua_register(state, lib->name, lib->func);
	}
	lua_getglobal(state, "package");
	lua_getfield(state, -1, "path");
	string strPath((DiceDir / "plugin" / "?.lua").u8string() + lua_tostring(state, -1));
	lua_pushstring(state, strPath.c_str());
	lua_setfield(state, -3, "path");
	lua_pop(state, 2);
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
		console.log(GlobalMsg["strSelfName"] + "读取lua文件" + file + "失败:" + pErrorMsg, 0b10);
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
		console.log(GlobalMsg["strSelfName"] + "运行lua文件" + file + "失败:" + pErrorMsg, 0b10);
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