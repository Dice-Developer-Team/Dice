#include <memory>
#include "filesystem.hpp"
#include "DiceMsgSend.h"
#include "DiceJS.h"
#include "DiceQJS.h"
#include "DiceEvent.h"
#include "DiceMod.h"
#include "DiceSelfData.h"
#include "DiceSession.h"
#include "CharacterCard.h"
#include "DDAPI.h"
#define countof(x) (sizeof(x) / sizeof((x)[0]))

JSRuntime* rt{ nullptr };
int js_toInt(JSContext* ctx, JSValueConst val) {
	int32_t num = 0;
	JS_ToInt32(ctx, &num, val);
	return (int)num;
}
string js_toGBK(JSContext* ctx, JSValue val) {
	auto s{ JS_ToCString(ctx, val) };
	string ret{ UTF8toGBK(s) };
	JS_FreeCString(ctx, s);
	return ret;
}
string js_toNativeString(JSContext* ctx, JSValue val) {
	auto s{ JS_ToCString(ctx, val) };
#ifdef _WIN32
	string ret{ UTF8toGBK(s) };
#else
	string ret{ s };
#endif
	JS_FreeCString(ctx, s);
	return ret;
}
string js_NativeToGBK(JSContext* ctx, JSValue val) {
	auto s{ JS_ToCString(ctx, val) };
#ifdef _WIN32
	string ret{ s };
#else
	string ret{ UTF8toGBK(s) };
#endif
	JS_FreeCString(ctx, s);
	return ret;
}
AttrIndex js_toAttrIndex(JSContext* ctx, JSValue val) {
	if (JS_IsNumber(val)) {
		double key{ js_toDouble(ctx,val) };
		return key;
	}
	else {
		string key{ js_toGBK(ctx, val) };
		return key;
	}
}
AttrIndex js_AtomToIndex(JSContext* ctx, JSAtom atom) {
	auto val = JS_AtomToValue(ctx, atom);
	if(JS_IsNumber(val)){
		double key{ js_toDouble(ctx,val) };
		JS_FreeValue(ctx, val);
		return key;
	}
	else {
		string key{ js_toGBK(ctx, val) };
		JS_FreeValue(ctx, val);
		return key;
	}
}
string js_AtomtoGBK(JSContext* ctx, JSAtom val) {
	auto s{ JS_AtomToCString(ctx, val) };
	string ret{ UTF8toGBK(s) };
	JS_FreeCString(ctx, s);
	return ret;
}
JSValue js_newGBK(JSContext* ctx, const string& val) {
	return JS_NewString(ctx, GBKtoUTF8(val).c_str());
}
js_context::js_context() : ctx(JS_NewContext(rt)) {
	js_std_add_helpers(ctx, 0, NULL);
	/* system modules */
	js_init_module_std(ctx, "std");
	js_init_module_os(ctx, "os");
	auto global = JS_GetGlobalObject(ctx);
	JS_SetPropertyFunctionList(ctx, global, js_dice_funcs, countof(js_dice_funcs));
	//Set
	JSValue pro_set = JS_NewObject(ctx);
	auto set_ctor = JS_NewCFunction2(ctx, js_dice_Set_constructor, "Set", 0,
		JS_CFUNC_constructor, 0);
	JS_SetPropertyStr(ctx, global, "Set", set_ctor);
	JS_SetConstructor(ctx, set_ctor, pro_set);
	JS_SetClassProto(ctx, js_dice_Set_id, pro_set);
	//Context
	JSValue pro_context = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, pro_context, js_dice_Context_proto_funcs,
		countof(js_dice_Context_proto_funcs));
	JS_SetClassProto(ctx, js_dice_context_id, pro_context);
	//SelfData
	JSValue pro_selfdata = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, pro_selfdata, js_dice_selfdata_proto_funcs,
		countof(js_dice_selfdata_proto_funcs));
	auto selfdata_ctor = JS_NewCFunction2(ctx, js_dice_selfdata_constructor, "SelfData", 1,
		JS_CFUNC_constructor, 0);
	JS_SetPropertyStr(ctx, global, "SelfData", selfdata_ctor);
	JS_SetConstructor(ctx, selfdata_ctor, pro_selfdata);
	JS_SetClassProto(ctx, js_dice_selfdata_id, pro_selfdata);
	//GameTable
	JSValue pro_game = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, pro_game, js_dice_GameTable_proto_funcs,
		countof(js_dice_GameTable_proto_funcs));
	JS_SetClassProto(ctx, js_dice_GameTable_id, pro_game);
	//Actor
	JSValue pro_actor = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, pro_actor, js_dice_actor_proto_funcs,
		countof(js_dice_actor_proto_funcs));
	JS_SetClassProto(ctx, js_dice_actor_id, pro_actor);
	JS_FreeValue(ctx, global);
}
js_context::~js_context() {
	JS_FreeContext(ctx);
}
AttrVar js_toAttr(JSContext* ctx, JSValue val) {
	switch (auto tag{ JS_VALUE_GET_TAG(val) }) {
	case JS_TAG_BOOL:
		return JS_VALUE_GET_INT(val) != 0;
		break;
	case JS_TAG_INT:
		return JS_VALUE_GET_INT(val);
		break;
	case JS_TAG_BIG_INT:
		return js_toLongLong(ctx, val);
		break;
	case JS_TAG_BIG_FLOAT:
	case JS_TAG_FLOAT64:
		if (auto d = js_toDouble(ctx, val); d == (long long)d)return (long long)d;
		else return d;
		break;
	case JS_TAG_STRING:
		return js_toGBK(ctx, val);
		break;
	case JS_TAG_OBJECT:
		if (void* p{ nullptr }; JS_GetClassID(val, &p) == js_dice_context_id) {
			return *(ptr<AnysTable>*)p;
		}
		else if (JSClassID jid{ JS_GetClassID(val, &p) }; jid == js_dice_actor_id) {
			return *(PC*)p;
		}
		else if (jid == js_dice_GameTable_id) {
			return *(ptr<Session>*)p;
		}
		else if (jid == js_dice_Set_id) {
			return *(AttrSet*)p;
		}
		else if (JS_IsArray(ctx, val)) {
			VarArray ary;
			auto p = JS_VALUE_GET_OBJ(val);
			auto len = (uint32_t)JS_VALUE_GET_INT(JS_GetPropertyStr(ctx, val, "length"));
			for (uint32_t i = 0; i < len; i++)
				ary.emplace_back(js_toAttr(ctx, JS_GetPropertyUint32(ctx,val,i)));
			return AnysTable(ary);
		}
		else {
			AttrObject obj;
			JSPropertyEnum* tab{ nullptr };
			uint32_t len = 0;
			if (!JS_GetOwnPropertyNames(ctx, &tab, &len, val, JS_GPN_ENUM_ONLY | JS_GPN_STRING_MASK)) {
				for (uint32_t i = 0; i < len; ++i) {
					obj->set(js_AtomtoGBK(ctx, tab[i].atom), js_toAttr(ctx, JS_GetProperty(ctx, val, tab[i].atom)));
				}
				js_free_prop_enum(ctx, tab, len);
			}
			return obj;
		}
		break;
	case JS_TAG_UNINITIALIZED:
	case JS_TAG_NULL:
	case JS_TAG_EXCEPTION:
	case JS_TAG_UNDEFINED:
		return {};
	default:
		console.log("js_value_tag:" + to_string(JS_VALUE_GET_TAG(val)), 0);
		return {};
	}
	return {};
}
static JSValue js_newDiceContext(JSContext* ctx, const ptr<AnysTable>& context) {
	auto obj = JS_NewObjectClass(ctx, js_dice_context_id);
	JS_SetOpaque(obj, new ptr<AnysTable>(context));
	return obj;
}
static JSValue js_newGameTable(JSContext* ctx, const ptr<DiceSession>& p) {
	auto obj = JS_NewObjectClass(ctx, js_dice_GameTable_id);
	JS_SetOpaque(obj, new ptr<DiceSession>(p));
	return obj;
}
static JSValue js_newActor(JSContext* ctx, const PC& p) {
	auto obj = JS_NewObjectClass(ctx, js_dice_actor_id);
	JS_SetOpaque(obj, new PC(p));
	return obj;
}
JSValue js_newAttr(JSContext* ctx, const AttrVar& var) {
	switch (var.type) {
	case AttrVar::Type::Boolean:
		return JS_NewBool(ctx, var.bit);
		break;
	case AttrVar::Type::Integer:
		return JS_NewInt32(ctx, (int32_t)var.attr);
		break;
	case AttrVar::Type::Number:
		return JS_NewFloat64(ctx, var.number);
		break;
	case AttrVar::Type::Text:
		return JS_NewString(ctx, GBKtoUTF8(var.text).c_str());
		break;
	case AttrVar::Type::Table:
		if (auto t{ var.table->getType() }; t == AnysTable::MetaType::Context) {
			return js_newDiceContext(ctx, var.table.p);
		}
		else if (t == AnysTable::MetaType::Actor) {
			return js_newActor(ctx, std::static_pointer_cast<CharaCard>(var.table.p));
		}
		else if (t == AnysTable::MetaType::Game) {
			return js_newGameTable(ctx, std::static_pointer_cast<Session>(var.table.p));
		}
		else if (!var.table->as_dict().empty()) {
			auto dict = JS_NewObject(ctx);
			if (var.table->to_list()) {
				uint32_t idx{ 0 };
				for (auto& val : *var.table->to_list()) {
					JS_SetPropertyUint32(ctx, dict, idx++, js_newAttr(ctx, val));
				}
			}
			for (auto& [key, val] : var.table->as_dict()) {
				JS_SetPropertyStr(ctx, dict, GBKtoUTF8(key).c_str(), js_newAttr(ctx, val));
			}
			return dict;
		}
		else if (var.table->to_list()) {
			auto ary = JS_NewArray(ctx);
			uint32_t idx{ 0 };
			for (auto& val : *var.table->to_list()) {
				JS_SetPropertyUint32(ctx, ary, idx++, js_newAttr(ctx, val));
			}
			return ary;
		}
		break;
	case AttrVar::Type::ID:
		return JS_NewInt64(ctx, (int64_t)var.id);
		break;
	case AttrVar::Type::Set:
		if (auto obj = JS_NewObjectClass(ctx, js_dice_Set_id)) {
			AttrSet* p = new AttrSet(var.flags);
			JS_SetOpaque(obj, p);
			return obj;
		}
		break;
	case AttrVar::Type::Function:
	case AttrVar::Type::Nil:
	default:
		break;
	}
	return JS_UNDEFINED;
}
string js_context::getException() {
	auto e{ JS_GetException(ctx) };
	auto err{ js_toGBK(ctx, e) };
	if (JS_IsNull(e) || JS_IsUndefined(e)) goto Free;
	if (JSValue stack = JS_GetPropertyStr(ctx, e, "stack"); !JS_IsException(stack)) {
		err += "\n" + js_toGBK(ctx, stack);
	}
Free:
	JS_FreeValue(ctx, e);
	return err;
}
AttrVar js_context::getValue(JSValue val) {
	return js_toAttr(ctx, val);
}
void js_context::setContext(const std::string& name, const AttrObject& context) {
	auto global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, name.c_str(), js_newDiceContext(ctx, context.p));
	JS_FreeValue(ctx, global);
}
JSValue js_context::evalString(const std::string& s, const string& title) {
	string exp{ GBKtoUTF8(s) };
	return JS_Eval(ctx, exp.c_str(), exp.length(), GBKtoUTF8(title).c_str(), JS_EVAL_TYPE_GLOBAL);
}
JSValue js_context::evalStringLocal(const std::string& s, const string& title, const AttrObject& context) {
	string exp{ GBKtoUTF8(s) };
	return JS_EvalThis(ctx, js_newDiceContext(ctx, context.p), exp.c_str(), exp.length(), GBKtoUTF8(title).c_str(), JS_EVAL_TYPE_GLOBAL);
}
JSValue js_context::evalFile(const std::filesystem::path& p) {
	size_t buf_len{ 0 };
	if (uint8_t * buf{ js_load_file(ctx, &buf_len, p.string().c_str())}) {
		auto ret = JS_Eval(ctx, (char*)buf, buf_len, p.u8string().c_str(),
			JS_EVAL_TYPE_GLOBAL);
		js_free(ctx, buf);
		return ret;
	}
	return JS_EXCEPTION;
}
JSValue js_context::evalFileLocal(const std::string& s, const AttrObject& context) {
	size_t buf_len{ 0 };
	if (uint8_t * buf{ js_load_file(ctx, &buf_len, s.c_str()) }) {
		auto ret = JS_EvalThis(ctx, js_newDiceContext(ctx, context.p), (char*)buf, buf_len, s.c_str(),
			JS_EVAL_TYPE_MODULE);
		js_free(ctx, buf);
		return ret;
	}
	return JS_EXCEPTION;
}
std::unique_ptr<js_context> js_main;
dict<js_context> js_event_pool;
void js_global_init() {
	rt = JS_NewRuntime();
	js_std_init_handlers(rt);
	/* loader for ES6 modules */
	JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
	JS_NewClassID(&js_dice_Set_id);
	JS_NewClass(rt, js_dice_Set_id, &js_dice_Set_class);
	JS_NewClassID(&js_dice_context_id);
	JS_NewClass(rt, js_dice_context_id, &js_dice_context_class);
	JS_NewClassID(&js_dice_selfdata_id);
	JS_NewClass(rt, js_dice_selfdata_id, &js_dice_selfdata_class);
	JS_NewClassID(&js_dice_GameTable_id);
	JS_NewClass(rt, js_dice_GameTable_id, &js_dice_GameTable_class);
	JS_NewClassID(&js_dice_actor_id);
	JS_NewClass(rt, js_dice_actor_id, &js_dice_actor_class);
	js_main = std::make_unique<js_context>();
}
void js_global_end() {
	js_std_free_handlers(rt);
	JS_FreeRuntime(rt);
}
JSValue js_newSet(JSContext* ctx, const AttrSet& set) {
	auto obj = JS_NewObjectClass(ctx, js_dice_Set_id);
	AttrSet* p = new AttrSet(set);
	JS_SetOpaque(obj, p);
	return obj;
}
QJSDEF(Set_constructor) {
	auto obj = JS_NewObjectClass(ctx, js_dice_Set_id);
	AttrSet* p = new AttrSet(std::make_shared<fifo_set<AttrIndex>>());
	JS_SetOpaque(obj, p);
	for (int idx = 0; idx < argc; ++idx) {
		(*p)->insert(js_toAttrIndex(ctx, argv[idx]));
	}
	return obj;
}
void js_dice_Set_finalizer(JSRuntime* rt, JSValue val) {
	JS2SET(val);
	delete &set;
}
QJSDEF(Set_in) {
	JS2SET(this_val);
	return set->count(js_toAttrIndex(ctx, argv[0])) ? JS_TRUE : JS_FALSE;
}
QJSDEF(Set_add) {
	JS2SET(this_val);
	return set->insert(js_toAttrIndex(ctx, argv[0])).second ? JS_TRUE : JS_FALSE;
}
QJSDEF(Set_toArray) {
	JS2SET(this_val);
	auto items = JS_NewArray(ctx);
	int64_t i = 0;
	for (auto& elem : *set) {
		JS_SetPropertyInt64(ctx, items, i++, js_newAttr(ctx, AttrVar(elem.val)));
	}
	return items;
}
int js_dice_Set_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop) {
	JS2SET(obj);
	if (desc) {
		string key{ js_AtomtoGBK(ctx, prop) };
		if (key == "add") {
			desc->flags = JS_PROP_NORMAL;
			desc->value = JS_NewCFunction(ctx, js_dice_Set_add, "add", 1);
		}
		else if (key == "in") {
			desc->flags = JS_PROP_NORMAL;
			desc->value = JS_NewCFunction(ctx, js_dice_Set_in, "in", 1);
		}
		else if (key == "toArray") {
			desc->flags = JS_PROP_NORMAL;
			desc->value = JS_NewCFunction(ctx, js_dice_Set_toArray, "toArray", 0);
		}
		else if (set->count(key)) {
			desc->flags = JS_PROP_ENUMERABLE;
			desc->value = JS_TRUE;
		}
		else {
			desc->flags = JS_PROP_NORMAL;
			desc->value = JS_FALSE;
		}
		desc->getter = JS_UNDEFINED;
		desc->setter = JS_UNDEFINED;
		return TRUE;
	}
	return FALSE;
}
int js_dice_Set_delete(JSContext* ctx, JSValue obj, JSAtom atom) {
	JS2SET(obj);
	return set->erase(js_AtomToIndex(ctx, atom)) != set->end() ? JS_TRUE : JS_FALSE;
}

QJSDEF(log) {
	string info{ js_toGBK(ctx, argv[0]) };
	int note_lv{ 0 };
	int type{ 0 };
	for (int idx = 1; idx < argc; ++idx) {
		if (!JS_ToInt32(ctx, &type, argv[idx])) {
			if (type >= 0 && type < 10) {
				note_lv |= (1 << type);
			}
		}
		else if (JS_IsString(argv[idx])) {
			console.log(fmt->format(info), js_toNativeString(ctx, argv[idx]));
			return JS_TRUE;
		}
		else return JS_EXCEPTION;
	}
	console.log(fmt->format(info), note_lv);
	return JS_TRUE;
}
QJSDEF(loadJS) {
	if (argc > 0) {
		string nameJS{ js_toNativeString(ctx,argv[0]) };
		std::filesystem::path pathFile{ nameJS };
		if (fmt->has_js(nameJS)) {
			pathFile = fmt->js_path(nameJS);
		}
		else {
			if (pathFile.extension() != ".js")pathFile = nameJS + ".js";
			if (pathFile.is_relative())pathFile = dirExe / "Diceki" / "js" / pathFile;
		}
		size_t buf_len{ 0 };
		if (uint8_t * buf{ js_load_file(ctx, &buf_len, pathFile.string().c_str()) }) {
			auto ret = JS_EvalThis(ctx, this_val, (char*)buf, buf_len, nameJS.c_str(),
				JS_EVAL_TYPE_MODULE);
			js_free(ctx, buf);
			return ret;
		}
		else {
			JS_ThrowReferenceError(ctx, "could not load '%s'", pathFile.u8string().c_str());
			return JS_EXCEPTION;
		}
	}
	else {
		JS_ThrowTypeError(ctx, "undefined js file");
		return JS_EXCEPTION;
	}
}
QJSDEF(getDiceID) {
	return JS_NewInt64(ctx, (int64_t)console.DiceMaid);
}
QJSDEF(getDiceDir) {
	return JS_NewString(ctx, DiceDir.u8string().c_str());;
}
QJSDEF(getSelfData) {
	string file{ js_toGBK(ctx, argv[0])};
	if (!selfdata_byFile.count(file)) {
		auto& data{ selfdata_byFile[file] = std::make_shared<SelfData>(DiceDir / "selfdata" / file) };
		if (string name{ cut_stem(file) }; !selfdata_byStem.count(name)) {
			selfdata_byStem[name] = data;
		}
	}
	auto data = JS_NewObjectClass(ctx, js_dice_selfdata_id);
	JS_SetOpaque(data, new ptr<SelfData>(selfdata_byFile[file]));
	return data;
}
QJSDEF(eventMsg) {
	AttrObject eve;
	if (JS_IsObject(argv[0])) {
		eve = js_toAttr(ctx, argv[0]).to_obj();
	}
	else {
		if (JS_IsUndefined(argv[2])) {
			JS_ThrowSyntaxError(ctx, "uid can't be 0");
			return JS_EXCEPTION;
		}
		string fromMsg{ js_toGBK(ctx, argv[0]) };
		long long gid{ JS_IsUndefined(argv[1]) ? 0 : js_toLongLong(ctx, argv[1]) },
			uid{ JS_IsUndefined(argv[2]) ? 0 : js_toLongLong(ctx, argv[2]) };
		eve = gid
			? AnysTable{ {{"fromMsg",fromMsg},{"gid",gid}, {"uid", uid}} }
		: AnysTable{ {{"fromMsg",fromMsg}, {"uid", uid}} };
	}
	std::thread th([=]() {
		DiceEvent(*eve.p).virtualCall();
		});
	th.detach();
	return JS_TRUE;
}
QJSDEF(sendMsg) {
	AttrObject chat;
	if (JS_IsObject(argv[0])) {
		chat = js_toAttr(ctx, argv[0]).to_obj();
		AddMsgToQueue(fmt->format(chat->get_str("fwdMsg"), chat),
			chatInfo{ chat->get_ll("uid") ,chat->get_ll("gid") ,chat->get_ll("chid") });
	}
	else {
		long long gid{ JS_IsUndefined(argv[1]) ? 0 : js_toLongLong(ctx, argv[1]) },
			uid{ JS_IsUndefined(argv[2]) ? 0 : js_toLongLong(ctx, argv[2]) };
		if (!gid && !uid) {
			JS_ThrowSyntaxError(ctx, "gid and uid are both 0 is invalid");
			return JS_EXCEPTION;
		}
		string msg{ js_toGBK(ctx, argv[0]) };
		if (uid)chat->set("uid", uid);
		if (gid)chat->set("gid", gid);
		AddMsgToQueue(fmt->format(msg, chat), { uid,gid,0 });
	}
	return JS_TRUE;
}
QJSDEF(getGroupAttr) {
	string item{ js_toGBK(ctx, argv[1]) };
	if (item[0] == '&')item = fmt->format(item);
	if (JS_IsUndefined(argv[0])) {
		if (item.empty()) {
			JS_ThrowSyntaxError(ctx, "neither gid and prop is undefined");
			return JS_EXCEPTION;
		}
		auto items{ JS_NewArray(ctx) };
		for (auto& [gid, data] : ChatList) {
			if (data->has(item))JS_SetPropertyInt64(ctx, items, (int64_t)gid, js_newAttr(ctx, data->get(item)));
		}
		return items;
	}
	if (long long gid{ js_toLongLong(ctx,argv[0]) }) {
		if (item.empty()) return js_newDiceContext(ctx, chat(gid).shared_from_this());
		else if (item == "members") {
			auto items{ JS_NewArray(ctx) };
			uint32_t i = 0;
			if (JS_IsUndefined(argv[2])) {
				for (auto uid : DD::getGroupMemberList(gid)) {
					JS_SetPropertyUint32(ctx, items, i++, JS_NewInt64(ctx, (int64_t)uid));
				}
				return items;
			}
			else {
				string subitem{ js_toGBK(ctx, argv[2]) };
				if (subitem == "card") {
					for (auto uid : DD::getGroupMemberList(gid)) {
						JS_SetPropertyUint32(ctx, items, uid, js_newGBK(ctx, DD::getGroupNick(gid, uid)));
					}
				}
				else if (subitem == "lst") {
					for (auto uid : DD::getGroupMemberList(gid)) {
						if (auto lst{ DD::getGroupLastMsg(gid, uid) }) {
							JS_SetPropertyUint32(ctx, items, uid, JS_NewInt64(ctx, (int64_t)lst));
						}
						else JS_SetPropertyUint32(ctx, items, uid, JS_FALSE);
					}
				}
				else if (subitem == "auth") {
					for (auto uid : DD::getGroupMemberList(gid)) {
						if (auto auth{ DD::getGroupAuth(gid, uid, 0) }) {
							JS_SetPropertyUint32(ctx, items, uid, JS_NewInt64(ctx, (int64_t)auth));
						}
						else JS_SetPropertyUint32(ctx, items, uid, JS_FALSE);
					}
				}
				return items;
			}
		}
		else if (item == "admins") {
			auto items{ JS_NewArray(ctx) };
			uint32_t i = 0;
			for (auto uid : DD::getGroupAdminList(gid)) {
				JS_SetPropertyUint32(ctx, items, i++, JS_NewInt64(ctx, (int64_t)uid));
			}
			return items;
		}
		if (auto val{ getGroupItem(gid,item) }; !val.is_null())return js_newAttr(ctx, val);
		else return argv[2];
	}
	JS_ThrowSyntaxError(ctx, "gid can't be 0");
	return JS_EXCEPTION;
}
QJSDEF(setGroupAttr) {
	if (JS_IsUndefined(argv[0])) {
		JS_ThrowSyntaxError(ctx, "uid can't be 0");
		return JS_EXCEPTION;
	}
	string item{ js_toGBK(ctx, argv[1]) };
	if (item[0] == '&')item = fmt->format(item);
	if (item.empty()) {
		JS_ThrowSyntaxError(ctx, "prop can't be empty");
		return JS_EXCEPTION;
	}
	long long gid = js_toLongLong(ctx, argv[0]);
	Chat& grp{ chat(gid) };
	if (item.find("card#") == 0) {
		long long uid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			uid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		string card{ js_toGBK(ctx, argv[2]) };
		DD::setGroupCard(gid, uid, card);
	}
	else if (!JS_IsUndefined(argv[2])) {
		grp.set(item, js_toAttr(ctx, argv[2]));
	}
	else grp.reset(item);
	return JS_TRUE;
}
QJSDEF(getUserAttr) {
	string item{ js_toGBK(ctx, argv[1]) };
	if (item[0] == '&')item = fmt->format(item);
	if (JS_IsUndefined(argv[0])) {
		if (item.empty()) {
			JS_ThrowSyntaxError(ctx, "neither uid and prop is undefined");
			return JS_EXCEPTION;
		}
		auto items{ JS_NewArray(ctx) };
		for (auto& [id, data] : UserList) {
			if (data->has(item))JS_SetPropertyInt64(ctx, items, (int64_t)id, js_newAttr(ctx, data->get(item)));
		}
		return items;
	}
	if (long long uid{ js_toLongLong(ctx,argv[0]) }) {
		if (item.empty()) return js_newDiceContext(ctx, getUser(uid).shared_from_this());
		if (auto val{ getUserItem(uid,item) }; !val.is_null())return js_newAttr(ctx, val);
		else return argv[2];
	}
	JS_ThrowSyntaxError(ctx, "uid can't be 0");
	return JS_EXCEPTION;
}
QJSDEF(setUserAttr) {
	if (JS_IsUndefined(argv[0])) {
		JS_ThrowSyntaxError(ctx, "uid can't be 0");
		return JS_EXCEPTION;
	}
	long long uid = js_toLongLong(ctx, argv[0]);
	string item{ js_toGBK(ctx, argv[1]) };
	if (item[0] == '&')item = fmt->format(item);
	if (item.empty()) {
		JS_ThrowSyntaxError(ctx, "prop can't be empty");
		return JS_EXCEPTION;
	}
	else if (item == "trust") {
		int trust{ (int)JS_ToInt32(ctx,&trust, 3) };
		if (trust > 4 || trust < 0)return JS_FALSE;
		User& user{ getUser(uid) };
		if (user.nTrust > 4)return JS_FALSE;
		user.trust(trust);
	}
	else if (item.find("nn#") == 0) {
		long long gid{ 0 };
		if (size_t l{ item.find_first_of(chDigit) }; l != string::npos) {
			gid = stoll(item.substr(l, item.find_first_not_of(chDigit, l) - l));
		}
		if (!JS_IsUndefined(argv[2])) {
			getUser(uid).rmNick(gid);
		}
		else {
			getUser(uid).setNick(gid, js_toAttr(ctx, argv[2]));
		}
	}
	else if (JS_IsUndefined(argv[2])) {
		getUser(uid).rmConf(item);
	}
	else getUser(uid).setConf(item, js_toAttr(ctx, argv[2]));
	return JS_TRUE;
}
QJSDEF(getUserToday) {
	string item{ js_toGBK(ctx, argv[1]) };
	if (item[0] == '&')item = fmt->format(item);
	if (JS_IsUndefined(argv[0])) {
		if (item.empty()) {
			JS_ThrowSyntaxError(ctx, "neither uid and prop is undefined");
			return JS_EXCEPTION;
		}
		auto items{ JS_NewArray(ctx) };
		for (auto& [uid, data] : today->getUserInfo()) {
			if (data->has(item))JS_SetPropertyInt64(ctx, items, (int64_t)uid, js_newAttr(ctx, data->get(item)));
		}
		return items;
	}
	if (long long uid{ js_toLongLong(ctx,argv[0]) }) {
		if (item.empty()) return js_newDiceContext(ctx, today->get(uid).p);
		else if (item == "jrrp")
			return js_newAttr(ctx, today->getJrrp(uid));
		else if (auto p{ today->get_if(uid, item) })
			return js_newAttr(ctx, *p);
	}
	JS_ThrowSyntaxError(ctx, "uid can't be 0");
	return JS_EXCEPTION;
}
QJSDEF(setUserToday) {
	if (JS_IsUndefined(argv[0])) {
		JS_ThrowSyntaxError(ctx, "uid can't be 0");
		return JS_EXCEPTION;
	}
	long long uid = js_toLongLong(ctx, argv[0]);
	string item{ js_toGBK(ctx, argv[1]) };
	if (item[0] == '&')item = fmt->format(item);
	if (item.empty()) {
		JS_ThrowSyntaxError(ctx, "prop can't be empty");
		return JS_EXCEPTION;
	}
	else if (!JS_IsUndefined(argv[2])) {
		today->set(uid, item, js_toAttr(ctx, argv[2]));
	}
	else today->set(uid, item, {});
	return JS_TRUE;
}
QJSDEF(getPlayerCard) {
	long long uid = js_toLongLong(ctx, argv[0]);
	if (uid && PList.count(uid)) {
		auto& pl{ getPlayer(uid) };
		if (auto type = JS_VALUE_GET_TAG(argv[1]); type == JS_TAG_STRING) {
			return js_newActor(ctx, pl[js_toGBK(ctx, argv[1])]);
		}
		else if (JS_IsNumber(argv[1])) {
			return js_newActor(ctx, pl[js_toLongLong(ctx, argv[1])]);
		}
		else if (JS_IsUndefined(argv[1])) {
			return js_newActor(ctx, pl[0]);
		}
		else {
			JS_ThrowTypeError(ctx, "#2 must be string or int!");
		}
	}
	else JS_ThrowTypeError(ctx, "#1 invalid user id!");
	return JS_EXCEPTION;
}
//Context
void js_dice_context_finalizer(JSRuntime* rt, JSValue val) {
	JS2OBJ(val);
	delete &obj;
}
int js_dice_context_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst this_val, JSAtom prop) {
	JS2OBJ(this_val);
	if (desc) {
		string key{ js_AtomtoGBK(ctx, prop) };
		if (key == "user" && obj->has("uid")) {
			desc->value = js_newDiceContext(ctx, getUser(obj->get_ll("uid")).shared_from_this());
		}
		else if ((key == "grp" || key == "group") && obj->has("gid")) {
			desc->value = js_newDiceContext(ctx, chat(obj->get_ll("gid")).shared_from_this());
		}
		else if (key == "game") {
			if (auto game = sessions.get_if(*obj)) {
				desc->value = js_newGameTable(ctx, game);
			}
			else return FALSE;
		}
		else if (auto val{ getContextItem(obj, key) }) {
			desc->value = js_newAttr(ctx, val);
		}
		else return FALSE;
	}
	else return FALSE;
	desc->flags = JS_PROP_C_W_E;
	desc->getter = JS_UNDEFINED;
	desc->setter = JS_UNDEFINED;
	return TRUE;
}
int js_dice_context_get_keys(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst this_val) {
	JS2OBJ(this_val);
	if ((*plen = obj->size()) > 0) {
		JSPropertyEnum* tab = (JSPropertyEnum*)js_malloc(ctx, sizeof(JSPropertyEnum) * (*plen));
		int i = 0;
		for (const auto& [key, val] : obj->as_dict()) {
			if (auto atom = JS_NewAtom(ctx, GBKtoUTF8(key).c_str());
				atom != JS_ATOM_NULL) {
				DD::debugLog("newAtom:" + js_AtomtoGBK(ctx, atom) + "#" + to_string(atom));
				tab[i++].atom = atom;
				tab[i++].is_enumerable = TRUE;
			}
			else {
				js_free_prop_enum(ctx, tab, *plen);
				*plen = 0;
				return -1;
			}
		}
		*ptab = tab;
	}
	return 0;
}
int js_dice_context_delete(JSContext* ctx, JSValue this_val, JSAtom atom) {
	JS2OBJ(this_val);
	obj->reset(js_AtomtoGBK(ctx, atom));
	return TRUE;
}
int js_dice_context_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags) {
	JS2OBJ(this_obj);
	auto str = js_AtomtoGBK(ctx, prop);
	obj->set(str, js_toAttr(ctx, val));
	return TRUE;
}
QJSDEF(context_get) {
	JS2OBJ(this_val);
	if (argc > 0) {
		string key{ js_toGBK(ctx, argv[0]) };
		if (auto item{ getContextItem(obj,key) }) {
			return js_newAttr(ctx, item);
		}
		return argc > 1 ? argv[1] : JS_UNDEFINED;
	}
	else {
		return js_newAttr(ctx, obj);
	}
}
QJSDEF(context_format) {
	JS2OBJ(this_val);
	if (argc > 0) {
		return js_newGBK(ctx, fmt->format(js_toGBK(ctx, argv[0]), obj));
	}
	else {
		JS_ThrowTypeError(ctx, "undefined field");
		return JS_EXCEPTION;
	}
}
QJSDEF(context_echo) {
	JS2OBJ(this_val);
	if (argc > 0) {
		string msg{ js_toGBK(ctx, argv[0]) };
		bool isRaw = argc > 1 ? JS_ToBool(ctx, argv[1]) : false;
		reply(obj, msg, !isRaw);
		return JS_TRUE;
	}
	else {
		JS_ThrowTypeError(ctx, "undefined msg");
		return JS_EXCEPTION;
	}
}
QJSDEF(context_inc) {
	JS2OBJ(this_val);
	if (argc > 0) {
		string key{ js_toGBK(ctx, argv[0]) };
		return obj->inc(key) ? js_newAttr(ctx, obj->get(key))
			: (argc > 1 ? argv[1] : JS_UNDEFINED);
		return argc > 1 ? obj->inc(key, js_toInt(ctx, argv[1])) : obj->inc(key);
	}
	else {
		JS_ThrowTypeError(ctx, "undefined field");
		return JS_EXCEPTION;
	}
}
//SelfData
QJSDEF(selfdata_constructor) {
	if (argc > 0) {
		string file{ js_toNativeString(ctx, argv[0]) };
		if (!selfdata_byFile.count(file)) {
			auto& data{ selfdata_byFile[file] = std::make_shared<SelfData>(DiceDir / "selfdata" / file) };
			if (string name{ cut_stem(file) }; !selfdata_byStem.count(name)) {
				selfdata_byStem[name] = data;
			}
		}
		auto obj = JS_NewObjectClass(ctx, js_dice_selfdata_id);
		ptr<SelfData>* data = new ptr<SelfData>(selfdata_byFile[file]);
		JS_SetOpaque(obj, data);
		return obj;
	}
	JS_ThrowTypeError(ctx, "undefined filename");
	return JS_EXCEPTION;
}
void js_dice_selfdata_finalizer(JSRuntime* rt, JSValue val) {
	JS2DATA(val);
	delete &data;
}
int js_dice_selfdata_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst this_val, JSAtom prop) {
	JS2DATA(this_val);
	if (data && desc) {
		string key{ js_AtomtoGBK(ctx, prop) };
		if (data->data.to_obj()->has(key)) {
			desc->flags = JS_PROP_C_W_E;
			desc->value = js_newAttr(ctx, data->data.to_obj()->get(key));
			desc->getter = JS_UNDEFINED;
			desc->setter = JS_UNDEFINED;
			return TRUE;
		}
	}
	return FALSE;
}
int js_dice_selfdata_delete(JSContext* ctx, JSValue this_val, JSAtom atom) {
	JS2DATA(this_val);
	if (data) {
		auto str = js_AtomtoGBK(ctx, atom);
		data->data.to_obj()->reset(str);
		data->save();
		return TRUE;
	}
	return FALSE;
}
int js_dice_selfdata_define(JSContext* ctx, JSValueConst this_obj,
	JSAtom prop, JSValueConst val,
	JSValueConst getter, JSValueConst setter,
	int flags) {
	JS2DATA(this_obj);
	if (data) {
		auto str = js_AtomtoGBK(ctx, prop);
		data->data.to_obj()->set(str, js_toAttr(ctx, val));
		data->save();
		return TRUE;
	}
	return FALSE;
}
int js_dice_selfdata_set(JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst value, JSValueConst receiver, int flags) {
	JS2DATA(obj);
	if (data) {
		auto str = js_AtomtoGBK(ctx, atom);
		data->data.to_obj()->set(str, js_toAttr(ctx, value));
		data->save();
		return TRUE;
	}
	return FALSE;
}
QJSDEF(selfdata_append) {
	JS2DATA(this_val);
	if (data) {
		return TRUE;
	}
	return FALSE;
}
//GameTable
void js_dice_GameTable_finalizer(JSRuntime* rt, JSValue val) {
	JS2GAME(val);
	delete &game;
}
int js_dice_GameTable_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop) {
	JS2GAME(obj);
	if (game) {
		auto key = js_AtomtoGBK(ctx, prop);
		if (key == "gms") {
			desc->value = js_newSet(ctx, game->get_gm());
			desc->flags = JS_PROP_NORMAL;
			desc->getter = JS_UNDEFINED;
			desc->setter = JS_UNDEFINED;
			return TRUE;
		}
		else if (key == "pls") {
			desc->value = js_newSet(ctx, game->get_pl());
			desc->flags = JS_PROP_NORMAL;
			desc->getter = JS_UNDEFINED;
			desc->setter = JS_UNDEFINED;
			return TRUE;
		}
		else if (key == "obs") {
			desc->value = js_newSet(ctx, game->get_ob());
			desc->flags = JS_PROP_NORMAL;
			desc->getter = JS_UNDEFINED;
			desc->setter = JS_UNDEFINED;
			return TRUE;
		}
		else if (game->has(key)) {
			desc->value = js_newAttr(ctx, game->get(key));
		}
		else return FALSE;
	}
	else return FALSE;
	desc->flags = JS_PROP_C_W_E;
	desc->getter = JS_UNDEFINED;
	desc->setter = JS_UNDEFINED;
	return TRUE;
}
int js_dice_GameTable_delete(JSContext* ctx, JSValue obj, JSAtom atom) {
	JS2GAME(obj);
	if (game) {
		auto str = js_AtomtoGBK(ctx, atom);
		game->reset(str);
		return TRUE;
	}
	return FALSE;
}
int js_dice_GameTable_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags) {
	JS2GAME(this_obj);
	if (game) {
		auto str = js_AtomtoGBK(ctx, prop);
		game->set(str, js_toAttr(ctx, val));
		return TRUE;
	}
	return FALSE;
}
QJSDEF(GameTable_message) {
	JS2GAME(this_val);
	if (string msg{ js_toGBK(ctx,argv[0]) }; !msg.empty()) {
		AddMsgToQueue(msg, *game->areas.begin());
		return JS_TRUE;
	}
	return JS_FALSE;
}
//Actor
const dict<int> prop_desc{
	{"__Name",JS_PROP_NORMAL},
	{"__Type",JS_PROP_CONFIGURABLE|JS_PROP_WRITABLE },
	{"__Update",JS_PROP_ENUMERABLE},
};
void js_dice_actor_finalizer(JSRuntime* rt, JSValue val) {
	JS2PC(val);
	delete &pc;
}
int js_dice_actor_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst this_val, JSAtom prop) {
	JS2PC(this_val);
	if (string key{ js_AtomtoGBK(ctx, prop) }; 
		desc && pc && pc->has(key = pc->standard(key))) {
		desc->flags = (prop_desc.count(key)) ? prop_desc.at(key) : JS_PROP_C_W_E;
		desc->value = js_newAttr(ctx, pc->get(key));
		desc->getter = JS_UNDEFINED;
		desc->setter = JS_UNDEFINED;
		return TRUE;
	}
	return FALSE;
}
int js_dice_actor_get_keys(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst this_val) {
	JS2PC(this_val);
	if ((*plen = pc->size() - 3) > 0) {
		JSPropertyEnum* tab = (JSPropertyEnum*)js_malloc(ctx, sizeof(JSPropertyEnum) * (*plen));
		int i = 0;
		for (const auto& [key, val] : pc->as_dict()) {
			if (prop_desc.count(key)) {
				continue;
			}
			else if (auto atom = JS_NewAtom(ctx, GBKtoUTF8(key).c_str());
				atom != JS_ATOM_NULL) {
				tab[i++].atom = atom;
				tab[i++].is_enumerable = TRUE;
			}
			else {
				js_free_prop_enum(ctx, tab, *plen);
				*plen = 0;
				return -1;
			}
		}
		*ptab = tab;
	}
	return 0;
}
int js_dice_actor_delete(JSContext* ctx, JSValue this_val, JSAtom atom) {
	JS2PC(this_val);
	string attr{ js_AtomtoGBK(ctx, atom) };
	return pc->erase(attr);
}
int js_dice_actor_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags) {
	JS2PC(this_obj);
	switch (pc->set(js_AtomtoGBK(ctx, prop), js_toAttr(ctx, val))) {
	case -11:
		JS_ThrowRangeError(ctx, "input string is too long.");
		break;
	case -8:
		JS_ThrowTypeError(ctx, "__Name cannot be modified.");
		break;
	default:
		return TRUE;
	}
	return -1;
}
QJSDEF(actor_set) {
	JS2PC(this_val);
	if (JS_IsString(argv[0])) {
		if (0 == pc->set(js_toGBK(ctx, argv[0]), js_toAttr(ctx, argv[1])))return JS_NewInt32(ctx, 1);
	}
	else if (JS_IsObject(argv[0])) {
		JSPropertyEnum* tab{ nullptr };
		uint32_t len = 0;
		int32_t cnt = 0;
		if (!JS_GetOwnPropertyNames(ctx, &tab, &len, argv[0], JS_GPN_ENUM_ONLY | JS_GPN_STRING_MASK)) {
			for (uint32_t i = 0; i < len; ++i) {
				if (0 == pc->set(js_AtomtoGBK(ctx, tab[i].atom), js_toAttr(ctx, JS_GetProperty(ctx, argv[0], tab[i].atom))))++cnt;
			}
			js_free_prop_enum(ctx, tab, len);
		}
		return JS_NewInt32(ctx, cnt);
	}
	return JS_FALSE;
}
QJSDEF(actor_rollDice) {
	JS2PC(this_val);
	auto res = JS_NewObject(ctx);
	string exp{ JS_ToBool(ctx,argv[0]) ? js_toGBK(ctx,argv[0])
		: pc->has("__DefaultDiceExp") ? pc->get("__DefaultDiceExp").to_str()
		: "D" };
	int diceFace{ pc->get("__DefaultDice").to_int() };
	RD rd{ exp, diceFace ? diceFace : 100 };
	JS_SetPropertyStr(ctx, res, "expr", js_newGBK(ctx, rd.strDice));
	if (int_errno err = rd.Roll(); !err) {
		JS_SetPropertyStr(ctx, res, "sum", JS_NewInt32(ctx, (int32_t)rd.intTotal));
		JS_SetPropertyStr(ctx, res, "expansion", JS_NewString(ctx, rd.FormCompleteString().c_str()));
	}
	else {
		JS_SetPropertyStr(ctx, res, "error", JS_NewInt32(ctx, (int32_t)err));
	}
	return res;
}
QJSDEF(actor_locked) {
	JS2PC(this_val);
	string key{ js_toGBK(ctx, argv[0]) };
	return JS_NewBool(ctx, pc->locked(key));
}
QJSDEF(actor_lock) {
	JS2PC(this_val);
	string key{ js_toGBK(ctx, argv[0]) };
	return JS_NewBool(ctx, pc->lock(key));
}
QJSDEF(actor_unlock) {
	JS2PC(this_val);
	string key{ js_toGBK(ctx, argv[0]) };
	return JS_NewBool(ctx, pc->unlock(key));
}

AttrVar js_context_eval(const std::string& s, const AttrObject& context) {
	if (auto ret{ js_main->evalStringLocal(s, "<eval>", context) }; !JS_IsException(ret)) {
		return js_main->getValue(ret);
	}
	else {
		console.log(getMsg("strSelfName") + "÷¥––js”Ôæ‰ ß∞‹:\n" + s + "\n" + js_main->getException() , 0b10);
	}
	return {};
}

bool js_call_event(const AttrObject& eve, const AttrVar& action) {
	string title{ eve->has("hook") ? eve->get_str("hook") : eve->get_str("Event") };
	string script{ action.to_str() };
	bool isFile{ action.is_character() && fmt->has_js(script) };
	if (auto ret = isFile ? js_event_pool[title].evalFileLocal(fmt->js_path(script), eve) 
		: js_event_pool[title].evalStringLocal(script, title, eve);
		JS_IsException(ret)) {
		console.log(getMsg("strSelfName") + " ¬º˛°∏" + title + "°π÷¥––js ß∞‹!\n" + js_event_pool[title].getException(), 0b10);
		return false;
	}
	return true;
}
void js_msg_call(DiceEvent* msg, const AttrVar& js) {
	string jScript{ js };
	js_context ctx;
	ctx.setContext("msg", *msg);
	JSValue ret = 0;
	if (fmt->has_js(jScript)) {
		if (JS_IsException(ret = ctx.evalFile(fmt->js_path(jScript)))) {
			console.log(getMsg("strSelfName") + "‘À––" + jScript + " ß∞‹!\n" + ctx.getException(), 0b10);
			msg->set("lang", "JavaScript");
			msg->reply(getMsg("strScriptRunErr"));
		}
		else if (auto ans{ js_toAttr(ctx, ret) }; ans.is_character())
			msg->reply(ans);
	}
	else if (!jScript.empty()) {
		if (JS_IsException(ret = ctx.evalString(jScript, msg->get_str("reply_title")))) {
			console.log(getMsg("strSelfName") + "œÏ”¶°∏" + msg->get_str("reply_title") + "°πjs”Ôæ‰ ß∞‹!\n" + ctx.getException(), 0b10);
			msg->set("lang", "JavaScript");
			msg->reply(getMsg("strScriptRunErr"));
		}
		else if (auto ans{ js_toAttr(ctx, ret) }; ans.is_character()) {
			msg->reply(ans);
		}
	}
	if(ret)JS_FreeValue(ctx, ret);
}

AttrVar js_eval_simple(const std::string& exp) {
	static js_context ctx;
	if (JSValue ret{ ctx.evalString(exp, "simple formula") }; !JS_IsException(ret)) {
		return js_toAttr(ctx, ret);
	}
	else {
		console.log(getMsg("strSelfName") + "º∆À„js”Ôæ‰" + exp + " ß∞‹!\n" + ctx.getException(), 0b10);
		return {};
	}
}