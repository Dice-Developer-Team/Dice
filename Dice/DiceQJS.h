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
	extern const JSCFunctionListEntry js_dice_funcs[3];
	extern JSClassID js_dice_context_id;
	extern JSClassDef js_dice_context_class;
	QJSDEF(context_echo);
	extern const JSCFunctionListEntry js_dice_context_proto_funcs[1];
	void js_dice_context_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_context_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_context_get_keys(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj);
	int js_dice_context_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	JSValue js_dice_context_get(JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst receiver);
	long long js_toBigInt(JSContext* ctx, JSValueConst);
	double js_toDouble(JSContext* ctx, JSValueConst);
	JSValue js_newInt64(JSContext* ctx, int64_t num); 
#ifndef FALSE
#define FALSE               0
#endif
#ifndef TRUE
#define TRUE                1
#endif
#ifdef __cplusplus
}
#endif