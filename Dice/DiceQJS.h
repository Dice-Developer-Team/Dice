#pragma once
#include "quickjs.h"
#ifdef __cplusplus
extern "C" {
#endif
#define QJSDEF(name) __declspec(dllexport) JSValue js_dice_##name(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	QJSDEF(getDiceID);
	QJSDEF(log);
	extern const JSCFunctionListEntry js_dice_funcs[2];
#ifdef __cplusplus
}
#endif