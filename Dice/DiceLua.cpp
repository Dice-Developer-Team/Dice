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
#include "Jsonio.h"

unordered_set<lua_State*> UTF8Luas;
constexpr const char* chDigit{ "0123456789" };
const char* LuaTypes[]{ "nil","boolean","lightuserdata","number","string","table","function","userdata","thread","numtypes" };

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
	void reboot() {
		lua_close(state);
		state = luaL_newstate();
		if (!state)return;
		luaL_openlibs(state);
		regist();
	}
};
double lua_to_number(lua_State* L, int idx = -1) {
	return luaL_checknumber(L, idx);
}
long long lua_to_int(lua_State* L, int idx = -1) {
	return luaL_checkinteger(L, idx);
}
long long lua_to_int_or_zero(lua_State* L, int idx = -1) {
	return lua_isnoneornil(L, idx) ? 0 : luaL_checkinteger(L, idx);
}

// Return string without convert
string lua_to_raw_string(lua_State* L, int idx = -1) {
	return luaL_checkstring(L, idx);
}
// Return a GB18030 string
string lua_to_gbstring(lua_State* L, int idx = -1) {
	return UTF8toGBK(luaL_checkstring(L, idx), !UTF8Luas.count(L));
}

// Return a GB18030 string on Windows and a UTF-8 string on other platforms
string lua_to_native_string(lua_State* L, int idx = -1) {
	const char* str{ luaL_checkstring(L, idx) };
#ifdef _WIN32
	return UTF8toGBK(str, !UTF8Luas.count(L));
#else
	return GBKtoUTF8(str, UTF8Luas.count(L));
#endif
}


void lua_push_string(lua_State* L, const string& str) {
	lua_pushstring(L, UTF8Luas.count(L) ? GBKtoUTF8(str).c_str() : str.c_str());
}
void lua_push_raw_string(lua_State* L, const string& str) {
	lua_pushstring(L, str.c_str());
}
void lua_set_field(lua_State* L, int idx, const string& str) {
	lua_setfield(L, idx, UTF8Luas.count(L) ? GBKtoUTF8(str).c_str() : str.c_str());
}

void lua_push_Context(lua_State* L, AttrObject& vars) {
	AttrObject** p{ (AttrObject**)lua_newuserdata(L, sizeof(AttrObject*)) };
	*p = &vars;
	luaL_setmetatable(L, "Context");
}

static int lua_writer(lua_State* L, const void* b, size_t size, void* B) {
	ByteS* buffer{ (ByteS*)B };
	if (size_t len{ buffer->len }) {
		char* p = new char[len + size + 1];
		memcpy(p, buffer->bytes, len);
		memcpy(p + len, (const char*)b, size);
		delete buffer->bytes;
		buffer->bytes = p;
		buffer->len = len + size;
	}
	else {
		char* p = new char[size + 1];
		memcpy(p, (const char*)b, size);
		buffer->bytes = p;
		buffer->len = size;
	}
	return 0;
}
const char* lua_reader(lua_State* L, void* ud, size_t* sz) {
	(void)L;
	ByteS* s{ (ByteS*)ud };
	*sz = s->len;
	return s->bytes;
}
ByteS lua_to_chunk(lua_State* L, int idx = -1) {
	luaL_checktype(L, idx, LUA_TFUNCTION);
	lua_pushvalue(L, idx);
	ByteS b;
	if (lua_dump(L, lua_writer, &b, 0) != 0) {
		luaL_error(L, "unable to dump given function");
		return {};
	}
	lua_pop(L, 1);
	b.isUTF8 = UTF8Luas.count(L);
	return b;
}
void lua_push_attr(lua_State* L, const AttrVar& attr) {
	switch (attr.type) {
	case AttrVar::AttrType::Boolean:
		lua_pushboolean(L, attr.bit);
		break;
	case AttrVar::AttrType::Integer:
		lua_pushinteger(L, attr.attr);
		break;
	case AttrVar::AttrType::ID:
		lua_pushinteger(L, attr.id);
		break;
	case AttrVar::AttrType::Number:
		lua_pushnumber(L, attr.number);
		break;
	case AttrVar::AttrType::Text:
		lua_push_string(L, attr.text);
		break;
	case AttrVar::AttrType::Table:
		lua_newtable(L);
		if (unordered_set<string> idxs; !attr.table.dict.empty()) {
			int idx{ 0 };
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
		}
		break;
	case AttrVar::AttrType::Nil:
	default:
		lua_pushnil(L);
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
			auto i{ lua_tointeger(L, idx) };
			return (i > 10000000 || i < -10000000) ? AttrVar(i) : AttrVar((int)i);
		}
		else {
			return lua_tonumber(L, idx);
		}
		break;
	case LUA_TSTRING:
		return lua_to_gbstring(L, idx);
		break;
	case LUA_TFUNCTION:
		return lua_to_chunk(L, idx);
		break;
	case LUA_TTABLE:
		AttrVars tab;
		if (idx < 0)idx = lua_gettop(L) + idx + 1;
		lua_pushnil(L);
		while (lua_next(L, idx)) {
			if (lua_type(L, -2) == LUA_TNUMBER) {
				tab[to_string(lua_tointeger(L, -2))] = lua_to_attr(L, -1);
			}
			else {
				tab[lua_to_gbstring(L, -2)] = lua_to_attr(L, -1);
			}
			lua_pop(L, 1);
		}
		return AttrVar(tab);
		break;
	}
	return {};
}
int lua_push_table(lua_State* L, const AttrVars& tab) {
	lua_newtable(L);
	for (auto& [key, val] : tab) {
		val ? lua_push_attr(L, val) : lua_pushnil(L);
		lua_set_field(L, -2, key.c_str());
	}
	return 1;
}
AttrVars lua_to_table(lua_State* L, int idx = -1) {
	if (lua_istable(L, idx)) {
		AttrVars tab;
		if (idx < 0)idx = lua_gettop(L) + idx + 1;
		lua_pushnil(L);
		while (lua_next(L, idx)) {
			if (lua_type(L, -2) == LUA_TNUMBER) {
				tab[to_string(lua_tointeger(L, -2))] = lua_to_attr(L, -1);
			}
			else {
				tab[lua_to_gbstring(L, -2)] = lua_to_attr(L, -1);
			}
			lua_pop(L, 1);
		}
		return tab;
	}
	return {};
}

AttrVars lua_to_dict(lua_State* L, int idx = -1) {
	AttrVars tab;
	if (idx < 0)idx = lua_gettop(L) + idx + 1;
	lua_pushnil(L);
	while (lua_next(L, idx)) {
		if (lua_isstring(L, -2) && !lua_isnil(L, -1)) {
			tab[lua_to_gbstring(L, -2)] = lua_to_attr(L, -1);
		}
		lua_pop(L, 1);
	}
	return tab;
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
		string pErrorMsg = lua_to_gbstring(L, -1);
		console.log(getMsg("strSelfName") + "运行lua文件" + fileGB18030 + "失败:" + pErrorMsg, 0b10);
		return 0;
	}
	lua_getglobal(L, func); 
	lua_push_Context(L, msg->vars);
	if (lua_pcall(L, 1, 2, 0)) {
		string pErrorMsg = lua_to_gbstring(L, -1);
		console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "失败!\n" + pErrorMsg, 0b10);
		msg->reply(getMsg("strReplyLuaErr"));
		return false;
	}
	if (lua_gettop(L) && lua_type(L, 1) != LUA_TNIL) {
		if (!lua_isstring(L, 1)) {
			console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "返回值格式错误(" + LuaTypes[lua_type(L, 1)] + ")!", 1);
			msg->reply(getMsg("strReplyLuaErr"));
			return false;
		}
		if (!(msg->vars["msg_reply"] = lua_to_gbstring(L, 1)).str_empty()) {
			msg->reply(msg->vars["msg_reply"].to_str());
		}
		if (lua_type(L, 2) != LUA_TNIL) {
			if (!lua_isstring(L, 2)) {
				console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "返回值格式错误(" + LuaTypes[lua_type(L, 2)] + ")!", 1);
				return false;
			}
			if (!(msg->vars["msg_hidden"] = lua_to_gbstring(L, 2)).str_empty()) {
				msg->replyHidden(msg->vars["msg_hidden"].to_str());
			}
		}
	}
	return true;
}

//为msg直接调用lua语句回复
bool lua_msg_reply(FromMsg* msg, const AttrVar& lua) {
	string luas{ lua.to_str() };
	bool isFile{ lua.is_character() && fmt->script_has(luas) };
	LuaState L{ isFile ? fmt->script_path(luas).c_str() : nullptr };
	if (!L)return false;
	lua_push_Context(L, msg->vars);
	lua_setglobal(L, "msg");
	if (isFile) {
		if (lua_pcall(L, 0, 2, 0)) {
			string pErrorMsg = lua_to_gbstring(L, -1);
			console.log(getMsg("strSelfName") + "运行lua脚本" + luas + "失败:" + pErrorMsg, 0b10);
			return 0;
		}
	}
	else if (lua.is_function()) {
		ByteS bytes{ lua.to_bytes() };
		if (bytes.isUTF8)UTF8Luas.insert(L);
		if (lua_load(L, lua_reader, &bytes, msg->vars.get_str("reply_title").c_str(), "bt")
			|| lua_pcall(L, 0, 2, 0)) {
			string pErrorMsg = lua_to_gbstring(L, -1);
			console.log(getMsg("strSelfName") + "调用" + msg->vars.get_str("reply_title") + "回复失败!\n" + pErrorMsg, 0b10);
			msg->reply(getMsg("strReplyLuaErr"));
			return false;
		}
	}
	else if (luaL_loadstring(L, luas.c_str()) || lua_pcall(L, 0, 2, 0)) {
		string pErrorMsg = lua_to_gbstring(L, -1);
		console.log(getMsg("strSelfName") + "调用" + msg->vars.get_str("reply_title") + "回复失败!\n" + pErrorMsg, 0b10);
		msg->reply(getMsg("strReplyLuaErr"));
		return false;
	}
	if (!lua_isnoneornil(L, 1)) {
		if (!lua_isstring(L, 1)) {
			console.log(getMsg("strSelfName") + "调用" + msg->vars.get_str("reply_title") + "回复返回值格式错误(" + LuaTypes[lua_type(L, 1)] + ")!", 0b10);
			msg->reply(getMsg("strReplyLuaErr"));
			return false;
		}
		if (!(msg->vars["msg_reply"] = lua_to_gbstring(L, 1)).str_empty()) {
			msg->reply(msg->vars["msg_reply"].to_str());
		}
		if (!lua_isnoneornil(L, 2)) {
			if (!lua_isstring(L, 2)) {
				console.log(getMsg("strSelfName") + "调用" + msg->vars.get_str("reply_title") + "回复返回值格式错误("+ LuaTypes[lua_type(L, 2)] + ")!", 1);
				return false;
			}
			if (!(msg->vars["msg_hidden"] = lua_to_gbstring(L, 2)).str_empty()) {
				msg->replyHidden(msg->vars["msg_hidden"].to_str());
			}
		}
	}
	return true;
}
bool lua_call_event(AttrObject eve, const AttrVar& lua) {
	string luas{ lua.to_str() };
	bool isFile{ lua.is_character() && fmt->script_has(luas) };
	LuaState L{ isFile ? fmt->script_path(luas).c_str() : nullptr };
	if (!L)return false;
	lua_push_Context(L, eve);
	lua_setglobal(L, "event");
	if (isFile) {
		if (lua_pcall(L, 0, 2, 0)) {
			string pErrorMsg = lua_to_gbstring(L, -1);
			console.log(getMsg("strSelfName") + "运行lua脚本" + luas + "失败:" + pErrorMsg, 0b10);
			return 0;
		}
	}
	else if (lua.is_function()) {
		ByteS bytes{ lua.to_bytes() };
		if (bytes.isUTF8)UTF8Luas.insert(L);
		if (lua_load(L, lua_reader, (void*)&bytes, eve.get_str("Type").c_str(), "bt")
			|| lua_pcall(L, 0, 2, 0)) {
			string pErrorMsg = lua_to_gbstring(L, -1);
			console.log(getMsg("strSelfName") + "调用事件lua失败!\n" + pErrorMsg, 0b10);
			return false;
		}
	}
	else if (luaL_loadstring(L, luas.c_str()) || lua_pcall(L, 0, 2, 0)) {
		string pErrorMsg = lua_to_gbstring(L, -1);
		console.log(getMsg("strSelfName") + "调用事件lua失败!\n" + pErrorMsg, 0b10);
		return false;
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
		string pErrorMsg = lua_to_gbstring(L, -1);
		console.log(getMsg("strSelfName") + "运行lua文件" + fileGB18030 + "失败:" + pErrorMsg, 0b10);
		return 0;
	}
	lua_getglobal(L, func);
	if (lua_pcall(L, 0, 0, 0)) {
		string pErrorMsg = lua_to_gbstring(L, -1);
		console.log(getMsg("strSelfName") + "调用" + fileGB18030 + "函数" + func + "失败!\n" + pErrorMsg, 0b10);
		return false;
	}
	return true;
}

/**
 * 供lua调用的类
 */
class SelfData {
	enum FileType { Bin, Json };
	FileType type{ Bin };
	std::filesystem::path pathFile;
public:
	std::mutex exWrite;
	AttrObject data;
	void init(const std::filesystem::path& p) {
		if (p.extension() == ".json")type = SelfData::Json;
		if (std::filesystem::exists(pathFile = p)) {
			switch (type) {
			case SelfData::Json:
				from_json(freadJson(pathFile), *data);
				break;
			case SelfData::Bin:
				if (std::ifstream fs{ pathFile })data.readb(fs);
				break;
			default:
				break;
			}
		}
	}
	void save() {
		std::lock_guard<std::mutex> lock(exWrite);
		switch (type) {
		case SelfData::Json:
			fwriteJson(pathFile,to_json(*data), 0);
			break;
		case SelfData::Bin:
			if (std::ofstream fs{ pathFile })data.writeb(fs);
			break;
		default:
			break;
		}
	}
};
dict<SelfData> selfdata_list;
int SelfData_index(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	SelfData& file{ **(SelfData**)luaL_checkudata(L, 1, "SelfData") };
	string key{ lua_to_gbstring(L, 2) };
	if (file.data.has(key)) {
		lua_push_attr(L, file.data[key]);
		return 1;
	}
	return 0;
}
int SelfData_newindex(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	SelfData& file{ **(SelfData**)luaL_checkudata(L, 1, "SelfData") };
	string key{ lua_to_gbstring(L, 2) };
	if (std::lock_guard<std::mutex> lock(file.exWrite); lua_gettop(L) < 3) {
		file.data.reset(key);
	}
	else if (AttrVar val{ lua_to_attr(L, 3) }; val.is_null()) {
		file.data.reset(key);
	}
	else {
		file.data[key] = val;
	}
	file.save();
	return 0;
}
static const luaL_Reg SelfData_funcs[] = {
	{"__index", SelfData_index},
	{"__newindex", SelfData_newindex},
	{NULL, NULL}
};
int luaopen_SelfData(lua_State* L) {
	luaL_newmetatable(L, "SelfData");
	luaL_setfuncs(L, SelfData_funcs, 0);
	return 1;
}

/**
 * 供lua调用的函数
 */

 //输出日志
int log(lua_State* L) {
	string info{ lua_to_gbstring(L, 1) };
	if (info.empty())return 0;
	int note_lv{ 0 };
	for (int idx = lua_gettop(L); idx > 1; --idx) {
		auto type{ lua_to_int(L,idx) };
		if (type < 0 || type > 9)continue;
		else{
			note_lv |= (1 << type);
		}
	}
	console.log(fmt->format(info), note_lv);
	return 0;
}
 //加载其他lua脚本
int loadLua(lua_State* L) {
	string nameLua{ lua_to_native_string(L, 1) };
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
		console.log("待加载Lua未找到:" + UTF8toGBK(pathFile.u8string()), 1);
		return 0;
	}
	if (luaL_loadstring(L, strLua.c_str())) {
		string pErrorMsg = lua_to_gbstring(L, -1);
		console.log(getMsg("strSelfName") + "读取lua文件" + UTF8toGBK(pathFile.u8string()) + "失败:"+ pErrorMsg, 0b10);
		return 0;
	}
	if (lua_pcall(L, 0, 1, 0)) {
		string pErrorMsg = lua_to_gbstring(L, -1);
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
	string dir{ lua_to_native_string(L, 1) };
	mkDir(dir);
	return 0;
}
int getSelfData(lua_State* L) {
	string file{ lua_to_native_string(L, 1) };
	if (!selfdata_list.count(file)) {
		mkDir(DiceDir / "selfdata");
		selfdata_list[file].init(DiceDir / "selfdata" / file);
	}
	SelfData** p{ (SelfData**)lua_newuserdata(L, sizeof(SelfData*)) };
	*p = &selfdata_list[file];
	luaL_setmetatable(L, "SelfData");
	return 1;
}
int getGroupConf(lua_State* L) {
	int top{ lua_gettop(L) };
	if (top > 3)lua_settop(L, top = 3);
	else if (top < 1)return 0;
	string item;
	if (lua_isstring(L, 2)) {
		if ((item = lua_to_gbstring(L, 2))[0] == '&')item = fmt->format(item);
	}
	if (lua_isnil(L, 1)) {
		if (item.empty())return 0;
		lua_newtable(L);
		for (auto& [uid, data] : UserList) {
			if (data.confs.has(item)) {
				lua_push_attr(L, data.confs[item]);
				lua_set_field(L, -2, to_string(uid));
			}
		}
		return 1;
	}
	long long id{ lua_to_int(L, 1) };
	if (!id)return 0;
	if (item == "size") {
		lua_pushnumber(L, (double)DD::getGroupSize(id).currSize);
	}
	else if (item == "maxsize") {
		lua_pushnumber(L, (double)DD::getGroupSize(id).maxSize);
	}
	else if (item == "members") {
		lua_newtable(L);
		long long i{ 0 };
		for (auto id : DD::getGroupMemberList(id)) {
			lua_push_raw_string(L, to_string(id));
			lua_rawseti(L, -2, ++i);
		}
	}
	else  if (item == "admins") {
		lua_newtable(L);
		long long i{ 0 };
		for (auto id : DD::getGroupAdminList(id)) {
			lua_push_raw_string(L, to_string(id));
			lua_rawseti(L, -2, ++i);
		}
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
		int authDefault{ top == 3 ? (int)lua_to_int(L,3) : 0 };
		lua_pushnumber(L, DD::getGroupAuth(id, uid, authDefault));
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
		if (item.empty()) {
			lua_push_Context(L, grp.confs);
			return 1;
		}
		else if (item == "name") {
			lua_push_string(L, grp.Name = DD::getGroupName(id));
		}
		else if (item == "firstCreate") {
			lua_pushnumber(L, (double)grp.tCreated);
		}
		else if (item == "lastUpdate") {
			lua_pushnumber(L, (double)grp.tUpdated);
		}
		else if (grp.confs.has(item)) {
			lua_push_attr(L, grp.confs[item]);
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
	string item{ lua_to_gbstring(L, 2) };
	if (!id || item.empty())return 0;
	if (item[0] == '&')item = fmt->format(item);
	Chat& grp{ chat(id) };
	if (item.find("card#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		string card{ lua_to_gbstring(L, 3) };
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
	else if (top < 1)return 0;
	string item;
	if (lua_isstring(L, 2)) {
		if ((item = lua_to_gbstring(L, 2))[0] == '&')item = fmt->format(item);
	}
	if (lua_isnil(L, 1)) {
		if (item.empty())return 0;
		lua_newtable(L);
		for (auto& [uid, data] : UserList) {
			if (data.isset(item)) {
				lua_push_attr(L, data.confs[item]);
				lua_set_field(L, -2, to_string(uid));
			}
		}
		return 1;
	}
	long long uid{ lua_to_int(L, 1) };
	if (!uid)return 0;
	if (item == "name") {
		if (string name{ DD::getQQNick(uid) };!name.empty())
			lua_push_string(L, name);
	}
	else if (item == "nick" ) {
		lua_push_string(L, getName(uid));
	}
	else if (item == "trust") {
		lua_pushnumber(L, trustedQQ(uid));
	}
	else if (item.find("nick#") == 0) {
		long long gid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		lua_push_string(L, getName(uid, gid));
	}
	else if (UserList.count(uid)) {
		User& user{ getUser(uid) };
		if (item.empty()) {
			lua_push_Context(L, user.confs);
			return 1;
		}
		else if (item == "firstCreate") {
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
		else if (user.isset(item)) {
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
	string item{ lua_to_gbstring(L, 2) };
	if (item.empty())return 0;
	if (item[0] == '&')item = fmt->format(item);
	if (item == "trust") {
		if (!lua_isnumber(L, 3))return 0;
		int trust{ (int)lua_tonumber(L, 3) };
		if (trust > 4 || trust < 0)return 0;
		User& user{ getUser(uid) };
		if (user.nTrust > 4)return 0;
		user.trust(trust);
	}
	else if (item.find("nn#") == 0) {
		long long gid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		if (lua_isnoneornil(L, 3)) {
			getUser(uid).rmNick(gid);
		}
		else {
			getUser(uid).setNick(gid, lua_to_gbstring(L, 3));
		}
	}
	else if (lua_isnoneornil(L, 3)) {
		getUser(uid).rmConf(item);
	}
	else getUser(uid).setConf(item, lua_to_attr(L, 3));
	return 0;
}
int getUserToday(lua_State* L) {
	int top{ lua_gettop(L) };
	if (top > 3)lua_settop(L, top = 3);
	else if (top < 1)return 0;
	string item;
	if (lua_isstring(L, 2)) {
		if ((item = lua_to_gbstring(L, 2))[0] == '&')item = fmt->format(item);
	}
	if (lua_isnil(L, 1)) {
		if (item.empty())return 0;
		lua_newtable(L);
		for (auto& [uid,data] : today->getUserInfo()) {
			if (data.has(item)) {
				lua_push_attr(L,data[item]);
				lua_set_field(L, -2, to_string(uid));
			}
		}
		return 1;
	}
	long long uid{ lua_to_int(L, 1) };
	if (!uid)return 0;
	if (item.empty()) {
		lua_push_Context(L, today->get(uid));
		return 1;
	}
	else if (item == "jrrp")
		lua_pushnumber(L, today->getJrrp(uid));
	else if (AttrVar* p{ today->get_if(uid, item) })
		lua_push_attr(L, *p);
	else if (top == 3) {
		lua_pushnil(L);
		lua_insert(L, 3);
	}
	else {
		lua_pushinteger(L, 0);
	}
	return 1;
}
int setUserToday(lua_State* L) {
	long long uid{ lua_to_int_or_zero(L, 1) };
	string item{ lua_to_gbstring(L, 2) };
	if (item.empty())return 0;
	if (item[0] == '&')item = fmt->format(item);
	today->set(uid, item, lua_to_attr(L, 3));
	return 0;
}

int getPlayerCardAttr(lua_State* L) {
	if (int argc{ lua_gettop(L) }; argc > 4)lua_settop(L, 4);
	else if (argc == 3)lua_pushnil(L);
	else if (argc < 3)return 0;
	long long plQQ{ lua_to_int(L, 1) };
	long long group{ lua_to_int_or_zero(L, 2) };
	string key{ lua_to_gbstring(L, 3) };
	if (!plQQ || key.empty())return 0;
	CharaCard& pc = getPlayer(plQQ)[group];
	if (pc.Attr.has(key)) {
		lua_push_attr(L, pc.Attr[key]);
	}
	else if (key = pc.standard(key); pc.Attr.has(key)) {
		lua_push_attr(L, pc.Attr[key]);
	}
	else if (key == "note") {
		lua_push_string(L, pc.Note);
	}
	else if (pc.Attr.has("&" + key)) {
		lua_push_string(L, pc.Attr.get_str("&" + key));
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
	long long group{ lua_to_int_or_zero(L, 2) };
	AttrObject** p{ (AttrObject**)lua_newuserdata(L, sizeof(AttrObject*)) };
	*p = &getPlayer(plQQ)[group].Attr;
	luaL_setmetatable(L, "Actor");
	return 1;
}
int setPlayerCardAttr(lua_State* L) {
	long long plQQ{ lua_to_int(L, 1) };
	long long group{ lua_to_int_or_zero(L, 2) };
	string item{ lua_to_gbstring(L, 3) };
	if (!plQQ || item.empty())return 0;
	//参数4为空则视为删除,__Name除外
	CharaCard& pc = getPlayer(plQQ)[group];
	if (item == "__Name") {
		getPlayer(plQQ).renameCard(pc.getName(), lua_to_gbstring(L, 4));
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
	string nameDeck{ lua_to_gbstring(L, 3) };
	if (nameDeck.empty())return 0;
	long long fromGID{ lua_to_int_or_zero(L, 1) };
	long long fromUID{ lua_to_int_or_zero(L, 2) };
	if (fromGID || fromUID) {
		chatInfo fromChat{ fromUID,fromGID };
		if (auto s{ sessions.has_session(fromChat) }) {
			lua_push_string(L, sessions.get_if(fromChat)->deck_draw(nameDeck));
			return 1;
		}
	}
	if (CardDeck::findDeck(nameDeck)) {
		vector<string>& deck = CardDeck::mPublicDeck[nameDeck];
		lua_push_string(L, CardDeck::draw(deck[RandomGenerator::Randint(0, deck.size() - 1)]));
	}
	else {
		lua_push_string(L, "{" + nameDeck + "}");
	}
	return 1;
}

int sendMsg(lua_State* L) {
	int top{ lua_gettop(L) };
	if (top < 1)return 0;
	AttrObject chat;
	if (lua_istable(L, 1)) {
		chat = lua_to_table(L, 1);
	}
	else {
		chat["fwdMsg"] = lua_to_gbstring(L, 1);
		if (top < 2)return 0;
		chat["gid"] = lua_to_int(L, 2);
		if (top >= 3)chat["uid"] = lua_to_int(L, 3);
		if (top >= 4)chat["chid"] = lua_to_int(L, 4);
	}
	if (!chat.get_ll("gid") && !chat.get_ll("uid"))return 0;
	msgtype type{ chat.get_ll("gid") ? msgtype::Group
		: msgtype::Private };
	AddMsgToQueue(fmt->format(chat["fwdMsg"].text, chat),
		{ chat.get_ll("uid"),chat.get_ll("gid") });
	return 0;
}
int eventMsg(lua_State* L) {
	string fromMsg{ lua_to_gbstring(L, 1) };
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

int Msg_echo(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	AttrObject& vars{ **(AttrObject**)luaL_checkudata(L, 1, "Context") };
	string msg{ lua_to_gbstring(L, 2) };
	reply(vars, msg);
	return 0;
}

int Context_index(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	string key{ lua_to_gbstring(L, 2) };
	if (key == "echo") {
		lua_pushcfunction(L, Msg_echo);
		return 1;
	}
	AttrObject& vars{ **(AttrObject**)luaL_checkudata(L, 1, "Context") };
	if (vars.has(key)) {
		lua_push_attr(L, vars[key]);
		return 1;
	}
	else if (key == "fromQQ" || key == "fromUser") {
		if (vars.has("uid")) {
			lua_push_string(L, (vars[key] = vars["uid"].to_str()).to_str());
			return 1;
		}
	}
	else if (key == "fromGroup") {
		if (vars.has("gid")) {
			lua_push_string(L, (vars[key] = vars["gid"].to_str()).to_str());
			return 1;
		}
	}
	return 0;
}
int Context_newindex(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	AttrObject& vars{ **(AttrObject**)luaL_checkudata(L, 1, "Context") };
	string key{ lua_to_gbstring(L, 2) };
	if (lua_gettop(L) < 3) {
		vars.reset(key);
	}
	else if (AttrVar val{ lua_to_attr(L, 3) }; val.is_null()) {
		vars.reset(key);
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
//metatable Actor
int Actor_index(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	string key{ lua_to_gbstring(L, 2) };
	AttrObject& vars{ **(AttrObject**)luaL_checkudata(L, 1, "Actor") };
	if (vars.has(key)) {
		lua_push_attr(L, vars[key]);
		return 1;
	}
	return 0;
}
int Actor_newindex(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	AttrObject& vars{ **(AttrObject**)luaL_checkudata(L, 1, "Actor") };
	string key{ lua_to_gbstring(L, 2) };
	if (key == "__Name") {
		return 0;
	}
	else if (lua_gettop(L) < 3) {
		vars.reset(key);
	}
	else if (AttrVar val{ lua_to_attr(L, 3) }; val.is_null()) {
		vars.reset(key);
	}
	else {
		vars["__Update"] = (long long)time(nullptr);
		vars[key] = val;
	}
	return 0;
}
static const luaL_Reg Actor_funcs[] = {
	{"__index", Actor_index},
	{"__newindex", Actor_newindex},
	{NULL, NULL}
};
int luaopen_Actor(lua_State* L) {
	luaL_newmetatable(L, "Actor");
	luaL_setfuncs(L, Actor_funcs, 0);
	return 1;
}

int httpGet(lua_State* L) {
	string url{ lua_to_raw_string(L,1) };
	if (url.empty()) {
		return 0;
	}
	string ret;
	lua_pushboolean(L, Network::GET(url, ret));
	lua_push_raw_string(L, ret);
	return 2;
}
int httpPost(lua_State* L) {
	if (lua_gettop(L) < 2)return 0;
	string url{ lua_tostring(L,1) };
	string json{ lua_to_raw_string(L,2) };
	if (url.empty() || json.empty()) {
		return 0;
	}
	string type{ lua_gettop(L) > 2 ? lua_tostring(L,3) : "application/json" };
	string ret;
	lua_pushboolean(L, Network::POST(url, json, type, ret));
	lua_push_raw_string(L, ret);
	return 2;
}
int httpUrlEncode(lua_State* L) {
	string url{ lua_to_raw_string(L,1) };
	lua_push_raw_string(L, UrlEncode(url));
	return 1;
}
int httpUrlDecode(lua_State* L) {
	string url{ lua_to_raw_string(L,1) };
	lua_push_raw_string(L, UrlDecode(url));
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
		REGIST(getSelfData)
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
		{"SelfData", luaopen_SelfData},
		{"Actor", luaopen_Actor},
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
			string pErrorMsg = lua_to_gbstring(state, -1);
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
		string pErrorMsg = lua_to_gbstring(L, -1);
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
			string key = lua_to_gbstring(L, -2);
			string value = lua_to_gbstring(L, -1);
			tab[key] = value;
			lua_pop(L, 1);
		}
		return tab.size();
	} catch (...) {
		return -3;
	}
}

void lua_preLoadMod(lua_State* L) {
	lua_newtable(L);
	lua_setglobal(L, "msg_reply");
	lua_newtable(L);
	lua_setglobal(L, "event");
}

void DiceModManager::loadLuaMod(const vector<fs::path>& files, ResList& res) {
	LuaState L;
	UTF8Luas.insert(L);
	ShowList err;
	for (auto file : files) {
		lua_preLoadMod(L);
		if (luaL_dofile(L, getNativePathString(file).c_str())) {
			string pErrorMsg = lua_to_gbstring(L, -1);
			err << pErrorMsg;
			L.reboot();
			continue;
		}
		else {
			lua_getglobal(L, "msg_reply");
			if (!lua_isnoneornil(L, 1)) {
				if (lua_type(L, 1) != LUA_TTABLE) {
					err << "msg_reply数据格式错误:" + UTF8toGBK(file.filename().u8string());
					continue;
				}
				for (auto& [key, val] : lua_to_dict(L)) {
					mod_reply_list[key] = val.to_dict();
				}
			}
			lua_pop(L, 1);
			lua_getglobal(L, "event");
			if (!lua_isnoneornil(L, 1)) {
				if (lua_type(L, 1) != LUA_TTABLE) {
					err << "event数据格式错误:" + UTF8toGBK(file.filename().u8string());
					continue;
				}
				for (auto& [key, val] : lua_to_dict(L)) {
					events[key] = val.to_dict();
				}
			}
		}
	}
	if (!mod_reply_list.empty()) {
		res << "注册reply" + to_string(mod_reply_list.size()) + "项";
		for (const auto& [key, val] : mod_reply_list) {
			if (!key.empty()) {
				ptr<DiceMsgReply> reply{ std::make_shared<DiceMsgReply>() };
				reply->title = key;
				reply->from_obj(val);
				final_msgreply[key] = reply;
			}
		}
		res << "注册event" + to_string(events.size()) + "项";
	}
	if (!err.empty()) {
		res << "mod文件读取错误:" + err.show("\n");
	}
}