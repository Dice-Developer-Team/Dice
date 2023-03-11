#include <memory>
#include "filesystem.hpp"
#include "DiceJS.h"
#include "DiceQJS.h"
#include "DiceEvent.h"
#include "DiceMod.h"
#define countof(x) (sizeof(x) / sizeof((x)[0]))

JSRuntime* rt{ nullptr };
//string expImport{ "import * as dice from 'dice';\n" };
string getString(JSContext* ctx, JSValue val) {
	auto s{ JS_ToCString(ctx, val) };
	string ret{ UTF8toGBK(s) };
	JS_FreeCString(ctx, s);
	return ret;
}
js_context::js_context() : ctx(JS_NewContext(rt)) {
	js_init(rt, ctx);
	auto global = JS_GetGlobalObject(ctx);
	JS_SetPropertyFunctionList(ctx, global, js_dice_funcs, countof(js_dice_funcs));
	JS_FreeValue(ctx, global);
}
js_context::~js_context() {
	JS_FreeContext(ctx);
}
AttrVar js_context::getVal(JSValue val) {
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
		return getString(ctx, val);
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
		return JS_NewArray(ctx);
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
	auto err{ getString(ctx, e) };
	if (JS_IsNull(e) || JS_IsUndefined(e)) goto Free;
	if (JSValue stack = JS_GetPropertyStr(ctx, e, "stack"); !JS_IsException(stack)) {
		err += "\n" + getString(ctx, stack);
	}
Free:
	JS_FreeValue(ctx, e);
	return err;
}
void js_context::setContext(const std::string& name, const AttrObject& context) {
	auto obj = JS_NewObjectClass(ctx, js_dice_context_id);
	AttrObject* vars = new AttrObject(context);
	JS_SetOpaque(obj, vars);
	auto global = JS_GetGlobalObject(ctx);
	JS_SetPropertyStr(ctx, global, name.c_str(), obj);
	//JS_SetPropertyFunctionList(ctx, global, js_dice_funcs, 2);
	JS_FreeValue(ctx, global);
}
JSValue js_context::evalString(const std::string& s, const string& title) {
	string exp{ GBKtoUTF8(s) };
	return JS_Eval(ctx, exp.c_str(), exp.length(), GBKtoUTF8(title).c_str(), JS_EVAL_TYPE_GLOBAL);
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
js_context* jsMain;
void js_global_init() {
	rt = JS_NewRuntime();
	js_std_init_handlers(rt);
	/* loader for ES6 modules */
	JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
	JS_NewClassID(&js_dice_context_id);
	JS_NewClass(rt, js_dice_context_id, &js_dice_context_class); 
}
void js_global_end() {
	js_std_free_handlers(rt);
	JS_FreeRuntime(rt);
}

QJSDEF(log) {
	string info{ getString(ctx, argv[0]) };
	int note_lv{ 0 };
	for (int idx = argc; idx > 1; --idx) {
		int type{ 0 };
		if (JS_ToInt32(ctx, &type, argv[idx]))
			return JS_EXCEPTION;
		if (type >= 0 && type <10) {
			note_lv |= (1 << type);
		}
	}
	console.log(fmt->format(info), note_lv);
	return JS_UNDEFINED;
}
QJSDEF(getDiceID) {
	return JS_NewInt64(ctx, (int64_t)console.DiceMaid);
}
#define JS2OBJ(val) AttrObject* obj = (AttrObject*)JS_GetOpaque(val, js_dice_context_id)

void js_dice_context_finalizer(JSRuntime* rt, JSValue val) {
	JS2OBJ(val);
	delete obj;
}
int js_dice_context_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst this_val, JSAtom prop) {
	JS2OBJ(this_val);
	if (obj && desc) {
		auto str = JS_AtomToCString(ctx, prop);
		string key{ UTF8toGBK(str) };
		JS_FreeCString(ctx, str);
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
int js_dice_context_delete(JSContext* ctx, JSValue this_val, JSAtom atom) {
	JS2OBJ(this_val);
	if (obj) {
		auto str = JS_AtomToCString(ctx, atom);
		obj->reset(UTF8toGBK(str));
		JS_FreeCString(ctx, str);
		return TRUE;
	}
	return FALSE;
}
JSValue js_dice_context_get(JSContext* ctx, JSValueConst this_val, JSAtom atom, JSValueConst receiver) {
	JS2OBJ(this_val);
	auto str = JS_AtomToCString(ctx, atom);
	string key{ UTF8toGBK(str) };
	JS_FreeCString(ctx, str);
	return js_toValue(ctx, obj->get(key));
}
QJSDEF(context_echo) {
	JS2OBJ(this_val);
	if (!obj)return JS_FALSE;
	string msg{ getString(ctx, argv[0]) };
	bool isRaw = argc > 1 ? JS_ToBool(ctx, argv[1]) : false;
	reply(*obj, UTF8toGBK(msg), !isRaw);
	return JS_TRUE;
}

bool js_call_event(AttrObject, const AttrVar&) {
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
		else if (auto ans{ ctx.getVal(ret) }; ans.is_character())
			msg->reply(ans);
	}
	else if (!jScript.empty()) {
		if (JS_IsException(ret = ctx.evalString(jScript, msg->get_str("reply_title")))) {
			console.log(getMsg("strSelfName") + "ÏìÓ¦¡¸" + msg->get_str("reply_title") + "¡¹jsÓï¾äÊ§°Ü!\n" + ctx.getException(), 0b10);
			msg->set("lang", "JavaScript");
			msg->reply(getMsg("strScriptRunErr"));
		}
		else if (auto ans{ ctx.getVal(ret) }; ans.is_character()) {
			msg->reply(ans);
		}
	}
	if(ret)JS_FreeValue(ctx, ret);
}