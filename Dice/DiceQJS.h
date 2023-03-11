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
	QJSDEF(getDiceID);
	QJSDEF(log);
	extern const JSCFunctionListEntry js_dice_funcs[2];
	extern JSClassID js_dice_context_id;
	extern JSClassDef js_dice_context_class;
	QJSDEF(context_echo);
	extern const JSCFunctionListEntry js_dice_context_proto_funcs[1];
	void js_dice_context_finalizer(JSRuntime* rt, JSValue val);
	int js_dice_context_get_own(JSContext* ctx, JSPropertyDescriptor* desc, JSValueConst obj, JSAtom prop);
	int js_dice_context_delete(JSContext* ctx, JSValue obj, JSAtom atom);
	JSValue js_dice_context_get(JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst receiver);
	//JSValue js_newException();
	//JSValue js_newUndefined();
	void js_init(JSRuntime* rt, JSContext* ctx);
	long long js_toBigInt(JSContext* ctx, JSValueConst);
	double js_toDouble(JSContext* ctx, JSValueConst);
	JSValue js_newInt64(JSContext* ctx, int64_t num);
#ifdef __cplusplus
}
#endif