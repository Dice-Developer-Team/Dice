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
	JSValue js_newException();
	JSValue js_newUndefined();
	void js_init(JSRuntime* rt, JSContext* ctx);
	long long js_toBigInt(JSContext* ctx, JSValueConst);
	double js_toDouble(JSContext* ctx, JSValueConst);
	JSValue js_newInt64(JSContext* ctx, int64_t num);
#ifdef __cplusplus
}
#endif