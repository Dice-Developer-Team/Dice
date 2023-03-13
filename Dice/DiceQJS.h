#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs-libc.h"
#ifdef _MSC_VER
#define QJSDEF(name) __declspec(dllexport) JSValue js_dice_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
#else
#define QJSDEF(name) JSValue js_dice_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
#endif
	QJSDEF(log);
	QJSDEF(getDiceID);
	QJSDEF(getDiceDir);
	QJSDEF(eventMsg);
	QJSDEF(sendMsg);
	QJSDEF(getGroupAttr);
	QJSDEF(setGroupAttr);
	QJSDEF(getUserAttr);
	QJSDEF(setUserAttr);
	QJSDEF(getUserToday);
	QJSDEF(setUserToday);
	extern const JSCFunctionListEntry js_dice_funcs[11];
	extern JSClassID js_dice_context_id;
	extern JSClassDef js_dice_context_class;
	void js_dice_context_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_context_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_context_get_keys(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj);
	int js_dice_context_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	QJSDEF(context_get);
	QJSDEF(context_format);
	QJSDEF(context_echo);
	QJSDEF(context_inc);
	extern const JSCFunctionListEntry js_dice_context_proto_funcs[4];
	long long js_toBigInt(JSContext* ctx, JSValueConst);
	double js_toDouble(JSContext* ctx, JSValueConst);
#ifndef FALSE
#define FALSE               0
#endif
#ifndef TRUE
#define TRUE                1
#endif
#ifdef __cplusplus
#define JS2OBJ(val) AttrObject* obj = (AttrObject*)JS_GetOpaque(val, js_dice_context_id)
}
#endif