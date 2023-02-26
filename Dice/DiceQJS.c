#define CONFIG_BIGNUM
#include "quickjs-libc.h"
#include "DiceQJS.h"

const JSCFunctionListEntry js_dice_funcs[] = {
	JS_CFUNC_DEF("log",0,js_dice_log),
	JS_CFUNC_DEF("getDiceID",0,js_dice_getDiceID),
};

long long js_toBigInt(JSContext* ctx, JSValueConst val) {
	int64_t num = 0;
	JS_ToBigInt64(ctx, &num, val);
	return (long long)num;
}
double js_toDouble(JSContext* ctx, JSValueConst val) {
	double num = 0;
	JS_ToFloat64(ctx, &num, val);
	return num;
}
JSValue js_newException() {
	return JS_EXCEPTION;
}
JSValue js_newUndefined() {
	return JS_UNDEFINED;
}