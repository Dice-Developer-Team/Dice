#include <memory>
#include "filesystem.hpp"
#include "DiceJS.h"
#include "DiceQJS.h"
#include "DiceEvent.h"
#include "DiceMod.h"
#include "DiceSelfData.h"
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
	JS_FreeValue(ctx, global);
	JSValue pro_context = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, pro_context, js_dice_context_proto_funcs,
		countof(js_dice_context_proto_funcs));
	JS_SetClassProto(ctx, js_dice_context_id, pro_context);
	JSValue pro_selfdata = JS_NewObject(ctx);
	JS_SetPropertyFunctionList(ctx, pro_selfdata, js_dice_selfdata_proto_funcs,
		countof(js_dice_selfdata_proto_funcs));
	auto selfdata_ctor = JS_NewCFunction2(ctx, js_dice_selfdata_constructor, "SelfData", 1,
		JS_CFUNC_constructor, 0);
	JS_SetConstructor(ctx, selfdata_ctor, pro_selfdata);
	JS_SetClassProto(ctx, js_dice_selfdata_id, pro_selfdata);
	JS_SetPropertyStr(ctx, global, "SelfData", selfdata_ctor);
	JS_FreeValue(ctx, global);
}
js_context::~js_context() {
	JS_FreeContext(ctx);
}
AttrVar js_toAttr(JSContext* ctx, JSValue val) {
	switch (auto tag{ JS_VALUE_GET_TAG(val) }) {
	case JS_TAG_BOOL:
		return bool(JS_ToBool(ctx, val));
		break;
	case JS_TAG_INT:
		return JS_VALUE_GET_INT(val);
		break;
	case JS_TAG_BIG_INT:
		return js_toBigInt(ctx, val);
		break;
	case JS_TAG_BIG_FLOAT:
	case JS_TAG_FLOAT64:
		return js_toDouble(ctx, val);
		break;
	case JS_TAG_STRING:
		return js_toGBK(ctx, val);
		break;
	case JS_TAG_OBJECT:
		if (void* p{ nullptr }; JS_GetClassID(val, &p) == js_dice_context_id) {
			return *(AttrObject*)p;
		}
		else if (JS_IsArray(ctx, val)) {
			VarArray ary;
			auto p = JS_VALUE_GET_OBJ(val);
			return AttrObject(ary);
		}
		else {
			AttrObject obj;
			JSPropertyEnum* tab{ nullptr };
			uint32_t len = 0;
			if (!JS_GetOwnPropertyNames(ctx, &tab, &len, val, 0)) {
				for (uint32_t i = 0; i < len; ++i) {
					auto prop = JS_AtomToValue(ctx, tab[i].atom);
					if (JS_IsString(prop)) {
						obj.set(js_toGBK(ctx, prop), js_toAttr(ctx, JS_GetProperty(ctx, val, prop)));
					}
					JS_FreeValue(ctx, prop);
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
JSValue js_toValue(JSContext* ctx, const AttrVar& var) {
	switch (var.type) {
	case AttrVar::AttrType::Boolean:
		return JS_NewBool(ctx, var.bit);
		break;
	case AttrVar::AttrType::Integer:
		return JS_NewInt32(ctx, (int32_t)var.attr);
		break;
	case AttrVar::AttrType::Number:
		return JS_NewFloat64(ctx, var.number);
		break;
	case AttrVar::AttrType::Text:
		return JS_NewString(ctx, GBKtoUTF8(var.text).c_str());
		break;
	case AttrVar::AttrType::Table:
		if (!var.table.to_dict()->empty()) {
			auto dict = JS_NewObject(ctx);
			if (var.table.to_list()) {
				uint32_t idx{ 0 };
				for (auto& val : *var.table.to_list()) {
					JS_SetPropertyUint32(ctx, dict, idx++, js_toValue(ctx, val));
				}
			}
			for (auto& [key, val] : *var.table.to_dict()) {
				JS_SetPropertyStr(ctx, dict, GBKtoUTF8(key).c_str(), js_toValue(ctx, val));
			}
			return dict;
		}
		else if (var.table.to_list()) {
			auto ary = JS_NewArray(ctx);
			uint32_t idx{ 0 };
			for (auto& val : *var.table.to_list()) {
				JS_SetPropertyUint32(ctx, ary, idx++, js_toValue(ctx, val));
			}
			return ary;
		}
		break;
	case AttrVar::AttrType::ID:
		return JS_NewInt64(ctx, (int64_t)var.attr);
		break;
	case AttrVar::AttrType::Function:
	case AttrVar::AttrType::Nil:
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
JSValue js_newDiceContext(JSContext* ctx, const AttrObject& context) {
	auto obj = JS_NewObjectClass(ctx, js_dice_context_id);
	AttrObject* vars = new AttrObject(context);
	JS_SetOpaque(obj, vars);
	return obj;
}
void js_context::setContext(const std::string& name, const AttrObject& context) {
	auto global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, name.c_str(), js_newDiceContext(ctx, context));
	JS_FreeValue(ctx, global);
}
JSValue js_context::evalString(const std::string& s, const string& title) {
	string exp{ GBKtoUTF8(s) };
	return JS_Eval(ctx, exp.c_str(), exp.length(), GBKtoUTF8(title).c_str(), JS_EVAL_TYPE_GLOBAL);
}
JSValue js_context::evalStringLocal(const std::string& s, const string& title, const AttrObject& context) {
	string exp{ GBKtoUTF8(s) };
	return JS_EvalThis(ctx, js_newDiceContext(ctx, context), exp.c_str(), exp.length(), GBKtoUTF8(title).c_str(), JS_EVAL_TYPE_GLOBAL);
}
JSValue js_context::evalFile(const std::string& s) {
	size_t buf_len{ 0 };
	if (uint8_t * buf{ js_load_file(ctx, &buf_len, s.c_str()) }) {
		auto ret = JS_Eval(ctx, (char*)buf, buf_len, s.c_str(),
			JS_EVAL_TYPE_GLOBAL);
		js_free(ctx, buf);
		return ret;
	}
	return JS_EXCEPTION;
}
JSValue js_context::evalFileLocal(const std::string& s, const AttrObject& context) {
	size_t buf_len{ 0 };
	if (uint8_t * buf{ js_load_file(ctx, &buf_len, s.c_str()) }) {
		auto ret = JS_EvalThis(ctx, js_newDiceContext(ctx,context), (char*)buf, buf_len, s.c_str(),
			JS_EVAL_TYPE_GLOBAL);
		js_free(ctx, buf);
		return ret;
	}
	return JS_EXCEPTION;
}
dict<js_context> js_event_pool;
void js_global_init() {
	rt = JS_NewRuntime();
	js_std_init_handlers(rt);
	/* loader for ES6 modules */
	JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
	JS_NewClassID(&js_dice_context_id);
	JS_NewClass(rt, js_dice_context_id, &js_dice_context_class);
	JS_NewClassID(&js_dice_selfdata_id);
	JS_NewClass(rt, js_dice_selfdata_id, &js_dice_selfdata_class);
}
void js_global_end() {
	js_std_free_handlers(rt);
	JS_FreeRuntime(rt);
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
		long long gid{ JS_IsUndefined(argv[1]) ? 0 : js_toBigInt(ctx, argv[1]) },
			uid{ JS_IsUndefined(argv[2]) ? 0 : js_toBigInt(ctx, argv[2]) };
		eve = gid
			? AttrVars{ {"fromMsg",fromMsg},{"gid",gid}, {"uid", uid} }
		: AttrVars{ {"fromMsg",fromMsg}, {"uid", uid} };
	}
	std::thread th([=]() {
		DiceEvent(eve).virtualCall();
		});
	th.detach();
	return JS_TRUE;
}
QJSDEF(sendMsg) {
	AttrObject chat;
	if (JS_IsObject(argv[0])) {
		chat = js_toAttr(ctx, argv[0]).to_obj();
		AddMsgToQueue(fmt->format(chat.get_str("fwdMsg"), chat),
			chatInfo{ chat.get_ll("uid") ,chat.get_ll("gid") ,chat.get_ll("chid") });
	}
	else {
		long long gid{ JS_IsUndefined(argv[1]) ? 0 : js_toBigInt(ctx, argv[1]) },
			uid{ JS_IsUndefined(argv[2]) ? 0 : js_toBigInt(ctx, argv[2]) };
		if (!gid && !uid) {
			JS_ThrowSyntaxError(ctx, "gid and uid are both 0 is invalid");
			return JS_EXCEPTION;
		}
		string msg{ js_toGBK(ctx, argv[0]) };
		if (uid)chat.set("uid", uid);
		if (gid)chat.set("gid", gid);
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
			if (data.confs.has(item))JS_SetPropertyInt64(ctx, items, (int64_t)gid, js_toValue(ctx, data.confs.get(item)));
		}
		return items;
	}
	if (long long gid{ js_toBigInt(ctx,argv[0]) }) {
		if (item.empty()) return js_newDiceContext(ctx, chat(gid).confs);
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
		if (auto val{ getGroupItem(gid,item) }; !val.is_null())return js_toValue(ctx, val);
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
	long long gid = js_toBigInt(ctx, argv[0]);
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
			if (data.confs.has(item))JS_SetPropertyInt64(ctx, items, (int64_t)id, js_toValue(ctx, data.confs.get(item)));
		}
		return items;
	}
	if (long long uid{ js_toBigInt(ctx,argv[0]) }) {
		if (item.empty()) return js_newDiceContext(ctx, getUser(uid).confs);
		if (auto val{ getUserItem(uid,item) }; !val.is_null())return js_toValue(ctx, val);
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
	long long uid = js_toBigInt(ctx, argv[0]);
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
			if (data.has(item))JS_SetPropertyInt64(ctx, items, (int64_t)uid, js_toValue(ctx, data.get(item)));
		}
		return items;
	}
	if (long long uid{ js_toBigInt(ctx,argv[0]) }) {
		if (item.empty()) return js_newDiceContext(ctx,today->get(uid));
		else if (item == "jrrp")
			return JS_NewInt32(ctx, (int32_t)today->getJrrp(uid));
		else if (auto p{ today->get_if(uid, item) })
			return js_toValue(ctx, *p);
	}
	JS_ThrowSyntaxError(ctx, "uid can't be 0");
	return JS_EXCEPTION;
}
QJSDEF(setUserToday) {
	if (JS_IsUndefined(argv[0])) {
		JS_ThrowSyntaxError(ctx, "uid can't be 0");
		return JS_EXCEPTION;
	}
	long long uid = js_toBigInt(ctx, argv[0]);
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

void js_dice_context_finalizer(JSRuntime* rt, JSValue val) {
	JS2OBJ(val);
	delete obj;
}
int js_dice_context_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst this_val, JSAtom prop) {
	JS2OBJ(this_val);
	if (obj && desc) {
		string key{ js_AtomtoGBK(ctx, prop) };
		if (obj->has(key)) {
			desc->flags = JS_PROP_NORMAL;
			desc->value = js_toValue(ctx, obj->get(key));
			desc->getter = JS_UNDEFINED;
			desc->setter = JS_UNDEFINED;
			return TRUE;
		}
	}
	return FALSE;
}
int js_dice_context_get_keys(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst this_val) {
	JS2OBJ(this_val);
	if (!obj)return -1;
	if ((*plen = obj->size()) > 0) {
		JSPropertyEnum* tab = (JSPropertyEnum*)js_malloc(ctx, sizeof(JSPropertyEnum) * (*plen));
		for (const auto& [key, val] : *obj->to_dict()) {
			if (auto atom = JS_NewAtom(ctx, GBKtoUTF8(key).c_str());
				atom != JS_ATOM_NULL) {
				tab[*plen].atom = atom;
				tab[*plen].is_enumerable = FALSE;
			}
			else {
				js_free_prop_enum(ctx, tab, *plen);
				*plen = 0;
				return -1;
			}
		}
		js_free_prop_enum(ctx, tab, *plen);
		ptab = &tab;
	}
	return 0;
}
int js_dice_context_delete(JSContext* ctx, JSValue this_val, JSAtom atom) {
	JS2OBJ(this_val);
	if (obj) {
		obj->reset(js_AtomtoGBK(ctx, atom));
		return TRUE;
	}
	return FALSE;
}
int js_dice_context_define(JSContext* ctx, JSValueConst this_obj, JSAtom prop, JSValueConst val, JSValueConst getter, JSValueConst setter, int flags) {
	JS2OBJ(this_obj);
	if (obj) {
		auto str = js_AtomtoGBK(ctx, prop);
		obj->set(str, js_toAttr(ctx, val));
		return TRUE;
	}
	return FALSE;
}
QJSDEF(context_get) {
	JS2OBJ(this_val);
	if (argc > 0) {
		string key{ js_toGBK(ctx, argv[0]) };
		return obj->has(key) ? js_toValue(ctx, obj->get(key))
			: (argc > 1 ? argv[1] : JS_UNDEFINED);
	}
	else {
		return js_toValue(ctx, *obj);
	}
}
QJSDEF(context_format) {
	JS2OBJ(this_val);
	if (argc > 0) {
		return js_newGBK(ctx, fmt->format(js_toGBK(ctx, argv[0]), *obj));
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
		reply(*obj, msg, !isRaw);
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
		return obj->inc(key) ? js_toValue(ctx, obj->get(key))
			: (argc > 1 ? argv[1] : JS_UNDEFINED);
		return argc > 1 ? obj->inc(key, js_toInt(ctx, argv[1])) : obj->inc(key);
	}
	else {
		JS_ThrowTypeError(ctx, "undefined field");
		return JS_EXCEPTION;
	}
}
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
	delete data;
}
int js_dice_selfdata_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst this_val, JSAtom prop) {
	JS2DATA(this_val);
	if (data && desc) {
		string key{ js_AtomtoGBK(ctx, prop) };
		if ((*data)->data.to_dict()->count(key)) {
			desc->flags = JS_PROP_NORMAL;
			desc->value = js_toValue(ctx, (*data)->data.to_obj().get(key));
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
		(*data)->data.to_obj().reset(str);
		(*data)->save();
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
		(*data)->data.to_obj().set(str, js_toAttr(ctx, val));
		(*data)->save();
		return TRUE;
	}
	return FALSE;
}
int js_dice_selfdata_set(JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst value, JSValueConst receiver, int flags) {
	JS2DATA(obj);
	if (data) {
		auto str = js_AtomtoGBK(ctx, atom);
		(*data)->data.to_obj().set(str, js_toAttr(ctx, value));
		(*data)->save();
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

bool js_call_event(AttrObject eve, const AttrVar& action) {
	string title{ eve.has("hook") ? eve.get_str("hook") : eve.get_str("Event") };
	string script{ action.to_str() };
	bool isFile{ action.is_character() && fmt->has_js(script) };
	if (auto ret = isFile ? js_event_pool[title].evalFileLocal(fmt->py_path(script), eve) 
		: js_event_pool[title].evalStringLocal(script, title, eve);
		JS_IsException(ret)) {
		console.log(getMsg("strSelfName") + "ÊÂ¼þ¡¸" + title + "¡¹Ö´ÐÐjsÊ§°Ü!\n" + js_event_pool[title].getException(), 0b10);
		return false;
	}
	return true;
}
void js_msg_call(DiceEvent* msg, const AttrObject& js) {
	string jScript{ js.get_str("script") };
	js_context ctx;
	ctx.setContext("msg", *msg);
	JSValue ret = 0;
	if (fmt->has_js(jScript)) {
		if (JS_IsException(ret = ctx.evalFile(fmt->js_path(jScript)))) {
			console.log(getMsg("strSelfName") + "ÔËÐÐ" + jScript + "Ê§°Ü!\n" + ctx.getException(), 0b10);
			msg->set("lang", "JavaScript");
			msg->reply(getMsg("strScriptRunErr"));
		}
		else if (auto ans{ js_toAttr(ctx, ret) }; ans.is_character())
			msg->reply(ans);
	}
	else if (!jScript.empty()) {
		if (JS_IsException(ret = ctx.evalString(jScript, msg->get_str("reply_title")))) {
			console.log(getMsg("strSelfName") + "ÏìÓ¦¡¸" + msg->get_str("reply_title") + "¡¹jsÓï¾äÊ§°Ü!\n" + ctx.getException(), 0b10);
			msg->set("lang", "JavaScript");
			msg->reply(getMsg("strScriptRunErr"));
		}
		else if (auto ans{ js_toAttr(ctx, ret) }; ans.is_character()) {
			msg->reply(ans);
		}
	}
	if(ret)JS_FreeValue(ctx, ret);
}