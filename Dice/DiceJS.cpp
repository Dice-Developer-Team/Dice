#include <memory>
#include "filesystem.hpp"
#include "DiceJS.h"
#include "DiceQJS.h"
#include "DiceEvent.h"
#include "DiceMod.h"

string getString(JSContext* ctx, JSValue val) {
	auto s{ JS_ToCString(ctx, val) };
	string ret{ UTF8toGBK(s) };
	JS_FreeCString(ctx, s);
	return ret;
}
class js_runtime {
	JSRuntime* rt;
	JSContext* ctx;
public:
	js_runtime() :rt(JS_NewRuntime()), ctx(JS_NewContext(rt)) {
		js_init(rt, ctx);
		//string expImport{ "import * as dice from 'dice'" };
		//JS_Eval(ctx, expImport.c_str(), expImport.length(), "", JS_EVAL_TYPE_MODULE|JS_EVAL_TYPE_INDIRECT);
	}
	~js_runtime() {
		js_std_free_handlers(rt);
	}
	AttrVar getVal(JSValue val) {
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
		case JS_TAG_NULL:
		case JS_TAG_EXCEPTION:
			return {};
		case JS_TAG_UNDEFINED:
		default:
			console.log("js_value_tag:" + to_string(JS_VALUE_GET_TAG(val)), 0);
			return {};
		}
		return {};
	}
	string getException() {
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
	//int runFile(const std::filesystem::path&);
	bool execFile(const std::string& s, const AttrObject& context) {
		return true;
	}
	JSValue evalString(const std::string& s, const AttrObject & = {}) {
		string exp{ GBKtoUTF8(s) };
		return JS_Eval(ctx, exp.c_str(), exp.length(), "", JS_EVAL_TYPE_MODULE);
	}
};
QJSDEF(log) {
	string info{ getString(ctx, argv[0]) };
	int note_lv{ 0 };
	for (int idx = argc; idx > 1; --idx) {
		int type{ 0 };
		if (JS_ToInt32(ctx, &type, argv[idx]))
			return js_newException();
		if (type >= 0 && type <10) {
			note_lv |= (1 << type);
		}
	}
	console.log(fmt->format(info), note_lv);
	return js_newUndefined();
}
QJSDEF(getDiceID) {
	return JS_NewInt64(ctx, (int64_t)console.DiceMaid);
}

bool js_call_event(AttrObject, const AttrVar&) {
	return true;
}
bool js_msg_call(DiceEvent* msg, const AttrObject& js) {
	string jsFile{ js.get_str("file") };
	string jScript{ js.get_str("script") };
	if (jsFile.empty() && fmt->has_js(jScript)) {
		jsFile = fmt->js_path(jScript);
	}
	js_runtime rt;
	//lua_push_Context(L, *msg);
	if (!jsFile.empty()) {
		if (JSValue val{ rt.execFile(jsFile, *msg) }; JS_IsException(val)) {
			console.log(getMsg("strSelfName") + "运行js文件" + jsFile + "失败!\n" + rt.getException(), 0b10);
			msg->set("lang", "JavaScript");
			msg->reply(getMsg("strScriptRunErr"));
			return false;
		}
		else if (auto ret{ rt.getVal(val) }; ret.is_character())
			msg->reply(ret);
		return true;
	}
	else if (!jScript.empty()) {
		if (JSValue val{ rt.evalString(jScript, *msg) };JS_IsException(val)) {
			console.log(getMsg("strSelfName") + "执行js表达式" + jScript + "失败!\n" + rt.getException(), 0b10);
			msg->set("lang", "JavaScript");
			msg->reply(getMsg("strScriptRunErr"));
			return false;
		}
		else if (auto ret{ rt.getVal(val) }; ret.is_character())
			msg->reply(ret);
		return true;
	}
	return false;
}