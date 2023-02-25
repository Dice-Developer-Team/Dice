#pragma once
#include "quickjs.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifdef _MSC_VER
#define QJSDEF(name) __declspec(dllexport) JSValue js_dice_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
#else
#define QJSDEF(name) JSValue js_dice_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
#endif
	QJSDEF(getDiceID);
	QJSDEF(log);
	extern const JSCFunctionListEntry js_dice_funcs[2];
	long long js_toBigInt(JSContext* ctx, JSValueConst);
	double js_toDouble(JSContext* ctx, JSValueConst);
#ifdef __cplusplus
}
#endif